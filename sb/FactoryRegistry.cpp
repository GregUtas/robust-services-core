//==============================================================================
//
//  FactoryRegistry.cpp
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
#include "FactoryRegistry.h"
#include "StatisticsGroup.h"
#include <ostream>
#include <string>
#include "Debug.h"
#include "Factory.h"
#include "Formatters.h"
#include "FunctionGuard.h"
#include "SbCliParms.h"
#include "Singleton.h"
#include "SysTypes.h"

using namespace NodeBase;
using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace SessionBase
{
class FactoryStatsGroup : public StatisticsGroup
{
public:
   FactoryStatsGroup();
   ~FactoryStatsGroup();
   void DisplayStats
      (ostream& stream, id_t id, const Flags& options) const override;
};

//------------------------------------------------------------------------------

fn_name FactoryStatsGroup_ctor = "FactoryStatsGroup.ctor";

FactoryStatsGroup::FactoryStatsGroup() :
   StatisticsGroup("Factories [Factory::Id]")
{
   Debug::ft(FactoryStatsGroup_ctor);
}

//------------------------------------------------------------------------------

fn_name FactoryStatsGroup_dtor = "FactoryStatsGroup.dtor";

FactoryStatsGroup::~FactoryStatsGroup()
{
   Debug::ft(FactoryStatsGroup_dtor);
}

//------------------------------------------------------------------------------

fn_name FactoryStatsGroup_DisplayStats = "FactoryStatsGroup.DisplayStats";

void FactoryStatsGroup::DisplayStats
   (ostream& stream, id_t id, const Flags& options) const
{
   Debug::ft(FactoryStatsGroup_DisplayStats);

   StatisticsGroup::DisplayStats(stream, id, options);

   auto reg = Singleton< FactoryRegistry >::Instance();

   if(id == 0)
   {
      auto& facs = reg->Factories();

      for(auto f = facs.First(); f != nullptr; facs.Next(f))
      {
         f->DisplayStats(stream, options);
      }
   }
   else
   {
      auto f = reg->GetFactory(id);

      if(f == nullptr)
      {
         stream << spaces(2) << NoFactoryExpl << CRLF;
         return;
      }

      f->DisplayStats(stream, options);
   }
}

//==============================================================================

fn_name FactoryRegistry_ctor = "FactoryRegistry.ctor";

FactoryRegistry::FactoryRegistry()
{
   Debug::ft(FactoryRegistry_ctor);

   factories_.Init(Factory::MaxId, Factory::CellDiff(), MemImmutable);
   statsGroup_.reset(new FactoryStatsGroup);
}

//------------------------------------------------------------------------------

fn_name FactoryRegistry_dtor = "FactoryRegistry.dtor";

FactoryRegistry::~FactoryRegistry()
{
   Debug::ft(FactoryRegistry_dtor);
}

//------------------------------------------------------------------------------

fn_name FactoryRegistry_BindFactory = "FactoryRegistry.BindFactory";

bool FactoryRegistry::BindFactory(Factory& factory)
{
   Debug::ft(FactoryRegistry_BindFactory);

   return factories_.Insert(factory);
}

//------------------------------------------------------------------------------

void FactoryRegistry::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Immutable::Display(stream, prefix, options);

   stream << prefix << "statsGroup : ";
   stream << strObj(statsGroup_.get()) << CRLF;

   stream << prefix << "factories [FactoryId]" << CRLF;
   factories_.Display(stream, prefix + spaces(2), options);
}

//------------------------------------------------------------------------------

Factory* FactoryRegistry::GetFactory(FactoryId fid) const
{
   return factories_.At(fid);
}

//------------------------------------------------------------------------------

void FactoryRegistry::Patch(sel_t selector, void* arguments)
{
   Immutable::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name FactoryRegistry_Shutdown = "FactoryRegistry.Shutdown";

void FactoryRegistry::Shutdown(RestartLevel level)
{
   Debug::ft(FactoryRegistry_Shutdown);

   for(auto f = factories_.Last(); f != nullptr; factories_.Prev(f))
   {
      f->Shutdown(level);
   }

   if(level == RestartCold)
   {
      FunctionGuard guard(Guard_ImmUnprotect);
      statsGroup_.release();
   }
}

//------------------------------------------------------------------------------

fn_name FactoryRegistry_Startup = "FactoryRegistry.Startup";

void FactoryRegistry::Startup(RestartLevel level)
{
   Debug::ft(FactoryRegistry_Startup);

   if(statsGroup_ == nullptr)
   {
      FunctionGuard guard(Guard_ImmUnprotect, (level < RestartReboot));
      statsGroup_.reset(new FactoryStatsGroup);
   }

   for(auto f = factories_.First(); f != nullptr; factories_.Next(f))
   {
      f->Startup(level);
   }
}

//------------------------------------------------------------------------------

fn_name FactoryRegistry_UnbindFactory = "FactoryRegistry.UnbindFactory";

void FactoryRegistry::UnbindFactory(Factory& factory)
{
   Debug::ft(FactoryRegistry_UnbindFactory);

   factories_.Erase(factory);
}
}
