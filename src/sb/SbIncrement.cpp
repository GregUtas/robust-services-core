//==============================================================================
//
//  SbIncrement.cpp
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
#include "SbIncrement.h"
#include "CliCommand.h"
#include "CliText.h"
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iosfwd>
#include <sstream>
#include <string>
#include <vector>
#include "CliPtrParm.h"
#include "CliThread.h"
#include "Debug.h"
#include "Duration.h"
#include "Event.h"
#include "Factory.h"
#include "FactoryRegistry.h"
#include "Formatters.h"
#include "InvokerPool.h"
#include "InvokerPoolRegistry.h"
#include "NbCliParms.h"
#include "Parameter.h"
#include "Protocol.h"
#include "ProtocolRegistry.h"
#include "ProtocolSM.h"
#include "Q1Way.h"
#include "Registry.h"
#include "SbCliParms.h"
#include "SbPools.h"
#include "SbTracer.h"
#include "SbTypes.h"
#include "Service.h"
#include "ServiceRegistry.h"
#include "Signal.h"
#include "Singleton.h"
#include "State.h"
#include "Tool.h"
#include "ToolTypes.h"
#include "Trigger.h"

using namespace NodeBase;
using std::setw;

//------------------------------------------------------------------------------

namespace SessionBase
{
//  Parameters for trace tools.
//
class FactoryText : public CliText
{
public: FactoryText();
};

class ProtocolText : public CliText
{
public: ProtocolText();
};

class ServiceText : public CliText
{
public: ServiceText();
};

class SignalText : public CliText
{
public: SignalText();
};

fixed_string FactoryTextStr = "factory";
fixed_string FactoryTextExpl = "messages received by a specific factory";

FactoryText::FactoryText() : CliText(FactoryTextExpl, FactoryTextStr)
{
   BindParm(*new FactoryIdMandParm);
}

fixed_string FactoriesTextStr = "factories";
fixed_string FactoriesTextExpl = "all included/excluded factories";

fixed_string ProtocolTextStr = "protocol";
fixed_string ProtocolTextExpl = "messages in a specific protocol";

ProtocolText::ProtocolText() : CliText(ProtocolTextExpl, ProtocolTextStr)
{
   BindParm(*new ProtocolIdMandParm);
}

fixed_string ProtocolsTextStr = "protocols";
fixed_string ProtocolsTextExpl = "all included/excluded protocols";

fixed_string ServiceTextStr = "service";
fixed_string ServiceTextExpl =
   "contexts in which a specific service is running";

ServiceText::ServiceText() : CliText(ServiceTextExpl, ServiceTextStr)
{
   BindParm(*new ServiceIdMandParm);
}

fixed_string ServicesTextStr = "services";
fixed_string ServicesTextExpl = "all included/excluded services";

fixed_string SignalTextStr = "signal";
fixed_string SignalTextExpl = "messages with a specific protocol and signal";

SignalText::SignalText() : CliText(SignalTextExpl, SignalTextStr)
{
   BindParm(*new ProtocolIdMandParm);
   BindParm(*new SignalIdMandParm);
}

fixed_string SignalsTextStr = "signals";
fixed_string SignalsTextExpl = "all included/excluded signals";

fixed_string TimersTextStr = "timers";
fixed_string TimersTextExpl = "timer registry work";

//------------------------------------------------------------------------------
//
//  The CLEAR command.
//
SbClearWhatParm::SbClearWhatParm()
{
   BindText(*new FactoryText, SbClearCommand::FactoryIndex);
   BindText(*new CliText
      (FactoriesTextExpl, FactoriesTextStr), SbClearCommand::FactoriesIndex);
   BindText(*new ProtocolText, SbClearCommand::ProtocolIndex);
   BindText(*new CliText
      (ProtocolsTextExpl, ProtocolsTextStr), SbClearCommand::ProtocolsIndex);
   BindText(*new SignalText, SbClearCommand::SignalIndex);
   BindText(*new CliText
      (SignalsTextExpl, SignalsTextStr), SbClearCommand::SignalsIndex);
   BindText(*new ServiceText, SbClearCommand::ServiceIndex);
   BindText(*new CliText
      (ServicesTextExpl, ServicesTextStr), SbClearCommand::ServicesIndex);
   BindText(*new CliText
      (TimersTextExpl, TimersTextStr), SbClearCommand::TimersIndex);
}

SbClearCommand::SbClearCommand(bool bind) : NwClearCommand(false)
{
   if(bind) BindParm(*new SbClearWhatParm);
}

word SbClearCommand::ProcessSubcommand(CliThread& cli, id_t index) const
{
   Debug::ft("SbClearCommand.ProcessSubcommand");

   TraceRc rc;
   word id1 = 0;
   word id2 = 0;
   auto sbt = Singleton<SbTracer>::Instance();

   switch(index)
   {
   case SelectionsIndex:
      if(!cli.EndOfInput()) return -1;
      rc = sbt->ClearSelections(TraceAll);
      break;
   case FactoryIndex:
      if(!GetIntParm(id1, cli)) return -1;
      if(!cli.EndOfInput()) return -1;
      rc = sbt->SelectFactory(id1, TraceDefault);
      break;
   case FactoriesIndex:
      if(!cli.EndOfInput()) return -1;
      rc = sbt->ClearSelections(TraceFactory);
      break;
   case ProtocolIndex:
      if(!GetIntParm(id1, cli)) return -1;
      if(!cli.EndOfInput()) return -1;
      rc = sbt->SelectProtocol(id1, TraceDefault);
      break;
   case ProtocolsIndex:
      if(!cli.EndOfInput()) return -1;
      rc = sbt->ClearSelections(TraceProtocol);
      break;
   case SignalIndex:
      if(!GetIntParm(id1, cli)) return -1;
      if(!GetIntParm(id2, cli)) return -1;
      if(!cli.EndOfInput()) return -1;
      rc = sbt->SelectSignal(id1, id2, TraceDefault);
      break;
   case SignalsIndex:
      if(!cli.EndOfInput()) return -1;
      rc = sbt->ClearSelections(TraceSignal);
      break;
   case ServiceIndex:
      if(!GetIntParm(id1, cli)) return -1;
      if(!cli.EndOfInput()) return -1;
      rc = sbt->SelectService(id1, TraceDefault);
      break;
   case ServicesIndex:
      if(!cli.EndOfInput()) return -1;
      rc = sbt->ClearSelections(TraceService);
      break;
   case TimersIndex:
      if(!cli.EndOfInput()) return -1;
      rc = sbt->SelectTimers(TraceDefault);
      break;
   default:
      return NwClearCommand::ProcessSubcommand(cli, index);
   }

   return ExplainTraceRc(cli, rc);
}

//------------------------------------------------------------------------------
//
//  The CONTEXTS command.
//
class ContextsCommand : public CliCommand
{
public:
   ContextsCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string ContextsStr = "contexts";
fixed_string ContextsExpl = "Counts or displays contexts.";

ContextsCommand::ContextsCommand() : CliCommand(ContextsStr, ContextsExpl)
{
   BindParm(*new DispCSVParm);
}

word ContextsCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("ContextsCommand.ProcessCommand");

   char disp;

   if(GetCharParmRc(disp, cli) == Error) return -1;
   if(!cli.EndOfInput()) return -1;

   auto pool = Singleton<ContextPool>::Instance();
   size_t count = 0;

   if(disp == 'c')
   {
      count = pool->InUseCount();
      *cli.obuf << spaces(2) << count << CRLF;
   }
   else if(disp == 's')
   {
      count = pool->Summarize(*cli.obuf, 0);
   }
   else
   {
      count = pool->DisplayUsed(*cli.obuf, spaces(2), VerboseOpt, 0);
      if(count == 0) return cli.Report(0, NoContextsExpl);
   }

   return count;
}

//------------------------------------------------------------------------------
//
//  The EVENTS command.
//
class EventsCommand : public CliCommand
{
public:
   EventsCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string EventsStr = "events";
fixed_string EventsExpl = "Displays a service's event names.";

EventsCommand::EventsCommand() : CliCommand(EventsStr, EventsExpl)
{
   BindParm(*new ServiceIdMandParm);
   BindParm(*new EventIdOptParm);
   BindParm(*new DispCSBVParm);
}

word EventsCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("EventsCommand.ProcessCommand");

   word sid, id;
   char disp;

   if(!GetIntParm(sid, cli)) return -1;
   if(!GetIdDispV(*this, cli, id, disp)) return -1;
   if(!cli.EndOfInput()) return -1;

   auto svc = Singleton<ServiceRegistry>::Instance()->Services().At(sid);
   if(svc == nullptr) return cli.Report(-2, NoServiceExpl);
   auto size = svc->EventCount();

   if(disp == 'c')
   {
      *cli.obuf << size << CRLF;
   }
   else if(disp == 's')
   {
      svc->Summarize(*cli.obuf, Service::SummarizeEvents);
   }
   else if(id == NIL_ID)
   {
      *cli.obuf << spaces(2) << strClass(svc) << CRLF;
      *cli.obuf << spaces(4) << "eventNames [EventId]" << CRLF;
      if(size == 0) return cli.Report(0, NoEventsExpl, 6);

      for(auto i = 0; i <= Event::MaxId; ++i)
      {
         auto name = svc->EventName(i);
         if(name != nullptr)
         {
            *cli.obuf << spaces(6) << strIndex(i) << name << CRLF;
         }
      }
   }
   else
   {
      *cli.obuf << spaces(2) << strClass(svc) << CRLF;
      auto name = svc->EventName(id);
      if(name != nullptr)
         *cli.obuf << spaces(4) << name << CRLF;
      else
         return cli.Report(0, NoEventExpl, 4);
      return 1;
   }

   return size;
}

//------------------------------------------------------------------------------
//
//  The EXCLUDE command.
//
SbExcludeWhatParm::SbExcludeWhatParm()
{
   BindText(*new FactoryText, SbExcludeCommand::FactoryIndex);
   BindText(*new ProtocolText, SbExcludeCommand::ProtocolIndex);
   BindText(*new SignalText, SbExcludeCommand::SignalIndex);
   BindText(*new ServiceText, SbExcludeCommand::ServiceIndex);
   BindText(*new CliText
      (TimersTextExpl, TimersTextStr), SbExcludeCommand::TimersIndex);
}

SbExcludeCommand::SbExcludeCommand(bool bind) : NwExcludeCommand(false)
{
   if(bind) BindParm(*new SbExcludeWhatParm);
}

word SbExcludeCommand::ProcessSubcommand(CliThread& cli, id_t index) const
{
   Debug::ft("SbExcludeCommand.ProcessSubcommand");

   TraceRc rc;
   word id1, id2;
   auto sbt = Singleton<SbTracer>::Instance();

   switch(index)
   {
   case FactoryIndex:
      if(!GetIntParm(id1, cli)) return -1;
      if(!cli.EndOfInput()) return -1;
      rc = sbt->SelectFactory(id1, TraceExcluded);
      break;

   case ProtocolIndex:
      if(!GetIntParm(id1, cli)) return -1;
      if(!cli.EndOfInput()) return -1;
      rc = sbt->SelectProtocol(id1, TraceExcluded);
      break;

   case ServiceIndex:
      if(!GetIntParm(id1, cli)) return -1;
      if(!cli.EndOfInput()) return -1;
      rc = sbt->SelectService(id1, TraceExcluded);
      break;

   case SignalIndex:
      if(!GetIntParm(id1, cli)) return -1;
      if(!GetIntParm(id2, cli)) return -1;
      if(!cli.EndOfInput()) return -1;
      rc = sbt->SelectSignal(id1, id2, TraceExcluded);
      break;

   case TimersIndex:
      if(!cli.EndOfInput()) return -1;
      rc = sbt->SelectTimers(TraceExcluded);
      break;

   default:
      return NwExcludeCommand::ProcessSubcommand(cli, index);
   }

   return ExplainTraceRc(cli, rc);
}

//------------------------------------------------------------------------------
//
//  The FACTORIES command.
//
class FactoriesCommand : public CliCommand
{
public:
   FactoriesCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string FactoriesStr = "factories";
fixed_string FactoriesExpl = "Displays factories.";

FactoriesCommand::FactoriesCommand() : CliCommand(FactoriesStr, FactoriesExpl)
{
   BindParm(*new FactoryIdOptParm);
   BindParm(*new DispCSBVParm);
}

word FactoriesCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("FactoriesCommand.ProcessCommand");

   word id;
   char disp;

   if(!GetIdDispV(*this, cli, id, disp)) return -1;
   if(!cli.EndOfInput()) return -1;

   auto reg = Singleton<FactoryRegistry>::Instance();
   auto size = reg->Factories().Size();

   if(disp == 'c')
   {
      *cli.obuf << size << CRLF;
   }
   else if(disp == 's')
   {
      reg->Summarize(*cli.obuf, 0);
   }
   else if(id == NIL_ID)
   {
      reg->Output(*cli.obuf, 2, disp == 'v');
   }
   else
   {
      auto fac = reg->Factories().At(id);
      if(fac == nullptr) return cli.Report(0, NoFactoryExpl);
      fac->Output(*cli.obuf, 2, disp == 'v');
      return 1;
   }

   return size;
}

//------------------------------------------------------------------------------
//
//  The HANDLERS command.
//
class HandlersCommand : public CliCommand
{
public:
   HandlersCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string HandlersStr = "handlers";
fixed_string HandlersExpl = "Displays a service's event handlers.";

HandlersCommand::HandlersCommand() : CliCommand(HandlersStr, HandlersExpl)
{
   BindParm(*new ServiceIdMandParm);
   BindParm(*new HandlerIdOptParm);
   BindParm(*new DispCSBVParm);
}

word HandlersCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("HandlersCommand.ProcessCommand");

   word sid, id;
   char disp;

   if(!GetIntParm(sid, cli)) return -1;
   if(!GetIdDispV(*this, cli, id, disp)) return -1;
   if(!cli.EndOfInput()) return -1;

   auto svc = Singleton<ServiceRegistry>::Instance()->Services().At(sid);
   if(svc == nullptr) return cli.Report(-2, NoServiceExpl);
   auto& handlers = svc->Handlers();
   auto size = handlers.Size();

   if(disp == 'c')
   {
      *cli.obuf << size << CRLF;
   }
   else if(disp == 's')
   {
      svc->Summarize(*cli.obuf, Service::SummarizeHandlers);
   }
   else if(id == NIL_ID)
   {
      *cli.obuf << spaces(2) << strClass(svc) << CRLF;
      *cli.obuf << spaces(4) << "handlers [EventHandlerId]" << CRLF;
      if(size == 0) return cli.Report(0, NoHandlersExpl, 6);

      id_t hid = NIL_ID;

      for(auto h = handlers.First(hid); h != nullptr; h = handlers.Next(hid))
      {
         *cli.obuf << spaces(6) << strIndex(hid) << strObj(h) << CRLF;
      }
   }
   else
   {
      *cli.obuf << spaces(2) << strClass(svc) << CRLF;
      auto handler = handlers.At(id);
      if(handler == nullptr) return cli.Report(0, NoHandlerExpl, 4);
      *cli.obuf << spaces(4) << strObj(handler) << CRLF;
      return 1;
   }

   return size;
}

//------------------------------------------------------------------------------
//
//  The INCLUDE command.
//
SbIncludeWhatParm::SbIncludeWhatParm()
{
   BindText(*new FactoryText, SbIncludeCommand::FactoryIndex);
   BindText(*new ProtocolText, SbIncludeCommand::ProtocolIndex);
   BindText(*new SignalText, SbIncludeCommand::SignalIndex);
   BindText(*new ServiceText, SbIncludeCommand::ServiceIndex);
   BindText(*new CliText
      (TimersTextExpl, TimersTextStr), SbIncludeCommand::TimersIndex);
}

SbIncludeCommand::SbIncludeCommand(bool bind) : NwIncludeCommand(false)
{
   if(bind) BindParm(*new SbIncludeWhatParm);
}

word SbIncludeCommand::ProcessSubcommand(CliThread& cli, id_t index) const
{
   Debug::ft("SbIncludeCommand.ProcessSubcommand");

   TraceRc rc;
   word id1, id2;
   auto sbt = Singleton<SbTracer>::Instance();

   switch(index)
   {
   case FactoryIndex:
      if(!GetIntParm(id1, cli)) return -1;
      if(!cli.EndOfInput()) return -1;
      rc = sbt->SelectFactory(id1, TraceIncluded);
      break;

   case ProtocolIndex:
      if(!GetIntParm(id1, cli)) return -1;
      if(!cli.EndOfInput()) return -1;
      rc = sbt->SelectProtocol(id1, TraceIncluded);
      break;

   case SignalIndex:
      if(!GetIntParm(id1, cli)) return -1;
      if(!GetIntParm(id2, cli)) return -1;
      if(!cli.EndOfInput()) return -1;
      rc = sbt->SelectSignal(id1, id2, TraceIncluded);
      break;

   case ServiceIndex:
      if(!GetIntParm(id1, cli)) return -1;
      if(!cli.EndOfInput()) return -1;
      rc = sbt->SelectService(id1, TraceIncluded);
      break;

   case TimersIndex:
      if(!cli.EndOfInput()) return -1;
      rc = sbt->SelectTimers(TraceIncluded);
      break;

   default:
      return NwIncludeCommand::ProcessSubcommand(cli, index);
   }

   return ExplainTraceRc(cli, rc);
}

//------------------------------------------------------------------------------
//
//  The INITIATORS command.
//
class InitiatorsCommand : public CliCommand
{
public:
   InitiatorsCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string InitiatorsStr = "initiators";
fixed_string InitiatorsExpl =
   "Displays the initiators registered with a trigger.";

InitiatorsCommand::InitiatorsCommand() :
   CliCommand(InitiatorsStr, InitiatorsExpl)
{
   BindParm(*new ServiceIdMandParm);
   BindParm(*new TriggerIdMandParm);
   BindParm(*new DispCSBVParm);
}

word InitiatorsCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("InitiatorsCommand.ProcessCommand");

   word sid, tid;
   char disp;

   if(!GetIntParm(sid, cli)) return -1;
   if(!GetIntParm(tid, cli)) return -1;

   switch(GetCharParmRc(disp, cli))
   {
   case CliParm::None: disp = 's'; break;
   case CliParm::Ok: break;
   default: return -1;
   }

   if(!cli.EndOfInput()) return -1;

   auto svc = Singleton<ServiceRegistry>::Instance()->Services().At(sid);
   if(svc == nullptr) return cli.Report(-2, NoServiceExpl);
   auto trigger = svc->Triggers().At(tid);
   if(trigger == nullptr) return cli.Report(-2, NoTriggerExpl);
   auto size = trigger->Initiators().Size();

   if(disp == 'c')
   {
      *cli.obuf << size << CRLF;
   }
   else if(disp == 's')
   {
      trigger->Summarize(*cli.obuf, 0);
   }
   else
   {
      *cli.obuf << spaces(2) << strClass(svc) << CRLF;
      trigger->Output(*cli.obuf, 4, true);
   }

   return size;
}

//------------------------------------------------------------------------------
//
//  The INVPOOLS command.
//
class InvPoolsCommand : public CliCommand
{
public:
   InvPoolsCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string InvPoolsStr = "invpools";
fixed_string InvPoolsExpl = "Displays invoker pools.";

InvPoolsCommand::InvPoolsCommand() : CliCommand(InvPoolsStr, InvPoolsExpl)
{
   BindParm(*new FactionOptParm);
   BindParm(*new DispCSBVParm);
}

word InvPoolsCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("InvPoolsCommand.ProcessCommand");

   word sf;
   char disp;

   if(!GetIdDispV(*this, cli, sf, disp)) return -1;
   if(!cli.EndOfInput()) return -1;

   auto reg = Singleton<InvokerPoolRegistry>::Instance();
   auto size = reg->Pools().Size();

   if(disp == 'c')
   {
      *cli.obuf << size << CRLF;
   }
   else if(disp == 's')
   {
      reg->Summarize(*cli.obuf, 0);
   }
   else if(sf == IdleFaction)
   {
      reg->Output(*cli.obuf, 2, disp == 'v');
   }
   else
   {
      auto pool = reg->Pool(Faction(sf));
      if(pool == nullptr) return cli.Report(0, NoInvPoolExpl);
      pool->Output(*cli.obuf, 2, disp == 'v');
      return 1;
   }

   return size;
}

//------------------------------------------------------------------------------
//
//  The KILL command.
//
class KillCommand : public CliCommand
{
public:
   KillCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string PsmPtrText = "pointer to a PSM";

fixed_string KillStr = "kill";
fixed_string KillExpl = "Kills a PSM's context.";

KillCommand::KillCommand() : CliCommand(KillStr, KillExpl)
{
   BindParm(*new CliPtrParm(PsmPtrText));
}

word KillCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("KillCommand.ProcessCommand");

   void* p = nullptr;
   std::ostringstream prompt;

   if(!GetPtrParm(p, cli)) return -1;
   if(!cli.EndOfInput()) return -1;

   prompt << BadObjectPtrWarning << CRLF << ContinuePrompt;
   if(!cli.BoolPrompt(prompt.str())) return cli.Report(0, CommandAbortedExpl);
   ((ProtocolSM*) p)->Kill();
   return cli.Report(0, SuccessExpl);
}

//------------------------------------------------------------------------------
//
//  The MESSAGES command.
//
class MessagesCommand : public CliCommand
{
public:
   MessagesCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string MessagesStr = "messages";
fixed_string MessagesExpl = "Counts or displays messages.";

MessagesCommand::MessagesCommand() : CliCommand(MessagesStr, MessagesExpl)
{
   BindParm(*new ProtocolIdOptParm);
   BindParm(*new SignalIdOptParm);
   BindParm(*new DispCSVParm);
}

word MessagesCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("MessagesCommand.ProcessCommand");

   word pid, sid;
   char disp;

   if(GetIntParmRc(pid, cli) == Error) return -1;
   if(!GetIdDispS(*this, cli, sid, disp)) return -1;
   if(!cli.EndOfInput()) return -1;

   auto pool = Singleton<MessagePool>::Instance();
   uint32_t filter = (pid << 8) + sid;
   size_t count = 0;

   if(disp == 'c')
   {
      auto items = pool->GetUsed();
      for(auto i = items.cbegin(); i != items.cend(); ++i)
         if((*i)->Passes(filter)) ++count;
      if(count == 0) return cli.Report(0, NoMessagesExpl);
      *cli.obuf << spaces(2) << count << CRLF;
   }
   else if(disp == 's')
   {
      count = pool->Summarize(*cli.obuf, filter);
   }
   else
   {
      count = pool->DisplayUsed(*cli.obuf, spaces(2), VerboseOpt, filter);
   }

   return count;
}

//------------------------------------------------------------------------------
//
//  The MSGPORTS command.
//
class MsgPortsCommand : public CliCommand
{
public:
   MsgPortsCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string MsgPortsStr = "msgports";
fixed_string MsgPortsExpl = "Counts or displays message ports.";

MsgPortsCommand::MsgPortsCommand() : CliCommand(MsgPortsStr, MsgPortsExpl)
{
   BindParm(*new FactoryIdOptParm);
   BindParm(*new DispCSVParm);
}

word MsgPortsCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("MsgPortsCommand.ProcessCommand");

   word fid;
   char disp;

   if(!GetIdDispS(*this, cli, fid, disp)) return -1;
   if(!cli.EndOfInput()) return -1;

   auto pool = Singleton<MsgPortPool>::Instance();
   size_t count = 0;

   if(disp == 'c')
   {
      auto items = pool->GetUsed();
      for(auto i = items.cbegin(); i != items.cend(); ++i)
         if((*i)->Passes(fid)) ++count;
      if(count == 0) return cli.Report(0, NoMsgsPortsExpl);
      *cli.obuf << spaces(2) << count << CRLF;
   }
   else if(disp == 's')
   {
      count = pool->Summarize(*cli.obuf, fid);
   }
   else
   {
      count = pool->DisplayUsed(*cli.obuf, spaces(2), VerboseOpt, fid);
   }

   return count;
}

//------------------------------------------------------------------------------
//
//  The PARAMETERS command.
//
class ParametersCommand : public CliCommand
{
public:
   ParametersCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string ParametersStr = "parameters";
fixed_string ParametersExpl = "Displays a protocol's parameters.";

ParametersCommand::ParametersCommand() :
   CliCommand(ParametersStr, ParametersExpl)
{
   BindParm(*new ProtocolIdMandParm);
   BindParm(*new ParameterIdOptParm);
   BindParm(*new DispCSBVParm);
}

word ParametersCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("ParametersCommand.ProcessCommand");

   word prid, id;
   char disp;

   if(!GetIntParm(prid, cli)) return -1;
   if(!GetIdDispV(*this, cli, id, disp)) return -1;
   if(!cli.EndOfInput()) return -1;

   auto pro = Singleton<ProtocolRegistry>::Instance()->Protocols().At(prid);
   if(pro == nullptr) return cli.Report(-2, NoProtocolExpl);
   auto& parms = pro->Parameters();
   auto size = parms.Size();

   if(disp == 'c')
   {
      *cli.obuf << size << CRLF;
   }
   else if(disp == 's')
   {
      pro->Summarize(*cli.obuf, Protocol::SummarizeParameters);
   }
   else if(id == NIL_ID)
   {
      *cli.obuf << spaces(2) << strClass(pro) << CRLF;
      *cli.obuf << spaces(4) << "parameters [ParameterId]" << CRLF;

      auto one = false;

      for(auto p = pro->FirstParm(); p != nullptr; pro->NextParm(p))
      {
         *cli.obuf << spaces(6) << strIndex(p->Pid()) << CRLF;
         p->Output(*cli.obuf, 8, disp == 'v');
         one = true;
      }

      if(!one) return cli.Report(0, NoParametersExpl, 6);
   }
   else
   {
      *cli.obuf << spaces(2) << strClass(pro) << CRLF;
      auto parm = pro->GetParameter(id);
      if(parm == nullptr) return cli.Report(0, NoParameterExpl, 4);
      parm->Output(*cli.obuf, 4, disp == 'v');
      return 1;
   }

   return size;
}

//------------------------------------------------------------------------------
//
//  The PROTOCOLS command.
//
class ProtocolsCommand : public CliCommand
{
public:
   ProtocolsCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string ProtocolsStr = "protocols";
fixed_string ProtocolsExpl = "Displays protocols.";

ProtocolsCommand::ProtocolsCommand() : CliCommand(ProtocolsStr, ProtocolsExpl)
{
   BindParm(*new ProtocolIdOptParm);
   BindParm(*new DispCSBVParm);
}

word ProtocolsCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("ProtocolsCommand.ProcessCommand");

   word id;
   char disp;

   if(!GetIdDispV(*this, cli, id, disp)) return -1;
   if(!cli.EndOfInput()) return -1;

   auto reg = Singleton<ProtocolRegistry>::Instance();
   auto size = reg->Protocols().Size();

   if(disp == 'c')
   {
      *cli.obuf << size << CRLF;
   }
   else if(disp == 's')
   {
      reg->Summarize(*cli.obuf, 0);
   }
   else if(id == NIL_ID)
   {
      reg->Output(*cli.obuf, 2, disp == 'v');
   }
   else
   {
      auto pro = reg->Protocols().At(id);
      if(pro == nullptr) return cli.Report(0, NoProtocolExpl);
      pro->Output(*cli.obuf, 2, disp == 'v');
      return 1;
   }

   return size;
}

//------------------------------------------------------------------------------
//
//  The PSMS command.
//
class PsmsCommand : public CliCommand
{
public:
   PsmsCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string PsmsStr = "psms";
fixed_string PsmsExpl = "Counts or displays protocol state machines.";

PsmsCommand::PsmsCommand() : CliCommand(PsmsStr, PsmsExpl)
{
   BindParm(*new FactoryIdOptParm);
   BindParm(*new StateIdOptParm);
   BindParm(*new DispCSVParm);
}

word PsmsCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("PsmsCommand.ProcessCommand");

   word fid = NIL_ID;
   word stid = NIL_ID;
   char disp;

   if(GetIntParmRc(fid, cli) == Error) return -1;
   if(!GetIdDispS(*this, cli, stid, disp)) return -1;
   if(!cli.EndOfInput()) return -1;

   auto pool = Singleton<ProtocolSMPool>::Instance();
   uint32_t selector = (fid << 8) + stid;
   size_t count = 0;

   if(disp == 'c')
   {
      auto items = pool->GetUsed();
      for(auto i = items.cbegin(); i != items.cend(); ++i)
         if((*i)->Passes(fid)) ++count;
      if(count == 0) return cli.Report(0, NoPsmsExpl);
      *cli.obuf << spaces(2) << count << CRLF;
   }
   else if(disp == 's')
   {
      count = pool->Summarize(*cli.obuf, selector);
   }
   else
   {
      auto opts = (disp == 'v' ? VerboseOpt : NoFlags);
      count = pool->DisplayUsed(*cli.obuf, spaces(2), opts, fid);
   }

   return count;
}

//------------------------------------------------------------------------------
//
//  The QUERY command.
//
SbQueryCommand::SbQueryCommand(bool bind) : NwQueryCommand(bind) { }

word SbQueryCommand::ProcessSubcommand(CliThread& cli, id_t index) const
{
   Debug::ft("SbQueryCommand.ProcessSubcommand");

   if(index != SelectionsIndex)
   {
      return NwQueryCommand::ProcessSubcommand(cli, index);
   }

   Singleton<SbTracer>::Instance()->QuerySelections(*cli.obuf);
   return 0;
}

//------------------------------------------------------------------------------
//
//  The SERVICES command.
//
class ServicesCommand : public CliCommand
{
public:
   ServicesCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string ServicesStr = "services";
fixed_string ServicesExpl = "Displays services.";

ServicesCommand::ServicesCommand() : CliCommand(ServicesStr, ServicesExpl)
{
   BindParm(*new ServiceIdOptParm);
   BindParm(*new DispCSBVParm);
}

word ServicesCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("ServicesCommand.ProcessCommand");

   word id;
   char disp;

   if(!GetIdDispV(*this, cli, id, disp)) return -1;
   if(!cli.EndOfInput()) return -1;

   auto reg = Singleton<ServiceRegistry>::Instance();
   auto size = reg->Services().Size();

   if(disp == 'c')
   {
      *cli.obuf << size << CRLF;
   }
   else if(disp == 's')
   {
      reg->Summarize(*cli.obuf, 0);
   }
   else if(id == NIL_ID)
   {
      reg->Output(*cli.obuf, 2, disp == 'v');
   }
   else
   {
      auto svc = reg->Services().At(id);
      if(svc == nullptr) return cli.Report(0, NoServiceExpl);
      svc->Output(*cli.obuf, 2, disp == 'v');
      return 1;
   }

   return size;
}

//------------------------------------------------------------------------------
//
//  The SIGNALS command.
//
class SignalsCommand : public CliCommand
{
public:
   SignalsCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string SignalsStr = "signals";
fixed_string SignalsExpl = "Displays a protocol's signals.";

SignalsCommand::SignalsCommand() : CliCommand(SignalsStr, SignalsExpl)
{
   BindParm(*new ProtocolIdMandParm);
   BindParm(*new SignalIdOptParm);
   BindParm(*new DispCSBVParm);
}

word SignalsCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("SignalsCommand.ProcessCommand");

   word prid, id;
   char disp;

   if(!GetIntParm(prid, cli)) return -1;
   if(!GetIdDispV(*this, cli, id, disp)) return -1;
   if(!cli.EndOfInput()) return -1;

   auto pro = Singleton<ProtocolRegistry>::Instance()->Protocols().At(prid);
   if(pro == nullptr) return cli.Report(-2, NoProtocolExpl);
   auto& signals = pro->Signals();
   auto size = signals.Size();

   if(disp == 'c')
   {
      *cli.obuf << size << CRLF;
   }
   else if(disp == 's')
   {
      pro->Summarize(*cli.obuf, Protocol::SummarizeSignals);
   }
   else if(id == NIL_ID)
   {
      *cli.obuf << spaces(2) << strClass(pro) << CRLF;
      *cli.obuf << spaces(4) << "signals [SignalId]" << CRLF;

      auto one = false;

      for(auto s = pro->FirstSignal(); s != nullptr; pro->NextSignal(s))
      {
         *cli.obuf << spaces(6) << strIndex(s->Sid()) << CRLF;
         s->Output(*cli.obuf, 8, disp == 'v');
         one = true;
      }

      if(!one) return cli.Report(0, NoSignalsExpl, 6);
   }
   else
   {
      *cli.obuf << spaces(2) << strClass(pro) << CRLF;
      auto sig = pro->GetSignal(id);
      if(sig == nullptr) return cli.Report(0, NoSignalExpl, 4);
      sig->Output(*cli.obuf, 4, disp == 'v');
      return 1;
   }

   return size;
}

//------------------------------------------------------------------------------
//
//  The SSMS command.
//
class SsmsCommand : public CliCommand
{
public:
   SsmsCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string SsmsStr = "ssms";
fixed_string SsmsExpl = "Counts or displays service state machines.";

SsmsCommand::SsmsCommand() : CliCommand(SsmsStr, SsmsExpl)
{
   BindParm(*new ServiceIdOptParm);
   BindParm(*new StateIdOptParm);
   BindParm(*new DispCSVParm);
}

word SsmsCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("SsmsCommand.ProcessCommand");

   word sid = NIL_ID;
   word stid = NIL_ID;
   char disp;

   if(GetIntParmRc(sid, cli) == Error) return -1;
   if(!GetIdDispS(*this, cli, stid, disp)) return -1;
   if(!cli.EndOfInput()) return -1;

   auto pool = Singleton<ServiceSMPool>::Instance();
   uint32_t selector = (sid << 8) + stid;
   size_t count = 0;

   if(disp == 'c')
   {
      auto items = pool->GetUsed();
      for(auto i = items.cbegin(); i != items.cend(); ++i)
         if((*i)->Passes(sid)) ++count;
      if(count == 0) return cli.Report(0, NoSsmsExpl);
      *cli.obuf << spaces(2) << count << CRLF;
   }
   else if(disp == 's')
   {
      count = pool->Summarize(*cli.obuf, selector);
   }
   else
   {
      auto opts = (disp == 'v' ? VerboseOpt : NoFlags);
      count = pool->DisplayUsed(*cli.obuf, spaces(2), opts, sid);
   }

   return count;
}

//------------------------------------------------------------------------------
//
//  The STATES command.
//
class StatesCommand : public CliCommand
{
public:
   StatesCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string StatesStr = "states";
fixed_string StatesExpl = "Displays a service's states.";

StatesCommand::StatesCommand() : CliCommand(StatesStr, StatesExpl)
{
   BindParm(*new ServiceIdMandParm);
   BindParm(*new StateIdOptParm);
   BindParm(*new DispCSBVParm);
}

word StatesCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("StatesCommand.ProcessCommand");

   word sid, id;
   char disp;

   if(!GetIntParm(sid, cli)) return -1;
   if(!GetIdDispV(*this, cli, id, disp)) return -1;
   if(!cli.EndOfInput()) return -1;

   auto svc = Singleton<ServiceRegistry>::Instance()->Services().At(sid);
   if(svc == nullptr) return cli.Report(-2, NoServiceExpl);
   auto& states = svc->States();
   auto size = states.Size();

   if(disp == 'c')
   {
      *cli.obuf << size << CRLF;
   }
   else if(disp == 's')
   {
      svc->Summarize(*cli.obuf, Service::SummarizeStates);
   }
   else if(id == NIL_ID)
   {
      *cli.obuf << spaces(2) << strClass(svc) << CRLF;
      *cli.obuf << spaces(4) << "states [State::Id]" << CRLF;
      if(size == 0) return cli.Report(0, NoStatesExpl, 6);

      for(auto s = states.First(); s != nullptr; states.Next(s))
      {
         *cli.obuf << spaces(6) << strIndex(s->Stid()) << CRLF;
         s->Output(*cli.obuf, 8, disp == 'v');
      }
   }
   else
   {
      *cli.obuf << spaces(2) << strClass(svc) << CRLF;
      auto state = svc->States().At(id);
      if(state == nullptr) return cli.Report(0, NoStateExpl);
      state->Output(*cli.obuf, 4, disp == 'v');
      return 1;
   }

   return size;
}

//------------------------------------------------------------------------------
//
//  The STATUS command.
//
word SbStatusCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("SbStatusCommand.ProcessCommand");

   NwStatusCommand::ProcessCommand(cli);

   *cli.obuf << CRLF;
   *cli.obuf << "INGRESS WORK QUEUES" << CRLF;
   *cli.obuf << "    Curr     Max     Max" << CRLF;
   *cli.obuf << "  Length  Length   Delay   Faction" << CRLF;

   auto& pools = Singleton<InvokerPoolRegistry>::Instance()->Pools();

   for(auto p = pools.First(); p != nullptr; pools.Next(p))
   {
      *cli.obuf << setw(8) << p->WorkQCurrLength(INGRESS);
      *cli.obuf << setw(8) << p->WorkQMaxLength(INGRESS);
      *cli.obuf << setw(8) << p->WorkQMaxDelay(INGRESS).count() / NS_TO_MS;
      *cli.obuf << spaces(3) << p->GetFaction() << CRLF;
   }

   *cli.obuf << CRLF;
   *cli.obuf << "FACTORY DISCARDS" << CRLF;

   bool one = false;

   auto& facs = Singleton<FactoryRegistry>::Instance()->Factories();

   for(auto f = facs.First(); f != nullptr; facs.Next(f))
   {
      auto msgs = f->DiscardedMessageCount();
      auto ctxs = f->DiscardedContextCount();

      if((msgs > 0) || (ctxs > 0))
      {
         if(!one) *cli.obuf << "   Msgs   Ctxs  Factory" << CRLF;
         one = true;
         *cli.obuf << setw(7) << msgs;
         *cli.obuf << setw(7) << ctxs;
         *cli.obuf << spaces(2) << f->Name() << CRLF;
      }
   }

   if(!one) *cli.obuf << spaces(2) << NoDiscardsExpl << CRLF;

   return 0;
}

//------------------------------------------------------------------------------
//
//  The TIMERS command.
//
class TimersCommand : public CliCommand
{
public:
   TimersCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string TimersStr = "timers";
fixed_string TimersExpl = "Counts or displays timers.";

TimersCommand::TimersCommand() : CliCommand(TimersStr, TimersExpl)
{
   BindParm(*new FactoryIdOptParm);
   BindParm(*new DispCSVParm);
}

word TimersCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("TimersCommand.ProcessCommand");

   word fid;
   char disp;

   if(!GetIdDispS(*this, cli, fid, disp)) return -1;
   if(!cli.EndOfInput()) return -1;

   auto pool = Singleton<TimerPool>::Instance();
   size_t count = 0;

   if(disp == 'c')
   {
      auto items = pool->GetUsed();
      for(auto i = items.cbegin(); i != items.cend(); ++i)
         if((*i)->Passes(fid)) ++count;
      count = items.size();
      if(count == 0) return cli.Report(0, NoTimersExpl);
      *cli.obuf << spaces(2) << count << CRLF;
   }
   else if(disp == 's')
   {
      count = pool->Summarize(*cli.obuf, fid);
   }
   else
   {
      auto opts = (disp == 'v' ? VerboseOpt : NoFlags);
      count = pool->DisplayUsed(*cli.obuf, spaces(2), opts, fid);
   }

   return count;
}

//------------------------------------------------------------------------------
//
//  The TRIGGERS command.
//
class TriggersCommand : public CliCommand
{
public:
   TriggersCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string TriggersStr = "triggers";
fixed_string TriggersExpl = "Displays a service's triggers.";

TriggersCommand::TriggersCommand() : CliCommand(TriggersStr, TriggersExpl)
{
   BindParm(*new ServiceIdMandParm);
   BindParm(*new TriggerIdOptParm);
   BindParm(*new DispCSBVParm);
}

word TriggersCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("TriggersCommand.ProcessCommand");

   word sid, id;
   char disp;

   if(!GetIntParm(sid, cli)) return -1;
   if(!GetIdDispV(*this, cli, id, disp)) return -1;
   if(!cli.EndOfInput()) return -1;

   auto svc = Singleton<ServiceRegistry>::Instance()->Services().At(sid);
   if(svc == nullptr) return cli.Report(-2, NoServiceExpl);
   auto& triggers = svc->Triggers();
   auto size = triggers.Size();

   if(disp == 'c')
   {
      *cli.obuf << size << CRLF;
   }
   else if(disp == 's')
   {
      svc->Summarize(*cli.obuf, Service::SummarizeTriggers);
   }
   else if(id == NIL_ID)
   {
      *cli.obuf << spaces(2) << strClass(svc) << CRLF;
      *cli.obuf << spaces(4) << "triggers [TriggerId]" << CRLF;
      if(size == 0) return cli.Report(0, NoTriggersExpl, 6);

      auto tid = NIL_ID;

      for(auto t = triggers.First(tid); t != nullptr; t = triggers.Next(tid))
      {
         *cli.obuf << spaces(6) << strIndex(t->Tid()) << CRLF;
         t->Output(*cli.obuf, 8, disp == 'v');
      }
   }
   else
   {
      *cli.obuf << spaces(2) << strClass(svc) << CRLF;
      auto trigger = triggers.At(id);
      if(trigger == nullptr) return cli.Report(0, NoTriggerExpl, 4);
      trigger->Output(*cli.obuf, 4, disp == 'v');
      return 1;
   }

   return size;
}

//------------------------------------------------------------------------------
//
//  The SessionBase increment.
//
fixed_string SessionsText = "sb";
fixed_string SessionsExpl = "SessionBase Increment";

SbIncrement::SbIncrement() : CliIncrement(SessionsText, SessionsExpl)
{
   Debug::ft("SbIncrement.ctor");

   BindCommand(*new ServicesCommand);
   BindCommand(*new FactoriesCommand);
   BindCommand(*new StatesCommand);
   BindCommand(*new EventsCommand);
   BindCommand(*new HandlersCommand);
   BindCommand(*new TriggersCommand);
   BindCommand(*new InitiatorsCommand);
   BindCommand(*new ProtocolsCommand);
   BindCommand(*new SignalsCommand);
   BindCommand(*new ParametersCommand);
   BindCommand(*new ContextsCommand);
   BindCommand(*new SsmsCommand);
   BindCommand(*new PsmsCommand);
   BindCommand(*new MsgPortsCommand);
   BindCommand(*new MessagesCommand);
   BindCommand(*new TimersCommand);
   BindCommand(*new InvPoolsCommand);
   BindCommand(*new SbStatusCommand);
   BindCommand(*new SbIncludeCommand);
   BindCommand(*new SbExcludeCommand);
   BindCommand(*new SbQueryCommand);
   BindCommand(*new SbClearCommand);
   BindCommand(*new KillCommand);
}

//------------------------------------------------------------------------------

SbIncrement::~SbIncrement()
{
   Debug::ftnt("SbIncrement.dtor");
}
}
