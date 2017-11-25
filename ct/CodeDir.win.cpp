//==============================================================================
//
//  CodeDir.win.cpp
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
#ifdef OS_WIN
#include "CodeDir.h"
#include <direct.h>
#include <io.h>
#include "CodeFile.h"
#include "CxxString.h"
#include "Debug.h"
#include "Library.h"
#include "NbCliParms.h"
#include "Singleton.h"

using std::string;

//------------------------------------------------------------------------------

namespace CodeTools
{
fn_name CodeDir_Extract = "CodeDir.Extract";

word CodeDir::Extract(string& expl)
{
   Debug::ft(CodeDir_Extract);

   //  Set this as the current directory.
   //
   if(_chdir(path_.c_str()) != 0)
   {
      expl = "Could not open directory " + path_;
      return *_errno();
   }

   auto lib = Singleton< Library >::Instance();

   _finddata_t fileAttrs;

   auto fileIterator = _findfirst("*", &fileAttrs);

   if(fileIterator != -1)
   {
      do
      {
         if((fileAttrs.attrib & _A_SUBDIR) == 0)
         {
            auto name = string(fileAttrs.name);

            if(IsCodeFile(name))
            {
               auto f = lib->EnsureFile(name, this);
               f->Scan();
            }
         }
      }
      while(_findnext(fileIterator, &fileAttrs) == 0);
   }

   _findclose(fileIterator);
   expl = SuccessExpl;
   return 0;
}

//------------------------------------------------------------------------------

fn_name CodeDir_IsCodeFile = "CodeDir.IsCodeFile";

bool CodeDir::IsCodeFile(const string& name)
{
   Debug::ft(CodeDir_IsCodeFile);

   //  Besides the usual .h* and .c* extensions, treat a file with
   //  no extension (e.g. <iosfwd>) as a code file.
   //
   if(name.find('.') == string::npos) return true;
   if(FileExtensionIs(name, "h")) return true;
   if(FileExtensionIs(name, "hpp")) return true;
   if(FileExtensionIs(name, "hxx")) return true;
   if(FileExtensionIs(name, "c")) return true;
   if(FileExtensionIs(name, "cpp")) return true;
   if(FileExtensionIs(name, "cxx")) return true;
   return false;
}
}
#endif
