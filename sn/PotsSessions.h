//==============================================================================
//
//  PotsSessions.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef POTSSESSIONS_H_INCLUDED
#define POTSSESSIONS_H_INCLUDED

#include "BcSessions.h"
#include "Clock.h"
#include "EventHandler.h"
#include "Initiator.h"
#include "NbTypes.h"
#include "NwTypes.h"
#include "ProxyBcSessions.h"
#include "SbExtInputHandler.h"
#include "SbTypes.h"
#include "SysTypes.h"
#include "UdpIpService.h"

namespace PotsBase
{
   struct PotsHeaderInfo;
   class PotsProfile;
   class PotsTreatment;
}

using namespace NodeBase;
using namespace SessionBase;
using namespace CallBase;

//------------------------------------------------------------------------------

namespace PotsBase
{
//  POTS call protocol over UDP.
//
class PotsCallIpService : public UdpIpService
{
   friend class Singleton< PotsCallIpService >;
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
   PotsCallIpService();

   //  Private because this singleton is not subclassed.
   //
   ~PotsCallIpService();

   //  Overridden to create a CLI parameter for identifying the protocol.
   //
   virtual CliText* CreateText() const override;

   //  Overridden to create the POTS call input handler.
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
//  Input handler for messages arriving from POTS circuits.
//
class PotsCallHandler : public SbExtInputHandler
{
public:
   //  Registers the input handler against PORT.
   //
   explicit PotsCallHandler(IpPort* port);

   //  Not subclassed.
   //
   ~PotsCallHandler();
private:
   //  Overridden to add a SessionBase header to a message arriving over the
   //  IP stack.
   //
   virtual void ReceiveBuff
      (MsgSize size, IpBufferPtr& buff, Faction faction) const override;

   //  Discards BUFF when it is invalid.  ERRVAL is included in the log.
   //
   void DiscardBuff
      (const IpBufferPtr& buff, const PotsHeaderInfo* phi, word errval) const;
};

//------------------------------------------------------------------------------
//
//  Factory for POTS originations.
//
class PotsCallFactory : public BcFactory
{
   friend class Singleton< PotsCallFactory >;
private:
   //  Private because this singleton is not subclassed.
   //
   PotsCallFactory();

   //  Private because this singleton is not subclassed.
   //
   ~PotsCallFactory();

   //  Sends a Release message when discarding an offhook-onhook message pair.
   //  MSG1 is the offhook message.
   //
   static void SendRelease(const Message& msg1);

   //  Overridden to return a CLI parameter that identifies the factory.
   //
   virtual CliText* CreateText() const override;

   //  Overridden to create a root SSM when MSG arrives to create a new
   //  session.
   //
   virtual RootServiceSM* AllocRoot
      (const Message& msg, ProtocolSM& psm) const override;

   //  Overridden to create a POTS call PSM.
   //
   virtual ProtocolSM* AllocIcPsm
      (const Message& msg, ProtocolLayer& lower) const override;

   //  Overridden to allocate a message to receive BUFF.
   //
   virtual Message* AllocIcMsg(SbIpBufferPtr& buff) const override;

   //  Overridden to allocate a message that will be sent by a test tool.
   //
   virtual Message* AllocOgMsg(SignalId sid) const override;

   //  Overridden to allocate a message to save BUFF.
   //
   virtual Message* ReallocOgMsg(SbIpBufferPtr& buff) const override;

   //  Overridden to record PORT in the user's profile.
   //
   virtual void PortAllocated
      (const MsgPort& port, const Message* msg) const override;

   //  Overridden to screen subsequent messages received while an offhook
   //  is waiting on the ingress work queue.
   //
   virtual bool ScreenIcMsgs(Q1Way< Message >& msgq) override;

   //  Overridden to verify that the DN referenced by RID is registered.
   //
   virtual Cause::Ind VerifyRoute(RouteResult::Id rid) const override;
};

//------------------------------------------------------------------------------
//
//  POTS basic call service.
//
class PotsBcService : public ProxyBcService
{
   friend class Singleton< PotsBcService >;
private:
   //  Private because this singleton is not subclassed.  Registers all
   //  POTS states, event handlers, and triggers.
   //
   PotsBcService();

   //  Private because this singleton is not subclassed.
   //
   ~PotsBcService();
};

//------------------------------------------------------------------------------
//
//  POTS basic call states.
//
class PotsBcNull : public ProxyBcNull
{
   friend class Singleton< PotsBcNull >;
private:
   PotsBcNull();
   ~PotsBcNull();
};

class PotsBcAuthorizingOrigination : public ProxyBcAuthorizingOrigination
{
   friend class Singleton< PotsBcAuthorizingOrigination >;
private:
   PotsBcAuthorizingOrigination();
   ~PotsBcAuthorizingOrigination();
};

class PotsBcCollectingInformation : public ProxyBcCollectingInformation
{
   friend class Singleton< PotsBcCollectingInformation >;
private:
   PotsBcCollectingInformation();
   ~PotsBcCollectingInformation();
};

class PotsBcAnalyzingInformation : public ProxyBcAnalyzingInformation
{
   friend class Singleton< PotsBcAnalyzingInformation >;
private:
   PotsBcAnalyzingInformation();
   ~PotsBcAnalyzingInformation();
};

class PotsBcSelectingRoute : public ProxyBcSelectingRoute
{
   friend class Singleton< PotsBcSelectingRoute >;
private:
   PotsBcSelectingRoute();
   ~PotsBcSelectingRoute();
};

class PotsBcAuthorizingCallSetup : public ProxyBcAuthorizingCallSetup
{
   friend class Singleton< PotsBcAuthorizingCallSetup >;
private:
   PotsBcAuthorizingCallSetup();
   ~PotsBcAuthorizingCallSetup();
};

class PotsBcSendingCall : public ProxyBcSendingCall
{
   friend class Singleton< PotsBcSendingCall >;
private:
   PotsBcSendingCall();
   ~PotsBcSendingCall();
};

class PotsBcOrigAlerting : public ProxyBcOrigAlerting
{
   friend class Singleton< PotsBcOrigAlerting >;
private:
   PotsBcOrigAlerting();
   ~PotsBcOrigAlerting();
};

class PotsBcAuthorizingTermination : public ProxyBcAuthorizingTermination
{
   friend class Singleton< PotsBcAuthorizingTermination >;
private:
   PotsBcAuthorizingTermination();
   ~PotsBcAuthorizingTermination();
};

class PotsBcSelectingFacility : public ProxyBcSelectingFacility
{
   friend class Singleton< PotsBcSelectingFacility >;
private:
   PotsBcSelectingFacility();
   ~PotsBcSelectingFacility();
};

class PotsBcPresentingCall : public ProxyBcPresentingCall
{
   friend class Singleton< PotsBcPresentingCall >;
private:
   PotsBcPresentingCall();
   ~PotsBcPresentingCall();
};

class PotsBcTermAlerting : public ProxyBcTermAlerting
{
   friend class Singleton< PotsBcTermAlerting >;
private:
   PotsBcTermAlerting();
   ~PotsBcTermAlerting();
};

class PotsBcActive : public ProxyBcActive
{
   friend class Singleton< PotsBcActive >;
private:
   PotsBcActive();
   ~PotsBcActive();
};

class PotsBcLocalSuspending : public ProxyBcLocalSuspending
{
   friend class Singleton< PotsBcLocalSuspending >;
private:
   PotsBcLocalSuspending();
   ~PotsBcLocalSuspending();
};

class PotsBcRemoteSuspending : public ProxyBcRemoteSuspending
{
   friend class Singleton< PotsBcRemoteSuspending >;
private:
   PotsBcRemoteSuspending();
   ~PotsBcRemoteSuspending();
};

class PotsBcException : public ProxyBcException
{
   friend class Singleton< PotsBcException >;
private:
   PotsBcException();
   ~PotsBcException();
};

//------------------------------------------------------------------------------
//
//  POTS basic call triggers.
//
class PotsAuthorizeOriginationSap : public BcTrigger
{
   friend class Singleton< PotsAuthorizeOriginationSap >;
public:
   //  If both SUS and BOC are subscribed, SUS has priority.
   //
   static const Initiator::Priority PotsSusPriority = 50;
   static const Initiator::Priority PotsBocPriority = 45;
private:
   PotsAuthorizeOriginationSap();
   ~PotsAuthorizeOriginationSap();
};

class PotsCollectInformationSap : public BcTrigger
{
   friend class Singleton< PotsCollectInformationSap >;
public:
   //  HTL and WML are incompatiable, so they can have the same priority.
   //
   static const Initiator::Priority PotsHtlPriority = 50;
   static const Initiator::Priority PotsWmlPriority = 50;
private:
   PotsCollectInformationSap();
   ~PotsCollectInformationSap();
};

class PotsAuthorizeTerminationSap : public BcTrigger
{
   friend class Singleton< PotsAuthorizeTerminationSap >;
public:
   //  SUS has priority over BIC, which has priority over CFU.
   //
   static const Initiator::Priority PotsSusPriority = 50;
   static const Initiator::Priority PotsBicPriority = 45;
   static const Initiator::Priority PotsCfuPriority = 40;
private:
   PotsAuthorizeTerminationSap();
   ~PotsAuthorizeTerminationSap();
};

class PotsLocalBusySap : public BcTrigger
{
   friend class Singleton< PotsLocalBusySap >;
public:
   //  If both CWT and CFB are subscribed, CWT has priority.
   //
   static const Initiator::Priority PotsCwtPriority = 50;
   static const Initiator::Priority PotsCfbPriority = 45;
private:
   PotsLocalBusySap();
   ~PotsLocalBusySap();
};

class PotsLocalAlertingSnp : public BcTrigger
{
   friend class Singleton< PotsLocalAlertingSnp >;
public:
   static const Initiator::Priority PotsCfnPriority = 50;
private:
   PotsLocalAlertingSnp();
   ~PotsLocalAlertingSnp();
};

//------------------------------------------------------------------------------
//
//  POTS basic call SSM.
//
class PotsBcSsm : public ProxyBcSsm
{
public:
   //  Public to allow creation.  MSG is the incoming message, which was
   //  just received by PSM.
   //
   PotsBcSsm(ServiceId sid, const Message& msg, ProtocolSM& psm);

   //  Sets the profile associated with the call.
   //
   void SetProfile(PotsProfile* prof);

   //  Returns the profile associated with the call.
   //
   PotsProfile* Profile() const;

   //  Returns the PSM (UPSM or NPSM) that should be used to run the timer
   //  whose identifier is TID.
   //
   ProtocolSM* TimerPsm(TimerId tid) const;

   //  Starts a timer, identified by TID, for DURATION seconds.
   //
   void StartTimer(TimerId tid, secs_t duration);

   //  Stops the timer identified by TID.
   //
   void StopTimer(TimerId tid);

   //  Clears the timer identified by TID when a timeout message arrives.
   //
   void ClearTimer(TimerId tid);

   //  Acts as a catch-all for message analyzers, analyzing signals that
   //  can arrive in most states.
   //
   EventHandler::Rc AnalyzeMsg(const AnalyzeMsgEvent& ame, Event*& nextEvent);

   //  Sets the treatment to be applied during call takedown.
   //
   void SetTreatment(PotsTreatment* trmt) { trmt_ = trmt; }

   //  Returns the treatment to be applied during call takedown.
   //
   PotsTreatment* GetTreatment() const { return trmt_; }

   //  Clears the call for the reason specified by CAUSE.
   //
   virtual EventHandler::Rc ClearCall(Cause::Ind cause) override;

   //  Overridden to observe the next service alteration point.
   //
   virtual void SetNextSap(TriggerId sap) override;

   //  Overridden to observe the next service notification point.
   //
   virtual void SetNextSnp(TriggerId snp) override;

   //  Overridden to analyze timeout messages that can arrive on the CIP PSM.
   //
   virtual EventHandler::Rc AnalyzeNPsmTimeout
      (const TlvMessage& msg, Event*& nextEvent) override;

   //  Overridden to display member variables.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;
protected:
   //  Protected to restrict deletion.  Virtual to allow subclassing.
   //
   virtual ~PotsBcSsm();

   //  Overridden to handle deletion of the user-side PSM.
   //
   virtual void PsmDeleted(ProtocolSM& exPsm) override;
private:
   //  The subscriber profile associated with the call.
   //
   PotsProfile* prof_;

   //  The identifier (if any) of the timer that is currently running.
   //
   TimerId tid_;

   //  The treatment (if any) that is currently being applied.
   //
   PotsTreatment* trmt_;
};
}
#endif
