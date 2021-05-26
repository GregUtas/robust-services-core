//==============================================================================
//
//  CodeFile.cpp
//
//  Copyright (C) 2013-2021  Greg Utas
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
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <istream>
#include <iterator>
#include <sstream>
#include <utility>
#include "Algorithms.h"
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
#include "CxxString.h"
#include "CxxToken.h"
#include "Debug.h"
#include "Formatters.h"
#include "Lexer.h"
#include "Library.h"
#include "Parser.h"
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
      auto file = (*f)->GetDeclFile();
      auto include = true;

      for(auto b = symbols.bases.cbegin(); b != symbols.bases.cend(); ++b)
      {
         auto base = static_cast< const Class* >(*b);

         for(auto c = base->BaseClass(); c != nullptr; c = c->BaseClass())
         {
            if(c->GetDeclFile() == file)
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

void DisplayFileNames
   (ostream* stream, const LibItemSet& files, fixed_string title)
{
   //  Display, in STREAM, the names of files in FILES.  TITLE provides an
   //  explanation for the list.
   //
   if(stream == nullptr) return;
   if(files.empty()) return;

   *stream << spaces(3) << title << CRLF;

   for(auto f = files.cbegin(); f != files.cend(); ++f)
   {
      *stream << spaces(6) << (*f)->Name() << CRLF;
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

void GetTransitiveBases(const CxxNamedSet& bases, LibItemSet& tBaseSet)
{
   Debug::ft("CodeTools.GetTransitiveBases");

   for(auto b = bases.cbegin(); b != bases.cend(); ++b)
   {
      auto base = static_cast< const Class* >(*b);

      for(auto c = base; c != nullptr; c = c->BaseClass())
      {
         tBaseSet.insert(c->GetDeclFile());
      }
   }
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

CodeFile::CodeFile(const string& name, CodeDir* dir) :
   name_(name),
   dir_(dir),
   isHeader_(false),
   isSubsFile_(false),
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

   //  There is currently no situation in which this is invoked.

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

void CodeFile::AddIncludes
   (const CxxNamedSet& declSet, LibItemSet& inclSet) const
{
   Debug::ft("CodeFile.AddIncludes");

   for(auto n = declSet.cbegin(); n != declSet.cend(); ++n)
   {
      auto file = (*n)->GetDeclFile();
      if((file != nullptr) && (file != this)) inclSet.insert(file);
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

void CodeFile::AddToXref(bool insert) const
{
   for(auto i = items_.cbegin(); i != items_.cend(); ++i)
   {
      (*i)->AddToXref(insert);
   }
}

//------------------------------------------------------------------------------

void CodeFile::AddUsage(CxxNamed* item)
{
   Debug::ft("CodeFile.AddUsage");

   usages_.insert(item);
}

//------------------------------------------------------------------------------

void CodeFile::AddUser(CodeFile* file)
{
   Debug::ft("CodeFile.AddUser");

   userSet_.insert(file);
}

//------------------------------------------------------------------------------

const LibItemSet& CodeFile::Affecters()
{
   //  If affecterSet_ is empty, build it.
   //
   if(!affecterSet_.empty()) return affecterSet_;

   Debug::ft("CodeFile.Affecters");

   auto fileSet = new CodeFileSet(LibrarySet::TemporaryName(), nullptr);
   auto& context = fileSet->Items();
   context.insert(this);
   auto asSet = fileSet->Affecters();
   affecterSet_ = asSet->Items();
   return affecterSet_;
}

//------------------------------------------------------------------------------

size_t CodeFile::CalcGroup(CodeFile* file) const
{
   Debug::ft("CodeFile.CalcGroup(file)");

   if(file == nullptr) return 0;
   auto ext = file->IsSubsFile();
   if(declSet_.find(file) != declSet_.cend()) return (ext ? 1 : 2);
   if(baseSet_.find(file) != baseSet_.cend()) return (ext ? 3 : 4);
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
   CheckDirectives();
   CheckIncludeGuard();
   CheckUsings();
   CheckVerticalSpacing();
   CheckLineBreaks();
   CheckFunctionOrder();
   CheckDebugFt();
   CheckIncludes();
   CheckIncludeOrder();
   checked_ = true;
}

//------------------------------------------------------------------------------

void CodeFile::CheckDebugFt()
{
   Debug::ft("CodeFile.CheckDebugFt");

   const auto& lines = lexer_.GetLinesInfo();
   auto cover = Singleton< CodeCoverage >::Instance();
   size_t begin, left, end;
   string statement;
   string dname;
   string fname;

   //  For each function in this file, find the lines on which it begins
   //  and ends.  Within those lines, look for invocations of Debug::ft.
   //  If none are found, generate a warning.
   //
   for(auto f = funcs_.cbegin(); f != funcs_.cend(); ++f)
   {
      //  A function in a header is only expected to invoke Debug::ft if
      //  it's part of a template.  A function must also have a left
      //  brace (an implementation) to be checked.
      //
      if(IsHeader() && ((*f)->GetTemplateType() == NonTemplate)) continue;
      if(!(*f)->GetSpan3(begin, left, end)) continue;
      if(left == string::npos) continue;

      auto last = lexer_.GetLineNum(end);
      auto open = false, debug = false, code = false;
      std::ostringstream source;
      (*f)->Display(source, EMPTY_STR, Code_Mask);
      auto hash = string_hash(source.str().c_str());

      for(auto n = lexer_.GetLineNum(begin); n < last; ++n)
      {
         switch(lines[n].type)
         {
         case OpenBrace:
            open = true;
            break;

         case DebugFt:
            //
            //  Generate a warning if other code preceded the call to Debug::ft.
            //  Also generate a warning if the name passed to Debug::ft
            //  o does not accurately identify the function, or
            //  o has already been used by another function, or
            //  o is an fn_name that is only used once and can thus be inlined.
            //
            if(code && !debug) LogLine(n, DebugFtNotFirst);

            if(lexer_.GetNthLine(n, statement, true))
            {
               auto lpar = statement.find('(');
               if(lpar == string::npos) break;
               auto rpar = statement.find(')', lpar);
               if(rpar == string::npos) break;

               Data* data = nullptr;
               auto ok = false;
               auto lquo = statement.find(QUOTE, lpar);

               if(lquo != string::npos)
               {
                  auto rquo = statement.rfind(QUOTE, rpar);
                  if(rquo != string::npos)
                  {
                     fname = statement.substr(lquo + 1, rquo - lquo - 1);
                     ok = true;
                  }
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

                  if(!ok)
                  {
                     LogPos(lexer_.GetLineStart(n), DebugFtNameMismatch, *f);
                  }
                  else if(!debug && !cover->Insert(fname, hash))
                  {
                     LogPos(lexer_.GetLineStart(n), DebugFtNameDuplicated, *f);
                  }
                  else if((data != nullptr) && (data->Readers() <= 1))
                  {
                     LogPos(lexer_.GetLineStart(n), DebugFtCanBeLiteral, data);
                  }

                  debug = true;
               }
            }
            break;

         case CodeLine:
            if(open) code = true;
            break;
         }
      }

      if(!debug)
      {
         (*f)->Log(DebugFtNotInvoked);
      }
   }
}

//------------------------------------------------------------------------------

void CodeFile::CheckDirectives() const
{
   Debug::ft("CodeFile.CheckDirectives");

   for(auto d = dirs_.cbegin(); d != dirs_.cend(); ++d)
   {
      (*d)->Check();
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
   std::vector< const Function* > defns;

   for(auto f = funcs_.cbegin(); f != funcs_.cend(); ++f)
   {
      if((*f)->IsInTemplateInstance()) continue;
      if((*f)->GetDefnFile() != this) continue;
      defns.push_back((*f)->GetDefn());
   }

   //  Now sort the functions according to where they were defined and
   //  check that the functions within the same scope are sorted.
   //
   std::sort(defns.begin(), defns.end(), IsSortedByPos);

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
            prev = &(*f)->Name();
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
            (*f)->Log(FunctionNotSorted);
            break;

         case FuncOperator:
            curr = &(*f)->Name();
            if((prev != nullptr) && (strCompare(*curr, *prev) < 0))
            {
               if(prev->find(OPERATOR_STR) != 0)
               {
                  (*f)->Log(FunctionNotSorted);
               }
            }
            prev = curr;
            break;

         case FuncStandard:
            curr = &(*f)->Name();
            if((prev != nullptr) && (strCompare(*curr, *prev) < 0))
            {
               (*f)->Log(FunctionNotSorted);
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

   for(auto d = dirs_.cbegin(); d != dirs_.cend(); ++d)
   {
      if((*d)->IsIncludeGuard()) return;
   }

   //  An #include guard wasn't found.  Log this against the first
   //  line of code, which will usually be an #include directive.
   //
   const auto& lines = lexer_.GetLinesInfo();
   size_t pos = string::npos;
   size_t n;

   for(n = 0; (n < lines.size()) && (pos == string::npos); ++n)
   {
      if(LineTypeAttr::Attrs[lines[n].type].isCode)
      {
         LogLine(n, IncludeGuardMissing);
         return;
      }
   }
}

//------------------------------------------------------------------------------

void CodeFile::CheckIncludeOrder() const
{
   Debug::ft("CodeFile.CheckIncludeOrder");

   //  The desired order for #include directives is
   //    1. files in declSet_ or baseSet_
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
   auto name1 = &(*i1)->Name();

   for(auto i2 = std::next(i1); i2 != incls_.cend(); ++i2)
   {
      auto group2 = CalcGroup(**i2);
      auto name2 = &(*i2)->Name();

      //  After the first #include, they should be sorted by group,
      //  and alphabetically within each group.
      //
      if(group1 > group2)
      {
         (*i2)->Log(IncludeNotSorted);
      }
      else if(group1 == group2)
      {
         if(strCompare(*name1, *name2) > 0)
            (*i2)->Log(IncludeNotSorted);
      }

      //  Look for a duplicated #include.
      //
      for(auto i3 = i2; i3 != incls_.cend(); ++i3)
      {
         if(*name1 == (*i3)->Name())
            (*i3)->Log(IncludeDuplicated);
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
   const auto& lines = lexer_.GetLinesInfo();
   auto code = false;

   for(size_t i = 0; i < lines.size(); ++i)
   {
      switch(lines[i].type)
      {
      case HashDirective:
         break;
      case IncludeDirective:
         if(code) LogLine(i, IncludeFollowsCode);
         break;
      default:
         if(LineTypeAttr::Attrs[lines[i].type].isCode) code = true;
      }
   }
}

//------------------------------------------------------------------------------

void CodeFile::CheckLineBreaks()
{
   Debug::ft("CodeFile.CheckLineBreaks");

   //  There is no point checking the last line.  And if a line can merge
   //  with the next one, the next line can then be skipped.
   //
   auto limit = lexer_.LineCount() - 1;

   for(size_t n = 0; n < limit; ++n)
   {
      if(lexer_.CheckLineMerge(n) >= 0)
      {
         LogLine(n, RemoveLineBreak);
         ++n;
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
   const auto& lines = lexer_.GetLinesInfo();

   auto pos = lines[0].begin;
   auto ok = (code_.find(DoubleRule, pos) == pos);
   if(!ok) return LogLine(0, HeadingNotStandard);

   pos = lines[1].begin;
   ok = (code_.find(COMMENT_STR, pos) == pos);
   ok = ok && (lines[1].type == EmptyComment);
   if(!ok) return LogLine(1, HeadingNotStandard);

   pos = lines[2].begin;
   ok = (code_.find(COMMENT_STR, pos) == pos);
   ok = ok && (code_.find(Name(), pos) == pos + 4);
   if(!ok) return LogLine(2, HeadingNotStandard);

   auto prolog = Prolog();
   size_t line = 3;

   for(size_t i = 0; i < prolog.size(); ++i)
   {
      pos = lines[line].begin;
      ok = ok && (code_.find(COMMENT_STR, pos) == pos);

      if(prolog[i].empty())
         ok = ok && (lines[line].type == EmptyComment);
      else
         ok = ok && (code_.find(prolog[i], pos) == pos + 4);

      if(!ok) return LogLine(line, HeadingNotStandard);
      ++line;
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

void CodeFile::CheckVerticalSpacing()
{
   Debug::ft("CodeFile.CheckVerticalSpacing");

   auto actions = lexer_.CheckVerticalSpacing();

   for(size_t n = 0; n < actions.size(); ++n)
   {
      switch(actions[n])
      {
      case Lexer::InsertBlank:
         LogLine(n, AddBlankLine);
         break;

      case Lexer::DeleteLine:
         LogLine(n, RemoveLine);
         break;
      }
   }
}

//------------------------------------------------------------------------------

void CodeFile::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   LibraryItem::Display(stream, prefix, options);

   stream << prefix << "name       : " << name_ << CRLF;
   stream << prefix << "dir        : " << dir_ << CRLF;
   stream << prefix << "isHeader   : " << isHeader_ << CRLF;
   stream << prefix << "isSubsFile : " << isSubsFile_ << CRLF;
   stream << prefix << "parsed     : " << parsed_ << CRLF;
   stream << prefix << "checked    : " << checked_ << CRLF;

   auto lead = prefix + spaces(2);

   stream << prefix << "inclSet : " << inclSet_.size() << CRLF;

   for(auto i = inclSet_.cbegin(); i != inclSet_.cend(); ++i)
   {
      auto f = static_cast< const CodeFile* >(*i);
      stream << lead << f->Name() << CRLF;
   }

   stream << prefix << "userSet : " << userSet_.size() << CRLF;

   for(auto u = userSet_.cbegin(); u != userSet_.cend(); ++u)
   {
      auto f = static_cast< const CodeFile* >(*u);
      stream << lead << f->Name() << CRLF;
   }

   stream << prefix << "lexer :" << CRLF;
   lexer_.Display(stream, lead, options);

   stream << prefix << "editor :" << CRLF;
   editor_.Display(stream, lead, options);
}

//------------------------------------------------------------------------------

void CodeFile::DisplayItems(ostream& stream, const string& opts) const
{
   if(dir_ == nullptr) return;

   stream << Path();
   if(parsed_ == Unparsed) stream << ": NOT PARSED";
   stream << CRLF;
   if(parsed_ == Unparsed) return;

   auto lead = spaces(IndentSize());
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
      for(auto i = items_.cbegin(); i != items_.cend(); ++i)
      {
         (*i)->Display(stream, lead, options);
      }
      stream << '}' << CRLF;
   }
}

//------------------------------------------------------------------------------

void CodeFile::EraseClass(Class* cls)
{
   CodeTools::EraseItem(classes_, cls);
   EraseItem(cls);
}

//------------------------------------------------------------------------------

void CodeFile::EraseData(Data* data)
{
   CodeTools::EraseItem(data_, data);
   EraseItem(data);
}

//------------------------------------------------------------------------------

void CodeFile::EraseEnum(const Enum* item)
{
   CodeTools::EraseItem(enums_, item);
   EraseItem(item);
}

//------------------------------------------------------------------------------

void CodeFile::EraseForw(const Forward* forw)
{
   CodeTools::EraseItem(forws_, forw);
   EraseItem(forw);
}

//------------------------------------------------------------------------------

void CodeFile::EraseFunc(Function* func)
{
   CodeTools::EraseItem(funcs_, func);
   EraseItem(func);
}

//------------------------------------------------------------------------------

void CodeFile::EraseInclude(const Include* incl)
{
   //* This should update this file's inclSet_, trimSet_, affecterSet_, and
   //  the userSet_ of the file no longer #included.
   //
   EraseItemPtr(incls_, incl);
   EraseItem(incl);
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

void CodeFile::EraseItem(const CxxNamed* item)
{
   for(auto i = items_.cbegin(); i != items_.cend(); ++i)
   {
      if(*i == item)
      {
         items_.erase(i);
         return;
      }
   }
}

//------------------------------------------------------------------------------

void CodeFile::EraseSpace(const SpaceDefn* space)
{
   EraseItemPtr(spaces_, space);
   EraseItem(space);
}

//------------------------------------------------------------------------------

void CodeFile::EraseType(const Typedef* type)
{
   CodeTools::EraseItem(types_, type);
   EraseItem(type);
}

//------------------------------------------------------------------------------

void CodeFile::EraseUsing(const Using* use)
{
   CodeTools::EraseItem(usings_, use);
   EraseItem(use);
}

//------------------------------------------------------------------------------

Data* CodeFile::FindData(const string& name) const
{
   Debug::ft("CodeFile.FindData");

   for(auto d = data_.cbegin(); d != data_.cend(); ++d)
   {
      if((*d)->Name() == name) return *d;
   }

   return nullptr;
}

//------------------------------------------------------------------------------

void CodeFile::FindDeclSet()
{
   Debug::ft("CodeFile.FindDeclSet");

   //  If this is a .cpp, find declSet_, the headers that declare items that
   //  the .cpp defines.  Also find classSet_, the transitive base classes of
   //  the classes that the .cpp implements.
   //
   if(!IsCpp()) return;

   for(auto f = funcs_.cbegin(); f != funcs_.cend(); ++f)
   {
      auto file = (*f)->GetDistinctDeclFile();
      if(file != nullptr) declSet_.insert(file);

      auto c = (*f)->GetClass();
      if(c != nullptr)
      {
         for(auto b = c->BaseClass(); b != nullptr; b = b->BaseClass())
         {
            classSet_.insert(b->GetDeclFile());
         }
      }
   }

   for(auto d = data_.cbegin(); d != data_.cend(); ++d)
   {
      auto file = (*d)->GetDistinctDeclFile();
      if(file != nullptr) declSet_.insert(file);

      auto c = (*d)->GetClass();
      if(c != nullptr)
      {
         for(auto b = c->BaseClass(); b != nullptr; b = b->BaseClass())
         {
            classSet_.insert(b->GetDeclFile());
         }
      }
   }
}

//------------------------------------------------------------------------------

CodeWarning* CodeFile::FindLog
   (const CodeWarning& log, const CxxToken* item, word offset)
{
   Debug::ft("CodeFile.FindLog");

   editor_.Setup(this);
   return editor_.FindLog(log.GetWarning(), item, offset);
}

//------------------------------------------------------------------------------

SpaceDefn* CodeFile::FindNamespaceDefn(const CxxToken* item) const
{
   Debug::ft("CodeFile.FindNamespaceDefn");

   if(item->GetFile() != this) return nullptr;

   SpaceDefn* ns = nullptr;
   auto pos = item->GetPos();

   for(auto s = spaces_.cbegin(); s != spaces_.cend(); ++s)
   {
      if((*s)->GetPos() < pos) ns = s->get();
   }

   return ns;
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
      use->SetLoc(this, string::npos);
      scope->AddUsing(use);
   }
}

//------------------------------------------------------------------------------

Using* CodeFile::FindUsingFor(const string& fqName, size_t prefix,
   const CxxScoped* item, const CxxScope* scope)
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
      search.insert(item->GetFile());
   }

   for(auto f = search.cbegin(); f != search.cend(); ++f)
   {
      auto file = static_cast< const CodeFile* >(*f);
      u = file->GetUsingFor(fqName, prefix, item, scope);
      if(u != nullptr) return u;
   }

   return nullptr;
}

//------------------------------------------------------------------------------

word CodeFile::Fix(CliThread& cli, const FixOptions& opts, string& expl)
{
   Debug::ft("CodeFile.Fix");

   if(!CodeWarning::HasWarning(this, opts.warning)) return 0;

   editor_.Setup(this);
   auto rc = editor_.Fix(cli, opts, expl);

   if(rc >= -1) return 0;  // continue with other files
   return rc;              // don't continue with other files
}

//------------------------------------------------------------------------------

word CodeFile::Format(string& expl)
{
   Debug::ft("CodeFile.Format");

   Debug::Progress(Name() + CRLF);

   editor_.Setup(this);
   return editor_.Format(expl);
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

void CodeFile::GetDecls(CxxNamedSet& items)
{
   Debug::ft("CodeFile.GetDecls");

   for(auto i = items_.cbegin(); i != items_.cend(); ++i)
   {
      (*i)->GetDecls(items);
   }
}

//------------------------------------------------------------------------------

Editor& CodeFile::GetEditor()
{
   Debug::ft("CodeFile.GetEditor");

   editor_.Setup(this);
   return editor_;
}

//------------------------------------------------------------------------------

const Lexer& CodeFile::GetLexer() const
{
   return (editor_.IsInitialized() ? editor_ : lexer_);
}

//------------------------------------------------------------------------------

void CodeFile::GetLineCounts() const
{
   //  Don't count lines in substitute files.
   //
   if(isSubsFile_) return;

   const auto& lines = lexer_.GetLinesInfo();

   CodeWarning::AddLineType(AnyLine, lines.size());

   for(size_t n = 0; n < lines.size(); ++n)
   {
      CodeWarning::AddLineType(lines[n].type, 1);
   }
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
      for(auto d = declSet_.cbegin(); d != declSet_.cend(); ++d)
      {
         auto file = static_cast< const CodeFile* >(*d);
         auto classes = file->Classes();

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

const LibItemSet& CodeFile::Implementers()
{
   Debug::ft("CodeFile.Implementers");

   //  If implSet_ is empty, build it.
   //
   if(!implSet_.empty()) return implSet_;

   auto fileSet = new CodeFileSet(LibrarySet::TemporaryName(), nullptr);
   auto& imSet = fileSet->Items();
   imSet.insert(this);

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

   implSet_ = imSet;
   return implSet_;
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
   InsertItem(code);
   assembly_.push_back(code);
}

//------------------------------------------------------------------------------

void CodeFile::InsertClass(Class* cls)
{
   InsertItem(cls);
   classes_.push_back(cls);
}

//------------------------------------------------------------------------------

void CodeFile::InsertData(Data* data)
{
   InsertItem(data);
   data_.push_back(data);
}

//------------------------------------------------------------------------------

bool CodeFile::InsertDirective(DirectivePtr& dir)
{
   InsertItem(dir.get());
   dirs_.push_back(std::move(dir));
   return true;
}

//------------------------------------------------------------------------------

void CodeFile::InsertEnum(Enum* item)
{
   InsertItem(item);
   enums_.push_back(item);
}

//------------------------------------------------------------------------------

void CodeFile::InsertForw(Forward* forw)
{
   InsertItem(forw);
   forws_.push_back(forw);
}

//------------------------------------------------------------------------------

void CodeFile::InsertFunc(Function* func)
{
   InsertItem(func);
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
      if((*i)->Name() == fn)
      {
         InsertItem(i->get());
         return i->get();
      }
   }

   Context::SwLog(CodeFile_InsertInclude, fn, 0);
   return nullptr;
}

//------------------------------------------------------------------------------

void CodeFile::InsertItem(CxxNamed* item)
{
   //  Optimize for initial compilation, when items are always added at the end.
   //
   auto pos = item->GetPos();

   if(!items_.empty() && (items_.back()->GetPos() > pos))
   {
      for(auto i = items_.cbegin(); i != items_.cend(); ++i)
      {
         if((*i)->GetPos() > pos)
         {
            items_.insert(i, item);
            return;
         }
      }
   }

   items_.push_back(item);
}

//------------------------------------------------------------------------------

void CodeFile::InsertMacro(Macro* macro)
{
   InsertItem(macro);
   macros_.push_back(macro);
}

//------------------------------------------------------------------------------

void CodeFile::InsertSpace(SpaceDefnPtr& space)
{
   InsertItem(space.get());
   spaces_.push_back(std::move(space));
}

//------------------------------------------------------------------------------

void CodeFile::InsertStaticAssert(StaticAssert* assert)
{
   InsertItem(assert);
   asserts_.push_back(assert);
}

//------------------------------------------------------------------------------

void CodeFile::InsertType(Typedef* type)
{
   InsertItem(type);
   types_.push_back(type);
}

//------------------------------------------------------------------------------

void CodeFile::InsertUsing(Using* use)
{
   InsertItem(use);
   usings_.push_back(use);
}

//------------------------------------------------------------------------------

bool CodeFile::IsLastItem(const CxxNamed* item) const
{
   Debug::ft("CodeFile.IsLastItem");

   return (items_.empty() ? false : (items_.back() == item));
}

//------------------------------------------------------------------------------

fn_name CodeFile_LogAddForwards = "CodeFile.LogAddForwards";

void CodeFile::LogAddForwards(ostream* stream, const CxxNamedSet& items)
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
      LogPos(string::npos, ForwardAdd, nullptr, 0, *n);
   }

   DisplaySymbols(stream, items, "Add a forward declaration for");
}

//------------------------------------------------------------------------------

void CodeFile::LogAddIncludes(ostream* stream, const LibItemSet& files)
{
   Debug::ft("CodeFile.LogAddIncludes");

   //  For each file in FIDS, generate a log saying that an #include for
   //  it should be added.  Put the filename in angle brackets or quotes,
   //  whichever is appropriate.
   //
   for(auto f = files.cbegin(); f != files.cend(); ++f)
   {
      string fn;
      auto file = static_cast< const CodeFile* >(*f);
      auto x = file->IsSubsFile();
      fn.push_back(x ? '<' : QUOTE);
      fn += file->Name();
      fn.push_back(x ? '>' : QUOTE);
      LogPos(string::npos, IncludeAdd, nullptr, 0, fn);
   }

   DisplayFileNames(stream, files, "Add an #include for");
}

//------------------------------------------------------------------------------

void CodeFile::LogAddUsings(ostream* stream)
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

         LogPos(string::npos, UsingAdd, nullptr, 0, name);
      }
   }

   DisplaySymbols(stream, usings, "Add a using statement for");
}

//------------------------------------------------------------------------------

void CodeFile::LogCode(Warning warning, size_t pos,
   const CxxToken* item, word offset, const string& info)
{
   Debug::ft("CodeFile.LogCode");

   //  Don't log warnings in a substitute file or a template instance.
   //  When a template instance is being compiled, however, it might
   //  log a warning against an item defined in a regular file.
   //
   if(isSubsFile_) return;

   if(item != nullptr)
   {
      if(item->IsInternal()) return;
   }
   else
   {
      if(Context::ParsingTemplateInstance()) return;
   }

   CodeWarning log(warning, this, pos, item, offset, info);
   log.Insert();
}

//------------------------------------------------------------------------------

void CodeFile::LogLine(size_t line, Warning warning)
{
   Debug::ft("CodeFile.LogLine");

   auto pos = lexer_.GetLineStart(line);
   LogCode(warning, pos, nullptr);
}

//------------------------------------------------------------------------------

void CodeFile::LogPos(size_t pos, Warning warning,
   const CxxToken* item, word offset, const string& info)
{
   Debug::ft("CodeFile.LogPos");

   LogCode(warning, pos, item, offset, info);
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
            (*f)->Log(ForwardRemove);
      }
   }

   DisplaySymbols(stream, items, "Remove the forward declaration for");
}

//------------------------------------------------------------------------------

void CodeFile::LogRemoveIncludes(ostream* stream, const LibItemSet& files) const
{
   Debug::ft("CodeFile.LogRemoveIncludes");

   //  For each file in FIDS, generate a log saying that the #include for
   //  it should be removed.
   //
   for(auto f = files.cbegin(); f != files.cend(); ++f)
   {
      const auto& name = static_cast< const CodeFile* >(*f)->Name();

      for(auto i = incls_.cbegin(); i != incls_.cend(); ++i)
      {
         if((*i)->Name() == name)
         {
            (*i)->Log(IncludeRemove);
         }
      }
   }

   DisplayFileNames(stream, files, "Remove the #include for");
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
         (*u)->Log(UsingRemove);
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
   "Copyright (C) 2013-2021  Greg Utas",
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
   const LibItemSet& trimSet, CxxNamedSet& addForws) const
{
   Debug::ft("CodeFile.PruneForwardCandidates");

   //  Go through the possible forward declarations and remove those that
   //  are not needed.  Start by finding the files that affect (that is, are
   //  transitively #included by) trimSet (the files that will be #included).
   //
   auto inclSet = new CodeFileSet(LibrarySet::TemporaryName(), nullptr);
   auto& incls = inclSet->Items();
   SetUnion(incls, trimSet);
   auto affecters = inclSet->Affecters();
   auto& affecterSet = affecters->Items();

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
         //  Do not add a forward declaration for a type that will be #included,
         //  even transitively.
         //
         for(auto a = affecterSet.cbegin(); a != affecterSet.cend(); ++a)
         {
            remove = (*a == addFile);
            if(remove) break;
         }

         //  Do not add a forward declaration for a type that is already forward
         //  declared in a file that will be #included, even transitively.
         //
         if(!remove)
         {
            for(auto a = affecterSet.cbegin(); a != affecterSet.cend(); ++a)
            {
               auto incl = static_cast< const CodeFile* >(*a);

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

bool CodeFile::ReadCode(string& code) const
{
   Debug::ft("CodeFile.ReadCode");

   auto input = InputStream();
   if(input == nullptr) return false;
   input->clear();
   input->seekg(0);

   string str;

   while(input->peek() != EOF)
   {
      std::getline(*input, str);
      code += str;
      code.push_back(CRLF);
   }

   input.reset();
   return true;
}

//------------------------------------------------------------------------------

void CodeFile::RemoveHeaders(LibItemSet& inclSet) const
{
   Debug::ft("CodeFile.RemoveHeaders");

   //  If this is a .cpp, it implements items declared one or more headers
   //  (declSet_).  The .cpp need not #include anything that one of those
   //  headers will already #include.
   //
   if(IsCpp())
   {
      for(auto d = declSet_.cbegin(); d != declSet_.cend(); ++d)
      {
         auto file = static_cast< const CodeFile* >(*d);
         SetDifference(inclSet, file->TrimList());
      }

      //  Ensure that all files in declSet_ are #included.  It is possible for
      //  one to get dropped if, for example, the .cpp uses a subclass of an
      //  item that it implements.
      //
      SetUnion(inclSet, declSet_);
   }
}

//------------------------------------------------------------------------------

void CodeFile::RemoveInvalidIncludes(LibItemSet& addSet)
{
   Debug::ft("CodeFile.RemoveInvalidIncludes");

   //  A file should not #include
   //  (a) itself
   //  (b) a file that it affects (a file that transitively #includes it)
   //  (c) a .cpp
   //  The latter two can occur when a template uses one of its instances
   //  to resolve symbols accessed through a template parameter.
   //
   addSet.erase(this);

   for(auto f = addSet.cbegin(); f != addSet.cend(); NO_OP)
   {
      auto file = static_cast< CodeFile* >(*f);

      if(file != nullptr)
      {
         auto& affecters = file->Affecters();

         if(file->IsCpp() || (affecters.find(this) != affecters.cend()))
            f = addSet.erase(f);
         else
            ++f;
      }
   }
}

//------------------------------------------------------------------------------

void CodeFile::SaveBaseSet(const CxxNamedSet& bases)
{
   Debug::ft("CodeFile.SaveBaseSet");

   for(auto b = bases.cbegin(); b != bases.cend(); ++b)
   {
      auto base = static_cast< const Class* >(*b);
      baseSet_.insert(base->GetDeclFile());
   }
}

//------------------------------------------------------------------------------

void CodeFile::Scan()
{
   Debug::ft("CodeFile.Scan");

   if(!code_.empty()) return;
   if(!ReadCode(code_)) return;
   lexer_.Initialize(code_, this);
   lexer_.CalcLineTypes(true);
   lexer_.CalcDepths();
   lexer_.CheckPunctuation();

   //  Preprocess #include directives.
   //
   auto lines = lexer_.LineCount();
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
            inclSet_.insert(used);
            trimSet_.insert(used);
            used->AddUser(this);
         }

         IncludePtr incl(new Include(file, angle));
         incl->SetLoc(this, lexer_.GetLineStart(n), false);
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
   size += usages_.size() * 3 * sizeof(CxxNamed*);
   CxxStats::Vectors(CxxStats::CODE_FILE, size);
}

//------------------------------------------------------------------------------

void CodeFile::Trim(ostream* stream)
{
   Debug::ft("CodeFile.Trim");

   //  If this file should be trimmed, find the headers that declare items
   //  that this file defines, and assemble information about the symbols
   //  that this file uses.  FindDeclSet must be invoked before GetUsageInfo
   //  because the latter uses declSet_.
   //
   if(!CanBeTrimmed()) return;

   if(stream != nullptr) *stream << Name() << CRLF;
   FindDeclSet();

   CxxUsageSets symbols;
   GetUsageInfo(symbols);
   auto& bases = symbols.bases;
   auto& directs = symbols.directs;
   auto& indirects = symbols.indirects;
   auto& forwards = symbols.forwards;
   auto& friends = symbols.friends;
   auto& users = symbols.users;

   SaveBaseSet(bases);

   //  Remove direct and indirect symbols declared by the file itself.
   //  Find inclTypes, the types that were used directly or in executable
   //  code.  It will contain all the types that should be #included.
   //
   CxxNamedSet inclTypes;
   EraseInternals(directs);
   EraseInternals(indirects);
   AddDirectTypes(directs, inclTypes);

   //  Display the symbols that the file uses.
   //
   DisplaySymbolsAndFiles(stream, bases, "Base usage:");
   DisplaySymbolsAndFiles(stream, inclTypes, "Direct usage:");
   DisplaySymbolsAndFiles(stream, indirects, "Indirect usage:");
   DisplaySymbolsAndFiles(stream, forwards, "Forward usage:");
   DisplaySymbolsAndFiles(stream, friends, "Friend usage:");

   //  Expand inclTypes with types used indirectly but defined outside
   //  the code base.  Then expand it with forward declarations that
   //  resolved an indirect reference in this file.
   //
   AddIndirectExternalTypes(indirects, inclTypes);
   AddForwardDependencies(symbols, inclTypes);

   //  A derived class must #include its base class, so shrink inclTypes
   //  by removing items defined in an indirect base class.  Then shrink
   //  it by removing items defined in a base class of another item that
   //  will be #included.  Finally, shrink it again by removing classes
   //  that are named in a typedef that will be #included.
   //
   RemoveIndirectBaseItems(bases, inclTypes);
   RemoveIncludedBaseItems(inclTypes);
   RemoveAliasedClasses(inclTypes);

   //  For a .cpp, BASES includes base classes defined in its header.
   //  Regenerate it so that it is limited to base classes for those
   //  declared in the .cpp.  Before doing this, find the files that
   //  define base classes, including transitive base classes, of
   //  classes defined or implemented in this file.  This set (tBaseSet)
   //  will be needed later, when analyzing using statements.
   //
   LibItemSet tBaseSet;
   GetTransitiveBases(bases, tBaseSet);
   if(IsCpp()) GetDeclaredBaseClasses(bases);

   //  An #include should always appear for a base class.  Add them to
   //  inclTypes in case any of them were removed.
   //
   for(auto b = bases.cbegin(); b != bases.cend(); ++b)
   {
      inclTypes.insert(*b);
   }

   //  Assemble inclSet, all the files that need to be #included.  It
   //  starts with all of the files (excluding this one) that declare
   //  items in inclTypes.  A .cpp can then remove any file that will be
   //  #included by a header that declares an item that the .cpp defines.
   //
   LibItemSet inclSet;
   AddIncludes(inclTypes, inclSet);
   RemoveHeaders(inclSet);

   //  Output addSet, the #includes that should be added.  It contains
   //  inclSet, minus any spurious files and what is already #included.
   //
   LibItemSet addSet;
   SetDifference(addSet, inclSet, inclSet_);
   RemoveInvalidIncludes(addSet);
   LogAddIncludes(stream, addSet);

   //  Output delSet, which are the #includes that should be removed.  It
   //  contains what is already #included, minus everything that needs to
   //  be #included.
   //
   LibItemSet delSet;
   SetDifference(delSet, inclSet_, inclSet);
   LogRemoveIncludes(stream, delSet);

   //  Save the files that *should* be #included.
   //
   trimSet_.clear();
   SetUnion(trimSet_, inclSet_, addSet);
   SetDifference(trimSet_, delSet);

   //  Now determine the forward declarations that should be added.  Types
   //  used indirectly, as well as friend declarations, provide candidates.
   //  The candidates are then trimmed by removing, for example, forward
   //  declarations in other files that resolved symbols in this file.
   //
   CxxNamedSet addForws;
   FindForwardCandidates(symbols, addForws);
   PruneForwardCandidates(forwards, inclSet, addForws);

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

//------------------------------------------------------------------------------

void CodeFile::UpdatePos
   (EditorAction action, size_t begin, size_t count, size_t from)
{
   for(auto i = items_.cbegin(); i != items_.cend(); ++i)
   {
      (*i)->UpdatePos(action, begin, count, from);
   }

   items_.sort(IsSortedByPos);
}
}
