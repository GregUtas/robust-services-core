//==============================================================================
//
//  Q2Link.cpp
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
#include "Q2Link.h"
#include <ostream>
#include "Debug.h"
#include "SysTypes.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
Q2Link::Q2Link() : next(nullptr), prev(nullptr) { }

//------------------------------------------------------------------------------

fn_name Q2Link_dtor = "Q2Link.dtor";

Q2Link::~Q2Link()
{
   if(next != nullptr)
   {
      Debug::SwLog(Q2Link_dtor, 0, 0);

      next->prev = prev;
      prev->next = next;
   }
}

//------------------------------------------------------------------------------

void Q2Link::Display(ostream& stream, const string& prefix) const
{
   stream << prefix << "next : " << next << CRLF;
   stream << prefix << "prev : " << prev << CRLF;
}
}
