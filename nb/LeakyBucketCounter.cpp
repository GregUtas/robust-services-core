//==============================================================================
//
//  LeakyBucketCounter.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
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
   interval_(0),
   lastTime_(0),
   limit_(0),
   count_(0)
{
   Debug::ft(LeakyBucketCounter_ctor);
}

//------------------------------------------------------------------------------

fn_name LeakyBucketCounter_dtor = "LeakyBucketCounter.dtor";

LeakyBucketCounter::~LeakyBucketCounter()
{
   Debug::ft(LeakyBucketCounter_dtor);
}

//------------------------------------------------------------------------------

void LeakyBucketCounter::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Object::Display(stream, prefix, options);

   stream << prefix << "interval : " << interval_ << CRLF;
   stream << prefix << "lastTime : " << lastTime_ << CRLF;
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
   ticks_t now;
   auto elapsed = Clock::TicksSince(lastTime_, now);
   auto debits = elapsed / (interval_ / limit_);

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

   interval_ = Clock::SecsToTicks(seconds);
   lastTime_ = Clock::TicksNow();
   limit_ = limit;
   count_ = 0;
}

//------------------------------------------------------------------------------

void LeakyBucketCounter::Patch(sel_t selector, void* arguments)
{
   Object::Patch(selector, arguments);
}
}
