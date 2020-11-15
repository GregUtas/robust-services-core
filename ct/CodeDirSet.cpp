//==============================================================================
//
//  CodeDirSet.cpp
//
//  Copyright (C) 2013-2020  Greg Utas
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
#include "CodeDirSet.h"
#include <iomanip>
#include <ios>
#include <ostream>
#include <set>
#include "CodeDir.h"
#include "CodeFile.h"
#include "CodeFileSet.h"
#include "Debug.h"
#include "Formatters.h"
#include "Library.h"
#include "NbCliParms.h"
#include "Registry.h"
#include "Singleton.h"
#include "SysTypes.h"

using namespace NodeBase;
using std::ostream;
using std::setw;
using std::string;

//------------------------------------------------------------------------------

namespace CodeTools
{
CodeDirSet::CodeDirSet(const string& name, SetOfIds* set) : CodeSet(name, set)
{
   Debug::ft("CodeDirSet.ctor");
}

//------------------------------------------------------------------------------

CodeDirSet::~CodeDirSet()
{
   Debug::ftnt("CodeDirSet.dtor");
}

//------------------------------------------------------------------------------

LibrarySet* CodeDirSet::Create(const string& name, SetOfIds* set) const
{
   Debug::ft("CodeDirSet.Create");

   return new CodeDirSet(name, set);
}

//------------------------------------------------------------------------------

LibrarySet* CodeDirSet::Files() const
{
   Debug::ft("CodeDirSet.Files");

   //  Iterate over all code files to find those whose directory is in DIRSET.
   //
   auto& dirSet = Set();
   auto result = new CodeFileSet(TemporaryName(), nullptr);
   auto& fileSet = result->Set();
   auto& files = Singleton< Library >::Instance()->Files();

   for(auto f = files.First(); f != nullptr; files.Next(f))
   {
      auto d = f->Dir();

      if(d != nullptr)
      {
         SetOfIds::const_iterator it = dirSet.find(d->Did());

         if(it != dirSet.cend())
         {
            fileSet.insert(f->Fid());
         }
      }
   }

   return result;
}

//------------------------------------------------------------------------------

word CodeDirSet::List(ostream& stream, string& expl) const
{
   Debug::ft("CodeDirSet.List");

   auto& dirSet = Set();

   if(dirSet.empty())
   {
      stream << spaces(2) << EmptySet << CRLF;
      return 0;
   }

   auto& dirs = Singleton< Library >::Instance()->Directories();

   for(auto d = dirSet.cbegin(); d != dirSet.cend(); ++d)
   {
      stream << spaces(2) << setw(12) << std::right << dirs.At(*d)->Name();
      stream << spaces(2) << std::left << dirs.At(*d)->Path() << CRLF;
   }

   return 0;
}

//------------------------------------------------------------------------------

word CodeDirSet::Show(string& result) const
{
   Debug::ft("CodeDirSet.Show");

   auto& dirSet = Set();
   auto& dirs = Singleton< Library >::Instance()->Directories();

   for(auto d = dirSet.cbegin(); d != dirSet.cend(); ++d)
   {
      result = result + dirs.At(*d)->Name() + ", ";
   }

   return Shown(result);
}
}
