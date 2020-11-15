//==============================================================================
//
//  CodeFile.cpp
//
//  Copyright (C) 2013-2020  Greg Utas
//
//  This file is part of the Robust Services Core (RSC).
//
//  RSC is free software: you can redistribute it and/or modify it under the
//  terms of the GNU General Public License as published by the Free Software
//  Foundation, either version 3 of the License, or (at your option) any later
//  version.
//
//  RSC is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
//  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
//  details.
//
//  You should have received a copy of the GNU General Public License along
//  with RSC.  If not, see <http://www.gnu.org/licenses/>.
//
#include "CodeFile.h"
#include <cctype>
#include <cstdio>
#include <cstring>
#include <istream>
#include <iterator>
#include <list>
#include <sstream>
#include <utility>
#include "Algorithms.h"
#include "CliThread.h"
#include "CodeCoverage.h"
#include "CodeDir.h"
#include "CodeFileSet.h"
#include "CodeWarning.h"
#include "Cxx.h"
#include "CxxArea.h"
#include "CxxDirective.h"
#include "CxxExecute.h"
#include "CxxNamed.h"
#include "CxxRoot.h"
#include "CxxScope.h"
#include "CxxScoped.h"
#include "CxxToken.h"
#include "Debug.h"
#include "Editor.h"
#include "Formatters.h"
#include "FunctionName.h"
#include "Library.h"
#include "Parser.h"
#include "Registry.h"
#include "SetOperations.h"
#include "Singleton.h"
#include "SysFile.h"

using namespace NodeBase;
using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace CodeTools
{
void AddForwardDependencies(const CxxUsageSets& symbols, CxxNamedSet& inclSet)
{
   Debug::ft("CodeTools.AddForwardDependencies");

   //  SYMBOLS is the usage information for the symbols that appeared in
   //  this file.  An #include should appear for a forward declaration that
   //  resolved an indirect reference in this file.  Omit the #include,
   //  however, if the declaration appears in a file that defines one of
   //  our indirect base classes.
   //
   for(auto f = symbols.forwards.cbegin(); f != symbols.forwards.cend(); ++f)
   {
      auto fid = (*f)->GetDeclFid();
      auto include = true;

      for(auto b = symbols.bases.cbegin(); b != symbols.bases.cend(); ++b)
      {
         auto base = static_cast< const Class* >(*b);

         for(auto c = base->BaseClass(); c != nullptr; c = c->BaseClass())
         {
            if(c->GetDeclFid() == fid)
            {
               include = false;
               break;
            }
         }

         if(!include) break;
      }

      if(include) inclSet.insert(*f);
   }
}

//------------------------------------------------------------------------------

void DisplayFileNames(ostream* stream, const SetOfIds& fids, fixed_string title)
{
   //  Display, in STREAM, the names of files identified in FIDS.
   //  TITLE provides an explanation for the list.
   //
   if(stream == nullptr) return;
   if(fids.empty()) return;

   auto& files = Singleton< Library >::Instance()->Files();
   *stream << spaces(3) << title << CRLF;

   for(auto a = fids.cbegin(); a != fids.cend(); ++a)
   {
      *stream << spaces(6) << files.At(*a)->Name() << CRLF;
   }
}

//------------------------------------------------------------------------------

void DisplaySymbols
   (ostream* stream, const CxxNamedSet& items, fixed_string title)
{
   //  Display, in STREAM, the names in ITEMS, including their scope.
   //  TITLE provides an explanation for the list.  Put the symbols in
   //  a stringSet so that they will always appear in the same order.
   //
   if(stream == nullptr) return;
   if(items.empty()) return;
   *stream << spaces(3) << title << CRLF;

   stringSet names;

   for(auto a = items.cbegin(); a != items.cend(); ++a)
   {
      names.insert((*a)->ScopedName(true));
   }

   for(auto n = names.cbegin(); n != names.cend(); ++n)
   {
      *stream << spaces(6) << *n << CRLF;
   }
}

//------------------------------------------------------------------------------

void DisplaySymbolsAndFiles
   (ostream* stream, const CxxNamedSet& set, const string& title)
{
   //  Display, in STREAM, the symbols in SET and where they are defined.
   //  Include TITLE, which describes the contents of SET.  Put the symbols
   //  in a stringSet so that they will always appear in the same order.
   //
   if(stream == nullptr) return;
   if(set.empty()) return;
   *stream << spaces(3) << title << CRLF;

   stringSet names;

   for(auto i = set.cbegin(); i != set.cend(); ++i)
   {
      auto name = (*i)->XrefName(true);
      auto file = (*i)->GetFile();

      if(file != nullptr)
         name += " [" + file->Name() + ']';
      else
         name += " [file unknown]";

      names.insert(name);
   }

   for(auto n = names.cbegin(); n != names.cend(); ++n)
   {
      *stream << spaces(6) << *n << CRLF;
   }
}

//------------------------------------------------------------------------------

void FindForwardCandidates(const CxxUsageSets& symbols, CxxNamedSet& addForws)
{
   Debug::ft("CodeTools.FindForwardCandidates");

   //  A forward declaration may be required for a type that was referenced
   //  indirectly.
   //
   for(auto i = symbols.indirects.cbegin(); i != symbols.indirects.cend(); ++i)
   {
      addForws.insert(*i);
   }

   //  A forward declaration may be required for a type that was resolved by
   //  a friend, rather than a forward, declaration.
   //
   for(auto f = symbols.friends.cbegin(); f != symbols.friends.cend(); ++f)
   {
      auto r = (*f)->Referent();
      if(r != nullptr) addForws.insert(r);
   }
}

//------------------------------------------------------------------------------

void GetTransitiveBases(const CxxNamedSet& bases, SetOfIds& tBaseIds)
{
   Debug::ft("CodeTools.GetTransitiveBases");

   for(auto b = bases.cbegin(); b != bases.cend(); ++b)
   {
      auto base = static_cast< const Class* >(*b);

      for(auto c = base; c != nullptr; c = c->BaseClass())
      {
         tBaseIds.insert(c->GetDeclFid());
      }
   }
}

//------------------------------------------------------------------------------

bool IsSortedByPosition(const Function* func1, const Function* func2)
{
   if(func1->GetPos() < func2->GetPos()) return true;
   if(func1->GetPos() > func2->GetPos()) return false;
   return (func1 < func2);
}

//------------------------------------------------------------------------------

char LastCodeChar(const string& s, size_t slashSlashPos)
{
   //  S is a line of source code, and slashSlashPos is the location of any
   //  trailing comment on that line.  If there is no trailing comment,
   //  return the last character, else return the first non-blank character
   //  before the comment.
   //
   if(slashSlashPos == string::npos) return s.back();

   auto pos = RfindFirstNotOf(s, slashSlashPos - 1, WhitespaceChars);
   return s.at(pos);
}

//------------------------------------------------------------------------------

void RemoveAliasedClasses(CxxNamedSet& inclSet)
{
   Debug::ft("CodeTools.RemoveAliasedClasses");

   //  Look at all pairs of items in inclSet, whose files will be #included
   //  by this file.  If one item in the pair is a class and the other item
   //  is a typedef for it, an #include for the class is not required.
   //
   for(auto item1 = inclSet.begin(); item1 != inclSet.end(); NO_OP)
   {
      auto erase1 = false;
      auto cls = (*item1)->GetClass();

      if(cls != nullptr)
      {
         for(auto item2 = std::next(item1); item2 != inclSet.end(); ++item2)
         {
            if((*item2)->Type() == Cxx::Typedef)
            {
               auto type = static_cast< const Typedef* >(*item2);
               auto ref = type->Referent();

               if((cls == ref) || (cls == ref->GetTemplate()) ||
                  (cls->GetTemplate() == ref))
               {
                  erase1 = true;
                  break;
               }
            }
         }
      }

      if(erase1)
         inclSet.erase(*item1++);
      else
         ++item1;
   }

   for(auto item1 = inclSet.begin(); item1 != inclSet.end(); ++item1)
   {
      if((*item1)->Type() == Cxx::Typedef)
      {
         auto type = static_cast< const Typedef* >(*item1);
         auto ref = type->Referent();

         for(auto item2 = std::next(item1); item2 != inclSet.end(); NO_OP)
         {
            auto erase2 = false;
            auto cls = (*item2)->GetClass();

            if(cls != nullptr)
            {
               if((cls == ref) || (cls == ref->GetTemplate()) ||
                  (cls->GetTemplate() == ref))
               {
                  erase2 = true;
               }
            }

            if(erase2)
               inclSet.erase(*item2++);
            else
               ++item2;
         }
      }
   }
}

//------------------------------------------------------------------------------

void RemoveIncludedBaseItems(CxxNamedSet& inclSet)
{
   Debug::ft("CodeTools.RemoveIncludedBaseItems");

   //  Update inclSet by removing types defined in a base class of another
   //  item in inclSet.  An #include is not needed for such a type.
   //
   for(auto item1 = inclSet.begin(); item1 != inclSet.end(); NO_OP)
   {
      auto erase1 = false;
      auto cls1 = (*item1)->GetClass();

      if(cls1 != nullptr)
      {
         for(auto item2 = std::next(item1); item2 != inclSet.end(); NO_OP)
         {
            auto erase2 = false;
            auto cls2 = (*item2)->GetClass();

            if(cls2 != nullptr)
            {
               if(cls2->DerivesFrom(cls1))
               {
                  erase1 = true;
                  break;
               }

               erase2 = cls1->DerivesFrom(cls2);
            }

            if(erase2)
               inclSet.erase(*item2++);
            else
               ++item2;
         }
      }

      if(erase1)
         inclSet.erase(*item1++);
      else
         ++item1;
   }
}

//------------------------------------------------------------------------------

void RemoveIndirectBaseItems(const CxxNamedSet& bases, CxxNamedSet& inclSet)
{
   Debug::ft("CodeTools.RemoveIndirectBaseItems");

   //  Update inclSet by removing types defined in indirect base classes
   //  of BASES, which are the base classes implemented in this file.
   //
   for(auto item1 = inclSet.begin(); item1 != inclSet.end(); NO_OP)
   {
      auto erase = false;
      auto cls1 = (*item1)->GetClass();

      if(cls1 != nullptr)
      {
         for(auto b = bases.cbegin(); b != bases.cend(); ++b)
         {
            auto base = (*b)->GetClass();

            if(base->DerivesFrom(cls1))
            {
               erase = true;
               break;
            }
         }
      }

      if(erase)
         inclSet.erase(*item1++);
      else
         ++item1;
   }
}

//==============================================================================

CodeFile::CodeFile(const string& name, CodeDir* dir) : LibraryItem(name),
   dir_(dir),
   isHeader_(false),
   isSubsFile_(false),
   slashAsterisk_(false),
   parsed_(Unparsed),
   checked_(false)
{
   Debug::ft("CodeFile.ctor");

   isHeader_ = (name.find(".c") == string::npos);
   isSubsFile_ = (dir != nullptr) && dir->IsSubsDir();
   Singleton< Library >::Instance()->AddFile(*this);
   CxxStats::Incr(CxxStats::CODE_FILE);
}

//------------------------------------------------------------------------------

CodeFile::~CodeFile()
{
   Debug::ftnt("CodeFile.dtor");

   CxxStats::Decr(CxxStats::CODE_FILE);
}

//------------------------------------------------------------------------------

void CodeFile::AddDirectTypes
   (const CxxNamedSet& directs, CxxNamedSet& inclSet) const
{
   Debug::ft("CodeFile.AddDirectTypes");

   //  DIRECTS contains types that were used directly.  Types in executable
   //  code are also considered to be used directly, except for terminals
   //  and types defined in this file.
   //
   for(auto d = directs.cbegin(); d != directs.cend(); ++d)
   {
      inclSet.insert(*d);
   }

   for(auto u = usages_.cbegin(); u != usages_.cend(); ++u)
   {
      if((*u)->GetFile() == this) continue;
      if((*u)->Type() == Cxx::Terminal) continue;
      inclSet.insert(*u);
   }
}

//------------------------------------------------------------------------------

void CodeFile::AddIncludeIds
   (const CxxNamedSet& inclSet, SetOfIds& inclIds) const
{
   Debug::ft("CodeFile.AddIncludeIds");

   auto thisFid = Fid();

   for(auto n = inclSet.cbegin(); n != inclSet.cend(); ++n)
   {
      auto fid = (*n)->GetDeclFid();
      if((fid != NIL_ID) && (fid != thisFid)) inclIds.insert(fid);
   }
}

//------------------------------------------------------------------------------

void CodeFile::AddIndirectExternalTypes
   (const CxxNamedSet& indirects, CxxNamedSet& inclSet) const
{
   Debug::ft("CodeFile.AddIndirectExternalTypes");

   //  INDIRECTS contains types that were used indirectly.  Filter out those
   //  which are terminals (for which an #include is not required) or that
   //  are defined in the code base (for which an #include can be avoided
   //  by using a forward declaration).
   //
   for(auto i = indirects.cbegin(); i != indirects.cend(); ++i)
   {
      auto type = (*i)->Type();
      if(type == Cxx::Terminal) continue;
      if((type == Cxx::Class) && !(*i)->GetFile()->IsSubsFile()) continue;
      inclSet.insert(*i);
   }
}

//------------------------------------------------------------------------------

void CodeFile::AddToXref() const
{
   for(auto i = items_.cbegin(); i != items_.cend(); ++i)
   {
      (*i)->AddToXref();
   }
}

//------------------------------------------------------------------------------

void CodeFile::AddUsage(const CxxNamed* item)
{
   Debug::ft("CodeFile.AddUsage");

   usages_.insert(item);
}

//------------------------------------------------------------------------------

void CodeFile::AddUser(const CodeFile* file)
{
   Debug::ft("CodeFile.AddUser");

   userIds_.insert(file->Fid());
}

//------------------------------------------------------------------------------

const SetOfIds& CodeFile::Affecters() const
{
   Debug::ft("CodeFile.Affecters");

   //  If affecterIds_ is empty, build it.
   //
   if(!affecterIds_.empty()) return affecterIds_;

   auto fileSet = new CodeFileSet(LibrarySet::TemporaryName(), nullptr);
   auto& context = fileSet->Set();
   context.insert(Fid());
   auto asSet = fileSet->Affecters();
   affecterIds_ = static_cast< CodeFileSet* >(asSet)->Set();
   fileSet->Release();
   return affecterIds_;
}

//------------------------------------------------------------------------------

size_t CodeFile::CalcGroup(const CodeFile* file) const
{
   Debug::ft("CodeFile.CalcGroup(file)");

   if(file == nullptr) return 0;
   auto ext = file->IsSubsFile();
   auto fid = file->Fid();
   if(declIds_.find(fid) != declIds_.cend()) return (ext ? 1 : 2);
   if(baseIds_.find(fid) != baseIds_.cend()) return (ext ? 3 : 4);
   return (ext ? 5 : 6);
}

//------------------------------------------------------------------------------

size_t CodeFile::CalcGroup(const string& fn) const
{
   Debug::ft("CodeFile.CalcGroup(fn)");

   return CalcGroup(Singleton< Library >::Instance()->FindFile(fn));
}

//------------------------------------------------------------------------------

size_t CodeFile::CalcGroup(const Include& incl) const
{
   Debug::ft("CodeFile.CalcGroup(incl)");

   return CalcGroup(incl.FindFile());
}

//------------------------------------------------------------------------------

bool CodeFile::CanBeTrimmed() const
{
   Debug::ft("CodeFile.CanBeTrimmed");

   //  Don't trim unparsed files, empty files; or substitute files.
   //
   if(parsed_ == Unparsed) return false;
   if(code_.empty()) return false;
   if(isSubsFile_) return false;
   return true;
}

//------------------------------------------------------------------------------

ptrdiff_t CodeFile::CellDiff()
{
   uintptr_t local;
   auto fake = reinterpret_cast< const CodeFile* >(&local);
   return ptrdiff(&fake->fid_, fake);
}

//------------------------------------------------------------------------------

void CodeFile::Check()
{
   Debug::ft("CodeFile.Check");

   //  We don't do a Debug::Progress on our file name, because Trim
   //  (invoked below) already does it.
   //
   if(checked_) return;

   //  Don't check an empty file or a substitute file.
   //
   if(code_.empty() || isSubsFile_)
   {
      checked_ = true;
      return;
   }

   Debug::Progress(Name() + CRLF);
   Trim(nullptr);
   CheckProlog();
   CheckIncludeGuard();
   CheckUsings();
   CheckSeparation();
   CheckLineBreaks();
   CheckFunctionOrder();
   CheckDebugFt();
   CheckIncludes();
   CheckIncludeOrder();
   checked_ = true;
}

//------------------------------------------------------------------------------

void CodeFile::CheckDebugFt() const
{
   Debug::ft("CodeFile.CheckDebugFt");

   auto cover = Singleton< CodeCoverage >::Instance();
   size_t begin, end;
   string statement;
   string dname;
   string fname;

   //  For each function in this file, find the lines on which it begins
   //  and ends.  Within those lines, look for invocations of Debug::ft.
   //  When one is found, find the data member that is passed to Debug::ft
   //  and see if it defines a string literal that accurately identifies
   //  the function.  Also note an invocation of Debug::ft that is missing
   //  or that is not the first line of code in the function.
   //
   for(auto f = funcs_.cbegin(); f != funcs_.cend(); ++f)
   {
      //  The function must have an implementation to be checked.  A function
      //  in a header is only expected to invoke Debug::ft if it's part of a
      //  template; checking functions in a template instance would therefore
      //  be redundant.
      //
      if((*f)->GetRange(begin, end) == string::npos) continue;
      if(IsHeader() && ((*f)->GetTemplateType() == NonTemplate)) return;
      if((*f)->IsInTemplateInstance()) continue;

      auto last = lexer_.GetLineNum(end);
      auto open = false, debug = false, code = false;
      std::ostringstream source;
      (*f)->Display(source, EMPTY_STR, Code_Mask);
      auto hash = string_hash(source.str().c_str());

      for(auto n = lexer_.GetLineNum(begin); n < last; ++n)
      {
         switch(lineType_[n])
         {
         case OpenBrace:
            open = true;
            break;

         case DebugFt:
            if(code && !debug) LogLine(n, DebugFtNotFirst);
            debug = true;

            if(lexer_.GetNthLine(n, statement))
            {
               auto lpar = statement.find('(');
               if(lpar == string::npos) break;
               auto rpar = statement.find(')', lpar);
               if(rpar == string::npos) break;

               Data* data = nullptr;
               auto ok = false;

               if((statement[lpar + 1] == QUOTE) &&
                  (statement[rpar - 1] == QUOTE))
               {
                  fname = statement.substr(lpar + 2, rpar - lpar - 3);
                  ok = true;
               }
               else
               {
                  dname = statement.substr(lpar + 1, rpar - lpar - 1);
                  data = FindData(dname);
                  if(data == nullptr) break;
                  ok = data->GetStrValue(fname);
               }

               if(ok)
               {
                  ok = (*f)->CheckDebugName(fname);

                  if(!cover->Insert(fname, hash, Name()))
                  {
                     LogLine(n, DebugFtNameDuplicated);
                  }
                  else if(ok && (data != nullptr) && (data->Readers() <= 1))
                  {
                     LogPos(lexer_.GetLineStart(n), DebugFtCanBeLiteral, data);
                  }
               }

               if(!ok) LogPos(begin, DebugFtNameMismatch, *f);
            }
            break;

         case SourceCode:
            if(open) code = true;
            break;
         }
      }

      if(!debug)
      {
         LogPos(begin, DebugFtNotInvoked, *f);
      }
   }
}

//------------------------------------------------------------------------------

void CodeFile::CheckFunctionOrder() const
{
   Debug::ft("CodeFile.CheckFunctionOrder");

   if(IsHeader() || funcs_.empty()) return;

   //  Create a list of functions that are *defined* in this file.  Skip
   //  functions created in template instances, which were added to this
   //  file if it caused their instantiation.
   //
   std::list< const Function* > defns;

   for(auto f = funcs_.cbegin(); f != funcs_.cend(); ++f)
   {
      if((*f)->IsInTemplateInstance()) continue;
      if((*f)->GetDefnFile() != this) continue;
      defns.push_back((*f)->GetDefn());
   }

   //  Now sort the functions according to where they were defined and
   //  check that the functions within the same scope are sorted.
   //
   defns.sort(IsSortedByPosition);

   CxxScope* scope = nullptr;
   auto state = FuncCtor;
   const string* prev = nullptr;
   const string* curr = nullptr;

   for(auto f = defns.cbegin(); f != defns.cend(); ++f)
   {
      if((*f)->GetScope() != scope)
      {
         scope = (*f)->GetScope();
         state = FuncCtor;
         prev = nullptr;
         curr = nullptr;
      }

      switch(state)
      {
      case FuncCtor:
         switch((*f)->FuncType())
         {
         case FuncOperator:
         case FuncStandard:
            prev = (*f)->Name();
            //  [[fallthrough]]
         case FuncDtor:
            state = FuncStandard;
         }
         break;

      case FuncStandard:
         switch((*f)->FuncType())
         {
         case FuncCtor:
         case FuncDtor:
            LogPos((*f)->GetPos(), FunctionNotSorted);
            break;

         case FuncOperator:
            curr = (*f)->Name();
            if((prev != nullptr) && (strCompare(*curr, *prev) < 0))
            {
               if(prev->find(OPERATOR_STR) != 0)
               {
                  LogPos((*f)->GetPos(), FunctionNotSorted);
               }
            }
            prev = curr;
            break;

         case FuncStandard:
            curr = (*f)->Name();
            if((prev != nullptr) && (strCompare(*curr, *prev) < 0))
            {
               LogPos((*f)->GetPos(), FunctionNotSorted);
            }
            prev = curr;
         }
      }
   }
}

//------------------------------------------------------------------------------

void CodeFile::CheckIncludeGuard()
{
   Debug::ft("CodeFile.CheckIncludeGuard");

   if(IsCpp()) return;

   size_t pos = string::npos;
   size_t n;

   for(n = 0; (n < lineType_.size()) && (pos == string::npos); ++n)
   {
      if(!LineTypeAttr::Attrs[lineType_[n]].isCode) continue;

      switch(lineType_[n])
      {
      case HashDirective:
         pos = lexer_.GetLineStart(n);
         break;
      default:
         LogLine(n, IncludeGuardMissing);
         return;
      }
   }

   if((pos == string::npos) || (code_.find(HASH_IFNDEF_STR, pos) != pos))
   {
      if(code_.find(HASH_PRAGMA_STR, pos) == pos)
      {
         pos += strlen(HASH_PRAGMA_STR);
         pos = code_.find_first_not_of(WhitespaceChars, pos);
         if(code_.find("once", pos) == pos) return;
      }

      LogLine(n, IncludeGuardMissing);
      return;
   }

   lexer_.Reposition(pos + strlen(HASH_IFNDEF_STR));

   //  Assume that this is an include guard.  Check its name
   //  against the standard.
   //
   auto name = MakeGuardName();
   auto symbol = lexer_.NextIdentifier();
   if(symbol != name) LogLine(n, IncludeGuardMisnamed);
}

//------------------------------------------------------------------------------

void CodeFile::CheckIncludeOrder() const
{
   Debug::ft("CodeFile.CheckIncludeOrder");

   //  The desired order for #include directives is
   //    1. files in declIds_ or baseIds_
   //    2. external files
   //    3. internal files
   //  with each group in alphabetical order.  External and internal files are
   //  distinguished by whether they appear in angle brackets or quotes, but it
   //  is necessary to identify the file associated with each #include directive
   //  to determine which belong to group 1.  Directives in this group are also
   //  tagged so that the Editor can sort them.
   //
   auto i1 = incls_.cbegin();
   if(i1 == incls_.cend()) return;
   auto group1 = CalcGroup(**i1);
   auto name1 = (*i1)->Name();

   for(auto i2 = std::next(i1); i2 != incls_.cend(); ++i2)
   {
      auto group2 = CalcGroup(**i2);
      auto name2 = (*i2)->Name();

      //  After the first #include, they should be sorted by group,
      //  and alphabetically within each group.
      //
      if(group1 > group2)
      {
         LogPos((*i2)->GetPos(), IncludeNotSorted);
      }
      else if(group1 == group2)
      {
         if(strCompare(*name1, *name2) > 0)
            LogPos((*i2)->GetPos(), IncludeNotSorted);
      }

      //  Look for a duplicated #include.
      //
      for(auto i3 = i2; i3 != incls_.cend(); ++i3)
      {
         if(*name1 == *(*i3)->Name())
         {
            LogPos((*i3)->GetPos(), IncludeDuplicated);
         }
      }

      group1 = group2;
      name1 = name2;
   }
}

//------------------------------------------------------------------------------

void CodeFile::CheckIncludes()
{
   Debug::ft("CodeFile.CheckIncludes");

   //  Log any #include directive that follows code.
   //
   auto code = false;

   for(size_t i = 0; i < lineType_.size(); ++i)
   {
      switch(lineType_[i])
      {
      case HashDirective:
         break;
      case IncludeDirective:
         if(code) LogLine(i, IncludeFollowsCode);
         break;
      default:
         if(LineTypeAttr::Attrs[lineType_[i]].isCode) code = true;
      }
   }
}

//------------------------------------------------------------------------------

void CodeFile::CheckLineBreaks()
{
   Debug::ft("CodeFile.CheckLineBreaks");

   //  Look for lines that could be combined and stay within the maximum
   //  line length.
   //
   for(size_t n = 0; n < lineType_.size() - 1; ++n)
   {
      if(!LineTypeAttr::Attrs[lineType_[n]].isMergeable) continue;
      if(!LineTypeAttr::Attrs[lineType_[n + 1]].isMergeable) continue;
      auto begin1 = lexer_.GetLineStart(n);
      auto end1 = code_.find(CRLF, begin1) - 1;
      auto begin2 = lexer_.GetLineStart(n + 1);
      auto end2 = code_.find(CRLF, begin2) - 1;
      auto size = LineMergeLength(code_, begin1, end1, code_, begin2, end2);
      if(size <= LineLengthMax())
      {
         LogLine(n, RemoveLineBreak);
      }
   }
}

//------------------------------------------------------------------------------

void CodeFile::CheckProlog()
{
   Debug::ft("CodeFile.CheckProlog");

   //  Each file should begin with
   //
   //  //==================...
   //  //
   //  //  FileName.ext
   //  //  FileProlog [multiple lines]
   //
   auto pos = lexer_.GetLineStart(0);
   auto ok = (code_.find(DoubleRule, pos) == pos);
   if(!ok) return LogLine(0, HeadingNotStandard);

   pos = lexer_.GetLineStart(1);
   ok = (code_.find(COMMENT_STR, pos) == pos);
   ok = ok && (lineType_[1] == EmptyComment);
   if(!ok) return LogLine(1, HeadingNotStandard);

   pos = lexer_.GetLineStart(2);
   ok = (code_.find(COMMENT_STR, pos) == pos);
   ok = ok && (code_.find(Name(), pos) == pos + 4);
   if(!ok) return LogLine(2, HeadingNotStandard);

   auto prolog = Prolog();
   size_t line = 3;

   for(size_t i = 0; i < prolog.size(); ++i)
   {
      pos = lexer_.GetLineStart(line);
      ok = ok && (code_.find(COMMENT_STR, pos) == pos);

      if(prolog[i].empty())
         ok = ok && (lineType_[line] == EmptyComment);
      else
         ok = ok && (code_.find(prolog[i], pos) == pos + 4);

      if(!ok) return LogLine(line, HeadingNotStandard);
      ++line;
   }
}

//------------------------------------------------------------------------------

void CodeFile::CheckSeparation()
{
   Debug::ft("CodeFile.CheckSeparation");

   //  Look for warnings that involve looking at adjacent lines or the
   //  file's contents as a whole.
   //
   LineType prevType = BlankLine;
   slashAsterisk_ = false;

   for(size_t n = 0; n < lineType_.size(); ++n)
   {
      //  Based on the type of line just found, look for warnings that can
      //  only be found based on the type of line that preceded this one.
      //
      auto nextType =
         (n == lineType_.size() - 1 ? BlankLine : lineType_[n + 1]);

      switch(lineType_[n])
      {
      case SourceCode:
         switch(prevType)
         {
         case FileComment:
         case FunctionName:
         case IncludeDirective:
         case UsingStatement:
            LogLine(n, AddBlankLine);
            break;
         }
         break;

      case BlankLine:
      case EmptyComment:
         switch(prevType)
         {
         case BlankLine:
         case EmptyComment:
         case OpenBrace:
            LogLine(n, RemoveBlankLine);
            break;
         }
         break;

      case TextComment:
      case TaggedComment:
      case SlashAsteriskComment:
      case DebugFt:
         break;

      case SeparatorComment:
         if(!LineTypeAttr::Attrs[prevType].isBlank)
            LogLine(n, AddBlankLine);
         if(!LineTypeAttr::Attrs[nextType].isBlank)
            LogLine(n + 1, AddBlankLine);
         break;

      case OpenBrace:
      case CloseBrace:
      case CloseBraceSemicolon:
         if(LineTypeAttr::Attrs[prevType].isBlank)
            LogLine(n - 1, RemoveBlankLine);
         break;

      case AccessControl:
         if(LineTypeAttr::Attrs[prevType].isBlank)
            LogLine(n - 1, RemoveBlankLine);
         if(LineTypeAttr::Attrs[nextType].isBlank)
            LogLine(n + 1, RemoveBlankLine);
         break;

      case FunctionName:
         switch(prevType)
         {
         case BlankLine:
         case EmptyComment:
         case OpenBrace:
         case FunctionName:
            break;
         case TextComment:
            if(IsCpp()) LogLine(n, AddBlankLine);
            break;
         default:
            LogLine(n, AddBlankLine);
         }
         break;
      }

      prevType = lineType_[n];
   }
}

//------------------------------------------------------------------------------

void CodeFile::CheckUsings() const
{
   Debug::ft("CodeFile.CheckUsings");

   //  Check each using statement and then look for duplicates.
   //
   for(auto u = usings_.cbegin(); u != usings_.cend(); ++u)
   {
      (*u)->Check();
   }

   for(auto u1 = usings_.cbegin(); u1 != usings_.cend(); ++u1)
   {
      for(auto u2 = std::next(u1); u2 != usings_.cend(); ++u2)
      {
         if((*u2)->Referent() == (*u1)->Referent())
         {
            (*u2)->Log(UsingDuplicated);
         }
      }
   }
}

//------------------------------------------------------------------------------

LineType CodeFile::ClassifyLine
   (string s, bool& cont, std::set< Warning >& warnings) const
{
   Debug::ft("CodeFile.ClassifyLine(string)");

   cont = false;

   auto length = s.size();
   if(length == 0) return BlankLine;

   //  Flag the line if it is too long.
   //
   if(length > LineLengthMax()) warnings.insert(LineLength);

   //  Flag any tabs and convert them to spaces.
   //
   for(auto pos = s.find(TAB); pos != string::npos; pos = s.find(TAB))
   {
      warnings.insert(UseOfTab);
      s[pos] = SPACE;
   }

   //  Flag and strip trailing spaces.
   //
   if(s.find_first_not_of(SPACE) == string::npos)
   {
      warnings.insert(TrailingSpace);
      return BlankLine;
   }

   while(s.rfind(SPACE) == s.size() - 1)
   {
      warnings.insert(TrailingSpace);
      s.pop_back();
   }

   //  Flag a line that is not indented a multiple of the standard, unless
   //  it begins with a comment or string literal.
   //
   if(s.empty()) return BlankLine;
   auto pos = s.find_first_not_of(SPACE);
   if(pos > 0) s.erase(0, pos);

   if(pos % IndentSize() != 0)
   {
      if((s[0] != '/') && (s[0] != QUOTE)) warnings.insert(Indentation);
   }

   //  Now that the line has been reformatted, recalculate its length.
   //
   length = s.size();

   //  Look for lines that contain nothing but a brace (or brace and semicolon).
   //
   if((s[0] == '{') && (length == 1)) return OpenBrace;

   if(s[0] == '}')
   {
      if(length == 1) return CloseBrace;
      if((s[1] == ';') && (length == 2)) return CloseBraceSemicolon;
   }

   //  Classify lines that contain only a // comment.
   //
   size_t slashSlashPos = s.find(COMMENT_STR);

   if(slashSlashPos == 0)
   {
      if(length == 2) return EmptyComment;      //
      if(s[2] == '-') return SeparatorComment;  //-
      if(s[2] == '=') return SeparatorComment;  //=
      if(s[2] == '/') return SeparatorComment;  ///
      if(s[2] != SPACE) return TaggedComment;   //$ [$ != one of above]
      return TextComment;                       //  text
   }

   //  Flag a /* comment and see if it ends on the same line.
   //
   pos = FindSubstr(s, COMMENT_BEGIN_STR);

   if(pos != string::npos)
   {
      warnings.insert(UseOfSlashAsterisk);
      if(pos == 0) return SlashAsteriskComment;
   }

   //  Look for preprocessor directives (e.g. #include, #ifndef).
   //
   if(s[0] == '#')
   {
      pos = s.find(HASH_INCLUDE_STR);
      if(pos == 0) return IncludeDirective;
      return HashDirective;
   }

   //  Look for using statements.
   //
   if(s.find("using ") == 0)
   {
      cont = (LastCodeChar(s, slashSlashPos) != ';');
      return UsingStatement;
   }

   //  Look for access controls.
   //
   pos = s.find_first_not_of(WhitespaceChars);

   if(pos != string::npos)
   {
      if(s.find(PUBLIC_STR) == pos) return AccessControl;
      if(s.find(PROTECTED_STR) == pos) return AccessControl;
      if(s.find(PRIVATE_STR) == pos) return AccessControl;
   }

   //  Look for invocations of Debug::ft and its variants.
   //
   if(FindSubstr(s, "Debug::ft(") != string::npos) return DebugFt;
   if(FindSubstr(s, "Debug::ftnt(") != string::npos) return DebugFt;
   if(FindSubstr(s, "Debug::noft(") != string::npos) return DebugFt;

   //  Look for strings that provide function names for Debug::ft.  These
   //  have the format
   //    fn_name ClassName_FunctionName = "ClassName.FunctionName";
   //  with an endline after the '=' if the line would exceed LineLengthMax
   //  characters.
   //
   string type(FunctionName::TypeStr);
   type.push_back(SPACE);

   while(true)
   {
      if(s.find(type) != 0) break;
      auto begin1 = s.find_first_not_of(SPACE, type.size());
      if(begin1 == string::npos) break;
      auto under = s.find('_', begin1);
      if(under == string::npos) break;
      auto equals = s.find('=', under);
      if(equals == string::npos) break;

      if(LastCodeChar(s, slashSlashPos) == '=')
      {
         cont = true;
         return FunctionName;
      }

      auto end1 = s.find_first_not_of(ValidNextChars, under);
      if(end1 == string::npos) break;
      auto begin2 = s.find(QUOTE, equals);
      if(begin2 == string::npos) break;
      auto dot = s.find('.', begin2);
      if(dot == string::npos) break;
      auto end2 = s.find(QUOTE, dot);
      if(end2 == string::npos) break;

      auto front = under - begin1;
      if(s.substr(begin1, front) == s.substr(begin2 + 1, front))
      {
         return FunctionName;
      }
      break;
   }

   pos = FindSubstr(s, "  ");

   if(pos != string::npos)
   {
      auto next = s.find_first_not_of(SPACE, pos);

      if((next != string::npos) && (next != slashSlashPos) && (s[next] != '='))
      {
         warnings.insert(AdjacentSpaces);
      }
   }

   cont = (LastCodeChar(s, slashSlashPos) != ';');
   return SourceCode;
}

//------------------------------------------------------------------------------

LineType CodeFile::ClassifyLine(size_t n, bool& cont)
{
   Debug::ft("CodeFile.ClassifyLine(size_t)");

   //  Get the code for line N and classify it.
   //
   string s;
   if(!lexer_.GetNthLine(n, s)) return LineType_N;

   std::set< Warning > warnings;
   auto type = ClassifyLine(s, cont, warnings);

   //  A line within a /* comment can be logged spuriously.
   //
   if(slashAsterisk_)
   {
      warnings.erase(Indentation);
      warnings.erase(AdjacentSpaces);
   }

   //  Log any warnings that were reported.
   //
   for(auto w = warnings.cbegin(); w != warnings.cend(); ++w)
   {
      LogLine(n, *w);
   }

   //  There are some things that can only be determined by knowing what
   //  happened on previous lines.  First, see if a /* comment ended.
   //
   if(slashAsterisk_)
   {
      auto pos = s.find(COMMENT_END_STR);
      if(pos != string::npos) slashAsterisk_ = false;
      return TextComment;
   }

   //  See if a /* comment began, and whether it is still open.  Note that
   //  when a /* comment is used, a line that contains code after the */
   //  is classified as a comment unless the /* occurred somewhere after
   //  the start of that line.
   //
   if(warnings.find(UseOfSlashAsterisk) != warnings.end())
   {
      if(s.find(COMMENT_END_STR) == string::npos) slashAsterisk_ = true;
      if(s.find(COMMENT_BEGIN_STR) == 0) return SlashAsteriskComment;
   }

   return type;
}

//------------------------------------------------------------------------------

word CodeFile::CreateEditor(string& expl) const
{
   Debug::ft("CodeFile.CreateEditor");

   expl.clear();
   if(editor_ != nullptr) return 0;

   //  Fail if the file's directory is unknown.
   //
   if(dir_ == nullptr)
   {
      expl = "Directory not found for " + Name() + '.';
      return -1;
   }

   //  Fail if the editor can't be created.
   //
   editor_.reset(new Editor(this, expl));

   //  Fail if the editor set EXPL to explain an error.
   //
   if(!expl.empty()) return -1;
   return 0;
}

//------------------------------------------------------------------------------

void CodeFile::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   LibraryItem::Display(stream, prefix, options);

   stream << prefix << "fid        : " << fid_.to_str() << CRLF;
   stream << prefix << "dir        : " << dir_ << CRLF;
   stream << prefix << "isHeader   : " << isHeader_ << CRLF;
   stream << prefix << "isSubsFile : " << isSubsFile_ << CRLF;
   stream << prefix << "parsed     : " << parsed_ << CRLF;
   stream << prefix << "checked    : " << checked_ << CRLF;

   auto& files = Singleton< Library >::Instance()->Files();

   stream << prefix << "inclIds : " << inclIds_.size() << CRLF;

   auto lead = prefix + spaces(2);

   for(auto i = inclIds_.cbegin(); i != inclIds_.cend(); ++i)
   {
      auto f = files.At(*i);
      stream << lead << strIndex(*i) << f->Name() << CRLF;
   }

   stream << prefix << "userIds : " << userIds_.size() << CRLF;

   for(auto u = userIds_.cbegin(); u != userIds_.cend(); ++u)
   {
      auto f = files.At(*u);
      stream << lead << strIndex(*u) << f->Name() << CRLF;
   }

   Flags none;

   stream << prefix << "#includes : " << incls_.size() << CRLF;

   for(auto i = incls_.cbegin(); i != incls_.cend(); ++i)
   {
      (*i)->Display(stream, lead, none);
   }

   stream << prefix << "usings : " << usings_.size() << CRLF;

   for(auto u = usings_.cbegin(); u != usings_.cend(); ++u)
   {
      (*u)->Display(stream, lead, none);
   }
}

//------------------------------------------------------------------------------

void CodeFile::DisplayItems(ostream& stream, const string& opts) const
{
   if(dir_ == nullptr) return;

   stream << Path();
   if(parsed_ == Unparsed) stream << ": NOT PARSED";
   stream << CRLF;
   if(parsed_ == Unparsed) return;

   auto lead = spaces(INDENT_SIZE);
   Flags options(FQ_Mask);
   if(opts.find(ItemStatistics) != string::npos) options.set(DispStats);

   if(opts.find(CanonicalFileView) != string::npos)
   {
      stream << '{' << CRLF;
      DisplayObjects(incls_, stream, lead, options);
      DisplayObjects(macros_, stream, lead, options);
      DisplayObjects(asserts_, stream, lead, options);
      DisplayObjects(forws_, stream, lead, options);
      DisplayObjects(usings_, stream, lead, options);
      DisplayObjects(enums_, stream, lead, options);
      DisplayObjects(types_, stream, lead, options);
      DisplayObjects(funcs_, stream, lead, options);
      DisplayObjects(assembly_, stream, lead, options);
      DisplayObjects(data_, stream, lead, options);
      DisplayObjects(classes_, stream, lead, options);
      stream << '}' << CRLF;
   }

   if(opts.find(OriginalFileView) != string::npos)
   {
      stream << '{' << CRLF;
      DisplayObjects(items_, stream, lead, options);
      stream << '}' << CRLF;
   }
}

//------------------------------------------------------------------------------

void CodeFile::EraseInternals(CxxNamedSet& set) const
{
   Debug::ft("CodeFile.EraseInternals");

   for(auto i = set.cbegin(); i != set.cend(); NO_OP)
   {
      if((*i)->GetFile() == this)
         set.erase(*i++);
      else
         ++i;
   }
}

//------------------------------------------------------------------------------

Data* CodeFile::FindData(const string& name) const
{
   Debug::ft("CodeFile.FindData");

   for(auto d = data_.cbegin(); d != data_.cend(); ++d)
   {
      if(*(*d)->Name() == name) return *d;
   }

   return nullptr;
}

//------------------------------------------------------------------------------

void CodeFile::FindDeclIds()
{
   Debug::ft("CodeFile.FindDeclIds");

   //  If this is a .cpp, find declIds_, the headers that declare items that
   //  the .cpp defines.  Also find classIds_, the transitive base classes of
   //  the classes that the .cpp implements.
   //
   if(!IsCpp()) return;

   for(auto f = funcs_.cbegin(); f != funcs_.cend(); ++f)
   {
      auto fid = (*f)->GetDistinctDeclFid();
      if(fid != NIL_ID) declIds_.insert(fid);

      auto c = (*f)->GetClass();
      if(c != nullptr)
      {
         for(auto b = c->BaseClass(); b != nullptr; b = b->BaseClass())
         {
            classIds_.insert(b->GetDeclFid());
         }
      }
   }

   for(auto d = data_.cbegin(); d != data_.cend(); ++d)
   {
      auto fid = (*d)->GetDistinctDeclFid();
      if(fid != NIL_ID) declIds_.insert(fid);

      auto c = (*d)->GetClass();
      if(c != nullptr)
      {
         for(auto b = c->BaseClass(); b != nullptr; b = b->BaseClass())
         {
            classIds_.insert(b->GetDeclFid());
         }
      }
   }
}

//------------------------------------------------------------------------------

CodeWarning* CodeFile::FindLog(const CodeWarning& log,
   const CxxNamed* item, word offset, std::string& expl) const
{
   Debug::ft("CodeFile.FindLog");

   if(CreateEditor(expl) != 0) return nullptr;
   return editor_->FindLog(log, item, offset);
}

//------------------------------------------------------------------------------

void CodeFile::FindOrAddUsing(const CxxNamed* user)
{
   Debug::ft("CodeFile.FindOrAddUsing");

   string name;
   CxxScoped* ref;
   auto qname = user->GetQualName();

   if(qname != nullptr)
   {
      auto first = qname->First();
      name = first->QualifiedName(true, false);
      ref = first->DirectType();
   }
   else
   {
      name = user->QualifiedName(true, false);
      ref = user->DirectType();
   }

   if(ref == nullptr) return;
   auto tmplt = ref->GetTemplate();
   if(tmplt != nullptr) ref = tmplt;

   //  This loop was adapted from CxxScoped.NameRefersToItem, simplified
   //  to handle only the case where a symbol must be resolved by a using
   //  statement.
   //
   Using* u = nullptr;
   stringVector fqNames;
   ref->GetScopedNames(fqNames, false);

   for(auto fqn = fqNames.begin(); fqn != fqNames.end() && u == nullptr; ++fqn)
   {
      auto pos = NameCouldReferTo(*fqn, name);
      if(pos == string::npos) continue;
      fqn->erase(0, 2);

      //  If this file has a suitable using statement, keep it.
      //
      u = GetUsingFor(*fqn, pos - 4, ref, user->GetScope());
      if(u != nullptr) u->MarkForRetention();
   }

   if(u == nullptr)
   {
      //  This file did not have a suitable using statement, so it should add
      //  one.  If REF is external, add a using declaration (for a specific
      //  item).  If REF is internal, add a using directive (for a namespace).
      //
      if(!ref->GetFile()->IsSubsFile())
      {
         auto space = ref->GetSpace();
         if(space != nullptr) ref = space;
      }

      QualNamePtr qualName;
      name = ref->ScopedName(false);
      auto scope = Singleton< CxxRoot >::Instance()->GlobalNamespace();
      std::unique_ptr< Parser > parser(new Parser(scope));
      parser->ParseQualName(name, qualName);
      parser.reset();
      qualName->SetReferent(ref, nullptr);
      UsingPtr use(new Using(qualName, false, true));
      use->SetScope(scope);
      use->SetLoc(this, CxxLocation::NOT_IN_SOURCE);
      scope->AddUsing(use);
   }
}

//------------------------------------------------------------------------------

Using* CodeFile::FindUsingFor(const string& fqName, size_t prefix,
   const CxxScoped* item, const CxxScope* scope) const
{
   Debug::ft("CodeFile.FindUsingFor");

   //  It's easy if this file or SCOPE has a sufficient using statement.
   //
   auto u = GetUsingFor(fqName, prefix, item, scope);
   if(u != nullptr) return u;
   u = scope->GetUsingFor(fqName, prefix, item, scope);
   if(u != nullptr) return u;

   //  Something that this file #includes (transitively) must make ITEM visible.
   //  Search the files that affect this one.  A file in the resulting set must
   //  have a visible using statement for NAME, at least up to PREFIX.
   //
   auto search = this->Affecters();

   //  Omit files that also affect the one that defines NAME.  This removes the
   //  file that actually defines NAME, so add it back to the search.  Do not
   //  omit files, however, if ITEM is a forward or friend declaration, as its
   //  actual definition could occur totally outside of the search area.
   //
   if(!item->IsForward())
   {
      SetDifference(search, item->GetFile()->Affecters());
      search.insert(item->GetFile()->Fid());
   }

   auto& files = Singleton< Library >::Instance()->Files();

   for(auto f = search.cbegin(); f != search.cend(); ++f)
   {
      u = files.At(*f)->GetUsingFor(fqName, prefix, item, scope);
      if(u != nullptr) return u;
   }

   return nullptr;
}

//------------------------------------------------------------------------------

word CodeFile::Fix(CliThread& cli, const FixOptions& opts, string& expl) const
{
   Debug::ft("CodeFile.Fix");

   auto rc = CreateEditor(expl);

   if(rc < -1) return rc;  // don't continue with other files

   if(rc == -1)  // continue with other files
   {
      *cli.obuf << expl << CRLF;
      return 0;
   }

   rc = editor_->Fix(cli, opts, expl);

   if(rc >= -1) return 0;  // continue with other files
   return rc;              // don't continue with other files
}

//------------------------------------------------------------------------------

word CodeFile::Format(string& expl) const
{
   Debug::ft("CodeFile.Format");

   Debug::Progress(Name() + CRLF);

   auto rc = CreateEditor(expl);
   if(rc != 0) return rc;

   return editor_->Format(expl);
}

//------------------------------------------------------------------------------

void CodeFile::GenerateReport(ostream* stream, const SetOfIds& set)
{
   Debug::ft("CodeFile.GenerateReport");

   CodeWarning::GenerateReport(stream, set);
}

//------------------------------------------------------------------------------

void CodeFile::GetDeclaredBaseClasses(CxxNamedSet& bases) const
{
   Debug::ft("CodeFile.GetDeclaredBaseClasses");

   //  Reset BASES to the base classes of those declared in this file.
   //
   bases.clear();

   for(auto c = classes_.cbegin(); c != classes_.cend(); ++c)
   {
      auto base = (*c)->BaseClass();
      if(base != nullptr) bases.insert(base);
   }
}

//------------------------------------------------------------------------------

int8_t CodeFile::GetDepth(size_t line) const
{
   int8_t depth;
   bool cont;
   lexer_.GetDepth(line, depth, cont);

   if(line >= lineType_.size()) return depth;

   if(!LineTypeAttr::Attrs[lineType_[line]].isCode) return depth;

   switch(lineType_[line])
   {
   case IncludeDirective:
   case HashDirective:
      //
      //  Don't indent these.
      //
      return 0;

   default:
      //
      //  For code.
      //
      return (cont ? depth + 1 : depth);
   }
}

//------------------------------------------------------------------------------

Editor* CodeFile::GetEditor(string& expl) const
{
   Debug::ft("CodeFile.GetEditor");

   if(editor_ == nullptr)
   {
      CreateEditor(expl);
   }

   return editor_.get();
}

//------------------------------------------------------------------------------

void CodeFile::GetLineCounts() const
{
   //  Don't count lines in substitute files.
   //
   if(isSubsFile_) return;

   CodeWarning::AddLineType(AnyLine, lineType_.size());

   for(size_t n = 0; n < lineType_.size(); ++n)
   {
      CodeWarning::AddLineType(lineType_[n], 1);
   }
}

//------------------------------------------------------------------------------

LineType CodeFile::GetLineType(size_t n) const
{
   if(n >= lineType_.size()) return LineType_N;
   return lineType_[n];
}

//------------------------------------------------------------------------------

void CodeFile::GetUsageInfo(CxxUsageSets& symbols) const
{
   Debug::ft("CodeFile.GetUsageInfo");

   //  Ask each of the code items in this file to provide information
   //  about the symbols that it uses.
   //
   for(auto m = macros_.cbegin(); m != macros_.cend(); ++m)
   {
      (*m)->GetUsages(*this, symbols);
   }

   for(auto c = classes_.cbegin(); c != classes_.cend(); ++c)
   {
      //  Bypass a class template instantiation, which is registered
      //  against the file that caused its instantiation.
      //
      if((*c)->IsInTemplateInstance()) continue;
      (*c)->GetUsages(*this, symbols);
   }

   for(auto t = types_.cbegin(); t != types_.cend(); ++t)
   {
      (*t)->GetUsages(*this, symbols);
   }

   for(auto f = funcs_.cbegin(); f != funcs_.cend(); ++f)
   {
      //  Bypass a function template instantiation, which is registered
      //  against the file that caused its instantiation.
      //
      if((*f)->IsInTemplateInstance()) continue;
      (*f)->GetUsages(*this, symbols);
   }

   for(auto d = data_.cbegin(); d != data_.cend(); ++d)
   {
      (*d)->GetUsages(*this, symbols);
   }

   for(auto a = asserts_.cbegin(); a != asserts_.cend(); ++a)
   {
      (*a)->GetUsages(*this, symbols);
   }

   //  For a .cpp, include, as base classes, those defined in any header
   //  that the .cpp implements.
   //
   if(IsCpp())
   {
      auto& files = Singleton< Library >::Instance()->Files();

      for(auto d = declIds_.cbegin(); d != declIds_.cend(); ++d)
      {
         auto classes = files.At(*d)->Classes();

         for(auto c = classes->cbegin(); c != classes->cend(); ++c)
         {
            symbols.AddBase(*c);
         }
      }
   }
}

//------------------------------------------------------------------------------

Using* CodeFile::GetUsingFor(const string& fqName,
   size_t prefix, const CxxNamed* item, const CxxScope* scope) const
{
   Debug::ft("CodeFile.GetUsingFor");

   //c Verify that the using statement is visible to SCOPE.  Files don't have
   //  using statements: namespaces, classes, and function blocks do, although
   //  trying usings in the same file first is a good performance strategy.
   //
   for(auto u = usings_.cbegin(); u != usings_.cend(); ++u)
   {
      if((*u)->IsUsingFor(fqName, prefix, scope)) return *u;
   }

   return nullptr;
}

//------------------------------------------------------------------------------

bool CodeFile::HasForwardFor(const CxxNamed* item) const
{
   Debug::ft("CodeFile.HasForwardFor");

   for(auto f = forws_.cbegin(); f != forws_.cend(); ++f)
   {
      if((*f)->Referent() == item) return true;
   }

   return false;
}

//------------------------------------------------------------------------------

const SetOfIds& CodeFile::Implementers()
{
   Debug::ft("CodeFile.Implementers");

   //  If implIds_ is empty, build it.
   //
   if(!implIds_.empty()) return implIds_;

   auto fileSet = new CodeFileSet(LibrarySet::TemporaryName(), nullptr);
   auto& imSet = fileSet->Set();
   imSet.insert(Fid());

   //  Find all the files that declare or define the functions and data
   //  that this file defines or declares, and add them to the set.
   //
   for(auto c = classes_.cbegin(); c != classes_.cend(); ++c)
   {
      (*c)->AddFiles(imSet);
   }

   for(auto f = funcs_.cbegin(); f != funcs_.cend(); ++f)
   {
      (*f)->AddFiles(imSet);
   }

   for(auto d = data_.cbegin(); d != data_.cend(); ++d)
   {
      (*d)->AddFiles(imSet);
   }

   implIds_ = imSet;
   fileSet->Release();
   return implIds_;
}

//------------------------------------------------------------------------------

size_t CodeFile::IndentSize() const
{
   return INDENT_SIZE;
}

//------------------------------------------------------------------------------

istreamPtr CodeFile::InputStream() const
{
   Debug::ft("CodeFile.InputStream");

   //  If the file's directory is unknown (e.g. in the standard library), it
   //  can't be opened.  The file could nonetheless be known, however, as the
   //  result of parsing an #include directive.
   //
   if(dir_ == nullptr) return nullptr;
   return SysFile::CreateIstream(Path().c_str());
}

//------------------------------------------------------------------------------

void CodeFile::InsertAsm(Asm* code)
{
   items_.push_back(code);
   assembly_.push_back(code);
}

//------------------------------------------------------------------------------

void CodeFile::InsertClass(Class* cls)
{
   items_.push_back(cls);
   classes_.push_back(cls);
}

//------------------------------------------------------------------------------

void CodeFile::InsertData(Data* data)
{
   items_.push_back(data);
   data_.push_back(data);
}

//------------------------------------------------------------------------------

bool CodeFile::InsertDirective(DirectivePtr& dir)
{
   items_.push_back(dir.get());
   dirs_.push_back(std::move(dir));
   return true;
}

//------------------------------------------------------------------------------

void CodeFile::InsertEnum(Enum* item)
{
   items_.push_back(item);
   enums_.push_back(item);
}

//------------------------------------------------------------------------------

void CodeFile::InsertForw(Forward* forw)
{
   items_.push_back(forw);
   forws_.push_back(forw);
}

//------------------------------------------------------------------------------

void CodeFile::InsertFunc(Function* func)
{
   items_.push_back(func);
   funcs_.push_back(func);
}

//------------------------------------------------------------------------------

void CodeFile::InsertInclude(IncludePtr& incl)
{
   incls_.push_back(std::move(incl));
}

//------------------------------------------------------------------------------

fn_name CodeFile_InsertInclude = "CodeFile.InsertInclude(fn)";

Include* CodeFile::InsertInclude(const string& fn)
{
   Debug::ft(CodeFile_InsertInclude);

   for(auto i = incls_.cbegin(); i != incls_.cend(); ++i)
   {
      if(*(*i)->Name() == fn)
      {
         items_.push_back(i->get());
         return i->get();
      }
   }

   Context::SwLog(CodeFile_InsertInclude, fn, 0);
   return nullptr;
}

//------------------------------------------------------------------------------

void CodeFile::InsertMacro(Macro* macro)
{
   items_.push_back(macro);
   macros_.push_back(macro);
}

//------------------------------------------------------------------------------

void CodeFile::InsertSpace(SpaceDefnPtr& space)
{
   items_.push_back(space.get());
   spaces_.push_back(std::move(space));
}

//------------------------------------------------------------------------------

void CodeFile::InsertStaticAssert(StaticAssert* assert)
{
   items_.push_back(assert);
   asserts_.push_back(assert);
}

//------------------------------------------------------------------------------

void CodeFile::InsertType(Typedef* type)
{
   items_.push_back(type);
   types_.push_back(type);
}

//------------------------------------------------------------------------------

void CodeFile::InsertUsing(Using* use)
{
   items_.push_back(use);
   usings_.push_back(use);
}

//------------------------------------------------------------------------------

size_t CodeFile::LineLengthMax() const
{
   return LINE_LENGTH_MAX;
}

//------------------------------------------------------------------------------

fn_name CodeFile_LogAddForwards = "CodeFile.LogAddForwards";

void CodeFile::LogAddForwards(ostream* stream, const CxxNamedSet& items) const
{
   Debug::ft(CodeFile_LogAddForwards);

   //  For each item in ITEMS, generate a ForwardAdd log.  Put their names
   //  in a stringSet so they will always appear in the same order.
   //
   stringSet names;

   for(auto i = items.cbegin(); i != items.cend(); ++i)
   {
      std::ostringstream name;

      auto cls = (*i)->GetClass();

      if(cls != nullptr)
      {
         auto parms = cls->GetTemplateParms();
         if(parms != nullptr) parms->Print(name, NoFlags);

         auto tag = cls->GetClassTag();
         name << tag << SPACE;
      }
      else
      {
         string expl = "Non-class forward: " + (*i)->ScopedName(true);
         Debug::SwLog(CodeFile_LogAddForwards, expl, 0, false);
      }

      name << (*i)->ScopedName(true);
      names.insert(name.str());
   }

   for(auto n = names.cbegin(); n != names.cend(); ++n)
   {
      LogPos(0, ForwardAdd, nullptr, 0, *n);
   }

   DisplaySymbols(stream, items, "Add a forward declaration for");
}

//------------------------------------------------------------------------------

void CodeFile::LogAddIncludes(ostream* stream, const SetOfIds& fids) const
{
   Debug::ft("CodeFile.LogAddIncludes");

   //  For each file in FIDS, generate a log saying that an #include for
   //  it should be added.  Put the filename in angle brackets or quotes,
   //  whichever is appropriate.
   //
   auto& files = Singleton< Library >::Instance()->Files();

   for(auto i = fids.cbegin(); i != fids.cend(); ++i)
   {
      string fn;
      auto f = files.At(*i);
      auto x = f->IsSubsFile();
      fn.push_back(x ? '<' : QUOTE);
      fn += f->Name();
      fn.push_back(x ? '>' : QUOTE);
      LogPos(0, IncludeAdd, nullptr, 0, fn);
   }

   DisplayFileNames(stream, fids, "Add an #include for");
}

//------------------------------------------------------------------------------

void CodeFile::LogAddUsings(ostream* stream) const
{
   Debug::ft("CodeFile.LogAddUsings");

   //  Remove any redundant using statements.  These arise if, for example,
   //  a using statement to resolve A::B::C is added before one for A::B.
   //
   for(auto u1 = usings_.cbegin(); u1 != usings_.cend(); ++u1)
   {
      if((*u1)->IsToBeRemoved()) continue;

      for(auto u2 = std::next(u1); u2 != usings_.cend(); ++u2)
      {
         if((*u2)->IsToBeRemoved()) continue;

         auto ref1 = (*u1)->Referent();
         auto ref2 = (*u2)->Referent();

         auto fqName2 = ref2->ScopedName(false);
         if(ref1->IsSuperscopeOf(fqName2, false))
         {
            (*u2)->MarkForRemoval();
            continue;
         }

         auto fqName1 = ref1->ScopedName(false);
         if(ref2->IsSuperscopeOf(fqName1, false))
         {
            (*u1)->MarkForRemoval();
         }
      }
   }

   //  Log the using statements that should be added.  If a using statement
   //  is for an external item (e.g. in namespace std::), provide its fully
   //  qualified name.  If it is internal (e.g. part of RSC), just provide
   //  its namespace.
   //
   CxxNamedSet usings;

   for(auto u = usings_.cbegin(); u != usings_.cend(); ++u)
   {
      string name;

      if((*u)->WasAdded() && !(*u)->IsToBeRemoved())
      {
         auto ref = (*u)->Referent();

         if(ref->GetFile()->IsSubsFile())
         {
            name = ref->ScopedName(true);
            usings.insert(ref);
         }
         else
         {
            auto space = ref->GetSpace();
            name = NAMESPACE_STR;
            name.push_back(SPACE);
            name += space->ScopedName(false);
            usings.insert(space);
         }

         LogPos(0, UsingAdd, nullptr, 0, name);
      }
   }

   DisplaySymbols(stream, usings, "Add a using statement for");
}

//------------------------------------------------------------------------------

void CodeFile::LogCode(Warning warning, size_t line, size_t pos,
   const CxxNamed* item, word offset, const string& info, bool hide) const
{
   Debug::ft("CodeFile.LogCode");

   //  Don't log warnings in a substitute file or a template instance.
   //
   if(isSubsFile_) return;
   if(Context::ParsingTemplateInstance()) return;

   //  Log the warning if it is valid.
   //
   if((warning < Warning_N) &&
      (line < lineType_.size()) &&
      (pos < code_.size()))
   {
      CodeWarning log(warning, this, line, pos, item, offset, info, hide);
      log.Insert();
   }
}

//------------------------------------------------------------------------------

void CodeFile::LogLine(size_t line, Warning warning,
   word offset, const string& info, bool hide) const
{
   Debug::ft("CodeFile.LogLine");

   auto pos = lexer_.GetLineStart(line);
   LogCode(warning, line, pos, nullptr, offset, info, hide);
}

//------------------------------------------------------------------------------

void CodeFile::LogPos(size_t pos, Warning warning,
   const CxxNamed* item, word offset, const string& info, bool hide) const
{
   Debug::ft("CodeFile.LogPos");

   auto line = lexer_.GetLineNum(pos);
   LogCode(warning, line, pos, item, offset, info, hide);
}

//------------------------------------------------------------------------------

void CodeFile::LogRemoveForwards
   (ostream* stream, const CxxNamedSet& items) const
{
   Debug::ft("CodeFile.LogRemoveForwards");

   //  Log each forward declaration that should be removed.  If the
   //  declaration cannot be found (something that shouldn't occur),
   //  just display its symbol.
   //
   for(auto i = items.cbegin(); i != items.cend(); ++i)
   {
      auto name = (*i)->ScopedName(true);

      for(auto f = forws_.cbegin(); f != forws_.cend(); ++f)
      {
         if((*f)->ScopedName(true) == name)
         {
            LogPos((*f)->GetPos(), ForwardRemove);
         }
      }
   }

   DisplaySymbols(stream, items, "Remove the forward declaration for");
}

//------------------------------------------------------------------------------

void CodeFile::LogRemoveIncludes
   (ostream* stream, const SetOfIds& fids) const
{
   Debug::ft("CodeFile.LogRemoveIncludes");

   //  For each file in FIDS, generate a log saying that the #include for
   //  it should be removed.
   //
   auto& files = Singleton< Library >::Instance()->Files();

   for(auto i = fids.cbegin(); i != fids.cend(); ++i)
   {
      string fn;
      auto f = files.At(*i);
      auto x = f->IsSubsFile();
      fn.push_back(x ? '<' : QUOTE);
      fn += f->Name();
      fn.push_back(x ? '>' : QUOTE);
      auto pos = code_.find(fn);
      if(pos != string::npos) LogPos(pos, IncludeRemove);
   }

   DisplayFileNames(stream, fids, "Remove the #include for");
}

//------------------------------------------------------------------------------

void CodeFile::LogRemoveUsings(ostream* stream) const
{
   Debug::ft("CodeFile.LogRemoveUsings");

   //  Using statements still marked for removal should be deleted.
   //  Don't report any that were added by >trim.
   //
   CxxNamedSet delUsing;

   for(auto u = usings_.cbegin(); u != usings_.cend(); ++u)
   {
      if((*u)->IsToBeRemoved() && !(*u)->WasAdded())
      {
         delUsing.insert(*u);
         LogPos((*u)->GetPos(), UsingRemove);
      }
   }

   DisplaySymbols(stream, delUsing, "Remove the using statement for");
}

//------------------------------------------------------------------------------

string CodeFile::MakeGuardName() const
{
   Debug::ft("CodeFile.MakeGuardName");

   if(IsCpp()) return EMPTY_STR;

   auto name = Name();

   for(size_t i = 0; i < name.size(); ++i)
   {
      if(name[i] == '.')
         name[i] = '_';
      else
         name[i] = toupper(name[i]);
   }

   name += "_INCLUDED";
   return name;
}

//------------------------------------------------------------------------------

string CodeFile::Path(bool full) const
{
   if(dir_ == nullptr) return Name();
   auto name = dir_->Path() + PATH_SEPARATOR + Name();

   if(!full)
   {
      string path(Singleton< Library >::Instance()->SourcePath());
      path.push_back(PATH_SEPARATOR);

      if(name.find(path, 0) == 0)
      {
         name.erase(0, path.size());
      }
   }

   return name;
}

//------------------------------------------------------------------------------

const stringVector DEFAULT_PROLOG =
{
   EMPTY_STR,
   "Copyright (C) 2013-2020  Greg Utas",
   EMPTY_STR,
   "This file is part of the Robust Services Core (RSC).",
   EMPTY_STR,
   "RSC is free software: you can redistribute it and/or modify it under the",
   "terms of the GNU General Public License as published by the Free Software",
   "Foundation, either version 3 of the License, or (at your option) any later",
   "version.",
   EMPTY_STR,
   "RSC is distributed in the hope that it will be useful, but WITHOUT ANY",
   "WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS",
   "FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more",
   "details.",
   EMPTY_STR,
   "You should have received a copy of the GNU General Public License along",
   "with RSC.  If not, see <http://www.gnu.org/licenses/>.",
   EMPTY_STR
};

const stringVector& CodeFile::Prolog() const
{
   return DEFAULT_PROLOG;
}

//------------------------------------------------------------------------------

void CodeFile::PruneForwardCandidates(const CxxNamedSet& forwards,
   const SetOfIds& inclIds, CxxNamedSet& addForws) const
{
   Debug::ft("CodeFile.PruneForwardCandidates");

   //  Go through the possible forward declarations and remove those that
   //  are not needed.
   //
   auto& files = Singleton< Library >::Instance()->Files();

   //  Find the files that affect (that is, are transitively #included
   //  by) inclIds (the files that will be #included).
   //
   auto inclSet = new CodeFileSet(LibrarySet::TemporaryName(), nullptr);
   auto& incls = inclSet->Set();
   SetUnion(incls, inclIds);
   auto affecterSet = inclSet->Affecters();
   inclSet->Release();
   auto& affecterIds = static_cast< CodeFileSet* >(affecterSet)->Set();

   for(auto add = addForws.begin(); add != addForws.end(); NO_OP)
   {
      auto addFile = (*add)->GetFile();
      auto remove = false;

      //  Do not add a forward declaration for a type that was resolved by
      //  an existing forward declaration, whether in this file or another.
      //
      for(auto f = forwards.cbegin(); f != forwards.cend(); ++f)
      {
         remove = ((*f)->Referent() == *add);
         if(remove) break;
      }

      if(!remove)
      {
         auto addFid = addFile->Fid();

         //  Do not add a forward declaration for a type that will be #included,
         //  even transitively.
         //
         for(auto a = affecterIds.cbegin(); a != affecterIds.cend(); ++a)
         {
            remove = (*a == addFid);
            if(remove) break;
         }

         //  Do not add a forward declaration for a type that is already forward
         //  declared in a file that will be #included, even transitively.
         //
         if(!remove)
         {
            for(auto a = affecterIds.cbegin(); a != affecterIds.cend(); ++a)
            {
               auto incl = files.At(*a);

               if(incl != this)
               {
                  remove = incl->HasForwardFor(*add);
                  if(remove) break;
               }
            }
         }
      }

      if(remove)
         addForws.erase(*add++);
      else
         ++add;
   }

   affecterSet->Release();
}

//------------------------------------------------------------------------------

void CodeFile::PruneLocalForwards
   (CxxNamedSet& addForws, CxxNamedSet& delForws) const
{
   Debug::ft("CodeFile.PruneLocalForwards");

   //  Keep a forward declaration that resolved a symbol (possibly on behalf
   //  of a different file) or that *will* resolve a symbol that now needs a
   //  forward declaration.  Delete a declaration if its referent cannot be
   //  found.
   //
   for(auto f = forws_.cbegin(); f != forws_.cend(); ++f)
   {
      auto remove = true;
      auto ref = (*f)->Referent();

      if(ref != nullptr)
      {
         if((*f)->IsUnused())
         {
            for(auto a = addForws.begin(); a != addForws.end(); ++a)
            {
               if((*a) == ref)
               {
                  addForws.erase(*a);
                  remove = false;
                  break;
               }
            }
         }
         else
         {
            remove = false;
         }
      }

      if(remove) delForws.insert(*f);
   }
}

//------------------------------------------------------------------------------

void CodeFile::RemoveHeaderIds(SetOfIds& inclIds) const
{
   Debug::ft("CodeFile.RemoveHeaderIds");

   //  If this is a .cpp, it implements items declared one or more headers
   //  (declIds_).  The .cpp need not #include anything that one of those
   //  headers will already #include.
   //
   auto& files = Singleton< Library >::Instance()->Files();

   if(IsCpp())
   {
      for(auto d = declIds_.cbegin(); d != declIds_.cend(); ++d)
      {
         SetDifference(inclIds, files.At(*d)->TrimList());
      }

      //  Ensure that all files in declIds_ are #included.  It is possible for
      //  one to get dropped if, for example, the .cpp uses a subclass of an
      //  item that it implements.
      //
      SetUnion(inclIds, declIds_);
   }
}

//------------------------------------------------------------------------------

void CodeFile::RemoveInvalidIncludes(SetOfIds& addIds) const
{
   Debug::ft("CodeFile.RemoveInvalidIncludes");

   //  A file should not #include
   //  (a) itself
   //  (b) a file that it affects (a file that transitively #includes it)
   //  (c) a .cpp
   //  The latter two can occur when a template uses one of its instances
   //  to resolve symbols accessed through a template parameter.
   //
   auto lib = Singleton< Library >::Instance();
   auto fid = Fid();

   addIds.erase(fid);

   for(auto f = addIds.cbegin(); f != addIds.cend(); NO_OP)
   {
      auto file = lib->Files().At(*f);

      if(file != nullptr)
      {
         auto& affecters = file->Affecters();

         if(file->IsCpp() || (affecters.find(fid) != affecters.cend()))
            f = addIds.erase(f);
         else
            ++f;
      }
   }
}

//------------------------------------------------------------------------------

void CodeFile::SaveBaseIds(const CxxNamedSet& bases)
{
   Debug::ft("CodeFile.SaveBaseIds");

   for(auto b = bases.cbegin(); b != bases.cend(); ++b)
   {
      auto base = static_cast< const Class* >(*b);
      baseIds_.insert(base->GetDeclFid());
   }
}

//------------------------------------------------------------------------------

void CodeFile::Scan()
{
   Debug::ft("CodeFile.Scan");

   if(!code_.empty()) return;

   auto input = InputStream();
   if(input == nullptr) return;
   input->clear();
   input->seekg(0);

   code_.clear();

   string str;

   while(input->peek() != EOF)
   {
      std::getline(*input, str);
      code_ += str;
      code_ += CRLF;
   }

   input.reset();
   lexer_.Initialize(&code_);
   lexer_.CalcDepths();

   auto lines = lexer_.LineCount();

   for(size_t n = 0; n < lines; ++n)
   {
      lineType_.push_back(LineType_N);
   }

   //  Categorize each line.  If the previous line failed to finish
   //  a using statement or function name definition, carry it over
   //  to the next line.
   //
   auto prevCont = false;
   auto prevType = LineType_N;

   for(size_t n = 0; n < lines; ++n)
   {
      auto currCont = false;
      auto currType = ClassifyLine(n, currCont);

      if(prevCont)
      {
         if((prevType != UsingStatement) && (prevType != FunctionName))
         {
            prevCont = false;
         }
      }

      lineType_[n] = (prevCont ? prevType: currType);
      prevCont = currCont;
      prevType = currType;
   }

   for(size_t n = 0; n < lines; ++n)
   {
      auto t = lineType_[n];

      if(LineTypeAttr::Attrs[t].isCode) break;

      if((t != EmptyComment) && (t != SlashAsteriskComment))
      {
         lineType_[n] = FileComment;
      }
   }

   //  Preprocess #include directives.
   //
   auto lib = Singleton< Library >::Instance();
   string file;
   bool angle;

   for(size_t n = 0; n < lines; ++n)
   {
      if(lexer_.GetIncludeFile(lexer_.GetLineStart(n), file, angle))
      {
         auto used = lib->EnsureFile(file);

         if(used != nullptr)
         {
            auto id = used->Fid();
            inclIds_.insert(id);
            trimIds_.insert(id);
            used->AddUser(this);
         }

         IncludePtr incl(new Include(file, angle));
         incl->SetLoc(this, lexer_.GetLineStart(n));
         InsertInclude(incl);
      }
   }
}

//------------------------------------------------------------------------------

void CodeFile::SetDir(CodeDir* dir)
{
   Debug::ft("CodeFile.SetDir");

   dir_ = dir;
   isSubsFile_ = dir_->IsSubsDir();
}

//------------------------------------------------------------------------------

void CodeFile::SetParsed(bool passed)
{
   Debug::ft("CodeFile.SetParsed");

   parsed_ = (passed ? Passed : Failed);
}

//------------------------------------------------------------------------------

void CodeFile::Shrink()
{
   code_.shrink_to_fit();
   CxxStats::Strings(CxxStats::CODE_FILE, code_.capacity());

   for(auto i = incls_.cbegin(); i != incls_.cend(); ++i)
   {
      (*i)->Shrink();
   }

   for(auto d = dirs_.cbegin(); d != dirs_.cend(); ++d)
   {
      (*d)->Shrink();
   }

   for(auto u = usings_.cbegin(); u != usings_.cend(); ++u)
   {
      (*u)->Shrink();
   }

   forws_.shrink_to_fit();
   macros_.shrink_to_fit();
   spaces_.shrink_to_fit();
   classes_.shrink_to_fit();
   enums_.shrink_to_fit();
   types_.shrink_to_fit();
   funcs_.shrink_to_fit();
   data_.shrink_to_fit();
   assembly_.shrink_to_fit();
   asserts_.shrink_to_fit();
   items_.shrink_to_fit();

   auto size = incls_.capacity() * sizeof(IncludePtr);
   size += dirs_.capacity() * sizeof(DirectivePtr);
   size += usings_.capacity() * sizeof(UsingPtr);
   size += forws_.capacity() * sizeof(Forward*);
   size += macros_.capacity() * sizeof(Macro*);
   size += spaces_.capacity() * sizeof(SpaceDefn*);
   size += classes_.capacity() * sizeof(Class*);
   size += enums_.capacity() * sizeof(Enum*);
   size += types_.capacity() * sizeof(Typedef*);
   size += funcs_.capacity() * sizeof(Function*);
   size += data_.capacity() * sizeof(Data*);
   size += assembly_.capacity() * sizeof(Asm*);
   size += asserts_.capacity() * sizeof(StaticAssert*);
   size += items_.capacity() * sizeof(StaticAssert*);
   size += usages_.size() * 3 * sizeof(CxxNamed*);
   CxxStats::Vectors(CxxStats::CODE_FILE, size);
}

//------------------------------------------------------------------------------

void CodeFile::Trim(ostream* stream)
{
   Debug::ft("CodeFile.Trim");

   //  If this file should be trimmed, find the headers that declare items
   //  that this file defines, and assemble information about the symbols
   //  that this file uses.  FindDeclIds must be invoked before GetUsageInfo
   //  because the latter uses declIds_.
   //
   if(!CanBeTrimmed()) return;

   if(stream != nullptr) *stream << Name() << CRLF;
   FindDeclIds();

   CxxUsageSets symbols;
   GetUsageInfo(symbols);
   auto& bases = symbols.bases;
   auto& directs = symbols.directs;
   auto& indirects = symbols.indirects;
   auto& forwards = symbols.forwards;
   auto& friends = symbols.friends;
   auto& users = symbols.users;

   SaveBaseIds(bases);

   //  Remove direct and indirect symbols declared by the file itself.
   //  Find inclSet, the types that were used directly or in executable
   //  code.  It will contain all the types that should be #included.
   //
   CxxNamedSet inclSet;
   EraseInternals(directs);
   EraseInternals(indirects);
   AddDirectTypes(directs, inclSet);

   //  Display the symbols that the file uses.
   //
   DisplaySymbolsAndFiles(stream, bases, "Base usage:");
   DisplaySymbolsAndFiles(stream, inclSet, "Direct usage:");
   DisplaySymbolsAndFiles(stream, indirects, "Indirect usage:");
   DisplaySymbolsAndFiles(stream, forwards, "Forward usage:");
   DisplaySymbolsAndFiles(stream, friends, "Friend usage:");

   //  Expand inclSet with types used indirectly but defined outside
   //  the code base.  Then expand it with forward declarations that
   //  resolved an indirect reference in this file.
   //
   AddIndirectExternalTypes(indirects, inclSet);
   AddForwardDependencies(symbols, inclSet);

   //  A derived class must #include its base class, so shrink inclSet
   //  by removing items defined in an indirect base class.  Then shrink
   //  it by removing items defined in a base class of another item that
   //  will be #included.  Finally, shrink it again by removing classes
   //  that are named in a typedef that will be #included.
   //
   RemoveIndirectBaseItems(bases, inclSet);
   RemoveIncludedBaseItems(inclSet);
   RemoveAliasedClasses(inclSet);

   //  For a .cpp, BASES includes base classes defined in its header.
   //  Regenerate it so that it is limited to base classes for those
   //  declared in the .cpp.  Before doing this, find the files that
   //  define base classes, including transitive base classes, of
   //  classes defined or implemented in this file.  This set (tBaseIds)
   //  will be needed later, when analyzing using statements.
   //
   SetOfIds tBaseIds;
   GetTransitiveBases(bases, tBaseIds);
   if(IsCpp()) GetDeclaredBaseClasses(bases);

   //  An #include should always appear for a base class.  Add them to
   //  inclSet in case any of them were removed.
   //
   for(auto b = bases.cbegin(); b != bases.cend(); ++b)
   {
      inclSet.insert(*b);
   }

   //  Assemble inclIds, all the files that need to be #included.  It
   //  starts with all of the files (excluding this one) that declare
   //  items in inclSet.  A .cpp can then remove any file that will be
   //  #included by a header that declares an item that the .cpp defines.
   //
   SetOfIds inclIds;
   AddIncludeIds(inclSet, inclIds);
   RemoveHeaderIds(inclIds);

   //  Output addIds, the #includes that should be added.  It contains
   //  inclIds, minus any spurious files and what is already #included.
   //
   SetOfIds addIds;
   SetDifference(addIds, inclIds, inclIds_);
   RemoveInvalidIncludes(addIds);
   LogAddIncludes(stream, addIds);

   //  Output delIds, which are the #includes that should be removed.  It
   //  contains what is already #included, minus everything that needs to
   //  be #included.
   //
   SetOfIds delIds;
   SetDifference(delIds, inclIds_, inclIds);
   LogRemoveIncludes(stream, delIds);

   //  Save the files that *should* be #included.
   //
   trimIds_.clear();
   SetUnion(trimIds_, inclIds_, addIds);
   SetDifference(trimIds_, delIds);

   //  Now determine the forward declarations that should be added.  Types
   //  used indirectly, as well as friend declarations, provide candidates.
   //  The candidates are then trimmed by removing, for example, forward
   //  declarations in other files that resolved symbols in this file.
   //
   CxxNamedSet addForws;
   FindForwardCandidates(symbols, addForws);
   PruneForwardCandidates(forwards, inclIds, addForws);

   //  Determine which of the file's current forward declarations to keep.
   //
   CxxNamedSet delForws;
   PruneLocalForwards(addForws, delForws);

   //  Output the forward declarations that should be added or removed.
   //
   LogAddForwards(stream, addForws);
   LogRemoveForwards(stream, delForws);

   //  Look at each name (N) that was resolved by a using statement, and
   //  determine if this file should have a using statement to resolve it.
   //  First, mark all of the using statements in this file for removal.
   //  When FindOrAddUsing determines that one is required, it will be
   //  marked for retention.
   //
   for(auto u = usings_.cbegin(); u != usings_.cend(); ++u)
   {
      (*u)->MarkForRemoval();
   }

   for(auto n = users.cbegin(); n != users.cend(); ++n)
   {
      FindOrAddUsing(*n);
   }

   //  Output the using statements that should be added and removed.
   //
   LogAddUsings(stream);
   LogRemoveUsings(stream);

   //  For a header, output the symbols that were resolved by a using
   //  statement.
   //
   if(IsHeader())
   {
      CxxNamedSet qualify;

      for(auto u = users.cbegin(); u != users.cend(); ++u)
      {
         qualify.insert((*u)->DirectType());
      }

      DisplaySymbolsAndFiles(stream, qualify,
         "To remove dependencies on using statements, qualify");
   }
}
}
