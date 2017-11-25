//==============================================================================
//
//  FunctionTrace.cpp
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
#include "FunctionTrace.h"
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <ostream>
#include <string>
#include "Algorithms.h"
#include "Debug.h"
#include "FunctionName.h"
#include "Memory.h"
#include "Singleton.h"
#include "SysThreadStack.h"
#include "TraceBuffer.h"
#include "TraceDump.h"

using std::ostream;
using std::setw;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
const Flags FunctionTrace::FTmask = Flags(1 << FunctionTracer);
FunctionTrace::Scope FunctionTrace::Scope_ = FullTrace;
fn_depth FunctionTrace::MinDepth_ = 0;
const fn_depth FunctionTrace::MaxDispDepth = 40;

//------------------------------------------------------------------------------

FunctionTrace::FunctionTrace(fn_name_arg func, fn_depth depth,
   size_t size) : TimedRecord(size, FunctionTracer),
   func_(func),
   depth_(depth),
   moved_(false),
   stop_(false),
   gross_(0),
   net_(0)
{
   if(depth_ < 0) depth_ = 0;
   rid_ = NIL_ID;
}

//------------------------------------------------------------------------------

fn_name FunctionTrace_CalcFuncTime = "FunctionTrace.CalcFuncTime";

void FunctionTrace::CalcFuncTime()
{
   Debug::ft(FunctionTrace_CalcFuncTime);

   auto buff = Singleton< TraceBuffer >::Instance();
   auto mask = FTmask;
   auto nid = Nid();
   TraceRecord* rec = this;

   //  Start by calculating the gross time for this function.
   //
   gross_ = CalcGrossTime();

   //  The net time for a function at depth_ is its gross time minus
   //  the sum of all gross times spent in functions at depth_ + 1.
   //
   net_ = gross_;
   if(net_ == 0) return;

   for(buff->Next(rec, mask); rec != nullptr; buff->Next(rec, mask))
   {
      auto curr = static_cast< FunctionTrace* >(rec);

      if(curr->depth_ <= depth_) return;
      if(curr->Nid() != nid) return;
      if(curr->IsFirstAfterContextSwitch()) return;

      if(curr->depth_ == (depth_ + 1))
      {
         net_ -= curr->CalcGrossTime();
      }
   }
}

//------------------------------------------------------------------------------

fn_name FunctionTrace_CalcGrossTime = "FunctionTrace.CalcGrossTime";

usecs_t FunctionTrace::CalcGrossTime()
{
   Debug::ft(FunctionTrace_CalcGrossTime);

   auto buff = Singleton< TraceBuffer >::Instance();
   auto mask = FTmask;
   TraceRecord* rec = this;
   FunctionTrace* prev = this;
   auto nid = Nid();
   ticks_t gross;

   //  A function's gross time is the time between when it was invoked and
   //  when the next function at the same (or lower) depth was invoked--in
   //  the same thread and during the same transaction.
   //
   for(buff->Next(rec, mask); rec != nullptr; buff->Next(rec, mask))
   {
      auto curr = static_cast< FunctionTrace* >(rec);

      if(curr->Nid() != nid) break;
      if(curr->IsFirstAfterContextSwitch()) break;

      if(curr->depth_ <= depth_)
      {
         gross = curr->GetTicks() - GetTicks();
         return Clock::TicksToUsecs(gross);
      }

      prev = curr;
   }

   gross = prev->GetTicks() - GetTicks();
   return Clock::TicksToUsecs(gross);
}

//------------------------------------------------------------------------------

fn_name Cxx_delete = "C++.delete";

void FunctionTrace::Capture(fn_name_arg func)
{
   //  The actual trace is
   //    func
   //      Debug::ft
   //        Thread::FunctionInvoked
   //          FunctionTrace::Capture
   //  FUNC is therefore three levels above the current depth.
   //
   if(Scope_ == CountsOnly)
   {
      auto buff = Singleton< TraceBuffer >::Instance();
      buff->RecordInvocation(func);
      return;
   }

   auto depth = SysThreadStack::FuncDepth() - 3;

   //  If this is a destructor call that is not one level deeper than the last
   //  destructor or function, add a call to a compiler-generated "C++.delete"
   //  function.  The compiler generates this code to invoke a delete operator
   //  after a destructor.  If it turns out that a delete operator is not
   //  invoked, Postprocess (see below) invalidates this call to C++.delete.
   //  Because many spurious instances of C++.delete are invalidated, don't
   //  add any during immediate tracing.
   //
   if(FunctionName::rfind(func, FunctionName::DtorTag) != string::npos)
   {
      auto buff = Singleton< TraceBuffer >::Instance();

      if(!buff->ImmediateTraceOn())
      {
         auto insert = (buff->LastDtorDepth() != depth - 1);

         if(!insert)
         {
            auto rec = buff->LastRecord();

            if(rec != nullptr)
            {
               if(rec->Owner() == FunctionTracer)
               {
                  auto last = static_cast< const FunctionTrace* >(rec);
                  insert = (last->depth_ != depth - 1);
               }
            }
         }

         if(insert)
         {
            new FunctionTrace(Cxx_delete, depth - 1, sizeof(FunctionTrace));
         }

         buff->SetLastDtorDepth(depth);
      }
   }

   new FunctionTrace(func, depth, sizeof(FunctionTrace));
}

//------------------------------------------------------------------------------

bool FunctionTrace::Display(ostream& stream)
{
   if(!TimedRecord::Display(stream)) return false;

   stream << setw(TraceDump::TotWidth) << gross_ << TraceDump::Tab();
   stream << setw(TraceDump::NetWidth) << net_ << TraceDump::Tab();

   auto dispDepth = std::min(depth_, MaxDispDepth);

   for(auto d = 0; d < dispDepth; ++d)
   {
      if((d & 1) == 1)
         stream << ": ";
      else
         stream << "  ";
   }

   if(depth_ > MaxDispDepth)
   {
      stream << "[+" << setw(4) << depth_ - MaxDispDepth << "] ";
   }

   if((func_ != nullptr) && (strlen(func_) > 0))
      stream << func_;
   else
      stream << "unknown function";

// if(moved_) stream << '^';
// if(stop_) stream << '#';

   return true;
}

//------------------------------------------------------------------------------

fn_name FunctionTrace_FindDeleteOperator = "FunctionTrace.FindDeleteOperator";

bool FunctionTrace::FindDeleteOperator()
{
   Debug::ft(FunctionTrace_FindDeleteOperator);

   auto buff = Singleton< TraceBuffer >::Instance();
   auto mask = FTmask;
   auto n = 500;
   TraceRecord* rec = this;

   //  Search the next N function calls to see if this function (C++.delete)
   //  invoked a delete operator.  The operator should be at depth_ + 1.
   //
   for(buff->Next(rec, mask); ((rec != nullptr) && (n > 0));
      buff->Next(rec, mask), --n)
   {
      auto curr = static_cast< FunctionTrace* >(rec);

      if(curr->depth_ <= depth_) return false;

      if((curr->depth_ == depth_ + 1) &&
         (FunctionName::rfind
            (curr->func_, FunctionName::OpDelTag) != string::npos))
      {
         return true;
      }
   }

   return false;
}

//------------------------------------------------------------------------------

fn_name FunctionTrace_InvertCtors = "FunctionTrace.InvertCtors";

void FunctionTrace::InvertCtors(fn_depth limit)
{
   Debug::ft(FunctionTrace_InvertCtors);

   auto buff = Singleton< TraceBuffer >::Instance();
   auto mask = FTmask;
   TraceRecord* rec;
   FunctionTrace* curr = nullptr;
   const size_t MaxCtors = 24;
   FunctionTrace* ctors[MaxCtors];
   size_t count = 0;

   //  Scan ahead for constructors from depth_ - 1 back to LIMIT.
   //
   for(auto depth = depth_ - 1; depth >= limit; --depth)
   {
      rec = this;

      for(buff->Next(rec, mask); rec != nullptr; buff->Next(rec, mask))
      {
         curr = static_cast< FunctionTrace* >(rec);

         //  The stop_ flag is set for the function that follows a previous
         //  chain of inverted constructor calls.  It is encountered when
         //  that chain contains a nested set of constructor calls (that is,
         //  when an outer object contains an inner object).  It therefore
         //  acts as a boundary when inverting constructor calls for the
         //  inner object.
         //
         if(curr->stop_) break;

         //  If this function is at less than the target depth, we must be
         //  missing a constructor (that is, one of the constructors in the
         //  class hierarchy did not invoke Debug::ft).  Continue the search
         //  at the previous level.
         //
         if(curr->depth_ < depth) break;

         if(curr->depth_ == depth)
         {
            //  A call to operator new at the same level means that the chain
            //  has ended.  This check exists because some leaf classes may
            //  use default constructors and therefore won't invoke Debug::ft
            //  to leave a constructor at the level of their operator new.
            //
            if(FunctionName::rfind
               (curr->func_, FunctionName::OpNewTag) != string::npos)
            {
               break;
            }

            //  If this is a constructor call at the desired depth, add it to
            //  the list of constructors whose order is to be inverted and
            //  continue the search at the previous level.
            //
            if(FunctionName::rfind
               (curr->func_, FunctionName::CtorTag) != string::npos)
            {
               ctors[count++] = curr;
               break;
            }
         }
      }

      //  Stop searching for constructors if the buffer has no more records
      //  or a boundary was reached.
      //
      if(rec == nullptr) break;
      if(curr->stop_) break;
      if(count >= MaxCtors) break;
   }

   if(count > 0)
   {
      //  If the last constructor resides above "this", then the buffer
      //  wrapped around somewhere in the constructor chain.  This will
      //  corrupt the buffer when Memory::Copy is invoked.  It's not
      //  worth the effort to handle it as a special case, so just return.
      //
      if(uintptr_t(ctors[count - 1]) < uintptr_t(this)) return;

      //  We need to invert at least one constructor.  Find the function
      //  that follows the outermost constructor and set its stop_ flag.
      //
      moved_ = true;
      rec = ctors[count - 1];
      buff->Next(rec, mask);
      if(rec == nullptr) return;
      curr = static_cast< FunctionTrace* >(rec);
      curr->stop_ = true;

      //  Invert the constructors, starting with the deepest one.
      //
      for(size_t i = 0; i < count; ++i)
      {
         //  Make a copy of the constructor to be inserted at the front of the
         //  chain.  Reset its time to that of Base.ctor (first in the chain)
         //  and mark it as having moved.  Push the constructors starting at
         //  "this" down the buffer and insert the copied constructor ahead of
         //  them.  (Initially, "this" points to the leaf class constructor,
         //  but it points to successive base class constructors as they move
         //  to the top of the chain.)
         //
         FunctionTrace ft = *ctors[i];
         ft.SetTicks(this->GetTicks());
         ft.moved_ = true;
         auto src = (ptr_t) this;
         auto dst = src + sizeof(FunctionTrace);
         auto len = ptrdiff(ctors[i], src);
         Memory::Copy(dst, src, len);
         Memory::Copy(src, &ft, sizeof(FunctionTrace));
      }
   }
}

//------------------------------------------------------------------------------

bool FunctionTrace::IsFirstAfterContextSwitch() const
{
   return false;
}

//------------------------------------------------------------------------------

fn_name FunctionTrace_Postprocess = "FunctionTrace.Postprocess";

void FunctionTrace::Postprocess()
{
   Debug::ft(FunctionTrace_Postprocess);

   auto buff = Singleton< TraceBuffer >::Instance();
   MinDepth_ = INT16_MAX;

   buff->Lock();
   {
      TraceRecord* rec = nullptr;
      auto mask = FTmask;
      fn_depth limit = 1;

      for(buff->Next(rec, mask); rec != nullptr; buff->Next(rec, mask))
      {
         auto curr = static_cast< FunctionTrace* >(rec);

         if(curr->depth_ < MinDepth_) MinDepth_ = curr->depth_;

         if(FunctionName::rfind
            (curr->func_, FunctionName::OpNewTag) != string::npos)
         {
            //  A new operator precedes a chain of constructor calls.  The
            //  last of them will be at the same depth as the new operator,
            //  so save operator new's depth as the limit when inverting
            //  constructor calls.
            //
            limit = curr->depth_;
            continue;
         }

         if(FunctionName::rfind
            (curr->func_, FunctionName::CtorTag) != string::npos)
         {
            //  By default, a chain of constructor calls begins at the base
            //  class and proceeds to the leaf class.  This makes the total
            //  and net time spent in each constructor misleading.  The order
            //  of constructors is therefore reversed to show a more accurate
            //  picture.  Although an object's members are initialized starting
            //  with the base class, the function calls actually occur in the
            //  reverse order.  However, the call to the base class constructor
            //  occurs *before* a constructor's first line of code, namely the
            //  invocation of Debug::ft, can add the constructor to the trace.
            //  This produces a right-to-left staircase that we invert to a
            //  left-to-right, upside-down staircase.  After this inversion,
            //  we re-encounter the constructors that were moved, so we need
            //  to skip them instead of letting them trigger a new inversion.
            //
            if(curr->moved_) continue;
            curr->InvertCtors(limit);
            limit = 1;
            continue;
         }

         if(FunctionName::compare(curr->func_, Cxx_delete) == 0)
         {
            //  A "C++.delete" function is inserted ahead of a destructor that
            //  is other than one level deeper than the previous function.  It
            //  represents a compiler-generated delete function that invokes
            //  destructors (starting with the leaf class), followed by the
            //  delete operator.  Therefore, if a delete operator at this depth
            //  soon appears in the trace, this record is retained; otherwise,
            //  it is invalidated.
            //
            if(!curr->FindDeleteOperator())
            {
               curr->Nullify();
            }
            continue;
         }
      }

      //  Now that constructors and destructors have been post-processed, find
      //  the total and net time spent in each function.  After this has been
      //  done, a function's depth can be adjusted.
      //
      rec = nullptr;

      for(buff->Next(rec, mask); rec != nullptr; buff->Next(rec, mask))
      {
         auto curr = static_cast< FunctionTrace* >(rec);
         curr->CalcFuncTime();
         curr->depth_ -= MinDepth_;
      }
   }
   buff->Unlock();

   //  MinDepth_ must be reset in case an immediate trace is performed.
   //
   MinDepth_ = 0;
}

//------------------------------------------------------------------------------

fn_name FunctionTrace_SetScope = "FunctionTrace.SetScope";

TraceRc FunctionTrace::SetScope(Scope scope)
{
   Debug::ft(FunctionTrace_SetScope);

   if(Debug::TraceOn()) return NotWhileTracing;

   Scope_ = scope;
   return TraceOk;
}
}
