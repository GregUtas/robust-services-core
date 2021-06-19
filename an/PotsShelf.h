//==============================================================================
//
//  PotsShelf.h
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
#ifndef POTSSHELF_H_INCLUDED
#define POTSSHELF_H_INCLUDED

#include "MsgFactory.h"
#include "SbExtInputHandler.h"
#include "UdpIpService.h"
#include "IpPortCfgParm.h"
#include "NbTypes.h"
#include "NwTypes.h"
#include "Switch.h"

using namespace NodeBase;
using namespace SessionBase;
using namespace MediaBase;
using namespace NetworkBase;

//------------------------------------------------------------------------------

namespace PotsBase
{
//  POTS shelf protocol over UDP.
//
class PotsShelfIpService : public UdpIpService
{
   friend class Singleton< PotsShelfIpService >;
public:
   //  Overridden to return the service's attributes.
   //
   c_string Name() const override { return "POTS Shelf"; }
   ipport_t Port() const override { return portCfg_->GetPort(); }
   Faction GetFaction() const override { return PayloadFaction; }
private:
   //  Private because this is a singleton.
   //
   PotsShelfIpService();

   //  Private because this is a singleton.
   //
   ~PotsShelfIpService();

   //  Overridden to create the POTS shelf input handler.
   //
   InputHandler* CreateHandler(IpPort* port) const override;

   //  Overridden to create a CLI parameter for identifying the protocol.
   //
   CliText* CreateText() const override;

   //  The configuration parameter for setting the service's port.
   //
   IpPortCfgParmPtr portCfg_;
};

//------------------------------------------------------------------------------
//
//  Input handler for a message to a POTS circuit.
//
class PotsShelfHandler : public SbExtInputHandler
{
public:
   //  Registers the input handler against PORT.
   //
   explicit PotsShelfHandler(IpPort* port);

   //  Not subclassed.
   //
   ~PotsShelfHandler();
private:
   //  Overridden to add a SessionBase header to a message arriving over the
   //  IP stack.
   //
   void ReceiveBuff
      (IpBufferPtr& buff, size_t size, Faction faction) const override;
};

//------------------------------------------------------------------------------
//
//  Factory for a message to a POTS circuit.
//
class PotsShelfFactory : public MsgFactory
{
   friend class Singleton< PotsShelfFactory >;

   //  Private because this is a singleton.
   //
   PotsShelfFactory();

   //  Private because this is a singleton.
   //
   ~PotsShelfFactory();

   //  Invoked when an invalid message is found.
   //
   static void DiscardMsg(const Message& msg, Switch::PortId port);

   //  Overridden to wrap an incoming message.
   //
   Message* AllocIcMsg(SbIpBufferPtr& buff) const override;

   //  Overridden to allocate an outgoing message that will be injected via
   //  a test tool.
   //
   Message* AllocOgMsg(SignalId sid) const override;

   //  Overridden to return a CLI parameter that identifies the factory.
   //
   CliText* CreateText() const override;

   //  Overridden to inject a message on behalf of a test tool.
   //
   bool InjectMsg(Message& msg) const override;

   //  Overridden to process an incoming message.
   //
   void ProcessIcMsg(Message& msg) const override;

   //  Overridden to create a message wrapper when a test tool saves BUFF.
   //
   Message* ReallocOgMsg(SbIpBufferPtr& buff) const override;
};
}
#endif
