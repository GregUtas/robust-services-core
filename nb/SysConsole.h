//==============================================================================
//
//  SysConsole.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef SYSCONSOLE_H_INCLUDED
#define SYSCONSOLE_H_INCLUDED

#include <iosfwd>

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Operating system abstraction layer: console I/O.
//
namespace SysConsole
{
   //  Returns the stream from which console input is received.
   //
   std::istream& In();

   //  Returns the stream to which console output is sent.
   //
   std::ostream& Out();
}
}
#endif
