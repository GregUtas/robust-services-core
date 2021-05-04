//==============================================================================
//
//  SysConsole.win.cpp
//
//  Copyright (C) 2013-2021  Greg Utas
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
#include "SysConsole.h"
#include <windows.h>
#include "Debug.h"

using std::string;
using std::wstring;

//------------------------------------------------------------------------------

namespace NodeBase
{
bool SysConsole::Minimize(bool minimize)
{
   Debug::ft("SysConsole.Minimize");

   auto window = GetConsoleWindow();
   auto mode = (minimize ? SW_MINIMIZE : SW_RESTORE);
   return ShowWindow(window, mode);
}

//------------------------------------------------------------------------------

bool SysConsole::SetTitle(const string& title)
{
   Debug::ft("SysConsole.SetTitle");

   wstring wtitle(title.begin(), title.end());
   return SetConsoleTitle(wtitle.c_str());
}
}
