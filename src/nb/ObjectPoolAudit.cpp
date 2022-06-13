//==============================================================================
//
//  ObjectPoolAudit.cpp
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
#include "ObjectPoolAudit.h"
#include <ostream>
#include <ratio>
#include <string>
#include "Debug.h"
#include "NbDaemons.h"
#include "ObjectPoolRegistry.h"
#include "Singleton.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
ObjectPoolAudit::ObjectPoolAudit() :
   Thread(AuditFaction, Singleton<ObjectDaemon>::Instance()),
   interval_(msecs_t(5000)),
   phase_(CheckingFreeq),
   pid_(NIL_ID)
{
   Debug::ft("ObjectPoolAudit.ctor");

   SetInitialized();
}

//------------------------------------------------------------------------------

ObjectPoolAudit::~ObjectPoolAudit()
{
   Debug::ftnt("ObjectPoolAudit.dtor");
}

//------------------------------------------------------------------------------

c_string ObjectPoolAudit::AbbrName() const
{
   return ObjectDaemonName;
}

//------------------------------------------------------------------------------

void ObjectPoolAudit::Destroy()
{
   Debug::ft("ObjectPoolAudit.Destroy");

   Singleton<ObjectPoolAudit>::Destroy();
}

//------------------------------------------------------------------------------

void ObjectPoolAudit::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Thread::Display(stream, prefix, options);

   stream << prefix << "interval : " << to_string(interval_) << CRLF;
   stream << prefix << "phase    : " << phase_ << CRLF;
   stream << prefix << "pid      : " << int(pid_) << CRLF;
}

//------------------------------------------------------------------------------

void ObjectPoolAudit::Enter()
{
   Debug::ft("ObjectPoolAudit.Enter");

   //  Audit blocks forever.
   //
   while(true)
   {
      Pause(interval_);
      Singleton<ObjectPoolRegistry>::Instance()->AuditPools();
   }
}

//------------------------------------------------------------------------------

void ObjectPoolAudit::Patch(sel_t selector, void* arguments)
{
   Thread::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

void ObjectPoolAudit::SetInterval(const msecs_t& interval)
{
   Debug::ft("ObjectPoolAudit.SetInterval");

   auto prev = interval_;
   interval_ = interval;

   //  If the thread was sleeping forever and has now been enabled, wake it.
   //
   if((prev == TIMEOUT_NEVER) && (interval_ != TIMEOUT_NEVER)) Interrupt();
}
}
