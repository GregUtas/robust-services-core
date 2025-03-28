//==============================================================================
//
//  Q1Link.cpp
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
#include "Q1Link.h"
#include <cstdint>
#include <iosfwd>
#include <sstream>
#include "Debug.h"
#include "SysTypes.h"

using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
Q1Link::Q1Link() : next(nullptr) { }

//------------------------------------------------------------------------------

fn_name Q1Link_dtor = "Q1Link.dtor";

Q1Link::~Q1Link()
{
   //  If the item is still queued, exqueue it.  This is a serious problem if
   //  it is the tail item, because it will leave the queue head pointing to
   //  a deleted item.
   //
   if(next == nullptr) return;

   Debug::ftnt(Q1Link_dtor);

   auto prev = this;
   auto curr = next;
   auto i = INT16_MAX;

   while((i >= 0) && (curr != nullptr))
   {
      if(curr == this)
      {
         auto before = INT16_MAX - i;

         if(before == 0)
         {
            Debug::SwLog(Q1Link_dtor, "tail item was still queued", before);
            prev->next = nullptr;
         }
         else
         {
            Debug::SwLog(Q1Link_dtor, "item was still queued", before);
            prev->next = curr->next;
         }
         return;
      }

      --i;
      prev = curr;
      curr = curr->next;
   }

   Debug::SwLog(Q1Link_dtor, "item is still queued", INT16_MAX - i);
}

//------------------------------------------------------------------------------

string Q1Link::to_str() const
{
   std::ostringstream stream;
   stream << next;
   return stream.str();
}
}
