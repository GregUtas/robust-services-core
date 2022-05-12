//==============================================================================
//
//  TimedRecord.h
//
//  Copyright (C) 2013-2022  Greg Utas
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
#ifndef TIMEDRECORD_H_INCLUDED
#define TIMEDRECORD_H_INCLUDED

#include "TraceRecord.h"
#include <chrono>
#include <ratio>
#include <string>
#include "SysDecls.h"
#include "SystemTime.h"
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
   virtual ~TimedRecord() = default;

   //  Returns the tick time at which the event occurred.
   //
   SystemTime::Point GetTime() const { return time_; }

   //  Sets the time at which the event occurred.
   //
   void SetTime(const SystemTime::Point& time) { time_ = time; }

   //  Returns the native thread identifier associated with the event.
   //
   SysThreadId Nid() const { return nid_; }

   //  Returns the time (mins:secs.msecs) at which the event occurred.
   //  Returns "00:00.000" if OPTS specifies NoTimeData.
   //
   std::string GetTime(const std::string& opts) const;

   //  Overridden to display the timestamp and executing thread.  Displays
   //  nothing and returns false if the thread is to be excluded from this
   //  trace.  May be overridden, but this version should be invoked first.
   //
   bool Display(std::ostream& stream, const std::string& opts) override;
protected:
   //  See TraceRecord for a description of the arguments.  Protected
   //  because this class is virtual.
   //
   explicit TimedRecord(FlagId owner);
private:
   //  The thread that was running when the function was invoked.
   //
   const SysThreadId nid_;

   //  The time when the record was created.
   //
   SystemTime::Point time_;
};
}
#endif
