//==============================================================================
//
//  FactoryRegistry.cpp
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
#include "FactoryRegistry.h"
#include "StatisticsGroup.h"
#include <iomanip>
#include <ostream>
#include <string>
#include "Debug.h"
#include "Factory.h"
#include "Formatters.h"
#include "FunctionGuard.h"
#include "Restart.h"
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
class FactoryStatsGroup : public StatisticsGroup
{
public:
   FactoryStatsGroup();
   ~FactoryStatsGroup();
   void DisplayStats
      (ostream& stream, id_t id, const Flags& options) const override;
};

//------------------------------------------------------------------------------

FactoryStatsGroup::FactoryStatsGroup() :
   StatisticsGroup("Factories [Factory::Id]")
{
   Debug::ft("FactoryStatsGroup.ctor");
}

//------------------------------------------------------------------------------

FactoryStatsGroup::~FactoryStatsGroup()
{
   Debug::ftnt("FactoryStatsGroup.dtor");
}

//------------------------------------------------------------------------------

void FactoryStatsGroup::DisplayStats
   (ostream& stream, id_t id, const Flags& options) const
{
   Debug::ft("FactoryStatsGroup.DisplayStats");

   StatisticsGroup::DisplayStats(stream, id, options);

   auto reg = Singleton<FactoryRegistry>::Instance();

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
      auto f = reg->Factories().At(id);

      if(f == nullptr)
      {
         stream << spaces(2) << NoFactoryExpl << CRLF;
         return;
      }

      f->DisplayStats(stream, options);
   }
}

//==============================================================================

FactoryRegistry::FactoryRegistry()
{
   Debug::ft("FactoryRegistry.ctor");

   factories_.Init(Factory::MaxId, Factory::CellDiff(), MemImmutable);
   statsGroup_.reset(new FactoryStatsGroup);
}

//------------------------------------------------------------------------------

fn_name FactoryRegistry_dtor = "FactoryRegistry.dtor";

FactoryRegistry::~FactoryRegistry()
{
   Debug::ftnt(FactoryRegistry_dtor);

   Debug::SwLog(FactoryRegistry_dtor, UnexpectedInvocation, 0);
}

//------------------------------------------------------------------------------

bool FactoryRegistry::BindFactory(Factory& factory)
{
   Debug::ft("FactoryRegistry.BindFactory");

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

void FactoryRegistry::Patch(sel_t selector, void* arguments)
{
   Immutable::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

void FactoryRegistry::Shutdown(RestartLevel level)
{
   Debug::ft("FactoryRegistry.Shutdown");

   for(auto f = factories_.Last(); f != nullptr; factories_.Prev(f))
   {
      f->Shutdown(level);
   }

   FunctionGuard guard(Guard_ImmUnprotect);
   Restart::Release(statsGroup_);
}

//------------------------------------------------------------------------------

void FactoryRegistry::Startup(RestartLevel level)
{
   Debug::ft("FactoryRegistry.Startup");

   if(statsGroup_ == nullptr)
   {
      FunctionGuard guard(Guard_ImmUnprotect);
      statsGroup_.reset(new FactoryStatsGroup);
   }

   for(auto f = factories_.First(); f != nullptr; factories_.Next(f))
   {
      f->Startup(level);
   }
}

//------------------------------------------------------------------------------

fixed_string FactoryHeader = " Id  Type     Faction  Protocol  Name";
//                           |  3     6          12        10..<name>

size_t FactoryRegistry::Summarize(ostream& stream, uint32_t selector) const
{
   stream << FactoryHeader << CRLF;

   for(auto f = factories_.First(); f != nullptr; factories_.Next(f))
   {
      stream << setw(3) << f->Fid();
      stream << setw(6) << f->GetType();
      stream << setw(12) << f->GetFaction();
      stream << setw(10) << f->GetProtocol();
      stream << spaces(2) << strClass(f) << CRLF;
   }

   return factories_.Size();
}

//------------------------------------------------------------------------------

void FactoryRegistry::UnbindFactory(Factory& factory)
{
   Debug::ftnt("FactoryRegistry.UnbindFactory");

   factories_.Erase(factory);
}
}
