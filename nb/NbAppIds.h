//==============================================================================
//
//  NbAppIds.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
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
