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
   cout << "o On entry, launches a specified RSC executable after prompting\n";
   cout << "  for any additional command line parameters.\n";
   cout << "o If RSC is forced to exit (>restart exit), launches it after\n";
   cout << "  reprompting for its directory and command line parameters.\n";
   cout << "o Immediately relaunches RSC if it requires a RestartReboot.\n";
}

//------------------------------------------------------------------------------
//
//  Removes leading and trailing blanks from STR.
//
static void RemoveBlanks(string& str)
{
   while(!str.empty() && isblank(str.back()))
   {
      str.pop_back();
   }

   while(!str.empty() && isblank(str.front()))
   {
      str.erase(0, 1);
   }
}

//------------------------------------------------------------------------------
//
//  Returns input from the console.
//
static string GetInput()
{
   string input;
   cin.clear();
   getline(cin, input);
   RemoveBlanks(input);
   return input;
}

//------------------------------------------------------------------------------
//
//  Gets the path to the rsc.exe executable that is to be launched.
//  Returns false if the launcher should exit.
//
static bool GetExecutable(string& exe)
{
   cout << '\n' << string(80, '=') << '\n';

   while(true)
   {
      cout << "Enter the full path to the RSC executable or 'Q' to exit:\n  ";
      exe = GetInput();

      if(exe.size() == 1)
      {
         auto first = tolower(exe.front());
         if(first == 'q') return false;
      }

      auto rsc = new std::ifstream(exe);

      if((rsc == nullptr) || rsc->fail() || (rsc->peek() == EOF))
      {
         cout << "That executable was not found.\n";
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
   cout << "Enter extra command line parameters. Hit the 'enter' key if \n";
   cout << "there are no command line parameters (if that does nothing, \n";
   cout << "enter a space first): \n";

   parms = GetInput();
   return true;
}

//------------------------------------------------------------------------------
//
//  Displays the rsc.exe that is to be launched, along with its command line
//  parameters.  Returns false to allow this information to be modified.
//
static bool Proceed(const string& exe, const string& parms)
{
   cout << "Launching " << exe << '\n';

   if(!parms.empty())
   {
      cout << "with the command line parameters\n  " << parms << '\n';
   }

   while(true)
   {
      cout << "Enter Y or N: ";

      auto input = GetInput();

      if(input.size() == 1)
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
int main(int argc, char* argv[])
{
   string exe;
   string parms;
   auto code = Reprompt;

   cout << argv[0] << '\n' << '\n';

   Explain();

   while(true)
   {
      if(code != Relaunch)
      {
         while(true)
         {
            if(!GetExecutable(exe)) exit(0);
            if(!GetParameters(parms)) exit(0);
            if(Proceed(exe, parms)) break;
         }
      }

      cout << '\n' << string(80, '=') << '\n';
      code = LaunchRsc(exe, parms);
   }
}
