//==============================================================================
//
//  ObjectPoolRegistry.cpp
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
#include "ObjectPoolRegistry.h"
#include "StatisticsGroup.h"
#include "Tool.h"
#include <iomanip>
#include <ostream>
#include <string>
#include "Algorithms.h"
#include "CfgParmRegistry.h"
#include "Debug.h"
#include "Formatters.h"
#include "FunctionGuard.h"
#include "NbCliParms.h"
#include "ObjectPool.h"
#include "ObjectPoolAudit.h"
#include "Restart.h"
#include "Singleton.h"
#include "SysTypes.h"
#include "ThisThread.h"
#include "ToolTypes.h"

using std::ostream;
using std::setw;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
fixed_string ObjPoolTraceToolName = "ObjPoolTracer";
fixed_string ObjPoolTraceToolExpl = "traces pooled objects";

class ObjPoolTraceTool : public Tool
{
   friend class Singleton<ObjPoolTraceTool>;

   ObjPoolTraceTool() : Tool(ObjPoolTracer, 'o', true) { }
   ~ObjPoolTraceTool() = default;
   c_string Expl() const override { return ObjPoolTraceToolExpl; }
   c_string Name() const override { return ObjPoolTraceToolName; }
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

ObjectPoolStatsGroup::ObjectPoolStatsGroup() :
   StatisticsGroup("Object Pools [ObjectPoolId]")
{
   Debug::ft("ObjectPoolStatsGroup.ctor");
}

//------------------------------------------------------------------------------

ObjectPoolStatsGroup::~ObjectPoolStatsGroup()
{
   Debug::ftnt("ObjectPoolStatsGroup.dtor");
}

//------------------------------------------------------------------------------

void ObjectPoolStatsGroup::DisplayStats
   (ostream& stream, id_t id, const Flags& options) const
{
   Debug::ft("ObjectPoolStatsGroup.DisplayStats");

   StatisticsGroup::DisplayStats(stream, id, options);

   auto reg = Singleton<ObjectPoolRegistry>::Instance();

   if(id == 0)
   {
      const auto& pools = reg->Pools();

      for(auto p = pools.First(); p != nullptr; pools.Next(p))
      {
         p->DisplayStats(stream, options);
      }
   }
   else
   {
      auto p = reg->Pools().At(id);

      if(p == nullptr)
      {
         stream << spaces(2) << NoPoolExpl << CRLF;
         return;
      }

      p->DisplayStats(stream, options);
   }
}

//==============================================================================

ObjectPoolRegistry::ObjectPoolRegistry()
{
   Debug::ft("ObjectPoolRegistry.ctor");

   Singleton<ObjPoolTraceTool>::Instance();
   pools_.Init(ObjectPool::MaxId, ObjectPool::CellDiff(), MemProtected);
   statsGroup_.reset(new ObjectPoolStatsGroup);
   nullifyObjectDataCfg_.reset(new CfgBoolParm("NullifyObjectData", "F",
      "set to nullify the data after an object's vptr"));
   Singleton<CfgParmRegistry>::Instance()->BindParm(*nullifyObjectDataCfg_);
}

//------------------------------------------------------------------------------

fn_name ObjectPoolRegistry_dtor = "ObjectPoolRegistry.dtor";

ObjectPoolRegistry::~ObjectPoolRegistry()
{
   Debug::ftnt(ObjectPoolRegistry_dtor);

   Debug::SwLog(ObjectPoolRegistry_dtor, UnexpectedInvocation, 0);
}

//------------------------------------------------------------------------------

fn_name ObjectPoolRegistry_AuditPools = "ObjectPoolRegistry.AuditPools";

void ObjectPoolRegistry::AuditPools() const
{
   Debug::ft(ObjectPoolRegistry_AuditPools);

   auto thread = Singleton<ObjectPoolAudit>::Instance();

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
         [[fallthrough]];

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
         [[fallthrough]];

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

bool ObjectPoolRegistry::BindPool(ObjectPool& pool)
{
   Debug::ft("ObjectPoolRegistry.BindPool");

   FunctionGuard guard(Guard_MemUnprotect);
   return pools_.Insert(pool);
}

//------------------------------------------------------------------------------

void ObjectPoolRegistry::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Protected::Display(stream, prefix, options);

   stream << prefix << "statsGroup        : ";
   stream << strObj(statsGroup_.get()) << CRLF;
   stream << prefix << "nullifyObjectDataCfg : ";
   stream << strObj(nullifyObjectDataCfg_.get()) << CRLF;

   stream << prefix << "pools [ObjectPoolId]" << CRLF;
   pools_.Display(stream, prefix + spaces(2), options);
}

//------------------------------------------------------------------------------

void ObjectPoolRegistry::Patch(sel_t selector, void* arguments)
{
   Protected::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

void ObjectPoolRegistry::Shutdown(RestartLevel level)
{
   Debug::ft("ObjectPoolRegistry.Shutdown");

   for(auto p = pools_.Last(); p != nullptr; pools_.Prev(p))
   {
      p->Shutdown(level);
   }

   if(Restart::ClearsMemory(MemType())) return;

   FunctionGuard guard(Guard_MemUnprotect);
   Restart::Release(statsGroup_);
}

//------------------------------------------------------------------------------

void ObjectPoolRegistry::Startup(RestartLevel level)
{
   Debug::ft("ObjectPoolRegistry.Startup");

   if(statsGroup_ == nullptr)
   {
      FunctionGuard guard(Guard_MemUnprotect);
      statsGroup_.reset(new ObjectPoolStatsGroup);
   }

   for(auto p = pools_.First(); p != nullptr; pools_.Next(p))
   {
      p->Startup(level);
   }
}

//------------------------------------------------------------------------------

fixed_string PoolHeader = "Id   Avail   InUse  Segments  MemoryType  Name";
//                        | 2.      7.      7.        9.         11..<name>

size_t ObjectPoolRegistry::Summarize(ostream& stream, uint32_t selector) const
{
   stream << PoolHeader << CRLF;

   for(auto p = pools_.First(); p != nullptr; pools_.Next(p))
   {
      stream << setw(2) << int(p->Pid());
      stream << SPACE << setw(7) << p->AvailCount();
      stream << SPACE << setw(7) << p->InUseCount();
      stream << SPACE << setw(9) << p->Segments();
      stream << SPACE << setw(11) << p->BlockType();
      stream << spaces(2) << p->Name() << CRLF;
   }

   return pools_.Size();
}

//------------------------------------------------------------------------------

void ObjectPoolRegistry::UnbindPool(ObjectPool& pool)
{
   Debug::ftnt("ObjectPoolRegistry.UnbindPool");

   FunctionGuard guard(Guard_MemUnprotect);
   pools_.Erase(pool);
}
}
