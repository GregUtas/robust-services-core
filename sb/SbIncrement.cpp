//==============================================================================
//
//  SbIncrement.cpp
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
#include "SbIncrement.h"
#include "CliCommand.h"
#include "CliPtrParm.h"
#include "CliText.h"
#include <iomanip>
#include <iosfwd>
#include <sstream>
#include <string>
#include "CliThread.h"
#include "Debug.h"
#include "Duration.h"
#include "Event.h"
#include "Factory.h"
#include "FactoryRegistry.h"
#include "Formatters.h"
#include "InvokerPool.h"
#include "InvokerPoolRegistry.h"
#include "LocalAddress.h"
#include "Message.h"
#include "MsgPort.h"
#include "NbCliParms.h"
#include "Parameter.h"
#include "Protocol.h"
#include "ProtocolRegistry.h"
#include "ProtocolSM.h"
#include "Registry.h"
#include "SbCliParms.h"
#include "SbPools.h"
#include "SbTracer.h"
#include "SbTypes.h"
#include "Service.h"
#include "ServiceRegistry.h"
#include "ServiceSM.h"
#include "Signal.h"
#include "Singleton.h"
#include "State.h"
#include "ThisThread.h"
#include "Timer.h"
#include "TimerRegistry.h"
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

class FactoriesText : public CliText
{
public: FactoriesText();
};

class ProtocolText : public CliText
{
public: ProtocolText();
};

class ProtocolsText : public CliText
{
public: ProtocolsText();
};

class ServiceText : public CliText
{
public: ServiceText();
};

class ServicesText : public CliText
{
public: ServicesText();
};

class SignalText : public CliText
{
public: SignalText();
};

class SignalsText : public CliText
{
public: SignalsText();
};

class TimersText : public CliText
{
public: TimersText();
};

fixed_string FactoryTextStr = "factory";
fixed_string FactoryTextExpl = "messages received by a specific factory";

FactoryText::FactoryText() : CliText(FactoryTextExpl, FactoryTextStr)
{
   BindParm(*new FactoryIdMandParm);
}

fixed_string FactoriesTextStr = "factories";
fixed_string FactoriesTextExpl = "all included/excluded factories";

FactoriesText::FactoriesText() :
   CliText(FactoriesTextExpl, FactoriesTextStr) { }

fixed_string ProtocolTextStr = "protocol";
fixed_string ProtocolTextExpl = "messages in a specific protocol";

ProtocolText::ProtocolText() : CliText(ProtocolTextExpl, ProtocolTextStr)
{
   BindParm(*new ProtocolIdMandParm);
}

fixed_string ProtocolsTextStr = "protocols";
fixed_string ProtocolsTextExpl = "all included/excluded protocols";

ProtocolsText::ProtocolsText() :
   CliText(ProtocolsTextExpl, ProtocolsTextStr) { }

fixed_string ServiceTextStr = "service";
fixed_string ServiceTextExpl =
   "contexts in which a specific service is running";

ServiceText::ServiceText() : CliText(ServiceTextExpl, ServiceTextStr)
{
   BindParm(*new ServiceIdMandParm);
}

fixed_string ServicesTextStr = "services";
fixed_string ServicesTextExpl = "all included/excluded services";

ServicesText::ServicesText() : CliText(ServicesTextExpl, ServicesTextStr) { }

fixed_string SignalTextStr = "signal";
fixed_string SignalTextExpl = "messages with a specific protocol and signal";

SignalText::SignalText() : CliText(SignalTextExpl, SignalTextStr)
{
   BindParm(*new ProtocolIdMandParm);
   BindParm(*new SignalIdMandParm);
}

fixed_string SignalsTextStr = "signals";
fixed_string SignalsTextExpl = "all included/excluded signals";

SignalsText::SignalsText() : CliText(SignalsTextExpl, SignalsTextStr) { }

fixed_string TimersTextStr = "timers";
fixed_string TimersTextExpl = "timer registry work";

TimersText::TimersText() : CliText(TimersTextExpl, TimersTextStr) { }

//------------------------------------------------------------------------------
//
//  The CLEAR command.
//
SbClearWhatParm::SbClearWhatParm()
{
   BindText(*new FactoryText, SbClearCommand::FactoryIndex);
   BindText(*new FactoriesText, SbClearCommand::FactoriesIndex);
   BindText(*new ProtocolText, SbClearCommand::ProtocolIndex);
   BindText(*new ProtocolsText, SbClearCommand::ProtocolsIndex);
   BindText(*new SignalText, SbClearCommand::SignalIndex);
   BindText(*new SignalsText, SbClearCommand::SignalsIndex);
   BindText(*new ServiceText, SbClearCommand::ServiceIndex);
   BindText(*new ServicesText, SbClearCommand::ServicesIndex);
   BindText(*new TimersText, SbClearCommand::TimersIndex);
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
   auto sbt = Singleton< SbTracer >::Instance();

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
   BindParm(*new DispCBVParm);
}

word ContextsCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("ContextsCommand.ProcessCommand");

   bool c, v;

   if(GetCBV(*this, cli, c, v) == Error) return -1;
   if(!cli.EndOfInput()) return -1;

   auto pool = Singleton< ContextPool >::Instance();
   auto num = pool->InUseCount();
   auto opts = (v ? VerboseOpt : NoFlags);

   if(c)
      *cli.obuf << spaces(2) << num << CRLF;
   else if(!pool->DisplayUsed(*cli.obuf, spaces(2), opts))
      return cli.Report(-2, NoContextsExpl);

   return num;
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
   BindParm(*new DispBVParm);
}

word EventsCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("EventsCommand.ProcessCommand");

   word sid, eid;
   bool all, one = false, v = false;

   if(!GetIntParm(sid, cli)) return -1;

   switch(GetIntParmRc(eid, cli))
   {
   case None: all = true; break;
   case Ok: all = false; break;
   default: return -1;
   }

   if(GetBV(*this, cli, v) == Error) return -1;
   if(!cli.EndOfInput()) return -1;

   auto svc = Singleton< ServiceRegistry >::Instance()->GetService(sid);

   if(svc == nullptr) return cli.Report(-2, NoServiceExpl);

   *cli.obuf << spaces(2) << strClass(svc) << CRLF;

   if(all)
   {
      *cli.obuf << spaces(4) << "eventNames [EventId]" << CRLF;

      for(auto i = 0; i <= Event::MaxId; ++i)
      {
         auto name = svc->EventName(i);

         if(name != nullptr)
         {
            *cli.obuf << spaces(6) << strIndex(i) << name << CRLF;
            one = true;
         }
      }

      if(!one) return cli.Report(-2, NoEventsExpl, 6);
   }
   else
   {
      auto name = svc->EventName(eid);

      if(name != nullptr)
         *cli.obuf << spaces(4) << name << CRLF;
      else
         return cli.Report(-2, NoEventExpl, 4);
   }

   return 0;
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
   BindText(*new TimersText, SbExcludeCommand::TimersIndex);
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
   auto sbt = Singleton< SbTracer >::Instance();

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
   BindParm(*new DispBVParm);
}

word FactoriesCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("FactoriesCommand.ProcessCommand");

   word fid;
   bool all, v = false;

   switch(GetIntParmRc(fid, cli))
   {
   case None: all = true; break;
   case Ok: all = false; break;
   default: return -1;
   }

   if(GetBV(*this, cli, v) == Error) return -1;
   if(!cli.EndOfInput()) return -1;

   auto reg = Singleton< FactoryRegistry >::Instance();

   if(all)
   {
      reg->Output(*cli.obuf, 2, v);
   }
   else
   {
      auto fac = reg->GetFactory(fid);
      if(fac == nullptr) return cli.Report(-2, NoFactoryExpl);
      fac->Output(*cli.obuf, 2, v);
   }

   return 0;
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
   BindParm(*new DispBVParm);
}

word HandlersCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("HandlersCommand.ProcessCommand");

   word sid, ehid;
   bool all, one = false, v = false;

   if(!GetIntParm(sid, cli)) return -1;

   switch(GetIntParmRc(ehid, cli))
   {
   case None: all = true; break;
   case Ok: all = false; break;
   default: return -1;
   }

   if(GetBV(*this, cli, v) == Error) return -1;
   if(!cli.EndOfInput()) return -1;

   auto svc = Singleton< ServiceRegistry >::Instance()->GetService(sid);
   if(svc == nullptr) return cli.Report(-2, NoServiceExpl);

   *cli.obuf << spaces(2) << strClass(svc) << CRLF;

   if(all)
   {
      *cli.obuf << spaces(4) << "handlers [EventHandlerId]" << CRLF;

      auto& handlers = svc->Handlers();
      auto id = NIL_ID;

      for(auto eh = handlers.First(id); eh != nullptr; eh = handlers.Next(id))
      {
         *cli.obuf << spaces(6) << strIndex(id) << strObj(eh) << CRLF;
         one = true;
      }

      if(!one) return cli.Report(-2, NoHandlersExpl, 6);
   }
   else
   {
      auto handler = svc->GetHandler(ehid);
      if(handler == nullptr) return cli.Report(-2, NoHandlerExpl, 4);
      *cli.obuf << spaces(4) << strObj(handler) << CRLF;
   }

   return 0;
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
   BindText(*new TimersText, SbIncludeCommand::TimersIndex);
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
   auto sbt = Singleton< SbTracer >::Instance();

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
   BindParm(*new DispBVParm);
}

word InvPoolsCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("InvPoolsCommand.ProcessCommand");

   word sc;
   bool all, v = false;

   switch(GetIntParmRc(sc, cli))
   {
   case None: all = true; break;
   case Ok: all = false; break;
   default: return -1;
   }

   if(GetBV(*this, cli, v) == Error) return -1;
   if(!cli.EndOfInput()) return -1;

   auto reg = Singleton< InvokerPoolRegistry >::Instance();

   if(all)
   {
      reg->Output(*cli.obuf, 2, v);
   }
   else
   {
      auto pool = reg->Pool(Faction(sc));
      if(pool == nullptr) return cli.Report(-2, NoInvPoolExpl);
      pool->Output(*cli.obuf, 2, v);
   }

   return 0;
}

//------------------------------------------------------------------------------
//
//  The KILL command.
//
class PsmPtrParm : public CliPtrParm
{
public: PsmPtrParm();
};

class KillCommand : public CliCommand
{
public:
   KillCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string PsmPtrText = "pointer to a PSM";

PsmPtrParm::PsmPtrParm() : CliPtrParm(PsmPtrText) { }

fixed_string KillStr = "kill";
fixed_string KillExpl = "Kills a PSM's context.";

KillCommand::KillCommand() : CliCommand(KillStr, KillExpl)
{
   BindParm(*new PsmPtrParm);
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
   BindParm(*new DispCBVParm);
}

word MessagesCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("MessagesCommand.ProcessCommand");

   word pid, sid;
   bool allProtocols, allSignals, c, v;

   switch(GetIntParmRc(pid, cli))
   {
   case None: allProtocols = true; break;
   case Ok: allProtocols = false; break;
   default: return -1;
   }

   switch(GetIntParmRc(sid, cli))
   {
   case None: allSignals = true; break;
   case Ok: allSignals = false; break;
   default: return -1;
   }

   if(GetCBV(*this, cli, c, v) == Error) return -1;
   if(!cli.EndOfInput()) return -1;

   auto pool = Singleton< MessagePool >::Instance();

   if(c)
   {
      auto num = pool->InUseCount();
      *cli.obuf << spaces(2) << num << CRLF;
      return num;
   }

   PooledObjectId id;
   auto time = 200;
   word count = 0;

   for(auto obj = pool->FirstUsed(id); obj != nullptr; obj = pool->NextUsed(id))
   {
      auto msg = static_cast< Message* >(obj);
      auto show = allProtocols || (msg->GetProtocol() == pid);

      show = show && (allSignals || (msg->GetSignal() == sid));

      if(show)
      {
         ++count;

         if(allProtocols)
         {
            *cli.obuf << spaces(2) << strObj(msg) << CRLF;
            --time;
         }
         else
         {
            msg->Output(*cli.obuf, 2, v);
            time -= 25;
         }

         if(time <= 0)
         {
            ThisThread::Pause();
            time = 200;
         }
      }
   }

   if(count == 0) return cli.Report(-2, NoMessagesExpl);
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
   BindParm(*new DispCBVParm);
}

word MsgPortsCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("MsgPortsCommand.ProcessCommand");

   word fid;
   bool all, c, v;

   switch(GetIntParmRc(fid, cli))
   {
   case None: all = true; break;
   case Ok: all = false; break;
   default: return -1;
   }

   if(GetCBV(*this, cli, c, v) == Error) return -1;
   if(!cli.EndOfInput()) return -1;

   auto pool = Singleton< MsgPortPool >::Instance();

   if(c)
   {
      auto num = pool->InUseCount();
      *cli.obuf << spaces(2) << num << CRLF;
      return num;
   }

   PooledObjectId id;
   word count = 0;
   auto time = 200;

   for(auto obj = pool->FirstUsed(id); obj != nullptr; obj = pool->NextUsed(id))
   {
      auto port = static_cast< MsgPort* >(obj);

      if(all || (port->ObjAddr().fid == fid))
      {
         ++count;

         if(all)
         {
            *cli.obuf << spaces(2) << strObj(port) << CRLF;
            --time;
         }
         else
         {
            port->Output(*cli.obuf, 2, v);
            time -= 25;
         }

         if(time <= 0)
         {
            ThisThread::Pause();
            time = 200;
         }
      }
   }

   if(count == 0) return cli.Report(-2, NoMsgsPortsExpl);
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
   BindParm(*new DispBVParm);
}

word ParametersCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("ParametersCommand.ProcessCommand");

   word prid, pid;
   bool all, one = false, v = false;

   if(!GetIntParm(prid, cli)) return -1;

   switch(GetIntParmRc(pid, cli))
   {
   case None: all = true; break;
   case Ok: all = false; break;
   default: return -1;
   }

   if(GetBV(*this, cli, v) == Error) return -1;
   if(!cli.EndOfInput()) return -1;

   auto pro = Singleton< ProtocolRegistry >::Instance()->GetProtocol(prid);
   if(pro == nullptr) return cli.Report(-2, NoProtocolExpl);

   *cli.obuf << spaces(2) << strClass(pro) << CRLF;

   if(all)
   {
      *cli.obuf << spaces(4) << "parameters [ParameterId]" << CRLF;

      for(auto p = pro->FirstParm(); p != nullptr; pro->NextParm(p))
      {
         *cli.obuf << spaces(6) << strIndex(p->Pid()) << CRLF;
         p->Output(*cli.obuf, 8, v);
         one = true;
      }

      if(!one) return cli.Report(-2, NoParametersExpl, 6);
   }
   else
   {
      auto parm = pro->GetParameter(pid);
      if(parm == nullptr) return cli.Report(-2, NoParameterExpl, 4);
      parm->Output(*cli.obuf, 4, v);
   }

   return 0;
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
   BindParm(*new DispBVParm);
}

word ProtocolsCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("ProtocolsCommand.ProcessCommand");

   word prid;
   bool all, v = false;

   switch(GetIntParmRc(prid, cli))
   {
   case None: all = true; break;
   case Ok: all = false; break;
   default: return -1;
   }

   if(GetBV(*this, cli, v) == Error) return -1;
   if(!cli.EndOfInput()) return -1;

   auto reg = Singleton< ProtocolRegistry >::Instance();

   if(all)
   {
      reg->Output(*cli.obuf, 2, v);
   }
   else
   {
      auto pro = reg->GetProtocol(prid);
      if(pro == nullptr) return cli.Report(-2, NoProtocolExpl);
      pro->Output(*cli.obuf, 2, v);
   }

   return 0;
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
   BindParm(*new DispCBVParm);
}

word PsmsCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("PsmsCommand.ProcessCommand");

   word fid;
   bool all, c, v;

   switch(GetIntParmRc(fid, cli))
   {
   case None: all = true; break;
   case Ok: all = false; break;
   default: return -1;
   }

   if(GetCBV(*this, cli, c, v) == Error) return -1;
   if(!cli.EndOfInput()) return -1;

   auto pool = Singleton< ProtocolSMPool >::Instance();

   if(c)
   {
      auto num = pool->InUseCount();
      *cli.obuf << spaces(2) << num << CRLF;
      return num;
   }

   PooledObjectId id;
   word count = 0;
   auto time = 200;

   for(auto obj = pool->FirstUsed(id); obj != nullptr; obj = pool->NextUsed(id))
   {
      auto psm = static_cast< ProtocolSM* >(obj);

      if(all || (psm->GetFactory() == fid))
      {
         ++count;

         if(all)
         {
            *cli.obuf << spaces(2) << strObj(psm) << CRLF;
            --time;
         }
         else
         {
            psm->Output(*cli.obuf, 2, v);
            time -= 25;
         }

         if(time <= 0)
         {
            ThisThread::Pause();
            time = 200;
         }
      }
   }

   if(count == 0) return cli.Report(-2, NoPsmsExpl);
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

   Singleton< SbTracer >::Instance()->QuerySelections(*cli.obuf);
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
   BindParm(*new DispBVParm);
}

word ServicesCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("ServicesCommand.ProcessCommand");

   word sid;
   bool all, v = false;

   switch(GetIntParmRc(sid, cli))
   {
   case None: all = true; break;
   case Ok: all = false; break;
   default: return -1;
   }

   if(GetBV(*this, cli, v) == Error) return -1;
   if(!cli.EndOfInput()) return -1;

   auto reg = Singleton< ServiceRegistry >::Instance();

   if(all)
   {
      reg->Output(*cli.obuf, 2, v);
   }
   else
   {
      auto svc = reg->GetService(sid);
      if(svc == nullptr) return cli.Report(-2, NoServiceExpl);
      svc->Output(*cli.obuf, 2, v);
   }

   return 0;
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
   BindParm(*new DispBVParm);
}

word SignalsCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("SignalsCommand.ProcessCommand");

   word prid, sid;
   bool all, one = false, v = false;

   if(!GetIntParm(prid, cli)) return -1;

   switch(GetIntParmRc(sid, cli))
   {
   case None: all = true; break;
   case Ok: all = false; break;
   default: return -1;
   }

   if(GetBV(*this, cli, v) == Error) return -1;
   if(!cli.EndOfInput()) return -1;

   auto pro = Singleton< ProtocolRegistry >::Instance()->GetProtocol(prid);
   if(pro == nullptr) return cli.Report(-2, NoProtocolExpl);

   *cli.obuf << spaces(2) << strClass(pro) << CRLF;

   if(all)
   {
      *cli.obuf << spaces(4) << "signals [SignalId]" << CRLF;

      for(auto s = pro->FirstSignal(); s != nullptr; pro->NextSignal(s))
      {
         *cli.obuf << spaces(6) << strIndex(s->Sid()) << CRLF;
         s->Output(*cli.obuf, 8, v);
         one = true;
      }

      if(!one) return cli.Report(-2, NoSignalsExpl, 6);
   }
   else
   {
      auto sig = pro->GetSignal(sid);
      if(sig == nullptr) return cli.Report(-2, NoSignalExpl, 4);
      sig->Output(*cli.obuf, 4, v);
   }

   return 0;
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
   BindParm(*new DispCBVParm);
}

word SsmsCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("SsmsCommand.ProcessCommand");

   word sid;
   bool all, c, v;

   switch(GetIntParmRc(sid, cli))
   {
   case None: all = true; break;
   case Ok: all = false; break;
   default: return -1;
   }

   if(GetCBV(*this, cli, c, v) == Error) return -1;
   if(!cli.EndOfInput()) return -1;

   auto pool = Singleton< ServiceSMPool >::Instance();

   if(c)
   {
      auto num = pool->InUseCount();
      *cli.obuf << spaces(2) << num << CRLF;
      return num;
   }

   PooledObjectId id;
   word count = 0;
   auto time = 200;

   for(auto obj = pool->FirstUsed(id); obj != nullptr; obj = pool->NextUsed(id))
   {
      auto ssm = static_cast< ServiceSM* >(obj);

      if(all || (ssm->Sid() == sid))
      {
         ++count;

         if(all)
         {
            *cli.obuf << spaces(2) << strObj(ssm) << CRLF;
            --time;
         }
         else
         {
            ssm->Output(*cli.obuf, 2, v);
            time -= 25;
         }

         if(time <= 0)
         {
            ThisThread::Pause();
            time = 200;
         }
      }
   }

   if(count == 0) return cli.Report(-2, NoSsmsExpl);
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
   BindParm(*new DispBVParm);
}

word StatesCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("StatesCommand.ProcessCommand");

   word sid, stid;
   bool all, one = false, v = false;

   if(!GetIntParm(sid, cli)) return -1;

   switch(GetIntParmRc(stid, cli))
   {
   case None: all = true; break;
   case Ok: all = false; break;
   default: return -1;
   }

   if(GetBV(*this, cli, v) == Error) return -1;
   if(!cli.EndOfInput()) return -1;

   auto svc = Singleton< ServiceRegistry >::Instance()->GetService(sid);
   if(svc == nullptr) return cli.Report(-2, NoServiceExpl);

   *cli.obuf << spaces(2) << strClass(svc) << CRLF;

   if(all)
   {
      *cli.obuf << spaces(4) << "states [State::Id]" << CRLF;

      auto& states = svc->States();

      for(auto s = states.First(); s != nullptr; states.Next(s))
      {
         *cli.obuf << spaces(6) << strIndex(s->Stid()) << CRLF;
         s->Output(*cli.obuf, 8, v);
         one = true;
      }

      if(!one) return cli.Report(-2, NoStatesExpl, 6);
   }
   else
   {
      auto state = svc->GetState(stid);
      if(state == nullptr) return cli.Report(-2, NoStateExpl);
      state->Output(*cli.obuf, 4, v);
   }

   return 0;
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

   auto& pools = Singleton< InvokerPoolRegistry >::Instance()->Pools();

   for(auto p = pools.First(); p != nullptr; pools.Next(p))
   {
      *cli.obuf << setw(8) << p->WorkQCurrLength(INGRESS);
      *cli.obuf << setw(8) << p->WorkQMaxLength(INGRESS);
      *cli.obuf << setw(8) << p->WorkQMaxDelay(INGRESS).To(mSECS);
      *cli.obuf << spaces(3) << p->GetFaction() << CRLF;
   }

   *cli.obuf << CRLF;
   *cli.obuf << "FACTORY DISCARDS" << CRLF;

   bool one = false;

   auto& facs = Singleton< FactoryRegistry >::Instance()->Factories();

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
   BindParm(*new DispCBVParm);
}

word TimersCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("TimersCommand.ProcessCommand");

   word fid;
   bool all, c, v;

   switch(GetIntParmRc(fid, cli))
   {
   case None: all = true; break;
   case Ok: all = false; break;
   default: return -1;
   }

   if(GetCBV(*this, cli, c, v) == Error) return -1;
   if(!cli.EndOfInput()) return -1;

   auto pool = Singleton< TimerPool >::Instance();

   if(c)
   {
      auto num = pool->InUseCount();
      *cli.obuf << spaces(2) << num << CRLF;
      return num;
   }

   Singleton< TimerRegistry >::Instance()->Output(*cli.obuf, 2, false);

   PooledObjectId id;
   word count = 0;
   auto time = 200;

   for(auto obj = pool->FirstUsed(id); obj != nullptr; obj = pool->NextUsed(id))
   {
      auto tmr = static_cast< Timer* >(obj);

      if(all || (tmr->Psm()->GetFactory() == fid))
      {
         ++count;

         if(all)
         {
            *cli.obuf << spaces(2) << strObj(tmr) << CRLF;
            --time;
         }
         else
         {
            tmr->Output(*cli.obuf, 2, v);
            time -= 25;
         }

         if(time <= 0)
         {
            ThisThread::Pause();
            time = 0;
         }
      }
   }

   if(count == 0) return cli.Report(-2, NoTimersExpl);
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
   BindParm(*new DispBVParm);
}

word TriggersCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("TriggersCommand.ProcessCommand");

   word sid, tid;
   bool all, one = false, v = false;

   if(!GetIntParm(sid, cli)) return -1;

   switch(GetIntParmRc(tid, cli))
   {
   case None: all = true; break;
   case Ok: all = false; break;
   default: return -1;
   }

   if(GetBV(*this, cli, v) == Error) return -1;
   if(!cli.EndOfInput()) return -1;

   auto svc = Singleton< ServiceRegistry >::Instance()->GetService(sid);
   if(svc == nullptr) return cli.Report(-2, NoServiceExpl);

   *cli.obuf << spaces(2) << strClass(svc) << CRLF;

   if(all)
   {
      *cli.obuf << spaces(4) << "triggers [TriggerId]" << CRLF;

      auto& triggers = svc->Triggers();
      auto id = NIL_ID;

      for(auto t = triggers.First(id); t != nullptr; t = triggers.Next(id))
      {
         *cli.obuf << spaces(6) << strIndex(t->Tid()) << CRLF;
         t->Output(*cli.obuf, 8, v);
         one = true;
      }

      if(!one) return cli.Report(-2, NoTriggersExpl, 6);
   }
   else
   {
      auto trigger = svc->GetTrigger(tid);
      if(trigger == nullptr) return cli.Report(-2, NoTriggerExpl, 4);
      trigger->Output(*cli.obuf, 4, v);
   }

   return 0;
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
   BindCommand(*new StatesCommand);
   BindCommand(*new EventsCommand);
   BindCommand(*new HandlersCommand);
   BindCommand(*new TriggersCommand);
   BindCommand(*new FactoriesCommand);
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
