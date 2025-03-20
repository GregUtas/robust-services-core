//==============================================================================
//
//  LocalAddress.cpp
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
#include "LocalAddress.h"
#include <iosfwd>
#include <sstream>
#include "SysTypes.h"

using namespace NodeBase;
using std::string;

//------------------------------------------------------------------------------

namespace SessionBase
{
LocalAddress::LocalAddress() :
   bid(NodeBase::NIL_ID),
   seq(0),
   pid(NodeBase::NIL_ID),
   fid(NodeBase::NIL_ID)
{
}

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
