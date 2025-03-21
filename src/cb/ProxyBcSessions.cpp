//==============================================================================
//
//  ProxyBcSessions.cpp
//
//  Copyright (C) 2013-2025  Greg Utas
//
//  This file is part of the Robust Services Core (RSC).
//
//  RSC is free software: you can redistribute it and/or modify it under the
//  terms of the Lesser GNU General Public License as published by the Free
//  Software Foundation, either version 3 of the License, or (at your option)
//  any later version.
//
//  RSC is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
//  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
//  details.
//
//  You should have received a copy of the Lesser GNU General Public License
//  along with RSC.  If not, see <http://www.gnu.org/licenses/>.
//
#include "ProxyBcSessions.h"
#include "CliText.h"
#include <ostream>
#include <string>
#include "BcRouting.h"
#include "Debug.h"
#include "FactoryRegistry.h"
#include "GlobalAddress.h"
#include "IpPortRegistry.h"
#include "NwTypes.h"
#include "Registry.h"
#include "SbAppIds.h"
#include "SbEvents.h"
#include "Singleton.h"
#include "SsmContext.h"
#include "SysTypes.h"
#include "Tones.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace CallBase
{
//b Support creating a pair of PPSMs for call join.
//  o allocate remote PPSM directly instead of sending a message that uses join
//    mode (the latter would need a new parameter to identify the target call)
//  o need to put NBC1-PPSM1-PPSM2-NBC2 into correct state for relaying:
//    NPSM1 \ NPSM2  AltRcvd   |     AnmRcvd     |     SusRcvd     |
//    -------+-----------------+-----------------+-----------------+
//    AnmRcvd| AnmRcvd AnmRcvd | AnmRcvd AnmRcvd | AnmRcvd AnmRcvd |
//    AnmSent| AnmRcvd AnmSent | AnmRcvd AnmSent | AnmRcvd SusSent*| * send SUS
//    SusRcvd| AnmRcvd AnmRcvd | AnmRcvd AnmRcvd | AnmRcvd AnmRcvd |
//b Support changing the called DN when broadcasting an IAM.
//b Add a FAC signal and facility parameter to CIP.
//b Support initiating a service using CIP's facility parameter (NPSM or UPSM).
//b Support recreating an SSPM to re-present a call.
//  --includes morphing a proxy SSM back to its base class
//
//  Proxy call use cases (PPSM = proxy UPSM, SPSM = subscriber UPSM)
//  --------------------
//  a) creating n PPSMs [n=1 only; application can iterate]
//  b) SPSM coexisting with PPSM(s) [in CFN variant and (briefly) during
//     CFN, CXR, and CPU]
//  c) broadcasting a message to all PPSMs
//  d) sending a message to a specific PPSM or skipping a PPSM during
//     broadcasting
//  e) analyzing a CIP message received on a PPSM
//  f) applying ringback to the NPSM if a PPSM is first to report alerting
//  g) releasing all but one UPSM (possibly including SPSM) when a PPSM answers
//  h) relaying a CIP message received on the NSPM to PPSM(s)
//     [see (c); done by application]
//  i) releasing all UPSMs, possibly upon a release from the NPSM
//     [see (c); done by application]
//  j) finding the profile associated with a proxy OBC (only one PPSM)
//     [CIP parameter]
//  k) finding the profile associated with a proxy TBC (multiple PPSMs)
//     [CIP parameter]
//  l) joining two calls (CXF, CPU), with each releasing its SPSM (if it
//     exists)
//  m) redirecting a call (CFX), with the optional release of the SPSM
//     if it exists
//  n) transitioning through TBC states [e.g. on CFU in AT state, on CFB
//     in SF state]
//  o) SNP for releasing a modifier when the SPSM is released
//     [e.g. CWT ends when CFN redirects]
//  p) morphing an SSM to its proxy subclass [on redirection only;
//     distribution or origination starts as proxy]
//  q) modifying the called DN each time that an IAM is broadcast
//  r) recreating the SPSM to recall a transferrer (CXR), with the
//     optional release of a PPSM
//  s) morphing a proxy SSM back to its base class [in (r)]
//
fixed_string ProxyBcReleaseUserEventStr = "ProxyBcReleaseUserEvent";
fixed_string ProxyBcProgressEventStr    = "ProxyBcProgressEvent";
fixed_string ProxyBcAnswerEventStr      = "ProxyBcAnswerEvent";
fixed_string ProxyBcReleaseEventStr     = "ProxyBcReleaseEvent";

//------------------------------------------------------------------------------

ProxyBcService::ProxyBcService(Id sid, bool modifiable) :
   BcService(sid, modifiable)
{
   Debug::ft("ProxyBcService.ctor");

   BindHandler(*Singleton<ProxyBcAnalyzeProxyMessage>::Instance(),
      ProxyBcEventHandler::AnalyzeProxyMessage);
   BindHandler(*Singleton<ProxyBcProgressHandler>::Instance(),
      ProxyBcEventHandler::ProxyProgress);
   BindHandler(*Singleton<ProxyBcAnswerHandler>::Instance(),
      ProxyBcEventHandler::ProxyAnswer);
   BindHandler(*Singleton<ProxyBcReleaseHandler>::Instance(),
      ProxyBcEventHandler::ProxyRelease);

   BindEventName(ProxyBcReleaseUserEventStr, ProxyBcEvent::ReleaseUser);
   BindEventName(ProxyBcProgressEventStr, ProxyBcEvent::ProxyProgress);
   BindEventName(ProxyBcAnswerEventStr, ProxyBcEvent::ProxyAnswer);
   BindEventName(ProxyBcReleaseEventStr, ProxyBcEvent::ProxyRelease);
}

//------------------------------------------------------------------------------

ProxyBcService::~ProxyBcService()
{
   Debug::ftnt("ProxyBcService.dtor");
}

//------------------------------------------------------------------------------

fixed_string ProxyPortStr = "Proxy port";

c_string ProxyBcService::PortName(PortId pid) const
{
   if(pid == ProxyPort) return ProxyPortStr;

   return BcService::PortName(pid);
}

//==============================================================================

ProxyBcNull::ProxyBcNull(ServiceId sid) : BcNull(sid) { }

//------------------------------------------------------------------------------

ProxyBcAuthorizingOrigination::ProxyBcAuthorizingOrigination(ServiceId sid) :
   BcAuthorizingOrigination(sid) { }

//------------------------------------------------------------------------------

ProxyBcCollectingInformation::ProxyBcCollectingInformation(ServiceId sid) :
   BcCollectingInformation(sid) { }

//------------------------------------------------------------------------------

ProxyBcAnalyzingInformation::ProxyBcAnalyzingInformation(ServiceId sid) :
   BcAnalyzingInformation(sid) { }

//------------------------------------------------------------------------------

ProxyBcSelectingRoute::ProxyBcSelectingRoute(ServiceId sid) :
   BcSelectingRoute(sid) { }

//------------------------------------------------------------------------------

ProxyBcAuthorizingCallSetup::ProxyBcAuthorizingCallSetup(ServiceId sid) :
   BcAuthorizingCallSetup(sid) { }

//------------------------------------------------------------------------------

ProxyBcSendingCall::ProxyBcSendingCall(ServiceId sid) : BcSendingCall(sid)
{
   BindEventHandler
      (ProxyBcEventHandler::ReleaseUser, ProxyBcEvent::ReleaseUser);
}

//------------------------------------------------------------------------------

ProxyBcOrigAlerting::ProxyBcOrigAlerting(ServiceId sid) : BcOrigAlerting(sid)
{
   BindEventHandler
      (ProxyBcEventHandler::ReleaseUser, ProxyBcEvent::ReleaseUser);
}

//------------------------------------------------------------------------------

ProxyBcAuthorizingTermination::ProxyBcAuthorizingTermination(ServiceId sid) :
   BcAuthorizingTermination(sid) { }

//------------------------------------------------------------------------------

ProxyBcSelectingFacility::ProxyBcSelectingFacility(ServiceId sid) :
   BcSelectingFacility(sid) { }

//------------------------------------------------------------------------------

ProxyBcPresentingCall::ProxyBcPresentingCall(ServiceId sid) :
   BcPresentingCall(sid)
{
   BindMsgAnalyzer
      (ProxyBcEventHandler::AnalyzeProxyMessage, ProxyBcService::ProxyPort);
   BindEventHandler
      (ProxyBcEventHandler::ReleaseUser, ProxyBcEvent::ReleaseUser);
   BindEventHandler
      (ProxyBcEventHandler::ProxyProgress, ProxyBcEvent::ProxyProgress);
   BindEventHandler
      (ProxyBcEventHandler::ProxyAnswer, ProxyBcEvent::ProxyAnswer);
   BindEventHandler
      (ProxyBcEventHandler::ProxyRelease, ProxyBcEvent::ProxyRelease);
}

//------------------------------------------------------------------------------

ProxyBcTermAlerting::ProxyBcTermAlerting(ServiceId sid) : BcTermAlerting(sid)
{
   BindMsgAnalyzer
      (ProxyBcEventHandler::AnalyzeProxyMessage, ProxyBcService::ProxyPort);
   BindEventHandler
      (ProxyBcEventHandler::ReleaseUser, ProxyBcEvent::ReleaseUser);
   BindEventHandler
      (ProxyBcEventHandler::TaLocalAlerting, BcEvent::LocalAlerting);
   BindEventHandler
      (ProxyBcEventHandler::ProxyProgress, ProxyBcEvent::ProxyProgress);
   BindEventHandler
      (ProxyBcEventHandler::ProxyAnswer, ProxyBcEvent::ProxyAnswer);
   BindEventHandler
      (ProxyBcEventHandler::ProxyRelease, ProxyBcEvent::ProxyRelease);
}

//------------------------------------------------------------------------------

ProxyBcActive::ProxyBcActive(ServiceId sid) : BcActive(sid)
{
   BindMsgAnalyzer
      (ProxyBcEventHandler::AnalyzeProxyMessage, ProxyBcService::ProxyPort);
   BindEventHandler
      (ProxyBcEventHandler::ReleaseUser, ProxyBcEvent::ReleaseUser);
   BindEventHandler
      (ProxyBcEventHandler::AcLocalAnswer, BcEvent::LocalAnswer);
   BindEventHandler
      (ProxyBcEventHandler::ProxyProgress, ProxyBcEvent::ProxyProgress);
   BindEventHandler
      (ProxyBcEventHandler::ProxyAnswer, ProxyBcEvent::ProxyAnswer);
   BindEventHandler
      (ProxyBcEventHandler::ProxyRelease, ProxyBcEvent::ProxyRelease);
}

//------------------------------------------------------------------------------

ProxyBcLocalSuspending::ProxyBcLocalSuspending(ServiceId sid) :
   BcLocalSuspending(sid)
{
   BindMsgAnalyzer
      (ProxyBcEventHandler::AnalyzeProxyMessage, ProxyBcService::ProxyPort);
   BindEventHandler
      (ProxyBcEventHandler::ReleaseUser, ProxyBcEvent::ReleaseUser);
   BindEventHandler
      (ProxyBcEventHandler::ProxyProgress, ProxyBcEvent::ProxyProgress);
   BindEventHandler
      (ProxyBcEventHandler::ProxyAnswer, ProxyBcEvent::ProxyAnswer);
   BindEventHandler
      (ProxyBcEventHandler::ProxyRelease, ProxyBcEvent::ProxyRelease);
}

//------------------------------------------------------------------------------

ProxyBcRemoteSuspending::ProxyBcRemoteSuspending(ServiceId sid) :
   BcRemoteSuspending(sid)
{
   BindMsgAnalyzer
      (ProxyBcEventHandler::AnalyzeProxyMessage, ProxyBcService::ProxyPort);
   BindEventHandler
      (ProxyBcEventHandler::ReleaseUser, ProxyBcEvent::ReleaseUser);
   BindEventHandler
      (ProxyBcEventHandler::ProxyProgress, ProxyBcEvent::ProxyProgress);
   BindEventHandler
      (ProxyBcEventHandler::ProxyAnswer, ProxyBcEvent::ProxyAnswer);
   BindEventHandler
      (ProxyBcEventHandler::ProxyRelease, ProxyBcEvent::ProxyRelease);
}

//------------------------------------------------------------------------------

ProxyBcDisconnecting::ProxyBcDisconnecting(ServiceId sid) :
   BcDisconnecting(sid) { }

//------------------------------------------------------------------------------

ProxyBcException::ProxyBcException(ServiceId sid) : BcException(sid)
{
   BindEventHandler
      (ProxyBcEventHandler::ReleaseUser, ProxyBcEvent::ReleaseUser);
}

//==============================================================================

ProxyBcReleaseUserEvent::ProxyBcReleaseUserEvent
   (ServiceSM& owner, Cause::Ind cause) :
   BcReleaseEvent(ProxyBcEvent::ReleaseUser, owner, cause)
{
   Debug::ft("ProxyBcReleaseUserEvent.ctor");
}

ProxyBcReleaseUserEvent::~ProxyBcReleaseUserEvent()
{
   Debug::ftnt("ProxyBcReleaseUserEvent.dtor");
}

//------------------------------------------------------------------------------

ProxyBcProgressEvent::ProxyBcProgressEvent
   (ServiceSM& owner, Progress::Ind progress) :
   BcProgressEvent(ProxyBcEvent::ProxyProgress, owner, progress)
{
   Debug::ft("ProxyBcProgressEvent.ctor");
}

ProxyBcProgressEvent::~ProxyBcProgressEvent()
{
   Debug::ftnt("ProxyBcProgressEvent.dtor");
}

//------------------------------------------------------------------------------

ProxyBcAnswerEvent::ProxyBcAnswerEvent(ServiceSM& owner) :
   BcEvent(ProxyBcEvent::ProxyAnswer, owner)
{
   Debug::ft("ProxyBcAnswerEvent.ctor");
}

ProxyBcAnswerEvent::~ProxyBcAnswerEvent()
{
   Debug::ftnt("ProxyBcAnswerEvent.dtor");
}

//------------------------------------------------------------------------------

ProxyBcReleaseEvent::ProxyBcReleaseEvent(ServiceSM& owner, Cause::Ind cause) :
   BcReleaseEvent(ProxyBcEvent::ProxyRelease, owner, cause)
{
   Debug::ft("ProxyBcReleaseEvent.ctor");
}

ProxyBcReleaseEvent::~ProxyBcReleaseEvent()
{
   Debug::ftnt("ProxyBcReleaseEvent.dtor");
}

//==============================================================================

fn_name ProxyBcAnalyzeProxyMessage_ProcessEvent =
   "ProxyBcAnalyzeProxyMessage.ProcessEvent";

EventHandler::Rc ProxyBcAnalyzeProxyMessage::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(ProxyBcAnalyzeProxyMessage_ProcessEvent);

   auto&         ame = static_cast<AnalyzeMsgEvent&>(currEvent);
   auto          msg = static_cast<CipMessage*>(ame.Msg());
   auto          sid = msg->GetSignal();
   auto&         pssm = static_cast<ProxyBcSsm&>(ssm);
   ProgressInfo* cpi;
   CauseInfo*    cci;

   switch(sid)
   {
   case CipSignal::CPG:
      cpi = msg->FindType<ProgressInfo>(CipParameter::Progress);
      return pssm.RaiseProxyProgress(nextEvent, cpi->progress);

   case CipSignal::ANM:
      return pssm.RaiseProxyAnswer(nextEvent);

   case CipSignal::REL:
      cci = msg->FindType<CauseInfo>(CipParameter::Cause);
      return pssm.RaiseProxyRelease(nextEvent, cci->cause);
   }

   Debug::SwLog(ProxyBcAnalyzeProxyMessage_ProcessEvent, "invalid signal", sid);
   return Suspend;
}

//------------------------------------------------------------------------------

EventHandler::Rc ProxyBcProgressHandler::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("ProxyBcProgressHandler.ProcessEvent");

   auto& ppe = static_cast<ProxyBcProgressEvent&>(currEvent);

   //  If a proxy UPSM reports alerting, see if the NPSM has already sent a
   //  CPG(Alerting).  If it hasn't, apply ringback to the NPSM and relay
   //  the message.  The call remains in the PC state, however, because it
   //  tracks the subscriber's state.
   //
   if(ppe.GetProgress() == Progress::Alerting)
   {
      auto& pssm = static_cast<ProxyBcSsm&>(ssm);
      auto npsm = pssm.NPsm();
      auto state = npsm->GetState();

      switch(state)
      {
      case BcPsm::IamRcvd:
      case BcPsm::EosSent:
         npsm->SetOgTone(Tone::Ringback);
         pssm.Relay(*npsm);
      }
   }

   return Suspend;
}

//------------------------------------------------------------------------------

EventHandler::Rc ProxyBcAnswerHandler::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("ProxyBcAnswerHandler.ProcessEvent");

   //  When a proxy UPSM reports answer, award it the call and release all
   //  other UPSMs.
   //
   auto& pssm = static_cast<ProxyBcSsm&>(ssm);
   auto ppsm = static_cast<ProxyBcPsm*>(Context::ContextPsm());
   auto npsm = pssm.NPsm();

   //  Ensure a media flow between the proxy UPSM that answered and the NPSM.
   //
   ppsm->EnsureMedia(*npsm);

   pssm.Relay(*npsm);

   pssm.ReleaseProxies(ppsm, Cause::CallRedirected);
   pssm.SetNextSnp(ProxyBcTrigger::ProxyAnswerSnp);
   pssm.SetNextState(BcState::Active);
   return pssm.RaiseReleaseUser(nextEvent, Cause::CallRedirected);
}

//------------------------------------------------------------------------------

EventHandler::Rc ProxyBcReleaseHandler::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("ProxyBcReleaseHandler.ProcessEvent");

   //  There is nothing to do.  The proxy UPSM will idle itself.  If it is
   //  the last proxy UPSM, the call will become a non-proxy call with the
   //  only UPSM being the subscriber's.
   //
   return Suspend;
}

//==============================================================================

ProxyBcPsm::ProxyBcPsm() : BcPsm(ProxyCallFactoryId),
   exclude_(false)
{
   Debug::ft("ProxyBcPsm.ctor(first)");
}

//------------------------------------------------------------------------------

ProxyBcPsm::ProxyBcPsm(ProtocolLayer& adj, bool upper) :
   BcPsm(ProxyCallFactoryId, adj, upper),
   exclude_(false)
{
   Debug::ft("ProxyBcPsm.ctor(subseq)");
}

//------------------------------------------------------------------------------

ProxyBcPsm::~ProxyBcPsm()
{
   Debug::ftnt("ProxyBcPsm.dtor");
}

//------------------------------------------------------------------------------

void ProxyBcPsm::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   MediaPsm::Display(stream, prefix, options);

   stream << prefix << "exclude : " << exclude_ << CRLF;
}

//------------------------------------------------------------------------------

ProtocolSM::OutgoingRc ProxyBcPsm::ProcessOgMsg(Message& msg)
{
   Debug::ft("ProxyBcPsm.ProcessOgMsg");

   //  Send all proxy messages with immediate priority.
   //
   msg.SetPriority(IMMEDIATE);

   //  If this PSM is not part of a broadcast, simply allow our base class to
   //  handle the message.
   //
   if(exclude_)
   {
      return BcPsm::ProcessOgMsg(msg);
   }

   //  This message is being broadcast.  Save it so that it will not be
   //  deleted when it is sent.  Invoke our base class to update our state
   //  and then send the message manually.  If this is an initial message,
   //  it must provide the source and destination addresses.
   //
   if(AddressesUnknown(&msg))
   {
      const auto& self = IpPortRegistry::LocalAddr();
      GlobalAddress addr(self, NilIpPort, ProxyCallFactoryId);

      msg.SetSender(addr);
      msg.SetReceiver(addr);
   }

   msg.Save();
   BcPsm::ProcessOgMsg(msg);
   SendToLower(msg);

   //  Find the next PSM in the broadcast group and move the message to it.
   //  Unsave the message so that it will be deleted (unless someone else
   //  saved it) after it has been broadcast to all proxy PSMs.  We handle
   //  the message entirely, so tell the PSM not to process it.
   //
   auto pssm = static_cast<ProxyBcSsm*>(RootSsm());
   auto ppsm = this;
   pssm->NextBroadcast(ppsm);
   if(ppsm != nullptr) msg.Retrieve(ppsm);
   msg.Unsave();
   return SkipMessage;
}

//------------------------------------------------------------------------------

Message::Route ProxyBcPsm::Route() const
{
   Debug::ft("ProxyBcPsm.Route");

   return Message::Internal;
}

//==============================================================================

ProxyBcSsm::ProxyBcSsm(ServiceId sid) : BcSsm(sid),
   proxyCount_(0)
{
   Debug::ft("ProxyBcSsm.ctor");
}

//------------------------------------------------------------------------------

ProxyBcSsm::~ProxyBcSsm()
{
   Debug::ftnt("ProxyBcSsm.dtor");
}

//------------------------------------------------------------------------------

ProxyBcPsm* ProxyBcSsm::AllocOgProxy()
{
   Debug::ft("ProxyBcSsm.AllocOgProxy");

   auto ppsm = new ProxyBcPsm;

   ++proxyCount_;

   auto upsm = UPsm();

   if((upsm == nullptr) || (upsm->GetFactory() == ProxyCallFactoryId))
   {
      //  A new PSM is henq'd on the SSM's PSM queue.  During broadcasting, we
      //  want to queue the outgoing message on the first PSM and then cascade
      //  it through the other proxy PSMs.  The UPSM therefore needs to be the
      //  first proxy PSM in the SSM's PSM queue.
      //
      SetUPsm(*ppsm);
   }

   return ppsm;
}

//------------------------------------------------------------------------------

ServicePortId ProxyBcSsm::CalcPort(const AnalyzeMsgEvent& ame)
{
   Debug::ft("ProxyBcSsm.CalcPort");

   auto psm = ame.Msg()->Psm();

   //  If this is a proxy PSM, return ProxyPort if the UPSM is *not* a proxy
   //  PSM.  This distinguishes proxy PSMs from the UPSM.  Otherwise, *all*
   //  UPSMs are proxy PSMs, so return UserPort.
   //
   if(psm->GetFactory() == ProxyCallFactoryId)
   {
      auto upsm = UPsm();

      if(upsm->GetFactory() != ProxyCallFactoryId)
         return ProxyBcService::ProxyPort;
      else
         return Service::UserPort;
   }

   return BcSsm::CalcPort(ame);
}

//------------------------------------------------------------------------------

void ProxyBcSsm::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   BcSsm::Display(stream, prefix, options);

   stream << prefix << "proxyCount : " << proxyCount_ << CRLF;
}

//------------------------------------------------------------------------------

void ProxyBcSsm::EndOfTransaction()
{
   Debug::ft("ProxyBcSsm.EndOfTransaction");

   //  Before invoking the base class, look for proxy PSMs that are excluded
   //  from the broadcast group and reinclude them.
   //
   for(auto p = FirstProxy(); p != nullptr; NextProxy(p))
   {
      if(p->IsExcluded())
      {
         p->SetExclude(false);
      }
   }

   BcSsm::EndOfTransaction();
}

//------------------------------------------------------------------------------

ProxyBcPsm* ProxyBcSsm::FirstBroadcast() const
{
   Debug::ft("ProxyBcSsm.FirstBroadcast");

   for(auto p = FirstProxy(); p != nullptr; NextProxy(p))
   {
      if(!p->IsExcluded())
      {
         return p;
      }
   }

   return nullptr;
}

//------------------------------------------------------------------------------

ProxyBcPsm* ProxyBcSsm::FirstProxy() const
{
   Debug::ft("ProxyBcSsm.FirstProxy");

   auto ctx = GetContext();

   for(auto p = ctx->FirstPsm(); p != nullptr; ctx->NextPsm(p))
   {
      if(p->GetFactory() == ProxyCallFactoryId)
      {
         return static_cast<ProxyBcPsm*>(p);
      }
   }

   return nullptr;
}

//------------------------------------------------------------------------------

void ProxyBcSsm::NextBroadcast(ProxyBcPsm*& ppsm) const
{
   Debug::ft("ProxyBcSsm.NextBroadcast");

   for(NextProxy(ppsm); ppsm != nullptr; NextProxy(ppsm))
   {
      if(!ppsm->IsExcluded()) return;
   }
}

//------------------------------------------------------------------------------

void ProxyBcSsm::NextProxy(ProxyBcPsm*& ppsm) const
{
   Debug::ft("ProxyBcSsm.NextProxy");

   auto ctx = GetContext();
   ProtocolSM* psm = ppsm;

   for(ctx->NextPsm(psm); psm != nullptr; ctx->NextPsm(psm))
   {
      if(psm->GetFactory() == ProxyCallFactoryId)
      {
         ppsm = static_cast<ProxyBcPsm*>(psm);
         return;
      }
   }

   ppsm = nullptr;
}

//------------------------------------------------------------------------------

void ProxyBcSsm::PsmDeleted(const ProtocolSM& exPsm)
{
   Debug::ft("ProxyBcSsm.PsmDeleted");

   //  Track the number of proxy UPSMs.
   //
   if(exPsm.GetFactory() == ProxyCallFactoryId)
   {
      --proxyCount_;
   }

   BcSsm::PsmDeleted(exPsm);

   if((proxyCount_ > 0) && (UPsm() == nullptr))
   {
      SetUPsm(*FirstProxy());
   }
}

//------------------------------------------------------------------------------

EventHandler::Rc ProxyBcSsm::RaiseProxyAnswer(Event*& nextEvent)
{
   Debug::ft("ProxyBcSsm.RaiseProxyAnswer");

   SetNextSap(ProxyBcTrigger::ProxyAnswerSap);
   nextEvent = new ProxyBcAnswerEvent(*this);
   return EventHandler::Continue;
}

//------------------------------------------------------------------------------

EventHandler::Rc ProxyBcSsm::RaiseProxyProgress
   (Event*& nextEvent, Progress::Ind progress)
{
   Debug::ft("ProxyBcSsm.RaiseProxyProgress");

   nextEvent = new ProxyBcProgressEvent(*this, progress);
   return EventHandler::Continue;
}

//------------------------------------------------------------------------------

EventHandler::Rc ProxyBcSsm::RaiseProxyRelease
   (Event*& nextEvent, Cause::Ind cause)
{
   Debug::ft("ProxyBcSsm.RaiseProxyRelease");

   nextEvent = new ProxyBcReleaseEvent(*this, cause);
   return EventHandler::Continue;
}

//------------------------------------------------------------------------------

EventHandler::Rc ProxyBcSsm::RaiseReleaseUser
   (Event*& nextEvent, Cause::Ind cause)
{
   Debug::ft("ProxyBcSsm.RaiseReleaseUser");

   nextEvent = new ProxyBcReleaseUserEvent(*this, cause);
   return EventHandler::Continue;
}

//------------------------------------------------------------------------------

fn_name ProxyBcSsm_Relay = "ProxyBcSsm.Relay";

void ProxyBcSsm::Relay(BcPsm& target) const
{
   Debug::ft(ProxyBcSsm_Relay);

   if(&target == nullptr)
   {
      Debug::SwLog(ProxyBcSsm_Relay, "null target PSM", 0);
      return;
   }

   auto msg = Context::ContextMsg();

   if(msg == nullptr)
   {
      Debug::SwLog(ProxyBcSsm_Relay, "message not found", 1);
      return;
   }

   auto prid = msg->GetProtocol();

   if(prid != CipProtocolId)
   {
      Debug::SwLog(ProxyBcSsm_Relay, "invalid protocol", prid);
      return;
   }

   if(&target == NPsm()) msg->SetPriority(PROGRESS);
   if(msg->Relay(target)) return;

   Context::Kill("failed to relay message", 0);
}

//------------------------------------------------------------------------------

void ProxyBcSsm::ReleaseProxies(ProxyBcPsm* skip, Cause::Ind cause) const
{
   Debug::ft("ProxyBcSsm.ReleaseProxies");

   //  If one of the proxy PSMs is to be skipped, excluded it when
   //  the REL is broadcast.
   //
   if(skip != nullptr) skip->SetExclude(true);

   //  Find the first proxy UPSM that is part of the broadcast group.
   //  Construct a CIP REL for CAUSE and queue it on that PSM.
   //
   auto ppsm = FirstBroadcast();
   if(ppsm == nullptr) return;

   auto msg = new CipMessage(ppsm, 16);

   CauseInfo cci;

   msg->SetSignal(CipSignal::REL);
   cci.cause = cause;
   msg->AddCause(cci);
}

//------------------------------------------------------------------------------

void ProxyBcSsm::SetUPsm(MediaPsm& psm)
{
   Debug::ft("ProxyBcSsm.SetUPsm");

   if((proxyCount_ == 0) && (psm.GetFactory() == ProxyCallFactoryId))
   {
      proxyCount_ = 1;
   }

   BcSsm::SetUPsm(psm);
}

//==============================================================================

class ProxyBcFactoryText : public CliText
{
public: ProxyBcFactoryText();
};

fixed_string ProxyBcFactoryStr = "PX";
fixed_string ProxyBcFactoryExpl = "Proxy Call (user side)";

ProxyBcFactoryText::ProxyBcFactoryText() :
   CliText(ProxyBcFactoryExpl, ProxyBcFactoryStr) { }

//------------------------------------------------------------------------------

ProxyBcFactory::ProxyBcFactory() :
   CipFactory(ProxyCallFactoryId, "Proxy Calls")
{
   Debug::ft("ProxyBcFactory.ctor");

   AddIncomingSignal(Signal::Timeout);
   AddIncomingSignal(CipSignal::IAM);
   AddIncomingSignal(CipSignal::CPG);
   AddIncomingSignal(CipSignal::ANM);
   AddIncomingSignal(CipSignal::REL);

   AddOutgoingSignal(CipSignal::IAM);
   AddOutgoingSignal(CipSignal::CPG);
   AddOutgoingSignal(CipSignal::ANM);
   AddOutgoingSignal(CipSignal::REL);
}

//------------------------------------------------------------------------------

ProxyBcFactory::~ProxyBcFactory()
{
   Debug::ftnt("ProxyBcFactory.dtor");
}

//------------------------------------------------------------------------------

ProtocolSM* ProxyBcFactory::AllocIcPsm
   (const Message& msg, ProtocolLayer& lower) const
{
   Debug::ft("ProxyBcFactory.AllocIcPsm");

   return new ProxyBcPsm(lower, false);
}

//------------------------------------------------------------------------------

RootServiceSM* ProxyBcFactory::AllocRoot
   (const Message& msg, ProtocolSM& psm) const
{
   Debug::ft("ProxyBcFactory.AllocRoot");

   const auto& tmsg = static_cast<const CipMessage&>(msg);
   auto rte = tmsg.FindType<RouteResult>(CipParameter::Route);
   if(rte == nullptr) return nullptr;

   auto reg = Singleton<FactoryRegistry>::Instance();
   auto fac = static_cast<SsmFactory*>(reg->Factories().At(rte->selector));
   return fac->AllocRoot(msg, psm);
}

//------------------------------------------------------------------------------

CliText* ProxyBcFactory::CreateText() const
{
   Debug::ft("ProxyBcFactory.CreateText");

   return new ProxyBcFactoryText;
}
}
