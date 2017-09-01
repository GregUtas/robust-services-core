//==============================================================================
//
//  NbTracer.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef NBTRACER_H_INCLUDED
#define NBTRACER_H_INCLUDED

#include "Permanent.h"
#include <iosfwd>
#include "NbTypes.h"
#include "SysTypes.h"
#include "ToolTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Although it is possible to capture everything in the lab, doing so in the
//  field is impossible because the trace buffer would quickly overflow.  The
//  real-time cost would also be too great.  It must therefore be possible to
//  perform tracing selectively, and so the following are supported:
//
//  What to trace:                          Trace tool:
//  specific threads                        NbTracer
//  specific factions                       NbTracer
//  specific IP addresses/ports (external)  NwTracer
//  specific IP ports (internal)            NwTracer
//  specific sessions                       SbTracer
//
//  The specific entities to be traced are selected using the >include command.
//  The set of selected threads or factions can also be modified after stopping
//  tracing.  The file created by the >save command will then omit the function
//  calls that occurred on threads or factions which are no longer selected:
//
//    >stop                              // stop tracing
//    >clear selections                  // select nothing
//    >include faction &faction.payload  // include the payload faction
//    >save trace <fn>                   // display work in payload threads only
//
//  TraceBuffer tracks which trace tools have been selected, whereas the various
//  trace tools track which entities to capture during tracing.  When tracing is
//  active, TraceRecord subclasses are constructed in TraceBuffer.  When tracing
//  stops, a report generator (TraceDump) displays the trace records or analyzes
//  them (FunctionProfiler, MscBuilder) to generate a report.
//
class NbTracer : public Permanent
{
   friend class Singleton< NbTracer >;
public:
   //  Traces FAC according to STATUS.
   //
   TraceRc SelectFaction(Faction faction, TraceStatus status);

   //  Traces TID according to STATUS.
   //
   static TraceRc SelectThread(ThreadId tid, TraceStatus status);

   //  Returns true if no threads are included or excluded.
   //
   static bool ThreadsEmpty();

   //  Returns the trace status of the faction identified by FAC.
   //
   TraceStatus FactionStatus(Faction faction) const;

   //  Displays, in STREAM, everything that has been included or excluded.
   //
   void QuerySelections(std::ostream& stream) const;

   //  Removes everything of type FILTER that has been included or excluded.
   //
   TraceRc ClearSelections(FlagId filter);
private:
   //  Private because this singleton is not subclassed.
   //
   NbTracer();

   //  Private because this singleton is not subclassed.
   //
   ~NbTracer();

   //  The trace status of each faction.
   //
   TraceStatus factions_[Faction_N];
};
}
#endif
