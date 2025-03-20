//==============================================================================
//
//  PotsCircuit.cpp
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
#include "PotsCircuit.h"
#include <iomanip>
#include <sstream>
#include "Algorithms.h"
#include "Debug.h"
#include "Log.h"
#include "MsgHeader.h"
#include "NbAppIds.h"
#include "PotsLogs.h"
#include "PotsProfile.h"
#include "Switch.h"

using std::ostream;
using std::setw;
using std::string;

//------------------------------------------------------------------------------

namespace PotsBase
{
const PotsCircuit::SignalEntry PotsCircuit::NilSignalEntry = { };

//  The number of circuits in each state.
//
static int StateCount_[PotsCircuit::State_N] = { 0 };

//------------------------------------------------------------------------------

PotsCircuit::PotsCircuit(PotsProfile& profile) :
   state_(Idle),
   offhook_(false),
   ringing_(false),
   digits_(false),
   flash_(false),
   cause_(Cause::NilInd),
   profile_(&profile),
   trafficId_(0),
   buffIndex_(0),
   trace_{NilSignalEntry}
{
   Debug::ft("PotsCircuit.ctor");

   StateCount_[state_]++;
}

//------------------------------------------------------------------------------

PotsCircuit::~PotsCircuit()
{
   Debug::ftnt("PotsCircuit.dtor");

   StateCount_[state_]--;
}

//------------------------------------------------------------------------------

void PotsCircuit::ClearTrafficId(size_t tid)
{
   Debug::ft("PotsCircuit.ClearTrafficId");

   if(trafficId_ == tid) trafficId_ = 0;
}

//------------------------------------------------------------------------------

Pots_UN_Message* PotsCircuit::CreateMsg(PotsSignal::Id sid) const
{
   Debug::ft("PotsCircuit.CreateMsg");

   auto msg = new Pots_UN_Message(nullptr, 12);
   msg->Header()->injected = true;

   PotsHeaderInfo phi;
   phi.signal = sid;
   phi.port = TsPort();
   msg->AddHeader(phi);

   return msg;
}

//------------------------------------------------------------------------------

void PotsCircuit::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Circuit::Display(stream, prefix, options);

   stream << prefix << "state     : " << state_ << CRLF;
   stream << prefix << "offhook   : " << offhook_ << CRLF;
   stream << prefix << "ringing   : " << ringing_ << CRLF;
   stream << prefix << "digits    : " << digits_ << CRLF;
   stream << prefix << "flash     : " << flash_ << CRLF;
   stream << prefix << "cause     : " << cause_ << CRLF;
   stream << prefix << "trafficId : " << trafficId_ << CRLF;
   stream << prefix << "trace     : " << strTrace() << CRLF;

   if(profile_ != nullptr)
      stream << prefix << "profile DN : " << profile_->GetDN();
   else
      stream << prefix << "profile : undefined";
   stream << CRLF;
}

//------------------------------------------------------------------------------

fixed_string CircuitStateStr[PotsCircuit::State_N] =
{
   "Idle", "Actv", "Orig", "Term", "Lock"
};

void PotsCircuit::DisplayStateCounts(ostream& stream, const string& prefix)
{
   stream << prefix;
   for(size_t i = 0; i < State_N; ++i) stream << setw(6) << CircuitStateStr[i];
   stream << CRLF;

   stream << prefix;
   for(size_t i = 0; i < State_N; ++i) stream << setw(6) << StateCount_[i];
   stream << CRLF;
}

//------------------------------------------------------------------------------

string PotsCircuit::Name() const
{
   string name = "POTS ";

   if(profile_ != nullptr)
      name += std::to_string(profile_->GetDN());
   else
      name += "unassigned";

   return name;
}

//------------------------------------------------------------------------------

void PotsCircuit::ReceiveMsg(const Pots_NU_Message& msg)
{
   Debug::ft("PotsCircuit.ReceiveMsg");

   PotsRingInfo* pri;
   PotsScanInfo* psi;
   CauseInfo* pci;

   auto phi = msg.FindType<PotsHeaderInfo>(PotsParameter::Header);

   SignalEntry entry = NilSignalEntry;
   entry.signal = phi->signal & 0x0f;

   switch(phi->signal)
   {
   case PotsSignal::Supervise:
      //
      //  Look for all possible parameters.
      //
      if(state_ == Idle) SetState(Terminator);
      pri = msg.FindType<PotsRingInfo>(PotsParameter::Ring);
      psi = msg.FindType<PotsScanInfo>(PotsParameter::Scan);
      pci = msg.FindType<CauseInfo>(PotsParameter::Cause);

      if(pri != nullptr)
      {
         if(pri->on)
         {
            entry.ringOn = true;
            Trace(entry);
            ringing_ = true;

            if(state_ == Active) SetState(Terminator);

            //  If onhook, send an Alerting unless an alerting timeout is
            //  wanted.  If offhook, send an Offhook because the previous
            //  offhook might have arrived on the ingress queue and been
            //  rejected by overload controls.
            //
            if(!offhook_)
            {
               if(!Debug::SwFlagOn(CipAlertingTimeoutFlag))
               {
                  SendMsg(PotsSignal::Alerting);
               }
            }
            else
            {
               SendMsg(PotsSignal::Offhook);
            }
         }
         else
         {
            entry.ringOff = true;
            Trace(entry);
            ringing_ = false;
         }
      }

      //  Update the events to be reported.
      //
      if(psi != nullptr)
      {
         digits_ = psi->digits;
         flash_ = psi->flash;

         if(digits_)
            entry.digsOn = true;
         else
            entry.digsOff = true;

         if((state_ == Active) && digits_) SetState(Originator);
      }

      if(pci != nullptr) cause_ = pci->cause;
      if(pri == nullptr) Trace(entry);
      return;

   case PotsSignal::Lockout:
      //
      //  Connect silence.  Wait for an onhook.  Report nothing else.
      //
      Trace(entry);
      SetState(LockedOut);
      digits_ = false;
      flash_ = false;
      MakeConn(Switch::SilentPort);
      return;

   case PotsSignal::Release:
      //
      //  Idle the circuit.  If it is offhook, send an offhook immediately.
      //
      Trace(entry);
      pci = msg.FindType<CauseInfo>(PotsParameter::Cause);

      if((pci != nullptr) && (pci->cause == Cause::ResetCircuit))
      {
         ResetCircuit();
         return;
      }

      SetState(offhook_ ? Active : Idle);
      MakeConn(Switch::SilentPort);
      ringing_ = false;
      digits_ = false;
      flash_ = false;
      cause_ = Cause::NilInd;
      if(state_ == Active) SendMsg(PotsSignal::Offhook);
      return;

   default:
      auto log = Log::Create(PotsLogGroup, PotsShelfIcSignal);
      if(log == nullptr) return;
      *log << Log::Tab << "sig=" << phi->signal << SPACE << strState();
      Log::Submit(log);
      return;
   }
}

//------------------------------------------------------------------------------

void PotsCircuit::ResetCircuit()
{
   Debug::ft("PotsCircuit.ResetCircuit");

   //  If the circuit is not in its initial state, reset it and
   //  generate a log.
   //
   bool err;
   string info;
   auto rx = RxFrom();

   err = ((rx != Switch::SilentPort) || (state_ != Idle) || offhook_);
   err = (err || ringing_ || digits_ || flash_ || (cause_ != Cause::NilInd));

   if(err)
   {
      info = strState();
      MakeConn(Switch::SilentPort);
      SetState(Idle);
      offhook_ = false;
      ringing_ = false;
      digits_ = false;
      flash_ = false;
      cause_ = Cause::NilInd;

      auto log = Log::Create(PotsLogGroup, PotsShelfCircuitReset);
      if(log == nullptr) return;
      *log << Log::Tab << info;
      Log::Submit(log);
   }
}

//------------------------------------------------------------------------------

void PotsCircuit::ResetStateCounts(RestartLevel level)
{
   Debug::ft("PotsCircuit.ResetStateCounts");

   if(level < RestartCold) return;

   for(size_t i = 0; i < State_N; ++i) StateCount_[i] = 0;
}

//------------------------------------------------------------------------------

bool PotsCircuit::SendMsg(Pots_UN_Message& msg)
{
   Debug::ft("PotsCircuit.SendMsg");

   bool ok = false;
   auto sid = msg.GetSignal();

   switch(sid)
   {
   case PotsSignal::Offhook:
      //
      //  Send this even if already offhook. There are two reasons for this.
      //
      //  Overload controls discard origination attempts.  Under the "dial
      //  tone at all costs" approach, an Offhook is therefore periodically
      //  retransmitted so that users who wait patiently (instead of rattling
      //  the switchhook) eventually get dial tone.  POTS call software takes
      //  this into account by discarding a retransmitted offhook.
      //
      //e Race conditions can cause lost messages.  For example, the suspend
      //  timer can expire just as a user goes back offhook.  The call gets
      //  released, the offhook message (queued on the context) gets discarded,
      //  and the circuit receives a Release.  When the circuit processes the
      //  Release, it must retransmit the Offhook.  It can be argued that when
      //  a context is deleted, messages still queued against it (such as the
      //  Offhook) should be reinjected.  This would cause the creation of a
      //  context to process the message, but this has not been implemented.
      //
      offhook_ = true;
      ok = true;
      if(state_ == Idle) SetState(Active);
      break;
   case PotsSignal::Digits:
      ok = digits_;
      break;
   case PotsSignal::Flash:
      ok = flash_;
      break;
   case PotsSignal::Alerting:
      ok = ringing_;
      break;
   case PotsSignal::Onhook:
      if(offhook_)
      {
         offhook_ = false;
         digits_ = false;
         ok = true;
         if(state_ == Active) SetState(Idle);
         break;
      }
   }

   if(ok)
   {
      auto entry = NilSignalEntry;
      entry.signal = sid & 0xf;
      Trace(entry);
      return msg.Send(Message::External);
   }

   //  When msg.Send is invoked, the message is deleted, even on failure.
   //  We should therefore do the same.
   //
   delete &msg;

   auto log = Log::Create(PotsLogGroup, PotsShelfOgSignal);
   if(log == nullptr) return false;
   *log << Log::Tab << "sig=" << sid << SPACE << strState();
   Log::Submit(log);
   return false;
}

//------------------------------------------------------------------------------

fn_name PotsCircuit_SendMsg2 = "PotsCircuit.SendMsg(signal)";

bool PotsCircuit::SendMsg(PotsSignal::Id sid)
{
   Debug::ft(PotsCircuit_SendMsg2);

   switch(sid)
   {
   case PotsSignal::Offhook:
   case PotsSignal::Alerting:
   case PotsSignal::Flash:
   case PotsSignal::Onhook:
      break;
   default:
      //
      //  This includes PotsSignal::Digits, which needs a parameter.
      //
      Debug::SwLog(PotsCircuit_SendMsg2,
         "invalid signal", pack2(TsPort(), sid));
      return false;
   }

   auto msg = CreateMsg(sid);
   if(msg == nullptr) return false;
   return SendMsg(*msg);
}

//------------------------------------------------------------------------------

void PotsCircuit::SetState(State state)
{
   Debug::ft("PotsCircuit.SetState");

   StateCount_[state_]--;
   state_ = state;
   StateCount_[state_]++;
}

//------------------------------------------------------------------------------

string PotsCircuit::strState() const
{
   std::ostringstream stream;

   stream << "cct=" << Name();
   stream << " p=" << TsPort();
   stream << " rx=" << RxFrom();
   stream << " s=" << state_;
   stream << " h=" << offhook_;
   stream << " r=" << ringing_;
   stream << " d=" << digits_;
   stream << " f=" << flash_;
   stream << " c=" << int(cause_);
   stream << " t=" << trafficId_;
   stream << " m=" << strTrace();

   return stream.str();
}

//------------------------------------------------------------------------------

fixed_string SigChars = "01BDA5E78SLRcdef";

string PotsCircuit::strTrace() const
{
   string s;
   char c;
   size_t i = buffIndex_;

   while(true)
   {
      const auto& entry = trace_[i];

      if(entry.signal != 0)
      {
         c = SigChars[entry.signal];

         if(entry.signal == PotsSignal::Supervise)
         {
            if(entry.digsOn) c = '@';
            else if(entry.digsOff) c = '#';
            else if(entry.ringOn) c = '*';
            else if(entry.ringOff) c = '.';
         }

         s += c;
      }

      if(++i == TraceSize) i = 0;
      if(i == buffIndex_) break;
   }

   return s;
}

//------------------------------------------------------------------------------

void PotsCircuit::Trace(const SignalEntry& entry)
{
   trace_[buffIndex_] = entry;
   if(++buffIndex_ >= TraceSize) buffIndex_ = 0;
}
}
