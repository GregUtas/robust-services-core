//==============================================================================
//
//  PotsTrafficThread.cpp
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
#include "PotsTrafficThread.h"
#include "Dynamic.h"
#include <cstdint>
#include <iomanip>
#include <sstream>
#include "Algorithms.h"
#include "Debug.h"
#include "Formatters.h"
#include "FunctionGuard.h"
#include "Log.h"
#include "Memory.h"
#include "PotsCircuit.h"
#include "PotsLogs.h"
#include "PotsProfile.h"
#include "PotsProfileRegistry.h"
#include "PotsProtocol.h"
#include "Q1Link.h"
#include "Q1Way.h"
#include "Restart.h"
#include "SbPools.h"
#include "Singleton.h"
#include "Tones.h"

using namespace MediaBase;
using std::ostream;
using std::setw;
using std::string;
using namespace SessionBase;

//------------------------------------------------------------------------------

namespace PotsBase
{
//  Tracks a call created during a traffic run.
//
class TrafficCall : public Dynamic
{
public:
   //  Creates a call that will be set up by ORIG.
   //
   explicit TrafficCall(PotsCircuit& orig);

   //  Deletes the call when no active circuits remain.
   //
   ~TrafficCall();

   //  Deleted to prohibit copying.
   //
   TrafficCall(const TrafficCall& that) = delete;
   TrafficCall& operator=(const TrafficCall& that) = delete;

   enum State
   {
      Originating,  // waiting for dial tone
      Dialing,      // dialed partial digit string
      Terminating,  // waiting for ringback
      Ringing,      // receiving ringback
      Connected,    // talking
      Suspended,    // terminator suspended
      SingleEnded,  // single user, still offhook
      Releasing,    // single user ending call
      State_N       // number of states
   };

   //  Originates a call, returning how long to wait until the next
   //  message will be sent.  Returns 0 if the call is over.
   //
   msecs_t Originate() const;

   //  Selects the next action for a call in progress, returning how
   //  long to wait until the next message will be sent.  Returns 0
   //  if the call is over.
   //
   msecs_t Advance();

   //  Returns true if the call has no originator or terminator.
   //
   bool Empty() const
   { return ((orig_ == nullptr) && (term_ == nullptr)); }

   //  Displays the number of calls in each state.
   //
   static void DisplayStateCounts(ostream& stream, const string& prefix);

   //  Clears the number of calls in each state when the traffic
   //  thread exits.
   //
   static void ResetStateCounts();

   //  Returns the offset to link_.
   //
   static ptrdiff_t LinkDiff();

   //  Overridden to display member variables.
   //
   void Display(ostream& stream,
      const string& prefix, const Flags& options) const override;

   //  Overridden to obtain a TrafficCall from its object pool if
   //  available, else from the dynamic heap.
   //
   static void* operator new(size_t size);

   //  Overridden to return a TrafficCall to its object pool.
   //
   static void operator delete(void* addr);
private:
   //> The size of the DelayMsecs_ array.
   //
   static const size_t DelaySize = 4;

   //  Advances a call from the indicated state, returning how long
   //  to wait until the next message will be sent.
   //
   msecs_t ProcessOriginating();
   msecs_t ProcessDialing();
   msecs_t ProcessTerminating();
   msecs_t ProcessRinging();
   msecs_t ProcessConnected();
   msecs_t ProcessSuspended();
   msecs_t ProcessSingleEnded();
   msecs_t ProcessReleasing();

   //  Updates the circuit's state.
   //
   void SetState(State state);

   //  Timeouts can cause a terminator to be unknowingly released.  When
   //  this has occurred, this removes the terminator from the call and
   //  returns false.  If the terminator is still valid, it returns true.
   //
   bool CheckTerm();

   //  Sends an onhook from the originator or terminator, respectively.
   //
   void ReleaseOrig();
   void ReleaseTerm();

   //  Removes the originator or terminator, respectively.
   //
   void EraseOrig();
   void EraseTerm();

   //  Returns a string for displaying STATE.
   //
   static c_string strState(State state);

   //  The next call in the timeslot.
   //
   Q1Link link_;

   //  The call's identifier.
   //
   const size_t callid_;

   //  The circuit that is originating the call.
   //
   PotsCircuit* orig_;

   //  The times at which the originator entered and left the call.
   //
   const ticks_t origStart_;
   ticks_t origEnd_;

   //  The amount of time to wait before sending another offhook.
   //
   size_t delay_;

   //  The address of the destination.
   //
   Address::DN dest_;

   //  The circuit that is receiving the call.
   //
   PotsCircuit* term_;

   //  The times at which the terminator entered and left the call.
   //
   ticks_t termStart_;
   ticks_t termEnd_;

   //  The call's state.
   //
   State state_;

   //  The number of milliseconds to wait before looking for dial tone
   //  and retransmiting an offhook if dial tone is not yet connected.
   //
   static const msecs_t DelayMsecs_[DelaySize];

   //  The number of calls in each state.
   //
   static int StateCount_[State_N];

   //  A sequence number to distinguish traffic calls.
   //
   static size_t CallId_;
};

//  Object pool for TrafficCalls.
//
class TrafficCallPool : public Dynamic
{
   friend class Singleton< TrafficCallPool >;
public:
   //  Deleted to prohibit copying.
   //
   TrafficCallPool(const TrafficCallPool& that) = delete;
   TrafficCallPool& operator=(const TrafficCallPool& that) = delete;

   //  Gets a TrafficCall from the pool.
   //
   TrafficCall* Deq() { return freeq_.Deq(); }

   //  Returns a TrafficCall to the pool.
   //
   void Enq(TrafficCall* tc) { freeq_.Enq(*tc); }
private:
   //  Creates the pool.
   //
   TrafficCallPool();

   //  Destroys the pool.
   //
   ~TrafficCallPool();

   //  The free queue of calls, which minimizes use of the heap.
   //
   Q1Way< TrafficCall > freeq_;
};

//==============================================================================

fn_name TrafficCallPool_ctor = "TrafficCallPool.ctor";

TrafficCallPool::TrafficCallPool()
{
   Debug::ft(TrafficCallPool_ctor);

   freeq_.Init(TrafficCall::LinkDiff());
}

//------------------------------------------------------------------------------

fn_name TrafficCallPool_dtor = "TrafficCallPool.dtor";

TrafficCallPool::~TrafficCallPool()
{
   Debug::ft(TrafficCallPool_dtor);

   //  The TrafficCalls on freeq_ have already been destructed, so now they
   //  only need to return to their heap.
   //
   for(auto call = freeq_.Deq(); call != nullptr; call = freeq_.Deq())
   {
      Memory::Free(call);
   }
}

//==============================================================================

const msecs_t TrafficCall::DelayMsecs_[] = {2000, 3500, 5000, 7500};
int TrafficCall::StateCount_[] = { 0 };
size_t TrafficCall::CallId_ = 1;

//------------------------------------------------------------------------------

fn_name TrafficCall_ctor = "TrafficCall.ctor";

TrafficCall::TrafficCall(PotsCircuit& orig) :
   callid_(CallId_),
   orig_(&orig),
   origStart_(Clock::TicksNow()),
   origEnd_(0),
   delay_(0),
   dest_(Address::NilDN),
   term_(nullptr),
   termStart_(0),
   termEnd_(0),
   state_(Originating)
{
   Debug::ft(TrafficCall_ctor);

   ++CallId_;
   orig.SetTrafficId(callid_);
   ++StateCount_[state_];
}

//------------------------------------------------------------------------------

fn_name TrafficCall_dtor = "TrafficCall.dtor";

TrafficCall::~TrafficCall()
{
   Debug::ft(TrafficCall_dtor);

   --StateCount_[state_];

   //  Ensure that the originator and terminator have released.
   //
   ReleaseOrig();
   ReleaseTerm();

   //  Update holding times.  Because we wait 2 seconds when entering the
   //  Terminating state, add 2 seconds to the terminator's holding time.
   //
   auto thread = Singleton< PotsTrafficThread >::Instance();

   if(origEnd_ != 0)
   {
      auto ticks = origEnd_ - origStart_;
      thread->RecordHoldingTime(Clock::TicksToSecs(ticks));
   }

   if(termEnd_ != 0)
   {
      auto ticks = termEnd_ - termStart_;
      thread->RecordHoldingTime(Clock::TicksToSecs(ticks) + 2);
   }
}

//------------------------------------------------------------------------------

fn_name TrafficCall_Advance = "TrafficCall.Advance";

msecs_t TrafficCall::Advance()
{
   Debug::ft(TrafficCall_Advance);

   switch(state_)
   {
   case Originating:
      return ProcessOriginating();
   case Dialing:
      return ProcessDialing();
   case Terminating:
      return ProcessTerminating();
   case Ringing:
      return ProcessRinging();
   case Connected:
      return ProcessConnected();
   case Suspended:
      return ProcessSuspended();
   case SingleEnded:
      return ProcessSingleEnded();
   case Releasing:
      return ProcessReleasing();
   default:
      Debug::SwLog(TrafficCall_Advance, "unexpected state", state_);
   }

   return 0;
}

//------------------------------------------------------------------------------

fn_name TrafficCall_CheckTerm = "TrafficCall.CheckTerm";

bool TrafficCall::CheckTerm()
{
   Debug::ft(TrafficCall_CheckTerm);

   if(term_ == nullptr) return false;

   if(term_->GetTrafficId() == callid_) return true;

   //  The terminator was released, probably because of an answer or suspend
   //  timeout.  Release this call after ensuring that it won't try to send
   //  an Onhook on behalf of its former terminator.  This used to be logged
   //  but was only seen when trace tools were imposing significant overhead,
   //  so the log was removed.
   //
   Singleton< PotsTrafficThread >::Instance()->RecordAbort();
   EraseTerm();
   return false;
}

//------------------------------------------------------------------------------

void TrafficCall::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Dynamic::Display(stream, prefix, options);

   stream << prefix << "callid : " << callid_;
   stream << prefix << "orig   : ";
   if(orig_ == nullptr)
      stream << "none";
   else
      stream << orig_->Name();
   stream << CRLF;

   stream << prefix << "delay  : " << delay_ << CRLF;
   stream << prefix << "dest   : " << dest_ << CRLF;

   stream << prefix << "term   : ";
   if(term_ == nullptr)
      stream << "none";
   else
      stream << term_->Name();
   stream << CRLF;

   stream << prefix << "state  : " << strState(state_) << CRLF;
}

//------------------------------------------------------------------------------

fixed_string TrafficStateStr[TrafficCall::State_N + 1] =
{
   "Orig",
   "Dial",
   "Term",
   "Ring",
   "Conn",
   "Susp",
   "Disc",
   "Rlsg",
   ERROR_STR
};

void TrafficCall::DisplayStateCounts(ostream& stream, const string& prefix)
{
   stream << prefix;
   for(auto i = 0; i < State_N; ++i) stream << setw(6) << TrafficStateStr[i];
   stream << CRLF;

   stream << prefix;
   for(auto i = 0; i < State_N; ++i) stream << setw(6) << StateCount_[i];
   stream << CRLF;
}

//------------------------------------------------------------------------------

fn_name TrafficCall_EraseOrig = "TrafficCall.EraseOrig";

void TrafficCall::EraseOrig()
{
   Debug::ft(TrafficCall_EraseOrig);

   if(orig_ != nullptr)
   {
      orig_->ClearTrafficId(callid_);
      orig_ = nullptr;
      origEnd_ = Clock::TicksNow();
   }
}

//------------------------------------------------------------------------------

fn_name TrafficCall_EraseTerm = "TrafficCall.EraseTerm";

void TrafficCall::EraseTerm()
{
   Debug::ft(TrafficCall_EraseTerm);

   if(term_ != nullptr)
   {
      term_->ClearTrafficId(callid_);
      term_ = nullptr;
      termEnd_ = Clock::TicksNow();
   }
}

//------------------------------------------------------------------------------

ptrdiff_t TrafficCall::LinkDiff()
{
   uintptr_t local;
   auto fake = reinterpret_cast< const TrafficCall* >(&local);
   return ptrdiff(&fake->link_, fake);
}

//------------------------------------------------------------------------------

fn_name TrafficCall_delete = "TrafficCall.operator delete";

void TrafficCall::operator delete(void* addr)
{
   Debug::ft(TrafficCall_delete);

   Singleton< TrafficCallPool >::Instance()->Enq((TrafficCall*) addr);
}

//------------------------------------------------------------------------------

fn_name TrafficCall_new = "TrafficCall.operator new";

void* TrafficCall::operator new(size_t size)
{
   Debug::ft(TrafficCall_new);

   auto data = Singleton< TrafficCallPool >::Instance()->Deq();
   if(data != nullptr) return data;
   return Dynamic::operator new(size);
}

//------------------------------------------------------------------------------

fn_name TrafficCall_Originate = "TrafficCall.Originate";

msecs_t TrafficCall::Originate() const
{
   Debug::ft(TrafficCall_Originate);

   //  Send an offhook and look for dial tone after a brief delay.
   //
   if(!orig_->SendMsg(PotsSignal::Offhook)) return 0;
   return DelayMsecs_[0];
}

//------------------------------------------------------------------------------

fn_name TrafficCall_ProcessConnected = "TrafficCall.ProcessConnected";

msecs_t TrafficCall::ProcessConnected()
{
   Debug::ft(TrafficCall_ProcessConnected);

   if(!CheckTerm()) return 0;

   // o Release and decide what to do after 1 second (50%).
   // o Suspend and decide what to do after 1 to 7 seconds (40%).
   // o Release simultaneously (10%).
   //
   auto rnd = rand(1, 100);

   if(rnd <= 50)
   {
      ReleaseOrig();
      SetState(SingleEnded);
      return 1000;
   }

   if(rnd <= 90)
   {
      ReleaseTerm();
      SetState(Suspended);
      return rand(1000, 7000);
   }

   ReleaseOrig();
   ReleaseTerm();
   return 0;
}

//------------------------------------------------------------------------------

fn_name TrafficCall_ProcessDialing = "TrafficCall.ProcessDialing";

msecs_t TrafficCall::ProcessDialing()
{
   Debug::ft(TrafficCall_ProcessDialing);

   //  We check orig_->CanDial() here because the traffic thread falls
   //  behind when trace tools are imposing significant overhead.  Digit
   //  collection can time out, which will cause a POTS900 log if we try
   //  to send a Digits message.
   //
   //  o Abandon (5%).
   //  o Time out (5%).
   //  o Dial the rest of the DN (90%).
   //
   auto dial = orig_->CanDial();
   auto rnd = rand(1, 100);

   if((rnd <= 5) || !dial)
   {
      if(!dial) Singleton< PotsTrafficThread >::Instance()->RecordAbort();
      ReleaseOrig();
      return 0;
   }

   if(!orig_->CanDial())
   {
      ReleaseOrig();
      return 0;
   }

   if(rnd <= 10)
   {
      SetState(SingleEnded);
      return 1000 * PotsProtocol::InterDigitTimeout;
   }

   auto msg = orig_->CreateMsg(PotsSignal::Digits);
   if(msg == nullptr) return 1000;
   DigitString ds(dest_ % 100);

   msg->AddDigits(ds);
   if(!orig_->SendMsg(*msg)) return 0;
   SetState(Terminating);
   return 2000;
}

//------------------------------------------------------------------------------

fn_name TrafficCall_ProcessOriginating = "TrafficCall.ProcessOriginating";

msecs_t TrafficCall::ProcessOriginating()
{
   Debug::ft(TrafficCall_ProcessOriginating);

   auto state = orig_->GetState();

   //  Note that a call could have arrived while our offhook was in transit.
   //  If our offhook answered that call, another TrafficCall instance now
   //  owns our circuit.  But that call could also have released before it
   //  received our offhook, in which case the following occurs:
   //
   //          POTS call        POTS circuit
   //            :                       :
   //            |            <-offhook1-| state = Active, callid = n
   //  state = PC|-ring!---------------->| state = Terminator
   //   CIP REL->|            <-offhook2-|
   //            |-release-------------->| state = Idle, callid = n + 1
   //           ===           <-offhook3-| state = Active
   //  state = NU|<-offhook1             | offhook2 will be discarded
   //            |-digits?-------------->| state = Originator
   //  state = CI|<-offhook3             | offhook3 will be ignored
   //
   //  Now we're back to being an originator, and no other TrafficCall
   //  owns our circuit.  If we don't continue to manage it, it will go
   //  to Lockout and be left hanging there.  We could find such circuits
   //  at the end of the traffic run and send onhooks at that time, but
   //  we can try to handle this gracefully by continuing to manage the
   //  circuit if it's in the Active or Originator state, even if its
   //  call identifier has changed.
   //
   switch(state)
   {
   case PotsCircuit::Active:
      //
      //  We're still waiting for dial tone.
      //  o Abandon (20%).
      //  o Send another offhook and look for dial tone again, using the
      //    backoff scheme defined by the DelayMsecs_ array (80%).
      //
      if((rand(1, 100) <= 20) || (++delay_ >= DelaySize))
      {
         ReleaseOrig();
         return 0;
      }

      if(!orig_->SendMsg(PotsSignal::Offhook)) return 0;
      return DelayMsecs_[delay_];

   case PotsCircuit::Originator:
      //
      //  We should be able to dial now.
      //
      break;

   case PotsCircuit::Terminator:
   case PotsCircuit::Idle:
      //
      //  If our originator is now a terminator, it received a call while
      //  our offhook was in transit.  Our offhook will probably answer
      //  that call, so remove this one after erasing the originator so
      //  that our destructor won't send an onhook.  If we're idle, then
      //  the TrafficCall that terminated on our circuit decided to have
      //  it send an onhook.
      //
      orig_->ClearTrafficId(callid_);
      orig_ = nullptr;
      return 0;

   case PotsCircuit::LockedOut:
      //
      //  This shouldn't occur, but send an offhook anyway.
      //
      ReleaseOrig();
      return 0;
   }

   if(orig_->CanDial())
   {
      //  o Abandon (3%).
      //  o Time out (3%).
      //  o Dial an invalid number (3%).
      //  o Dial an unassigned DN (3%).
      //  o Dial a valid DN (and 80% of the time, one that is idle):
      //    --Send the full DN in the first message (76%).
      //    --Send the DN in two separate messages (12%).
      //
      auto rnd = rand(1, 100);

      if(rnd <= 3)
      {
         ReleaseOrig();
         return 0;
      }

      if(rnd <= 6)
      {
         SetState(SingleEnded);
         return (1000 * PotsProtocol::FirstDigitTimeout) + 500;
      }

      auto msg = orig_->CreateMsg(PotsSignal::Digits);
      if(msg == nullptr) return 1000;

      if(rnd <= 9)
      {
         DigitString ds("2000#");
         msg->AddDigits(ds);
         if(!orig_->SendMsg(*msg)) return 0;
         SetState(SingleEnded);
         return 2000;
      }

      auto thr = Singleton< PotsTrafficThread >::Instance();

      if(rnd <= 12)
         dest_ = thr->FindDn(PotsTrafficThread::Unassigned);
      else if(rand(1, 100) <= 80)
         dest_ = thr->FindDn(PotsTrafficThread::Idle);
      else
         dest_ = thr->FindDn(PotsTrafficThread::Assigned);

      if(rnd <= 88)
      {
         DigitString ds(dest_);
         msg->AddDigits(ds);
         if(!orig_->SendMsg(*msg)) return 0;
         SetState(Terminating);
         return 2000;
      }

      DigitString ds(dest_ / 100);
      msg->AddDigits(ds);
      if(!orig_->SendMsg(*msg)) return 0;
      SetState(Dialing);
      return rand(2000, 6000);
   }

   //  We can no longer dial, so digit collection must have timed out.
   //  Clear the call.
   //
   ReleaseOrig();
   return 0;
}

//------------------------------------------------------------------------------

fn_name TrafficCall_ProcessReleasing = "TrafficCall.ProcessReleasing";

msecs_t TrafficCall::ProcessReleasing()
{
   Debug::ft(TrafficCall_ProcessReleasing);

   //  Release whoever is still in the call.
   //
   ReleaseOrig();
   ReleaseTerm();
   return 0;
}

//------------------------------------------------------------------------------

fn_name TrafficCall_ProcessRinging = "TrafficCall.ProcessRinging";

msecs_t TrafficCall::ProcessRinging()
{
   Debug::ft(TrafficCall_ProcessRinging);

   if(!CheckTerm()) return 0;

   //  o Release (12%).
   //  o Release and answer simultaneously (2%).
   //  o Answer and decide what to do after 1 to 20 seconds (86%).
   //
   auto rnd = rand(1, 100);

   if(rnd <= 12)
   {
      ReleaseOrig();
      return 0;
   }

   if(rnd <= 14)
   {
      if(!term_->SendMsg(PotsSignal::Offhook)) return 0;
      ReleaseOrig();
      SetState(SingleEnded);
      return 2000;
   }

   if(!term_->SendMsg(PotsSignal::Offhook)) return 0;
   SetState(Connected);
   return rand(1000, 20000);
}

//------------------------------------------------------------------------------

fn_name TrafficCall_ProcessSingleEnded = "TrafficCall.ProcessSingleEnded";

msecs_t TrafficCall::ProcessSingleEnded()
{
   Debug::ft(TrafficCall_ProcessSingleEnded);

   // o Release after 2 to 6 seconds (90%).
   // o Release after 15 to 75 seconds (10%).
   //
   SetState(Releasing);
   if(rand(1, 100) <= 90) return rand(2000, 6000);
   return rand(15000, 75000);
}

//------------------------------------------------------------------------------

fn_name TrafficCall_ProcessSuspended = "TrafficCall.ProcessSuspended";

msecs_t TrafficCall::ProcessSuspended()
{
   Debug::ft(TrafficCall_ProcessSuspended);

   if(!CheckTerm()) return 0;

   // o Release (70%).
   // o Resume and decide what to do after 2 to 6 seconds (20%).
   // o Release and resume simultaneously (5%).
   // o Time out (5%), which will release the onhook terminator.
   //
   auto rnd = rand(1, 100);

   if(rnd <= 70)
   {
      ReleaseOrig();
      return 0;
   }

   if(rnd <= 90)
   {
      //  If this offhook isn't processed by the POTS call quickly
      //  enough, our call will be released (suspend timeout), and
      //  the offhook may originate a new one.
      //
      if(!term_->SendMsg(PotsSignal::Offhook)) return 0;
      SetState(Connected);
      return rand(2000, 6000);
   }

   if(rnd <= 95)
   {
      if(!term_->SendMsg(PotsSignal::Offhook)) return 0;
      ReleaseOrig();
      SetState(SingleEnded);
      return 2000;
   }

   SetState(SingleEnded);
   EraseTerm();
   return 1000 * PotsProtocol::SuspendTimeout;
}

//------------------------------------------------------------------------------

fn_name TrafficCall_ProcessTerminating = "TrafficCall.ProcessTerminating";

msecs_t TrafficCall::ProcessTerminating()
{
   Debug::ft(TrafficCall_ProcessTerminating);

   auto port = orig_->RxFrom();

   if((port == Tone::Ringback) || (port > Tone::MaxId))
   {
      //  Add the terminator to our call record after verifying that it is,
      //  indeed, a terminator.
      //
      auto reg = Singleton< PotsProfileRegistry >::Instance();
      auto prof = reg->Profile(dest_);
      auto term = prof->GetCircuit();
      auto state = term->GetState();

      if(state == PotsCircuit::Terminator)
      {
         term_ = term;
         term_->SetTrafficId(callid_);
         termStart_ = Clock::TicksNow();
      }
      else
      {
         ReleaseOrig();
         return 0;
      }

      if(port > Tone::MaxId)
      {
         //  We're connected to something other than a tone.  This means
         //  that the terminator has already answered, which occurs when
         //  its offhook planned to originate a call but answered ours
         //  instead--so quickly that we might have never even received
         //  ringback.  Let the call continue for 1 to 20 seconds before
         //  deciding what to do.
         //
         SetState(Connected);
         return rand(1000, 20000);
      }

      //  o Let the call ring for 1 to 36 seconds before deciding what
      //    to do (98%).
      //  o Let the call ring until answer timeout occurs, which will
      //    release the onhook terminator (2%).
      //
      auto rnd = rand(1, 100);

      if(rnd <= 98)
      {
         SetState(Ringing);
         return rand(1000, 36000);
      }

      SetState(SingleEnded);
      EraseTerm();
      return 1000 * PotsProtocol::AnswerTimeout;
   }

   if(port != Tone::Silence)
   {
      //  We're receiving a treatment.  Decide what to do after 2 seconds.
      //
      SetState(SingleEnded);
      return 2000;
   }

   //  We're still receiving silence, so there must be some post-dial delay.
   //  Look for ringback again in 2 seconds.
   //
   return 2000;
}

//------------------------------------------------------------------------------

fn_name TrafficCall_ReleaseOrig = "TrafficCall.ReleaseOrig";

void TrafficCall::ReleaseOrig()
{
   Debug::ft(TrafficCall_ReleaseOrig);

   //  If the originator is in the call, have it send an onhook.
   //  If the terminator is onhook, remove it from the call.
   //
   if(orig_ != nullptr)
   {
      orig_->SendMsg(PotsSignal::Onhook);
      EraseOrig();

      if((term_ != nullptr) && !term_->IsOffhook())
      {
         EraseTerm();
      }
   }
}

//------------------------------------------------------------------------------

fn_name TrafficCall_ReleaseTerm = "TrafficCall.ReleaseTerm";

void TrafficCall::ReleaseTerm()
{
   Debug::ft(TrafficCall_ReleaseTerm);

   //  If the terminator is offhook, have it send an onhook.  If the
   //  originator has released, remove the terminator from the call.
   //
   if(CheckTerm())
   {
      if(term_->IsOffhook())
      {
         term_->SendMsg(PotsSignal::Onhook);
      }

      if(orig_ == nullptr) EraseTerm();
   }
}

//------------------------------------------------------------------------------

fn_name TrafficCall_ResetStateCounts = "TrafficCall.ResetStateCounts";

void TrafficCall::ResetStateCounts()
{
   Debug::ft(TrafficCall_ResetStateCounts);

   for(auto i = 0; i < State_N; ++i) StateCount_[i] = 0;
   CallId_ = 1;
}

//------------------------------------------------------------------------------

fn_name TrafficCall_SetState = "TrafficCall.SetState";

void TrafficCall::SetState(State state)
{
   Debug::ft(TrafficCall_SetState);

   --StateCount_[state_];
   state_ = state;
   ++StateCount_[state_];
}

//------------------------------------------------------------------------------

c_string TrafficCall::strState(State state)
{
   if((state >= 0) && (state < State_N)) return TrafficStateStr[state];
   return TrafficStateStr[State_N];
}

//==============================================================================

const msecs_t PotsTrafficThread::MsecsPerTick = 100;
const secs_t PotsTrafficThread::MaxDelaySecs = 120;
const size_t PotsTrafficThread::NumOfSlots =
   1000 * PotsTrafficThread::MaxDelaySecs / PotsTrafficThread::MsecsPerTick + 1;
const Address::DN PotsTrafficThread::StartDN = 30001;
const secs_t PotsTrafficThread::HoldingTimeSecs = 35;
const size_t PotsTrafficThread::DNsPer100Calls = 165;
const size_t PotsTrafficThread::MaxCallsPerMin =
   (Address::LastDN - PotsTrafficThread::StartDN + 1) *  // number of DNs
   (6000 / PotsTrafficThread::HoldingTimeSecs) /         // 100 * calls/DN/min
   (3 * PotsTrafficThread::DNsPer100Calls / 2);          // 100 * DNs/call + 50%

//------------------------------------------------------------------------------

fn_name PotsTrafficThread_ctor = "PotsTrafficThread.ctor";

PotsTrafficThread::PotsTrafficThread() : Thread(LoadTestFaction),
   timeout_(TIMEOUT_NEVER),
   callsPerMin_(0),
   maxCallsPerTick_(0),
   milCallsPerTick_(0),
   firstDN_(Address::NilDN),
   lastDN_(Address::NilDN),
   currSlot_(0),
   totalCalls_(0),
   activeCalls_(0),
   totalTimes_(0),
   totalReports_(0),
   overflows_(0),
   aborts_(0),
   timewheel_(nullptr)
{
   Debug::ft(PotsTrafficThread_ctor);

   auto size = sizeof(Q1Way< TrafficCall >) * NumOfSlots;
   timewheel_ = (Q1Way< TrafficCall >*) Memory::Alloc(size, MemDynamic);

   for(auto i = 0; i < NumOfSlots; ++i)
   {
      new (&timewheel_[i]) Q1Way< TrafficCall >();
      timewheel_[i].Init(TrafficCall::LinkDiff());
   }

   SetInitialized();
}

//------------------------------------------------------------------------------

fn_name PotsTrafficThread_dtor = "PotsTrafficThread.dtor";

PotsTrafficThread::~PotsTrafficThread()
{
   Debug::ft(PotsTrafficThread_dtor);

   //  Don't clean up during a cold restart.  Every circuit will try to
   //  send a final message, which causes a flood of logs because the
   //  POTS shelf socket has already been freed.
   //
   if(Restart::GetLevel() < RestartCold)
   {
      if(timewheel_ != nullptr)
      {
         for(size_t i = 0; i < NumOfSlots; ++i)
         {
            timewheel_[i].Purge();
         }

         Memory::Free(timewheel_);
         timewheel_ = nullptr;
      }
   }

   TrafficCall::ResetStateCounts();
}

//------------------------------------------------------------------------------

c_string PotsTrafficThread::AbbrName() const
{
   return "traffic";
}

//------------------------------------------------------------------------------

fn_name PotsTrafficThread_Destroy = "PotsTrafficThread.Destroy";

void PotsTrafficThread::Destroy()
{
   Debug::ft(PotsTrafficThread_Destroy);

   Singleton< PotsTrafficThread >::Destroy();
}

//------------------------------------------------------------------------------

void PotsTrafficThread::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Thread::Display(stream, prefix, options);

   stream << prefix << "NumOfSlots      : " << NumOfSlots << CRLF;
   stream << prefix << "MaxCallsPerMin  : " << MaxCallsPerMin << CRLF;
   stream << prefix << "timeout         : " << timeout_ << CRLF;
   stream << prefix << "callsPerMin     : " << callsPerMin_ << CRLF;
   stream << prefix << "maxCallsPerTick : " << maxCallsPerTick_ << CRLF;
   stream << prefix << "milCallsPerTick : " << milCallsPerTick_ << CRLF;
   stream << prefix << "firstDN         : " << firstDN_ << CRLF;
   stream << prefix << "lastDN          : " << lastDN_ << CRLF;
   stream << prefix << "currSlot        : " << currSlot_ << CRLF;
   stream << prefix << "totalCalls      : " << totalCalls_ << CRLF;
   stream << prefix << "activeCalls     : " << activeCalls_ << CRLF;
   stream << prefix << "totalTimes      : " << totalTimes_ << CRLF;
   stream << prefix << "totalReports    : " << totalReports_ << CRLF;
   stream << prefix << "overflows       : " << overflows_ << CRLF;
   stream << prefix << "aborts          : " << aborts_ << CRLF;
}

//------------------------------------------------------------------------------

void PotsTrafficThread::DisplayStateCounts(ostream& stream,
   const string& prefix)
{
   TrafficCall::DisplayStateCounts(stream, prefix);
}

//------------------------------------------------------------------------------

fn_name PotsTrafficThread_Enqueue = "PotsTrafficThread.Enqueue";

void PotsTrafficThread::Enqueue(TrafficCall& call, msecs_t delay)
{
   Debug::ft(PotsTrafficThread_Enqueue);

   if((delay == 0) || call.Empty())
   {
      delete &call;
      --activeCalls_;
      return;
   }

   size_t incr = delay / MsecsPerTick;
   if(incr == 0) incr = 1;
   if(incr >= NumOfSlots) incr = NumOfSlots - 1;
   auto nextSlot = currSlot_ + incr;
   if(nextSlot >= NumOfSlots) nextSlot -= NumOfSlots;
   timewheel_[nextSlot].Enq(call);
}

//------------------------------------------------------------------------------

fn_name PotsTrafficThread_Enter = "PotsTrafficThread.Enter";

void PotsTrafficThread::Enter()
{
   Debug::ft(PotsTrafficThread_Enter);

   auto sleep = timeout_;

   while(true)
   {
      auto rc = Pause(sleep);

      switch(rc)
      {
      case DelayInterrupted:
         //
         //  Our call rate has been modified and we have work to do.
         //
         timeout_ = MsecsPerTick;
         break;

      case DelayCompleted:
         SendMessages();

         if((callsPerMin_ == 0) && (activeCalls_ == 0))
         {
            //  Release the resources that were allocated to run traffic
            //  and sleep until more traffic is to be generated.
            //
            Takedown();
            timeout_ = TIMEOUT_NEVER;
            sleep = timeout_;
         }
         break;

      default:
         Debug::SwLog(PotsTrafficThread_Enter, "unexpected result", rc);
      }

      //  Unless we're supposed to sleep forever, adjust our sleep
      //  time to account for how long we just ran.
      //
      if(timeout_ != TIMEOUT_NEVER)
      {
         auto runTime = MsecsSinceStart();
         sleep = (runTime > timeout_ ? 0 : timeout_ - runTime);
      }
   }
}

//------------------------------------------------------------------------------

fn_name PotsTrafficThread_ExitOnRestart = "PotsTrafficThread.ExitOnRestart";

bool PotsTrafficThread::ExitOnRestart(RestartLevel level) const
{
   Debug::ft(PotsTrafficThread_ExitOnRestart);

   //  Calls survive warm restarts, so continue to generate traffic when
   //  the restart ends.  Exit during other restarts.
   //
   return (level >= RestartCold);
}

//------------------------------------------------------------------------------

fn_name PotsTrafficThread_FindDn = "PotsTrafficThread.FindDn";

Address::DN PotsTrafficThread::FindDn(DnStatus status) const
{
   Debug::ft(PotsTrafficThread_FindDn);

   Address::DN dn;
   auto reg = Singleton< PotsProfileRegistry >::Instance();

   switch(status)
   {
   case Unassigned:
      return firstDN_ - 1;

   case Assigned:
      return rand(firstDN_, lastDN_);

   case Idle:
      dn = rand(firstDN_, lastDN_);

      for(auto n = lastDN_ - firstDN_ + 1; n > 0; --n)
      {
         auto prof = reg->Profile(dn);
         auto cct = prof->GetCircuit();
         if(cct->GetState() == PotsCircuit::Idle) return dn;
         dn = (dn == lastDN_ ? firstDN_ : dn + 1);
      }
      break;

   case Busy:
      dn = rand(firstDN_, lastDN_);

      for(auto n = lastDN_ - firstDN_ + 1; n > 0; --n)
      {
         auto prof = reg->Profile(dn);
         auto cct = prof->GetCircuit();
         if(cct->GetState() != PotsCircuit::Idle) return dn;
         dn = (dn == lastDN_ ? firstDN_ : dn + 1);
      }
      break;

   default:
      Debug::SwLog(PotsTrafficThread_FindDn, "unexpected status", status);
   }

   return Address::NilDN;
}

//------------------------------------------------------------------------------

fn_name PotsTrafficThread_InitialMsecs = "PotsTrafficThread.InitialMsecs";

msecs_t PotsTrafficThread::InitialMsecs() const
{
   Debug::ft(PotsTrafficThread_InitialMsecs);

   return Thread::InitialMsecs() << 4;
}

//------------------------------------------------------------------------------

fn_name PotsTrafficThread_Query = "PotsTrafficThread.Query";

void PotsTrafficThread::Query(ostream& stream) const
{
   Debug::ft(PotsTrafficThread_Query);

   stream << "Number of timewheel slots    " << NumOfSlots << CRLF;

   stream << "Timewheel interval (msecs)   ";
   if(timeout_ == TIMEOUT_NEVER)
      stream << "infinite";
   else
      stream << timeout_;
   stream << CRLF;

   stream << "Maximum calls per minute     " << MaxCallsPerMin << CRLF;
   stream << "Traffic rate (calls/min)     " << callsPerMin_ << CRLF;
   stream << "Maximum calls per tick       " << maxCallsPerTick_ << CRLF;
   stream << "Millicalls per tick          " << milCallsPerTick_ << CRLF;
   stream << "First DN added for traffic   " << firstDN_ << CRLF;
   stream << "Last DN added for traffic    " << lastDN_ << CRLF;
   stream << "Current timeslot             " << currSlot_ << CRLF;
   stream << "Total calls created          " << totalCalls_ << CRLF;
   stream << "Number of active calls       " << activeCalls_ << CRLF;
   stream << "Number of DN overflows       " << overflows_ << CRLF;
   stream << "Number of calls aborted      " << aborts_ << CRLF;
   stream << "Total holding time reports   " << totalReports_ << CRLF;

   if((totalReports_ > 0) && (totalCalls_ > 0))
   {
      auto htsecs = totalTimes_ / totalReports_;
      stream << "Average holding time (secs)  " << htsecs;
      if(htsecs > HoldingTimeSecs) stream << " ***";
      stream << CRLF;

      auto hdnspc = 100 * totalReports_ / totalCalls_;
      stream << "Average DNs/call * 100       " << hdnspc;
      if(hdnspc > DNsPer100Calls) stream << " ***";
      stream << CRLF;
   }

   if(activeCalls_ > 0)
   {
      stream << "First call after current timeslot:" << CRLF;

      auto lead = spaces(4);
      auto i = (currSlot_ + 1) % NumOfSlots;

      while(i != currSlot_)
      {
         auto c = timewheel_[i].First();

         if(c != nullptr)
         {
            stream << spaces(2) << strIndex(i) << CRLF;
            c->Display(stream, lead, NoFlags);
            return;
         }

         i = (i + 1) % NumOfSlots;
      }
   }
}

//------------------------------------------------------------------------------

void PotsTrafficThread::RecordHoldingTime(secs_t secs)
{
   totalTimes_ += secs;
   ++totalReports_;
}

//------------------------------------------------------------------------------

fn_name PotsTrafficThread_SendMessages = "PotsTrafficThread.SendMessages";

void PotsTrafficThread::SendMessages()
{
   Debug::ft(PotsTrafficThread_SendMessages);

   auto& slot = timewheel_[currSlot_];

   //  Create new calls unless we've been told to stop.
   //
   auto stop = (callsPerMin_ == 0);

   if(!stop)
   {
      auto reg = Singleton< PotsProfileRegistry >::Instance();
      auto n = rand(0, maxCallsPerTick_);
      if(rand(0, 999) < milCallsPerTick_) ++n;

      for(size_t i = 0; i < n; ++i)
      {
         auto dn = FindDn(Idle);

         if(dn != Address::NilDN)
         {
            auto prof = reg->Profile(dn);
            auto call = new TrafficCall(*prof->GetCircuit());
            ++totalCalls_;
            ++activeCalls_;
            Enqueue(*call, call->Originate());
         }
         else
         {
            overflows_ += (n - i);
            break;
         }
      }
   }

   //  Notify the existing calls that wanted to progress in this timeslot.
   //
   for(auto call = slot.Deq(); call != nullptr; call = slot.Deq())
   {
      Enqueue(*call, call->Advance());
   }

   currSlot_ = (currSlot_ + 1) % NumOfSlots;
}

//------------------------------------------------------------------------------

fn_name PotsTrafficThread_SetRate = "PotsTrafficThread.SetRate";

void PotsTrafficThread::SetRate(word rate)
{
   Debug::ft(PotsTrafficThread_SetRate);

   if(rate > callsPerMin_)
   {
      //  Add N more circuits, with a minimum of 20, starting at DN.
      //
      size_t n = 20;

      if(rate > 10)
      {
         n = (rate * HoldingTimeSecs / 60) * (3 * DNsPer100Calls / 200);
      }

      auto dn = StartDN;

      if(lastDN_ != Address::NilDN)
      {
         n -= (lastDN_ - firstDN_ + 1);
         dn = lastDN_ + 1;
      }
      else
      {
         firstDN_ = StartDN;
      }

      FunctionGuard guard(Guard_MemUnprotect);
      auto reg = Singleton< PotsProfileRegistry >::Instance();

      for(auto i = 0; i < n; ++i)
      {
         if(reg->Profile(dn) == nullptr)
         {
            new PotsProfile(dn);
         }

         lastDN_ = dn;

         if((++dn & 0x0f) == 0) Thread::PauseOver(90);
      }
   }

   //  Calculate the number of calls to generate per interval
   //  and wake our thread if it is sleeping forever.
   //
   auto wakeup = (callsPerMin_ == 0);

   callsPerMin_ = rate;

   if(callsPerMin_ > 0)
   {
      size_t ticksPerMin = 60000 / MsecsPerTick;
      size_t CallsPerTick1000 = (1000 * rate) / ticksPerMin;

      milCallsPerTick_ = CallsPerTick1000 % 1000;
      maxCallsPerTick_ = (CallsPerTick1000 / 1000) << 1;

      if(wakeup) Interrupt();
   }

   auto log = Log::Create(PotsLogGroup, PotsTrafficRate);
   if(log == nullptr) return;
   *log << Log::Tab << "rate=" << rate;
   Log::Submit(log);
}

//------------------------------------------------------------------------------

fn_name PotsTrafficThread_Takedown = "PotsTrafficThread.Takedown";

void PotsTrafficThread::Takedown()
{
   Debug::ft(PotsTrafficThread_Takedown);

   //  Deregister the DNs that we created, pausing after each group of 100.
   //  Although we're finished, calls are still in the process of clearing,
   //  and a call traps if we delete a user profile that it is still using.
   //
   auto contexts = Singleton< ContextPool >::Instance();
   auto curr = contexts->InUseCount();
   auto count = 60;

   while(count > 0)
   {
      auto prev = curr;
      Pause(1000);
      curr = contexts->InUseCount();
      if(curr == 0) break;
      if(curr == prev) --count;
   }

   FunctionGuard guard(Guard_MemUnprotect);

   auto reg = Singleton< PotsProfileRegistry >::Instance();

   for(auto dn = firstDN_; dn <= lastDN_; ++dn)
   {
      auto prof = reg->Profile(dn);

      if(prof != nullptr) prof->Deregister();

      if((dn & 0x0f) == 0) Thread::PauseOver(90);
   }

   firstDN_ = Address::NilDN;
   lastDN_ = Address::NilDN;

   Singleton< TrafficCallPool >::Destroy();
}
}
