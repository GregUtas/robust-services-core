//==============================================================================
//
//  RscLauncher.h
//
//  Copyright (C) 2013-2022  Greg Utas
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
#ifndef RSCLAUNCHER_H_INCLUDED
#define RSCLAUNCHER_H_INCLUDED

#include <string>

//------------------------------------------------------------------------------
//
//  Launches EXE (a path to an .exe) with command line parameters PARMS.
//  When EXE exits, returns its exit code.  Returns -1 if EXE could not
//  be launched.  Implementations are platform-specific.
//
int LaunchRsc(const std::string& exe, const std::string& parms);

//  Outcomes from LaunchRsc.
//
constexpr int Reprompt = 0;  // prompt for another executable or to quit
constexpr int Relaunch = 1;  // launch same executable immediately

#endif
