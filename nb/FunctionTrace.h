//==============================================================================
//
//  FunctionTrace.h
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
#ifndef FUNCTIONTRACE_H_INCLUDED
#define FUNCTIONTRACE_H_INCLUDED

#include "TimedRecord.h"
#include <cstddef>
#include "Clock.h"
#include "SysTypes.h"
#include "ToolTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Records a function call (the public interface is Debug::ft).
//
class FunctionTrace : public TimedRecord
{
   friend class Thread;
public:
   //  Sets func_ and depth_.
   //
   FunctionTrace(fn_name_arg func, fn_depth depth);

   //  Virtual to allow subclassing.
   //
   virtual ~FunctionTrace() = default;

   //  How functions are being traced.
   //
   enum Scope
   {
      FullTrace,  // capturing a detailed history of function invocations
      CountsOnly  // only counting how many times each function was invoked
   };

   //  Sets the scope of the function trace.
   //
   static TraceRc SetScope(Scope scope);

   //  Returns the scope of the function trace.
   //
   static Scope GetScope() { return Scope_; }

   //  Invoked when tracing stops.  Modifies records to handle constructors
   //  and destructors.
   //
   static void Postprocess();

   //  Returns the function whose invocation this record captured.
   //
   fn_name Func() const { return func_; }

   //  Returns the depth of the function whose invocation this record captured.
   //
   fn_depth Depth() const { return depth_; }

   //  Returns the net time spent in the function that this record captured.
   //
   usecs_t Net() const { return net_; }

   //  Overridden to display the trace record.
   //
   bool Display(std::ostream& stream, bool diff) override;

   //  Mask for selecting FunctionTrace records when using TraceBuffer::Next.
   //
   static const Flags FTmask;
private:
   //  Overridden to allocate space in the buffer allocated for records
   //  that belong to this class.
   //
   static void* operator new(size_t size);

   //  Overridden to return space to the buffer.  This does nothing because
   //  the buffer is circular and simply overwrites previous records when it
   //  cycles around.
   //
   static void operator delete(void* addr) { }

   //  Overridden to support placement new.
   //
   static void* operator new(size_t size, void* where);

   //  Overridden to handle the failure of placement new.  It will never be
   //  invoked, but the compiler generates a warning if it is not supplied.
   //
   static void operator delete(void* addr, void* where) { }

   //  Captures a call to FUNC when tracing is enabled.  Applications use
   //  Debug::ft instead of invoking this directly.
   //
   static void Capture(fn_name_arg func);

   //  Calculates the gross and net times spent in a function call.
   //
   void CalcFuncTime();

   //  Calculates the gross time spent in a function call.
   //
   usecs_t CalcGrossTime();

   //  Searches for constructor invocations associated with this record and
   //  reorders them so that they run from leaf class to base class instead
   //  of from base class to leaf class.  LIMIT is the minimum depth for one
   //  of these constructor calls.
   //
   void InvertCtors(fn_depth limit);

   //  The name of function that was invoked.
   //
   fn_name func_;

   //  The nesting level of the function call on the thread's stack.
   //
   fn_depth depth_;

   //  Set for a constructor that was moved during reordering.
   //
   bool moved_;

   //  Set for a function call that follows a reordered constructor.
   //  It acts as a boundary when reordering nested constructors.
   //
   bool stop_;

   //  The total time spent in the function call.  Calculated after
   //  tracing stops.
   //
   usecs_t gross_;

   //  The net time spent in the function call.  Calculated after
   //  tracing stops.
   //
   usecs_t net_;

   //  The scope of function tracing.
   //
   static Scope Scope_;

   //  The minimum depth of all function calls in the trace buffer.
   //
   static fn_depth MinDepth_;

   //> The maximum depth when displaying a function call.
   //
   static const fn_depth MaxDispDepth;
};
}
#endif
