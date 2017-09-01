//==============================================================================
//
//  PotsShelf.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef POTSSHELF_H_INCLUDED
#define POTSSHELF_H_INCLUDED

#include "MsgFactory.h"
#include "NbTypes.h"
#include "NwTypes.h"
#include "SbExtInputHandler.h"
#include "SbTypes.h"
#include "Switch.h"
#include "SysTypes.h"
#include "UdpIpService.h"

using namespace NodeBase;
using namespace SessionBase;
using namespace MediaBase;

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
   virtual const char* Name() const override;
   virtual ipport_t Port() const override;
   virtual Faction GetFaction() const override;
   virtual size_t RxSize() const override;
   virtual size_t TxSize() const override;
private:
   //  Private because this singleton is not subclassed.
   //
   PotsShelfIpService();

   //  Private because this singleton is not subclassed.
   //
   ~PotsShelfIpService();

   //  Overridden to create a CLI parameter for identifying the protocol.
   //
   virtual CliText* CreateText() const override;

   //  Overridden to create the POTS shelf input handler.
   //
   virtual InputHandler* CreateHandler(IpPort* port) const override;

   //  The port on which the protocol is running.
   //
   word port_;

   //  The configuration parameter for port_.
   //
   IpPortCfgParmPtr cfgPort_;
};

//------------------------------------------------------------------------------
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
   virtual void ReceiveBuff
      (MsgSize size, IpBufferPtr& buff, Faction faction) const override;
};

//------------------------------------------------------------------------------
//
//  Factory for a message to a POTS circuit.
//
class PotsShelfFactory : public MsgFactory
{
   friend class Singleton< PotsShelfFactory >;
private:
   //  Private because this singleton is not subclassed.
   //
   PotsShelfFactory();

   //  Private because this singleton is not subclassed.
   //
   ~PotsShelfFactory();

   //  Invoked when an invalid message is found.
   //
   static void DiscardMsg(const Message& msg, Switch::PortId port);

   //  Overridden to return a CLI parameter that identifies the factory.
   //
   virtual CliText* CreateText() const override;

   //  Overridden to wrap an incoming message.
   //
   virtual Message* AllocIcMsg(SbIpBufferPtr& buff) const override;

   //  Overridden to process an incoming message.
   //
   virtual void ProcessIcMsg(Message& msg) const override;

   //  Overridden to allocate an outgoing message that will be injected via
   //  a test tool.
   //
   virtual Message* AllocOgMsg(SignalId sid) const override;

   //  Overridden to inject a message on behalf of a test tool.
   //
   virtual bool InjectMsg(Message& msg) const override;

   //  Overridden to create a message wrapper when a test tool saves BUFF.
   //
   virtual Message* ReallocOgMsg(SbIpBufferPtr& buff) const override;
};
}
#endif
