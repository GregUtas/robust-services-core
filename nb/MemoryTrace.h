//==============================================================================
//
//  MemoryTrace.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef MEMORYTRACE_H_INCLUDED
#define MEMORYTRACE_H_INCLUDED

#include "TimedRecord.h"
#include <cstddef>
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Records memory allocations and deallocations.
//
class MemoryTrace : public TimedRecord
{
public:
   //  Types of memory trace records.
   //
   static const Id Alloc = 1;  // allocation
   static const Id Free  = 2;  // deallocation

   //  Sets addr_, type_, and size_.  RID specifies allocation or deallocation.
   //
   MemoryTrace(Id rid, const void* addr, MemoryType type, size_t size);

   //  Overridden to display the trace record.
   //
   virtual bool Display(std::ostream& stream) override;
private:
   //  Overridden to return a string for displaying this type of record.
   //
   virtual const char* EventString() const override;

   //  Returns a string for displaying TYPE.
   //
   static const char* TypeString(MemoryType type);

   //  The address where memory was allocated or freed.
   //
   const void* const addr_;

   //  The type of memory that was allocated or freed.
   //
   const MemoryType type_;

   //  The amount of memory that was allocated or freed.
   //
   const size_t size_;
};
}
#endif
