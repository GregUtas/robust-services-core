//==============================================================================
//
//  SysFile.cpp
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
#include "SysFile.h"
#include <cstddef>
#include <cstdio>
#include <fstream>
#include <ios>
#include <iosfwd>
#include "Debug.h"

using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
fn_name FileList_Advance = "FileList.Advance";

bool FileList::Advance()
{
   Debug::SwLog(FileList_Advance, strOver(this), 0);
   return false;
}

//------------------------------------------------------------------------------

fn_name FileList_AtEnd = "FileList.AtEnd";

bool FileList::AtEnd() const
{
   Debug::SwLog(FileList_AtEnd, strOver(this), 0);
   return true;
}

//------------------------------------------------------------------------------

fn_name FileList_GetName = "FileList.GetName";

void FileList::GetName(string& fileName) const
{
   Debug::SwLog(FileList_GetName, strOver(this), 0);
   fileName.clear();
}

//------------------------------------------------------------------------------

fn_name FileList_IsSubdir = "FileList.IsSubdir";

bool FileList::IsSubdir() const
{
   Debug::SwLog(FileList_IsSubdir, strOver(this), 0);
   return false;
}

//==============================================================================

fn_name SysFile_CreateIstream = "SysFile.CreateIstream";

istreamPtr SysFile::CreateIstream(const char* fileName)
{
   Debug::ft(SysFile_CreateIstream);

   istreamPtr stream(new std::ifstream(fileName));

   if((stream != nullptr) && (stream->peek() == EOF))
   {
      stream.reset();
      return nullptr;
   }

   return stream;
}

//------------------------------------------------------------------------------

fn_name SysFile_CreateOstream = "SysFile.CreateOstream";

ostreamPtr SysFile::CreateOstream(const char* fileName, bool trunc)
{
   Debug::ft(SysFile_CreateOstream);

   auto mode = (trunc ? std::ios::trunc : std::ios::app);
   ostreamPtr stream(new std::ofstream(fileName, mode));
   *stream << std::boolalpha << std::nouppercase;
   return stream;
}

//------------------------------------------------------------------------------

fn_name SysFile_FindFiles = "SysFile.FindFiles";

bool SysFile::FindFiles
   (const char* dirName, const char* fileExt, std::set< string >& fileNames)
{
   Debug::ft(SysFile_FindFiles);

   if(!SetDir(dirName)) return false;

   if(fileExt[0] != '.')
   {
      Debug::SwLog(SysFile_FindFiles, fileExt[0], 0);
      return false;
   }

   auto spec = "*" + string(fileExt);
   auto list = GetFileList(nullptr, spec.c_str());

   if(list != nullptr)
   {
      string name;

      do
      {
         if(!list->IsSubdir())
         {
            list->GetName(name);
            auto pos = name.rfind(fileExt);
            name.erase(pos);
            fileNames.insert(name);
         }
      }
      while(list->Advance());
   }

   return true;
}

//------------------------------------------------------------------------------

fn_name SysFile_Normalize1 = "SysFile.Normalize";

void SysFile::Normalize(string& path)
{
   Debug::ft(SysFile_Normalize1);

   for(size_t pos = 0; pos < path.size(); ++pos)
   {
      if(path[pos] == BACKSLASH) path[pos] = PATH_SEPARATOR;
   }
}

//------------------------------------------------------------------------------

fn_name SysFile_Normalize2 = "SysFile.Normalize(const)";

string SysFile::Normalize(const string& path)
{
   Debug::ft(SysFile_Normalize2);

   auto copy = path;
   Normalize(copy);
   return copy;
}
}
