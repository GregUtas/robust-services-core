//==============================================================================
//
//  NbAppIds.h
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
#ifndef NBAPPIDS_H_INCLUDED
#define NBAPPIDS_H_INCLUDED

#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Object pool identifiers.  A new object pool must define an identifier here.
//  Fixed identifiers are required so that it would be possible to serialize an
//  object in one software release and import it into another release.
//
enum ObjectPoolIds
{
   MsgBufferObjPoolId = 1,
   IpBufferObjPoolId = 2,
   TinyBufferObjPoolId = 3,
   SmallBufferObjPoolId = 4,
   MediumBufferObjPoolId = 5,
   LargeBufferObjPoolId = 6,
   HugeBufferObjPoolId = 7,
   SbIpBufferObjPoolId = 8,
   BtIpBufferObjPoolId = 9,
   ContextObjPoolId = 10,
   MessageObjPoolId = 11,
   MsgPortObjPoolId = 12,
   ProtocolSMObjPoolId = 13,
   TimerObjPoolId = 14,
   EventObjPoolId = 15,
   ServiceSMObjPoolId = 16,
   MediaEndptObjPoolId = 17,
   DipIpBufferObjPoolId = 18
};

//------------------------------------------------------------------------------
//
//  Reserved software debugging flags.  Ad hoc usage should begin with the
//  last FlagId defined here.
//
constexpr FlagId DisableRootThread = 0;
constexpr FlagId ThreadReenterFlag = 1;
constexpr FlagId ThreadRecoverTrapFlag = 2;
constexpr FlagId ThreadCtorTrapFlag = 3;
constexpr FlagId ThreadCtorRetrapFlag = 4;
constexpr FlagId ThreadDtorTrapFlag = 5;
constexpr FlagId ThreadRetrapFlag = 6;
constexpr FlagId CallTrapFlag = 7;
constexpr FlagId CipIamTimeoutFlag = 8;
constexpr FlagId CipAlertingTimeoutFlag = 9;
constexpr FlagId FirstAppDebugFlag = 10;
}
#endif
