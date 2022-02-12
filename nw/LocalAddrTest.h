//==============================================================================
//
//  LocalAddrTest.h
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
#ifndef LOCALADDRTEST_H_INCLUDED
#define LOCALADDRTEST_H_INCLUDED

#include "Deferred.h"
#include "InputHandler.h"
#include "Thread.h"
#include "UdpIpService.h"
#include "Duration.h"
#include "IpPortCfgParm.h"
#include "NbTypes.h"
#include "NwTypes.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace NetworkBase
{
//  Thread for sending messages to each of this element's addresses to
//  confirm their validity.
//
class SendLocalThread : public Thread
{
   friend class Singleton< SendLocalThread >;
public:
   //  Invoked from the CLI to retest the addresses.
   //
   void Retest();

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
private:
   //  Private because this is a singleton.
   //
   SendLocalThread();

   //  Private because this is a singleton.
   //
   ~SendLocalThread();

   //  Overridden to return a name for the thread.
   //
   c_string AbbrName() const override;

   //  Overridden to delete the singleton.
   //
   void Destroy() override;

   //  Overridden to send a message to each of this element's addresses.
   //
   void Enter() override;

   //  Set when performing a retest.
   //
   bool retest_;
};

//------------------------------------------------------------------------------

class LocalAddrRetest : public Deferred
{
public:
   //  Recreates SendLocalThread after TIMEOUT.
   //
   explicit LocalAddrRetest(secs_t timeout);

   //  Not subclassed.
   //
   ~LocalAddrRetest();

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
private:
   //  Creates SendLocalThread to test the local address.
   //
   void EventHasOccurred(Event event) override;
};

//------------------------------------------------------------------------------
//
//  Local address test protocol over UDP.
//
class SendLocalIpService : public UdpIpService
{
   friend class Singleton< SendLocalIpService >;
public:
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;
   Faction GetFaction() const override { return MaintenanceFaction; }
   c_string Name() const override { return "Local Address Test"; }
   void Patch(sel_t selector, void* arguments) override;
   ipport_t Port() const override { return portCfg_->GetPort(); }
private:
   //  Private because this is a singleton.
   //
   SendLocalIpService();

   //  Private because this is a singleton.
   //
   ~SendLocalIpService();

   //  Overridden to create the input handler for receiving the messages.
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
//  Input handler for a message to sent to confirm the validity of one of
//  this element's addresses.
//
class LocalAddrHandler : public InputHandler
{
public:
   //  Registers the input handler against PORT.
   //
   explicit LocalAddrHandler(IpPort* port);

   //  Not subclassed.
   //
   ~LocalAddrHandler();

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
private:
   //  Overridden to record successful reception of a message.
   //
   void ReceiveBuff
      (IpBufferPtr& buff, size_t size, Faction faction) const override;
};
}
#endif
