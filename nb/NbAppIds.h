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
enum ModuleIds
{                    // namespace:      Module:
   NbModuleId = 1,   // NodeBase        NbModule
   NtModuleId = 2,   // NodeTools       NtModule
   CtModuleId = 3,   // CodeTools       CtModule
   NwModuleId = 4,   // NodeBase        NwModule
   SbModuleId = 5,   // SessionBase     SbModule
   StModuleId = 6,   // SessionTools    StModule
   MbModuleId = 7,   // MediaBase       MbModule
   CbModuleId = 8,   // CallBase        CbModule
   PbModuleId = 9,   // PotsBase        PbModule
   OnModuleId = 10,  // OperationsNode  OnModule
   CnModuleId = 11,  // ControlNode     CnModule
   RnModuleId = 12,  // RoutingNode     RnModule
   SnModuleId = 13,  // ServiceNode     SnModule
   AnModuleId = 14   // AccessNode      AnModule
};

//------------------------------------------------------------------------------
//
//  Object pool identifiers.  A new object pool must define an identifier here.
//
enum ObjectPoolIds
{
   ThreadObjPoolId = 1,
   MsgBufferObjPoolId = 2,
   SbIpBufferObjPoolId = 3,
   BtIpBufferObjPoolId = 4,
   ContextObjPoolId = 5,
   MessageObjPoolId = 6,
   MsgPortObjPoolId = 7,
   ProtocolSMObjPoolId = 8,
   TimerObjPoolId = 9,
   EventObjPoolId = 10,
   ServiceSMObjPoolId = 11,
   MediaEndptObjPoolId = 12
};

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
