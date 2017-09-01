//==============================================================================
//
//  Q2Link.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "Q2Link.h"
#include <ostream>
#include "Debug.h"
#include "SysTypes.h"

using std::ostream;

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
      Debug::SwErr(Q2Link_dtor, 0, 0);

      next->prev = prev;
      prev->next = next;
   }
}

//------------------------------------------------------------------------------

void Q2Link::Display(ostream& stream, const std::string& prefix) const
{
   stream << prefix << "next : " << next << CRLF;
   stream << prefix << "prev : " << prev << CRLF;
}
}
