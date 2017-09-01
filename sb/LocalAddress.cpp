//==============================================================================
//
//  LocalAddress.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "LocalAddress.h"
#include <iosfwd>
#include <sstream>
#include "SysTypes.h"

using std::string;

//------------------------------------------------------------------------------

namespace SessionBase
{
const LocalAddress NilLocalAddress = {NIL_ID, 0, NIL_ID, NIL_ID};

//------------------------------------------------------------------------------

bool LocalAddress::operator==(const LocalAddress& that) const
{
   if(bid == NIL_ID)
   {
      return ((that.bid == NIL_ID) && (fid == that.fid));
   }

   return ((bid == that.bid) && (seq == that.seq) && (pid == that.pid));
}

//------------------------------------------------------------------------------

bool LocalAddress::operator!=(const LocalAddress& that) const
{
   return !(*this == that);
}

//------------------------------------------------------------------------------

string LocalAddress::to_str() const
{
   std::ostringstream stream;

   stream << "bid=" << bid;
   stream << "  seq=" << int(seq);
   stream << "  pid=" << int(pid);
   stream << "  fid=" << int(fid);

   return stream.str();
}
}
