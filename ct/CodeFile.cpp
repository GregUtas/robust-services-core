//==============================================================================
//
//  CodeFile.cpp
//
//  Copyright (C) 2017  Greg Utas
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
#include <cstring>
#include <istream>
#include <iterator>
#include <sstream>
#include <utility>
#include "Algorithms.h"
#include "CliThread.h"
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
#include "Editor.h"
#include "Formatters.h"
#include "FunctionName.h"
#include "Library.h"
#include "NbCliParms.h"
#include "Parser.h"
#include "Registry.h"
#include "SetOperations.h"
#include "Singleton.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace CodeTools
{
fn_name CodeTools_AddForwardDependencies = "CodeTools.AddForwardDependencies";

void AddForwardDependencies(const CxxUsageSets& symbols, CxxNamedSet& inclSet)
{
   Debug::ft(CodeTools_AddForwardDependencies);

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
      auto name = (*i)->ScopedName(false);
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

fn_name CodeTools_FindForwardCandidates = "CodeTools.FindForwardCandidates";

void FindForwardCandidates(const CxxUsageSets& symbols, CxxNamedSet& addForws)
{
   Debug::ft(CodeTools_FindForwardCandidates);

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

fn_name CodeTools_GetTransitiveBases = "CodeTools.GetTransitiveBases";

void GetTransitiveBases(const CxxNamedSet& bases, SetOfIds& tBaseIds)
{
   Debug::ft(CodeTools_GetTransitiveBases);

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

fn_name CodeTools_RemoveAliasedClasses = "CodeTools.RemoveAliasedClasses";

void RemoveAliasedClasses(CxxNamedSet& inclSet)
{
   Debug::ft(CodeTools_RemoveAliasedClasses);

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

fn_name CodeTools_RemoveIncludedBaseItems = "CodeTools.RemoveIncludedBaseItems";

void RemoveIncludedBaseItems(CxxNamedSet& inclSet)
{
   Debug::ft(CodeTools_RemoveIncludedBaseItems);

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

fn_name CodeTools_RemoveIndirectBaseItems = "CodeTools.RemoveIndirectBaseItems";

void RemoveIndirectBaseItems(const CxxNamedSet& bases, CxxNamedSet& inclSet)
{
   Debug::ft(CodeTools_RemoveIndirectBaseItems);

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

fn_name CodeFile_ctor = "CodeFile.ctor";

CodeFile::CodeFile(const string& name, CodeDir* dir) : LibraryItem(name),
   dir_(dir),
   isHeader_(false),
   isSubsFile_(false),
   slashAsterisk_(false),
   parsed_(Unparsed),
   location_(FuncNoTemplate),
   checked_(false),
   modified_(false)
{
   Debug::ft(CodeFile_ctor);

   isHeader_ = (name.find(".c") == string::npos);
   isSubsFile_ = (dir != nullptr) && dir->IsSubsDir();
   Singleton< Library >::Instance()->AddFile(*this);
   CxxStats::Incr(CxxStats::CODE_FILE);
}

//------------------------------------------------------------------------------

fn_name CodeFile_dtor = "CodeFile.dtor";

CodeFile::~CodeFile()
{
   Debug::ft(CodeFile_dtor);

   CxxStats::Decr(CxxStats::CODE_FILE);
}

//------------------------------------------------------------------------------

fn_name CodeFile_AddDirectTypes = "CodeFile.AddDirectTypes";

void CodeFile::AddDirectTypes
   (const CxxNamedSet& directs, CxxNamedSet& inclSet) const
{
   Debug::ft(CodeFile_AddDirectTypes);

   //  SYMBOLS contains types that were used directly.  Types in executable
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

fn_name CodeFile_AddIncludeIds = "CodeFile.AddIncludeIds";

void CodeFile::AddIncludeIds
   (const CxxNamedSet& inclSet, SetOfIds& inclIds) const
{
   Debug::ft(CodeFile_AddIncludeIds);

   auto thisFid = Fid();

   for(auto n = inclSet.cbegin(); n != inclSet.cend(); ++n)
   {
      auto fid = (*n)->GetDeclFid();
      if((fid != NIL_ID) && (fid != thisFid)) inclIds.insert(fid);
   }
}

//------------------------------------------------------------------------------

fn_name CodeFile_AddIndirectExternalTypes = "CodeFile.AddIndirectExternalTypes";

void CodeFile::AddIndirectExternalTypes
   (const CxxNamedSet& indirects, CxxNamedSet& inclSet) const
{
   Debug::ft(CodeFile_AddIndirectExternalTypes);

   //  SYMBOLS contains types that were used indirectly.  Filter out those
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

fn_name CodeFile_AddUsage = "CodeFile.AddUsage";

void CodeFile::AddUsage(const CxxNamed* item)
{
   Debug::ft(CodeFile_AddUsage);

   usages_.insert(item);
}

//------------------------------------------------------------------------------

fn_name CodeFile_AddUser = "CodeFile.AddUser";

void CodeFile::AddUser(const CodeFile* file)
{
   Debug::ft(CodeFile_AddUser);

   userIds_.insert(file->Fid());
}

//------------------------------------------------------------------------------

fn_name CodeFile_Affecters = "CodeFile.Affecters";

const SetOfIds& CodeFile::Affecters() const
{
   Debug::ft(CodeFile_Affecters);

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

fn_name CodeFile_CanBeTrimmed = "CodeFile.CanBeTrimmed";

bool CodeFile::CanBeTrimmed(ostream* stream) const
{
   Debug::ft(CodeFile_CanBeTrimmed);

   //  Don't trim empty files, substitute files, or unexecuted files
   //  (template headers).
   //
   if(code_.empty()) return false;
   if(isSubsFile_) return false;
   if(stream == nullptr) return (location_ != FuncInTemplate);

   *stream << Name() << CRLF;

   switch(location_)
   {
   case FuncInTemplate:
      *stream << spaces(3) << "OMITTED: mostly unexecuted." << CRLF;
      return false;
   case FuncIsTemplate:
      *stream << spaces(3) << "WARNING: partially unexecuted." << CRLF;
   }

   return true;
}

//------------------------------------------------------------------------------

ptrdiff_t CodeFile::CellDiff()
{
   int local;
   auto fake = reinterpret_cast< const CodeFile* >(&local);
   return ptrdiff(&fake->fid_, fake);
}

//------------------------------------------------------------------------------

fn_name CodeFile_Check = "CodeFile.Check";

void CodeFile::Check()
{
   Debug::ft(CodeFile_Check);

   if(checked_) return;

   Debug::Progress(Name(), true);

   //  Don't check an empty file or a substitute file.
   //
   if(code_.empty() || isSubsFile_)
   {
      checked_ = true;
      return;
   }

   for(auto u = usings_.cbegin(); u != usings_.cend(); ++u)
   {
      (*u)->Check();
   }

   CheckProlog();
   CheckIncludeGuard();
   CheckIncludeOrder();
   CheckUsings();
   CheckSeparation();
   CheckFunctionOrder();
   CheckDebugFt();
   Trim(nullptr);
   checked_ = true;
}

//------------------------------------------------------------------------------

fn_name CodeFile_CheckDebugFt = "CodeFile.CheckDebugFt";

void CodeFile::CheckDebugFt() const
{
   Debug::ft(CodeFile_CheckDebugFt);

   //  Functions in a header are not expected to invoke Debug::ft unless
   //  this is a template header.
   //
   if(IsHeader() && !IsTemplateHeader()) return;

   size_t begin, end;
   string s;

   //  For each function in this file, find the lines on which it begins
   //  and ends.  Within those lines, look for invocations of Debug::ft.
   //  When one is found, find the data member that is passed to Debug::ft
   //  and see if it defines a string literal that accurately identifies
   //  the function.  Also note an invocation of Debug::ft that is missing
   //  or that is not the first line of code in the function.
   //
   for(auto f = funcs_.cbegin(); f != funcs_.cend(); ++f)
   {
      if((*f)->IsInTemplateInstance()) continue;
      (*f)->GetDefnRange(begin, end);
      if(begin >= end) continue;
      auto last = lexer_.GetLineNum(end);
      auto open = false, debug = false, code = false;

      for(auto n = lexer_.GetLineNum(begin); n < last; ++n)
      {
         switch(lineType_[n])
         {
         case OpenBrace:
            open = true;
            break;

         case DebugFt:
            debug = true;
            if(code) LogLine(n, DebugFtNotFirst);
            code = true;

            if(lexer_.GetNthLine(n, s))
            {
               auto prev = s.find('(');
               if(prev == string::npos) break;
               auto next = s.find(')', prev);
               if(next == string::npos) break;
               auto name = s.substr(prev + 1, next - prev - 1);
               auto data = FindData(name);
               if(data == nullptr) break;
               if(data->CheckFunctionString(*f)) break;
               LogLine(n, DebugFtNameMismatch);
            }
            break;

         case Code:
            if(open) code = true;
            break;
         }
      }

      if(!debug && !((*f)->IsExemptFromTracing()))
      {
         LogPos(begin, DebugFtNotInvoked);
      }
   }
}

//------------------------------------------------------------------------------

fn_name CodeFile_CheckFunctionOrder = "CodeFile.CheckFunctionOrder";

void CodeFile::CheckFunctionOrder() const
{
   Debug::ft(CodeFile_CheckFunctionOrder);

   if(IsHeader() || funcs_.empty()) return;

   CxxScope* scope = nullptr;
   auto state = FuncCtor;
   const string* prev = nullptr;
   const string* curr = nullptr;
   std::set< Function* > forwards;

   for(auto f = funcs_.cbegin(); f != funcs_.cend(); ++f)
   {
      //  Skip functions created in template instances, which are added to
      //  the file that caused their instantiation.
      //
      if((*f)->IsInTemplateInstance()) continue;

      //  Functions only need to be sorted until a function in a new scope
      //  is encountered.
      //
      if((*f)->GetScope() != scope)
      {
         scope = (*f)->GetScope();
         state = FuncCtor;
         prev = nullptr;
         curr = nullptr;
         forwards.clear();
      }

      //  If a function is declared forward in a .cpp, add it to FORWARDS
      //  the first time (its declaration) so that it can be handled when
      //  its definition appears.
      //
      if(((*f)->GetDeclFile() == this) &&
         (forwards.find(*f) == forwards.end()))
      {
         forwards.insert(*f);
      }
      else
      {
         switch(state)
         {
         case FuncCtor:
            switch((*f)->FuncType())
            {
            case FuncOperator:
            case FuncStandard:
               prev = (*f)->Name();
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
}

//------------------------------------------------------------------------------

fn_name CodeFile_CheckIncludeGuard = "CodeFile.CheckIncludeGuard";

void CodeFile::CheckIncludeGuard()
{
   Debug::ft(CodeFile_CheckIncludeGuard);

   if(IsCpp()) return;

   size_t pos = string::npos;
   size_t n;

   for(n = 0; (n < lineType_.size()) && (pos == string::npos); ++n)
   {
      switch(lineType_[n])
      {
      case EmptyComment:
      case LicenseComment:
      case TextComment:
      case SeparatorComment:
      case TaggedComment:
      case Blank:
         continue;
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
      LogLine(n, IncludeGuardMissing);
      return;
   }

   lexer_.Reposition(pos + strlen(HASH_IFNDEF_STR));

   //  Assume that this is an include guard.  Check that its name is
   //  derived from the filename. For FileName.h, the guard should
   //  be FILENAME_H_INCLUDED.
   //
   auto name = Name();
   auto symbol = lexer_.NextIdentifier();

   for(size_t i = 0; i < name.size(); ++i)
   {
      if(name[i] == '.')
         name[i] = '_';
      else
         name[i] = toupper(name[i]);
   }

   name += "_INCLUDED";
   if(symbol != name) LogLine(n, IncludeGuardMisnamed);
}

//------------------------------------------------------------------------------

fn_name CodeFile_CheckIncludeOrder = "CodeFile.CheckIncludeOrder";

void CodeFile::CheckIncludeOrder() const
{
   Debug::ft(CodeFile_CheckIncludeOrder);

   const Include* prev = nullptr;

   for(auto i1 = incls_.cbegin(); i1 != incls_.cend(); ++i1)
   {
      auto i2 = std::next(i1);

      //  After the first #include, they should be sorted alphabetically,
      //  with those in angle brackets first.
      //
      if(i2 != incls_.cend())
      {
         //  PREV is the #include directive preceding this one.  This one
         //  is not in the desired sort order if
         //  o it is in angle brackets, whereas PREV is in quotes;
         //  o it precedes PREV alphabetically, unless PREV is in angle
         //    brackets and this one is in quotes.
         //
         if(prev != nullptr)
         {
            auto a1 = prev->InAngleBrackets();
            auto a2 = (*i2)->InAngleBrackets();
            auto err = !a1 && a2;

            if(a1 == a2)
            {
               err = err || (strCompare(*(*i2)->Name(), *prev->Name()) < 0);
            }

            if(err) LogPos((*i2)->GetPos(), IncludeNotSorted);
         }

         prev = i2->get();
      }

      //  Look for a duplicated #include.
      //
      for(NO_OP; i2 != incls_.cend(); ++i2)
      {
         if(*(*i1)->Name() == *(*i2)->Name())
         {
            LogPos((*i2)->GetPos(), IncludeDuplicated);
         }
      }
   }
}

//------------------------------------------------------------------------------

const size_t FilePrologSize = 18;

fixed_string FileProlog[FilePrologSize] =
{
   EMPTY_STR,
   "Copyright (C) 2017  Greg Utas",
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

fn_name CodeFile_CheckProlog = "CodeFile.CheckProlog";

void CodeFile::CheckProlog()
{
   Debug::ft(CodeFile_CheckProlog);

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
   ok = ok && (code_.find(COMMENT_STR, pos) == pos);
   ok = ok && (lineType_[1] == EmptyComment);
   if(!ok) return LogLine(1, HeadingNotStandard);

   pos = lexer_.GetLineStart(2);
   ok = ok && (code_.find(COMMENT_STR, pos) == pos);
   ok = ok && (code_.find(Name(), pos) == pos + 4);
   if(!ok) return LogLine(2, HeadingNotStandard);
   lineType_[2] = LicenseComment;

   size_t line = 3;

   for(auto i = 0; i < FilePrologSize; ++i)
   {
      pos = lexer_.GetLineStart(line);
      ok = ok && (code_.find(COMMENT_STR, pos) == pos);

      if(FileProlog[i] == EMPTY_STR)
      {
         ok = ok && (lineType_[line] == EmptyComment);
      }
      else
      {
         ok = ok && (code_.find(FileProlog[i], pos) == pos + 4);
         if(ok) lineType_[line] = LicenseComment;
      }

      if(!ok) return LogLine(line, HeadingNotStandard);
      ++line;
   }
}

//------------------------------------------------------------------------------

fn_name CodeFile_CheckSeparation = "CodeFile.CheckSeparation";

void CodeFile::CheckSeparation()
{
   Debug::ft(CodeFile_CheckSeparation);

   //  Look for warnings that involve looking at adjacent lines or the
   //  file's contents as a whole.
   //
   LineType prevType = Blank;
   slashAsterisk_ = false;

   for(size_t n = 0; n < lineType_.size(); ++n)
   {
      //  Based on the type of line just found, look for warnings that can
      //  only be found based on the type of line that preceded this one.
      //
      auto nextType = (n == lineType_.size() - 1 ? Blank : lineType_[n + 1]);

      switch(lineType_[n])
      {
      case Code:
         switch(prevType)
         {
         case SeparatorComment:
         case FunctionName:
         case IncludeDirective:
         case UsingDirective:
            LogLine(n, InsertBlankLine);
            break;
         }
         break;

      case Blank:
      case EmptyComment:
         switch(prevType)
         {
         case Blank:
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
         switch(prevType)
         {
         case Blank:
         case EmptyComment:
            break;
         default:
            LogLine(n, InsertBlankLine);
         }

         switch(nextType)
         {
         case Blank:
         case EmptyComment:
            break;
         default:
            LogLine(n + 1, InsertBlankLine);
         }
         break;

      case OpenBrace:
      case CloseBrace:
      case CloseBraceSemicolon:
         switch(prevType)
         {
         case Blank:
         case EmptyComment:
            LogLine(n - 1, RemoveBlankLine);
            break;
         case SeparatorComment:
            LogLine(n, InsertBlankLine);
            break;
         }
         break;

      case FunctionName:
      case FunctionNameSplit:
         switch(prevType)
         {
         case Blank:
         case EmptyComment:
         case OpenBrace:
         case FunctionName:
         case FunctionNameSplit:
            break;
         case TextComment:
            if(IsCpp()) LogLine(n, InsertBlankLine);
            break;
         default:
            LogLine(n, InsertBlankLine);
         }
         break;
      }

      prevType = lineType_[n];
   }
}

//------------------------------------------------------------------------------

fn_name CodeFile_CheckUsings = "CodeFile.CheckUsings";

void CodeFile::CheckUsings() const
{
   Debug::ft(CodeFile_CheckUsings);

   //  Look for duplicated using statements.
   //
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

fn_name CodeFile_ClassifyLine = "CodeFile.ClassifyLine";

LineType CodeFile::ClassifyLine(size_t n)
{
   Debug::ft(CodeFile_ClassifyLine);

   //  Get the line's length.  An empty line can immediately be classified
   //  as a Blank.
   //
   string s;
   if(!lexer_.GetNthLine(n, s)) return LineType_N;

   auto length = s.size();
   if(length == 0) return Blank;

   //  If the line is too long, flag it unless it ends in a string literal
   //  (a quotation mark followed by a semicolon or, within a list, a comma).
   //
   if(length > 80)
   {
      auto end = s.substr(length - 2);
      if((end != "\";") && (end != "\",")) LogLine(n, LineLength);
   }

   //  Flag any tabs and convert them to spaces.
   //
   for(auto pos = s.find(TAB); pos != string::npos; pos = s.find(TAB))
   {
      LogLine(n, UseOfTab);
      s[pos] = SPACE;
   }

   //  Flag and strip trailing spaces.
   //
   if(s.find_first_not_of(SPACE) == string::npos)
   {
      LogLine(n, TrailingSpace);
      return Blank;
   }

   while(s.rfind(SPACE) == s.size() - 1)
   {
      LogLine(n, TrailingSpace);
      s.pop_back();
   }

   //  Flag a line that is not indented a multiple of the standard, unless
   //  it begins with a comment or string literal.
   //
   if(s.empty()) return Blank;
   auto pos = s.find_first_not_of(SPACE);
   if(pos > 0) s.erase(0, pos);

   if(pos % Indent_Size != 0)
   {
      if((s.front() != '/') && (s.front() != QUOTE)) LogLine(n, Indentation);
   }

   //  Now that the line has been reformatted, recalculate its length.
   //
   length = s.size();

   //  If a /* comment is open, check if this line closes it, and then
   //  classify it as a text comment.
   //
   if(slashAsterisk_)
   {
      if(s.find(COMMENT_END_STR) != string::npos) slashAsterisk_ = false;
      return TextComment;
   }

   //  Look for lines that contain nothing but a brace (or brace and semicolon).
   //
   if((s.front() == '{') && (length == 1)) return OpenBrace;

   if(s.front() == '}')
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
      if(s[2] != SPACE) return TaggedComment;   //@ [@ != one of above]
      return TextComment;                       //  text
   }

   //  Flag a /* comment and see if it ends on the same line.
   //
   pos = FindSubstr(s, COMMENT_START_STR);

   if(pos != string::npos)
   {
      LogLine(n, UseOfSlashAsterisk);
      if(s.find(COMMENT_END_STR) == string::npos) slashAsterisk_ = true;
      if(pos == 0) return SlashAsteriskComment;
   }

   //  Look for preprocessor directives (e.g. #include, #ifndef).
   //
   if(s.front() == '#')
   {
      pos = s.find(HASH_INCLUDE_STR);
      if(pos == 0) return IncludeDirective;
      return HashDirective;
   }

   //  Looking for using directives.
   //
   if(s.find("using ") == 0) return UsingDirective;

   //  Look for invocations of Debug::ft.
   //
   if(FindSubstr(s, "Debug::ft(") != string::npos) return DebugFt;

   //  Look for strings that provide function names for Debug::ft.  These
   //  have the format
   //    fn_name ClassName_FunctionName = "ClassName.FunctionName";
   //  with an endline after the '=' if the line would exceed 80 characters.
   //
   string type(FunctionName::TypeStr);
   type.push_back(SPACE);

   while(true)
   {
      if(s.find(type) != 0) break;
      auto begin1 = s.find_first_not_of(SPACE, type.size());
      if(begin1 == string::npos) break;
      auto under = s.find('_', begin1);
      if(begin1 == string::npos) break;
      auto equals = s.find('=', under);
      if(equals == string::npos) break;
      if(equals == length - 1) return FunctionNameSplit;

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
         LogLine(n, AdjacentSpaces);
      }
   }

   if((n > 0) && (lineType_[n - 1] == FunctionNameSplit)) return FunctionName;
   return Code;
}

//------------------------------------------------------------------------------

fn_name CodeFile_CreateEditor = "CodeFile.CreateEditor";

Editor* CodeFile::CreateEditor(word& rc, string& expl)
{
   Debug::ft(CodeFile_CreateEditor);

   //  Fail if
   //  (a) the file has already been modified
   //  (b) its directory is unknown
   //  (c) the file can't be opened.
   //
   if(modified_)
   {
      expl = "This file has already been modified.";
      rc = -1;
      return nullptr;
   }

   if(dir_ == nullptr)
   {
      expl = "Directory not specified.";
      rc = -2;
      return nullptr;
   }

   auto input = InputStream();
   if(input == nullptr)
   {
      expl = "Failed to open source code file.";
      rc = -7;
      return nullptr;
   }

   return new Editor(this, input);
}

//------------------------------------------------------------------------------

void CodeFile::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   LibraryItem::Display(stream, prefix, options);

   stream << prefix << "fid       : " << fid_.to_str() << CRLF;
   stream << prefix << "dir       : " << dir_ << CRLF;
   stream << prefix << "isHeader  : " << isHeader_ << CRLF;
   stream << prefix << "code      : " << &code_ << CRLF;
   stream << prefix << "parsed    : " << parsed_ << CRLF;
   stream << prefix << "checked   : " << checked_ << CRLF;
   stream << prefix << "modified  : " << modified_ << CRLF;

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

   auto none = Flags();

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

   stream << FullName();
   if(parsed_ == Unparsed) stream << ": NOT PARSED";
   stream << CRLF;
   if(parsed_ == Unparsed) return;

   auto lead = spaces(Indent_Size);
   auto options = Flags(FQ_Mask);
   if(opts.find(ItemStatistics) != string::npos) options.set(DispStats);

   if(opts.find(CanonicalFileView) != string::npos)
   {
      stream << '{' << CRLF;
      DisplayObjects(incls_, stream, lead, options);
      DisplayObjects(macros_, stream, lead, options);
      DisplayObjects(forws_, stream, lead, options);
      DisplayObjects(usings_, stream, lead, options);
      DisplayObjects(enums_, stream, lead, options);
      DisplayObjects(types_, stream, lead, options);
      DisplayObjects(funcs_, stream, lead, options);
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

fn_name CodeFile_EraseInternals = "CodeFile.EraseInternals";

void CodeFile::EraseInternals(CxxNamedSet& set) const
{
   Debug::ft(CodeFile_EraseInternals);

   for(auto i = set.cbegin(); i != set.cend(); NO_OP)
   {
      if((*i)->GetFile() == this)
         set.erase(*i++);
      else
         ++i;
   }
}

//------------------------------------------------------------------------------

fn_name CodeFile_FindData = "CodeFile.FindData";

Data* CodeFile::FindData(const string& name) const
{
   Debug::ft(CodeFile_FindData);

   for(auto d = data_.cbegin(); d != data_.cend(); ++d)
   {
      if(*(*d)->Name() == name) return *d;
   }

   return nullptr;
}

//------------------------------------------------------------------------------

fn_name CodeFile_FindDeclIds = "CodeFile.FindDeclIds";

void CodeFile::FindDeclIds()
{
   Debug::ft(CodeFile_FindDeclIds);

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

fn_name CodeFile_FindOrAddUsing = "CodeFile.FindOrAddUsing";

void CodeFile::FindOrAddUsing(const CxxNamed* user,
   const CodeFileVector usingFiles, CxxNamedSet& addUsing)
{
   Debug::ft(CodeFile_FindOrAddUsing);

   //  See if a file in usingFiles has a using statement that would make USER
   //  visible.  If so, this file does not need one.  Verify, however, that
   //  such a using statement is not tagged for removal.  This code was adapted
   //  from CxxScoped.NameRefersToItem, simplified to handle only the case of a
   //  symbol that needs to be resolved by a using statement.
   //
   string name;
   CxxNamed* ref;
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

   auto found = false;
   string fqName;
   size_t i = 0;

   while(!found && ref->GetScopedName(fqName, i))
   {
      auto pos = NameCouldReferTo(fqName, name);
      fqName.erase(0, 2);

      if(pos != string::npos)
      {
         for(auto i = usingFiles.cbegin(); i != usingFiles.cend(); ++i)
         {
            auto u = (*i)->GetUsingFor(fqName, pos - 4);

            if((u != nullptr) && !u->IsToBeRemoved())
            {
               found = true;
               break;
            }
         }

         if(!found)
         {
            //  No file in usingFiles had a suitable using statement.  If
            //  this file has one, keep it.  Note that this code can be run
            //  twice, by both Check() and Trim().  Using statements added
            //  to the file (this occurs below) must therefore be added to
            //  the set addUsing so that they will be logged as needing to
            //  be added, because they are not yet part of the source code.
            //
            auto u = GetUsingFor(fqName, pos - 4);

            if(u != nullptr)
            {
               u->MarkForRetention();
               if(u->WasAdded()) addUsing.insert(u->Referent());
               found = true;
            }
         }
      }

      ++i;
   }

   if(!found)
   {
      //  Neither a file in usingFiles nor this file had a suitable using
      //  statement.  This file should therefore add one.  If REF is external,
      //  add a using declaration (for a specific symbol).  If it is internal,
      //  add a using directive (for a namespace).
      //
      if(!ref->GetFile()->IsSubsFile())
      {
         auto space = ref->GetSpace();
         if(space != nullptr) ref = space;
      }

      addUsing.insert(ref);

      QualNamePtr qualName;
      auto name = ref->ScopedName(false);
      auto scope = Singleton< CxxRoot >::Instance()->GlobalNamespace();
      auto parser = std::unique_ptr< Parser >(new Parser(scope));
      parser->ParseQualName(name, qualName);
      parser.reset();
      qualName->SetReferent(ref);
      auto u = UsingPtr(new Using(qualName, false, true));
      InsertUsing(u);
   }
}

//------------------------------------------------------------------------------

fn_name CodeFile_FindUsingFor = "CodeFile.FindUsingFor";

Using* CodeFile::FindUsingFor(const string& name, size_t prefix,
   const CxxScoped* item, const CxxScope* scope) const
{
   Debug::ft(CodeFile_FindUsingFor);

   //  It's easy if this file or SCOPE has a sufficient using statement.
   //
   auto u = GetUsingFor(name, prefix);
   if(u != nullptr) return u;
   u = scope->GetUsingFor(name, prefix);
   if(u != nullptr) return u;

   //  Something that this file #includes (transitively) must make ITEM visible.
   //  Search the files that affect this one.  A file in the resulting set must
   //  have a using statment for NAME, at least up to PREFIX.
   //
   auto search = this->Affecters();

   //  Omit files that also affect the one that defines NAME.  This removes the
   //  file that actually defines NAME, so add it back to the search.  Do not
   //  exclude files, however, if ITEM is a forward or friend declaration, as
   //  its actual definition could occur totally outside of the search area.
   //
   auto type = item->Type();
   if((type != Cxx::Forward) && (type != Cxx::Friend))
   {
      SetDifference(search, item->GetFile()->Affecters());
      search.insert(item->GetFile()->Fid());
   }

   auto& files = Singleton< Library >::Instance()->Files();

   for(auto f = search.cbegin(); f != search.cend(); ++f)
   {
      u = files.At(*f)->GetUsingFor(name, prefix);
      if(u != nullptr) return u;
   }

   return nullptr;
}

//------------------------------------------------------------------------------

fixed_string FixPrompt = "  Fix?";
fixed_string FixChars = "ynsq";
fixed_string FixHelp = "Enter y(yes) n(no) s(skip file) q(quit): ";

fn_name CodeFile_Fix = "CodeFile.Fix";

word CodeFile::Fix(CliThread& cli, string& expl)
{
   Debug::ft(CodeFile_Fix);

   WarningLogVector warnings;
   CodeInfo::GetWarnings(this, warnings);
   if(warnings.empty()) return 0;

   //  Create an editor for the file.
   //
   word rc = 0;
   auto editor = std::unique_ptr< Editor >(CreateEditor(rc, expl));
   if(editor == nullptr) return rc;

   rc = editor->Read(expl);
   if(rc != 0) return rc;

   char reply = 'y';

   //  Sort the warnings by warning type/file/line.
   //
   std::sort(warnings.begin(), warnings.end(), CodeInfo::IsSortedByWarning);

   size_t found = 0;
   auto exit = false;

   //  Run through all the warnings.  If fixing a warning is not supported,
   //  skip it.
   //
   for(auto item = warnings.cbegin(); item != warnings.cend(); ++item)
   {
      switch(item->warning)
      {
      case IncludeNotSorted:
      case IncludeAdd:
      case IncludeRemove:
      case ForwardAdd:
      case ForwardRemove:
      case UsingAdd:
      case UsingRemove:
      case TrailingSpace:
      case RemoveBlankLine:
         ++found;
         break;
      default:
         continue;
      }

      //  Fixing this warning is supported, so display what can be fixed.
      //
      if(found == 1) *cli.obuf << item->file->Name() << ':' << CRLF;
      *cli.obuf << spaces(2) << "Line " << item->line + 1;
      if(item->offset != 0) *cli.obuf << '/' << item->offset;
      *cli.obuf << ": " << Warning(item->warning);
      if(!item->info.empty()) *cli.obuf << ": " << item->info;
      *cli.obuf << CRLF;

      if((item->line != 0) || item->info.empty())
      {
         *cli.obuf << spaces(2);
         *cli.obuf << item->file->GetLexer().GetNthLine(item->line) << CRLF;
      }

      //  See if the user wishes to fix the warning.  The valid responses are
      //  o 'y' = fix it, which invokes the appropriate function
      //  o 'n' = don't fix it
      //  o 's' = skip this file
      //  o 'q' = done fixing warnings
      //
      reply = cli.CharPrompt(FixPrompt, FixChars, FixHelp);
      expl.clear();

      switch(reply)
      {
      case 'y':
         switch(item->warning)
         {
         case IncludeNotSorted:
            rc = editor->SortIncludes(*item, expl);
            break;
         case IncludeAdd:
            rc = editor->AddInclude(*item, expl);
            break;
         case IncludeRemove:
            rc = editor->RemoveInclude(*item, expl);
            break;
         case ForwardAdd:
            rc = editor->AddForward(*item, expl);
            break;
         case ForwardRemove:
            rc = editor->RemoveForward(*item, expl);
            break;
         case UsingAdd:
            rc = editor->AddUsing(*item, expl);
            break;
         case UsingRemove:
            rc = editor->RemoveUsing(*item, expl);
            break;
         case TrailingSpace:
            rc = editor->EraseTrailingBlanks();
            break;
         case RemoveBlankLine:
            rc = editor->EraseBlankLinePairs();
            break;
         default:
            expl = "Fixing this type of warning is not supported.";
         }

         //  Display EXPL if it isn't empty, else assume that the fix succeeded.
         //
         *cli.obuf << spaces(2) << (expl.empty() ? SuccessExpl : expl) << CRLF;
         break;

      case 'n':
         break;

      case 's':
      case 'q':
         exit = true;
      }

      cli.Flush();
   }

   if((found > 0) && !exit)
   {
      *cli.obuf << spaces(2) << "End of supported warnings." << CRLF;
   }

   //  Write out the file.  The possible outcomes are
   //  o < 0: an error occurred.  Return this result to stop fixing files.
   //  o = 0: the file wasn't changed.
   //  o = 1: the file was changed.  Inform the user.
   //  If the user entered 'q', return -1 to stop fixing files unless an
   //  error occurred.
   //
   rc = editor->Write(FullName(), expl);
   if(rc == 1) *cli.obuf << spaces(2) << "...committed." << CRLF;
   if(rc >= 0) rc = (reply == 'q' ? -1 : 0);
   return rc;
}

//------------------------------------------------------------------------------

fn_name CodeFile_Format = "CodeFile.Format";

word CodeFile::Format(string& expl)
{
   Debug::ft(CodeFile_Format);

   Debug::Progress(Name(), false, true);

   word rc = 0;
   auto editor = std::unique_ptr< Editor >(CreateEditor(rc, expl));
   if(editor == nullptr) return rc;

   rc = editor->Read(expl);
   if(rc != 0) return rc;

   editor->EraseTrailingBlanks();
   editor->EraseBlankLinePairs();
   return editor->Write(FullName(), expl);
}

//------------------------------------------------------------------------------

string CodeFile::FullName() const
{
   if(dir_ == nullptr) return Name();
   return dir_->Path() + PATH_SEPARATOR + Name();
}

//------------------------------------------------------------------------------

fn_name CodeFile_GenerateReport = "CodeFile.GenerateReport";

void CodeFile::GenerateReport(ostream* stream, const SetOfIds& set)
{
   Debug::ft(CodeFile_GenerateReport);

   CodeInfo::GenerateReport(stream, set);
}

//------------------------------------------------------------------------------

fn_name CodeFile_GetDeclaredBaseClasses = "CodeFile.GetDeclaredBaseClasses";

void CodeFile::GetDeclaredBaseClasses(CxxNamedSet& bases) const
{
   Debug::ft(CodeFile_GetDeclaredBaseClasses);

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

fn_name CodeFile_GetLineCounts = "CodeFile.GetLineCounts";

void CodeFile::GetLineCounts() const
{
   Debug::ft(CodeFile_GetLineCounts);

   //  Don't count lines in substitute files.
   //
   if(isSubsFile_) return;

   CodeInfo::AddLineType(AnyLine, lineType_.size());

   for(size_t n = 0; n < lineType_.size(); ++n)
   {
      CodeInfo::AddLineType(lineType_[n], 1);
   }
}

//------------------------------------------------------------------------------

LineType CodeFile::GetLineType(size_t n) const
{
   if(n >= lineType_.size()) return LineType_N;
   return lineType_[n];
}

//------------------------------------------------------------------------------

fn_name CodeFile_GetUsageInfo = "CodeFile.GetUsageInfo";

void CodeFile::GetUsageInfo(CxxUsageSets& symbols) const
{
   Debug::ft(CodeFile_GetUsageInfo);

   //  Ask each of the code items in this file to provide information
   //  about the symbols that it uses.
   //
   auto& files = Singleton< Library >::Instance()->Files();

   for(auto m = macros_.cbegin(); m != macros_.cend(); ++m)
   {
      (*m)->GetUsages(*this, symbols);
   }

   for(auto c = classes_.cbegin(); c != classes_.cend(); ++c)
   {
      (*c)->GetUsages(*this, symbols);
   }

   for(auto t = types_.cbegin(); t != types_.cend(); ++t)
   {
      (*t)->GetUsages(*this, symbols);
   }

   for(auto f = funcs_.cbegin(); f != funcs_.cend(); ++f)
   {
      (*f)->GetUsages(*this, symbols);
   }

   for(auto d = data_.cbegin(); d != data_.cend(); ++d)
   {
      (*d)->GetUsages(*this, symbols);
   }

   //  For a .cpp, include, as base classes, those defined in any header
   //  that the .cpp implements.
   //
   if(IsCpp())
   {
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

fn_name CodeFile_GetUsingFor = "CodeFile.GetUsingFor";

Using* CodeFile::GetUsingFor(const string& name, size_t prefix) const
{
   Debug::ft(CodeFile_GetUsingFor);

   for(auto u = usings_.cbegin(); u != usings_.cend(); ++u)
   {
      if((*u)->IsUsingFor(name, prefix)) return u->get();
   }

   return nullptr;
}

//------------------------------------------------------------------------------

fn_name CodeFile_HasForwardFor = "CodeFile.HasForwardFor";

bool CodeFile::HasForwardFor(const CxxNamed* item) const
{
   Debug::ft(CodeFile_HasForwardFor);

   for(auto f = forws_.cbegin(); f != forws_.cend(); ++f)
   {
      if((*f)->Referent() == item) return true;
   }

   return false;
}

//------------------------------------------------------------------------------

fn_name CodeFile_Implementers = "CodeFile.Implementers";

const SetOfIds& CodeFile::Implementers()
{
   Debug::ft(CodeFile_Implementers);

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

fn_name CodeFile_InputStream = "CodeFile.InputStream";

istreamPtr CodeFile::InputStream() const
{
   Debug::ft(CodeFile_InputStream);

   //  If the file's directory is unknown (e.g. in the standard library), it
   //  can't be opened.  The file could nonetheless be known, however, as the
   //  result of parsing an #include directive.
   //
   if(dir_ == nullptr) return nullptr;
   return istreamPtr(SysFile::CreateIstream(FullName().c_str()));
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

fn_name CodeFile_InsertInclude = "CodeFile.InsertInclude";

Include* CodeFile::InsertInclude(const string& fn)
{
   for(auto i = incls_.cbegin(); i != incls_.cend(); ++i)
   {
      if(*(*i)->Name() == fn)
      {
         (*i)->SetScope(Context::Scope());
         items_.push_back(i->get());
         return i->get();
      }
   }

   Context::SwErr(CodeFile_InsertInclude, fn, 0);
   return nullptr;
}

//------------------------------------------------------------------------------

void CodeFile::InsertMacro(Macro* macro)
{
   items_.push_back(macro);
   macros_.push_back(macro);
}

//------------------------------------------------------------------------------

void CodeFile::InsertType(Typedef* type)
{
   items_.push_back(type);
   types_.push_back(type);
}

//------------------------------------------------------------------------------

void CodeFile::InsertUsing(UsingPtr& use)
{
   items_.push_back(use.get());
   usings_.push_back(std::move(use));
}

//------------------------------------------------------------------------------

fn_name CodeFile_IsTemplateHeader = "CodeFile.IsTemplateHeader";

bool CodeFile::IsTemplateHeader() const
{
   Debug::ft(CodeFile_IsTemplateHeader);

   for(auto c = classes_.cbegin(); c != classes_.cend(); ++c)
   {
      if((*c)->IsTemplate()) return true;
   }

   for(auto f = funcs_.cbegin(); f != funcs_.cend(); ++f)
   {
      if(!(*f)->IsTemplate()) return false;
   }

   return (!funcs_.empty());
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

   for(auto a = items.cbegin(); a != items.cend(); ++a)
   {
      std::ostringstream name;

      auto cls = (*a)->GetClass();

      if(cls != nullptr)
      {
         auto tag = cls->GetClassTag();
         name << tag << SPACE;
      }
      else
      {
         Debug::SwErr(CodeFile_LogAddForwards,
            "Non-class forward: " + (*a)->ScopedName(true), 0, InfoLog);
      }

      name << (*a)->ScopedName(true);
      names.insert(name.str());
   }

   for(auto n = names.cbegin(); n != names.cend(); ++n)
   {
      LogPos(0, ForwardAdd, 0, *n);
   }

   DisplaySymbols(stream, items, "Add a forward declaration for");
}

//------------------------------------------------------------------------------

fn_name CodeFile_LogAddIncludes = "CodeFile.LogAddIncludes";

void CodeFile::LogAddIncludes(ostream* stream, const SetOfIds& fids) const
{
   Debug::ft(CodeFile_LogAddIncludes);

   //  For each file in FIDS, generate a log saying that an #include for
   //  it should be added.  Put the filename in angle brackets or quotes,
   //  whichever is appropriate.
   //
   auto& files = Singleton< Library >::Instance()->Files();

   for(auto a = fids.cbegin(); a != fids.cend(); ++a)
   {
      string fn;
      auto f = files.At(*a);
      auto x = f->IsSubsFile();
      fn.push_back(x ? '<' : QUOTE);
      fn += f->Name();
      fn.push_back(x ? '>' : QUOTE);
      LogPos(0, IncludeAdd, 0, fn);
   }

   DisplayFileNames(stream, fids, "Add an #include for");
}

//------------------------------------------------------------------------------

fn_name CodeFile_LogAddUsings = "CodeFile.LogAddUsings";

void CodeFile::LogAddUsings(ostream* stream, const CxxNamedSet& items) const
{
   Debug::ft(CodeFile_LogAddUsings);

   //  If an item is external (e.g. in namespace std::), provide its fully
   //  qualified name.  If it is internal (e.g. part of RSC), just provide
   //  its namespace.
   //
   CxxNamedSet usings;

   for(auto a = items.cbegin(); a != items.cend(); ++a)
   {
      string name;
      auto space = (*a)->GetSpace();

      if(space->GetFile()->IsSubsFile())
      {
         name = (*a)->ScopedName(true);
         usings.insert(*a);
      }
      else
      {
         name = NAMESPACE_STR;
         name.push_back(SPACE);
         name += *space->Name();
         usings.insert(space);
      }

      LogPos(0, UsingAdd, 0, name);
   }

   DisplaySymbols(stream, usings, "Add a using statement for");
}

//------------------------------------------------------------------------------

fn_name CodeFile_LogLine = "CodeFile.LogLine";

void CodeFile::LogLine
   (size_t n, Warning warning, size_t offset, const string& info) const
{
   Debug::ft(CodeFile_LogLine);

   //  Don't log warnings in a substitute file or a template instance.
   //
   if(isSubsFile_) return;
   if(Context::ParsingTemplateInstance()) return;

   //  Log the warning if it is valid, has a valid line number, and is
   //  not a duplicate.
   //
   if((warning < Warning_N) && (n < lineType_.size()))
   {
      WarningLog log;

      log.file = this;
      log.line = n;
      log.warning = warning;
      log.offset = offset;
      log.info = info;

      CodeInfo::AddWarning(log);
   }
}

//------------------------------------------------------------------------------

fn_name CodeFile_LogPos = "CodeFile.LogPos";

void CodeFile::LogPos
   (size_t pos, Warning warning, size_t offset, const string& info) const
{
   Debug::ft(CodeFile_LogPos);

   LogLine(lexer_.GetLineNum(pos), warning, offset, info);
}

//------------------------------------------------------------------------------

fn_name CodeFile_LogRemoveForwards = "CodeFile.LogRemoveForwards";

void CodeFile::LogRemoveForwards
   (ostream* stream, const CxxNamedSet& items) const
{
   Debug::ft(CodeFile_LogRemoveForwards);

   //  Log each forward declaration that should be removed.  If the
   //  declaration cannot be found (something that shouldn't occur),
   //  just display its symbol.
   //
   for(auto a = items.cbegin(); a != items.cend(); ++a)
   {
      auto found = false;
      auto name = (*a)->ScopedName(true);

      for(auto f = forws_.cbegin(); f != forws_.cend(); ++f)
      {
         if((*f)->ScopedName(true) == name)
         {
            LogPos((*f)->GetPos(), ForwardRemove);
            found = true;
         }
      }

      if(!found) LogPos(0, ForwardRemove, 0, name);
   }

   DisplaySymbols(stream, items, "Remove the forward declaration for");
}

//------------------------------------------------------------------------------

fn_name CodeFile_LogRemoveIncludes = "CodeFile.LogRemoveIncludes";

void CodeFile::LogRemoveIncludes
   (ostream* stream, const SetOfIds& fids) const
{
   Debug::ft(CodeFile_LogRemoveIncludes);

   //  For each file in FIDS, generate a log saying that the #include for
   //  it should be removed.  If the #include cannot be found (something
   //  that shouldn't occur), just display its filename.
   //
   auto& files = Singleton< Library >::Instance()->Files();

   for(auto a = fids.cbegin(); a != fids.cend(); ++a)
   {
      string fn;
      auto f = files.At(*a);
      auto x = f->IsSubsFile();
      fn.push_back(x ? '<' : QUOTE);
      fn += f->Name();
      fn.push_back(x ? '>' : QUOTE);
      auto pos = code_.find(fn);

      if(pos == string::npos)
         LogPos(0, IncludeRemove, 0, fn);
      else
         LogPos(pos, IncludeRemove);
   }

   DisplayFileNames(stream, fids, "Remove the #include for");
}

//------------------------------------------------------------------------------

fn_name CodeFile_LogRemoveUsings = "CodeFile.LogRemoveUsings";

void CodeFile::LogRemoveUsings(ostream* stream) const
{
   Debug::ft(CodeFile_LogRemoveUsings);

   //  Using statements still marked for removal should be deleted.
   //  Don't report any that were added by >trim.
   //
   CxxNamedSet delUsing;

   for(auto u = usings_.cbegin(); u != usings_.cend(); ++u)
   {
      if((*u)->IsToBeRemoved() && !(*u)->WasAdded())
      {
         delUsing.insert(u->get());
         LogPos((*u)->GetPos(), UsingRemove);
      }
   }

   DisplaySymbols(stream, delUsing, "Remove the using statement for");
}

//------------------------------------------------------------------------------

fn_name CodeFile_PruneForwardCandidates = "CodeFile.PruneForwardCandidates";

void CodeFile::PruneForwardCandidates(const CxxNamedSet& forwards,
   const SetOfIds& inclIds, CxxNamedSet& addForws) const
{
   Debug::ft(CodeFile_PruneForwardCandidates);

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

fn_name CodeFile_PruneLocalForwards = "CodeFile.PruneLocalForwards";

void CodeFile::PruneLocalForwards
   (CxxNamedSet& addForws, CxxNamedSet& delForws) const
{
   Debug::ft(CodeFile_PruneLocalForwards);

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

fn_name CodeFile_RemoveHeaderIds = "CodeFile.RemoveHeaderIds";

void CodeFile::RemoveHeaderIds(SetOfIds& inclIds) const
{
   Debug::ft(CodeFile_RemoveHeaderIds);

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

fn_name CodeFile_SaveBaseIds = "CodeFile.SaveBaseIds";

void CodeFile::SaveBaseIds(const CxxNamedSet& bases)
{
   Debug::ft(CodeFile_SaveBaseIds);

   for(auto b = bases.cbegin(); b != bases.cend(); ++b)
   {
      auto base = static_cast< const Class* >(*b);
      baseIds_.insert(base->GetDeclFid());
   }
}

//------------------------------------------------------------------------------

fn_name CodeFile_Scan = "CodeFile.Scan";

void CodeFile::Scan()
{
   Debug::ft(CodeFile_Scan);

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
   auto lines = lexer_.LineCount();

   for(size_t n = 0; n < lines; ++n)
   {
      lineType_.push_back(LineType_N);
   }

   //  Categorize each line unless this is a substitute file.
   //
   if(!isSubsFile_)
   {
      for(size_t n = 0; n < lines; ++n)
      {
         lineType_[n] = ClassifyLine(n);
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

         auto incl = IncludePtr(new Include(file, angle));
         incl->SetPos(this, lexer_.GetLineStart(n));
         InsertInclude(incl);
      }
   }
}

//------------------------------------------------------------------------------

fn_name CodeFile_SetDir = "CodeFile.SetDir";

void CodeFile::SetDir(CodeDir* dir)
{
   Debug::ft(CodeFile_SetDir);

   dir_ = dir;
   isSubsFile_ = dir_->IsSubsDir();
}

//------------------------------------------------------------------------------

fn_name CodeFile_SetLocation = "CodeFile.SetLocation";

void CodeFile::SetLocation(TemplateLocation loc)
{
   Debug::ft(CodeFile_SetLocation);

   if(loc > location_) location_ = loc;
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
   classes_.shrink_to_fit();
   enums_.shrink_to_fit();
   types_.shrink_to_fit();
   funcs_.shrink_to_fit();
   data_.shrink_to_fit();

   auto size = incls_.capacity() * sizeof(IncludePtr);
   size += dirs_.capacity() * sizeof(DirectivePtr);
   size += usings_.capacity() * sizeof(UsingPtr);
   size += forws_.capacity() * sizeof(Forward*);
   size += macros_.capacity() * sizeof(Macro*);
   size += classes_.capacity() * sizeof(Class*);
   size += enums_.capacity() * sizeof(Enum*);
   size += types_.capacity() * sizeof(Typedef*);
   size += funcs_.capacity() * sizeof(Function*);
   size += data_.capacity() * sizeof(Data*);
   CxxStats::Vectors(CxxStats::CODE_FILE, size);
}

//------------------------------------------------------------------------------

fn_name CodeFile_Trim = "CodeFile.Trim";

void CodeFile::Trim(ostream* stream)
{
   Debug::ft(CodeFile_Trim);

   //  If this file should be trimmed, find the headers that declare items
   //  that this file defines, and assemble information about the symbols
   //  that this file uses.
   //
   if(!CanBeTrimmed(stream)) return;
   Debug::Progress(Name(), true);

   CxxUsageSets symbols;
   GetUsageInfo(symbols);
   auto& bases = symbols.bases;
   auto& directs = symbols.directs;
   auto& indirects = symbols.indirects;
   auto& forwards = symbols.forwards;
   auto& friends = symbols.friends;
   auto& users = symbols.users;

   FindDeclIds();
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
   //  inclIds, minus the file itself and what is already #included.
   //
   SetOfIds addIds;
   SetDifference(addIds, inclIds, inclIds_);
   addIds.erase(Fid());
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

   //  Create usingFiles, the files that this file can rely on for accessing
   //  using statements.  This set consists of transitive base class files
   //  (tBaseIds), files that declare what this file defines (declIds_), and
   //  transitive base classes of classes that this file defines (classIds_).
   //
   CodeFileVector usingFiles;

   auto& files = Singleton< Library >::Instance()->Files();

   for(auto b = tBaseIds.cbegin(); b != tBaseIds.cend(); ++b)
   {
      usingFiles.push_back(files.At(*b));
   }

   for(auto d = declIds_.cbegin(); d != declIds_.cend(); ++d)
   {
      usingFiles.push_back(files.At(*d));
   }

   for(auto c = classIds_.cbegin(); c != classIds_.cend(); ++c)
   {
      usingFiles.push_back(files.At(*c));
   }

   //  Look at each name (N) that was resolved by a using statement, and
   //  determine if this file should have a using statement to resolve it.
   //  First, mark all of the using statements in this file for removal.
   //  When it is determined that one is required, it will be marked for
   //  retention.
   //
   CxxNamedSet addUsing;

   for(auto u = usings_.cbegin(); u != usings_.cend(); ++u)
   {
      (*u)->MarkForRemoval();
   }

   for(auto n = users.cbegin(); n != users.cend(); ++n)
   {
      FindOrAddUsing(*n, usingFiles, addUsing);
   }

   //  Output the using statements that should be added and removed.
   //
   LogAddUsings(stream, addUsing);
   LogRemoveUsings(stream);

   //  For a header, output the external symbols that were resolved by a
   //  using statement.
   //
   if(IsHeader())
   {
      for(auto u = users.cbegin(); u != users.cend(); ++u)
      {
         if((*u)->GetFile()->IsSubsFile())
            qualify_.insert((*u)->DirectType());
      }

      DisplaySymbolsAndFiles(stream, qualify_,
         "To remove dependencies on using statements, qualify");
   }
}
}
