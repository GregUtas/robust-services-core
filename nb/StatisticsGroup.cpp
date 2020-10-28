//==============================================================================
//
//  StatisticsGroup.cpp
//
//  Copyright (C) 2013-2020  Greg Utas
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
#include "StatisticsGroup.h"
#include <cstdint>
#include <ostream>
#include "Algorithms.h"
#include "Debug.h"
#include "Formatters.h"
#include "Singleton.h"
#include "StatisticsRegistry.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
const size_t StatisticsGroup::MaxExplSize = 44;
const size_t StatisticsGroup::ReportWidth = 76;
fixed_string StatisticsGroup::ReportHeader = "      Curr      Prev         All";
//<----------------group name---------------->      Curr      Prev         All
//  <-------------member name---------------->
//    <---individual statistic explanation---> nnnnnnnnn nnnnnnnnn nnnnnnnnnnn
//         1         2         3         4         5         6         7
//1234567890123456789012345678901234567890123456789012345678901234567890123456

//------------------------------------------------------------------------------

fn_name StatisticsGroup_ctor = "StatisticsGroup.ctor";

StatisticsGroup::StatisticsGroup(const string& expl) : expl_(expl.c_str())
{
   Debug::ft(StatisticsGroup_ctor);

   if(expl_.size() > MaxExplSize)
   {
      Debug::SwLog(StatisticsGroup_ctor, "expl length", expl_.size());
   }

   Singleton< StatisticsRegistry >::Instance()->BindGroup(*this);
}

//------------------------------------------------------------------------------

fn_name StatisticsGroup_dtor = "StatisticsGroup.dtor";

StatisticsGroup::~StatisticsGroup()
{
   Debug::ftnt(StatisticsGroup_dtor);

   Debug::SwLog(StatisticsGroup_dtor, UnexpectedInvocation, 0);
   Singleton< StatisticsRegistry >::Extant()->UnbindGroup(*this);
}

//------------------------------------------------------------------------------

ptrdiff_t StatisticsGroup::CellDiff()
{
   uintptr_t local;
   auto fake = reinterpret_cast< const StatisticsGroup* >(&local);
   return ptrdiff(&fake->gid_, fake);
}

//------------------------------------------------------------------------------

void StatisticsGroup::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Dynamic::Display(stream, prefix, options);

   stream << prefix << "gid  : " << gid_.to_str() << CRLF;
   stream << prefix << "expl : " << expl_ << CRLF;
}

//------------------------------------------------------------------------------

fn_name StatisticsGroup_DisplayStats = "StatisticsGroup.DisplayStats";

void StatisticsGroup::DisplayStats
   (ostream& stream, id_t id, const Flags& options) const
{
   Debug::ft(StatisticsGroup_DisplayStats);

   stream << expl_ << spaces(MaxExplSize - expl_.size());
   stream << ReportHeader << CRLF;
}

//------------------------------------------------------------------------------

void StatisticsGroup::Patch(sel_t selector, void* arguments)
{
   Dynamic::Patch(selector, arguments);
}
}
