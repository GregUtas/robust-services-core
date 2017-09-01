//==============================================================================
//
//  Q1Link.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
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

   Debug::ft(Q1Link_dtor);

   auto prev = this;
   auto curr = next;
   auto i = INT16_MAX;

   while((i >= 0) && (curr != nullptr))
   {
      if(curr == this)
      {
         Debug::SwErr(Q1Link_dtor, INT16_MAX - i, 0);
         prev->next = curr->next;
         return;
      }

      --i;
      prev = curr;
      curr = curr->next;
   }

   Debug::SwErr(Q1Link_dtor, INT16_MAX - i, 1);
}

//------------------------------------------------------------------------------

string Q1Link::to_str() const
{
   std::ostringstream stream;
   stream << next;
   return stream.str();
}
}
