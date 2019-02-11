//==============================================================================
//
//  NbAppIds.h
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
#ifndef NBAPPIDS_H_INCLUDED
#define NBAPPIDS_H_INCLUDED

#include "NbTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Module identifiers.  A new module must define an identifier here.
//
//  Modules are currently initialized in ascending order of ModuleId.  The
//  order will eventually be based on that provided by Module::Dependencies
//  functions.  Until then, a new ModuleId must be inserted in the correct
//  location.  Renumbering existing identifiers or leaving gaps is OK.
//
                                           // namespace:      Module:
constexpr ModuleId NbModuleId = 1;         // NodeBase        NbModule
constexpr ModuleId NtModuleId = 2;         // NodeTools       NtModule
constexpr ModuleId CtModuleId = 3;         // CodeTools       CtModule
constexpr ModuleId NwModuleId = 4;         // NetworkBase     NwModule
constexpr ModuleId SbModuleId = 5;         // SessionBase     SbModule
constexpr ModuleId StModuleId = 6;         // SessionTools    StModule
constexpr ModuleId MbModuleId = 7;         // MediaBase       MbModule
constexpr ModuleId CbModuleId = 8;         // CallBase        CbModule
constexpr ModuleId PbModuleId = 9;         // PotsBase        PbModule
constexpr ModuleId OnModuleId = 10;        // OperationsNode  OnModule
constexpr ModuleId CnModuleId = 11;        // ControlNode     CnModule
constexpr ModuleId RnModuleId = 12;        // RoutingNode     RnModule
constexpr ModuleId SnModuleId = 13;        // ServiceNode     SnModule
constexpr ModuleId AnModuleId = 14;        // AccessNode      AnModule
constexpr ModuleId FirstAppModuleId = 15;  // start of applicaton modules

//------------------------------------------------------------------------------
//
//  Object pool identifiers.  A new object pool must define an identifier here.
//
constexpr ObjectPoolId ThreadObjPoolId = 1;
constexpr ObjectPoolId MsgBufferObjPoolId = 2;
constexpr ObjectPoolId SbIpBufferObjPoolId = 3;
constexpr ObjectPoolId BtIpBufferObjPoolId = 4;
constexpr ObjectPoolId ContextObjPoolId = 5;
constexpr ObjectPoolId MessageObjPoolId = 6;
constexpr ObjectPoolId MsgPortObjPoolId = 7;
constexpr ObjectPoolId ProtocolSMObjPoolId = 8;
constexpr ObjectPoolId TimerObjPoolId = 9;
constexpr ObjectPoolId EventObjPoolId = 10;
constexpr ObjectPoolId ServiceSMObjPoolId = 11;
constexpr ObjectPoolId MediaEndptObjPoolId = 12;
constexpr ObjectPoolId FirstAppObjPoolId = 13;    // start of application pools

//------------------------------------------------------------------------------
//
//  Reserved software debugging flags.  Ad hoc usage should begin after the
//  last FlagId defined here.
//
enum SwFlagIds
{
   ThreadCriticalFlag,
   ThreadCtorTrapFlag,
   ThreadRecoveryTrapFlag,
   ShowToolProgress,
   CallTrapFlag,
   CipAlwaysOverIpFlag,
   CipIamTimeoutFlag,
   CipAlertingTimeoutFlag
};
}
#endif
