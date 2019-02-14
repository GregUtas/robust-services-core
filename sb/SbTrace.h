//==============================================================================
//
//  SbTrace.h
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
#ifndef SBTRACE_H_INCLUDED
#define SBTRACE_H_INCLUDED

#include "TimedRecord.h"
#include <cstddef>
#include <iosfwd>
#include <string>
#include "Clock.h"
#include "EventHandler.h"
#include "LocalAddress.h"
#include "Message.h"
#include "NbTypes.h"
#include "SbTypes.h"
#include "SysTypes.h"

using namespace NodeBase;

//------------------------------------------------------------------------------
//
//  Trace records for SessionBase objects.
//
namespace SessionBase
{
//  Records a transaction.
//
class TransTrace : public TimedRecord
{
public:
   //  Types of transaction trace records.
   //
   static const Id RxNet = 1;  // incoming external message (I/O thread)
   static const Id Trans = 2;  // transaction (invoker thread)

   //  Creates an RxNet trace for MSG, which is being received by FAC.
   //
   TransTrace(const Message& msg, const Factory& fac);

   //  Creates a Trans trace for MSG, which is being processed by CTX and INV.
   //
   TransTrace(const Context& ctx, const Message& msg, const InvokerThread* inv);

   //  When a trace tool starts its work, it calls Clock::TicksNow()
   //  to obtain the current clock time in ticks.  When it finishes its
   //  work, it calls this function so that the time used by the tool
   //  can be excluded from the cost of the current transaction.  THEN
   //  was the value obtained from TicksNow.
   //
   void ResumeTime(const ticks_t& then);

   //  Called to set the context once it is known.
   //
   void SetContext(const void* ctx);

   //  Called to set the root SSM's ServiceId once it is allocated.
   //
   void SetService(ServiceId sid);

   //  Called at the end of an RxNet transaction to finalize its cost.
   //
   void EndOfTransaction();

   //  For accessing information captured by this record.
   //
   const void* Rcvr() const { return rcvr_; }
   id_t Cid() const { return cid_; }
   bool Service() const { return service_; }
   const void* Buff() const { return buff_; }
   ContextType Type() const { return type_; }

   //  Overridden to display the trace record.
   //
   virtual bool Display(std::ostream& stream, bool diff) override;
private:
   //  Overridden to return a string for displaying this type of record.
   //
   virtual const char* EventString() const override;

   //  The object (context or factory) that received the message.  The raw
   //  pointer correlates transactions processed by the same context.  If the
   //  recipient was a context, it has probably been deleted.
   //
   const void* rcvr_;

   //  The IP buffer associated with the message.  The raw pointer correlates
   //  a message received by an I/O thread with its eventual processing by an
   //  invoker thread.  The IP buffer itself will have been deleted.
   //
   const void* const buff_;

   //  The time when the transaction began.
   //
   ticks_t ticks0_;

   //  The time when the transaction ended.
   //
   ticks_t ticks1_;

   //  The FactoryId (or ServiceId, if known) involved in the transaction.
   //
   id_t cid_;

   //  Set if cid_ is a ServiceId.
   //
   bool service_;

   //  The type of context that handled the transaction.
   //
   const ContextType type_;

   //  The incoming message's priority.
   //
   const Message::Priority prio_;

   //  The incoming message's protocol.
   //
   const ProtocolId prid_;

   //  The incoming message's signal.
   //
   const SignalId sid_;
};

//------------------------------------------------------------------------------
//
//  Records an entire incoming or outgoing message.
//
class BuffTrace : public TimedRecord
{
public:
   //  Types of buffer trace records.
   //
   static const Id IcMsg = 1;  // incoming message
   static const Id OgMsg = 2;  // outgoing message

   //  Creates an SbIpBuffer trace for BUFF, travelling in the direction
   //  specified by RID.
   //
   BuffTrace(Id rid, const SbIpBuffer& buff);

   //  Releases buff_.  Not subclassed.
   //
   ~BuffTrace();

   //  Starting at BT, finds the next message with SID that was received
   //  by FID.  Updates SKIP with information about signals (if any) that
   //  matched FID but that were skipped before also matching SID.
   //
   static BuffTrace* NextIcMsg
      (BuffTrace* bt, FactoryId fid, SignalId sid, SkipInfo& skip);

   //  Returns the message's header.
   //
   MsgHeader* Header() const;

   //  Reconstructs a full message from the message trace record.
   //
   Message* Rewrap();

   //  Overridden to display the trace record.
   //
   virtual bool Display(std::ostream& stream, bool diff) override;
private:
   //  For an incoming (outgoing) message, returns the identifier of the
   //  factory that received (sent) the message.
   //
   FactoryId ActiveFid() const;

   //  Overridden to claim buff_.
   //
   virtual void ClaimBlocks() override;

   //  Overridden to return a string for displaying this type of record.
   //
   virtual const char* EventString() const override;

   //  Overridden to nullify the record if buff_ will vanish.
   //
   virtual void Shutdown(RestartLevel level) override;

   //  Deleted to prohibit copying.
   //
   BuffTrace(const BuffTrace& that) = delete;
   BuffTrace& operator=(const BuffTrace& that) = delete;

   //  A clone of the buffer being captured.
   //
   SbIpBuffer* buff_;

   //  Set when Rewrap is invoked, after which NextIcMsg will skip the buffer.
   //
   bool verified_;

   //  Set if the buffer caused a trap.
   //
   bool corrupt_;
};

//------------------------------------------------------------------------------
//
//  Records an event for a SessionBase object.
//
class SboTrace : public TimedRecord
{
public:
   //  Virtual to allow subclassing.
   //
   virtual ~SboTrace() = default;

   //  Overridden to display the trace record.
   //
   virtual bool Display(std::ostream& stream, bool diff) override;
protected:
   //  Creates a trace record for SBO, with the SIZE specified.
   //  Protected because this class is virtual.
   //
   SboTrace(size_t size, const Pooled& sbo);

   //  Displays ID in the trace's identifier column, preceded by LABEL.
   //  If ID is NIL_ID, it is not displayed.  Enough spaces to reach the
   //  description column are then inserted after the output.
   //
   static std::string OutputId(const std::string& label, id_t id);
private:
   //  The object associated with this trace record.  By the time the
   //  record is displayed, the object will have been deleted, so only
   //  the value of the pointer itself ("this") should be used.
   //
   const Pooled* const sbo_;
};

//------------------------------------------------------------------------------
//
//  Records the creation or deletion of an SSM.
//
class SsmTrace : public SboTrace
{
public:
   //  Types of SSM trace records.
   //
   static const Id Creation = 1;
   static const Id Deletion = 2;

   //  Creates a trace record for SSM, with a record type of RID.
   //
   SsmTrace(Id rid, const ServiceSM& ssm);

   //  Overridden to display the trace record.
   //
   virtual bool Display(std::ostream& stream, bool diff) override;
private:
   //  Overridden to return a string for displaying this type of record.
   //
   virtual const char* EventString() const override;

   //  The service whose SSM was created or deleted.
   //
   const ServiceId sid_;
};

//------------------------------------------------------------------------------
//
//  Records the creation or deletion of a PSM.
//
class PsmTrace : public SboTrace
{
public:
   //  Types of PSM trace records.
   //
   static const Id Creation = 3;
   static const Id Deletion = 4;

   //  Creates a trace record for PSM, with a record type of RID.
   //
   PsmTrace(Id rid, const ProtocolSM& psm);

   //  Overridden to display the trace record.
   //
   virtual bool Display(std::ostream& stream, bool diff) override;
private:
   //  Overridden to return a string for displaying this type of record.
   //
   virtual const char* EventString() const override;

   //  The factory associated with the PSM.
   //
   const FactoryId fid_;

   //  The object block identifier of the PSM's port, if known.
   //
   PooledObjectId bid_;
};

//------------------------------------------------------------------------------
//
//  Records the creation or deletion of a message port.
//
class PortTrace : public SboTrace
{
public:
   //  Types of port trace records.
   //
   static const Id Creation = 5;
   static const Id Deletion = 6;

   //  Creates a trace record for PORT, with a record type of RID.
   //
   PortTrace(Id rid, const MsgPort& port);

   //  Overridden to display the trace record.
   //
   virtual bool Display(std::ostream& stream, bool diff) override;
private:
   //  Overridden to return a string for displaying this type of record.
   //
   virtual const char* EventString() const override;

   //  The factory associated with the port.
   //
   const FactoryId fid_;

   //  The port's object block identifier.
   //
   const PooledObjectId bid_;
};

//------------------------------------------------------------------------------
//
//  Records an incoming or outgoing message's protocol and signal.
//
class MsgTrace : public SboTrace
{
   friend class TransTrace;
public:
   //  Types of message trace records.
   //
   static const Id Creation     = 7;
   static const Id Deletion     = 8;
   static const Id Reception    = 9;   // incoming message
   static const Id Transmission = 10;  // outgoing message

   //  Creates a trace record for MSG, with a record type of RID.
   //
   MsgTrace(Id rid, const Message& msg, Message::Route route);

   //  For accessing information captured by this record.
   //
   ProtocolId Prid() const { return prid_; }
   SignalId Sid() const { return sid_; }
   const LocalAddress& LocAddr() const { return locAddr_; }
   const LocalAddress& RemAddr() const { return remAddr_; }
   Message::Route Route() const { return route_; }
   bool NoCtx() const { return noCtx_; }
   bool Self() const { return self_; }

   //  Overridden to display the trace record.
   //
   virtual bool Display(std::ostream& stream, bool diff) override;
private:
   //  Overridden to return a string for displaying this type of record.
   //
   virtual const char* EventString() const override;

   //  The message's protocol.
   //
   const ProtocolId prid_;

   //  The message's signal.
   //
   const SignalId sid_;

   //  The local address.
   //
   LocalAddress locAddr_;

   //  The remote address.
   //
   LocalAddress remAddr_;

   //  The route that the message took.
   //
   const Message::Route route_;

   //  Set if the message was not sent by a context.
   //
   const bool noCtx_;

   //  Set if the sender sent the message to itself.
   //
   const bool self_;
};

//------------------------------------------------------------------------------
//
//  Records the creation or deletion of a timer.
//
class TimerTrace : public SboTrace
{
public:
   //  Types of timer trace records.
   //
   static const Id Creation = 11;
   static const Id Deletion = 12;

   //  Creates a trace record for TMR, with a record type of RID.
   //
   TimerTrace(Id rid, const Timer& tmr);

   //  Overridden to display the trace record.
   //
   virtual bool Display(std::ostream& stream, bool diff) override;
private:
   //  Overridden to return a string for displaying this type of record.
   //
   virtual const char* EventString() const override;

   //  The timer's identifier.
   //
   const TimerId tid_;

   //  The timer's duration.
   //
   const secs_t secs_;

   //  The PSM associated with the timer.
   //
   const Pooled* const psm_;
};

//------------------------------------------------------------------------------
//
//  Records the creation or deletion of an event.
//
class EventTrace : public SboTrace
{
public:
   //  Types of event trace records.
   //
   static const Id Creation = 13;
   static const Id Deletion = 14;
   static const Id Handler  = 15;
   static const Id SxpEvent = 16;
   static const Id SipEvent = 17;

   //  Creates a trace record for EVT, with a record type of RID and
   //  the specified SIZE.
   //
   EventTrace(Id rid, const Event& evt);

   //  Public to allow subclassing.
   //
   virtual ~EventTrace() = default;

   //  Overridden to display the trace record.
   //
   virtual bool Display(std::ostream& stream, bool diff) override;
protected:
   //  For subclasses.
   //
   EventTrace(size_t size, const Event& evt);

   //  Displays the event name associated with SID and EID.
   //
   static void DisplayEvent(std::ostream& stream, ServiceId sid, EventId eid);

   //  Overridden to return a string for displaying this type of record.
   //
   virtual const char* EventString() const override;

   //  The service that owns the event.
   //
   ServiceId owner_;

   //  The event's identifier.
   //
   const EventId eid_;
};

//------------------------------------------------------------------------------
//
//  Records the invocation of an event handler.
//
class HandlerTrace : public EventTrace
{
public:
   //  Public to allow subclassing.
   //
   virtual ~HandlerTrace() = default;

   //  Creates a trace record when the service identified by SID, in
   //  STATE, has processed EVT, with the event handler returning RC.
   //
   HandlerTrace(ServiceId sid, const State& state, const Event& evt,
      EventHandler::Rc rc);

   //  Overridden to display the trace record.
   //
   virtual bool Display(std::ostream& stream, bool diff) override;
protected:
   //  For subclasses.
   //
   HandlerTrace(size_t size, ServiceId sid, const State& state,
      const Event& evt, EventHandler::Rc rc);

   //  Displays the state associated with sid_ and stid_.
   //
   void DisplayState(std::ostream& stream) const;

   //  The service whose event handler received the event.
   //
   const ServiceId sid_;

   //  The state in which the event occurred.
   //
   const StateId stid_;

   //  What the event handler returned.
   //
   const EventHandler::Rc rc_;
};

//------------------------------------------------------------------------------
//
//  Records the invocation of an event handler with an SAP or SNP event.
//
class SxpTrace : public HandlerTrace
{
public:
   //  See the constructor for HandlerTrace.
   //
   SxpTrace(ServiceId sid, const State& state,
      const Event& sxp, EventHandler::Rc rc);

   //  Overridden to display the trace record.
   //
   virtual bool Display(std::ostream& stream, bool diff) override;
private:
   //  The event identifier for the SAP or SNP's currEvent_.
   //
   EventId curr_;
};

//------------------------------------------------------------------------------
//
//  Records the invocation of an event handler with an SIP event.
//
class SipTrace : public HandlerTrace
{
public:
   //  See the constructor for HandlerTrace.
   //
   SipTrace(ServiceId sid, const State& state,
      const Event& sip, EventHandler::Rc rc);

   //  Overridden to display the trace record.
   //
   virtual bool Display(std::ostream& stream, bool diff) override;
private:
   //  The service whose initiation was requested.
   //
   const ServiceId mod_;
};
}
#endif
