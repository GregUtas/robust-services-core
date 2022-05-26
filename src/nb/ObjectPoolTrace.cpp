//==============================================================================
//
//  ObjectPoolTrace.cpp
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
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
ObjectPoolTrace::ObjectPoolTrace(Id rid, const Pooled& obj) :
   TimedRecord(ObjPoolTracer),
   obj_(&obj),
   pid_(ObjectPool::ObjPid(&obj))
{
   rid_ = rid;
}

//------------------------------------------------------------------------------

bool ObjectPoolTrace::Display(ostream& stream, const string& opts)
{
   if(!TimedRecord::Display(stream, opts)) return false;

   auto pool = Singleton<ObjectPoolRegistry>::Instance()->Pool(pid_);

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

c_string ObjectPoolTrace::EventString() const
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
