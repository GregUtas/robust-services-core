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
#include <cstdint>
#include <iosfwd>
#include <map>
#include <string>
#include <utility>
#include "CodeTypes.h"
#include "CxxFwd.h"
#include "LibraryTypes.h"
#include "SysTypes.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace CodeTools
{
//  A code warning.
//
struct CodeWarning
{
   //  Set if the editor can fix the warning.
   //
   const bool fixable;

   //  The warning's sort order.  A warning with a lower value is fixed
   //  before one with a higher value.  This can help to simplify the
   //  editor.
   //
   const uint8_t order;

   //  Set if the warning is associated with an unused item.
   //
   const bool unusedItem;

   //  Set to suppress the warning.
   //
   bool suppressed;

   //  A string that explains the warning.
   //
   fixed_string title;

   //  For inserting elements into the attributes map.
   //
   typedef std::pair< Warning, CodeWarning > WarningPair;

   //  A type for mapping warnings to their attributes.
   //
   typedef std::map< Warning, CodeWarning > AttrsMap;

   //  Maps a warning to its attributes.
   //
   static AttrsMap Attrs;

   //  Initializes the Attrs map.
   //
   static void Initialize();
private:
   //  Constructs a warning with the specified attributes.
   //
   CodeWarning(bool fix, uint8_t order,
      bool unused, bool suppress, fixed_string title);
};

//------------------------------------------------------------------------------
//
//  Inserts a string for WARNING into STREAM.
//
std::ostream& operator<<(std::ostream& stream, Warning warning);

//  Whether a warning has been fixed.
//
enum WarningStatus
{
   NotSupported,  // editor does not support fixing this warning
   NotFixed,      // code not changed
   Pending,       // code changed but not written to file
   Fixed          // code changed and written to file
};

//------------------------------------------------------------------------------
//
//  Used to log a warning.
//
struct WarningLog
{
   Warning warning;       // type of warning
   const CodeFile* file;  // file where warning occurred
   size_t line;           // line where warning occurred
   size_t pos;            // position in FILE where warning occurred
   const CxxNamed* item;  // item associated with warning
   size_t offset;         // warning-specific; displayed if non-zero
   std::string info;      // warning-specific
   bool hide;             // set to stop warning from being displayed
   WarningStatus status;  // whether warning has been fixed/committed

   WarningLog(Warning warning, const CodeFile* file,
      size_t line, size_t pos, const CxxNamed* item,
      size_t offset, const std::string& info, bool hide = false);
   bool operator==(const WarningLog& that) const;
   bool operator!=(const WarningLog& that) const;
   bool DisplayCode() const
      { return ((line != 0) || info.empty()); }
   bool DisplayInfo() const
      { return (info.find_first_not_of(' ') != std::string::npos); }
};

//------------------------------------------------------------------------------
//
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

   //  Adds LOG to the global set of warnings.
   //
   static void AddWarning(const WarningLog& log);

   //  Updates WARNINGS with those that were logged in FILE.
   //
   static void GetWarnings(const CodeFile* file, WarningLogVector& warnings);

   //  Returns true if LOG2 > LOG1 when sorting by file/warning/line.
   //
   static bool IsSortedByFile(const WarningLog& log1, const WarningLog& log2);

   //  Returns true if LOG2 > LOG1 when sorting by file/line/reverse pos.
   //
   static bool IsSortedForFixing
      (const WarningLog& log1, const WarningLog& log2);

   //  Returns true if LOG2 > LOG1 when sorting by warning/file/line.
   //
   static bool IsSortedByWarning
      (const WarningLog& log1, const WarningLog& log2);

   //  Adds N to the number of line types of type T.
   //
   static void AddLineType(LineType t, size_t n) { LineTypeCounts_[t] += n; }
private:
   //  Returns LOG's index if it has already been reported, else -1.
   //
   static NodeBase::word FindWarning(const WarningLog& log);

   //  Returns the string "Wnnn", where nnn is WARNING's integer value.
   //
   static std::string WarningCode(Warning warning);

   //  Warnings found in all files.
   //
   static WarningLogVector Warnings_;

   //  The total number of warnings of each type, globally.
   //
   static size_t WarningCounts_[Warning_N];

   //  The number of lines of each type, globally.
   //
   static size_t LineTypeCounts_[LineType_N];
};
}
#endif
