//==============================================================================
//
//  SysIpL2Addr.cpp
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
#include "SysIpL2Addr.h"
#include <iosfwd>
#include <sstream>
#include "Debug.h"
#include "SysTypes.h"

using namespace NodeBase;
using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NetworkBase
{
SysIpL2Addr::SysIpL2Addr(ipv4addr_t v4Addr) : v4Addr_(v4Addr)
{
   Debug::ft("SysIpL2Addr.ctor(IPv4addr)");
}

//------------------------------------------------------------------------------

SysIpL2Addr::~SysIpL2Addr()
{
   Debug::ftnt("SysIpL2Addr.dtor");
}

//------------------------------------------------------------------------------

void SysIpL2Addr::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Object::Display(stream, prefix, options);

   stream << prefix << "v4Addr : " << to_str() << CRLF;
}

//------------------------------------------------------------------------------

string SysIpL2Addr::to_str() const
{
   std::ostringstream stream;

   stream << ((v4Addr_ & 0xff000000) >> 24) << '.';
   stream << ((v4Addr_ & 0x00ff0000) >> 16) << '.';
   stream << ((v4Addr_ & 0x0000ff00) >> 8) << '.';
   stream << (v4Addr_ & 0x000000ff);

   return stream.str();
}
}
