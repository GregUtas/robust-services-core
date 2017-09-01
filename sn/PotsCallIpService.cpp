//==============================================================================
//
//  PotsCallIpService.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "PotsSessions.h"
#include <memory>
#include <string>
#include "CfgParmRegistry.h"
#include "CliText.h"
#include "Debug.h"
#include "Formatters.h"
#include "IoThread.h"
#include "IpPortCfgParm.h"
#include "Singleton.h"

//------------------------------------------------------------------------------

namespace PotsBase
{
class PotsCallServiceText : public CliText
{
public: PotsCallServiceText();
};

fixed_string PotsCallServiceStr = "POTS Call/UDP";
fixed_string PotsCallServiceExpl = "POTS Call Protocol";

PotsCallServiceText::PotsCallServiceText() :
   CliText(PotsCallServiceStr, PotsCallServiceExpl) { }

//------------------------------------------------------------------------------

fixed_string PotsCallIpPortKey = "PotsCallIpPort";
fixed_string PotsCallIpPortExpl = "POTS Call Protocol: UDP port";

fn_name PotsCallIpService_ctor = "PotsCallIpService.ctor";

PotsCallIpService::PotsCallIpService() : port_(NilIpPort)
{
   Debug::ft(PotsCallIpService_ctor);

   auto port = strInt(PotsCallIpPort);
   cfgPort_.reset(new IpPortCfgParm
      (PotsCallIpPortKey, port.c_str(), &port_, PotsCallIpPortExpl, this));
   Singleton< CfgParmRegistry >::Instance()->BindParm(*cfgPort_);
}

//------------------------------------------------------------------------------

fn_name PotsCallIpService_dtor = "PotsCallIpService.dtor";

PotsCallIpService::~PotsCallIpService()
{
   Debug::ft(PotsCallIpService_dtor);
}

//------------------------------------------------------------------------------

fn_name PotsCallIpService_CreateHandler = "PotsCallIpService.CreateHandler";

InputHandler* PotsCallIpService::CreateHandler(IpPort* port) const
{
   Debug::ft(PotsCallIpService_CreateHandler);

   return new PotsCallHandler(port);
}

//------------------------------------------------------------------------------

fn_name PotsCallIpService_CreateText = "PotsCallIpService.CreateText";

CliText* PotsCallIpService::CreateText() const
{
   Debug::ft(PotsCallIpService_CreateText);

   return new PotsCallServiceText;
}

//------------------------------------------------------------------------------

Faction PotsCallIpService::GetFaction() const { return PayloadFaction; }

//------------------------------------------------------------------------------

fixed_string PotsCallIpServiceName = "POTS Call";

const char* PotsCallIpService::Name() const { return PotsCallIpServiceName; }

//------------------------------------------------------------------------------

ipport_t PotsCallIpService::Port() const { return ipport_t(port_); }

//------------------------------------------------------------------------------

size_t PotsCallIpService::RxSize() const { return IoThread::MaxRxBuffSize; }

//------------------------------------------------------------------------------

size_t PotsCallIpService::TxSize() const { return IoThread::MaxTxBuffSize; }
}
