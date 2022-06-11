//==============================================================================
//
//  SbTypes.cpp
//
//  Copyright (C) 2013-2022  Greg Utas
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
#include "SbTypes.h"
#include <ostream>

using namespace NodeBase;
using std::ostream;

//------------------------------------------------------------------------------

namespace SessionBase
{
fixed_string ContextTypeStrings[ContextType_N + 1] =
{
   "msg",  // SingleMsg
   "psm",  // SinglePort
   "ssm",  // MultiPort
   "???"
};

ostream& operator<<(ostream& stream, ContextType type)
{
   if((type >= 0) && (type < ContextType_N))
      stream << ContextTypeStrings[type];
   else
      stream << ContextTypeStrings[ContextType_N];
   return stream;
}

c_string strContextType(ContextType type)
{
   if((type >= 0) && (type < ContextType_N)) return ContextTypeStrings[type];
   return ContextTypeStrings[ContextType_N];
}

//------------------------------------------------------------------------------

fixed_string MsgPriorityStrings[MAX_PRIORITY + 2] =
{
   "ingress",
   "egress",
   "progress",
   "immediate",
   ERROR_STR
};

c_string strMsgPriority(MsgPriority prio)
{
   return (prio <= MAX_PRIORITY ?
      MsgPriorityStrings[prio] : MsgPriorityStrings[MAX_PRIORITY + 1]);
}
}
