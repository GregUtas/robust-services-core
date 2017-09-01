//==============================================================================
//
//  NbTracer.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "NbTracer.h"
#include <ostream>
#include <string>
#include "Debug.h"
#include "Formatters.h"
#include "FunctionTrace.h"
#include "Registry.h"
#include "Singleton.h"
#include "Thread.h"
#include "ThreadRegistry.h"
#include "Tool.h"
#include "TraceBuffer.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Tool for the trace buffer's internal use.
//
fixed_string TraceBufferToolName = "ToolBuffer";
fixed_string TraceBufferToolExpl = "internal use";

class TraceBufferTool : public Tool
{
   friend class Singleton< TraceBufferTool >;
private:
   TraceBufferTool() : Tool(ToolBuffer, 0, true) { }
   ~TraceBufferTool() { }
   virtual const char* Name() const override { return TraceBufferToolName; }
   virtual const char* Expl() const override { return TraceBufferToolExpl; }
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
private:
   FunctionTraceTool() : Tool(FunctionTracer, 'f', true) { }
   ~FunctionTraceTool() { }
   virtual const char* Name() const override { return FunctionTraceToolName; }
   virtual const char* Expl() const override { return FunctionTraceToolExpl; }
   virtual string Status() const override;
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
private:
   MemoryTraceTool() : Tool(MemoryTracer, 'm', true) { }
   ~MemoryTraceTool() { }
   virtual const char* Name() const override { return MemoryTraceToolName; }
   virtual const char* Expl() const override { return MemoryTraceToolExpl; }
};

//------------------------------------------------------------------------------
//
fn_name NbTracer_ctor = "NbTracer.ctor";

NbTracer::NbTracer()
{
   Debug::ft(NbTracer_ctor);

   for(auto f = 0; f < Faction_N; ++f) factions_[f] = TraceDefault;

   Singleton< TraceBufferTool >::Instance();
   Singleton< FunctionTraceTool >::Instance();
   Singleton< MemoryTraceTool >::Instance();
}

//------------------------------------------------------------------------------

fn_name NbTracer_dtor = "NbTracer.dtor";

NbTracer::~NbTracer()
{
   Debug::ft(NbTracer_dtor);
}

//------------------------------------------------------------------------------

fn_name NbTracer_ClearSelections = "NbTracer.ClearSelections";

TraceRc NbTracer::ClearSelections(FlagId filter)
{
   Debug::ft(NbTracer_ClearSelections);

   auto buff = Singleton< TraceBuffer >::Instance();
   auto& threads = Singleton< ThreadRegistry >::Instance()->Threads();

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
      for(auto t = threads.First(); t != nullptr; threads.Next(t))
      {
         t->SetStatus(TraceDefault);
      }
      buff->ClearFilter(TraceThread);
      break;

   case TraceAll:
      buff->ClearFilter(TraceAll);
      ClearSelections(TraceFaction);
      ClearSelections(TraceThread);
      break;

   default:
      Debug::SwErr(NbTracer_ClearSelections, filter, 0);
   }

   return TraceOk;
}

//------------------------------------------------------------------------------

TraceStatus NbTracer::FactionStatus(Faction faction) const
{
   if(faction >= Faction_N) return TraceDefault;
   return factions_[faction];
}

//------------------------------------------------------------------------------

fixed_string AllSelected      = "ALL ACTIVITY selected.";
fixed_string FactionsSelected = "Factions: ";
fixed_string ThreadsSelected  = "Threads: ";

fn_name NbTracer_QuerySelections = "NbTracer.QuerySelections";

void NbTracer::QuerySelections(ostream& stream) const
{
   Debug::ft(NbTracer_QuerySelections);

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
      auto& threads = Singleton< ThreadRegistry >::Instance()->Threads();

      for(auto t = threads.First(); t != nullptr; threads.Next(t))
      {
         if(t->GetStatus() != TraceDefault)
         {
            stream << spaces(2) << t->GetStatus() << ": ";
            stream << strObj(t) << CRLF;
         }
      }
   }
}

//------------------------------------------------------------------------------

fn_name NbTracer_SelectFaction = "NbTracer.SelectFaction";

TraceRc NbTracer::SelectFaction(Faction faction, TraceStatus status)
{
   Debug::ft(NbTracer_SelectFaction);

   auto buff = Singleton< TraceBuffer >::Instance();

   factions_[faction] = status;

   if(status != TraceDefault) buff->SetFilter(TraceFaction);

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

fn_name NbTracer_SelectThread = "NbTracer.SelectThread";

TraceRc NbTracer::SelectThread(ThreadId tid, TraceStatus status)
{
   Debug::ft(NbTracer_SelectThread);

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

//------------------------------------------------------------------------------

fn_name NbTracer_ThreadsEmpty = "NbTracer.ThreadsEmpty";

bool NbTracer::ThreadsEmpty()
{
   Debug::ft(NbTracer_ThreadsEmpty);

   auto& threads = Singleton< ThreadRegistry >::Instance()->Threads();

   for(auto t = threads.First(); t != nullptr; threads.Next(t))
   {
      if(t->GetStatus() != TraceDefault) return false;
   }

   return true;
}
}
