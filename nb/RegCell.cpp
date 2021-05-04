//==============================================================================
//
//  RegCell.cpp
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
#include "RegCell.h"
#include "Algorithms.h"
#include "Debug.h"

using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
RegCell::RegCell() : id(NIL_ID), bound(false) { }

//------------------------------------------------------------------------------

fn_name RegCell_dtor = "RegCell.dtor";

RegCell::~RegCell()
{
   if(bound)
   {
      Debug::SwLog(RegCell_dtor, "item is still registered", id);
   }
}

//------------------------------------------------------------------------------

fn_name RegCell_SetId = "RegCell.SetId";

void RegCell::SetId(id_t cid)
{
   if(bound)
      Debug::SwLog(RegCell_SetId, "item already registered", pack2(id, cid));
   else
      id = cid;
}

//------------------------------------------------------------------------------

string RegCell::to_str() const
{
   auto str = std::to_string(id);
   if(!bound) str += " (not bound)";
   return str;
}
}
