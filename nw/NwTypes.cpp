//==============================================================================
//
//  NwTypes.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "NwTypes.h"
#include "SysTypes.h"

using std::ostream;

//------------------------------------------------------------------------------

namespace NodeBase
{
fixed_string ProtocolStrings[IpProtocol_N + 1] =
{
   "Any",
   "UDP",
   "TCP",
   ERROR_STR
};

ostream& operator<<(ostream& stream, IpProtocol proto)
{
   if((proto >= 0) && (proto < IpProtocol_N))
      stream << ProtocolStrings[proto];
   else
      stream << ProtocolStrings[IpProtocol_N];
   return stream;
}
}