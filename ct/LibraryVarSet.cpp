//==============================================================================
//
//  LibraryVarSet.cpp
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
#include "LibraryVarSet.h"
#include "Debug.h"
#include "Library.h"
#include "Q2Way.h"
#include "Singleton.h"

using namespace NodeBase;
using std::string;

//------------------------------------------------------------------------------

namespace CodeTools
{
LibraryVarSet::LibraryVarSet(const string& name) : LibrarySet(name)
{
   Debug::ft("LibraryVarSet.ctor");
}

//------------------------------------------------------------------------------

LibraryVarSet::~LibraryVarSet()
{
   Debug::ftnt("LibraryVarSet.dtor");
}

//------------------------------------------------------------------------------

word LibraryVarSet::Count(string& result) const
{
   Debug::ft("LibraryVarSet.Count");

   auto size = Singleton< Library >::Instance()->Variables().Size();
   return Counted(result, &size);
}

//------------------------------------------------------------------------------

word LibraryVarSet::Show(string& result) const
{
   Debug::ft("LibraryVarSet.Show");

   auto& vars = Singleton< Library >::Instance()->Variables();

   for(auto v = vars.First(); v != nullptr; vars.Next(v))
   {
      if(!v->IsTemporary()) result = result + v->Name() + ", ";
   }

   return Shown(result);
}
}
