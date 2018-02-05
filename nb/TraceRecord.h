//==============================================================================
//
//  TraceRecord.h
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
#ifndef TRACERECORD_H_INCLUDED
#define TRACERECORD_H_INCLUDED

#include <cstddef>
#include <cstdint>
#include <iosfwd>
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
   typedef uint8_t Id;

   //  Identifies a record that has not been fully constructed.
   //
   static const Id InvalidId = UINT8_MAX;

   //  Virtual to allow subclassing.
   //
   virtual ~TraceRecord() { }

   //  Returns the trace tool that owns this record.  This allows records
   //  to be included or excluded based on which tools are enabled.
   //
   FlagId Owner() const { return owner_; }

   //  Returns the type of record (owner-specific).
   //
   Id Rid() const { return rid_; }

   //  Nullifies a record.  This causes it to be ignored when dumping
   //  trace records.
   //
   void Nullify() { owner_ = NIL_ID; }

   //  Invoked to display the record in STREAM when the trace is printed.
   //  A subclass must begin by invoking TraceRecord::Display.  Returns
   //  false if nothing was displayed, which suppresses insertion of an
   //  endline.
   //
   virtual bool Display(std::ostream& stream);

   //  Overridden to allocate space in the trace buffer.
   //
   static void* operator new(size_t size);

   //  Overridden to return space to the trace buffer.  This does nothing
   //  because the trace buffer is circular and simply overwrites records
   //  as it cycles around.
   //
   static void operator delete(void* addr) { }

   //  Overridden to support placement new.
   //
   static void* operator new(size_t size, void* where);

   //  Overridden to handle the failure of placement new.  It will never be
   //  invoked, but the compiler generates a warning if it is not supplied.
   //
   static void operator delete(void* addr, void* where) { }
protected:
   //  SIZE is the record's size (in bytes), and OWNER is the tool that
   //  created the record.  Protected because this class is virtual.
   //
   TraceRecord(size_t size, FlagId owner);

   //  Returns a five-character string to be displayed in the EVENT field.
   //  The default version returns an empty string and should generally be
   //  overridden by subclasses.
   //
   virtual const char* EventString() const;
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

   //  The record's size.
   //
   const int16_t size_ : 16;

   //  The record's owner.
   //
   FlagId owner_ : 8;
protected:
   //  The record's identifier.  This typically identifies the type of event
   //  that was recorded, but its interpretation is owner specific.  *Must*
   //  be overwritten by a constructor.
   //
   Id rid_ : 8;
};
}
#endif
