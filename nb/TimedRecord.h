//==============================================================================
//
//  TimedRecord.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef TIMEDRECORD_H_INCLUDED
#define TIMEDRECORD_H_INCLUDED

#include "TraceRecord.h"
#include <cstddef>
#include <string>
#include "Clock.h"
#include "NbTypes.h"
#include "SysDecls.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Base class for trace records that include their time of creation.
//
class TimedRecord : public TraceRecord
{
public:
   //  Virtual to allow subclassing.
   //
   virtual ~TimedRecord() { }

   //  Returns the tick time at which the event occurred.
   //
   ticks_t GetTicks() const { return ticks_; }

   //  Sets the tick time at which the event occurred.
   //
   void SetTicks(const ticks_t& ticks) { ticks_ = ticks; }

   //  Returns the native thread identifier associated with the event.
   //
   SysThreadId Nid() const { return nid_; }

   //  Returns the time (mins:secs.msecs) at which the event occurred.
   //
   std::string GetTime() const;

   //  Returns the thread identifier associated with the event.
   //
   ThreadId Tid() const;

   //  Overridden to display the timestamp and executing thread.  Displays
   //  nothing and returns false if the thread is to be excluded from this
   //  trace.  May be overridden, but this version should be invoked first.
   //
   virtual bool Display(std::ostream& stream) override;
protected:
   //  See TraceRecord for a description of the arguments.  Protected
   //  because this class is virtual.
   //
   TimedRecord(size_t size, FlagId owner);
private:
   //  The thread that was running when the function was invoked.
   //
   const SysThreadId nid_ : 16;

   //  The time when the record was created.
   //
   ticks_t ticks_;
};
}
#endif
