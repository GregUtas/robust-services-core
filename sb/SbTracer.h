//==============================================================================
//
//  SbTracer.h
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
#ifndef SBTRACER_H_INCLUDED
#define SBTRACER_H_INCLUDED

#include "Permanent.h"
#include <cstddef>
#include <iosfwd>
#include "Factory.h"
#include "NbTypes.h"
#include "Protocol.h"
#include "SbTypes.h"
#include "Service.h"
#include "SysTypes.h"
#include "ToolTypes.h"

//------------------------------------------------------------------------------
//
//  Interface for controlling SessionBase trace tools:
//  o TransTracer records the processing of incoming messages (at I/O level)
//    and the processing of those messages at task level, within a context
//    (by an SSM, PSM, or Factory).  Its primary purpose is to measure the
//    real-time cost of each message in order to support capacity models.
//  o BufferTracer records entire messages.
//  o ContextTracer records events inside a context.  These include the
//    creation and deletion of SSMs and PSMs, the sending and receiving of
//    messages, and the invocation of event handlers (state-event pairs).
//
//  Users control these tools through the SessionBase CLI increment (see
//  SbIncrement.cpp).  For detailed debugging, a common usage is
//
//  >set tools fb on    // enable FunctionTracer and BufferTracer
//  >include all on     // capture all activity
//  >start              // start tracing
//  run some scenario
//  >stop               // stop tracing
//  >save trace <fn>    // display function calls/messages in "<fn>.trace.txt"
//
//  For higher level debugging, performance analysis, or training purposes,
//  more useful is
//
//  >set tools tc on    // enable TransTracer and ContextTracer
//  >include all on
//  >start
//  run some scenario
//  >stop
//  >save trace <fn>    // display transactions/events in "<fn>.trace.txt"
//  >sb                 // enter SessionBase CLI increment
//  >save msc <fn>      // create a message sequence chart in "<fn>.msc.html"
//
//  A subset of what was captured can be displayed by turning off the tools
//  whose events should not be included, prior to issuing the >save command.
//  For example:
//
//  >set tools fbtc on  // turn on multiple tools
//  >include all on
//  >start
//  run some scenario
//  >stop
//  >set tools tc off   // disable TransTracer and ContextTracer
//  >save trace <fn>    // same as first output above
//  >set tools fb off   // disable BufferTracer and FunctionTracer
//  >set tools tc on    // re-enable TransTracer and ContextTracer
//  >save trace <fn>    // same as second output above
//
//  SbTracer supports focused tracing of the following SessionBase entities:
//  o specific factories
//  o specific protocols
//  o specific signals
//  o specific services
//
//  Any one of these entities can be specifically included or excluded from
//  the trace.  When a message is received or sent, the function MsgStatus
//  is invoked to see if it should be traced.  This is how the tracing of IP
//  addresses, IP ports, factories, protocols, and signals is triggered.  The
//  logic for MsgStatus is
//
//  1. Check inclusion/exclusion for the message's signal.
//  2. Check inclusion/exclusion for the message's IP address.
//  3. Check inclusion/exclusion for the message's IP port.
//  4. Check inclusion/exclusion for the message's factory.
//  5. Check inclusion/exclusion for the running thread.
//  6. Check inclusion/exclusion for the running thread's faction.
//  7. Check if all activity is included.
//
//  The function ServiceIsTraced is invoked when an SSM is created, so that
//  this can also trigger tracing.  The logic of ServiceIsTraced is similar:
//
//  1. Check inclusion/exclusion for the SSM's service.
//  2. Check if all activity is included.
//
namespace SessionBase
{
class SbTracer : public NodeBase::Permanent
{
   friend class NodeBase::Singleton< SbTracer >;
public:
   //  Traces FID according to STATUS.
   //
   NodeBase::TraceRc SelectFactory(FactoryId fid, NodeBase::TraceStatus status);

   //  Traces PRID according to STATUS.
   //
   NodeBase::TraceRc SelectProtocol
      (ProtocolId prid, NodeBase::TraceStatus status);

   //  Traces SID, which belongs to PRID, according to STATUS.
   //
   NodeBase::TraceRc SelectSignal
      (ProtocolId prid, SignalId sid, NodeBase::TraceStatus status);

   //  Traces SID according to STATUS.
   //
   NodeBase::TraceRc SelectService(ServiceId sid, NodeBase::TraceStatus status);

   //  Traces the timer registry according to STATUS.
   //
   NodeBase::TraceRc SelectTimers(NodeBase::TraceStatus status);

   //  Displays, in STREAM, everything that has been included or excluded.
   //
   void QuerySelections(std::ostream& stream) const;

   //  Removes everything of type FILTER that has been included or excluded.
   //
   NodeBase::TraceRc ClearSelections(NodeBase::FlagId filter);

   //  Determines whether MSG, travelling in DIR, should be traced.
   //
   NodeBase::TraceStatus MsgStatus
      (const Message& msg, NodeBase::MsgDirection dir) const;

   //  Returns true if SID should be traced.
   //
   bool ServiceIsTraced(ServiceId sid) const;

   //  Determines if timer threads should be traced.
   //
   NodeBase::TraceStatus TimersStatus() const { return timers_; }

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
private:
   //  Private because this singleton is not subclassed.
   //
   SbTracer();

   //  Private because this singleton is not subclassed.
   //
   ~SbTracer();

   //> The number of signals that can be specifically included or excluded
   //  from a trace.
   //
   static const size_t MaxSignalEntries = 8;

   //  The trace status of a signal.
   //
   struct SignalFilter
   {
      SignalFilter();
      SignalFilter(ProtocolId p, SignalId s, NodeBase::TraceStatus ts);

      ProtocolId prid;     // protocol identifier
      SignalId sid;        // signal identifier
      NodeBase::TraceStatus status;  // whether included or excluded
   };

   //  If PRID/SID is included or excluded, returns its index in signals_[].
   //  Returns -1 if PRID/SID has not been included or excluded.
   //
   int FindSignal(ProtocolId prid, SignalId sid) const;

   //  Returns the trace status of PRID/SID.
   //
   NodeBase::TraceStatus SignalStatus(ProtocolId prid, SignalId sid) const;

   //  Returns true if no factories are included or excluded.
   //
   bool FactoriesEmpty() const;

   //  Returns true if no protocols are included or excluded.
   //
   bool ProtocolsEmpty() const;

   //  Returns true if no signals are included or excluded.
   //
   bool SignalsEmpty() const;

   //  Returns true if no services are included or excluded.
   //
   bool ServicesEmpty() const;

   //  Whether a specific factory is included or excluded.
   //
   NodeBase::TraceStatus factories_[Factory::MaxId + 1];

   //  Whether a specific protocol is included or excluded.
   //
   NodeBase::TraceStatus protocols_[Protocol::MaxId + 1];

   //  A list of included or excluded signals.
   //
   SignalFilter signals_[MaxSignalEntries];

   //  Whether a specific service is included or excluded.
   //
   NodeBase::TraceStatus services_[Service::MaxId + 1];

   //  Whether times are included or excluded.
   //
   NodeBase::TraceStatus timers_;
};
}
#endif
