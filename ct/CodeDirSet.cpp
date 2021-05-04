//==============================================================================
//
//  CodeDirSet.cpp
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
#include "CodeDirSet.h"
#include <cstddef>
#include <iomanip>
#include <ios>
#include <iosfwd>
#include <set>
#include <sstream>
#include <vector>
#include "CodeDir.h"
#include "CodeFile.h"
#include "CodeFileSet.h"
#include "Debug.h"
#include "Formatters.h"
#include "Library.h"
#include "Singleton.h"

using namespace NodeBase;
using std::setw;
using std::string;

//------------------------------------------------------------------------------

namespace CodeTools
{
CodeDirSet::CodeDirSet(const string& name, const LibItemSet* items) :
   CodeSet(name, items)
{
   Debug::ft("CodeDirSet.ctor");
}

//------------------------------------------------------------------------------

CodeDirSet::~CodeDirSet()
{
   Debug::ftnt("CodeDirSet.dtor");
}

//------------------------------------------------------------------------------

LibrarySet* CodeDirSet::Create
   (const string& name, const LibItemSet* items) const
{
   Debug::ft("CodeDirSet.Create");

   return new CodeDirSet(name, items);
}

//------------------------------------------------------------------------------

LibrarySet* CodeDirSet::Directories() const
{
   Debug::ft("CodeDirSet.Directories");

   //  Return the same set.
   //
   return new CodeDirSet(TemporaryName(), &Items());
}

//------------------------------------------------------------------------------

LibrarySet* CodeDirSet::Files() const
{
   Debug::ft("CodeDirSet.Files");

   //  Iterate over all code files to find those whose directory is in DIRSET.
   //
   auto& dirSet = Items();
   auto result = new CodeFileSet(TemporaryName(), nullptr);
   auto& files = Singleton< Library >::Instance()->Files().Items();

   for(auto f = files.cbegin(); f != files.cend(); ++f)
   {
      auto file = static_cast< CodeFile* >(*f);
      auto dir = file->Dir();

      if(dir != nullptr)
      {
         if(dirSet.find(dir) != dirSet.cend())
         {
            result->Items().insert(file);
         }
      }
   }

   return result;
}

//------------------------------------------------------------------------------

void CodeDirSet::to_str(stringVector& strings, bool verbose) const
{
   Debug::ft("CodeDirSet.to_str");

   auto& dirSet = Items();

   size_t width = 0;

   for(auto d = dirSet.cbegin(); d != dirSet.cend(); ++d)
   {
      auto dir = static_cast< CodeDir* >(*d);
      auto size = dir->Name().size();
      if(size > width) width = size;
   }

   auto indent = spaces(2);

   for(auto d = dirSet.cbegin(); d != dirSet.cend(); ++d)
   {
      auto dir = static_cast< CodeDir* >(*d);

      if(verbose)
      {
         std::ostringstream stream;
         stream << setw(width) << std::left << dir->Name();
         stream << indent << dir->Path();
         strings.push_back(stream.str());
      }
      else
      {
         strings.push_back(dir->Name());
      }
   }
}
}
