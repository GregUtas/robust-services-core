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
#include <map>
#include <ostream>
#include <stack>
#include <string>
#include <vector>
#include "Debug.h"
#include "FunctionName.h"
#include "Singleton.h"
#include "SysDecls.h"
#include "SysThread.h"
#include "SysThreadStack.h"
#include "TraceBuffer.h"
#include "TraceDump.h"

using std::ostream;
using std::setw;

//------------------------------------------------------------------------------

namespace NodeBase
{
//  The following is used to postprocess constructor calls before displaying a
//  function trace.  When class0 is constructed, a raw trace looks like this:
//    : : : class3.ctor (inner/first class)
//    : : class2.ctor
//    : class1.ctor
//    class0.ctor (outer/last class)
//  We want to display this as
//    class0.ctor
//    : : : class3.ctor
//    : : class2.ctor
//    : class1.ctor
//  This provides better timing information, with the cost of the constructor
//  chain being imputed to class0.ctor, and is closer to what actually occurs
//  during execution.
//
class CtorChain
{
public:
   //  This class has no resources to release.
   //
   ~CtorChain() = default;

   //  Determines where to insert CTOR.  Returns CTOR unless this
   //  relocates it, in which case it returns the function that
   //  now occupies CTOR's slot.
   //
   static TraceRecord* HandleCtor(FunctionTrace* ctor);

   //  Determines how FUNC affects the current constructor chains.
   //
   static void HandleFunction(FunctionTrace* func);

   //  Moves the outer constructor so that it precedes the inner
   //  constructor when finalizing the chain.
   //
   void MoveOuterAboveInner() const;
private:
   //  Used when a constructor at a deeper level is encountered.
   //
   explicit CtorChain(FunctionTrace* inner);

   //  Used when a new operator is encountered.  The second argument
   //  distinguishes this constructor from the preceding one.
   //
   CtorChain(FunctionTrace* opnew, int dummy);

   //  What to do with a newly encountered constructor.
   //
   enum Action
   {
      Create,      // create a new chain
      SetAsFirst,  // set as the first constructor of this chain
      SetAsNext,   // set as a subsequent constructor of this chain
      SetAsInit,   // set as possible initializer of next constructor's class
      Finalize     // finalize this chain and create a new one
   };

   //  Determines how CTOR affects this chain.  POPPED is set if CTOR
   //  has already finalized a chain.
   //
   Action CalcAction(const FunctionTrace* ctor, bool popped) const;

   //  Adds OUTER to this chain as its outer constructor.
   //
   void SetOuter(FunctionTrace* outer);

   //  Checks if CTOR should be added to a previous chain instead of
   //  the most recent chain.  Returns true if it does this.
   //
   static bool AddToPreviousChain(FunctionTrace* ctor);

   //  Records a function at the same depth as the outer constructor.
   //  It may have been invoked in the member initialization list of
   //  the *next* class's constructor, which has yet to be encountered.
   //
   static void HandleInitializer(const FunctionTrace* init);

   //  Finalizes any chains that are known to be complete because CURR
   //  was invoked.  Returns CURR unless this reorders functions, in
   //  which case it returns the function that now occupies CURR's slot.
   //
   static TraceRecord* CheckForEndOfChains(const FunctionTrace* curr);

   //  Returns true if the invocation of CURR finalizes this chain.
   //
   bool FunctionEndsChain(const FunctionTrace* curr) const;

   //  Returns true if CURR is at the same depth as the outer constructor,
   //  in which case it might have been invoked to initialize a member of
   //  the *next* class's constructor, which has yet to be encountered.
   //
   static bool CheckForInitializer(const FunctionTrace* curr);

   //  Moves the outer constructor so that it precedes the first function
   //  invoked in its member initialization list.  NOTE: Moving the inner
   //  constructor above functions invoked in its member initialization
   //  list is not supported.  Such functions appear before the new chain
   //  is recognized.
   //
   void MoveOuterAboveInit();

   //  The constructor chain.
   //
   std::vector< FunctionTrace* > ctors_;

   //  The call to a new operator, if any, that precedes the chain.
   //
   FunctionTrace* const opnew_;

   //  A function possibly invoked in the member initialization list of
   //  the next constructor that will be added to the chain if it exists.
   //
   const FunctionTrace* init_;
};

//------------------------------------------------------------------------------
//
//  Constructor chains must be tracked on a per-thread basis.  As part of
//  this, it must be possible to refer to the previous function invoked on
//  a thread when considering the current function.
//
struct PerThreadInfo
{
   std::vector< CtorChain > chains;  // chains yet to be finalized
   std::stack< fn_depth > depths;    // depths of active functions

   PerThreadInfo()
   {
      depths.push(0);  // represents C++ run-time
   }

   void PopBack()
   {
      //  When finalizing a constructor chain, move its outermost
      //  constructor so that it precedes the innermost constructor.
      //
      chains.back().MoveOuterAboveInner();
      chains.pop_back();
   }
};

//  The constructor chains that have yet to be finalized.  The map's key is
//  the current function's native thread identifier.
//
std::map< SysThreadId, PerThreadInfo > ThreadInfo;

//==============================================================================

CtorChain::CtorChain(FunctionTrace* inner) :
   opnew_(nullptr),
   init_(nullptr)
{
   ctors_.push_back(inner);
}

//------------------------------------------------------------------------------

CtorChain::CtorChain(FunctionTrace* opnew, int dummy) :
   opnew_(opnew),
   init_(nullptr)
{
}

//------------------------------------------------------------------------------

bool CtorChain::AddToPreviousChain(FunctionTrace* ctor)
{
   auto& thrd = ThreadInfo[ctor->Nid()];
   auto& chains = thrd.chains;
   auto curr = chains.size() - 2;
   auto depth = ctor->Depth();

   //  Check if CTOR should be added to a previous chain instead of being
   //  set as the new outer constructor of the current chain.
   //
   for(NO_OP; curr != SIZE_MAX; --curr)
   {
      auto& chain = chains.at(curr);
      auto outer = (!chain.ctors_.empty() ? chain.ctors_.back() : nullptr);

      //  If CTOR's depth is as great as this chain's outer constructor,
      //  CTOR can't be its new outer constructor.
      //
      if(outer != nullptr)
      {
         auto diff = depth - outer->Depth();
         if(diff >= 0) return false;
      }

      //  If a new operator created this chain, CTOR is its final constructor
      //  if it is at the same depth as the new operator.
      //
      auto opnew = chain.opnew_;

      if((opnew != nullptr) && (depth == opnew->Depth()))
      {
         if((outer == nullptr) || (depth < outer->Depth()))
         {
            chains.at(curr).SetOuter(ctor);
            ++curr;

            //  All the chains that follow CURR have now been finalized.
            //
            while(chains.size() > curr)
            {
               thrd.PopBack();
            }

            return true;
         }
      }
   }

   return false;
}

//------------------------------------------------------------------------------

CtorChain::Action CtorChain::CalcAction
   (const FunctionTrace* ctor, bool popped) const
{
   auto ctorDepth = ctor->Depth();

   if(ctors_.empty())
   {
      //  A new operator is waiting for its first constructor.
      //
      if(ctorDepth >= opnew_->Depth())
      {
         return SetAsFirst;
      }

      //  CTOR is at a lower depth and is therefore associated with
      //  an earlier chain.
      //
      return Finalize;
   }

   auto outer = ctors_.back();
   auto diff = ctorDepth - outer->Depth();

   //  If CTOR is deeper than the outer constructor, it belongs to
   //  a deeper chain.
   //
   if(diff > 0)
   {
      return Create;
   }

   if(diff == 0)
   {
      //  CTOR is initializing a member of the next outer constructor's
      //  class *unless* this chain has a new operator and its outermost
      //  constructor (at the same depth as the new operator) is known.
      //
      if((opnew_ == nullptr) || (opnew_->Depth() < outer->Depth()))
         return SetAsInit;
      else
         return Finalize;
   }

   if(diff == -1)
   {
      //  CTOR becomes the new outer constructor for this chain *unless*
      //  o this chain has a new operator and its outermost constructor is
      //    known (it should be at the same depth as the new operator), or
      //  o CTOR already finalized a chain and was invoked from a depth
      //    that is more than one less than this chain's existing outer
      //    constructor.
      //
      if((opnew_ != nullptr) && (opnew_->Depth() >= outer->Depth()))
         return Finalize;
      if(popped && (outer->InvokerDepth() - ctor->InvokerDepth() > 1))
         return Finalize;
      return SetAsNext;
   }

   //  We assume that constructors in the interior of a chain invoke
   //  Debug::ft, so a constructor more than one level above the outer
   //  constructor must be associated with another chain.
   //
   return Finalize;
}

//------------------------------------------------------------------------------

TraceRecord* CtorChain::HandleCtor(FunctionTrace* ctor)
{
   auto buff = Singleton< TraceBuffer >::Instance();
   auto& thrd = ThreadInfo[ctor->Nid()];
   auto& chains = thrd.chains;
   auto slot = ctor->Slot();
   auto done = false;
   auto popped = false;

   while(!chains.empty() && !done)
   {
      auto& chain = chains.back();

      switch(chain.CalcAction(ctor, popped))
      {
      case Create:
         done = true;
         break;

      case SetAsFirst:
         chain.ctors_.push_back(ctor);
         return ctor;

      case SetAsNext:
         if(!AddToPreviousChain(ctor))
         {
            chain.SetOuter(ctor);
         }
         return buff->At(slot);

      case SetAsInit:
         if(chain.init_ == nullptr) chain.init_ = ctor;
         return ctor;

      case Finalize:
         thrd.PopBack();
         popped = true;
         break;
      }
   }

   //  If we get here, CTOR starts a new chain.
   //
   chains.push_back(CtorChain(ctor));
   return ctor;
}

//------------------------------------------------------------------------------

TraceRecord* CtorChain::CheckForEndOfChains(const FunctionTrace* curr)
{
   auto& thrd = ThreadInfo[curr->Nid()];
   auto& chains = thrd.chains;
   auto slot = curr->Slot();

   while(!chains.empty())
   {
      if(chains.back().FunctionEndsChain(curr))
         thrd.PopBack();
      else
         break;
   }

   return Singleton< TraceBuffer >::Instance()->At(slot);
}

//------------------------------------------------------------------------------

bool CtorChain::CheckForInitializer(const FunctionTrace* curr)
{
   auto& chains = ThreadInfo[curr->Nid()].chains;
   if(chains.empty()) return true;

   auto& chain = chains.back();
   if(chain.ctors_.empty()) return true;
   if(curr->Depth() != chain.ctors_.back()->Depth()) return false;
   if(chain.opnew_ != nullptr) return false;  //*?
   HandleInitializer(curr);
   return true;
}

//------------------------------------------------------------------------------

bool CtorChain::FunctionEndsChain(const FunctionTrace* curr) const
{
   if(opnew_ != nullptr)
   {
      //  The function must be at the same depth or less as a new operator
      //  to finalize this chain.
      //
      return (curr->Depth() <= opnew_->Depth());
   }

   //  The function must be at a lesser depth than the outer constructor
   //  to finalize this chain.
   //
   return (curr->Depth() < ctors_.back()->Depth());
}

//------------------------------------------------------------------------------

void CtorChain::HandleFunction(FunctionTrace* func)
{
   auto& thrd = ThreadInfo[func->Nid()];

   //  See if this function might have been invoked to initialize
   //  a member of the next constructor's class.
   //
   if(!CtorChain::CheckForInitializer(func))
   {
      //  No, so see if this function's depth is such that it
      //  finalizes any constructor chains.
      //
      CtorChain::CheckForEndOfChains(func);
   }

   //  A new operator precedes a constructor chain whose outer
   //  constructor will be at the same depth as the new operator.
   //
   if(strstr(func->Func(), FunctionName::OpNewTag) != nullptr)
   {
      thrd.chains.push_back(CtorChain(func, 0));
   }
}

//------------------------------------------------------------------------------

void CtorChain::HandleInitializer(const FunctionTrace* init)
{
   //  Only the first function invoked to initialize a member is recorded,
   //  so that the class's constructor can be moved directly above it.
   //
   auto& back = ThreadInfo[init->Nid()].chains.back();
   if(back.init_ == nullptr) back.init_ = init;
}

//------------------------------------------------------------------------------

void CtorChain::MoveOuterAboveInit()
{
   if(!ctors_.empty() && (init_ != nullptr))
   {
      auto outer = ctors_.back();
      Singleton< TraceBuffer >::Instance()->MoveAbove(outer, init_);
      outer->SetTicks(init_->GetTicks());
   }

   init_ = nullptr;
}

//------------------------------------------------------------------------------

void CtorChain::MoveOuterAboveInner() const
{
   if(ctors_.size() > 1)
   {
      auto inner = ctors_.front();
      auto outer = ctors_.back();
      Singleton< TraceBuffer >::Instance()->MoveAbove(outer, inner);
      outer->SetTicks(inner->GetTicks());
   }
}

//------------------------------------------------------------------------------

void CtorChain::SetOuter(FunctionTrace* outer)
{
   ctors_.push_back(outer);

   //  If any function was invoked to initialize a member of OUTER's
   //  class, move OUTER so that it precedes those functions.
   //
   if(init_ != nullptr)
   {
      MoveOuterAboveInit();
   }
}

//------------------------------------------------------------------------------

const Flags FunctionTrace::FTmask = Flags(1 << FunctionTracer);
FunctionTrace::Scope FunctionTrace::Scope_ = FullTrace;
const fn_depth FunctionTrace::MaxDispDepth = 40;

//------------------------------------------------------------------------------

FunctionTrace::FunctionTrace(fn_name_arg func, fn_depth depth) :
   TimedRecord(FunctionTracer),
   func_(func),
   depth_(depth),
   invokerDepth_(0),
   gross_(0),
   net_(0)
{
   if(depth_ < 0) depth_ = 0;
   rid_ = NIL_ID;
}

//------------------------------------------------------------------------------

FunctionTrace::FunctionTrace() :
   TimedRecord(FunctionTracer),
   func_(nullptr),
   depth_(0),
   invokerDepth_(0),
   gross_(0),
   net_(0)
{
   rid_ = NIL_ID;
}

//------------------------------------------------------------------------------

void FunctionTrace::AdjustDepths()
{
   auto buff = Singleton< TraceBuffer >::Instance();
   TraceRecord* rec = nullptr;
   auto mask = FTmask;

   auto minDepth = INT16_MAX;

   for(buff->Next(rec, mask); rec != nullptr; buff->Next(rec, mask))
   {
      auto curr = static_cast< FunctionTrace* >(rec);
      if(curr->depth_ < minDepth) minDepth = curr->depth_;
   }

   //  Decrement minDepth before subtracting it from each function's depth_
   //  and invokerDepth_.  A value of 0 for the latter represents the C++
   //  run-time, so we don't want any functions claiming to be at depth 0.
   //
   --minDepth;

   rec = nullptr;

   for(buff->Next(rec, mask); rec != nullptr; buff->Next(rec, mask))
   {
      auto curr = static_cast< FunctionTrace* >(rec);
      curr->depth_ -= minDepth;
      curr->invokerDepth_ -= minDepth;
   }
}

//------------------------------------------------------------------------------

void FunctionTrace::CalcFuncTimes()
{
   auto buff = Singleton< TraceBuffer >::Instance();
   TraceRecord* rec = nullptr;
   auto mask = FTmask;

   //  Find the gross and net time spent in each function and adjust
   //  each function's depth to avoid unnecessary indentation.
   //
   for(buff->Next(rec, mask); rec != nullptr; buff->Next(rec, mask))
   {
      auto curr = static_cast< FunctionTrace* >(rec);
      curr->CalcTimes();
   }
}

//------------------------------------------------------------------------------

usecs_t FunctionTrace::CalcGrossTime()
{
   auto buff = Singleton< TraceBuffer >::Instance();
   auto mask = FTmask;
   TraceRecord* rec = this;
   FunctionTrace* prev = this;
   auto nid = Nid();
   ticks_t others = 0;

   //  A function's gross time is the time between when it was invoked and
   //  when the next function at the same (or lower) depth was invoked--in
   //  the same thread and during the same transaction.  Subtract any time
   //  spent in other threads.
   //
   for(buff->Next(rec, mask); rec != nullptr; buff->Next(rec, mask))
   {
      auto curr = static_cast< FunctionTrace* >(rec);

      if(prev->Nid() != nid)
      {
         others += (curr->GetTicks() - prev->GetTicks());
      }

      if(curr->Nid() == nid)
      {
         if(curr->depth_ <= depth_)
         {
            auto gross = curr->GetTicks() - GetTicks() - others;
            return Clock::TicksToUsecs(gross);
         }
      }

      prev = curr;
   }

   auto gross = prev->GetTicks() - GetTicks() - others;
   return Clock::TicksToUsecs(gross);
}

//------------------------------------------------------------------------------

void FunctionTrace::CalcTimes()
{
   auto buff = Singleton< TraceBuffer >::Instance();
   auto mask = FTmask;
   auto nid = Nid();
   TraceRecord* rec = this;

   //  Start by calculating the gross time for this function.
   //
   gross_ = CalcGrossTime();

   //  The net time for a function at depth_ is its gross time minus
   //  the sum of all gross times spent in functions at depth_ + 1
   //  on the same thread.
   //
   net_ = gross_;
   if(net_ == 0) return;

   for(buff->Next(rec, mask); rec != nullptr; buff->Next(rec, mask))
   {
      auto curr = static_cast< FunctionTrace* >(rec);

      if(curr->Nid() != nid) continue;
      if(curr->depth_ <= depth_) return;

      if(curr->depth_ == (depth_ + 1))
      {
         net_ -= curr->CalcGrossTime();
      }
   }
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

   auto buff = Singleton< TraceBuffer >::Instance();
   auto depth = SysThreadStack::FuncDepth() - 3;

   //  If this is a destructor call that is not one level deeper than the last
   //  destructor or function, add a call to a compiler-generated "C++.delete"
   //  function.  The compiler generates this code to invoke a delete operator
   //  after a destructor.
   //
   if(strstr(func, FunctionName::DtorTag) != nullptr)
   {
      auto nid = SysThread::RunningThreadId();
      auto insert = (buff->LastDtorDepth(nid) != depth - 1);

      if(!insert)
      {
         auto rec = buff->LastFunction(nid);

         if(rec != nullptr)
         {
            insert = (rec->depth_ != depth - 1);
         }
      }

      if(insert)
      {
         auto rec = new FunctionTrace(Cxx_delete, depth - 1);
         buff->Insert(rec);
      }
   }

   auto rec = new FunctionTrace(func, depth);
   buff->Insert(rec);
}

//------------------------------------------------------------------------------

bool FunctionTrace::Display(ostream& stream, bool diff)
{
   if(!TimedRecord::Display(stream, diff)) return false;

   //  Suppress timing information if a >diff is planned.  Display each
   //  function's depth and its invoker's apparent depth instead.
   //
   usecs_t gross = (diff ? depth_ : gross_);
   usecs_t net = (diff ? invokerDepth_ : net_);

   stream << setw(TraceDump::TotWidth) << gross << TraceDump::Tab();
   stream << setw(TraceDump::NetWidth) << net << TraceDump::Tab();

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
   return true;
}

//------------------------------------------------------------------------------

bool FunctionTrace::FindDeleteOperator()
{
   auto buff = Singleton< TraceBuffer >::Instance();
   TraceRecord* rec = this;
   auto mask = FTmask;
   auto nid = Nid();
   auto stop = Depth();

   for(buff->Next(rec, mask); rec != nullptr; buff->Next(rec, mask))
   {
      auto curr = static_cast< FunctionTrace* >(rec);
      if(curr->Nid() != nid) continue;

      auto depth = curr->Depth();
      if(depth > stop + 1) continue;
      if(depth <= stop) return false;
      if(strstr(curr->func_, FunctionName::OpDelTag) != nullptr) return true;
   }

   return false;
}

//------------------------------------------------------------------------------

void FunctionTrace::FindInvokerDepths()
{
   auto buff = Singleton< TraceBuffer >::Instance();
   TraceRecord* rec = nullptr;
   auto mask = FTmask;

   for(buff->Next(rec, mask); rec != nullptr; buff->Next(rec, mask))
   {
      auto curr = static_cast< FunctionTrace* >(rec);
      auto& thrd = ThreadInfo[curr->Nid()];
      while(thrd.depths.top() >= curr->depth_) thrd.depths.pop();
      curr->invokerDepth_ = thrd.depths.top();
      thrd.depths.push(curr->depth_);
   }
}

//------------------------------------------------------------------------------

void FunctionTrace::FixCtorChains()
{
   auto buff = Singleton< TraceBuffer >::Instance();
   TraceRecord* rec = nullptr;
   auto mask = FTmask;

   for(buff->Next(rec, mask); rec != nullptr; buff->Next(rec, mask))
   {
      auto curr = static_cast< FunctionTrace* >(rec);

      //  Constructors are analyzed differently than other functions.
      //
      auto ctor = (strstr(curr->func_, FunctionName::CtorTag) != nullptr);

      if(ctor)
         rec = CtorChain::HandleCtor(curr);
      else
         CtorChain::HandleFunction(curr);
   }

   //  Finalize any chain whose resolution is still pending.
   //
   for(auto i = ThreadInfo.begin(); i != ThreadInfo.end(); ++i)
   {
      while(!i->second.chains.empty())
      {
         i->second.PopBack();
      }
   }
}

//------------------------------------------------------------------------------

void* FunctionTrace::operator new(size_t size)
{
   return Singleton< TraceBuffer >::Instance()->AddFunction();
}

//------------------------------------------------------------------------------

void* FunctionTrace::operator new(size_t size, void* where)
{
   return where;
}

//------------------------------------------------------------------------------

fn_name FunctionTrace_Process = "FunctionTrace.Process";

void FunctionTrace::Process()
{
   Debug::ft(FunctionTrace_Process);

   //  If the trace records have already been processed, don't process
   //  them again.
   //
   auto buff = Singleton< TraceBuffer >::Instance();
   if(buff->HasBeenProcessed()) return;

   buff->Lock();
      ThreadInfo.clear();
      AdjustDepths();
      FindInvokerDepths();
      RemoveCxxDeletes();
      FixCtorChains();
      CalcFuncTimes();
   buff->Unlock();
}

//------------------------------------------------------------------------------

fn_name FunctionTrace_RemoveCxxDeletes = "FunctionTrace.RemoveCxxDeletes";

void FunctionTrace::RemoveCxxDeletes()
{
   Debug::ft(FunctionTrace_RemoveCxxDeletes);

   auto buff = Singleton< TraceBuffer >::Instance();
   TraceRecord* rec = nullptr;
   auto mask = FTmask;

   for(buff->Next(rec, mask); rec != nullptr; buff->Next(rec, mask))
   {
      auto curr = static_cast< FunctionTrace* >(rec);

      if(strcmp(curr->func_, Cxx_delete) == 0)
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
      }
   }
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
