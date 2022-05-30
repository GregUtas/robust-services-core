//==============================================================================
//
//  MemoryTrace.cpp
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
#include "MemoryTrace.h"
#include <ostream>
#include <string>
#include "Formatters.h"
#include "Singleton.h"
#include "ToolTypes.h"
#include "TraceBuffer.h"
#include "TraceDump.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
fixed_string MemTypeStrings[MemoryType_N + 1] =
{
   ERROR_STR,
   "temp",
   "dyn ",
   "slab",
   "prot",
   "perm",
   "imm ",
   ERROR_STR
};

//  Returns a string for displaying TYPE.
//
static c_string TypeString(MemoryType type)
{
   if((type >= 0) && (type < MemoryType_N)) return MemTypeStrings[type];
   return MemTypeStrings[MemoryType_N];
}

//------------------------------------------------------------------------------

MemoryTrace::MemoryTrace(Id rid, const void* addr, MemoryType type,
   size_t size) : TimedRecord(MemoryTracer),
   addr_(addr),
   type_(type),
   size_(size)
{
   rid_ = rid;
}

//------------------------------------------------------------------------------

bool MemoryTrace::Display(ostream& stream, const string& opts)
{
   if(!TimedRecord::Display(stream, opts)) return false;

   stream << spaces(TraceDump::EvtToObj) << addr_ << TraceDump::Tab();
   stream << "type=" << TypeString(type_) << spaces(3);
   stream << "size=" << size_;

   if(rid_ == Free) return true;

   //  If there is no record of this memory being freed, flag it.
   //
   auto buff = Singleton<TraceBuffer>::Instance();
   Flags mask(1 << MemoryTracer);
   TraceRecord* rec = this;

   for(buff->Next(rec, mask); rec != nullptr; buff->Next(rec, mask))
   {
      auto curr = static_cast<MemoryTrace*>(rec);

      if((curr->addr_ == addr_) && (curr->rid_ == Free)) return true;
   }

   stream << " NOT FREED";
   return true;
}

//------------------------------------------------------------------------------

fixed_string AllocEventStr = " +mem";
fixed_string FreeEventStr  = " -mem";

c_string MemoryTrace::EventString() const
{
   switch(rid_)
   {
   case Alloc: return AllocEventStr;
   case Free: return FreeEventStr;
   }

   return ERROR_STR;
}
}
