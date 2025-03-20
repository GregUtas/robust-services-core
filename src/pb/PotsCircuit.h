//==============================================================================
//
//  PotsCircuit.h
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
#ifndef POTSCIRCUIT_H_INCLUDED
#define POTSCIRCUIT_H_INCLUDED

#include "Circuit.h"
#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <string>
#include "BcCause.h"
#include "PotsProtocol.h"
#include "SbAppIds.h"
#include "SysTypes.h"

namespace PotsBase
{
   class PotsProfile;
}

using namespace MediaBase;
using namespace CallBase;
using namespace NodeBase;
using namespace SessionBase;

//------------------------------------------------------------------------------

namespace PotsBase
{
//  Apart from profile_, this mimics the application interface to low-level
//  software that controls a POTS circuit.
//
class PotsCircuit : public Circuit
{
public:
   //  The circuit's overall state.
   //
   enum State
   {
      Idle,        // received Release
      Active,      // sent a message when Idle
      Originator,  // digit collection started when Active
      Terminator,  // received Supervise when Idle or ringing when Active
      LockedOut,   // received Lockout
      State_N      // number of states
   };

   //  Creates a circuit associated with PROFILE.
   //
   explicit PotsCircuit(PotsProfile& profile);

   //  Not subclassed.
   //
   ~PotsCircuit();

   //  Returns the profile associated with the circuit.
   //
   PotsProfile* Profile() const { return profile_; }

   //  Returns the circuit's state.
   //
   State GetState() const { return state_; }

   //  Returns true if the circuit is offhook.
   //
   bool IsOffhook() const { return offhook_; }

   //  Returns true if digits from the circuit will be reported.
   //
   bool CanDial() const { return digits_; }

   //  Returns true if the circuit is ringing.
   //
   bool IsRinging() const { return ringing_; }

   //  Returns true if a flash from the circuit will be reported.
   //
   bool CanFlash() const { return flash_; }

   //  Returns the reason that the call was cleared.  This is
   //  only available until the circuit enters the Idle state.
   //
   Cause::Ind GetCause() const { return cause_; }

   //  Returns a non-zero value if the circuit is involved in a traffic call.
   //
   size_t GetTrafficId() const { return trafficId_; }

   //  Called when the circuit is added to a traffic call.
   //
   void SetTrafficId(size_t tid) { trafficId_ = tid; }

   //  Called when the circuit is removed from a traffic call.
   //
   void ClearTrafficId(size_t tid);

   //  Invoked when MSG is sent to the circuit.
   //
   void ReceiveMsg(const Pots_NU_Message& msg);

   //  Creates a message with the signal SID from the circuit.
   //
   Pots_UN_Message* CreateMsg(PotsSignal::Id sid) const;

   //  Sends a message with the signal SID from the circuit.
   //
   bool SendMsg(PotsSignal::Id sid);

   //  Sends MSG from the circuit.
   //
   bool SendMsg(Pots_UN_Message& msg);

   //  Resets the circuit to its initial state.  Used during testing.
   //
   void ResetCircuit();

   //  Displays the number of circuits in each state.
   //
   static void DisplayStateCounts
      (std::ostream& stream, const std::string& prefix);

   //  Returns a string summarizing the circuit's state.  Used for logs.
   //
   std::string strState() const;

   //  Resets the number of circuits in each state during a restart.
   //
   static void ResetStateCounts(RestartLevel level);

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden to return a string that identifies the circuit.
   //
   std::string Name() const override;

   //  Overridden to indicate that the circuit supports the POTS protocol.
   //
   bool Supports(ProtocolId prid) const override
      { return (prid == PotsProtocolId); }
private:
   //  The size of the trace buffer, which maintains a message history
   //  that is included in logs.
   //
   static const size_t TraceSize = 16;

   //  The contents of each trace.
   //
   struct SignalEntry
   {
      uint8_t signal : 4;  // the signal received or sent
      bool digsOn : 1;     // set if a Supervise started digit collection
      bool digsOff : 1;    // set if a Supervise stopped digit collection
      bool ringOn : 1;     // set if a Supervise started ringing
      bool ringOff : 1;    // set if a Supervise stopped ringing
   };

   //  A template for initializing a SignalEntry.
   //
   static const SignalEntry NilSignalEntry;

   //  Updates the circuit's state.
   //
   void SetState(State state);

   //  Adds a trace entry.
   //
   void Trace(const SignalEntry& entry);

   //  Returns a string summarizing the circuit's trace.  Used for logs.
   //
   std::string strTrace() const;

   //  The circuit's state.
   //
   State state_;

   //  Set if the circuit is offhook.
   //
   bool offhook_;

   //  Set if ringing is being applied to the circuit.
   //
   bool ringing_;

   //  Set if the circuit should report dialed digits.
   //
   bool digits_;

   //  Set if the circuit should report a flash.
   //
   bool flash_;

   //  The reason that the call is being released.  It is set by a
   //  Supervise that releases the call, and cleared by the Release.
   //
   Cause::Ind cause_;

   //  The profile associated with the circuit.
   //
   PotsProfile* const profile_;

   //  Identifies (if non-zero) the traffic call that is using the circuit.
   //
   size_t trafficId_;

   //  The current index into the trace buffer, which wraps around.
   //
   size_t buffIndex_;

   //  The trace buffer.
   //
   SignalEntry trace_[TraceSize];
};
}
#endif
