//==============================================================================
//
//  TraceRecord.h
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
#ifndef TRACERECORD_H_INCLUDED
#define TRACERECORD_H_INCLUDED

#include <cstdint>
#include <iosfwd>
#include <string>
#include "NbTypes.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Base class for trace records.  Trace tools define subclasses to capture
//  debug information.
//
class TraceRecord
{
   friend class TraceBuffer;
public:
   //  Identifier for trace records, which indicates the type of event that
   //  a record captured.  Each record is initialized with InvalidId, which
   //  MUST be overwritten by the *last statement* in a subclass constructor,
   //  even if only with NIL_ID.  Identifiers can be used to avoid trivial
   //  subclassing or to determine the correct subclass for a cast.  Values
   //  are specific to each trace tool.
   //
   typedef TraceRecordId Id;

   //  Identifies a record that has not been added to the trace buffer.
   //  Assigned as the initial value for slot_.
   //
   static const uint32_t InvalidSlot;

   //  Virtual to allow subclassing.
   //
   virtual ~TraceRecord() = default;

   //  Returns the trace buffer slot assigned to the record.
   //
   uint32_t Slot() const { return slot_; }

   //  Returns the trace tool that owns this record.  This allows records
   //  to be included or excluded based on which tools are enabled.
   //
   FlagId Owner() const { return owner_; }

   //  Returns the type of record (owner-specific).
   //
   Id Rid() const { return rid_; }

   //  Invoked to display the record in STREAM.  OPTS specifies options:
   //  o NoTimeData ('t') suppresses timing data that would otherwise
   //    result in undesirable mismatches in a >diff between traces
   //  o NoCtorRelocation ('c') suppresses the relocation of leaf class
   //    constructors, which usually provides better timing data but which
   //    sometimes moves a constructor to an incorrect location
   //  A subclass may invoke this version, which displays EventString()
   //  (see below) followed by a TraceDump::Tab.  Returns false if nothing
   //  was displayed, which prevents the insertion of an endline.
   //
   virtual bool Display(std::ostream& stream, const std::string& opts);
protected:
   //  OWNER is the tool that created the record.  Protected because this
   //  class is virtual.
   //
   explicit TraceRecord(FlagId owner);

   //  Returns a five-character string to be displayed in the EVENT field.
   //  The default version returns an empty string and should generally be
   //  overridden by subclasses.
   //
   virtual c_string EventString() const;

   //  Nullifies a record.  This causes it to be ignored when dumping
   //  trace records.
   //
   void Nullify() { owner_ = NIL_ID; }
private:
   //  Must be overridden to claim any Pooled object owned by a subclass.
   //  TraceBuffer::ClaimBlocks must also be modified to add the owner
   //  to its search mask.
   //
   virtual void ClaimBlocks() { }

   //  Invoked when entering a restart at LEVEL.  Typically overridden to
   //  nullify or otherwise modify a record that has a pointer to something
   //  that will vanish during the restart.
   //
   virtual void Shutdown(RestartLevel level) { }

   //  The record's slot in the trace buffer.
   //
   uint32_t slot_;

   //  The record's owner (a trace tool identifier).
   //
   FlagId owner_ : 8;
protected:
   //  The record's identifier.  This typically identifies the type of event
   //  that was recorded, but its interpretation is owner specific.  It is
   //  initialized to InvalidId.
   //
   Id rid_ : 8;
};

//------------------------------------------------------------------------------
//
//  Options for the CLI >save command.
//
constexpr char NoTimeData = 't';
constexpr char NoCtorRelocation = 'c';
}
#endif
