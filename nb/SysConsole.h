//==============================================================================
//
//  SysConsole.h
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
#ifndef SYSCONSOLE_H_INCLUDED
#define SYSCONSOLE_H_INCLUDED

#include <iosfwd>
#include <string>

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Operating system abstraction layer: console I/O.
//
namespace SysConsole
{
   //  Returns the stream from which console input is received.  Applications
   //  must use the CLI interfaces instead of using this directly.
   //
   std::istream& In();

   //  Returns the stream to which console output is sent.  Applications must
   //  use CoutThread instead of using this directly.
   //
   std::ostream& Out();

   //  Minimizes or restores the console window.
   //
   bool Minimize(bool minimize);

   //  Sets the console window's title.
   //
   bool SetTitle(const std::string& title);
}
}
#endif
