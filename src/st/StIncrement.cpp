//==============================================================================
//
//  StIncrement.cpp
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
#include "StIncrement.h"
#include "CliCommand.h"
#include "CliText.h"
#include "NtIncrement.h"
#include <iosfwd>
#include <memory>
#include <sstream>
#include <string>
#include "CliBoolParm.h"
#include "CliIntParm.h"
#include "CliThread.h"
#include "Context.h"
#include "Debug.h"
#include "Factory.h"
#include "FactoryRegistry.h"
#include "Formatters.h"
#include "FunctionGuard.h"
#include "LocalAddress.h"
#include "Message.h"
#include "MscBuilder.h"
#include "MsgHeader.h"
#include "NbCliParms.h"
#include "NtTestData.h"
#include "Parameter.h"
#include "Protocol.h"
#include "ProtocolRegistry.h"
#include "Registry.h"
#include "SbCliParms.h"
#include "SbPools.h"
#include "SbTypes.h"
#include "Signal.h"
#include "Singleton.h"
#include "StTestData.h"
#include "SysTypes.h"
#include "TestSessions.h"
#include "ThisThread.h"
#include "ToolTypes.h"
#include "TraceBuffer.h"

using std::string;
using namespace NodeTools;
using namespace SessionBase;

//------------------------------------------------------------------------------

namespace SessionTools
{
//  The CORRUPT command.
//
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
   word ProcessSubcommand(CliThread& cli, id_t index) const override;
};

fixed_string ContextTextStr = "context";
fixed_string ContextTextExpl = "first in-use context";

StCorruptWhatParm::StCorruptWhatParm()
{
   BindText(*new CliText
      (ContextTextExpl, ContextTextStr), StCorruptCommand::ContextIndex);
}

StCorruptCommand::StCorruptCommand() : CorruptCommand(false)
{
   BindParm(*new StCorruptWhatParm);
}

word StCorruptCommand::ProcessSubcommand(CliThread& cli, id_t index) const
{
   Debug::ft("StCorruptCommand.ProcessSubcommand");

   if(index != ContextIndex)
      return CorruptCommand::ProcessSubcommand(cli, index);

   PooledObjectId bid;

   if(!cli.EndOfInput()) return -1;

   auto pool = Singleton<ContextPool>::Instance();
   auto ctx = static_cast<Context*>(pool->FirstUsed(bid));
   if(ctx == nullptr) return cli.Report(-2, NoContextExpl);
   ctx->Corrupt();
   return cli.Report(0, SuccessExpl);
}

//------------------------------------------------------------------------------
//
//  Parameters for the Inject and Verify commands.
//
fixed_string TestSessionIdMandExpl = "TestSessionId";

fixed_string TestSessionIdOptExpl = "TestSessionId (default=0: next message)";

fixed_string WhichFactoryExpl = "factory abbreviation...";

fixed_string WhichSignalExpl = "signal abbreviation...";

//------------------------------------------------------------------------------
//
//  The INJECT command.
//
class InjectCommand : public CliCommand
{
public:
   InjectCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string InjectStr = "inject";
fixed_string InjectExpl = "Sends a message FROM a factory or one of its PSMs.";

InjectCommand::InjectCommand() : CliCommand(InjectStr, InjectExpl)
{
   Debug::ft("InjectCommand.ctor");

   auto& facs = Singleton<FactoryRegistry>::Instance()->Factories();
   auto preg = Singleton<ProtocolRegistry>::Instance();

   //  Add the parameter that will contain the factories that support this
   //  command.
   //
   auto fparm = new CliTextParm(WhichFactoryExpl, false, Factory::MaxId + 1);
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
         ftext->BindParm(*new CliIntParm
            (TestSessionIdMandExpl, 1, TestSession::MaxId));
      }

      auto prid = fac->GetProtocol();
      auto pro = preg->GetProtocol(prid);

      auto sparm = new CliTextParm(WhichSignalExpl, false, Signal::MaxId + 1);
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

word InjectCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("InjectCommand.ProcessCommand");

   id_t fid, sid;
   word tid = 0;
   auto failed = false;

   //  Find the factory associated with the message to be injected.
   //
   if(!GetTextIndex(fid, cli)) return -1;

   auto fac = Singleton<FactoryRegistry>::Instance()->GetFactory(fid);
   if(fac == nullptr) return cli.Report(-2, NoFactoryExpl);

   //  Find the message's protocol.
   //
   auto prid = fac->GetProtocol();
   auto pro = Singleton<ProtocolRegistry>::Instance()->GetProtocol(prid);
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
               << int(p->Pid()) << " (" << strClass(p) << "),"
               << CRLF << "rc: " << Parameter::ExplainRc(rc) << CRLF;
            failed = true;
         }
      }
   }

   if(!cli.EndOfInput()) return -1;

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

   //  If the message could not be sent, report failure.
   //
   if(failed)
   {
      return -3;
   }

   return cli.Report(0, SuccessExpl);
}

//------------------------------------------------------------------------------
//
//  The SAVE command.
//
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
   word ProcessSubcommand(CliThread& cli, id_t index) const override;
};

fixed_string DebugTraceExpl = "include internal data structures? (default=f)";

fixed_string MscTextStr = "msc";
fixed_string MscTextExpl = "message sequence chart";

MscText::MscText() : CliText(MscTextExpl, MscTextStr)
{
   BindParm(*new OstreamMandParm);
   BindParm(*new CliBoolParm(DebugTraceExpl, true));
}

StSaveWhatParm::StSaveWhatParm()
{
   BindText(*new MscText, StSaveCommand::MscIndex);
}

StSaveCommand::StSaveCommand() : NtSaveCommand(false)
{
   BindParm(*new StSaveWhatParm);
}

word StSaveCommand::ProcessSubcommand(CliThread& cli, id_t index) const
{
   Debug::ft("StSaveCommand.ProcessSubcommand");

   if(index != MscIndex) return NtSaveCommand::ProcessSubcommand(cli, index);

   TraceRc rc;
   string title;
   auto debug = false;

   auto yield = cli.GenerateReportPreemptably();
   FunctionGuard guard(Guard_MakePreemptable, yield);

   if(!GetFileName(title, cli)) return -1;
   if(GetBoolParmRc(debug, cli) == Error) return -1;
   if(!cli.EndOfInput()) return -1;

   auto stream = cli.FileStream();
   if(stream == nullptr) return cli.Report(-7, CreateStreamFailure);

   auto buff = Singleton<TraceBuffer>::Instance();
   if(buff->Empty()) return ExplainTraceRc(cli, BufferEmpty);

   std::unique_ptr<MscBuilder> msc(new MscBuilder(debug));
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
//  The TESTS command.
//
class TestVerifyText : public CliText
{
public: TestVerifyText();
};

class StTestsAction : public TestsAction
{
public: StTestsAction();
};

class StTestsCommand : public TestsCommand
{
public:
   static const id_t TestVerifyIndex = LastNtIndex + 1;

   StTestsCommand();
private:
   word ProcessSubcommand(CliThread& cli, id_t index) const override;
};

fixed_string TestVerifyTextStr = "verify";
fixed_string TestVerifyTextExpl = "enables or disables the >verify command";

TestVerifyText::TestVerifyText() :
   CliText(TestVerifyTextExpl, TestVerifyTextStr)
{
   BindParm(*new SetHowParm);
}

StTestsAction::StTestsAction()
{
   BindText(*new TestVerifyText, StTestsCommand::TestVerifyIndex);
}

StTestsCommand::StTestsCommand() : TestsCommand(false)
{
   BindParm(*new StTestsAction);
}

word StTestsCommand::ProcessSubcommand(CliThread& cli, id_t index) const
{
   Debug::ft("StTestsCommand.ProcessSubcommand");

   switch(index)
   {
   case TestBeginIndex:
   case TestVerifyIndex:
      break;
   default:
      return TestsCommand::ProcessSubcommand(cli, index);
   }

   auto test = StTestData::Access(cli);
   if(test == nullptr) return cli.Report(-7, AllocationError);

   if(index == TestBeginIndex)
   {
      test->SetVerify(true);
      return TestsCommand::ProcessSubcommand(cli, index);
   }

   if(index == TestVerifyIndex)
   {
      id_t setHowIndex;
      auto flag = false;

      if(!GetTextIndex(setHowIndex, cli)) return -1;
      if(!cli.EndOfInput()) return -1;
      flag = (setHowIndex == SetHowParm::On);
      test->SetVerify(flag);
      return cli.Report(0, SuccessExpl);
   }

   return TestsCommand::ProcessSubcommand(cli, index);
}

//------------------------------------------------------------------------------
//
//  The VERIFY command.
//
const SkipInfo NilSkipInfo = { 0, 0 };

class VerifyCommand : public CliCommand
{
public:
   VerifyCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string VerifyStr = "verify";
fixed_string VerifyExpl =
   "Checks a message RECEIVED by a factory or one of its PSMs.";

VerifyCommand::VerifyCommand() : CliCommand(VerifyStr, VerifyExpl)
{
   Debug::ft("VerifyCommand.ctor");

   auto& facs = Singleton<FactoryRegistry>::Instance()->Factories();
   auto preg = Singleton<ProtocolRegistry>::Instance();

   //  Add the parameter that will contain the factories that support this
   //  command.
   //
   auto fparm = new CliTextParm(WhichFactoryExpl, false, Factory::MaxId + 1);
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
         ftext->BindParm(*new CliIntParm
            (TestSessionIdOptExpl, 0, TestSession::MaxId, true));
      }

      //  Find the factory's protocol and create a parameter that will
      //  contain each signal that the factory can receive.
      //
      auto prid = fac->GetProtocol();
      auto pro = preg->GetProtocol(prid);

      auto sparm = new CliTextParm(WhichSignalExpl, false, Signal::MaxId + 1);
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

word VerifyCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("VerifyCommand.ProcessCommand");

   auto ntest = NtTestData::Access(cli);
   if(ntest == nullptr) return cli.Report(-7, AllocationError);

   auto stest = StTestData::Access(cli);
   if(stest == nullptr) return cli.Report(-7, AllocationError);

   id_t fid, sid;
   word tid = 0;
   auto failed = false;

   //  Return if the command is currently disabled.
   //
   if(!stest->VerifyOn()) return 0;

   //  Find the factory associated with the expected message.
   //
   if(!GetTextIndex(fid, cli)) return -1;

   auto fac = Singleton<FactoryRegistry>::Instance()->GetFactory(fid);
   if(fac == nullptr) return cli.Report(-2, NoFactoryExpl);

   //  Find the expected message's protocol.
   //
   auto prid = fac->GetProtocol();
   auto pro = Singleton<ProtocolRegistry>::Instance()->GetProtocol(prid);
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
      if(sess == nullptr) return ntest->SetFailed(-7, AllocationError);
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

   if(!cli.EndOfInput()) return -1;
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

StIncrement::StIncrement() : CliIncrement(StIncrText, StIncrExpl)
{
   Debug::ft("StIncrement.ctor");

   BindCommand(*new StSaveCommand);
   BindCommand(*new StTestsCommand);
   BindCommand(*new StCorruptCommand);
}

//------------------------------------------------------------------------------

StIncrement::~StIncrement()
{
   Debug::ftnt("StIncrement.dtor");
}

//------------------------------------------------------------------------------

void StIncrement::Enter()
{
   Debug::ft("StIncrement.Enter");

   //  The binding of these commands is deferred until the increment
   //  is first entered because their parameters can only be created
   //  once all factories and protocols have registered during system
   //  initialization.
   //
   FunctionGuard guard(Guard_ImmUnprotect);

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
