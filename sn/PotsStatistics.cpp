//==============================================================================
//
//  PotsStatistics.cpp
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
#include "PotsStatistics.h"
#include <ostream>
#include <string>
#include "Debug.h"
#include "Formatters.h"
#include "Singleton.h"
#include "Statistics.h"
#include "SysTypes.h"

using std::ostream;

//------------------------------------------------------------------------------

namespace PotsBase
{
fixed_string StatExplStrings[PotsStatistics::MaxId + 1] =
{
   "originations attempted",
   "originations abandoned",
   "terminations attempted",
   "terminations abandoned",
   "calls alerted",
   "calls answered",
   "calls resumed",
   "proxy calls attempted",
   "proxy calls answered"
};

//------------------------------------------------------------------------------

PotsStatistics::PotsStatistics() : StatisticsGroup("POTS Calls")
{
   Debug::ft("PotsStatistics.ctor");

   for(auto i = 0; i <= MaxId; ++i)
   {
      basicCalls_[i].reset(new Counter(StatExplStrings[i]));
   }

   for(auto i = 0; i <= Cause::MaxInd; ++i)
   {
      treatments_[i].reset(new Counter(Cause::strInd(i)));
   }
}

//------------------------------------------------------------------------------

PotsStatistics::~PotsStatistics()
{
   Debug::ftnt("PotsStatistics.dtor");
}

//------------------------------------------------------------------------------

void PotsStatistics::DisplayStats
   (ostream& stream, id_t id, const Flags& options) const
{
   Debug::ft("PotsStatistics.DisplayStats");

   StatisticsGroup::DisplayStats(stream, id, options);

   stream << spaces(2) << "Basic Calls" << CRLF;

   for(auto i = 0; i <= MaxId; ++i)
   {
      basicCalls_[i]->DisplayStat(stream, options);
   }

   stream << spaces(2) << "Treatments (by Cause)" << CRLF;

   for(auto i = 0; i <= Cause::MaxInd; ++i)
   {
      treatments_[i]->DisplayStat(stream, options);
   }
}

//------------------------------------------------------------------------------

fn_name PotsStatistics_Incr = "PotsStatistics.Incr";

void PotsStatistics::Incr(Id id)
{
   Debug::ft(PotsStatistics_Incr);

   if(id <= MaxId)
   {
      Singleton< PotsStatistics >::Instance()->basicCalls_[id]->Incr();
      return;
   }

   Debug::SwLog(PotsStatistics_Incr, "invalid id", id);
}

//------------------------------------------------------------------------------

fn_name PotsStatistics_IncrCause = "PotsStatistics.IncrCause";

void PotsStatistics::IncrCause(Cause::Ind cause)
{
   Debug::ft(PotsStatistics_IncrCause);

   if(cause <= Cause::MaxInd)
   {
      Singleton< PotsStatistics >::Instance()->treatments_[cause]->Incr();
      return;
   }

   Debug::SwLog(PotsStatistics_IncrCause, "invalid Cause::Ind", cause);
}
}
