//==============================================================================
//
//  InvokerPoolRegistry.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "InvokerPoolRegistry.h"
#include <memory>
#include <ostream>
#include <string>
#include "Debug.h"
#include "Formatters.h"
#include "InvokerPool.h"
#include "SbCliParms.h"
#include "Singleton.h"
#include "StatisticsGroup.h"
#include "SysTypes.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace SessionBase
{
class InvokerPoolStatsGroup : public StatisticsGroup
{
public:
   InvokerPoolStatsGroup();
   ~InvokerPoolStatsGroup();
   virtual void DisplayStats(ostream& stream, id_t id) const override;
};

//------------------------------------------------------------------------------

fn_name InvokerPoolStatsGroup_ctor = "InvokerPoolStatsGroup.ctor";

InvokerPoolStatsGroup::InvokerPoolStatsGroup() :
   StatisticsGroup("Invoker Pools [Faction]")
{
   Debug::ft(InvokerPoolStatsGroup_ctor);
}

//------------------------------------------------------------------------------

fn_name InvokerPoolStatsGroup_dtor = "InvokerPoolStatsGroup.dtor";

InvokerPoolStatsGroup::~InvokerPoolStatsGroup()
{
   Debug::ft(InvokerPoolStatsGroup_dtor);
}

//------------------------------------------------------------------------------

fn_name InvokerPoolStatsGroup_DisplayStats =
   "InvokerPoolStatsGroup.DisplayStats";

void InvokerPoolStatsGroup::DisplayStats(ostream& stream, id_t id) const
{
   Debug::ft(InvokerPoolStatsGroup_DisplayStats);

   StatisticsGroup::DisplayStats(stream, id);

   auto reg = Singleton< InvokerPoolRegistry >::Instance();

   if(id == 0)
   {
      auto& pools = reg->Pools();

      for(auto p = pools.First(); p != nullptr; pools.Next(p))
      {
         p->DisplayStats(stream);
      }
   }
   else
   {
      auto p = reg->Pool(Faction(id));

      if(p == nullptr)
      {
         stream << spaces(2) << NoInvPoolExpl << CRLF;
         return;
      }

      p->DisplayStats(stream);
   }
}

//==============================================================================

fn_name InvokerPoolRegistry_ctor = "InvokerPoolRegistry.ctor";

InvokerPoolRegistry::InvokerPoolRegistry() : poolToAudit_(0)
{
   Debug::ft(InvokerPoolRegistry_ctor);

   pools_.Init(Faction_N, InvokerPool::CellDiff(), MemDyn);
   statsGroup_.reset(new InvokerPoolStatsGroup);
}

//------------------------------------------------------------------------------

fn_name InvokerPoolRegistry_dtor = "InvokerPoolRegistry.dtor";

InvokerPoolRegistry::~InvokerPoolRegistry()
{
   Debug::ft(InvokerPoolRegistry_dtor);
}

//------------------------------------------------------------------------------

fn_name InvokerPoolRegistry_BindPool = "InvokerPoolRegistry.BindPool";

bool InvokerPoolRegistry::BindPool(InvokerPool& pool)
{
   Debug::ft(InvokerPoolRegistry_BindPool);

   return pools_.Insert(pool);
}

//------------------------------------------------------------------------------

fn_name InvokerPoolRegistry_ClaimBlocks = "InvokerPoolRegistry.ClaimBlocks";

void InvokerPoolRegistry::ClaimBlocks()
{
   Debug::ft(InvokerPoolRegistry_ClaimBlocks);

   while(poolToAudit_ < Faction_N)
   {
      auto pool = pools_.At(poolToAudit_);
      if(pool != nullptr) pool->ClaimBlocks();
      ++poolToAudit_;
   }

   poolToAudit_ = 0;
}

//------------------------------------------------------------------------------

void InvokerPoolRegistry::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Dynamic::Display(stream, prefix, options);

   stream << prefix << "statsGroup  : ";
   stream << strObj(statsGroup_.get()) << CRLF;
   stream << prefix << "poolToAudit : " << poolToAudit_ << CRLF;

   stream << prefix << "pools [Faction]" << CRLF;
   pools_.Display(stream, prefix + spaces(2), options);
}

//------------------------------------------------------------------------------

void InvokerPoolRegistry::Patch(sel_t selector, void* arguments)
{
   Dynamic::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

InvokerPool* InvokerPoolRegistry::Pool(Faction faction) const
{
   return pools_.At(faction);
}

//------------------------------------------------------------------------------

fn_name InvokerPoolRegistry_Startup = "InvokerPoolRegistry.Startup";

void InvokerPoolRegistry::Startup(RestartLevel level)
{
   Debug::ft(InvokerPoolRegistry_Startup);

   for(auto p = pools_.First(); p != nullptr; pools_.Next(p))
   {
      p->Startup(level);
   }
}

//------------------------------------------------------------------------------

fn_name InvokerPoolRegistry_UnbindPool = "InvokerPoolRegistry.UnbindPool";

void InvokerPoolRegistry::UnbindPool(InvokerPool& pool)
{
   Debug::ft(InvokerPoolRegistry_UnbindPool);

   pools_.Erase(pool);
}
}
