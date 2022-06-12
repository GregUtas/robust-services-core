//==============================================================================
//
//  InvokerPoolRegistry.cpp
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
#include "InvokerPoolRegistry.h"
#include "StatisticsGroup.h"
#include <iomanip>
#include <ostream>
#include <string>
#include "Debug.h"
#include "Formatters.h"
#include "InvokerPool.h"
#include "SbCliParms.h"
#include "Singleton.h"
#include "SysTypes.h"

using namespace NodeBase;
using std::ostream;
using std::setw;
using std::string;

//------------------------------------------------------------------------------

namespace SessionBase
{
class InvokerPoolStatsGroup : public StatisticsGroup
{
public:
   InvokerPoolStatsGroup();
   ~InvokerPoolStatsGroup();
   void DisplayStats
      (ostream& stream, id_t id, const Flags& options) const override;
};

//------------------------------------------------------------------------------

InvokerPoolStatsGroup::InvokerPoolStatsGroup() :
   StatisticsGroup("Invoker Pools [Faction]")
{
   Debug::ft("InvokerPoolStatsGroup.ctor");
}

//------------------------------------------------------------------------------

InvokerPoolStatsGroup::~InvokerPoolStatsGroup()
{
   Debug::ftnt("InvokerPoolStatsGroup.dtor");
}

//------------------------------------------------------------------------------

void InvokerPoolStatsGroup::DisplayStats
   (ostream& stream, id_t id, const Flags& options) const
{
   Debug::ft("InvokerPoolStatsGroup.DisplayStats");

   StatisticsGroup::DisplayStats(stream, id, options);

   auto reg = Singleton<InvokerPoolRegistry>::Instance();

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
      auto p = reg->Pool(Faction(id));

      if(p == nullptr)
      {
         stream << spaces(2) << NoInvPoolExpl << CRLF;
         return;
      }

      p->DisplayStats(stream, options);
   }
}

//==============================================================================

InvokerPoolRegistry::InvokerPoolRegistry() : poolToAudit_(0)
{
   Debug::ft("InvokerPoolRegistry.ctor");

   pools_.Init(Faction_N, InvokerPool::CellDiff(), MemDynamic);
   statsGroup_.reset(new InvokerPoolStatsGroup);
}

//------------------------------------------------------------------------------

fn_name InvokerPoolRegistry_dtor = "InvokerPoolRegistry.dtor";

InvokerPoolRegistry::~InvokerPoolRegistry()
{
   Debug::ftnt(InvokerPoolRegistry_dtor);

   Debug::SwLog(InvokerPoolRegistry_dtor, UnexpectedInvocation, 0);
}

//------------------------------------------------------------------------------

bool InvokerPoolRegistry::BindPool(InvokerPool& pool)
{
   Debug::ft("InvokerPoolRegistry.BindPool");

   return pools_.Insert(pool);
}

//------------------------------------------------------------------------------

void InvokerPoolRegistry::ClaimBlocks()
{
   Debug::ft("InvokerPoolRegistry.ClaimBlocks");

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

void InvokerPoolRegistry::Shutdown(RestartLevel level)
{
   Debug::ft("InvokerPoolRegistry.Shutdown");

   for(auto p = pools_.First(); p != nullptr; pools_.Next(p))
   {
      p->Shutdown(level);
   }
}

//------------------------------------------------------------------------------

void InvokerPoolRegistry::Startup(RestartLevel level)
{
   Debug::ft("InvokerPoolRegistry.Startup");

   for(auto p = pools_.First(); p != nullptr; pools_.Next(p))
   {
      p->Startup(level);
   }
}

//------------------------------------------------------------------------------

fixed_string InvokerHeader = "Id     Faction  Invokers  Name";
//                           | 2          12        10..<name>

size_t InvokerPoolRegistry::Summarize(ostream& stream, uint32_t selector) const
{
   stream << InvokerHeader << CRLF;

   for(auto p = pools_.First(); p != nullptr; pools_.Next(p))
   {
      auto sf = p->GetFaction();
      stream << setw(2) << int(sf);
      stream << setw(12) << sf;
      stream << setw(10) << p->Invokers().Size();
      stream << spaces(2) << strClass(this) << CRLF;
   }

   return pools_.Size();
}

//------------------------------------------------------------------------------

void InvokerPoolRegistry::UnbindPool(InvokerPool& pool)
{
   Debug::ftnt("InvokerPoolRegistry.UnbindPool");

   pools_.Erase(pool);
}
}
