//==============================================================================
//
//  CodeDir.cpp
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
#include "CodeDir.h"
#include <cstdint>
#include <ostream>
#include "Algorithms.h"
#include "CodeFile.h"
#include "CxxString.h"
#include "Debug.h"
#include "Library.h"
#include "NbCliParms.h"
#include "Registry.h"
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
fn_name CodeDir_ctor = "CodeDir.ctor";

CodeDir::CodeDir(const string& name, const string& path) : LibraryItem(name),
   path_(path)
{
   Debug::ft(CodeDir_ctor);
}

//------------------------------------------------------------------------------

fn_name CodeDir_dtor = "CodeDir.dtor";

CodeDir::~CodeDir()
{
   Debug::ft(CodeDir_dtor);
}

//------------------------------------------------------------------------------

ptrdiff_t CodeDir::CellDiff()
{
   uintptr_t local;
   auto fake = reinterpret_cast< const CodeDir* >(&local);
   return ptrdiff(&fake->did_, fake);
}

//------------------------------------------------------------------------------

fn_name CodeDir_CppCount = "CodeDir.CppCount";

size_t CodeDir::CppCount() const
{
   Debug::ft(CodeDir_CppCount);

   size_t count = 0;

   auto& files = Singleton< Library >::Instance()->Files();

   for(auto f = files.First(); f != nullptr; files.Next(f))
   {
      if((f->Dir() == this) && f->IsCpp()) ++count;
   }

   return count;
}

//------------------------------------------------------------------------------

void CodeDir::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   LibraryItem::Display(stream, prefix, options);

   stream << prefix << "did  : " << did_.to_str() << CRLF;
   stream << prefix << "path : " << path_ << CRLF;
}

//------------------------------------------------------------------------------

fn_name CodeDir_Extract = "CodeDir.Extract";

word CodeDir::Extract(string& expl)
{
   Debug::ft(CodeDir_Extract);

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

fn_name CodeDir_HeaderCount = "CodeDir.HeaderCount";

size_t CodeDir::HeaderCount() const
{
   Debug::ft(CodeDir_HeaderCount);

   size_t count = 0;

   auto& files = Singleton< Library >::Instance()->Files();

   for(auto f = files.First(); f != nullptr; files.Next(f))
   {
      if((f->Dir() == this) && f->IsHeader()) ++count;
   }

   return count;
}

//------------------------------------------------------------------------------

fn_name CodeDir_IsSubsDir = "CodeDir.IsSubsDir";

bool CodeDir::IsSubsDir() const
{
   Debug::ft(CodeDir_IsSubsDir);

   return PathIncludes(path_, Library::SubsDir);
}
}
