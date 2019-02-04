//==============================================================================
//
//  BcProtocol.h
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
#ifndef BCPROTOCOL_H_INCLUDED
#define BCPROTOCOL_H_INCLUDED

#include "MediaPsm.h"
#include "SbInputHandler.h"
#include "Signal.h"
#include "SsmFactory.h"
#include "TcpIpService.h"
#include "TlvMessage.h"
#include "TlvParameter.h"
#include "TlvProtocol.h"
#include "UdpIpService.h"
#include "Clock.h"
#include "NbTypes.h"
#include "NwTypes.h"
#include "SbTypes.h"
#include "SysTypes.h"
#include "TcpIoThread.h"

namespace CallBase
{
   class DigitString;
   struct ProgressInfo;
   struct CauseInfo;
   struct RouteResult;
}

using namespace MediaBase;

//------------------------------------------------------------------------------

namespace CallBase
{
//  Call Interworking Protocol (CIP).  This protocol sets up a call between
//  an originating and a terminating interface.  CIP is based on ISUP, which
//  is probably the most commonly used call setup protocol.  The version of
//  CIP defined here is sufficient for demonstration purposes, but the full
//  version would be much larger.
//
class CipProtocol : public TlvProtocol
{
   friend class Singleton< CipProtocol >;
public:
   //  Timeout while waiting for a response to an IAM, which is the first
   //  signal sent from the originator of a call to the terminator.
   //
   static const secs_t IamTimeout = 10;

   //  Identifies the timer for IamTimeout.
   //
   static const TimerId IamTimeoutId = 1;
private:
   //  Private because this singleton is not subclassed.
   //
   CipProtocol();

   //  Private because this singleton is not subclassed.
   //
   ~CipProtocol();
};

//------------------------------------------------------------------------------
//
//  CIP signals.
//
class CipSignal : public Signal
{
public:
   static const Id IAM = NextId;      // Initial Address Message
   static const Id CPG = NextId + 1;  // Call Progress Message
   static const Id ANM = NextId + 2;  // Answer Message
   static const Id REL = NextId + 3;  // Release Message
protected:
   //  Protected because this class is virtual.
   //
   explicit CipSignal(Id sid);

   //  Protected because subclasses should be singletons.
   //
   virtual ~CipSignal() = default;
};

//------------------------------------------------------------------------------
//
//  CIP parameters.
//
class CipParameter : public TlvParameter
{
public:
   static const Id Route           = NextId;      // destination factory
   static const Id Calling         = NextId + 1;  // calling address
   static const Id Called          = NextId + 2;  // called address
   static const Id OriginalCalling = NextId + 3;  // original calling address
   static const Id OriginalCalled  = NextId + 4;  // original called address
   static const Id Progress        = NextId + 5;  // progress indicator for CPG
   static const Id Cause           = NextId + 6;  // cause value for REL
   static const Id Media           = NextId + 7;  // specifies a media address
protected:
   //  Protected because this class is virtual.
   //
   explicit CipParameter(Id pid);

   //  Protected because subclasses should be singletons.
   //
   virtual ~CipParameter() = default;
};

//------------------------------------------------------------------------------
//
//  CIP message.
//
class CipMessage : public TlvMessage
{
public:
   //  Constructs an incoming message from BUFF.
   //
   explicit CipMessage(SbIpBufferPtr& buff);

   //  Constructs an outgoing message, initially of SIZE bytes, to
   //  be sent from PSM.
   //
   CipMessage(ProtocolSM* psm, MsgSize size);

   //  Public because applications may create and destroy instances.
   //
   virtual ~CipMessage();

   //  Adds a route parameter to an IAM.
   //
   RouteResult* AddRoute(const RouteResult& route);

   //  Adds an address parameter to an IAM.  PID is the type of address.
   //  o The calling and called addresses are mandatory.
   //  o In the redirection chain A-B-C
   //    --A is included as the original calling address
   //    --B is included as the calling address
   //    --C is included as the called address
   //  o In the redirection chain A-B-C-D
   //    --A is included as the original calling address
   //    --B is included as the original called address
   //    --C is included as the calling address
   //    --D is included as the called address
   //    --any subsequent redirection attempt is blocked
   //
   DigitString* AddAddress(const DigitString& ds, CipParameter::Id pid);

   //  Adds a progress indicator to a CPG.
   //
   ProgressInfo* AddProgress(const ProgressInfo& progress);

   //  Adds a cause value to a REL.
   //
   CauseInfo* AddCause(const CauseInfo& cause);

   //  Adds a media address to the message.
   //
   MediaInfo* AddMedia(const MediaInfo& media);
};

//------------------------------------------------------------------------------
//
//  Basic call PSM.  This is a virtual base class that allows CIP to be used
//  on both the network and user sides of a basic call.  The former is for
//  interworking (setting up a call between two interfaces), and the latter
//  is for proxy calls (handling a call in which a subscriber is logically,
//  but not physically, present).
//
class BcPsm : public MediaPsm
{
public:
   //  States for a PSM that supports CIP.
   //
   static const StateId IamSent = Idle + 1;
   static const StateId IamRcvd = Idle + 2;
   static const StateId EosSent = Idle + 3;
   static const StateId EosRcvd = Idle + 4;
   static const StateId AltSent = Idle + 5;
   static const StateId AltRcvd = Idle + 6;
   static const StateId AnmSent = Idle + 7;
   static const StateId AnmRcvd = Idle + 8;
   static const StateId SusSent = Idle + 9;
   static const StateId SusRcvd = Idle + 10;

   //  Searches the received message queue for a message whose signal
   //  matches SID.
   //
   CipMessage* FindRcvdMsg(CipSignal::Id sid) const;

   //  Overridden to display member variables.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;
protected:
   //  Creates a PSM from an adjacent layer.  The arguments are the same as
   //  those for the base class.  Protected because this class is virtual.
   //
   BcPsm(FactoryId fid, ProtocolLayer& adj, bool upper);

   //  Creates a PSM that will send an initial message.  The arguments are
   //  the same as those for the base class.  Protected because this class
   //  is virtual.
   //
   explicit BcPsm(FactoryId fid);

   //  Protected to restrict deletion.  Virtual to allow subclassing.
   //
   virtual ~BcPsm();

   //  Overridden to create an outgoing message for a media parameter.
   //
   virtual void EnsureMediaMsg() override;

   //  Overridden to update the PSM's state when a message is received.
   //
   virtual IncomingRc ProcessIcMsg(Message& msg, Event*& event) override;

   //  Overridden to update the PSM's state when a message is sent.
   //
   virtual OutgoingRc ProcessOgMsg(Message& msg) override;

   //  Overridden to inject a REL if the node associated with the PSM's
   //  peer goes out of service.
   //
   virtual void InjectFinalMsg() override;
private:
   //  Determines whether a timer is started when sending an IAM.
   //
   virtual bool UsesIamTimer() const { return true; }

   //  Overridden to send a REL if the PSM is not idle when its context
   //  is destroyed.
   //
   virtual void SendFinalMsg() override;

   //  Set if the IAM timer is running.
   //
   bool iamTimer_;
};

//------------------------------------------------------------------------------
//
//  CIP protocol state machine.
//
class CipPsm : public BcPsm
{
public:
   //  Creates a PSM that will send an IAM.
   //
   CipPsm();

   //  Creates a PSM from an adjacent layer.  The arguments are the same
   //  as those for the base class.
   //
   CipPsm(FactoryId fid, ProtocolLayer& adj, bool upper);
private:
   //  Private to restrict deletion.  Not subclassed.
   //
   ~CipPsm();

   //  Overridden to create a TCP socket if CIP is using TCP.
   //
   virtual SysSocket* CreateAppSocket() override;

   //  Overridden to specify that messages can bypass the IP stack.
   //
   virtual Message::Route Route() const override;
};

//------------------------------------------------------------------------------
//
//  CIP over UDP.
//
class CipUdpService : public UdpIpService
{
   friend class Singleton< CipUdpService >;
public:
   //  Overridden to return the service's attributes.
   //
   virtual const char* Name() const override { return "Call Interworking"; }
   virtual ipport_t Port() const override { return ipport_t(port_); }
   virtual Faction GetFaction() const override { return PayloadFaction; }
private:
   //  Private because this singleton is not subclassed.
   //
   CipUdpService();

   //  Private because this singleton is not subclassed.
   //
   ~CipUdpService();

   //  Overridden to create a CLI parameter for identifying the protocol.
   //
   virtual CliText* CreateText() const override;

   //  Overridden to create the CIP input handler.
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
//
//  CIP over TCP.
//
class CipTcpService : public TcpIpService
{
   friend class Singleton< CipTcpService >;
public:
   //  Overridden to return the service's attributes.
   //
   virtual const char* Name() const override { return "Call Interworking"; }
   virtual ipport_t Port() const override { return ipport_t(port_); }
   virtual Faction GetFaction() const override { return PayloadFaction; }
   virtual size_t MaxConns() const override { return TcpIoThread::MaxConns; }
   virtual size_t MaxBacklog() const override { return 200; }
private:
   //  Private because this singleton is not subclassed.
   //
   CipTcpService();

   //  Private because this singleton is not subclassed.
   //
   ~CipTcpService();

   //  Overridden to return the buffer sizes for an application socket.
   //
   virtual void GetAppSocketSizes
      (size_t& rxSize, size_t& txSize) const override;

   //  Overridden to create a CLI parameter for identifying the protocol.
   //
   virtual CliText* CreateText() const override;

   //  Overridden to create the CIP input handler.
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
//
//  CIP input handler.
//
class CipHandler : public SbInputHandler
{
public:
   //  Registers the input handler with PORT.
   //
   explicit CipHandler(IpPort* port);

   //  Not subclassed.
   //
   ~CipHandler();
};

//------------------------------------------------------------------------------
//
//  Base class for CIP factories.
//
class CipFactory : public SsmFactory
{
protected:
   //  Protected because this class is virtual.
   //
   CipFactory(Id fid, const char* name);

   //  Protected because subclasses should be singletons.
   //
   virtual ~CipFactory();

   //  Overridden to allocate a message to receive BUFF.
   //
   virtual Message* AllocIcMsg(SbIpBufferPtr& buff) const override;

   //  Overridden to allocate a message that will be sent by a test tool.
   //
   virtual Message* AllocOgMsg(SignalId sid) const override;

   //  Overridden to allocate a message to save BUFF.
   //
   virtual Message* ReallocOgMsg(SbIpBufferPtr& buff) const override;
};

//------------------------------------------------------------------------------
//
//  CIP factory for originating (outgoing) calls.
//
class CipObcFactory : public CipFactory
{
   friend class Singleton< CipObcFactory >;
private:
   //  Private because this singleton is not subclassed.
   //
   CipObcFactory();

   //  Private because this singleton is not subclassed.
   //
   ~CipObcFactory();

   //  Overridden to create a PSM to support InjectCommand.
   //
   virtual ProtocolSM* AllocOgPsm(const Message& msg) const override;

   //  Overridden to return a CLI parameter that identifies the factory.
   //
   virtual CliText* CreateText() const override;
};

//------------------------------------------------------------------------------
//
//  CIP factory for terminating (incoming) calls.
//
class CipTbcFactory : public CipFactory
{
   friend class Singleton< CipTbcFactory >;
private:
   //  Private because this singleton is not subclassed.
   //
   CipTbcFactory();

   //  Private because this singleton is not subclassed.
   //
   ~CipTbcFactory();

   //  Overridden to return a CLI parameter that identifies the factory.
   //
   virtual CliText* CreateText() const override;

   //  Overridden to create a CIP PSM when a CIP IAM arrives.
   //
   virtual ProtocolSM* AllocIcPsm
      (const Message& msg, ProtocolLayer& lower) const override;

   //  Overridden to create the root SSM when a CIP IAM arrives on PSM
   //  to create the recipient's half of a new session.
   //
   virtual RootServiceSM* AllocRoot
      (const Message& msg, ProtocolSM& psm) const override;
};
}
#endif
