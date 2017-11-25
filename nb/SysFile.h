//==============================================================================
//
//  SysFile.h
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
#ifndef SYSFILE_H_INCLUDED
#define SYSFILE_H_INCLUDED

#include <iosfwd>
#include <memory>

//------------------------------------------------------------------------------

namespace NodeBase
{
//  For wrapping input and output streams.
//
typedef std::unique_ptr< std::istream > istreamPtr;
typedef std::unique_ptr< std::ostream > ostreamPtr;

//  Operating system abstraction layer: file I/O.
//
namespace SysFile
{
   //  Opens an existing file for input.
   //
   istreamPtr CreateIstream(const char* streamName);

   //  Creates a file for output.  If the file already exists,
   //  output is appended to it unless TRUNC is false.
   //
   ostreamPtr CreateOstream(const char* streamName, bool trunc = false);
}
}
#endif
