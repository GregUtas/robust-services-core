//==============================================================================
//
//  PotsStatistics.h
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
#ifndef POTSSTATISTICS_H_INCLUDED
#define POTSSTATISTICS_H_INCLUDED

#include "StatisticsGroup.h"
#include <cstdint>
#include "BcCause.h"
#include "NbTypes.h"

using namespace NodeBase;
using namespace CallBase;

//------------------------------------------------------------------------------

namespace PotsBase
{
//  Statistics for POTS calls.
//
class PotsStatistics : public StatisticsGroup
{
   friend class Singleton< PotsStatistics >;
public:
   //  The type that identifies each statistic.
   //
   typedef uint8_t Id;

   //  Identifiers for the statistics gathered during POTS calls.
   //
   static const Id OrigAttempted = 0;   // outgoing call attempts by POTS users
   static const Id OrigAbandoned = 1;   // calls abandoned while dialing
   static const Id TermAttempted = 2;   // incoming call attempts to POTS users
   static const Id TermAbandoned = 3;   // abandoned while waiting for answer
   static const Id Alerted = 4;         // calls that applied ringing
   static const Id Answered = 5;        // calls that were answered
   static const Id Resumed = 6;         // calls that were suspended and resumed
   static const Id ProxyAttempted = 7;  // proxy calls attempted (redirections)
   static const Id ProxyAnswered = 8;   // proxy calls answered
   static const Id MaxId = 8;           // range constant

   //  Increments the basic call statistic identified by ID.
   //
   static void Incr(Id id);

   //  Increments the number of calls that received the
   //  treatment associated with CAUSE.
   //
   static void IncrCause(Cause::Ind cause);

   //  Overridden to display the group's statistics.
   //
   void DisplayStats
      (std::ostream& stream, id_t id, const Flags& options) const override;
private:
   //  Private because this is a singleton.
   //
   PotsStatistics();

   //  Private because this is a singleton.
   //
   ~PotsStatistics();

   //  Basic call statistics.
   //
   CounterPtr basicCalls_[MaxId + 1];

   //  Treatment statistics.
   //
   CounterPtr treatments_[Cause::MaxInd + 1];
};
}
#endif
