//==============================================================================
//
//  FunctionProfiler.cpp
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
#include "FunctionProfiler.h"
#include <map>
#include <ostream>
#include <string>
#include <utility>
#include "Algorithms.h"
#include "Debug.h"
#include "FunctionName.h"
#include "FunctionStats.h"
#include "FunctionTrace.h"
#include "Memory.h"
#include "Singleton.h"
#include "Thread.h"
#include "ThreadRegistry.h"
#include "TraceBuffer.h"

using std::ostream;

//------------------------------------------------------------------------------

namespace NodeTools
{
const size_t FunctionProfiler::HashTableSizeLog2 = 10;
const uint32_t FunctionProfiler::HashMask = ((1 << HashTableSizeLog2) - 1);

//------------------------------------------------------------------------------

fn_name FunctionProfiler_ctor = "FunctionProfiler.ctor";

FunctionProfiler::FunctionProfiler() :
   size_(1 << HashTableSizeLog2),
   functionq_(nullptr)
{
   Debug::ft(FunctionProfiler_ctor);

   auto size = sizeof(Q2Way< FunctionStats >) * size_;
   functionq_ = (Q2Way< FunctionStats >*) Memory::Alloc(size, MemTemp);

   for(size_t i = 0; i < size_; ++i)
   {
      new (&functionq_[i]) Q2Way< FunctionStats >();
      functionq_[i].Init(FunctionStats::LinkDiff());
   }

   holdq_.Init(FunctionStats::LinkDiff());
   sortq_.Init(FunctionStats::LinkDiff());
}

//------------------------------------------------------------------------------

fn_name FunctionProfiler_dtor = "FunctionProfiler.dtor";

FunctionProfiler::~FunctionProfiler()
{
   Debug::ft(FunctionProfiler_dtor);

   //  Delete all of the data that was allocated to generate the statistics.
   //
   sortq_.Purge();
   holdq_.Purge();

   if(functionq_ != nullptr)
   {
      for(size_t i = 0; i < size_; ++i)
      {
         functionq_[i].Purge();
      }
   }

   Memory::Free(functionq_);
   functionq_ = nullptr;
}

//------------------------------------------------------------------------------

void FunctionProfiler::CheckHigh(FunctionStats*& high,
   FunctionStats* curr, int sort1, int sort2, int sort3)
{
   //  SORTn > 0 if HIGH > CURR, 0 (if HIGH = CURR), or < 0 (if HIGH < CURR).
   //
   if(sort1 < 0)
   {
      high = curr;
      return;
   }

   if(sort1 > 0) return;

   if(sort2 < 0)
   {
      high = curr;
      return;
   }

   if(sort2 > 0) return;

   if(sort3 < 0) high = curr;
}

//------------------------------------------------------------------------------

fn_name FunctionProfiler_EnsureRecord = "FunctionProfiler.EnsureRecord";

FunctionStats* FunctionProfiler::EnsureRecord(fn_name_arg func, size_t count)
{
   Debug::ft(FunctionProfiler_EnsureRecord);

   auto index = (stringHash(func) & HashMask);
   auto fsq = &functionq_[index];

   //  Search the hash location to see if an entry for this function
   //  already exists.
   //
   for(auto f = fsq->First(); f != nullptr; fsq->Next(f))
   {
      if(FunctionName::compare(f->Func(), func) == 0) return f;
   }

   auto f = new FunctionStats(func, count);
   fsq->Enq(*f);
   return f;
}

//------------------------------------------------------------------------------

fn_name FunctionProfiler_Generate = "FunctionProfiler.Generate";

TraceRc FunctionProfiler::Generate(ostream& stream, Sort sort)
{
   Debug::ft(FunctionProfiler_Generate);

   auto buff = Singleton< TraceBuffer >::Instance();
   auto rc = TraceOk;

   switch(FunctionTrace::GetScope())
   {
   case FunctionTrace::CountsOnly:
      //
      //  Transfer the function invocation records to FunctionStats records
      //  to prepare for sorting.
      //
      if(Debug::TraceOn())
      {
         return NotWhileTracing;
      }
      else
      {
         auto& data = buff->GetInvocations();

         if(data.empty()) return BufferEmpty;

         for(auto rec = data.cbegin(); rec != data.cend(); ++rec)
         {
            EnsureRecord(rec->first, rec->second);
         }
      }
      break;

   case FunctionTrace::FullTrace:
      //
      //  Extract function calls occurring on the threads to be included in the
      //  report (thread=0 or a thread that no longer exists is of interest and
      //  is therefore always included).  After ensuring that a function has a
      //  FunctionStats record, increment the number of times it was invoked and
      //  accumulate the total net time spent in it.
      //
      if(buff->Empty()) return BufferEmpty;

      buff->Lock();
      {
         TraceRecord* rec = nullptr;
         auto mask = FunctionTrace::FTmask;
         auto reg = Singleton< ThreadRegistry >::Instance();

         for(buff->Next(rec, mask); rec != nullptr; buff->Next(rec, mask))
         {
            auto ft = static_cast<FunctionTrace*>(rec);
            auto in = (ft->Tid() == NIL_ID);

            if(!in)
            {
               auto thr = reg->GetThread(ft->Tid());
               if(thr == nullptr)
                  in = true;
               else
                  in = (thr->CalcStatus(false) == TraceIncluded);
            }

            if(in)
            {
               auto fs = EnsureRecord(ft->Func(), 0);
               fs->IncrCalls(ft->Net());
            }
         }
      }

      buff->Unlock();
      break;

   default:
      Debug::SwErr(FunctionProfiler_Generate, FunctionTrace::GetScope(), 0);
      return NothingToDisplay;
   }

   rc = Show(stream, sort);
   return rc;
}

//------------------------------------------------------------------------------

fixed_string FpHeader    = "FUNCTION PROFILE";
fixed_string FpColumns   = "    Calls       uSecs   Function";
fixed_string FpSeparator = "    -----       -----   --------";

fn_name FunctionProfiler_Show = "FunctionProfiler.Show";

TraceRc FunctionProfiler::Show(ostream& stream, Sort sort)
{
   Debug::ft(FunctionProfiler_Show);

   auto buff = Singleton< TraceBuffer >::Instance();
   stream << FpHeader << buff->strTimePlace() << CRLF << CRLF;

   stream << FpColumns << CRLF;
   stream << FpSeparator << CRLF;

   //  Gather all of the entries into the holding queue.
   //
   for(size_t i = 0; i < size_; ++i)
   {
      auto fsq = &functionq_[i];

      for(auto f = fsq->Deq(); f != nullptr; f = fsq->Deq())
      {
         holdq_.Enq(*f);
      }
   }

   if(holdq_.Empty())
   {
      stream << "Nothing to display." << CRLF;
      return NothingToDisplay;
   }

   //  Selection sort the functions based on SORT.
   //
   while(!holdq_.Empty())
   {
      auto high = holdq_.First();

      for(auto curr = holdq_.Next(*high); curr != nullptr; holdq_.Next(curr))
      {
         auto csort = 0;  // sort by calls
         auto tsort = 0;  // sort by time

         if(high->Calls() > curr->Calls())
            csort = 1;
         else if(high->Calls() < curr->Calls())
            csort = -1;

         if(high->Time() > curr->Time())
            tsort = 1;
         else if(high->Time() < curr->Time())
            tsort = -1;

         auto nsort = curr->Compare(*high);

         switch(sort)
         {
         case ByCalls:
            CheckHigh(high, curr, csort, tsort, nsort);
            break;
         case ByTimes:
            CheckHigh(high, curr, tsort, csort, nsort);
            break;
         case ByNames:
            CheckHigh(high, curr, nsort, csort, tsort);
            break;
         }
      }

      holdq_.Exq(*high);
      sortq_.Enq(*high);
   }

   //  Output the function statistics.
   //
   for(auto f = sortq_.First(); f != nullptr; sortq_.Next(f))
   {
      f->Display(stream, EMPTY_STR, NoFlags);
   }

   stream << CRLF;
   stream << "Total functions: " << sortq_.Size() << CRLF;

   return TraceOk;
}
}
