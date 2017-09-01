//==============================================================================
//
//  PotsShelfIpService.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "PotsShelf.h"
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
class PotsShelfServiceText : public CliText
{
public: PotsShelfServiceText();
};

fixed_string PotsShelfServiceStr = "POTS Shelf/UDP";
fixed_string PotsShelfServiceExpl = "POTS Shelf Protocol";

PotsShelfServiceText::PotsShelfServiceText() :
   CliText(PotsShelfServiceStr, PotsShelfServiceExpl) { }

//------------------------------------------------------------------------------

fixed_string PotsShelfIpPortKey = "PotsShelfIpPort";
fixed_string PotsShelfIpPortExpl = "POTS Shelf Protocol: UDP port";

fn_name PotsShelfIpService_ctor = "PotsShelfIpService.ctor";

PotsShelfIpService::PotsShelfIpService() : port_(NilIpPort)
{
   Debug::ft(PotsShelfIpService_ctor);

   auto port = strInt(PotsShelfIpPort);
   cfgPort_.reset(new IpPortCfgParm
      (PotsShelfIpPortKey, port.c_str(), &port_, PotsShelfIpPortExpl, this));
   Singleton< CfgParmRegistry >::Instance()->BindParm(*cfgPort_);
}

//------------------------------------------------------------------------------

fn_name PotsShelfIpService_dtor = "PotsShelfIpService.dtor";

PotsShelfIpService::~PotsShelfIpService()
{
   Debug::ft(PotsShelfIpService_dtor);
}

//------------------------------------------------------------------------------

fn_name PotsShelfIpService_CreateHandler = "PotsShelfIpService.CreateHandler";

InputHandler* PotsShelfIpService::CreateHandler(IpPort* port) const
{
   Debug::ft(PotsShelfIpService_CreateHandler);

   return new PotsShelfHandler(port);
}

//------------------------------------------------------------------------------

fn_name PotsShelfIpService_CreateText = "PotsShelfIpService.CreateText";

CliText* PotsShelfIpService::CreateText() const
{
   Debug::ft(PotsShelfIpService_CreateText);

   return new PotsShelfServiceText;
}

//------------------------------------------------------------------------------

Faction PotsShelfIpService::GetFaction() const { return PayloadFaction; }

//------------------------------------------------------------------------------

fixed_string PotsShelfIpServiceName = "POTS Shelf";

const char* PotsShelfIpService::Name() const
{
   return PotsShelfIpServiceName;
}

//------------------------------------------------------------------------------

ipport_t PotsShelfIpService::Port() const { return ipport_t(port_); }

//------------------------------------------------------------------------------

size_t PotsShelfIpService::RxSize() const { return IoThread::MaxRxBuffSize; }

//------------------------------------------------------------------------------

size_t PotsShelfIpService::TxSize() const { return IoThread::MaxTxBuffSize; }
}
