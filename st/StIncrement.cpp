//==============================================================================
//
//  StIncrement.cpp
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
#include "StIncrement.h"
#include <iosfwd>
#include <memory>
#include <sstream>
#include <string>
#include "CliBoolParm.h"
#include "CliCommand.h"
#include "CliIntParm.h"
#include "CliText.h"
#include "CliTextParm.h"
#include "CliThread.h"
#include "Debug.h"
#include "Element.h"
#include "Event.h"
#include "EventHandler.h"
#include "Factory.h"
#include "FactoryRegistry.h"
#include "Formatters.h"
#include "FunctionGuard.h"
#include "GlobalAddress.h"
#include "InitFlags.h"
#include "Initiator.h"
#include "LocalAddress.h"
#include "MscBuilder.h"
#include "MsgHeader.h"
#include "MsgPort.h"
#include "NbCliParms.h"
#include "NbTracer.h"
#include "NtTestData.h"
#include "ProtocolRegistry.h"
#include "ProtocolSM.h"
#include "Registry.h"
#include "RootServiceSM.h"
#include "SbCliParms.h"
#include "SbIpBuffer.h"
#include "SbPools.h"
#include "SbTrace.h"
#include "SbTypes.h"
#include "Service.h"
#include "Signal.h"
#include "Singleton.h"
#include "SsmContext.h"
#include "State.h"
#include "StTestData.h"
#include "SysTcpSocket.h"
#include "SysTypes.h"
#include "SysUdpSocket.h"
#include "TestSessions.h"
#include "TextTlvMessage.h"
#include "ThisThread.h"
#include "Timer.h"
#include "TimerProtocol.h"
#include "TlvParameter.h"
#include "TlvProtocol.h"
#include "ToolTypes.h"
#include "TraceBuffer.h"
#include "Trigger.h"

using namespace NetworkBase;
using std::string;
using namespace SessionBase;

//------------------------------------------------------------------------------

namespace SessionTools
{
//  The CORRUPT command.
//
class ContextText : public CliText
{
public: ContextText();
};

class StCorruptWhatParm : public CorruptWhatParm
{
public: StCorruptWhatParm();
};

class StCorruptCommand : public CorruptCommand
{
public:
   static const id_t ContextIndex = LastNtIndex + 1;

   StCorruptCommand();
private:
   virtual word ProcessSubcommand(CliThread& cli, id_t index) const override;
};

fixed_string ContextTextStr = "context";
fixed_string ContextTextExpl = "first in-use context";

ContextText::ContextText() : CliText(ContextTextExpl, ContextTextStr) { }

StCorruptWhatParm::StCorruptWhatParm()
{
   BindText(*new ContextText, StCorruptCommand::ContextIndex);
}

StCorruptCommand::StCorruptCommand() : CorruptCommand(false)
{
   BindParm(*new StCorruptWhatParm);
}

fn_name StCorruptCommand_ProcessSubcommand =
   "StCorruptCommand.ProcessSubcommand";

word StCorruptCommand::ProcessSubcommand(CliThread& cli, id_t index) const
{
   Debug::ft(StCorruptCommand_ProcessSubcommand);

   if(index != ContextIndex)
      return CorruptCommand::ProcessSubcommand(cli, index);

   PooledObjectId bid;

   cli.EndOfInput(false);

   auto pool = Singleton< ContextPool >::Instance();
   auto ctx = static_cast< Context* >(pool->FirstUsed(bid));
   if(ctx == nullptr) return cli.Report(-2, NoContextExpl);
   ctx->Corrupt();
   return cli.Report(0, SuccessExpl);
}

//------------------------------------------------------------------------------
//
//  Parameters for the Inject and Verify commands.
//
class TestSessionIdMandParm : public CliIntParm
{
public: TestSessionIdMandParm();
};

class TestSessionIdOptParm : public CliIntParm
{
public: TestSessionIdOptParm();
};

class WhichFactoryParm : public CliTextParm
{
public: WhichFactoryParm();
};

class WhichSignalParm : public CliTextParm
{
public: WhichSignalParm();
};

fixed_string TestSessionIdMandExpl = "TestSessionId";

TestSessionIdMandParm::TestSessionIdMandParm() :
   CliIntParm(TestSessionIdMandExpl, 1, TestSession::MaxId) { }

fixed_string TestSessionIdOptExpl = "TestSessionId (default=0: next message)";

TestSessionIdOptParm::TestSessionIdOptParm() :
   CliIntParm(TestSessionIdOptExpl, 0, TestSession::MaxId, true) { }

fixed_string WhichFactoryExpl = "factory abbreviation...";

WhichFactoryParm::WhichFactoryParm() :
   CliTextParm(WhichFactoryExpl, false, Factory::MaxId + 1) { }

fixed_string WhichSignalExpl = "signal abbreviation...";

WhichSignalParm::WhichSignalParm() :
   CliTextParm(WhichSignalExpl, false, Signal::MaxId + 1) { }

//------------------------------------------------------------------------------
//
//  The INJECT command.
//
class InjectCommand : public CliCommand
{
public:
   InjectCommand();
private:
   virtual word ProcessCommand(CliThread& cli) const override;
};

fixed_string InjectStr = "inject";
fixed_string InjectExpl = "Sends a message FROM a factory or one of its PSMs.";

fn_name InjectCommand_ctor = "InjectCommand.ctor";

InjectCommand::InjectCommand() : CliCommand(InjectStr, InjectExpl)
{
   Debug::ft(InjectCommand_ctor);

   auto& facs = Singleton< FactoryRegistry >::Instance()->Factories();
   auto preg = Singleton< ProtocolRegistry >::Instance();

   //  Add the parameter that will contain the factories that support this
   //  command.
   //
   auto fparm = new WhichFactoryParm;
   BindParm(*fparm);

   for(auto fac = facs.First(); fac != nullptr; facs.Next(fac))
   {
      //  Ask each factory to create a text parameter that identifies it;
      //  if it doesn't do so, then it doesn't support this command.
      //
      auto ftext = fac->CreateText();
      if(ftext == nullptr) continue;

      fparm->BindText(*ftext, fac->Fid());

      //  If a factory uses PSMs, add a mandatory parameter to identify a
      //  test PSM that runs in an SSM context which also contains one of
      //  the factory's PSMs.
      //
      if(fac->GetType() != SingleMsg)
      {
         ftext->BindParm(*new TestSessionIdMandParm);
      }

      auto prid = fac->GetProtocol();
      auto pro = preg->GetProtocol(prid);

      auto sparm = new WhichSignalParm;
      ftext->BindParm(*sparm);

      for(auto s = pro->FirstSignal(); s != nullptr; pro->NextSignal(s))
      {
         //  Create a text parameter that identifies each signal that the
         //  factory can send.
         //
         auto sid = s->Sid();
         if(!fac->IsLegalOgSignal(sid)) continue;

         auto stext = s->CreateText();
         if(stext == nullptr) continue;
         sparm->BindText(*stext, sid);

         //  Follow the signal with its mandatory parameters, and then its
         //  optional parameters.
         //
         for(auto use = Parameter::Mandatory; use >= Parameter::Optional; --use)
         {
            for(auto p = pro->FirstParm(); p != nullptr; pro->NextParm(p))
            {
               if(p->GetUsage(sid) == use)
               {
                  auto pparm = p->CreateCliParm(use);
                  if(pparm != nullptr) stext->BindParm(*pparm);
               }
            }
         }
      }

      //  Pause after handling each factory.
      //
      ThisThread::Pause();
   }
}

fn_name InjectCommand_ProcessCommand = "InjectCommand.ProcessCommand";

word InjectCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft(InjectCommand_ProcessCommand);

   id_t fid, sid;
   word tid = 0;
   bool failed = false;

   //  Find the factory associated with the message to be injected.
   //
   if(!GetTextIndex(fid, cli)) return -1;

   auto fac = Singleton< FactoryRegistry >::Instance()->GetFactory(fid);
   if(fac == nullptr) return cli.Report(-2, NoFactoryExpl);

   //  Find the message's protocol.
   //
   auto prid = fac->GetProtocol();
   auto pro = Singleton< ProtocolRegistry >::Instance()->GetProtocol(prid);
   if(pro == nullptr) return cli.Report(-6, NoFactoryProtocol);

   //  If the factory uses a PSM or SSM context, find the session from
   //  whose context the message will be injected.
   //
   if(fac->GetType() != SingleMsg)
   {
      if(!GetIntParm(tid, cli)) return -1;
   }

   //  Find the message's signal.
   //
   if(!GetTextIndex(sid, cli)) return -1;
   auto sig = pro->GetSignal(sid);
   if(sig == nullptr) return cli.Report(-2, NoSignalExpl);

   //  Allocate the message and set its protocol and signal.  Add the
   //  signal's mandatory parameters and then its optional parameters.
   //
   auto msg = fac->AllocOgMsg(sid);
   if(msg == nullptr) return cli.Report(-7, AllocationError);

   msg->SetProtocol(prid);
   msg->SetSignal(sid);
   msg->Header()->injected = true;

   for(auto use = Parameter::Mandatory; use >= Parameter::Optional; --use)
   {
      for(auto p = pro->FirstParm(); p != nullptr; pro->NextParm(p))
      {
         if(p->GetUsage(sid) == use)
         {
            auto rc = p->InjectMsg(cli, *msg, use);
            if(rc == Parameter::Ok) continue;

            *cli.obuf << spaces(2) << BadParameterExpl << "pid="
               << int(p->Pid()) << " (" << strClass(p)<< "),"
               << CRLF << "rc: " << Parameter::ExplainRc(rc) << CRLF;
            failed = true;
         }
      }
   }

   cli.EndOfInput(false);

   //  Inject the message.  If there is no session, have the factory send
   //  the message directly.  If there is a session, queue the message and
   //  send a message to the corresponding test PSM, which will eventually
   //  result in the message being retrieved and sent.  Include the sender's
   //  factory, which is needed to create the application PSM if this is an
   //  initial message.
   //
   if(!failed)
   {
      if(tid == 0)
      {
         if(!fac->InjectMsg(*msg)) failed = true;
      }
      else
      {
         auto test = StTestData::Access(cli);

         msg->Header()->txAddr.fid = fid;
         if(!test->InjectMsg(*msg, tid)) failed = true;
      }

      if(failed) *cli.obuf << spaces(2) << SendFailure << CRLF;
   }

   //  If the message could not be sent, delete it, else report success.
   //
   if(failed)
   {
      delete msg;
      return -3;
   }

   return cli.Report(0, SuccessExpl);
}

//------------------------------------------------------------------------------
//
//  The SAVE command.
//
class DebugTraceParm : public CliBoolParm
{
public: DebugTraceParm();
};

class MscText : public CliText
{
public: MscText();
};

class StSaveWhatParm : public NtSaveWhatParm
{
public: StSaveWhatParm();
};

class StSaveCommand : public NtSaveCommand
{
public:
   static const id_t MscIndex = LastNtIndex + 1;

   StSaveCommand();
private:
   virtual word ProcessSubcommand(CliThread& cli, id_t index) const override;
};

fixed_string DebugTraceExpl = "include internal data structures? (default=f)";

DebugTraceParm::DebugTraceParm() : CliBoolParm(DebugTraceExpl, true) { }

fixed_string MscTextStr = "msc";
fixed_string MscTextExpl = "message sequence chart";

MscText::MscText() : CliText(MscTextExpl, MscTextStr)
{
   BindParm(*new FileMandParm);
   BindParm(*new DebugTraceParm);
}

StSaveWhatParm::StSaveWhatParm()
{
   BindText(*new MscText, StSaveCommand::MscIndex);
}

StSaveCommand::StSaveCommand() : NtSaveCommand(false)
{
   BindParm(*new StSaveWhatParm);
}

fn_name StSaveCommand_ProcessSubcommand = "StSaveCommand.ProcessSubcommand";

word StSaveCommand::ProcessSubcommand(CliThread& cli, id_t index) const
{
   Debug::ft(StSaveCommand_ProcessSubcommand);

   if(index != MscIndex) return NtSaveCommand::ProcessSubcommand(cli, index);

   TraceRc rc;
   string title;
   bool debug = false;

   if(!GetFileName(title, cli)) return -1;
   if(GetBoolParmRc(debug, cli) == Error) return -1;
   cli.EndOfInput(false);

   auto stream = cli.FileStream();
   if(stream == nullptr) return cli.Report(-7, CreateStreamFailure);

   auto buff = Singleton< TraceBuffer >::Instance();
   if(buff->Empty()) return ExplainTraceRc(cli, BufferEmpty);

   auto yield = cli.GenerateReportPreemptably();
   FunctionGuard guard(FunctionGuard::MakePreemptable, yield);

   auto msc = std::unique_ptr< MscBuilder >(new MscBuilder(debug));
   if(msc == nullptr) return cli.Report(-7, AllocationError);
   rc = msc->Generate(*stream);
   msc.reset();

   if(rc == TraceOk)
   {
      title += ".msc.txt";
      cli.SendToFile(title, true);
   }

   return ExplainTraceRc(cli, rc);
}

//------------------------------------------------------------------------------
//
//  The SIZES command.
//
void StSizesCommand::DisplaySizes(CliThread& cli, bool all) const
{
   if(all)
   {
      SizesCommand::DisplaySizes(cli, all);
      *cli.obuf << CRLF;
   }

   //e IpBuffer, SysIpL2Addr, SysIpL3Addr, and SysUdpSocket should go in a
   //  Network layer tools increment.  The Sizes command is the only thing
   //  that such an increment would currently contain, so it has not been
   //  implemented.
   //
   *cli.obuf << "  BuffTrace = " << sizeof(BuffTrace) << CRLF;
   *cli.obuf << "  Context = " << sizeof(Context) << CRLF;
   *cli.obuf << "  Event = " << sizeof(Event) << CRLF;
   *cli.obuf << "  EventHandler = " << sizeof(EventHandler) << CRLF;
   *cli.obuf << "  Factory = " << sizeof(Factory) << CRLF;
   *cli.obuf << "  GlobalAddress = " << sizeof(GlobalAddress) << CRLF;
   *cli.obuf << "  Initiator = " << sizeof(Initiator) << CRLF;
   *cli.obuf << "  IpBuffer = " << sizeof(IpBuffer) << CRLF;
   *cli.obuf << "  LocalAddress = " << sizeof(LocalAddress) << CRLF;
   *cli.obuf << "  Message = " << sizeof(Message) << CRLF;
   *cli.obuf << "  MsgContext = " << sizeof(MsgContext) << CRLF;
   *cli.obuf << "  MsgHeader = " << sizeof(MsgHeader) << CRLF;
   *cli.obuf << "  MsgPort = " << sizeof(MsgPort) << CRLF;
   *cli.obuf << "  Parameter = " << sizeof(Parameter) << CRLF;
   *cli.obuf << "  Protocol = " << sizeof(Protocol) << CRLF;
   *cli.obuf << "  ProtocolLayer = " << sizeof(ProtocolLayer) << CRLF;
   *cli.obuf << "  ProtocolSM = " << sizeof(ProtocolSM) << CRLF;
   *cli.obuf << "  PsmContext = " << sizeof(PsmContext) << CRLF;
   *cli.obuf << "  RootServiceSM = " << sizeof(RootServiceSM) << CRLF;
   *cli.obuf << "  SbIpBuffer = " << sizeof(SbIpBuffer) << CRLF;
   *cli.obuf << "  Service = " << sizeof(Service) << CRLF;
   *cli.obuf << "  ServiceSM = " << sizeof(ServiceSM) << CRLF;
   *cli.obuf << "  Signal = " << sizeof(Signal) << CRLF;
   *cli.obuf << "  SsmContext = " << sizeof(SsmContext) << CRLF;
   *cli.obuf << "  State = " << sizeof(State) << CRLF;
   *cli.obuf << "  SysIpL3Addr = " << sizeof(SysIpL3Addr) << CRLF;
   *cli.obuf << "  SysTcpSocket = " << sizeof(SysTcpSocket) << CRLF;
   *cli.obuf << "  SysUdpSocket = " << sizeof(SysUdpSocket) << CRLF;
   *cli.obuf << "  TextTlvMessage = " << sizeof(TextTlvMessage) << CRLF;
   *cli.obuf << "  TimeoutParameter = " << sizeof(TimeoutParameter) << CRLF;
   *cli.obuf << "  Timer = " << sizeof(Timer) << CRLF;
   *cli.obuf << "  TlvMessage = " << sizeof(TlvMessage) << CRLF;
   *cli.obuf << "  TlvParameter = " << sizeof(TlvParameter) << CRLF;
   *cli.obuf << "  TlvParmHeader = " << sizeof(TlvParmHeader) << CRLF;
   *cli.obuf << "  TlvProtocol = " << sizeof(TlvProtocol) << CRLF;
   *cli.obuf << "  Trigger = " << sizeof(Trigger) << CRLF;
}

fn_name StSizesCommand_ProcessCommand = "StSizesCommand.ProcessCommand";

word StSizesCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft(StSizesCommand_ProcessCommand);

   bool all = false;

   if(GetBoolParmRc(all, cli) == Error) return -1;
   cli.EndOfInput(false);
   *cli.obuf << spaces(2) << SizesHeader << CRLF;
   DisplaySizes(cli, all);
   return 0;
}

//------------------------------------------------------------------------------
//
//  The TESTCASE command.
//
class TestVerifyText : public CliText
{
public: TestVerifyText();
};

class StTestcaseAction : public TestcaseAction
{
public: StTestcaseAction();
};

class StTestcaseCommand : public TestcaseCommand
{
public:
   static const id_t TestVerifyIndex = LastNtIndex + 1;

   StTestcaseCommand();
private:
   virtual word ProcessSubcommand(CliThread& cli, id_t index) const override;
   virtual void InitiateTest
      (CliThread& cli, const string& curr) const override;
};

fixed_string TestVerifyTextStr = "verify";
fixed_string TestVerifyTextExpl = "enables or disables the >verify command";

TestVerifyText::TestVerifyText() :
   CliText(TestVerifyTextExpl, TestVerifyTextStr)
{
   BindParm(*new SetHowParm);
}

StTestcaseAction::StTestcaseAction()
{
   BindText(*new TestVerifyText, StTestcaseCommand::TestVerifyIndex);
}

StTestcaseCommand::StTestcaseCommand() : TestcaseCommand(false)
{
   BindParm(*new StTestcaseAction);
}

fn_name StTestcaseCommand_InitiateTest = "StTestcaseCommand.InitiateTest";

void StTestcaseCommand::InitiateTest(CliThread& cli, const string& curr) const
{
   Debug::ft(StTestcaseCommand_InitiateTest);

   auto test = StTestData::Access(cli);

   test->SetVerify(true);
   TestcaseCommand::InitiateTest(cli, curr);
}

fn_name StTestcaseCommand_ProcessSubcommand =
   "StTestcaseCommand.ProcessSubcommand";

word StTestcaseCommand::ProcessSubcommand(CliThread& cli, id_t index) const
{
   Debug::ft(StTestcaseCommand_ProcessSubcommand);

   if(index != TestVerifyIndex)
   {
      return TestcaseCommand::ProcessSubcommand(cli, index);
   }

   auto test = StTestData::Access(cli);
   if(test == nullptr) return cli.Report(-7, CreateStreamFailure);

   id_t setHowIndex;
   bool flag = false;

   if(!GetTextIndex(setHowIndex, cli)) return -1;
   cli.EndOfInput(false);
   flag = (setHowIndex == SetHowParm::On);
   test->SetVerify(flag);
   return cli.Report(0, SuccessExpl);
}

//------------------------------------------------------------------------------
//
//  The VERIFY command.
//
const SkipInfo NilSkipInfo = {0, 0};

class VerifyCommand : public CliCommand
{
public:
   VerifyCommand();
private:
   virtual word ProcessCommand(CliThread& cli) const override;
};

fixed_string VerifyStr = "verify";
fixed_string VerifyExpl =
   "Checks a message RECEIVED by a factory or one of its PSMs.";

fn_name VerifyCommand_ctor = "VerifyCommand.ctor";

VerifyCommand::VerifyCommand() : CliCommand(VerifyStr, VerifyExpl)
{
   Debug::ft(VerifyCommand_ctor);

   auto& facs = Singleton< FactoryRegistry >::Instance()->Factories();
   auto preg = Singleton< ProtocolRegistry >::Instance();

   //  Add the parameter that will contain the factories that support this
   //  command.
   //
   auto fparm = new WhichFactoryParm;
   BindParm(*fparm);

   for(auto fac = facs.First(); fac != nullptr; facs.Next(fac))
   {
      //  Ask each factory to create a text parameter that identifies it;
      //  if it doesn't do so, then it doesn't support this command.
      //
      auto ftext = fac->CreateText();
      if(ftext == nullptr) continue;

      fparm->BindText(*ftext, fac->Fid());

      //  If a factory uses PSMs, add an optional parameter to identify a
      //  test PSM that runs in an SSM context which also contains one of
      //  the factory's PSMs.
      //
      if(fac->GetType() != SingleMsg)
      {
         ftext->BindParm(*new TestSessionIdOptParm);
      }

      //  Find the factory's protocol and create a parameter that will
      //  contain each signal that the factory can receive.
      //
      auto prid = fac->GetProtocol();
      auto pro = preg->GetProtocol(prid);

      auto sparm = new WhichSignalParm;
      ftext->BindParm(*sparm);

      for(auto s = pro->FirstSignal(); s != nullptr; pro->NextSignal(s))
      {
         //  Create a text parameter that identifies each signal that the
         //  factory can receive.
         //
         auto sid = s->Sid();
         if(!fac->IsLegalIcSignal(sid)) continue;

         auto stext = s->CreateText();
         if(stext == nullptr) continue;
         sparm->BindText(*stext, sid);

         //  Follow the signal with its mandatory parameters, and then its
         //  optional parameters.
         //
         for(auto use = Parameter::Mandatory; use >= Parameter::Optional; --use)
         {
            for(auto p = pro->FirstParm(); p != nullptr; pro->NextParm(p))
            {
               if(p->GetUsage(sid) == use)
               {
                  auto pparm = p->CreateCliParm(use);
                  if(pparm != nullptr) stext->BindParm(*pparm);
               }
            }
         }
      }

      //  Pause after handling each factory.
      //
      ThisThread::Pause();
   }
}

fn_name VerifyCommand_ProcessCommand = "VerifyCommand.ProcessCommand";

word VerifyCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft(VerifyCommand_ProcessCommand);

   auto ntest = NtTestData::Access(cli);
   if(ntest == nullptr) return cli.Report(-7, CreateStreamFailure);

   auto stest = StTestData::Access(cli);
   if(stest == nullptr) return cli.Report(-7, CreateStreamFailure);

   id_t fid, sid;
   word tid = 0;
   bool failed = false;

   //  Return if the command is currently disabled.
   //
   if(!stest->VerifyOn()) return 0;

   //  Find the factory associated with the expected message.
   //
   if(!GetTextIndex(fid, cli)) return -1;

   auto fac = Singleton< FactoryRegistry >::Instance()->GetFactory(fid);
   if(fac == nullptr) return cli.Report(-2, NoFactoryExpl);

   //  Find the expected message's protocol.
   //
   auto prid = fac->GetProtocol();
   auto pro = Singleton< ProtocolRegistry >::Instance()->GetProtocol(prid);
   if(pro == nullptr) return cli.Report(-6, NoFactoryProtocol);

   //  If the factory uses a PSM or SSM context, see if a session is to be
   //  associated with the expected message.
   //
   if(fac->GetType() != SingleMsg)
   {
      switch(GetIntParmRc(tid, cli))
      {
      case Ok:
      case None:
         break;
      default:
         return -1;
      }
   }

   //  Find the expected message's signal.
   //
   if(!GetTextIndex(sid, cli)) return -1;

   auto sig = pro->GetSignal(sid);
   if(sig == nullptr) return ntest->SetFailed(-2, NoSignalExpl);

   Message* msg = nullptr;
   auto skip = NilSkipInfo;

   //  If a session is associated with SID, ask it to find the candidate
   //  message.
   //
   //  A candidate message must match the expected factory, protocol, signal,
   //  and PSM.  It parameters will then be compared to the expected values.
   //
   if(tid != 0)
   {
      auto sess = stest->AccessSession(tid);
      if(sess == nullptr) return ntest->SetFailed(-4, AllocationError);
      msg = sess->NextIcMsg(fid, sid, skip);
   }
   else
   {
      msg = stest->NextIcMsg(fid, sid, skip);
   }

   if(msg == nullptr) return ntest->SetFailed(-3, MessageNotFound);

   //  If any messages were skipped before the candidate message was found,
   //  note this in the test results.
   //
   if(skip.count > 0)
   {
      sig = pro->GetSignal(skip.first);
      *cli.obuf << spaces(2) << SkippedMessagesExpl << skip.count << CRLF;
      *cli.obuf << spaces(2) << SkippedFirstExpl;

      if(sig != nullptr)
         *cli.obuf << strClass(sig, false);
      else
         *cli.obuf << skip.first;

      *cli.obuf << CRLF;
   }

   //  Iterate over the protocol's mandatory and optional parameters,
   //  verifying that the message contains the expected value for each.
   //  Note any mismatches in the test results.
   //
   for(auto use = Parameter::Mandatory; use >= Parameter::Illegal; --use)
   {
      for(auto p = pro->FirstParm(); p != nullptr; pro->NextParm(p))
      {
         if(p->GetUsage(sid) == use)
         {
            auto rc = p->VerifyMsg(cli, *msg, use);
            if(rc == Parameter::Ok) continue;

            std::ostringstream expl;

            expl << "pid=" << int(p->Pid()) << " (" << strClass(p)
               << ")," << CRLF << "rc: " << Parameter::ExplainRc(rc);

            ntest->SetFailed(-3, expl.str());
            failed = true;
         }
      }
   }

   cli.EndOfInput(false);
   delete msg;
   if(failed) return -3;
   return cli.Report(0, SuccessExpl);
}

//------------------------------------------------------------------------------
//
//  The SessionBase tools and test increment.
//
fixed_string StIncrText = "st";
fixed_string StIncrExpl = "SessionBase Tools and Tests";

fn_name StIncrement_ctor = "StIncrement.ctor";

StIncrement::StIncrement() : CliIncrement(StIncrText, StIncrExpl)
{
   Debug::ft(StIncrement_ctor);

   BindCommand(*new StSaveCommand);
   BindCommand(*new StTestcaseCommand);
   BindCommand(*new StSizesCommand);
   BindCommand(*new StCorruptCommand);

   //  See if SessionBase activity should be immediately traced on start-up.
   //
   if(InitFlags::TraceWork() && Element::RunningInLab())
   {
      auto buff = Singleton< TraceBuffer >::Instance();
      auto nbt = Singleton< NbTracer >::Instance();

      if(!Debug::TraceOn())
      {
         buff->SetTool(FunctionTracer, true);
         buff->SetTool(TransTracer, true);
         buff->SetTool(BufferTracer, true);
         buff->SetTool(ContextTracer, true);
         nbt->SelectFaction(PayloadFaction, TraceIncluded);
         buff->StartTrace(InitFlags::ImmediateTrace());
      }
   }
}

//------------------------------------------------------------------------------

fn_name StIncrement_dtor = "StIncrement.dtor";

StIncrement::~StIncrement()
{
   Debug::ft(StIncrement_dtor);
}

//------------------------------------------------------------------------------

fn_name StIncrement_Enter = "StIncrement.Enter";

void StIncrement::Enter()
{
   Debug::ft(StIncrement_Enter);

   //  The binding of these commands is deferred until the increment
   //  is first entered because their parameters can only be created
   //  once all factories and protocols have registered during system
   //  initialization.
   //
   if(FindCommand(InjectStr) == nullptr)
   {
      BindCommand(*new InjectCommand);
      ThisThread::Pause();
   }

   if(FindCommand(VerifyStr) == nullptr)
   {
      BindCommand(*new VerifyCommand);
      ThisThread::Pause();
   }
}
}
