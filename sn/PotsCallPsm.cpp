//==============================================================================
//
//  PotsCallPsm.cpp
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
#include "PotsProtocol.h"
#include <ostream>
#include "BcProgress.h"
#include "Debug.h"
#include "Formatters.h"
#include "GlobalAddress.h"
#include "IpPortRegistry.h"
#include "NwTypes.h"
#include "SbAppIds.h"
#include "SbEvents.h"
#include "SysTypes.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace PotsBase
{
fn_name PotsCallPsm_ctor1 = "PotsCallPsm.ctor(first)";

PotsCallPsm::PotsCallPsm(Switch::PortId port) : MediaPsm(PotsCallFactoryId),
   ogMsg_(nullptr),
   sendRing_(false),
   sendScan_(false),
   sendCause_(false),
   sendFacility_(false)
{
   Debug::ft(PotsCallPsm_ctor1);

   header_.port = port;
}

//------------------------------------------------------------------------------

fn_name PotsCallPsm_ctor2 = "PotsCallPsm.ctor(subseq)";

PotsCallPsm::PotsCallPsm(ProtocolLayer& adj, bool upper, Switch::PortId port) :
   MediaPsm(PotsCallFactoryId, adj, upper),
   ogMsg_(nullptr),
   sendRing_(false),
   sendScan_(false),
   sendCause_(false),
   sendFacility_(false)
{
   Debug::ft(PotsCallPsm_ctor2);

   header_.port = port;
}

//------------------------------------------------------------------------------

fn_name PotsCallPsm_dtor = "PotsCallPsm.dtor";

PotsCallPsm::~PotsCallPsm()
{
   Debug::ft(PotsCallPsm_dtor);
}

//------------------------------------------------------------------------------

fn_name PotsCallPsm_ApplyRinging = "PotsCallPsm.ApplyRinging";

void PotsCallPsm::ApplyRinging(bool on)
{
   Debug::ft(PotsCallPsm_ApplyRinging);

   if(ring_.on == on) return;
   ring_.on = on;
   sendRing_ = true;
   SendSignal(PotsSignal::Supervise);
}

//------------------------------------------------------------------------------

fn_name PotsCallPsm_Cast = "PotsCallPsm.Cast";

PotsCallPsm* PotsCallPsm::Cast(ProtocolSM* psm)
{
   Debug::ft(PotsCallPsm_Cast);

   if((psm != nullptr) && (psm->GetFactory() == PotsCallFactoryId))
   {
      return static_cast< PotsCallPsm* >(psm);
   }

   return nullptr;
}

//------------------------------------------------------------------------------

void PotsCallPsm::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   ProtocolSM::Display(stream, prefix, options);

   stream << prefix << "ogMsg        : " << ogMsg_ << CRLF;
   stream << prefix << "sendRing     : " << sendRing_ << CRLF;
   stream << prefix << "sendScan     : " << sendScan_ << CRLF;
   stream << prefix << "sendCause    : " << sendCause_ << CRLF;
   stream << prefix << "sendFacility : " << sendFacility_ << CRLF;
   stream << prefix << "header : " << CRLF;
   header_.Display(stream, prefix + spaces(2));
   stream << prefix << "ring : " << CRLF;
   ring_.Display(stream, prefix + spaces(2));
   stream << prefix << "scan : " << CRLF;
   scan_.Display(stream, prefix + spaces(2));
   stream << prefix << "cause : " << CRLF;
   cause_.Display(stream, prefix + spaces(2));
   stream << prefix << "facility : " << CRLF;
   facility_.Display(stream, prefix + spaces(2));
}

//------------------------------------------------------------------------------

fn_name PotsCallPsm_EnsureMediaMsg = "PotsCallPsm.EnsureMediaMsg";

void PotsCallPsm::EnsureMediaMsg()
{
   Debug::ft(PotsCallPsm_EnsureMediaMsg);

   //  A media update can be included in any message, so an outgoing
   //  message only needs to be created if one doesn't already exist.
   //
   if((FirstOgMsg() == nullptr) && (GetState() != Idle))
   {
      SendSignal(PotsSignal::Supervise);
   }
}

//------------------------------------------------------------------------------

fn_name PotsCallPsm_InjectFinalMsg = "PotsCallPsm.InjectFinalMsg";

void PotsCallPsm::InjectFinalMsg()
{
   Debug::ft(PotsCallPsm_InjectFinalMsg);

   auto msg = new PotsMessage(this, 20);

   header_.signal = PotsSignal::Release;
   msg->AddHeader(header_);
   cause_.cause = Cause::TemporaryFailure;
   msg->AddCause(cause_);
   msg->SendToSelf();
}

//------------------------------------------------------------------------------

fn_name PotsCallPsm_ProcessIcMsg = "PotsCallPsm.ProcessIcMsg";

ProtocolSM::IncomingRc PotsCallPsm::ProcessIcMsg(Message& msg, Event*& event)
{
   Debug::ft(PotsCallPsm_ProcessIcMsg);

   auto& pmsg = static_cast< Pots_UN_Message& >(msg);
   auto sid = pmsg.GetSignal();

   MediaPsm::UpdateIcMedia(pmsg, PotsParameter::Media);

   switch(sid)
   {
   case PotsSignal::Release:
      SetState(Idle);
      break;

   case PotsSignal::Progress:
      //
      //  If this is only a media update, do not raise the AnalyzeMsgEvent.
      {
         auto ppi = pmsg.FindType< ProgressInfo >(PotsParameter::Progress);
         if(ppi->progress == Progress::MediaUpdate) return DiscardMessage;
      }
      //  [[fallthrough]]
   default:
      SetState(Active);
   }

   event = new AnalyzeMsgEvent(msg);
   return EventRaised;
}

//------------------------------------------------------------------------------

fn_name PotsCallPsm_ProcessOgMsg = "PotsCallPsm.ProcessOgMsg";

ProtocolSM::OutgoingRc PotsCallPsm::ProcessOgMsg(Message& msg)
{
   Debug::ft(PotsCallPsm_ProcessOgMsg);

   auto& pmsg = static_cast< Pots_NU_Message& >(msg);

   if(&msg == ogMsg_)
   {
      ogMsg_ = nullptr;
   }
   else
   {
      bool del = false;

      //  We created ogMsg_ to send a signal, but now we're sending another
      //  message first.  This occurs when the incoming message was relayed,
      //  because relayed messages are henq'd on the outgoing message queue.
      //  If the relayed message is taking the call down, transition to the
      //  idle state and delete ogMsg_.  If it is not taking the call down,
      //  add any pending media parameter to it.
      //
      switch(pmsg.GetSignal())
      {
      case PotsSignal::Lockout:
      case PotsSignal::Release:
         SetState(Idle);
         del = true;
         break;
      default:
         MediaPsm::UpdateOgMedia(pmsg, PotsParameter::Media);
      }

      //  If ogMsg_ was only created to send a media parameter, delete it:
      //  the parameter has been included in the relayed message instead.
      //
      if(!del && (header_.signal == PotsSignal::Supervise) &&
         !sendRing_ && !sendScan_ && !sendCause_ && !sendFacility_)
      {
         del = true;
      }

      if(del)
      {
         header_.signal = NIL_ID;
         delete ogMsg_;
         ogMsg_ = nullptr;
      }

      return SendMessage;
   }

   switch(header_.signal)
   {
   case NIL_ID:
      return PurgeMessage;

   case PotsSignal::Supervise:
      SetState(Active);
      pmsg.AddHeader(header_);

      if(sendRing_)
      {
         pmsg.AddRing(ring_);
         sendRing_ = false;
      }

      if(sendScan_)
      {
         pmsg.AddScan(scan_);
         sendScan_ = false;
      }

      if(sendCause_)
      {
         pmsg.AddCause(cause_);
         sendCause_ = false;
         cause_.cause = Cause::NilInd;
      }

      if(sendFacility_)
      {
         pmsg.AddFacility(facility_);
         sendFacility_ = false;
      }

      MediaPsm::UpdateOgMedia(pmsg, PotsParameter::Media);
      break;

   case PotsSignal::Lockout:
      pmsg.AddHeader(header_);
      SetState(Idle);
      break;

   case PotsSignal::Release:
      pmsg.AddHeader(header_);
      pmsg.AddCause(cause_);
      SetState(Idle);
      break;

   case PotsSignal::Facility:
      SetState(Active);
      pmsg.AddHeader(header_);
      pmsg.AddFacility(facility_);
      sendFacility_ = false;
      MediaPsm::UpdateOgMedia(pmsg, PotsParameter::Media);
      break;

   default:
      Debug::SwErr(PotsCallPsm_ProcessOgMsg, header_.signal, 1);
      return PurgeMessage;
   }

   header_.signal = NIL_ID;

   //d If this is an initial message, it must provide the source and
   //  destination addresses.
   //
   if(AddressesUnknown(&msg))
   {
      auto host = IpPortRegistry::HostAddress();
      auto peer = IpPortRegistry::HostAddress();
      auto locAddr = GlobalAddress(host, PotsCallIpPort, PotsCallFactoryId);
      auto remAddr = GlobalAddress(peer, PotsShelfIpPort, PotsShelfFactoryId);

      msg.SetSender(locAddr);
      msg.SetReceiver(remAddr);
   }

   return SendMessage;
}

//------------------------------------------------------------------------------

fn_name PotsCallPsm_ReportDigits = "PotsCallPsm.ReportDigits";

void PotsCallPsm::ReportDigits(bool report)
{
   Debug::ft(PotsCallPsm_ReportDigits);

   if(scan_.digits == report) return;
   scan_.digits = report;
   sendScan_ = true;
   SendSignal(PotsSignal::Supervise);
}

//------------------------------------------------------------------------------

fn_name PotsCallPsm_ReportFlash = "PotsCallPsm.ReportFlash";

void PotsCallPsm::ReportFlash(bool report)
{
   Debug::ft(PotsCallPsm_ReportFlash);

   if(scan_.flash == report) return;
   scan_.flash = report;
   sendScan_ = true;
   SendSignal(PotsSignal::Supervise);
}

//------------------------------------------------------------------------------

fn_name PotsCallPsm_Route = "PotsCallPsm.Route";

Message::Route PotsCallPsm::Route() const
{
   Debug::ft(PotsCallPsm_Route);

   //  Messages to the POTS shelf are sent over the IP stack.  Messages to
   //  another POTS UPSM or a POTS multiplexer are sent internally.
   //
   if(PeerFactory() == PotsShelfFactoryId)
   {
      return Message::External;
   }

   return Message::Internal;
}

//------------------------------------------------------------------------------

fn_name PotsCallPsm_SendCause = "PotsCallPsm.SendCause";

void PotsCallPsm::SendCause(Cause::Ind cause)
{
   Debug::ft(PotsCallPsm_SendCause);

   cause_.cause = cause;
   sendCause_ = true;
   SendSignal(PotsSignal::Supervise);
}

//------------------------------------------------------------------------------

fn_name PotsCallPsm_SendFacility = "PotsCallPsm.SendFacility";

void PotsCallPsm::SendFacility(ServiceId sid, Facility::Ind ind)
{
   Debug::ft(PotsCallPsm_SendFacility);

   facility_.sid = sid;
   facility_.ind = ind;
   sendFacility_ = true;
   SendSignal(PotsSignal::Facility);
}

//------------------------------------------------------------------------------

fn_name PotsCallPsm_SendFinalMsg = "PotsCallPsm.SendFinalMsg";

void PotsCallPsm::SendFinalMsg()
{
   Debug::ft(PotsCallPsm_SendFinalMsg);

   if(GetState() == Idle) return;
   auto msg = new PotsMessage(this, 20);
   if(msg == nullptr) return;

   header_.signal = PotsSignal::Release;
   msg->AddHeader(header_);
   cause_.cause = Cause::TemporaryFailure;
   msg->AddCause(cause_);
   SendToLower(*msg);
}

//------------------------------------------------------------------------------

fn_name PotsCallPsm_SendSignal = "PotsCallPsm.SendSignal";

void PotsCallPsm::SendSignal(SignalId signal)
{
   Debug::ft(PotsCallPsm_SendSignal);

   if(ogMsg_ == nullptr)
   {
      ogMsg_ = new Pots_NU_Message(this, 32);
   }

   //  Update the signal to be sent.  The precedence order
   //  is Release > Lockout > Supervise > Facility.
   //
   switch(signal)
   {
   case PotsSignal::Release:
      header_.signal = signal;
      break;

   case PotsSignal::Lockout:
      if(header_.signal != PotsSignal::Release)
      {
         header_.signal = signal;
      }
      break;

   case PotsSignal::Supervise:
      if((header_.signal == NIL_ID) || (header_.signal == PotsSignal::Facility))
      {
         header_.signal = signal;
      }
      break;

   case PotsSignal::Facility:
      if(header_.signal == NIL_ID)
      {
         header_.signal = signal;
      }
      break;

   default:
      Debug::SwErr(PotsCallPsm_SendSignal, signal, 0);
   }
}

//------------------------------------------------------------------------------

fn_name PotsCallPsm_Synch = "PotsCallPsm.Synch";

void PotsCallPsm::Synch(PotsCallPsm& upsm) const
{
   Debug::ft(PotsCallPsm_Synch);

   upsm.SetState(GetState());
   SynchEdge(upsm);
   upsm.ring_ = ring_;
   upsm.scan_ = scan_;
}
}
