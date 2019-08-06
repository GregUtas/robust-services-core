//==============================================================================
//
//  SbTracer.cpp
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
#include "SbTracer.h"
#include "Tool.h"
#include <ostream>
#include <string>
#include "Debug.h"
#include "FactoryRegistry.h"
#include "Formatters.h"
#include "LocalAddress.h"
#include "Message.h"
#include "NwTracer.h"
#include "ProtocolRegistry.h"
#include "SbIpBuffer.h"
#include "ServiceRegistry.h"
#include "Signal.h"
#include "Singleton.h"
#include "Thread.h"
#include "TraceBuffer.h"

using namespace NetworkBase;
using namespace NodeBase;
using std::ostream;

//------------------------------------------------------------------------------

namespace SessionBase
{
fixed_string TransTraceToolName = "TransTracer";
fixed_string TransTraceToolExpl = "traces SessionBase transactions";

class TransTraceTool : public Tool
{
   friend class Singleton< TransTraceTool >;
private:
   TransTraceTool() : Tool(TransTracer, 't', true) { }
   c_string Name() const override { return TransTraceToolName; }
   c_string Expl() const override { return TransTraceToolExpl; }
};

//------------------------------------------------------------------------------

fixed_string BufferTraceToolName = "BufferTracer";
fixed_string BufferTraceToolExpl = "traces SessionBase IP buffers";

class BufferTraceTool : public Tool
{
   friend class Singleton< BufferTraceTool >;
private:
   BufferTraceTool() : Tool(BufferTracer, 'b', true) { }
   c_string Name() const override { return BufferTraceToolName; }
   c_string Expl() const override { return BufferTraceToolExpl; }
};

//------------------------------------------------------------------------------

fixed_string ContextTraceToolName = "ContextTracer";
fixed_string ContextTraceToolExpl = "traces SessionBase contexts";

class ContextTraceTool : public Tool
{
   friend class Singleton< ContextTraceTool >;
private:
   ContextTraceTool() : Tool(ContextTracer, 'c', true) { }
   c_string Name() const override { return ContextTraceToolName; }
   c_string Expl() const override { return ContextTraceToolExpl; }
};

//------------------------------------------------------------------------------

fixed_string FactoriesSelected = "Factories: ";
fixed_string ProtocolsSelected = "Protocols: ";
fixed_string SignalsSelected   = "Signals: ";
fixed_string ServicesSelected  = "Services: ";
fixed_string TimerThreads      = "Timer threads: ";

//------------------------------------------------------------------------------

SbTracer::SignalFilter::SignalFilter() :
   prid(NIL_ID), sid(NIL_ID), status(TraceDefault) { }

SbTracer::SignalFilter::SignalFilter(ProtocolId p, SignalId s, TraceStatus ts) :
   prid(p), sid(s), status(ts) { }

//------------------------------------------------------------------------------

fn_name SbTracer_ctor = "SbTracer.ctor";

SbTracer::SbTracer() : timers_(TraceDefault)
{
   Debug::ft(SbTracer_ctor);

   for(auto i = 0; i <= Factory::MaxId; ++i) factories_[i] = TraceDefault;
   for(auto i = 0; i <= Protocol::MaxId; ++i) protocols_[i] = TraceDefault;
   for(auto i = 0; i < MaxSignalEntries; ++i) signals_[i] = SignalFilter();
   for(auto i = 0; i <= Service::MaxId; ++i) services_[i] = TraceDefault;

   Singleton< TransTraceTool >::Instance();
   Singleton< BufferTraceTool >::Instance();
   Singleton< ContextTraceTool >::Instance();
}

//------------------------------------------------------------------------------

fn_name SbTracer_dtor = "SbTracer.dtor";

SbTracer::~SbTracer()
{
   Debug::ft(SbTracer_dtor);
}

//------------------------------------------------------------------------------

fn_name SbTracer_ClearSelections = "SbTracer.ClearSelections";

TraceRc SbTracer::ClearSelections(FlagId filter)
{
   Debug::ft(SbTracer_ClearSelections);

   auto buff = Singleton< TraceBuffer >::Instance();

   switch(filter)
   {
   case TraceFactory:
      for(auto i = 0; i <= Factory::MaxId; ++i) factories_[i] = TraceDefault;
      buff->ClearFilter(TraceFactory);
      break;

   case TraceProtocol:
      for(auto i = 0; i <= Protocol::MaxId; ++i) protocols_[i] = TraceDefault;
      buff->ClearFilter(TraceProtocol);
      break;

   case TraceSignal:
      for(auto i = 0; i < MaxSignalEntries; ++i) signals_[i] = SignalFilter();
      buff->ClearFilter(TraceSignal);
      break;

   case TraceService:
      for(auto i = 0; i <= Service::MaxId; ++i) services_[i] = TraceDefault;
      buff->ClearFilter(TraceService);
      break;

   case TraceTimers:
      timers_ = TraceDefault;
      buff->ClearFilter(TraceTimers);
      break;

   case TraceAll:
      Singleton< NwTracer >::Instance()->ClearSelections(TraceAll);
      ClearSelections(TraceFactory);
      ClearSelections(TraceProtocol);
      ClearSelections(TraceSignal);
      ClearSelections(TraceService);
      ClearSelections(TraceTimers);
      break;

   default:
      Debug::SwLog(SbTracer_ClearSelections, "unexpected filter", filter);
   }

   return TraceOk;
}

//------------------------------------------------------------------------------

fn_name SbTracer_FactoriesEmpty = "SbTracer.FactoriesEmpty";

bool SbTracer::FactoriesEmpty() const
{
   Debug::ft(SbTracer_FactoriesEmpty);

   for(auto i = 0; i <= Factory::MaxId; ++i)
   {
      if(factories_[i] != TraceDefault) return false;
   }

   return true;
}

//------------------------------------------------------------------------------

fn_name SbTracer_FindSignal = "SbTracer.FindSignal";

int SbTracer::FindSignal(ProtocolId prid, SignalId sid) const
{
   Debug::ft(SbTracer_FindSignal);

   for(auto i = 0; i < MaxSignalEntries; ++i)
   {
      if((signals_[i].prid == prid) && (signals_[i].sid == sid)) return i;
   }

   return -1;
}

//------------------------------------------------------------------------------

fn_name SbTracer_MsgStatus = "SbTracer.MsgStatus";

TraceStatus SbTracer::MsgStatus(const Message& msg, MsgDirection dir) const
{
   Debug::ft(SbTracer_MsgStatus);

   if(!Debug::TraceOn()) return TraceExcluded;

   TraceStatus status;

   if(Singleton< TraceBuffer >::Instance()->FilterIsOn(TraceSignal))
   {
      status = SignalStatus(msg.GetProtocol(), msg.GetSignal());
      if(status != TraceDefault) return status;
   }

   status = Singleton< NwTracer >::Instance()->BuffStatus(*msg.Buffer(), dir);
   if(status != TraceDefault) return status;

   status = factories_[msg.RxSbAddr().fid];
   if(status != TraceDefault) return status;

   status = protocols_[msg.GetProtocol()];
   if(status != TraceDefault) return status;

   return Thread::RunningThread()->CalcStatus(true);
}

//------------------------------------------------------------------------------

void SbTracer::Patch(sel_t selector, void* arguments)
{
   Permanent::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name SbTracer_ProtocolsEmpty = "SbTracer.ProtocolsEmpty";

bool SbTracer::ProtocolsEmpty() const
{
   Debug::ft(SbTracer_ProtocolsEmpty);

   for(auto i = 0; i <= Protocol::MaxId; ++i)
   {
      if(protocols_[i] != TraceDefault) return false;
   }

   return true;
}

//------------------------------------------------------------------------------

fn_name SbTracer_QuerySelections = "SbTracer.QuerySelections";

void SbTracer::QuerySelections(ostream& stream) const
{
   Debug::ft(SbTracer_QuerySelections);

   auto nwt = Singleton< NwTracer >::Instance();

   nwt->QuerySelections(stream);

   stream << FactoriesSelected << CRLF;

   auto buff = Singleton< TraceBuffer >::Instance();

   if(!buff->FilterIsOn(TraceFactory))
   {
      stream << spaces(2) << TraceBuffer::NoneSelected << CRLF;
   }
   else
   {
      auto reg = Singleton< FactoryRegistry >::Instance();

      for(auto i = 0; i <= Factory::MaxId; ++i)
      {
         if(factories_[i] != TraceDefault)
         {
            stream << spaces(2) << factories_[i] << ": ";
            stream << strClass(reg->GetFactory(i)) << CRLF;
         }
      }
   }

   stream << ProtocolsSelected << CRLF;

   if(!buff->FilterIsOn(TraceProtocol))
   {
      stream << spaces(2) << TraceBuffer::NoneSelected << CRLF;
   }
   else
   {
      auto reg = Singleton< ProtocolRegistry >::Instance();

      for(auto i = 0; i <= Protocol::MaxId; ++i)
      {
         if(protocols_[i] != TraceDefault)
         {
            stream << spaces(2) << protocols_[i] << ": ";
            stream << strClass(reg->GetProtocol(i)) << CRLF;
         }
      }
   }

   stream << SignalsSelected << CRLF;

   if(!buff->FilterIsOn(TraceSignal))
   {
      stream << spaces(2) << TraceBuffer::NoneSelected << CRLF;
   }
   else
   {
      auto reg = Singleton< ProtocolRegistry >::Instance();

      for(auto i = 0; i < MaxSignalEntries; ++i)
      {
         if(signals_[i].status != TraceDefault)
         {
            auto pro = reg->GetProtocol(signals_[i].prid);
            stream << spaces(2) << signals_[i].status << ": ";
            stream << strClass(pro) << '.';
            stream << strClass(pro->GetSignal(signals_[i].sid)) << CRLF;
         }
      }
   }

   stream << ServicesSelected << CRLF;

   if(!buff->FilterIsOn(TraceService))
   {
      stream << spaces(2) << TraceBuffer::NoneSelected << CRLF;
   }
   else
   {
      auto reg = Singleton< ServiceRegistry >::Instance();

      for(auto i = 0; i <= Service::MaxId; ++i)
      {
         if(services_[i] != TraceDefault)
         {
            stream << spaces(2) << services_[i] << ": ";
            stream << strClass(reg->GetService(i)) << CRLF;
         }
      }
   }

   if(buff->FilterIsOn(TraceTimers))
   {
      stream << TimerThreads << timers_ << CRLF;
   }
}

//------------------------------------------------------------------------------

fn_name SbTracer_SelectFactory = "SbTracer.SelectFactory";

TraceRc SbTracer::SelectFactory(FactoryId fid, TraceStatus status)
{
   Debug::ft(SbTracer_SelectFactory);

   if(Singleton< FactoryRegistry >::Instance()->GetFactory(fid) == nullptr)
   {
      return NoSuchItem;
   }

   auto buff = Singleton< TraceBuffer >::Instance();

   factories_[fid] = status;

   if(status == TraceDefault)
   {
      if(FactoriesEmpty()) buff->ClearFilter(TraceFactory);
      return TraceOk;
   }

   buff->SetFilter(TraceFactory);
   return TraceOk;
}

//------------------------------------------------------------------------------

fn_name SbTracer_SelectProtocol = "SbTracer.SelectProtocol";

TraceRc SbTracer::SelectProtocol(ProtocolId prid, TraceStatus status)
{
   Debug::ft(SbTracer_SelectProtocol);

   if(Singleton< ProtocolRegistry >::Instance()->GetProtocol(prid) == nullptr)
   {
      return NoSuchItem;
   }

   auto buff = Singleton< TraceBuffer >::Instance();

   protocols_[prid] = status;

   if(status == TraceDefault)
   {
      if(ProtocolsEmpty()) buff->ClearFilter(TraceProtocol);
      return TraceOk;
   }

   buff->SetFilter(TraceProtocol);
   return TraceOk;
}

//------------------------------------------------------------------------------

fn_name SbTracer_SelectService = "SbTracer.SelectService";

TraceRc SbTracer::SelectService(ServiceId sid, TraceStatus status)
{
   Debug::ft(SbTracer_SelectService);

   if(Singleton< ServiceRegistry >::Instance()->GetService(sid) == nullptr)
   {
      return NoSuchItem;
   }

   auto buff = Singleton< TraceBuffer >::Instance();

   services_[sid] = status;

   if(status == TraceDefault)
   {
      if(ServicesEmpty()) buff->ClearFilter(TraceService);
      return TraceOk;
   }

   buff->SetFilter(TraceService);
   return TraceOk;
}

//------------------------------------------------------------------------------

fn_name SbTracer_SelectSignal = "SbTracer.SelectSignal";

TraceRc SbTracer::SelectSignal
   (ProtocolId prid, SignalId sid, TraceStatus status)
{
   Debug::ft(SbTracer_SelectSignal);

   auto pro = Singleton< ProtocolRegistry >::Instance()->GetProtocol(prid);

   if(pro == nullptr) return NoSuchItem;

   if(pro->GetSignal(sid) == nullptr) return NoSuchItem;

   auto buff = Singleton< TraceBuffer >::Instance();
   auto i = FindSignal(prid, sid);

   if(i >= 0)
   {
      if(status == TraceDefault)
      {
         signals_[i] = SignalFilter();
         if(SignalsEmpty()) buff->ClearFilter(TraceSignal);
      }
      else
      {
         signals_[i].status = status;
      }

      return TraceOk;
   }

   if(status == TraceDefault) return TraceOk;

   i = FindSignal(NIL_ID, NIL_ID);

   if(i < 0) return RegistryIsFull;
   signals_[i] = SignalFilter(prid, sid, status);
   buff->SetFilter(TraceSignal);
   return TraceOk;
}

//------------------------------------------------------------------------------

fn_name SbTracer_SelectTimers = "SbTracer.SelectTimers";

TraceRc SbTracer::SelectTimers(TraceStatus status)
{
   Debug::ft(SbTracer_SelectTimers);

   auto buff = Singleton< TraceBuffer >::Instance();

   timers_ = status;

   if(status == TraceDefault)
      buff->ClearFilter(TraceTimers);
   else
      buff->SetFilter(TraceTimers);

   return TraceOk;
}

//------------------------------------------------------------------------------

fn_name SbTracer_ServiceIsTraced = "SbTracer.ServiceIsTraced";

bool SbTracer::ServiceIsTraced(ServiceId sid) const
{
   Debug::ft(SbTracer_ServiceIsTraced);

   if(!Debug::TraceOn()) return false;

   auto status = services_[sid];
   if(status == TraceIncluded) return true;
   if(status == TraceExcluded) return false;
   return Singleton< TraceBuffer >::Instance()->FilterIsOn(TraceAll);
}

//------------------------------------------------------------------------------

fn_name SbTracer_ServicesEmpty = "SbTracer.ServicesEmpty";

bool SbTracer::ServicesEmpty() const
{
   Debug::ft(SbTracer_ServicesEmpty);

   for(auto i = 0; i <= Service::MaxId; ++i)
   {
      if(services_[i] != TraceDefault) return false;
   }

   return true;
}

//------------------------------------------------------------------------------

fn_name SbTracer_SignalsEmpty = "SbTracer.SignalsEmpty";

bool SbTracer::SignalsEmpty() const
{
   Debug::ft(SbTracer_SignalsEmpty);

   for(auto i = 0; i < MaxSignalEntries; ++i)
   {
      if(signals_[i].status != TraceDefault) return false;
   }

   return true;
}

//------------------------------------------------------------------------------

fn_name SbTracer_SignalStatus = "SbTracer.SignalStatus";

TraceStatus SbTracer::SignalStatus(ProtocolId prid, SignalId sid) const
{
   Debug::ft(SbTracer_SignalStatus);

   auto i = FindSignal(prid, sid);
   if(i < 0) return TraceDefault;
   return signals_[i].status;
}
}
