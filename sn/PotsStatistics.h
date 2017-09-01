//==============================================================================
//
//  PotsStatistics.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
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
   static const Id OrigAttempted  = 0;  // outgoing call attempts by POTS users
   static const Id OrigAbandoned  = 1;  // calls abandoned while dialing
   static const Id TermAttempted  = 2;  // incoming call attempts to POTS users
   static const Id TermAbandoned  = 3;  // abandoned while waiting for answer
   static const Id Alerted        = 4;  // calls that applied ringing
   static const Id Answered       = 5;  // calls that were answered
   static const Id Resumed        = 6;  // calls that were suspended and resumed
   static const Id ProxyAttempted = 7;  // proxy calls attempted (redirections)
   static const Id ProxyAnswered  = 8;  // proxy calls answered
   static const Id MaxId          = 8;  // range constant

   //  Increments the basic call statistic identified by ID.
   //
   static void Incr(Id id);

   //  Increments the number of calls that received the
   //  treatment associated with CAUSE.
   //
   static void IncrCause(Cause::Ind cause);

   //  Overridden to display the group's statistics.
   //
   virtual void DisplayStats(std::ostream& stream, id_t id) const override;
private:
   //  Private because this singleton is not subclassed.
   //
   PotsStatistics();

   //  Private because this singleton is not subclassed.
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