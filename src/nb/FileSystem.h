//==============================================================================
//
//  FileSystem.h
//
//  Copyright (C) 2013-2025  Greg Utas
//
//  This file is part of the Robust Services Core (RSC).
//
//  RSC is free software: you can redistribute it and/or modify it under the
//  terms of the Lesser GNU General Public License as published by the Free
//  Software Foundation, either version 3 of the License, or (at your option)
//  any later version.
//
//  RSC is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
//  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
//  details.
//
//  You should have received a copy of the Lesser GNU General Public License
//  along with RSC.  If not, see <http://www.gnu.org/licenses/>.
//
#ifndef FILESYSTEM_H_INCLUDED
#define FILESYSTEM_H_INCLUDED

#include <cstddef>
#include <iosfwd>
#include <set>
#include <string>
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  File system functions.
//
namespace FileSystem
{
   //  Opens an existing file for input.  Returns nullptr if the file is
   //  empty or does not exist.
   //
   istreamPtr CreateIstream(c_string name);

   //  Creates a file for output.  If the file already exists, output is
   //  appended to it unless TRUNC is false.
   //
   ostreamPtr CreateOstream(c_string name, bool trunc = false);

   //  The same as std::getline, but removes the trailing '\r' at the end
   //  of STR when a text file created on Windows is read on Linux.
   //
   void GetLine(std::istream& stream, std::string& str);

   //  If NAME ends with EXT, returns the position where EXT begins, else
   //  returns string::npos.
   //
   size_t FindExt(const std::string& name, const std::string& ext);

   //  Adds the filenames in the directory DIR to NAMES, omitting any
   //  subdirectories.  Returns false if DIR could not be opened.
   //
   bool ListFiles(const std::string& dir, std::set<std::string>& names);

   //  Enables/disables file output.  By default, file output is enabled
   //  on startup, so main() should invoke this to suppress file output.
   //
   void DisableFileOutput(bool disabled);
}
}
#endif
