//==============================================================================
//
//  SysFile.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
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
