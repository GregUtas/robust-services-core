//==============================================================================
//
//  ObjectPoolRegistry.cpp
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
#include "ObjectPoolRegistry.h"
#include "StatisticsGroup.h"
#include "Tool.h"
#include <ostream>
#include <string>
#include "Algorithms.h"
#include "CfgBoolParm.h"
#include "CfgParmRegistry.h"
#include "Debug.h"
#include "Formatters.h"
#include "NbCliParms.h"
#include "ObjectPool.h"
#include "ObjectPoolAudit.h"
#include "Singleton.h"
#include "SysTypes.h"
#include "ThisThread.h"
#include "ToolTypes.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
fixed_string ObjPoolTraceToolName = "ObjPoolTracer";
fixed_string ObjPoolTraceToolExpl = "traces pooled objects";

class ObjPoolTraceTool : public Tool
{
   friend class Singleton< ObjPoolTraceTool >;
private:
   ObjPoolTraceTool() : Tool(ObjPoolTracer, 'o', true) { }
   c_string Name() const override { return ObjPoolTraceToolName; }
   c_string Expl() const override { return ObjPoolTraceToolExpl; }
};

//------------------------------------------------------------------------------

class ObjectPoolStatsGroup : public StatisticsGroup
{
public:
   ObjectPoolStatsGroup();
   ~ObjectPoolStatsGroup();
   void DisplayStats
      (ostream& stream, id_t id, const Flags& options) const override;
};

//------------------------------------------------------------------------------

fn_name ObjectPoolStatsGroup_ctor = "ObjectPoolStatsGroup.ctor";

ObjectPoolStatsGroup::ObjectPoolStatsGroup() :
   StatisticsGroup("Object Pools [ObjectPoolId]")
{
   Debug::ft(ObjectPoolStatsGroup_ctor);
}

//------------------------------------------------------------------------------

fn_name ObjectPoolStatsGroup_dtor = "ObjectPoolStatsGroup.dtor";

ObjectPoolStatsGroup::~ObjectPoolStatsGroup()
{
   Debug::ft(ObjectPoolStatsGroup_dtor);
}

//------------------------------------------------------------------------------

fn_name ObjectPoolStatsGroup_DisplayStats = "ObjectPoolStatsGroup.DisplayStats";

void ObjectPoolStatsGroup::DisplayStats
   (ostream& stream, id_t id, const Flags& options) const
{
   Debug::ft(ObjectPoolStatsGroup_DisplayStats);

   StatisticsGroup::DisplayStats(stream, id, options);

   auto reg = Singleton< ObjectPoolRegistry >::Instance();

   if(id == 0)
   {
      auto& pools = reg->Pools();

      for(auto p = pools.First(); p != nullptr; pools.Next(p))
      {
         p->DisplayStats(stream, options);
      }
   }
   else
   {
      auto p = reg->Pool(id);

      if(p == nullptr)
      {
         stream << spaces(2) << NoPoolExpl << CRLF;
         return;
      }

      p->DisplayStats(stream, options);
   }
}

//==============================================================================

bool ObjectPoolRegistry::NullifyObjectData_ = false;

//------------------------------------------------------------------------------

fn_name ObjectPoolRegistry_ctor = "ObjectPoolRegistry.ctor";

ObjectPoolRegistry::ObjectPoolRegistry()
{
   Debug::ft(ObjectPoolRegistry_ctor);

   Singleton< ObjPoolTraceTool >::Instance();
   pools_.Init(ObjectPool::MaxId, ObjectPool::CellDiff(), MemPersistent);
   statsGroup_.reset(new ObjectPoolStatsGroup);
   nullifyObjectData_.reset(new CfgBoolParm("NullifyObjectData", "F",
      &NullifyObjectData_, "set to nullify the data after an object's vptr"));
   Singleton< CfgParmRegistry >::Instance()->BindParm(*nullifyObjectData_);
}

//------------------------------------------------------------------------------

fn_name ObjectPoolRegistry_dtor = "ObjectPoolRegistry.dtor";

ObjectPoolRegistry::~ObjectPoolRegistry()
{
   Debug::ft(ObjectPoolRegistry_dtor);
}

//------------------------------------------------------------------------------

fn_name ObjectPoolRegistry_AuditPools = "ObjectPoolRegistry.AuditPools";

void ObjectPoolRegistry::AuditPools() const
{
   Debug::ft(ObjectPoolRegistry_AuditPools);

   auto thread = Singleton< ObjectPoolAudit >::Instance();

   //  This code is stateful.  When it is reentered after an exception, it
   //  resumes execution at the phase and pool where the exception occurred.
   //
   while(true)
   {
      switch(thread->phase_)
      {
      case ObjectPoolAudit::CheckingFreeq:
         //
         //  Audit each pool's free queue.
         //
         while(thread->pid_ <= ObjectPool::MaxId)
         {
            auto pool = pools_.At(thread->pid_);

            if(pool != nullptr)
            {
               pool->AuditFreeq();
               ThisThread::Pause();
            }

            ++thread->pid_;
         }

         thread->phase_ = ObjectPoolAudit::ClaimingBlocks;
         thread->pid_ = NIL_ID;
         //  [[fallthrough]]

      case ObjectPoolAudit::ClaimingBlocks:
         //
         //  Claim in-use blocks in each pool.  Each ClaimBlocks function
         //  finds its blocks in an application-specific way.  The blocks
         //  must be claimed after *all* blocks, in *all* pools, have been
         //  marked, because some ClaimBlocks functions claim blocks from
         //  multiple pools.
         //
         while(thread->pid_ <= ObjectPool::MaxId)
         {
            auto pool = pools_.At(thread->pid_);

            if(pool != nullptr)
            {
               pool->ClaimBlocks();
               ThisThread::Pause();
            }

            ++thread->pid_;
         }

         thread->phase_ = ObjectPoolAudit::RecoveringBlocks;
         thread->pid_ = NIL_ID;
         //  [[fallthrough]]

      case ObjectPoolAudit::RecoveringBlocks:
         //
         //  For each object pool, recover any block that is still marked.
         //  Such a block is an orphan that is neither on the free queue
         //  nor in use by an application.
         //
         while(thread->pid_ <= ObjectPool::MaxId)
         {
            auto pool = pools_.At(thread->pid_);

            if(pool != nullptr)
            {
               pool->RecoverBlocks();
               ThisThread::Pause();
            }

            ++thread->pid_;
         }

         thread->phase_ = ObjectPoolAudit::CheckingFreeq;
         thread->pid_ = NIL_ID;
         return;

      default:
         //
         //  An unknown phase.
         //
         Debug::SwLog(ObjectPoolRegistry_AuditPools,
            "unexpected phase", pack2(thread->pid_, thread->phase_));
         thread->phase_ = ObjectPoolAudit::CheckingFreeq;
         thread->pid_ = NIL_ID;
         return;
      }
   }
}

//------------------------------------------------------------------------------

fn_name ObjectPoolRegistry_BindPool = "ObjectPoolRegistry.BindPool";

bool ObjectPoolRegistry::BindPool(ObjectPool& pool)
{
   Debug::ft(ObjectPoolRegistry_BindPool);

   return pools_.Insert(pool);
}

//------------------------------------------------------------------------------

void ObjectPoolRegistry::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Persistent::Display(stream, prefix, options);

   stream << prefix << "statsGroup        : ";
   stream << strObj(statsGroup_.get()) << CRLF;
   stream << prefix << "NullifyObjectData : ";
   stream << NullifyObjectData_ << CRLF;
   stream << prefix << "nullifyObjectData : ";
   stream << strObj(nullifyObjectData_.get()) << CRLF;

   stream << prefix << "pools [ObjectPoolId]" << CRLF;
   pools_.Display(stream, prefix + spaces(2), options);
}

//------------------------------------------------------------------------------

void ObjectPoolRegistry::Patch(sel_t selector, void* arguments)
{
   Persistent::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

ObjectPool* ObjectPoolRegistry::Pool(ObjectPoolId pid) const
{
   return pools_.At(pid);
}

//------------------------------------------------------------------------------

fn_name ObjectPoolRegistry_Shutdown = "ObjectPoolRegistry.Shutdown";

void ObjectPoolRegistry::Shutdown(RestartLevel level)
{
   Debug::ft(ObjectPoolRegistry_Shutdown);

   for(auto p = pools_.Last(); p != nullptr; pools_.Prev(p))
   {
      p->Shutdown(level);
   }

   if(level < RestartCold) return;

   statsGroup_.release();
}

//------------------------------------------------------------------------------

fn_name ObjectPoolRegistry_Startup = "ObjectPoolRegistry.Startup";

void ObjectPoolRegistry::Startup(RestartLevel level)
{
   Debug::ft(ObjectPoolRegistry_Startup);

   if(statsGroup_ == nullptr) statsGroup_.reset(new ObjectPoolStatsGroup);

   for(auto p = pools_.First(); p != nullptr; pools_.Next(p))
   {
      p->Startup(level);
   }
}

//------------------------------------------------------------------------------

fn_name ObjectPoolRegistry_UnbindPool = "ObjectPoolRegistry.UnbindPool";

void ObjectPoolRegistry::UnbindPool(ObjectPool& pool)
{
   Debug::ft(ObjectPoolRegistry_UnbindPool);

   pools_.Erase(pool);
}
}
