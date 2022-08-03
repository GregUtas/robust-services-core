//==============================================================================
//
//  Launcher.win.cpp
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
//------------------------------------------------------------------------------

#ifdef OS_WIN

#include "Launcher.h"
#include <cstring>
#include <iostream>
#include <ostream>
#include <Windows.h>

//------------------------------------------------------------------------------

int LaunchRsc(const std::string& exe, const std::string& parms)
{
   //  Start the process EXE using the command line parameters PARMS.
   //
   STARTUPINFOA si;
   PROCESS_INFORMATION pi;

   memset(&si, 0, sizeof(si));
   si.cb = sizeof(si);
   memset(&pi, 0, sizeof(pi));

   auto command_line_parms = exe + ' ' + parms;
   char args[1024];
   strcpy_s(args, command_line_parms.c_str());

   if(!CreateProcessA(
      nullptr,  // executable is the first substring in ARGS
      args,     // command line parameters
      nullptr,  // process handle not inheritable
      nullptr,  // thread handle not inheritable
      false,    // other handles not inheritable
      0,        // run at normal priority in same console
      nullptr,  // use this process's environment block
      nullptr,  // use this process's starting directory
      &si,      // pointer to STARTUPINFOA structure
      &pi))     // pointer to PROCESS_INFORMATION structure
   {
      //  The process couldn't be started, so report success to prevent it
      //  from being automatically relaunched.
      //
      std::cout << "CreateProcessA failed: error=" << GetLastError() << '\n';
      return EXIT_SUCCESS;
   }

   //  Set the console title so that it identifies EXE.  When EXE
   //  exits, clean up its resources and report its exit code.
   //
   std::wstring title(exe.begin(), exe.end());
   SetConsoleTitle(title.c_str());
   WaitForSingleObject(pi.hProcess, INFINITE);

   DWORD code;
   GetExitCodeProcess(pi.hProcess, &code);

   CloseHandle(pi.hProcess);
   CloseHandle(pi.hThread);
   return (code == EXIT_SUCCESS ? EXIT_SUCCESS : EXIT_FAILURE);
}
#endif
