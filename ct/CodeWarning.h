//==============================================================================
//
//  CodeWarning.h
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
#ifndef CODEWARNING_H_INCLUDED
#define CODEWARNING_H_INCLUDED

#include <cstddef>
#include <iosfwd>
#include <string>
#include <vector>
#include "CodeTypes.h"
#include "CxxFwd.h"
#include "LibraryTypes.h"
#include "SysTypes.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace CodeTools
{
   //  Used to log a warning.
   //
   struct WarningLog
   {
      const CodeFile* file;  // file where warning occurred
      size_t line;           // line where warning occurred
      Warning warning;       // type of warning
      size_t offset;         // warning-specific; displayed if non-zero
      std::string info;      // warning-specific

      bool operator==(const WarningLog& that) const;
      bool operator!=(const WarningLog& that) const;
   };

   //  Information generated when analyzing, parsing, and executing code.
   //
   class CodeInfo
   {
   public:
      //  Generates a report in STREAM for the files in SET.  The report
      //  includes line type counts and warnings found during parsing and
      //  "execution".
      //
      static void GenerateReport(std::ostream* stream, const SetOfIds& set);

      //  Returns LOG's index if it has already been reported, else -1.
      //
      static word FindWarning(const WarningLog& log);

      //  The number of lines of each type, globally.
      //
      static size_t LineTypeCounts[LineType_N];

      //  Warnings found in all files.
      //
      static std::vector< WarningLog > Warnings;

      //  Returns true if LOG2 > LOG1 when sorting by file/line/warning.
      //
      static bool IsSortedByFile
         (const WarningLog& log1, const WarningLog& log2);

      //  Returns true if LOG2 > LOG1 when sorting by warning/file/line.
      //
      static bool IsSortedByWarning
         (const WarningLog& log1, const WarningLog& log2);
   private:
      //  Returns the string "Wnnn", where nnn is WARNING's integer value.
      //
      static std::string WarningCode(Warning warning);

      //  The total number of warnings of each type, globally.
      //
      static size_t WarningCounts[Warning_N];
   };
}
#endif
