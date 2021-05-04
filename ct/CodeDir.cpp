//==============================================================================
//
//  CodeDir.cpp
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
#include "CodeDir.h"
#include <ostream>
#include <set>
#include "CodeFile.h"
#include "CodeFileSet.h"
#include "CxxString.h"
#include "Debug.h"
#include "Library.h"
#include "NbCliParms.h"
#include "Singleton.h"
#include "SysFile.h"
#include "ThisThread.h"

using namespace NodeBase;
using std::ostream;
using std::string;

//------------------------------------------------------------------------------
//
namespace CodeTools
{
CodeDir::CodeDir(const string& name, const string& path) :
   name_(name),
   path_(path)
{
   Debug::ft("CodeDir.ctor");
}

//------------------------------------------------------------------------------

CodeDir::~CodeDir()
{
   Debug::ftnt("CodeDir.dtor");
}

//------------------------------------------------------------------------------

size_t CodeDir::CppCount() const
{
   Debug::ft("CodeDir.CppCount");

   size_t count = 0;

   auto& files = Singleton< Library >::Instance()->Files().Items();

   for(auto f = files.cbegin(); f != files.cend(); ++f)
   {
      auto file = static_cast< CodeFile* >(*f);
      if((file->Dir() == this) && file->IsCpp()) ++count;
   }

   return count;
}

//------------------------------------------------------------------------------

void CodeDir::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   LibraryItem::Display(stream, prefix, options);

   stream << prefix << "name : " << name_ << CRLF;
   stream << prefix << "path : " << path_ << CRLF;
}

//------------------------------------------------------------------------------

word CodeDir::Extract(string& expl)
{
   Debug::ft("CodeDir.Extract");

   //  Set this as the current directory.
   //
   if(!SysFile::SetDir(path_.c_str()))
   {
      expl = "Could not open directory " + path_;
      return -1;
   }

   auto list = SysFile::GetFileList(nullptr, "*");

   if(list != nullptr)
   {
      auto lib = Singleton< Library >::Instance();
      string name;

      do
      {
         if(!list->IsSubdir())
         {
            list->GetName(name);

            if(IsCodeFile(name))
            {
               auto f = lib->EnsureFile(name, this);
               f->Scan();
               ThisThread::Pause();
            }
         }
      }
      while(list->Advance());
   }

   expl = SuccessExpl;
   return 0;
}

//------------------------------------------------------------------------------

void CodeDir::GetDecls(std::set< CxxNamed* >& items)
{
   Debug::ft("CodeDir.GetDecls");

   auto& files = Singleton< Library >::Instance()->Files().Items();

   for(auto f = files.cbegin(); f != files.cend(); ++f)
   {
      auto file = static_cast< CodeFile* >(*f);
      file->GetDecls(items);
   }
}

//------------------------------------------------------------------------------

size_t CodeDir::HeaderCount() const
{
   Debug::ft("CodeDir.HeaderCount");

   size_t count = 0;

   auto& files = Singleton< Library >::Instance()->Files().Items();

   for(auto f = files.cbegin(); f != files.cend(); ++f)
   {
      auto file = static_cast< CodeFile* >(*f);
      if((file->Dir() == this) && file->IsHeader()) ++count;
   }

   return count;
}

//------------------------------------------------------------------------------

bool CodeDir::IsSubsDir() const
{
   Debug::ft("CodeDir.IsSubsDir");

   return PathIncludes(path_, Library::SubsDir);
}
}
