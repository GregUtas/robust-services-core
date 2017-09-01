//==============================================================================
//
//  ObjectPoolTrace.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "ObjectPoolTrace.h"
#include <ostream>
#include <string>
#include "Formatters.h"
#include "ObjectPool.h"
#include "ObjectPoolRegistry.h"
#include "Singleton.h"
#include "SysTypes.h"
#include "ToolTypes.h"
#include "TraceDump.h"

using std::ostream;

//------------------------------------------------------------------------------

namespace NodeBase
{
ObjectPoolTrace::ObjectPoolTrace(Id rid, const Pooled& obj) :
   TimedRecord(sizeof(ObjectPoolTrace), ObjPoolTracer),
   obj_(&obj),
   pid_(ObjectPool::ObjPid(&obj))
{
   rid_ = rid;
}

//------------------------------------------------------------------------------

bool ObjectPoolTrace::Display(ostream& stream)
{
   if(!TimedRecord::Display(stream)) return false;

   auto pool = Singleton< ObjectPoolRegistry >::Instance()->Pool(pid_);

   stream << spaces(TraceDump::EvtToObj) << obj_ << TraceDump::Tab();

   if(pool != nullptr)
      stream << strClass(pool);
   else
      stream << "poolid=" << int(pid_);

   return true;
}

//------------------------------------------------------------------------------

fixed_string ObjDeqEventStr   = "  deq";
fixed_string ObjEnqEventStr   = "  enq";
fixed_string ObjClaimEventStr = "claim";
fixed_string ObjRecoverStr    = "recov";

const char* ObjectPoolTrace::EventString() const
{
   switch(rid_)
   {
   case Dequeued: return ObjDeqEventStr;
   case Enqueued: return ObjEnqEventStr;
   case Claimed: return ObjClaimEventStr;
   case Recovered: return ObjRecoverStr;
   }

   return ERROR_STR;
}
}
