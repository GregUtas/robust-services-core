//==============================================================================
//
//  LibraryItem.cpp
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
#include "LibraryItem.h"
#include <ostream>
#include "Debug.h"
#include "SysTypes.h"

using namespace NodeBase;
using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace CodeTools
{
fn_name LibraryItem_ctor = "LibraryItem.ctor";

LibraryItem::LibraryItem(const string& name) : name_(name)
{
   Debug::ft(LibraryItem_ctor);
}

//------------------------------------------------------------------------------

fn_name LibraryItem_dtor = "LibraryItem.dtor";

LibraryItem::~LibraryItem()
{
   Debug::ft(LibraryItem_dtor);
}

//------------------------------------------------------------------------------

void LibraryItem::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Temporary::Display(stream, prefix, options);

   stream << prefix << "name : " << name_ << CRLF;
}
}
