//==============================================================================
//
//  Launcher.linux.cpp
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
//------------------------------------------------------------------------------

#ifdef OS_LINUX

#include "Launcher.h"
#include <cstdio>
#include <cstring>
#include <iostream>
#include <memory>
#include <ostream>
#include <sched.h>
#include <spawn.h>
#include <sys/wait.h>

//------------------------------------------------------------------------------

int LaunchRsc(const std::string& exe, const std::string& parms)
{
   pid_t pid;

   char* args[3] = { nullptr };
   char* envp[1] = { nullptr };

   std::unique_ptr<char[]> buff0(new char[exe.size() + 1]);
   strcpy(buff0.get(), exe.c_str());
   args[0] = buff0.get();

   if(!parms.empty())
   {
      std::unique_ptr<char[]> buff1(new char[parms.size() + 1]);
      strcpy(buff1.get(), parms.c_str());
      args[1] = buff1.get();
   }

   //  If the process can't be started, or if waitpid fails, report success
   //  to prevent it from being automatically relaunched.
   //
   auto code = posix_spawnp(&pid, exe.c_str(), nullptr, nullptr, args, envp);

   if(code == 0)
   {
      if(waitpid(pid, &code, 0) == -1)
      {
         perror("Error from waitpid");
         return EXIT_SUCCESS;
      }
   }
   else
   {
      std::cout << "Error launching RSC: " << strerror(code) << '\n';
      return EXIT_SUCCESS;
   }

   return (code == EXIT_SUCCESS ? EXIT_SUCCESS : EXIT_FAILURE);
}
#endif
