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
#include <iomanip>
#include <istream>
#include <iterator>
#include <sstream>
#include <utility>
#include "Algorithms.h"
#include "CodeDir.h"
#include "CodeFileSet.h"
#include "Cxx.h"
#include "CxxArea.h"
#include "CxxDirective.h"
#include "CxxExecute.h"
#include "CxxRoot.h"
#include "CxxScope.h"
#include "CxxString.h"
#include "CxxToken.h"
#include "Debug.h"
#include "Formatters.h"
#include "FunctionName.h"
#include "Library.h"
#include "Memory.h"
#include "NbCliParms.h"
#include "Registry.h"
#include "SetOperations.h"
#include "Singleton.h"

using std::ostream;
using std::setw;
using std::string;

//------------------------------------------------------------------------------

namespace CodeTools
{
//  Information about a line of source code.
//
struct LineInfo
{
   size_t begin;   // where the line begins (i.e. code_[begin])
   LineType type;  // the type of line (e.g. code, blank, comment)
};

//==============================================================================
//
//  Used when editing a code file.
//
class Editor
{
public:
   explicit Editor(istreamPtr& input);
   ~Editor();
   word Read(string& expl);
   word AddInclude(const string& item, string& expl);
   word RemoveInclude(const string& item, string& expl);
   word AddForward(string& item, string& expl);
   word RemoveForward(string& item, string& expl);
   word AddUsing(string& item, string& expl);
   word RemoveUsing(string& item, string& expl);
   word Write(const string& path, string& expl);
private:
   word GetProlog(string& expl);
   word GetIncludes(string& expl);
   word GetEpilog(string& expl);
   static void InsertInclude(const string& include, stringVector& list);
   static bool EraseInclude(const string& include, stringVector& list);

   istreamPtr input_;
   bool changed_;
   stringVector prolog_;
   stringVector extIncls_;
   stringVector intIncls_;
   stringVector epilog_;
};

//------------------------------------------------------------------------------

fn_name Editor_ctor = "Editor.ctor";

Editor::Editor(istreamPtr& input) :
   input_(std::move(input)),
   changed_(false)
{
   Debug::ft(Editor_ctor);

   input_->clear();
   input_->seekg(0);
}

//------------------------------------------------------------------------------

fn_name Editor_dtor = "Editor.dtor";

Editor::~Editor()
{
   Debug::ft(Editor_dtor);
}

//------------------------------------------------------------------------------

fn_name Editor_AddForward = "Editor.AddForward";

word Editor::AddForward(string& item, string& expl)
{
   Debug::ft(Editor_AddForward);

   //c Support adding a forward declaration using the >apply command.

   expl = NotImplementedExpl;
   return -1;
}

//------------------------------------------------------------------------------

fn_name Editor_AddInclude = "Editor.AddInclude";

word Editor::AddInclude(const string& item, string& expl)
{
   Debug::ft(Editor_AddInclude);

   auto file = Singleton< Library >::Instance()->FindFile(item);

   if(file == nullptr)
   {
      expl = NoFileExpl;
      return -2;
   }

   auto include = file->MakeInclude();
   auto& incls = (include.back() == '>' ? extIncls_ : intIncls_);

   InsertInclude(include, incls);
   changed_ = true;
   return 0;
}

//------------------------------------------------------------------------------

fn_name Editor_AddUsing = "Editor.AddUsing";

word Editor::AddUsing(string& item, string& expl)
{
   Debug::ft(Editor_AddUsing);

   //c Support adding a using declaration using the >apply command.

   expl = NotImplementedExpl;
   return -1;
}

//------------------------------------------------------------------------------

fn_name Editor_EraseInclude = "Editor.EraseInclude";

bool Editor::EraseInclude(const string& include, stringVector& list)
{
   Debug::ft(Editor_EraseInclude);

   for(auto i = list.cbegin(); i != list.cend(); ++i)
   {
      if(*i == include)
      {
         list.erase(i);
         return true;
      }
   }

   return false;
}

//------------------------------------------------------------------------------

fn_name Editor_GetEpilog = "Editor.GetEpilog";

word Editor::GetEpilog(string& expl)
{
   Debug::ft(Editor_GetEpilog);

   string str;

   //  Read the remaining lines in the file.
   //
   while(input_->peek() != EOF)
   {
      std::getline(*input_, str);
      epilog_.push_back(str);
   }

   return 0;
}

//------------------------------------------------------------------------------

fn_name Editor_GetIncludes = "Editor.GetIncludes";

word Editor::GetIncludes(string& expl)
{
   Debug::ft(Editor_GetIncludes);

   string str;

   //  Resuming after the first #include, add #include directives to one of
   //  two sorted sets, one for external files (e.g. #include <f.h>), and the
   //  other for internal files (e.g. #include "f.h").
   //
   while(input_->peek() != EOF)
   {
      std::getline(*input_, str);
      if(str.find(HASH_INCLUDE_STR) != 0)
      {
         epilog_.push_back(str);
         return 0;
      }

      auto next = str.find_first_not_of(SPACE, strlen(HASH_INCLUDE_STR));
      if(next == string::npos)
      {
         expl = "Empty #include directive.";
         return -3;
      }

      auto& incls = (str[next] == '<' ? extIncls_ : intIncls_);

      InsertInclude(str, incls);
      if(*incls.rbegin() != str) changed_ = true;
   }

   return 0;
}

//------------------------------------------------------------------------------

fn_name Editor_GetProlog = "Editor.GetProlog";

word Editor::GetProlog(string& expl)
{
   Debug::ft(Editor_GetProlog);

   string str;

   //  Read lines up to, and including, the first #include directive.  The
   //  first #include is not moved, under the assumption that it is either
   //  o for a .h*, the base class for the case defined in this file, or
   //  o for a .c*, the .h* that the .c* implements.
   //
   while(input_->peek() != EOF)
   {
      std::getline(*input_, str);
      prolog_.push_back(str);
      if(str.find(HASH_INCLUDE_STR) == 0) return 0;
   }

   expl = "File contains no #include statements";
   return -1;
}

//------------------------------------------------------------------------------

fn_name Editor_InsertInclude = "Editor.InsertInclude";

void Editor::InsertInclude(const string& include, stringVector& list)
{
   Debug::ft(Editor_InsertInclude);

   if(list.empty() || strCompare(include, list.back()) > 0)
   {
      list.push_back(include);
   }
   else
   {
      for(auto i = list.cbegin(); i != list.cend(); ++i)
      {
         if(strCompare(include, *i) < 0)
         {
            list.insert(i, include);
            return;
         }
      }
   }
}

//------------------------------------------------------------------------------

fn_name Editor_Read = "Editor.Read";

word Editor::Read(string& expl)
{
   Debug::ft(Editor_Read);

   auto rc = GetProlog(expl);
   if(rc != 0) return rc;
   rc = GetIncludes(expl);
   if(rc != 0) return rc;
   return GetEpilog(expl);
}

//------------------------------------------------------------------------------

fn_name Editor_RemoveForward = "Editor.RemoveForward";

word Editor::RemoveForward(string& item, string& expl)
{
   Debug::ft(Editor_RemoveForward);

   //c Support removing a forward declaration using the >apply command.

   expl = NotImplementedExpl;
   return -1;
}

//------------------------------------------------------------------------------

fn_name Editor_RemoveInclude = "Editor.RemoveInclude";

word Editor::RemoveInclude(const string& item, string& expl)
{
   Debug::ft(Editor_RemoveInclude);

   auto file = Singleton< Library >::Instance()->FindFile(item);

   if(file == nullptr)
   {
      expl = NoFileExpl;
      return -2;
   }

   auto include = file->MakeInclude();
   auto& incls = (include.back() == '>' ? extIncls_ : intIncls_);

   if(!EraseInclude(include, incls))
   {
      expl = "Directive not found: " + include;
      return -2;
   }

   changed_ = true;
   return 0;
}

//------------------------------------------------------------------------------

fn_name Editor_RemoveUsing = "Editor.RemoveUsing";

word Editor::RemoveUsing(string& item, string& expl)
{
   Debug::ft(Editor_RemoveUsing);

   //c Support removing a using declaration using the >apply command.

   expl = NotImplementedExpl;
   return -1;
}

//------------------------------------------------------------------------------

fn_name Editor_Write = "Editor.Write";

word Editor::Write(const string& path, string& expl)
{
   Debug::ft(Editor_Write);

   //  Return if nothing has changeed.
   //
   if(!changed_) return 0;

   //  Create a new file to hold the reformatted version.
   //
   auto file = path + ".tmp";
   auto output = ostreamPtr(SysFile::CreateOstream(file.c_str(), true));
   if(output == nullptr)
   {
      expl = "Failed to open output file.";
      return -7;
   }

   for(auto s = prolog_.cbegin(); s != prolog_.cend(); ++s)
   {
      *output << *s << CRLF;
   }

   for(auto s = extIncls_.cbegin(); s != extIncls_.cend(); ++s)
   {
      *output << *s << CRLF;
   }

   for(auto s = intIncls_.cbegin(); s != intIncls_.cend(); ++s)
   {
      *output << *s << CRLF;
   }

   for(auto s = epilog_.cbegin(); s != epilog_.cend(); ++s)
   {
      *output << *s << CRLF;
   }

   //  Close the files, delete the original, and replace it with the new one.
   //
   input_.reset();
   output.reset();
   remove(path.c_str());

   auto err = rename(file.c_str(), path.c_str());
   if(err != 0)
   {
      expl = "Failed to rename new file to old.";
      return -6;
   }

   return 1;
}

//==============================================================================
//
//  Used to log a warning.
//
struct WarningLog
{
   const CodeFile* file;   // file where warning occurred
   size_t line;            // line where warning occurred
   Warning warning;        // type of warning
   size_t offset;          // warning-specific; displayed if non-zero

   bool operator==(const WarningLog& that) const;
   bool operator!=(const WarningLog& that) const;
};

//------------------------------------------------------------------------------

bool WarningLog::operator==(const WarningLog& that) const
{
   if(this->file != that.file) return false;
   if(this->line != that.line) return false;
   if(this->warning != that.warning) return false;
   return (this->offset == that.offset);
}

//------------------------------------------------------------------------------

bool WarningLog::operator!=(const WarningLog& that) const
{
   return !(*this == that);
}

//==============================================================================
//
//  Information generated when analyzing, parsing, and executing code.
//
class CodeInfo
{
public:
   //  Generates a report in STREAM for the files in SET.  The report includes
   //  line type counts and warnings found during parsing and "execution".
   //
   static void GenerateReport(std::ostream& stream, const SetOfIds& set);

   //  Returns LOG's index if it has already been reported, else -1.
   //
   static word FindWarning(const WarningLog& log);

   //  The number of lines of each type, globally.
   //
   static size_t LineTypeCounts[LineType_N];

   //  Warnings found in all files.
   //
   static std::vector< WarningLog > Warnings;
private:
   //  Returns the string "Wnnn", where nnn is WARNING's integer value.
   //
   static std::string WarningCode(Warning warning);

   //  Returns true if LOG2 > LOG1 when sorting by file/line/warning.
   //
   static bool IsSortedByFile
      (const WarningLog& log1, const WarningLog& log2);

   //  Returns true if LOG2 > LOG1 when sorting by warning/file/line.
   //
   static bool IsSortedByWarning
      (const WarningLog& log1, const WarningLog& log2);

   //  The total number of warnings of each type, globally.
   //
   static size_t WarningCounts[Warning_N];
};

//------------------------------------------------------------------------------

size_t CodeInfo::LineTypeCounts[] = { };

size_t CodeInfo::WarningCounts[] = { };

std::vector< WarningLog > CodeInfo::Warnings = std::vector< WarningLog >();

//------------------------------------------------------------------------------

fn_name CodeInfo_GenerateReport = "CodeInfo.GenerateReport";

void CodeInfo::GenerateReport(ostream& stream, const SetOfIds& set)
{
   Debug::ft(CodeInfo_GenerateReport);

   //  Clear any previous report's global counts.
   //
   for(auto t = 0; t < LineType_N; ++t) LineTypeCounts[t] = 0;
   for(auto w = 0; w < Warning_N; ++w) WarningCounts[w] = 0;

   //  Run a check on each file in SET, as well as on each C++ item.
   //
   auto& files = Singleton< Library >::Instance()->Files();

   for(auto f = set.cbegin(); f != set.cend(); ++f)
   {
      files.At(*f)->Check();
   }

   Singleton< CxxRoot >::Instance()->GlobalNamespace()->Check();

   //  Count the lines of each type.
   //
   for(auto f = set.cbegin(); f != set.cend(); ++f)
   {
      files.At(*f)->GetLineCounts();
   }

   std::vector< WarningLog > warnings;

   //  Count the total number of warnings of each type that appear in files
   //  belonging to SET, extracing them into the local set of warnings.
   //
   for(auto item = Warnings.cbegin(); item != Warnings.cend(); ++item)
   {
      if(set.find(item->file->Fid()) != set.cend())
      {
         ++WarningCounts[item->warning];
         warnings.push_back(*item);
      }
   }

   //  Display the total number of lines of each type.
   //
   stream << "LINE COUNTS" << CRLF;

   for(auto t = 0; t < LineType_N; ++t)
   {
      stream << setw(12) << LineType(t)
         << spaces(2) << setw(6) << LineTypeCounts[t] << CRLF;
   }

   //  Display the total number of warnings of each type.
   //
   stream << CRLF << "WARNING COUNTS" << CRLF;

   for(auto w = 0; w < Warning_N; ++w)
   {
      if(WarningCounts[w] != 0)
      {
         stream << setw(6) << WarningCode(Warning(w))
            << setw(6) << WarningCounts[w] << spaces(2) << Warning(w) << CRLF;
      }
   }

   stream << string(120, '=') << CRLF;
   stream << "WARNINGS SORTED BY TYPE/FILE/LINE" << CRLF;

   //  Sort and output the warnings by warning type/file/line.
   //
   std::sort(warnings.begin(), warnings.end(), IsSortedByWarning);

   auto item = warnings.cbegin();
   auto last = warnings.cend();

   while(item != last)
   {
      auto w = item->warning;
      stream << WarningCode(w) << SPACE << Warning(w) << CRLF;

      do
      {
         stream << spaces(2) << item->file->FullName() << '(' << item->line + 1;
         if(item->offset != 0) stream << '/' << item->offset;
         stream << "): ";
         stream << item->file->GetNthLine(item->line) << CRLF;
         ++item;
      }
      while((item != last) && (item->warning == w));
   }

   stream << string(120, '=') << CRLF;
   stream << "WARNINGS SORTED BY FILE/TYPE/LINE" << CRLF;

   //  Sort and output the warnings by file/warning type/line.
   //
   std::sort(warnings.begin(), warnings.end(), IsSortedByFile);

   item = warnings.cbegin();
   last = warnings.cend();

   while(item != last)
   {
      auto f = item->file;
      stream << f->FullName() << ':' << CRLF;

      do
      {
         auto w = item->warning;
         stream << spaces(2) << WarningCode(w) << SPACE << w << CRLF;

         do
         {
            stream << spaces(4) << "line " << item->line + 1;
            if(item->offset != 0) stream << '/' << item->offset;
            stream << ": " << f->GetNthLine(item->line) << CRLF;
            ++item;
         }
         while((item != last) && (item->warning == w) && (item->file == f));
      }
      while((item != last) && (item->file == f));
   }
}

//------------------------------------------------------------------------------

bool CodeInfo::IsSortedByFile(const WarningLog& log1, const WarningLog& log2)
{
   auto result = strCompare(log1.file->FullName(), log2.file->FullName());
   if(result == -1) return true;
   if(result == 1) return false;
   if(log1.warning < log2.warning) return true;
   if(log1.warning > log2.warning) return false;
   if(log1.line < log2.line) return true;
   if(log1.line > log2.line) return false;
   return (&log1 < &log2);
}

//------------------------------------------------------------------------------

bool CodeInfo::IsSortedByWarning(const WarningLog& log1, const WarningLog& log2)
{
   if(log1.warning < log2.warning) return true;
   if(log1.warning > log2.warning) return false;
   auto result = strCompare(log1.file->FullName(), log2.file->FullName());
   if(result == -1) return true;
   if(result == 1) return false;
   if(log1.line < log2.line) return true;
   if(log1.line > log2.line) return false;
   return (&log1 < &log2);
}

//------------------------------------------------------------------------------

fn_name CodeInfo_FindWarning = "CodeInfo.FindWarning";

word CodeInfo::FindWarning(const WarningLog& log)
{
   Debug::ft(CodeInfo_FindWarning);

   for(size_t i = 0; i < Warnings.size(); ++i)
   {
      if(Warnings.at(i) == log) return i;
   }

   return -1;
}

//------------------------------------------------------------------------------

string CodeInfo::WarningCode(Warning warning)
{
   std::ostringstream stream;

   stream << 'W' << setw(3) << std::setfill('0') << int(warning);
   return stream.str();
}

//==============================================================================

fn_name CodeTools_AddForwardDependencies = "CodeTools.AddForwardDependencies";

void AddForwardDependencies(const CxxUsageSets& symbols, CxxNamedSet& inclSet)
{
   Debug::ft(CodeTools_AddForwardDependencies);

   //  SYMBOLS is the usage information for the symbols that appeared in this
   //  file.  An #include should appear for a forward declaration that resolved
   //  an indirect reference in this file.  Omit the #include, however, if the
   //  declaration appears in a file that defines one of our base classes,
   //  including an indirect base class.
   //
   for(auto f = symbols.forwards.cbegin(); f != symbols.forwards.cend(); ++f)
   {
      auto fid = (*f)->GetDeclFid();
      auto include = true;

      for(auto b = symbols.bases.cbegin(); b != symbols.bases.cend(); ++b)
      {
         auto base = static_cast< const Class* >(*b);

         for(auto s = base->BaseClass(); s != nullptr; s = s->BaseClass())
         {
            if(s->GetDeclFid() == fid)
            {
               include = false;
               break;
            }
         }
      }

      if(include) inclSet.insert(*f);
   }
}

//------------------------------------------------------------------------------

fn_name CodeTools_AddIntersection = "CodeTools.AddIntersection";

void AddIntersection
   (CxxNamedSet& n1, const CxxNamedSet& n2, const CxxNamedSet& n3)
{
   Debug::ft(CodeTools_AddIntersection);

   //  Add the intersection of N2 and N3 to N1.
   //
   for(auto i = n2.cbegin(); i != n2.cend(); ++i)
   {
      if(n3.find(*i) != n3.cend()) n1.insert(*i);
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

void DisplayFileNames(ostream& stream, const SetOfIds& fids, fixed_string title)
{
   //  Display, in STREAM, the names of files identified in FIDS.
   //  TITLE provides an explanation for the list.
   //
   if(fids.empty()) return;

   auto& files = Singleton< Library >::Instance()->Files();
   stream << spaces(3) << title << CRLF;

   for(auto a = fids.cbegin(); a != fids.cend(); ++a)
   {
      stream << spaces(6) << files.At(*a)->Name() << CRLF;
   }
}

//------------------------------------------------------------------------------

void DisplayFullNames
   (ostream& stream, const CxxNamedSet& items, fixed_string title)
{
   //  Display, in STREAM, the names in ITEMS, including their scope.
   //  TITLE provides an explanation for the list.
   //
   if(!items.empty())
   {
      stream << spaces(3) << title << CRLF;

      for(auto a = items.cbegin(); a != items.cend(); ++a)
      {
         stream << spaces(6) << (*a)->ScopedName(true) << CRLF;
      }
   }
}

//------------------------------------------------------------------------------

void DisplaySymbols
   (ostream& stream, const CxxNamedSet& set, const string& title)
{
   //  Display, in STREAM, the symbols in SET and where they are defined.
   //  Include TITLE, which describes the contents of SET.  Start by putting
   //  the symbol names into a stringSet so that they will always appear in
   //  the same order.
   //
   if(set.empty()) return;
   stream << spaces(3) << title << CRLF;

   stringSet names;

   for(auto i = set.cbegin(); i != set.cend(); ++i)
   {
      auto name = (*i)->ScopedName(false);
      auto file = (*i)->GetDeclFile();

      if(file != nullptr)
         name += " [" + file->Name() + ']';
      else
         name += " [file unknown]";

      names.insert(name);
   }

   for(auto n = names.cbegin(); n != names.cend(); ++n)
   {
      stream << spaces(6) << *n << CRLF;
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
   size_(0),
   lines_(0),
   info_(nullptr),
   firstIncl_(NIL_ID),
   slashAsterisk_(false),
   parsed_(Unparsed),
   location_(FuncNoTemplate)
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
      if((*u)->GetDeclFile() == this) continue;
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
      if((type == Cxx::Class) && !(*i)->GetDeclFile()->IsSubsFile()) continue;
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

bool CodeFile::CanBeTrimmed(ostream& stream) const
{
   Debug::ft(CodeFile_CanBeTrimmed);

   //  Don't trim empty files, substitute files, or unexecuted files
   //  (template headers).
   //
   if(size_ == 0) return false;
   if(isSubsFile_) return false;

   stream << Name() << CRLF;

   switch(location_)
   {
   case FuncInTemplate:
      stream << spaces(3) << "OMITTED: mostly unexecuted." << CRLF;
      return false;
   case FuncIsTemplate:
      stream << spaces(3) << "WARNING: partially unexecuted." << CRLF;
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

   Debug::Progress(Name(), true);

   //  Don't check an empty file or a substitute file.
   //
   if((lines_ == 0) || isSubsFile_) return;

   for(auto u = usings_.cbegin(); u != usings_.cend(); ++u)
   {
      (*u)->Check();
   }

   CheckProlog();
   CheckIncludeGuard();
   CheckIncludes();
   CheckUsings();
   CheckSeparation();
   CheckFunctionOrder();
   CheckDebugFt();
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
      if((*f)->IsTemplateInstance()) continue;
      (*f)->GetDefnRange(begin, end);
      if(begin >= end) continue;
      auto last = GetLineNum(end);
      auto open = false, debug = false, code = false;

      for(auto n = GetLineNum(begin); n < last; ++n)
      {
         switch(info_[n].type)
         {
         case OpenBrace:
            open = true;
            break;

         case DebugFt:
            debug = true;
            if(code) LogLine(n, DebugFtNotFirst);
            code = true;

            if(GetNthLine(n, s))
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

   if(!IsCpp() || funcs_.empty()) return;

   std::set< Function* > forwards;

   auto f = funcs_.cbegin();

   while(true)
   {
      auto scope = (*f)->GetScope();
      auto state = FuncCtor;
      const string* prev = nullptr;
      const string* curr = nullptr;
      forwards.clear();

      while((*f)->GetScope() == scope)
      {
         //  If a function is declared forward in a .cpp, add it to FORWARDS
         //  the first time (its declaration) so that it can be handled when
         //  its definition appears.
         //
         if(((*f)->GetDeclFile() == this) &&
            ((*f)->GetDefnFile() == this) &&
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
                  LogPos((*f)->GetDefnPos(), FunctionNotSorted);
                  break;

               case FuncOperator:
                  curr = (*f)->Name();
                  if((prev != nullptr) && (strCompare(*curr, *prev) < 0))
                  {
                     if(prev->find(OPERATOR_STR) != 0)
                     {
                        LogPos((*f)->GetDefnPos(), FunctionNotSorted);
                     }
                  }
                  prev = curr;
                  break;

               case FuncStandard:
                  curr = (*f)->Name();
                  if((prev != nullptr) && (strCompare(*curr, *prev) < 0))
                  {
                     LogPos((*f)->GetDefnPos(), FunctionNotSorted);
                  }
                  prev = curr;
               }
            }
         }

         ++f;
         if(f == funcs_.cend()) return;
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

   for(n = 0; (n < lines_) && (pos == string::npos); ++n)
   {
      switch(info_[n].type)
      {
      case EmptyComment:
      case LicenseComment:
      case TextComment:
      case SeparatorComment:
      case TaggedComment:
      case Blank:
         continue;
      case HashDirective:
         pos = info_[n].begin;
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

fn_name CodeFile_CheckIncludes = "CodeFile.CheckIncludes";

void CodeFile::CheckIncludes() const
{
   Debug::ft(CodeFile_CheckIncludes);

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

            if(err) LogPos((*i2)->GetDeclPos(), IncludeNotSorted);
         }

         prev = i2->get();
      }

      //  Look for a duplicated #include.
      //
      for(NO_OP; i2 != incls_.cend(); ++i2)
      {
         if(*(*i1)->Name() == *(*i2)->Name())
         {
            LogPos((*i2)->GetDeclPos(), IncludeDuplicated);
         }
      }
   }
}

//------------------------------------------------------------------------------

const string SingleRule(COMMENT_STR + string(78, '-'));
const string DoubleRule(COMMENT_STR + string(78, '='));

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

void CodeFile::CheckProlog() const
{
   Debug::ft(CodeFile_CheckProlog);

   //  Each file should begin with
   //
   //  //==================...
   //  //
   //  //  FileName.ext
   //  //  FileProlog [multiple lines]
   //
   auto pos = info_[0].begin;
   auto ok = (code_.find(DoubleRule, pos) == pos);
   if(!ok) return LogLine(0, HeadingNotStandard);

   pos = info_[1].begin;
   ok = ok && (code_.find(COMMENT_STR, pos) == pos);
   ok = ok && (info_[1].type == EmptyComment);
   if(!ok) return LogLine(1, HeadingNotStandard);

   pos = info_[2].begin;
   ok = ok && (code_.find(COMMENT_STR, pos) == pos);
   ok = ok && (code_.find(Name(), pos) == pos + 4);
   if(!ok) return LogLine(2, HeadingNotStandard);
   info_[2].type = LicenseComment;

   size_t line = 3;

   for(auto i = 0; i < FilePrologSize; ++i)
   {
      pos = info_[line].begin;
      ok = ok && (code_.find(COMMENT_STR, pos) == pos);

      if(FileProlog[i] == EMPTY_STR)
      {
         ok = ok && (info_[line].type == EmptyComment);
      }
      else
      {
         ok = ok && (code_.find(FileProlog[i], pos) == pos + 4);
         if(ok) info_[line].type = LicenseComment;
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

   for(size_t n = 0; n < lines_; ++n)
   {
      //  Based on the type of line just found, look for warnings that can
      //  only be found based on the type of line that preceded this one.
      //
      switch(info_[n].type)
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
         case TaggedComment:
         case TextComment:
            break;
         default:
            LogLine(n, InsertBlankLine);
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

      prevType = info_[n].type;
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
   if(!GetNthLine(n, s)) return LineType_N;

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

   if((n > 0) && (info_[n - 1].type == FunctionNameSplit)) return FunctionName;
   return Code;
}

//------------------------------------------------------------------------------

fn_name CodeFile_CreateEditor = "CodeFile.CreateEditor";

Editor* CodeFile::CreateEditor(word& rc, string& expl) const
{
   Debug::ft(CodeFile_CreateEditor);

   //  Fail if the directory for this file is unknown or the file
   //  can't be opened.
   //
   if(dir_ == nullptr)
   {
      expl = "Directory not specified.";
      rc = -2;
      return nullptr;
   }

   auto input = Stream();
   if(input == nullptr)
   {
      expl = "Failed to open source code file.";
      rc = -7;
      return nullptr;
   }

   return new Editor(input);
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
   stream << prefix << "size      : " << size_ << CRLF;
   stream << prefix << "lines     : " << lines_ << CRLF;
   stream << prefix << "firstIncl : " << firstIncl_ << CRLF;
   stream << prefix << "parsed    : " << parsed_ << CRLF;

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

   auto lead = spaces(Indent_Size);
   auto qual = Flags(FQ_Mask);

   if(opts.find('c') != string::npos)
   {
      stream << FullName() << CRLF;
      stream << '{' << CRLF;
      DisplayObjects(incls_, stream, lead, qual);
      DisplayObjects(forws_, stream, lead, qual);
      DisplayObjects(usings_, stream, lead, qual);
      DisplayObjects(macros_, stream, lead, qual);
      DisplayObjects(enums_, stream, lead, qual);
      DisplayObjects(types_, stream, lead, qual);
      DisplayObjects(funcs_, stream, lead, qual);
      DisplayObjects(data_, stream, lead, qual);
      DisplayObjects(classes_, stream, lead, qual);
      stream << '}' << CRLF;
   }

   if(opts.find('o') != string::npos)
   {
      stream << '{' << CRLF;
      DisplayObjects(items_, stream, lead, qual);
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
      if((*i)->GetDeclFile() == this)
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
   //  file that actually defines NAME, so add it back to the search.
   //
   SetDifference(search, item->GetDeclFile()->Affecters());
   search.insert(item->GetDeclFile()->Fid());

   auto& files = Singleton< Library >::Instance()->Files();

   for(auto f = search.cbegin(); f != search.cend(); ++f)
   {
      u = files.At(*f)->GetUsingFor(name, prefix);
      if(u != nullptr) return u;
   }

   return nullptr;
}

//------------------------------------------------------------------------------

fn_name CodeFile_Format = "CodeFile.Format";

word CodeFile::Format(string& expl)
{
   Debug::ft(CodeFile_Format);

   Debug::Progress(Name(), false, true);

   //  Reading the file, and then writing it, automatically sorts
   //  its #include statements into standard order.
   //
   word rc = 0;
   auto editor = std::unique_ptr< Editor >(CreateEditor(rc, expl));
   if(editor == nullptr) return rc;

   rc = editor->Read(expl);
   if(rc != 0) return rc;
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

void CodeFile::GenerateReport(ostream& stream, const SetOfIds& set)
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

fn_name CodeFile_GetDeclIds = "CodeFile.GetDeclIds";

void CodeFile::GetDeclIds(SetOfIds& declIds) const
{
   Debug::ft(CodeFile_GetDeclIds);

   //  If this is a .cpp, assemble the headers that declare
   //  items that the .cpp defines.
   //
   if(IsCpp())
   {
      for(auto f = funcs_.cbegin(); f != funcs_.cend(); ++f)
      {
         auto fid = (*f)->GetDistinctDeclFid();
         if(fid != NIL_ID) declIds.insert(fid);
      }

      for(auto d = data_.cbegin(); d != data_.cend(); ++d)
      {
         auto fid = (*d)->GetDistinctDeclFid();
         if(fid != NIL_ID) declIds.insert(fid);
      }
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

   CodeInfo::LineTypeCounts[AnyLine] += lines_;

   for(size_t n = 0; n < lines_; ++n)
   {
      ++CodeInfo::LineTypeCounts[info_[n].type];
   }
}

//------------------------------------------------------------------------------

size_t CodeFile::GetLineNum(size_t pos) const
{
   if(pos > size_) return string::npos;

   //  Do a binary search over the lines' starting positions.
   //
   int min = 0;
   int max = lines_ - 1;

   while(min < max)
   {
      auto mid = (min + max + 1) >> 1;

      if(info_[mid].begin > pos)
         max = mid - 1;
      else
         min = mid;
   }

   return min;
}

//------------------------------------------------------------------------------

LineType CodeFile::GetLineType(size_t n) const
{
   if(n >= lines_) return LineType_N;
   return info_[n].type;
}

//------------------------------------------------------------------------------

bool CodeFile::GetNthLine(size_t n, string& s) const
{
   if(n >= lines_)
   {
      s.clear();
      return false;
   }

   auto curr = info_[n].begin;

   if(n == lines_ - 1)
      s = code_.substr(curr, size_ - curr - 1);
   else
      s = code_.substr(curr, info_[n + 1].begin - curr - 1);

   return true;
}

//------------------------------------------------------------------------------

string CodeFile::GetNthLine(size_t n) const
{
   string s;
   GetNthLine(n, s);
   return s;
}

//------------------------------------------------------------------------------

fn_name CodeFile_GetUsageInfo = "CodeFile.GetUsageInfo";

void CodeFile::GetUsageInfo
   (const SetOfIds& declIds, CxxUsageSets& symbols) const
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
      for(auto d = declIds.cbegin(); d != declIds.cend(); ++d)
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

const SetOfIds& CodeFile::Implementers() const
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

fn_name CodeFile_LogLine = "CodeFile.LogLine";

void CodeFile::LogLine(size_t n, Warning warning, size_t offset) const
{
   Debug::ft(CodeFile_LogLine);

   //  Don't log warnings in a substitute file or a template instance.
   //
   if(isSubsFile_) return;
   if(Context::ParsingTemplateInstance()) return;

   //  Log the warning if it is valid, has a valid line number, and is
   //  not a duplicate.
   //
   if((warning < Warning_N) && (n < lines_))
   {
      WarningLog log;

      log.file = this;
      log.line = n;
      log.warning = warning;
      log.offset = offset;

      if(CodeInfo::FindWarning(log) < 0) CodeInfo::Warnings.push_back(log);
   }
}

//------------------------------------------------------------------------------

fn_name CodeFile_LogPos = "CodeFile.LogPos";

void CodeFile::LogPos(size_t pos, Warning warning, size_t offset) const
{
   Debug::ft(CodeFile_LogPos);

   LogLine(GetLineNum(pos), warning, offset);
}

//------------------------------------------------------------------------------

fn_name CodeFile_MakeInclude = "CodeFile.MakeInclude";

string CodeFile::MakeInclude() const
{
   Debug::ft(CodeFile_MakeInclude);

   auto front = (IsSubsFile() ? '<' : QUOTE);
   auto back = (front == '<' ? '>' : QUOTE);
   string include = HASH_INCLUDE_STR;
   include.push_back(SPACE);
   include += front + Name() + back;
   return include;
}

//------------------------------------------------------------------------------

fn_name CodeFile_Modify = "CodeFile.Modify";

word CodeFile::Modify(Modification act, string& item, string& expl)
{
   Debug::ft(CodeFile_Modify);

   //  Reading the file, and then writing it, automatically sorts its
   //  #include statements into standard order.
   //
   word rc = 0;
   auto editor = std::unique_ptr< Editor >(CreateEditor(rc, expl));
   if(editor == nullptr) return rc;

   rc = editor->Read(expl);
   if(rc != 0) return rc;

   switch(act)
   {
   case NoChange:
      break;
   case AddInclude:
      rc = editor->AddInclude(item, expl);
      break;
   case RemoveInclude:
      rc = editor->RemoveInclude(item, expl);
      break;
   case AddForward:
      rc = editor->AddForward(item, expl);
      break;
   case RemoveForward:
      rc = editor->RemoveForward(item, expl);
      break;
   case AddUsing:
      rc = editor->AddUsing(item, expl);
      break;
   case RemoveUsing:
      rc = editor->RemoveUsing(item, expl);
      break;
   default:
      expl = SystemErrorExpl;
      return act;
   }

   if(rc != 0) return rc;
   return editor->Write(FullName(), expl);
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
      auto addFile = (*add)->GetDeclFile();
      auto remove = false;

      //  Do not add a forward declaration for a type that was resolved by
      //  an existing forward declaration, whether in this file or another.
      //
      for(auto f = forwards.cbegin(); f != forwards.cend(); ++f)
      {
         remove = ((*f)->Referent() == *add);
         if(remove) break;
      }

      //  Double-check the forward declarations in this file.  One for a
      //  function argument can get omitted from FORWARDS as follows:
      //  o While parsing the function's *declaration*, the forward is used.
      //  o While parsing the function's *definition*, the forward is not
      //    used because an #include in the .cpp makes the argument's type
      //    directly visible.
      //  o When the function's declaration and definition are merged (see
      //    Function.UpdateDeclaration), the arguments in the definition
      //    replace those in the declaration.  Consequently, when GetUsages
      //    is invoked on the argument, the referent of its DataSpec is its
      //    actual declaration, not its forward declaration.
      //
      if(!remove) remove = HasForwardFor(*add);

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

void CodeFile::RemoveHeaderIds(const SetOfIds& declIds, SetOfIds& inclIds) const
{
   Debug::ft(CodeFile_RemoveHeaderIds);

   //  If this is a .cpp, it implements items declared one or more headers
   //  (declIds).  The .cpp need not #include anything that one of those
   //  headers already #includes.
   //
   auto& files = Singleton< Library >::Instance()->Files();

   if(IsCpp())
   {
      for(auto d = declIds.cbegin(); d != declIds.cend(); ++d)
      {
         SetDifference(inclIds, files.At(*d)->InclList());
      }

      //  Ensure that all files in declIds are #included.  It is possible for
      //  one to get dropped if, for example, the .cpp uses a subclass of an
      //  item that it implements.
      //
      SetUnion(inclIds, declIds);
   }
}

//------------------------------------------------------------------------------

fn_name CodeFile_Scan = "CodeFile.Scan";

void CodeFile::Scan()
{
   Debug::ft(CodeFile_Scan);

   if(!code_.empty()) return;

   auto input = Stream();
   if(input == nullptr) return;
   input->clear();
   input->seekg(0);

   code_.clear();

   string str;

   while(input->peek() != EOF)
   {
      //  Fetch the next line and analyze it.  At present, only #include
      //  directives are parsed, in order to analyze dependencies.
      //
      std::getline(*input, str);
      code_ += str;
      code_ += CRLF;
      ++lines_;
   }

   //  Initialize the information about each line.
   //
   input.reset();
   size_ = code_.size();
   info_ = (LineInfo*) Memory::Alloc(sizeof(LineInfo) * lines_, MemTemp);
   lexer_.Initialize(&code_);

   for(size_t n = 0, pos = 0; n < lines_; ++n)
   {
      info_[n].begin = pos;
      info_[n].type = LineType_N;
      pos = code_.find(CRLF, pos) + 1;
   }

   //  Categorize each line unless this is a substitute file.
   //
   if(!isSubsFile_)
   {
      for(size_t n = 0; n < lines_; ++n)
      {
         info_[n].type = ClassifyLine(n);
      }
   }

   //  Preprocess #include directives.
   //
   auto lib = Singleton< Library >::Instance();
   string file;
   bool angle;

   for(size_t n = 0; n < lines_; ++n)
   {
      if(lexer_.GetIncludeFile(info_[n].begin, file, angle))
      {
         auto used = lib->EnsureFile(file);

         if(used != nullptr)
         {
            auto id = used->Fid();

            inclIds_.insert(id);
            used->AddUser(this);
            if(firstIncl_ == NIL_ID) firstIncl_ = id;
         }

         auto incl = IncludePtr(new Include(file, angle));
         incl->SetDecl(this, info_[n].begin);
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

fn_name CodeFile_Stream = "CodeFile.Stream";

istreamPtr CodeFile::Stream() const
{
   Debug::ft(CodeFile_Stream);

   //  If the file's directory is unknown (e.g. in the standard library), it
   //  can't be opened.  The file could nonetheless be known, however, as the
   //  result of parsing an #include directive.
   //
   if(dir_ == nullptr) return nullptr;
   return istreamPtr(SysFile::CreateIstream(FullName().c_str()));
}

//------------------------------------------------------------------------------

fn_name CodeFile_Trim = "CodeFile.Trim";

void CodeFile::Trim(ostream& stream) const
{
   Debug::ft(CodeFile_Trim);

   //  If this file should be trimmed, find the headers that declare items
   //  that this file defines, and assemble information about the symbols
   //  that this file uses.
   //
   if(!CanBeTrimmed(stream)) return;
   Debug::Progress(Name(), true);

   SetOfIds declIds;
   CxxUsageSets symbols;
   GetDeclIds(declIds);
   GetUsageInfo(declIds, symbols);

   auto& bases = symbols.bases;
   auto& directs = symbols.directs;
   auto& indirects = symbols.indirects;
   auto& forwards = symbols.forwards;
   auto& friends = symbols.friends;
   auto& users = symbols.users;

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
   DisplaySymbols(stream, bases, "Base usage:");
   DisplaySymbols(stream, inclSet, "Direct usage:");
   DisplaySymbols(stream, indirects, "Indirect usage:");
   DisplaySymbols(stream, forwards, "Forward usage:");
   DisplaySymbols(stream, friends, "Friend usage:");

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
   //  declared in the .cpp.
   //
   if(IsCpp()) GetDeclaredBaseClasses(bases);

   //  An #include should always appear for a base class.  Add them to
   //  inclSet in case any of them were removed.  While iterating through
   //  the base classes, assemble baseIds, the files that declare them.
   //
   SetOfIds baseIds;

   for(auto b = bases.cbegin(); b != bases.cend(); ++b)
   {
      inclSet.insert(*b);
      baseIds.insert((*b)->GetDeclFid());
   }

   //  Assemble inclIds, all the files that need to be #included.  It
   //  starts with all of the files (excluding this one) that declare
   //  items in inclSet.  A .cpp can then remove any file that will be
   //  #included by a header that declares an item that the .cpp defines.
   //
   SetOfIds inclIds;
   AddIncludeIds(inclSet, inclIds);
   RemoveHeaderIds(declIds, inclIds);

   //  Output addIds, the #includes that should be added.  It contains
   //  inclIds, minus the file itself and what is already #included.
   //
   SetOfIds addIds;
   SetDifference(addIds, inclIds, inclIds_);
   addIds.erase(Fid());
   DisplayFileNames(stream, addIds, ADD_INCLUDE_STR);

   //  Output delIds, which are the #includes that should be removed.  It
   //  contains what is already #included, minus everything that needs to
   //  be #included.
   //
   SetOfIds delIds;
   SetDifference(delIds, inclIds_, inclIds);
   DisplayFileNames(stream, delIds, REMOVE_INCLUDE_STR);

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
   DisplayFullNames(stream, addForws, ADD_FORWARD_STR);
   DisplayFullNames(stream, delForws, REMOVE_FORWARD_STR);

   //  Create usingFiles, the files that this file can rely on for accessing
   //  using statements.  This set consists of the file itself, files that
   //  declare base classes (baseIds) of any class implemented in this file,
   //  and files (declIds) that declare items that this file defines.
   //
   CodeFileVector usingFiles;

   usingFiles.push_back(this);

   auto& files = Singleton< Library >::Instance()->Files();

   for(auto b = baseIds.cbegin(); b != baseIds.cend(); ++b)
   {
      usingFiles.push_back(files.At(*b));
   }

   for(auto d = declIds.cbegin(); d != declIds.cend(); ++d)
   {
      usingFiles.push_back(files.At(*d));
   }

   //  Create usingRefs, the referents for all symbols (USERS) in this
   //  file that were resolved by using statements.
   //
   CxxNamedSet usingRefs;

   for(auto u = users.cbegin(); u != users.cend(); ++ u)
   {
      usingRefs.insert((*u)->Referent());
   }

   //  Extract, from usingRefs, the symbols that may require an #include.
   //  Note that USERS (and, consequently, usingRefs) may contain symbols
   //  used in inherited functions.  But because such symbols are omitted
   //  by Function.GetUsages, they will not end up in EXTERNALS.
   //
   CxxNamedSet externals;
   AddIntersection(externals, bases, usingRefs);
   AddIntersection(externals, inclSet, usingRefs);
   AddIntersection(externals, indirects, usingRefs);
   AddIntersection(externals, forwards, usingRefs);
   AddIntersection(externals, friends, usingRefs);

   //  Look at each name (N) that was resolved by a using statement, and
   //  determine if this file should add a using statement to resolve it.
   //
   CxxNamedSet addUsing;

   for(auto n = users.cbegin(); n != users.cend(); ++n)
   {
      //  If N's referent is not in EXTERNALS, a using statement for it
      //  is not required.  This occurs, for example, if N appeared in
      //  the signature of an inherited function, as discussed above.
      //
      auto ref = (*n)->Referent();
      if(externals.find(ref) == externals.cend()) continue;

      //  See if a file in INCLUDED has a using statement that would make
      //  N visible.  If so, this file does not need to add one.  However,
      //  check that such a using statement was actually used by the file
      //  where it appears, because the file will be told to remove it if
      //  it didn't need it.  (This code was adapted from NameRefersToItem,
      //  simplified to handle only the case of a symbol that is *known* to
      //  require resolution by a using statement.)
      //
      auto found = false;
      auto name = (*n)->QualifiedName(true, false);
      string fqName;
      size_t i = 0;

      while(ref->GetScopedName(fqName, i))
      {
         auto pos = NameCouldReferTo(fqName, name);
         fqName.erase(0, 2);

         if(pos != string::npos)
         {
            for(auto i = usingFiles.cbegin(); i != usingFiles.cend(); ++i)
            {
               auto u = (*i)->GetUsingFor(fqName, pos - 4);

               if((u != nullptr) && u->ResolvedLocal())
               {
                  found = true;
                  break;
               }
            }
         }

         ++i;
      }

      if(!found) addUsing.insert(ref);
   }

   //  Output the using statements that should be added.
   //
   DisplayFullNames(stream, addUsing, ADD_USING_STR);

   //  Using statements that did not resolve a symbol used in this
   //  file should be removed.
   //
   CxxNamedSet delUsing;

   for(auto u = usings_.cbegin(); u != usings_.cend(); ++u)
   {
      if(!(*u)->ResolvedLocal()) delUsing.insert(u->get());
   }

   DisplayFullNames(stream, delUsing, REMOVE_USING_STR);

   //  Output the symbols that were resolved by a using statement.
   //
   CxxNamedSet qualify;

   for(auto u = users.cbegin(); u != users.cend(); ++u)
   {
      qualify.insert((*u)->DirectType());
   }

   DisplaySymbols(stream, qualify,
      "To remove dependencies on using statements, qualify");
}
}
