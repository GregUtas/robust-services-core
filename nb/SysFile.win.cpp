//==============================================================================
//
//  SysFile.win.cpp
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
#include "SysFile.h"
#include <cstdint>
#include <direct.h>
#include <io.h>
#include "Debug.h"

using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Concrete class for file iteration.
//
class FileWalker : public FileList
{
public:
   FileWalker(const char* dirName, const char* fileSpec);
   ~FileWalker();
   void GetName(string& fileName) const override;
   bool IsSubdir() const override;
   bool AtEnd() const override;
   bool Advance() override;
private:
   //  Releases iterator_;
   //
   bool Reset();

   //  Handle for iteration.
   //
   intptr_t iterator_;

   //  The current file's attributes.
   //
   _finddata_t attributes_;
};

//------------------------------------------------------------------------------

fn_name FileWalker_ctor = "FileWalker.ctor";

FileWalker::FileWalker(const char* dirName, const char* fileSpec) :
   iterator_(-1)
{
   Debug::ft(FileWalker_ctor);

   auto err = (dirName != nullptr ? SysFile::SetDir(dirName) : false);
   if(!err) iterator_ = _findfirst(fileSpec, &attributes_);
}

//------------------------------------------------------------------------------

fn_name FileWalker_dtor = "FileWalker.dtor";

FileWalker::~FileWalker()
{
   Debug::ft(FileWalker_dtor);

   Reset();
}

//------------------------------------------------------------------------------

fn_name FileWalker_Advance = "FileWalker.Advance";

bool FileWalker::Advance()
{
   Debug::ft(FileWalker_Advance);

   if(iterator_ == -1) return false;
   if(_findnext(iterator_, &attributes_) == 0) return true;
   return Reset();
}

//------------------------------------------------------------------------------

bool FileWalker::AtEnd() const
{
   return (iterator_ == -1);
}

//------------------------------------------------------------------------------

void FileWalker::GetName(string& fileName) const
{
   fileName.clear();
   if(iterator_ == -1) return;
   fileName = attributes_.name;
}

//------------------------------------------------------------------------------

bool FileWalker::IsSubdir() const
{
   if(iterator_ == -1) return false;
   return ((attributes_.attrib & _A_SUBDIR) != 0);
}

//------------------------------------------------------------------------------

fn_name FileWalker_Reset = "FileWalker.Reset";

bool FileWalker::Reset()
{
   Debug::ft(FileWalker_Reset);

   if(iterator_ == -1) return false;
   _findclose(iterator_);
   iterator_ = -1;
   return false;
}

//==============================================================================

fn_name SysFile_GetDir = "SysFile.GetDir";

void SysFile::GetDir(string& dirName)
{
   Debug::ft(SysFile_GetDir);

   char buff[256];

   dirName.clear();
   auto path = _getcwd(buff, 256);
   if(path == nullptr) return;
   dirName = buff;
}

//------------------------------------------------------------------------------

fn_name SysFile_GetFileList = "SysFile.GetFileList";

FileListPtr SysFile::GetFileList(const char* dirName, const char* fileSpec)
{
   Debug::ft(SysFile_GetFileList);

   if(dirName != nullptr)
   {
      if(!SysFile::SetDir(dirName)) return nullptr;
   }

   FileListPtr list(new FileWalker(dirName, fileSpec));
   if(list == nullptr) return nullptr;

   if(list->AtEnd())
   {
      list.reset();
      return nullptr;
   }

   return list;
}

//------------------------------------------------------------------------------

fn_name SysFile_SetDir = "SysFile.SetDir";

bool SysFile::SetDir(const char* dirName)
{
   Debug::ft(SysFile_SetDir);

   return (_chdir(dirName) == 0);
}
}
#endif
