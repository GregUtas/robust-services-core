//==============================================================================
//
//  NbTracer.cpp
//
//  Copyright (C) 2013-2021  Greg Utas
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
#include "NbTracer.h"
#include "Tool.h"
#include <ostream>
#include <string>
#include <vector>
#include "Debug.h"
#include "Formatters.h"
#include "FunctionTrace.h"
#include "Singleton.h"
#include "Thread.h"
#include "ThreadRegistry.h"
#include "TraceBuffer.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Returns true if no threads are included or excluded.
//
static bool ThreadsEmpty()
{
   Debug::ft("NodeBase.ThreadsEmpty");

   auto threads = Singleton< ThreadRegistry >::Instance()->GetThreads();

   for(auto t = threads.cbegin(); t != threads.cend(); ++t)
   {
      if((*t)->GetStatus() != TraceDefault) return false;
   }

   return true;
}

//------------------------------------------------------------------------------
//
//  Tool for the trace buffer's internal use.
//
fixed_string TraceBufferToolName = "ToolBuffer";
fixed_string TraceBufferToolExpl = "internal use";

class TraceBufferTool : public Tool
{
   friend class Singleton< TraceBufferTool >;

   TraceBufferTool() : Tool(ToolBuffer, NUL, true) { }
   ~TraceBufferTool() = default;
   c_string Expl() const override { return TraceBufferToolExpl; }
   c_string Name() const override { return TraceBufferToolName; }
};

//------------------------------------------------------------------------------
//
//  Tool for function tracing.
//
fixed_string FunctionTraceToolName = "FunctionTracer";
fixed_string FunctionTraceToolExpl = "traces function calls";

class FunctionTraceTool : public Tool
{
   friend class Singleton< FunctionTraceTool >;

   FunctionTraceTool() : Tool(FunctionTracer, 'f', true) { }
   ~FunctionTraceTool() = default;
   c_string Expl() const override { return FunctionTraceToolExpl; }
   c_string Name() const override { return FunctionTraceToolName; }
   string Status() const override;
};

string FunctionTraceTool::Status() const
{
   auto str = Tool::Status();

   if(FunctionTrace::GetScope() == FunctionTrace::CountsOnly)
   {
      str += " (invocation counts only)";
   }

   return str;
}

//------------------------------------------------------------------------------
//
//  Tool for memory tracing.
//
fixed_string MemoryTraceToolName = "MemoryTracer";
fixed_string MemoryTraceToolExpl = "traces memory allocations/deallocations";

class MemoryTraceTool : public Tool
{
   friend class Singleton< MemoryTraceTool >;

   MemoryTraceTool() : Tool(MemoryTracer, 'm', true) { }
   ~MemoryTraceTool() = default;
   c_string Expl() const override { return MemoryTraceToolExpl; }
   c_string Name() const override { return MemoryTraceToolName; }
};

//------------------------------------------------------------------------------
//
NbTracer::NbTracer()
{
   Debug::ft("NbTracer.ctor");

   for(auto f = 0; f < Faction_N; ++f) factions_[f] = TraceDefault;

   Singleton< TraceBufferTool >::Instance();
   Singleton< FunctionTraceTool >::Instance();
   Singleton< MemoryTraceTool >::Instance();
}

//------------------------------------------------------------------------------

fn_name NbTracer_dtor = "NbTracer.dtor";

NbTracer::~NbTracer()
{
   Debug::ftnt(NbTracer_dtor);

   Debug::SwLog(NbTracer_dtor, UnexpectedInvocation, 0);
}

//------------------------------------------------------------------------------

fn_name NbTracer_ClearSelections = "NbTracer.ClearSelections";

TraceRc NbTracer::ClearSelections(FlagId filter)
{
   Debug::ft(NbTracer_ClearSelections);

   auto buff = Singleton< TraceBuffer >::Instance();
   auto threads = Singleton< ThreadRegistry >::Instance()->GetThreads();

   switch(filter)
   {
   case TraceFaction:
      for(auto f = 0; f < Faction_N; ++f)
      {
         factions_[f] = TraceDefault;
      }
      buff->ClearFilter(TraceFaction);
      break;

   case TraceThread:
      for(auto t = threads.begin(); t != threads.end(); ++t)
      {
         (*t)->SetStatus(TraceDefault);
      }
      buff->ClearFilter(TraceThread);
      break;

   case TraceAll:
      buff->ClearFilter(TraceAll);
      ClearSelections(TraceFaction);
      ClearSelections(TraceThread);
      break;

   default:
      Debug::SwLog(NbTracer_ClearSelections, "unexpected filter", filter);
   }

   return TraceOk;
}

//------------------------------------------------------------------------------

TraceStatus NbTracer::FactionStatus(Faction faction) const
{
   if((faction >= 0) && (faction < Faction_N)) return factions_[faction];
   return TraceDefault;
}

//------------------------------------------------------------------------------

void NbTracer::Patch(sel_t selector, void* arguments)
{
   Permanent::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fixed_string AllSelected      = "ALL ACTIVITY selected.";
fixed_string FactionsSelected = "Factions: ";
fixed_string ThreadsSelected  = "Threads: ";

void NbTracer::QuerySelections(ostream& stream) const
{
   Debug::ft("NbTracer.QuerySelections");

   auto buff = Singleton< TraceBuffer >::Instance();

   if(buff->FilterIsOn(TraceAll)) stream << AllSelected << CRLF;

   stream << FactionsSelected << CRLF;

   if(!buff->FilterIsOn(TraceFaction))
   {
      stream << spaces(2) << TraceBuffer::NoneSelected << CRLF;
   }
   else
   {
      for(auto f = 0; f < Faction_N; ++f)
      {
         if(factions_[f] != TraceDefault)
         {
            stream << spaces(2) << factions_[f] << ": ";
            stream << Faction(f) << CRLF;
         }
      }
   }

   stream << ThreadsSelected << CRLF;

   if(!buff->FilterIsOn(TraceThread))
   {
      stream << spaces(2) << TraceBuffer::NoneSelected << CRLF;
   }
   else
   {
      auto threads = Singleton< ThreadRegistry >::Instance()->GetThreads();

      for(auto t = threads.cbegin(); t != threads.cend(); ++t)
      {
         if((*t)->GetStatus() != TraceDefault)
         {
            stream << spaces(2) << (*t)->GetStatus() << ": ";
            stream << strObj(*t) << CRLF;
         }
      }
   }
}

//------------------------------------------------------------------------------

TraceRc NbTracer::SelectFaction(Faction faction, TraceStatus status)
{
   Debug::ft("NbTracer.SelectFaction");

   auto buff = Singleton< TraceBuffer >::Instance();

   factions_[faction] = status;

   if(status != TraceDefault)
   {
      buff->SetFilter(TraceFaction);
      return TraceOk;
   }

   buff->ClearFilter(TraceFaction);

   for(auto f = 0; f < Faction_N; ++f)
   {
      if(factions_[f] != TraceDefault)
      {
         buff->SetFilter(TraceFaction);
         return TraceOk;
      }
   }

   return TraceOk;
}

//------------------------------------------------------------------------------

TraceRc NbTracer::SelectThread(ThreadId tid, TraceStatus status)
{
   Debug::ft("NbTracer.SelectThread");

   auto thr = Singleton< ThreadRegistry >::Instance()->GetThread(tid);

   if(thr == nullptr) return NoSuchItem;

   thr->SetStatus(status);

   auto buff = Singleton< TraceBuffer >::Instance();

   if(status == TraceDefault)
   {
      if(ThreadsEmpty()) buff->ClearFilter(TraceThread);
      return TraceOk;
   }

   buff->SetFilter(TraceThread);
   return TraceOk;
}
}
