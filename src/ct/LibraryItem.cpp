//==============================================================================
//
//  LibraryItem.cpp
//
//  Copyright (C) 2013-2025  Greg Utas
//
//  This file is part of the Robust Services Core (RSC).
//
//  RSC is free software: you can redistribute it and/or modify it under the
//  terms of the Lesser GNU General Public License as published by the Free
//  Software Foundation, either version 3 of the License, or (at your option)
//  any later version.
//
//  RSC is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
//  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
//  details.
//
//  You should have received a copy of the Lesser GNU General Public License
//  along with RSC.  If not, see <http://www.gnu.org/licenses/>.
//
#include "LibraryItem.h"
#include "Debug.h"
#include "Formatters.h"
#include "SysTypes.h"

using namespace NodeBase;
using std::string;

//------------------------------------------------------------------------------

namespace CodeTools
{
bool IsSortedByName(const LibraryItem* item1, const LibraryItem* item2)
{
   return (strCompare(item1->Name(), item2->Name()) < 0);
}

//==============================================================================

LibraryItem::LibraryItem()
{
   Debug::ft("LibraryItem.ctor");
}

//------------------------------------------------------------------------------

LibraryItem::~LibraryItem()
{
   Debug::ftnt("LibraryItem.dtor");
}

//------------------------------------------------------------------------------

const std::string& LibraryItem::Name() const
{
   Debug::ft("LibraryItem.Name");

   static string nameless(EMPTY_STR);

   if(!nameless.empty()) nameless.clear();
   return nameless;
}

//------------------------------------------------------------------------------

fn_name LibraryItem_Rename = "LibraryItem.Rename";

void LibraryItem::Rename(const std::string& name)
{
   Debug::ft(LibraryItem_Rename);

   Debug::SwLog(LibraryItem_Rename, strOver(this), 0);
}

//==============================================================================

bool LibItemSort::operator()
   (const LibraryItem* item1, const LibraryItem* item2) const
{
   auto result = strCompare(item1->Name(), item2->Name());
   if(result < 0) return true;
   if(result > 0) return false;
   return (item1 < item2);
}
}
