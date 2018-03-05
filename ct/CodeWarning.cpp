//==============================================================================
//
//  CodeWarning.cpp
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
#include "CodeWarning.h"
#include <algorithm>
#include <iomanip>
#include <iterator>
#include <sstream>
#include <vector>
#include "CodeFile.h"
#include "CodeFileSet.h"
#include "CxxArea.h"
#include "CxxRoot.h"
#include "Debug.h"
#include "Formatters.h"
#include "Lexer.h"
#include "Library.h"
#include "Registry.h"
#include "Singleton.h"

using namespace NodeBase;
using std::ostream;
using std::setw;
using std::string;

//------------------------------------------------------------------------------

namespace CodeTools
{
bool WarningLog::operator==(const WarningLog& that) const
{
   if(this->file != that.file) return false;
   if(this->line != that.line) return false;
   if(this->warning != that.warning) return false;
   if(this->offset != that.offset) return false;
   return (this->info == that.info);
}

//------------------------------------------------------------------------------

bool WarningLog::operator!=(const WarningLog& that) const
{
   return !(*this == that);
}

//==============================================================================

size_t CodeInfo::LineTypeCounts_[] = { };

size_t CodeInfo::WarningCounts_[] = { };

std::vector< WarningLog > CodeInfo::Warnings_ = std::vector< WarningLog >();

//------------------------------------------------------------------------------

fn_name CodeInfo_AddWarning = "CodeInfo.AddWarning";

void CodeInfo::AddWarning(const WarningLog& log)
{
   Debug::ft(CodeInfo_AddWarning);

   if(FindWarning(log) < 0) Warnings_.push_back(log);
}

//------------------------------------------------------------------------------

word CodeInfo::FindWarning(const WarningLog& log)
{
   for(size_t i = 0; i < Warnings_.size(); ++i)
   {
      if(Warnings_.at(i) == log) return i;
   }

   return -1;
}

//------------------------------------------------------------------------------

fn_name CodeInfo_GenerateReport = "CodeInfo.GenerateReport";

void CodeInfo::GenerateReport(ostream* stream, const SetOfIds& set)
{
   Debug::ft(CodeInfo_GenerateReport);

   //  Clear any previous report's global counts.
   //
   for(auto t = 0; t < LineType_N; ++t) LineTypeCounts_[t] = 0;
   for(auto w = 0; w < Warning_N; ++w) WarningCounts_[w] = 0;

   //  Sort the files to be checked in build order.  This is important because
   //  recommendations about adding and removing #include directives and using
   //  statements, for example, are affected by earlier recommendations for
   //  #included files.
   //
   auto checkSet = new SetOfIds(set);
   auto checkFiles = new CodeFileSet(LibrarySet::TemporaryName(), checkSet);
   auto order = static_cast< CodeFileSet* >(checkFiles)->SortInBuildOrder();

   //  Run a check on each file in ORDER, as well as on each C++ item.
   //
   auto& files = Singleton< Library >::Instance()->Files();

   for(auto f = order->cbegin(); f != order->cend(); ++f)
   {
      auto file = files.At(f->fid);
      if(file->IsHeader()) file->Check();
   }

   for(auto f = order->cbegin(); f != order->cend(); ++f)
   {
      auto file = files.At(f->fid);
      if(file->IsCpp()) file->Check();
   }

   Singleton< CxxRoot >::Instance()->GlobalNamespace()->Check();

   //  Return if a report is not required.
   //
   if(stream == nullptr) return;

   //  Count the lines of each type.
   //
   for(auto f = set.cbegin(); f != set.cend(); ++f)
   {
      files.At(*f)->GetLineCounts();
   }

   std::vector< WarningLog > warnings;

   //  Count the total number of warnings of each type that appear in files
   //  belonging to the original SET, extracing them into the local set of
   //  warnings.
   //
   for(auto item = Warnings_.cbegin(); item != Warnings_.cend(); ++item)
   {
      if(set.find(item->file->Fid()) != set.cend())
      {
         ++WarningCounts_[item->warning];
         warnings.push_back(*item);
      }
   }

   //  Display the total number of lines of each type.
   //
   *stream << "LINE COUNTS" << CRLF;

   for(auto t = 0; t < LineType_N; ++t)
   {
      *stream << setw(12) << LineType(t)
         << spaces(2) << setw(6) << LineTypeCounts_[t] << CRLF;
   }

   //  Display the total number of warnings of each type.
   //
   *stream << CRLF << "WARNING COUNTS" << CRLF;

   for(auto w = 0; w < Warning_N; ++w)
   {
      if(WarningCounts_[w] != 0)
      {
         *stream << setw(6) << WarningCode(Warning(w)) << setw(6)
            << WarningCounts_[w] << spaces(2) << Warning(w) << CRLF;
      }
   }

   *stream << string(120, '=') << CRLF;
   *stream << "WARNINGS SORTED BY TYPE/FILE/LINE" << CRLF;

   //  Sort and output the warnings by warning type/file/line.
   //
   std::sort(warnings.begin(), warnings.end(), IsSortedByWarning);

   auto item = warnings.cbegin();
   auto last = warnings.cend();

   while(item != last)
   {
      auto w = item->warning;
      *stream << WarningCode(w) << SPACE << w << CRLF;

      do
      {
         *stream << spaces(2) << item->file->FullName();
         *stream << '(' << item->line + 1;
         if(item->offset != 0) *stream << '/' << item->offset;
         *stream << "): ";

         if(item->DisplayCode())
         {
            *stream << item->file->GetLexer().GetNthLine(item->line);
         }

         if(item->DisplayInfo()) *stream << item->info;
         *stream << CRLF;
         ++item;
      }
      while((item != last) && (item->warning == w));
   }

   *stream << string(120, '=') << CRLF;
   *stream << "WARNINGS SORTED BY FILE/TYPE/LINE" << CRLF;

   //  Sort and output the warnings by file/warning type/line.
   //
   std::sort(warnings.begin(), warnings.end(), IsSortedByFile);

   item = warnings.cbegin();
   last = warnings.cend();

   while(item != last)
   {
      auto f = item->file;
      *stream << f->FullName() << CRLF;

      do
      {
         auto w = item->warning;
         *stream << spaces(2) << WarningCode(w) << SPACE << w << CRLF;

         do
         {
            *stream << spaces(4) << item->line + 1;
            if(item->offset != 0) *stream << '/' << item->offset;
            *stream << ": ";

            if((item->line != 0) || item->info.empty())
            {
               *stream << f->GetLexer().GetNthLine(item->line);
            }

            *stream << item->info << CRLF;
            ++item;
         }
         while((item != last) && (item->warning == w) && (item->file == f));
      }
      while((item != last) && (item->file == f));
   }
}

//------------------------------------------------------------------------------

void CodeInfo::GetWarnings(const CodeFile* file, WarningLogVector& warnings)
{
   for(auto w = Warnings_.cbegin(); w != Warnings_.cend(); ++w)
   {
      if(w->file == file)
      {
         warnings.push_back(*w);
      }
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
   if(log1.offset < log2.offset) return true;
   if(log1.offset > log2.offset) return false;
   if(log1.info < log2.info) return true;
   if(log1.info > log2.info) return false;
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
   if(log1.offset < log2.offset) return true;
   if(log1.offset > log2.offset) return false;
   if(log1.info < log2.info) return true;
   if(log1.info > log2.info) return false;
   return (&log1 < &log2);
}

//------------------------------------------------------------------------------

string CodeInfo::WarningCode(Warning warning)
{
   std::ostringstream stream;

   stream << 'W' << setw(3) << std::setfill('0') << int(warning);
   return stream.str();
}
}
