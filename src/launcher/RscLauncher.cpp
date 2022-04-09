//==============================================================================
//
//  RscLauncher.cpp
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

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iosfwd>
#include <iostream>
#include <ostream>
#include <string>
#include "RscLauncher.h"

using std::cin;
using std::cout;
using std::string;

//------------------------------------------------------------------------------
//
//  Describes what the RSC launcher does when it starts up.
//
static void Explain()
{
   cout << "RSC LAUNCHER\n";
   cout << "o On entry, launches RSC from a specified directory after\n";
   cout << "  prompting for its command line parameters.\n";
   cout << "o If RSC is forced to exit (>restart exit), launches it after\n";
   cout << "  reprompting for its directory and command line parameters.\n";
   cout << "o Immediately relaunches RSC if it requires a RestartReboot.\n";
}

//------------------------------------------------------------------------------
//
//  Gets the path to the rsc.exe executable that is to be launched.
//  Returns false if the launcher should exit.
//
static bool GetExecutable(string& exe)
{
   string dir;

   cout << string(80, '=') << '\n';

   while(true)
   {
      cout << "Enter full path to directory where rsc.exe is located";
      cout << " or 'Q' to exit : \n  ";
      std::getline(cin, dir);

      if(!dir.empty())
      {
         auto first = tolower(dir.front());
         if(first == 'q') return false;
      }

      auto separator = (dir.find('/') != string::npos ? '/' : '\\');

      exe = dir + separator + "rsc.exe";
      auto rsc = new std::ifstream(exe);

      if(rsc->peek() == EOF)
      {
         cout << "rsc.exe was not found in that directory.\n";
         delete rsc;
         continue;
      }

      delete rsc;
      return true;
   }
}

//------------------------------------------------------------------------------
//
//  Gets the command line parameters for the rsc.exe before launching it.
//  Returns false if the launcher should exit.
//
static bool GetParameters(string& parms)
{
   cout << "Enter command line parameters for rsc.exe or 'Q' to exit:\n  ";
   std::getline(cin, parms);

   if(!parms.empty())
   {
      auto first = tolower(parms.front());
      if(first == 'q') return false;
   }

   return true;
}

//------------------------------------------------------------------------------
//
//  Displays the rsc.exe that is to be launched, along with its command line
//  parameters.  Returns false to allow this information to be modified.
//
static bool Proceed(const string& exe, const string& parms)
{
   string input;

   while(true)
   {
      cout << "Launching\n  " << exe << '\n';
      cout << "with the command line parameters\n  " << parms << '\n';
      cout << "Enter Y or N: ";
      std::getline(cin, input);

      if(!input.empty())
      {
         auto response = tolower(input.front());
         if(response == 'y') return true;
         if(response == 'n') return false;
      }
   }
}

//------------------------------------------------------------------------------
//
//  Launches a specified EXE with command line parameters PARMS.  If EXE exits
//  with code 0, allows a new EXE and command line parameters to be specified.
//  Immediately relaunches EXE if it exits with a non-zero code.  This supports
//  an RSC reboot restart.
//
int main()
{
   string exe;
   string parms;
   int code = 0;

   Explain();

   while(true)
   {
      if(code == 0)
      {
         while(true)
         {
            if(!GetExecutable(exe)) exit(0);
            if(!GetParameters(parms)) exit(0);
            if(Proceed(exe, parms)) break;
         }
      }

      cout << string(80, '=') << '\n';
      code = LaunchRsc(exe, parms);
   }
}
