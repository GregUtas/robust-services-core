//==============================================================================
//
//  LeakyBucketCounter.cpp
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
#include "LeakyBucketCounter.h"
#include <ostream>
#include <string>
#include "Debug.h"
#include "SysTypes.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
fn_name LeakyBucketCounter_ctor = "LeakyBucketCounter.ctor";

LeakyBucketCounter::LeakyBucketCounter() :
   limit_(0),
   count_(0)
{
   Debug::ft(LeakyBucketCounter_ctor);
}

//------------------------------------------------------------------------------

fn_name LeakyBucketCounter_dtor = "LeakyBucketCounter.dtor";

LeakyBucketCounter::~LeakyBucketCounter()
{
   Debug::ftnt(LeakyBucketCounter_dtor);
}

//------------------------------------------------------------------------------

void LeakyBucketCounter::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Object::Display(stream, prefix, options);

   stream << prefix << "interval : " << interval_.to_str(mSECS) << CRLF;
   stream << prefix << "lastTime : " << lastTime_.Ticks() << CRLF;
   stream << prefix << "limit    : " << limit_ << CRLF;
   stream << prefix << "count    : " << count_ << CRLF;
}

//------------------------------------------------------------------------------

fn_name LeakyBucketCounter_HasReachedLimit =
   "LeakyBucketCounter.HasReachedLimit";

bool LeakyBucketCounter::HasReachedLimit()
{
   Debug::ft(LeakyBucketCounter_HasReachedLimit);

   //  Return false if the bucket has not been initialized.
   //
   if(limit_ == 0) return false;

   //  Calculate the number of events that have drained
   //  from the bucket since the last event occurred.
   //
   auto now = TimePoint::Now();
   auto elapsed = now - lastTime_;
   auto debits = elapsed.Ticks() / (interval_.Ticks() / limit_);

   //  If the bucket isn't empty, drain events.
   //
   if((count_ > 0) && (debits > 0))
   {
      if(debits < count_)
      {
         count_ -= debits;
         lastTime_ += debits * (interval_ / limit_);
      }
      else
      {
         count_ = 0;
      }
   }

   //  If the bucket is empty, it only starts to drain now.
   //
   if(count_ == 0) lastTime_ = now;

   //  Add an event to the bucket.  If it overflows, empty
   //  it and return true.
   //
   if(++count_ > limit_)
   {
      count_ = 0;
      lastTime_ = now;
      return true;
   }

   return false;
}

//------------------------------------------------------------------------------

fn_name LeakyBucketCounter_Initialize = "LeakyBucketCounter.Initialize";

void LeakyBucketCounter::Initialize(size_t limit, secs_t seconds)
{
   Debug::ft(LeakyBucketCounter_Initialize);

   interval_ = Duration(seconds, SECS);
   lastTime_ = TimePoint::Now();
   limit_ = limit;
   count_ = 0;
}

//------------------------------------------------------------------------------

void LeakyBucketCounter::Patch(sel_t selector, void* arguments)
{
   Object::Patch(selector, arguments);
}
}
