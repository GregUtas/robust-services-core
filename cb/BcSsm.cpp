//==============================================================================
//
//  BcSsm.cpp
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
#include "BcSessions.h"
#include <iomanip>
#include <ostream>
#include "Algorithms.h"
#include "BcProtocol.h"
#include "Debug.h"
#include "FactoryRegistry.h"
#include "Formatters.h"
#include "SbEvents.h"
#include "ServiceCodeRegistry.h"
#include "Singleton.h"
#include "Tones.h"

using std::ostream;
using std::setw;
using std::string;

//------------------------------------------------------------------------------

namespace CallBase
{
int BcSsm::StateCount_[] = { 0 };

//------------------------------------------------------------------------------

fn_name BcSsm_ctor = "BcSsm.ctor";

BcSsm::BcSsm(ServiceId sid) : MediaSsm(sid),
   model_(XbcModel),
   uPsm_(nullptr),
   nPsm_(nullptr)
{
   Debug::ft(BcSsm_ctor);

   StateCount_[BcState::Null]++;
}

//------------------------------------------------------------------------------

fn_name BcSsm_dtor = "BcSsm.dtor";

BcSsm::~BcSsm()
{
   Debug::ftnt(BcSsm_dtor);

   StateCount_[CurrState()]--;
}

//------------------------------------------------------------------------------

fn_name BcSsm_AllocNPsm = "BcSsm.AllocNPsm";

CipPsm* BcSsm::AllocNPsm()
{
   Debug::ft(BcSsm_AllocNPsm);

   if(nPsm_ != nullptr)
   {
      Debug::SwLog(BcSsm_AllocNPsm, "PSM already exists", Sid());
      return nPsm_;
   }

   nPsm_ = new CipPsm;
   return nPsm_;
}

//------------------------------------------------------------------------------

fn_name BcSsm_AnalyzeInformation = "BcSsm.AnalyzeInformation";

EventHandler::Rc BcSsm::AnalyzeInformation(Event*& nextEvent)
{
   Debug::ft(BcSsm_AnalyzeInformation);

   //  The next event depends on what digits have been dialed.
   //
   analysis_ = AnalysisResult(dialed_);

   switch(analysis_.selector)
   {
   case Address::DnType:
      return RaiseSelectRoute(nextEvent);
   case Address::ScType:
      return RequestService(nextEvent);
   }

   return RaiseInvalidInformation(nextEvent);
}

//------------------------------------------------------------------------------

fn_name BcSsm_AnalyzeNPsmTimeout = "BcSsm.AnalyzeNPsmTimeout";

EventHandler::Rc BcSsm::AnalyzeNPsmTimeout
   (const TlvMessage& msg, Event*& nextEvent)
{
   Debug::ft(BcSsm_AnalyzeNPsmTimeout);

   Debug::SwLog(BcSsm_AnalyzeNPsmTimeout, strOver(this), Sid());
   return RaiseReleaseCall(nextEvent, Cause::MessageInvalidForState);
}

//------------------------------------------------------------------------------

fn_name BcSsm_BuildCipCpg = "BcSsm.BuildCipCpg";

CipMessage* BcSsm::BuildCipCpg(Progress::Ind progress)
{
   Debug::ft(BcSsm_BuildCipCpg);

   if(nPsm_ == nullptr)
   {
      Debug::SwLog(BcSsm_BuildCipCpg, "null nPSM", Sid());
      return nullptr;
   }

   auto msg = new CipMessage(nPsm_, 16);
   ProgressInfo cpi;

   msg->SetSignal(CipSignal::CPG);
   cpi.progress = progress;
   msg->AddProgress(cpi);

   return msg;
}

//------------------------------------------------------------------------------

fn_name BcSsm_BuildCipIam = "BcSsm.BuildCipIam";

CipMessage* BcSsm::BuildCipIam()
{
   Debug::ft(BcSsm_BuildCipIam);

   //  Build the CIP IAM and enable two-way media.
   //
   if(uPsm_ == nullptr)
   {
      Debug::SwLog(BcSsm_BuildCipIam, "null uPSM", Sid());
      return nullptr;
   }

   //  In a distributed system, a query to a central database (name server) is
   //  often needed to find HOST (the destination's IP address).  For example:
   //  o querying a DNS server with a VoIP call's destination URL
   //  o querying a toll-free database with an 800 number
   //  o querying an HLR with a mobile subscriber's number
   //  o querying an internal server to find the service node that is currently
   //    managing the subscriber who is the intended recipient of this session
   //
   //  Such queries are sent in the Analyzing Info state after allocating a PSM
   //  that supports the database query protocol.  If many applications perform
   //  the query, a modifier SSM would manage this PSM.
   //
   if(nPsm_ == nullptr)
   {
      if(AllocNPsm() == nullptr) return nullptr;
   }

   auto iam = new CipMessage(nPsm_, 44);
   iam->SetSignal(CipSignal::IAM);
   iam->SetPriority(EGRESS);
   iam->AddRoute(route_);
   iam->AddAddress(DialedDigits().ToDN(), CipParameter::Called);

   uPsm_->CreateMedia(*nPsm_);

   SetNextSnp(BcTrigger::SendCallSnp);
   return iam;
}

//------------------------------------------------------------------------------

fn_name BcSsm_BuildCipRel = "BcSsm.BuildCipRel";

CipMessage* BcSsm::BuildCipRel(Cause::Ind cause)
{
   Debug::ft(BcSsm_BuildCipRel);

   //  Send a CIP REL and disable media.
   //
   if(nPsm_ == nullptr)
   {
      Debug::SwLog(BcSsm_BuildCipRel, "null nPSM", Sid());
      return nullptr;
   }

   auto msg = new CipMessage(nPsm_, 16);
   CauseInfo cci;

   msg->SetSignal(CipSignal::REL);
   cci.cause = cause;
   msg->AddCause(cci);

   nPsm_->DisableMedia();
   return msg;
}

//------------------------------------------------------------------------------

fn_name BcSsm_CalcPort = "BcSsm.CalcPort";

ServicePortId BcSsm::CalcPort(const AnalyzeMsgEvent& ame)
{
   Debug::ft(BcSsm_CalcPort);

   auto psm = ame.Msg()->Psm();

   if(uPsm_ == psm) return Service::UserPort;
   if(nPsm_ == psm) return Service::NetworkPort;
   return NIL_ID;
}

//------------------------------------------------------------------------------

fn_name BcSsm_ClearCall = "BcSsm.ClearCall";

EventHandler::Rc BcSsm::ClearCall(Cause::Ind cause)
{
   Debug::ft(BcSsm_ClearCall);

   if((nPsm_ != nullptr) && (nPsm_->GetState() != ProtocolSM::Idle))
   {
      BuildCipRel(cause);
   }

   //  Don't overwrite the LocalReleaseSnp.
   //
   if(cause != Cause::NormalCallClearing)
   {
      SetNextSnp(BcTrigger::CallClearedSnp);
   }

   SetNextState(BcState::Null);
   return EventHandler::Suspend;
}

//------------------------------------------------------------------------------

void BcSsm::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   MediaSsm::Display(stream, prefix, options);

   stream << prefix << "model : " << int(model_) << CRLF;
   stream << prefix << "upsm  : " << uPsm_ << CRLF;
   stream << prefix << "npsm  : " << nPsm_ << CRLF;
   stream << prefix << "dialed : " << CRLF;
   dialed_.Display(stream, prefix + spaces(2));
   stream << prefix << "analysis : " << CRLF;
   analysis_.Display(stream, prefix + spaces(2));
   stream << prefix << "route : " << CRLF;
   route_.Display(stream, prefix + spaces(2));
}

//------------------------------------------------------------------------------

fixed_string ObcStateHeader =
   "        AO   CI   AI   SR   AS   SC   OA        RS";          // row 1
//     Nu                                      Ac        Di   Ex  // row 2
fixed_string TbcStateHeader =
        "   AT             SF        PC   TA";  //  LS            // row 3
// 0         1         2         3         4         5         6
// 0123456789012345678901234567890123456789012345678901234567890

void BcSsm::DisplayStateCounts(ostream& stream, const string& prefix)
{
   stream << prefix << ObcStateHeader << CRLF;

   stream << prefix;
   stream << setw(5) << "Nu";
   for(auto s = BcState::AuthorizingOrigination; s <= BcState::OrigAlerting; ++s)
      stream << setw(5) << StateCount_[s];
   stream << setw(5) << "Ac";
   stream << setw(5) << StateCount_[BcState::RemoteSuspending];
   stream << setw(5) << "Di";
   stream << setw(5) << "Ex";
   stream << CRLF;

   stream << prefix;
   stream << setw(5) << StateCount_[BcState::Null];
   stream << TbcStateHeader;
   stream << setw(5) << StateCount_[BcState::Active];
   stream << setw(5) << "LS";
   stream << setw(5) << StateCount_[BcState::Disconnecting];
   stream << setw(5) << StateCount_[BcState::Exception];
   stream << CRLF;

   stream << prefix;
   stream << setw(10) << StateCount_[BcState::AuthorizingTermination];
   stream << setw(15) << StateCount_[BcState::SelectingFacility];
   stream << setw(10) << StateCount_[BcState::PresentingCall];
   stream << setw(5) << StateCount_[BcState::TermAlerting];
   stream << setw(10) << StateCount_[BcState::LocalSuspending];
   stream << CRLF;
}

//------------------------------------------------------------------------------

fn_name BcSsm_HandleLocalAlerting = "BcSsm.HandleLocalAlerting";

EventHandler::Rc BcSsm::HandleLocalAlerting()
{
   Debug::ft(BcSsm_HandleLocalAlerting);

   //  Send a CIP CPG(Alerting) and provide ringback.
   //
   if(nPsm_ == nullptr)
   {
      Debug::SwLog(BcSsm_HandleLocalAlerting, "null nPSM", Sid());
      return EventHandler::Suspend;
   }

   auto msg = new CipMessage(nPsm_, 16);
   ProgressInfo cpi;

   msg->SetSignal(CipSignal::CPG);
   cpi.progress = Progress::Alerting;
   msg->AddProgress(cpi);

   if(uPsm_ == nullptr)
   {
      Debug::SwLog(BcSsm_HandleLocalAlerting, "null uPSM", Sid());
      return EventHandler::Suspend;
   }

   uPsm_->SetIcTone(Tone::Ringback);

   SetNextSnp(BcTrigger::LocalAlertingSnp);
   SetNextState(BcState::TermAlerting);
   return EventHandler::Suspend;
}

//------------------------------------------------------------------------------

fn_name BcSsm_HandleLocalAnswer = "BcSsm.HandleLocalAnswer";

EventHandler::Rc BcSsm::HandleLocalAnswer()
{
   Debug::ft(BcSsm_HandleLocalAnswer);

   //  Send a CIP ANM and enable two-way media.
   //
   if(nPsm_ == nullptr)
   {
      Debug::SwLog(BcSsm_HandleLocalAnswer, "null nPSM", Sid());
      return EventHandler::Suspend;
   }

   auto msg = new CipMessage(nPsm_, 16);
   msg->SetSignal(CipSignal::ANM);

   if(uPsm_ == nullptr)
   {
      Debug::SwLog(BcSsm_HandleLocalAnswer, "null uPSM", Sid());
      return EventHandler::Suspend;
   }

   uPsm_->EnableMedia();
   SetNextSnp(BcTrigger::LocalAnswerSnp);
   SetNextState(BcState::Active);
   return EventHandler::Suspend;
}

//------------------------------------------------------------------------------

fn_name BcSsm_HandleRemoteRelease = "BcSsm.HandleRemoteRelease";

EventHandler::Rc BcSsm::HandleRemoteRelease(Event& currEvent)
{
   Debug::ft(BcSsm_HandleRemoteRelease);

   auto& rre = static_cast< BcRemoteReleaseEvent& >(currEvent);
   Cause::Ind cause = rre.GetCause();

   return ClearCall(cause);
}

//------------------------------------------------------------------------------

fn_name BcSsm_PsmDeleted = "BcSsm.PsmDeleted";

void BcSsm::PsmDeleted(ProtocolSM& exPsm)
{
   Debug::ft(BcSsm_PsmDeleted);

   if(uPsm_ == &exPsm)
      uPsm_ = nullptr;
   else if(nPsm_ == &exPsm)
      nPsm_ = nullptr;

   MediaSsm::PsmDeleted(exPsm);
}

//------------------------------------------------------------------------------

fn_name BcSsm_RaiseAnalyzeInformation = "BcSsm.RaiseAnalyzeInformation";

EventHandler::Rc BcSsm::RaiseAnalyzeInformation(Event*& nextEvent)
{
   Debug::ft(BcSsm_RaiseAnalyzeInformation);

   SetNextSnp(BcTrigger::InformationCollectedSnp);
   SetNextState(BcState::AnalyzingInformation);
   SetNextSap(BcTrigger::AnalyzeInformationSap);
   nextEvent = new BcAnalyzeInformationEvent(*this);
   return EventHandler::Continue;
}

//------------------------------------------------------------------------------

fn_name BcSsm_RaiseApplyTreatment = "BcSsm.RaiseApplyTreatment";

EventHandler::Rc BcSsm::RaiseApplyTreatment
   (Event*& nextEvent, Cause::Ind cause)
{
   Debug::ft(BcSsm_RaiseApplyTreatment);

   SetNextState(BcState::Exception);
   SetNextSap(BcTrigger::ApplyTreatmentSap);
   nextEvent = new BcApplyTreatmentEvent(*this, cause);
   return EventHandler::Continue;
}

//------------------------------------------------------------------------------

fn_name BcSsm_RaiseAuthorizeCallSetup = "BcSsm.RaiseAuthorizeCallSetup";

EventHandler::Rc BcSsm::RaiseAuthorizeCallSetup(Event*& nextEvent)
{
   Debug::ft(BcSsm_RaiseAuthorizeCallSetup);

   SetNextSnp(BcTrigger::RouteSelectedSnp);
   SetNextState(BcState::AuthorizingCallSetup);
   SetNextSap(BcTrigger::AuthorizeCallSetupSap);
   nextEvent = new BcAuthorizeCallSetupEvent(*this);
   return EventHandler::Continue;
}

//------------------------------------------------------------------------------

fn_name BcSsm_RaiseAuthorizeOrigination = "BcSsm.RaiseAuthorizeOrigination";

EventHandler::Rc BcSsm::RaiseAuthorizeOrigination(Event*& nextEvent)
{
   Debug::ft(BcSsm_RaiseAuthorizeOrigination);

   SetNextSnp(BcTrigger::OriginateSnp);
   SetNextState(BcState::AuthorizingOrigination);
   SetNextSap(BcTrigger::AuthorizeOriginationSap);
   nextEvent = new BcAuthorizeOriginationEvent(*this);
   return EventHandler::Continue;
}

//------------------------------------------------------------------------------

fn_name BcSsm_RaiseAuthorizeTermination = "BcSsm.RaiseAuthorizeTermination";

EventHandler::Rc BcSsm::RaiseAuthorizeTermination(Event*& nextEvent)
{
   Debug::ft(BcSsm_RaiseAuthorizeTermination);

   SetNextSnp(BcTrigger::TerminateSnp);
   SetNextState(BcState::AuthorizingTermination);
   SetNextSap(BcTrigger::AuthorizeTerminationSap);
   nextEvent = new BcAuthorizeTerminationEvent(*this);
   return EventHandler::Continue;
}

//------------------------------------------------------------------------------

fn_name BcSsm_RaiseCollectInformation = "BcSsm.RaiseCollectInformation";

EventHandler::Rc BcSsm::RaiseCollectInformation(Event*& nextEvent)
{
   Debug::ft(BcSsm_RaiseCollectInformation);

   SetNextSnp(BcTrigger::OriginatedSnp);
   SetNextState(BcState::CollectingInformation);
   SetNextSap(BcTrigger::CollectInformationSap);
   nextEvent = new BcCollectInformationEvent(*this);
   return EventHandler::Continue;
}

//------------------------------------------------------------------------------

fn_name BcSsm_RaiseCollectionTimeout = "BcSsm.RaiseCollectionTimeout";

EventHandler::Rc BcSsm::RaiseCollectionTimeout(Event*& nextEvent)
{
   Debug::ft(BcSsm_RaiseCollectionTimeout);

   SetNextSap(BcTrigger::CollectionTimeoutSap);
   nextEvent = new BcCollectionTimeoutEvent(*this, Cause::AddressTimeout);
   return EventHandler::Continue;
}

//------------------------------------------------------------------------------

fn_name BcSsm_RaiseFacilityFailure = "BcSsm.RaiseFacilityFailure";

EventHandler::Rc BcSsm::RaiseFacilityFailure(Event*& nextEvent)
{
   Debug::ft(BcSsm_RaiseFacilityFailure);

   SetNextSap(BcTrigger::FacilityFailureSap);
   nextEvent = new BcFacilityFailureEvent(*this, Cause::AlertingTimeout);
   return EventHandler::Continue;
}

//------------------------------------------------------------------------------

fn_name BcSsm_RaiseInvalidInformation = "BcSsm.RaiseInvalidInformation";

EventHandler::Rc BcSsm::RaiseInvalidInformation(Event*& nextEvent)
{
   Debug::ft(BcSsm_RaiseInvalidInformation);

   SetNextSap(BcTrigger::InvalidInformationSap);
   nextEvent = new BcInvalidInformationEvent(*this, Cause::InvalidAddress);
   return EventHandler::Continue;
}

//------------------------------------------------------------------------------

fn_name BcSsm_RaiseLocalAlerting = "BcSsm.RaiseLocalAlerting";

EventHandler::Rc BcSsm::RaiseLocalAlerting(Event*& nextEvent)
{
   Debug::ft(BcSsm_RaiseLocalAlerting);

   nextEvent = new BcLocalAlertingEvent(*this);
   return EventHandler::Continue;
}

//------------------------------------------------------------------------------

fn_name BcSsm_RaiseLocalAnswer = "BcSsm.RaiseLocalAnswer";

EventHandler::Rc BcSsm::RaiseLocalAnswer(Event*& nextEvent)
{
   Debug::ft(BcSsm_RaiseLocalAnswer);

   SetNextSap(BcTrigger::LocalAnswerSap);
   nextEvent = new BcLocalAnswerEvent(*this);
   return EventHandler::Continue;
}

//------------------------------------------------------------------------------

fn_name BcSsm_RaiseLocalBusy = "BcSsm.RaiseLocalBusy";

EventHandler::Rc BcSsm::RaiseLocalBusy(Event*& nextEvent)
{
   Debug::ft(BcSsm_RaiseLocalBusy);

   SetNextSap(BcTrigger::LocalBusySap);
   nextEvent = new BcLocalBusyEvent(*this, Cause::UserBusy);
   return EventHandler::Continue;
}

//------------------------------------------------------------------------------

fn_name BcSsm_RaiseLocalInformation = "BcSsm.RaiseLocalInformation";

EventHandler::Rc BcSsm::RaiseLocalInformation(Event*& nextEvent)
{
   Debug::ft(BcSsm_RaiseLocalInformation);

   SetNextSap(BcTrigger::LocalInformationSap);
   nextEvent = new BcLocalInformationEvent(*this);
   return EventHandler::Continue;
}

//------------------------------------------------------------------------------

fn_name BcSsm_RaiseLocalNoAnswer = "BcSsm.RaiseLocalNoAnswer";

EventHandler::Rc BcSsm::RaiseLocalNoAnswer(Event*& nextEvent)
{
   Debug::ft(BcSsm_RaiseLocalNoAnswer);

   SetNextSap(BcTrigger::LocalNoAnswerSap);
   nextEvent = new BcLocalNoAnswerEvent(*this, Cause::AnswerTimeout);
   return EventHandler::Continue;
}

//------------------------------------------------------------------------------

fn_name BcSsm_RaiseLocalProgress = "BcSsm.RaiseLocalProgress";

EventHandler::Rc BcSsm::RaiseLocalProgress
   (Event*& nextEvent, Progress::Ind progress)
{
   Debug::ft(BcSsm_RaiseLocalProgress);

   nextEvent = new BcLocalProgressEvent(*this, progress);
   return EventHandler::Continue;
}

//------------------------------------------------------------------------------

fn_name BcSsm_RaiseLocalRelease = "BcSsm.RaiseLocalRelease";

EventHandler::Rc BcSsm::RaiseLocalRelease
   (Event*& nextEvent, Cause::Ind cause)
{
   Debug::ft(BcSsm_RaiseLocalRelease);

   SetNextSap(BcTrigger::LocalReleaseSap);
   nextEvent = new BcLocalReleaseEvent(*this, cause);
   return EventHandler::Continue;
}

//------------------------------------------------------------------------------

fn_name BcSsm_RaiseLocalResume = "BcSsm.RaiseLocalResume";

EventHandler::Rc BcSsm::RaiseLocalResume(Event*& nextEvent)
{
   Debug::ft(BcSsm_RaiseLocalResume);

   nextEvent = new BcLocalResumeEvent(*this);
   return EventHandler::Continue;
}

//------------------------------------------------------------------------------

fn_name BcSsm_RaiseLocalSuspend = "BcSsm.RaiseLocalSuspend";

EventHandler::Rc BcSsm::RaiseLocalSuspend(Event*& nextEvent)
{
   Debug::ft(BcSsm_RaiseLocalSuspend);

   nextEvent = new BcLocalSuspendEvent(*this);
   return EventHandler::Continue;
}

//------------------------------------------------------------------------------

fn_name BcSsm_RaisePresentCall = "BcSsm.RaisePresentCall";

EventHandler::Rc BcSsm::RaisePresentCall(Event*& nextEvent)
{
   Debug::ft(BcSsm_RaisePresentCall);

   SetNextSnp(BcTrigger::FacilitySelectedSnp);
   SetNextState(BcState::PresentingCall);
   SetNextSap(BcTrigger::PresentCallSap);
   nextEvent = new BcPresentCallEvent(*this);
   return EventHandler::Continue;
}

//------------------------------------------------------------------------------

fn_name BcSsm_RaiseProtocolError = "BcSsm.RaiseProtocolError";

Event* BcSsm::RaiseProtocolError(ProtocolSM& psm, ProtocolSM::Error err)
{
   Debug::ft(BcSsm_RaiseProtocolError);

   switch(err)
   {
   case ProtocolSM::SignalInvalid:
      return new BcReleaseCallEvent(*this, Cause::MessageInvalidForState);
   case ProtocolSM::ParameterAbsent:
      return new BcReleaseCallEvent(*this, Cause::ParameterAbsent);
   case ProtocolSM::Timeout:
      return new BcReleaseCallEvent(*this, Cause::ProtocolTimeout);
   }

   return new BcReleaseCallEvent(*this, Cause::TemporaryFailure);
}

//------------------------------------------------------------------------------

fn_name BcSsm_RaiseReleaseCall = "BcSsm.RaiseReleaseCall";

EventHandler::Rc BcSsm::RaiseReleaseCall
   (Event*& nextEvent, Cause::Ind cause)
{
   Debug::ft(BcSsm_RaiseReleaseCall);

   SetNextSap(BcTrigger::ReleaseCallSap);
   nextEvent = new BcReleaseCallEvent(*this, cause);
   return EventHandler::Continue;
}

//------------------------------------------------------------------------------

fn_name BcSsm_RaiseRemoteAlerting = "BcSsm.RaiseRemoteAlerting";

EventHandler::Rc BcSsm::RaiseRemoteAlerting(Event*& nextEvent)
{
   Debug::ft(BcSsm_RaiseRemoteAlerting);

   nextEvent = new BcRemoteAlertingEvent(*this);
   return EventHandler::Continue;
}

//------------------------------------------------------------------------------

fn_name BcSsm_RaiseRemoteAnswer = "BcSsm.RaiseRemoteAnswer";

EventHandler::Rc BcSsm::RaiseRemoteAnswer(Event*& nextEvent)
{
   Debug::ft(BcSsm_RaiseRemoteAnswer);

   nextEvent = new BcRemoteAnswerEvent(*this);
   return EventHandler::Continue;
}

//------------------------------------------------------------------------------

fn_name BcSsm_RaiseRemoteBusy = "BcSsm.RaiseRemoteBusy";

EventHandler::Rc BcSsm::RaiseRemoteBusy(Event*& nextEvent)
{
   Debug::ft(BcSsm_RaiseRemoteBusy);

   SetNextSap(BcTrigger::RemoteBusySap);
   nextEvent = new BcRemoteBusyEvent(*this, Cause::UserBusy);
   return EventHandler::Continue;
}

//------------------------------------------------------------------------------

fn_name BcSsm_RaiseRemoteNoAnswer = "BcSsm.RaiseRemoteNoAnswer";

EventHandler::Rc BcSsm::RaiseRemoteNoAnswer(Event*& nextEvent)
{
   Debug::ft(BcSsm_RaiseRemoteNoAnswer);

   SetNextSap(BcTrigger::RemoteNoAnswerSap);
   nextEvent = new BcRemoteNoAnswerEvent(*this, Cause::AnswerTimeout);
   return EventHandler::Continue;
}

//------------------------------------------------------------------------------

fn_name BcSsm_RaiseRemoteProgress = "BcSsm.RaiseRemoteProgress";

EventHandler::Rc BcSsm::RaiseRemoteProgress
   (Event*& nextEvent, Progress::Ind progress)
{
   Debug::ft(BcSsm_RaiseRemoteProgress);

   nextEvent = new BcRemoteProgressEvent(*this, progress);
   return EventHandler::Continue;
}

//------------------------------------------------------------------------------

fn_name BcSsm_RaiseRemoteRelease = "BcSsm.RaiseRemoteRelease";

EventHandler::Rc BcSsm::RaiseRemoteRelease
   (Event*& nextEvent, Cause::Ind cause)
{
   Debug::ft(BcSsm_RaiseRemoteRelease);

   SetNextSap(BcTrigger::RemoteReleaseSap);
   nextEvent = new BcRemoteReleaseEvent(*this, cause);
   return EventHandler::Continue;
}

//------------------------------------------------------------------------------

fn_name BcSsm_RaiseRemoteResume = "BcSsm.RaiseRemoteResume";

EventHandler::Rc BcSsm::RaiseRemoteResume(Event*& nextEvent)
{
   Debug::ft(BcSsm_RaiseRemoteResume);

   nextEvent = new BcRemoteResumeEvent(*this);
   return EventHandler::Continue;
}

//------------------------------------------------------------------------------

fn_name BcSsm_RaiseRemoteSuspend = "BcSsm.RaiseRemoteSuspend";

EventHandler::Rc BcSsm::RaiseRemoteSuspend(Event*& nextEvent)
{
   Debug::ft(BcSsm_RaiseRemoteSuspend);

   nextEvent = new BcRemoteSuspendEvent(*this);
   return EventHandler::Continue;
}

//------------------------------------------------------------------------------

fn_name BcSsm_RaiseSelectFacility = "BcSsm.RaiseSelectFacility";

EventHandler::Rc BcSsm::RaiseSelectFacility(Event*& nextEvent)
{
   Debug::ft(BcSsm_RaiseSelectFacility);

   SetNextSnp(BcTrigger::TerminatedSnp);
   SetNextState(BcState::SelectingFacility);
   SetNextSap(BcTrigger::SelectFacilitySap);
   nextEvent = new BcSelectFacilityEvent(*this);
   return EventHandler::Continue;
}

//------------------------------------------------------------------------------

fn_name BcSsm_RaiseSelectRoute = "BcSsm.RaiseSelectRoute";

EventHandler::Rc BcSsm::RaiseSelectRoute(Event*& nextEvent)
{
   Debug::ft(BcSsm_RaiseSelectRoute);

   SetNextSnp(BcTrigger::InformationAnalyzedSnp);
   SetNextState(BcState::SelectingRoute);
   SetNextSap(BcTrigger::SelectRouteSap);
   nextEvent = new BcSelectRouteEvent(*this);
   return EventHandler::Continue;
}

//------------------------------------------------------------------------------

fn_name BcSsm_RaiseSendCall = "BcSsm.RaiseSendCall";

EventHandler::Rc BcSsm::RaiseSendCall(Event*& nextEvent)
{
   Debug::ft(BcSsm_RaiseSendCall);

   SetNextSnp(BcTrigger::CallSetupAuthorizedSnp);
   SetNextState(BcState::SendingCall);
   SetNextSap(BcTrigger::SendCallSap);
   nextEvent = new BcSendCallEvent(*this);
   return EventHandler::Continue;
}

//------------------------------------------------------------------------------

fn_name BcSsm_RequestService = "BcSsm.RequestService";

EventHandler::Rc BcSsm::RequestService(Event*& nextEvent)
{
   Debug::ft(BcSsm_RequestService);

   //  A service code should should have been dialed.
   //
   if(analysis_.selector == Address::ScType)
   {
      auto reg = Singleton< ServiceCodeRegistry >::Instance();
      auto sid = reg->GetService(analysis_.identifier);

      if(sid != NIL_ID)
      {
         nextEvent = new InitiationReqEvent(*this, sid);
         return EventHandler::Initiate;
      }
   }

   return RaiseInvalidInformation(nextEvent);
}

//------------------------------------------------------------------------------

fn_name BcSsm_ResetStateCounts = "BcSsm.ResetStateCounts";

void BcSsm::ResetStateCounts(RestartLevel level)
{
   Debug::ft(BcSsm_ResetStateCounts);

   if(level < RestartCold) return;

   for(auto i = 0; i <= BcState::MaxBcId; ++i) StateCount_[i] = 0;
}

//------------------------------------------------------------------------------

fn_name BcSsm_SelectRoute = "BcSsm.SelectRoute";

EventHandler::Rc BcSsm::SelectRoute(Event*& nextEvent)
{
   Debug::ft(BcSsm_SelectRoute);

   route_ = RouteResult(analysis_);

   //  A route should have been determined.
   //
   if(route_.selector != NIL_ID)
   {
      auto reg = Singleton< FactoryRegistry >::Instance();
      auto fac = static_cast< BcFactory* >(reg->GetFactory(route_.selector));

      if(fac != nullptr)
      {
         auto cause = fac->VerifyRoute(route_.identifier);

         if(cause != Cause::NilInd)
         {
            return RaiseReleaseCall(nextEvent, cause);
         }

         return RaiseAuthorizeCallSetup(nextEvent);
      }
   }

   Debug::SwLog(BcSsm_SelectRoute, "invalid route", route_.selector);
   return RaiseReleaseCall(nextEvent, Cause::ExchangeRoutingError);
}

//------------------------------------------------------------------------------

fn_name BcSsm_SetModel = "BcSsm.SetModel";

void BcSsm::SetModel(Model model)
{
   Debug::ft(BcSsm_SetModel);

   model_ = model;
}

//------------------------------------------------------------------------------

fn_name BcSsm_SetNextState = "BcSsm.SetNextState";

void BcSsm::SetNextState(StateId stid)
{
   Debug::ft(BcSsm_SetNextState);

   StateCount_[CurrState()]--;
   StateCount_[stid]++;

   MediaSsm::SetNextState(stid);
}

//------------------------------------------------------------------------------

fn_name BcSsm_SetNPsm = "BcSsm.SetNPsm";

void BcSsm::SetNPsm(CipPsm& psm)
{
   Debug::ft(BcSsm_SetNPsm);

   if(nPsm_ != nullptr)
   {
      Debug::SwLog(BcSsm_SetNPsm, "PSM already exists", Sid());
      return;
   }

   nPsm_ = &psm;
}

//------------------------------------------------------------------------------

fn_name BcSsm_SetUPsm = "BcSsm.SetUPsm";

void BcSsm::SetUPsm(MediaPsm& psm)
{
   Debug::ft(BcSsm_SetUPsm);

   if(uPsm_ != nullptr)
   {
      Debug::SwLog(BcSsm_SetUPsm, "PSM already exists",
         pack2(uPsm_->GetFactory(), Sid()));
      return;
   }

   uPsm_ = &psm;
}
}
