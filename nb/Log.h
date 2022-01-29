//==============================================================================
//
//  Log.h
//
//  Copyright (C) 2013-2021  Greg Utas
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
#ifndef LOG_H_INCLUDED
#define LOG_H_INCLUDED

#include "Immutable.h"
#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <string>
#include "NbTypes.h"
#include "RegCell.h"
#include "SysTypes.h"

namespace NodeBase
{
   class Alarm;
   class LogGroup;
   struct LogDynamic;
}

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Interface for defining and generating logs.  Logs survive all restarts
//  so that they can be generated during a restart.
//
class Log : public Immutable
{
   friend class Alarm;
public:
   //  Deleted to prohibit copying.
   //
   Log(const Log& that) = delete;

   //  Deleted to prohibit copy assignment.
   //
   Log& operator=(const Log& that) = delete;

   //  The maximum identifier for a log.
   //
   static const LogId MaxId = 999;

   //> The maximum length of the string that explains a log.
   //
   static const size_t MaxExplSize;

   //  The indentation for each subsequent line of a log.
   //
   static const col_t Indent;

   //  A string for inserting Tab spaces.
   //
   static const std::string Tab;

   //  Defines a log that belongs to GROUP and is identified by ID, which
   //  should be defined as a LogType plus offset.  EXPL provides a brief
   //  explanation of the log and is included with each occurrence.
   //
   Log(LogGroup* group, LogId id, c_string expl);

   //  Not subclassed.
   //
   ~Log();

   //  Creates an instance of the log identified by groupName and ID.  The
   //  version that includes alarmName and STATUS is used to set or clear
   //  an alarm.  Only trouble and threshold logs should set an alarm, but
   //  any type of log can clear one.  Returns the stream allocated for the
   //  log after formatting its header, which ends in a CRLF.  When adding
   //  more info to the log, start each line with Tab spaces (defined above)
   //  so that asterisks used to highlight an alarm remain prominent.
   //
   //  NOTE: Returns nullptr if the log is suppressed, so check for this!
   //        Applications should also try to avoid causing log floods, which
   //        usually occur when a log is continuously repeated or a higher
   //        level problem generates dependent logs.
   //
   static ostringstreamPtr Create(c_string groupName, LogId id);

   //  Submits STREAM to the log system.  STREAM must have been created by
   //  Create.  Adds a CRLF to stream if it does not end with one.  The log
   //  system assumes ownership of STREAM, which is set to nullptr.
   //
   static void Submit(ostringstreamPtr& stream);

   //  Returns the log's identifier within its group.
   //
   LogId Id() const { return id_; }

   //  Used to suppress or throttle the log:
   //    o 1: no effect
   //    o 0: suppresses all occurrences of the log
   //    o N: throttles the log: only every Nth occurrence is generated
   //  A log that sets an alarm is never suppressed.  A log that clears an
   //  alarm is suppressed if the alarm is already off.
   //
   void SetInterval(uint8_t interval);

   //  Returns the log associated with groupName and ID.  Updates
   //  GROUP to the log's group.
   //
   static Log* Find(c_string groupName, LogId id, LogGroup*& group);

   //  Returns the log associated with LOG by extracting the group
   //  name and LogId from the beginning of LOG.
   //
   static Log* Find(c_string log);

   //  Displays the log's statistics.
   //
   void DisplayStats(std::ostream& stream, const Flags& options) const;

   //  Returns the number of logs generated so far.
   //
   static size_t Count();

   //  Returns the offset to lid_.
   //
   static ptrdiff_t CellDiff();

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;

   //  Overridden for restarts.
   //
   void Shutdown(RestartLevel level) override;

   //  Overridden for restarts.
   //
   void Startup(RestartLevel level) override;
private:
   //  Creates an instance of the log identified by groupName and ID, with
   //  alarmName and STATUS used to set or clear an alarm.  Applications
   //  must use Alarm::Create, which invokes this.
   //
   static ostringstreamPtr Create(c_string groupName,
      LogId id, c_string alarmName, AlarmStatus status);

   //  Creates and returns the log's header.  STATUS is used when
   //  modifying an alarm.
   //
   ostringstreamPtr Format(AlarmStatus status = NoAlarm) const;

   //  Returns nullptr after incrementing the count of suppressed logs.
   //
   ostringstreamPtr Suppressed() const;

   //  The group to which the log belongs.
   //
   LogGroup* const group_;

   //  The log's identifier within its group.
   //
   const LogId id_;

   //  The log's explanation.
   //
   const ImmutableStr expl_;

   //  The log's index in its LogGroup's registry.
   //
   RegCell lid_;

   //  Data that changes too frequently to unprotect and reprotect memory
   //  when it needs to be modified.
   //
   std::unique_ptr< LogDynamic > dyn_;

   //  The number of times the log was buffered for output.
   //
   CounterPtr bufferCount_;

   //  The number of times the log was suppressed.
   //
   CounterPtr suppressCount_;

   //  The number of times the log was discarded.
   //
   CounterPtr discardCount_;
};
}
#endif
