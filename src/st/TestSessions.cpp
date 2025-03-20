//==============================================================================
//
//  TestSessions.cpp
//
//  Copyright (C) 2013-2025  Greg Utas
//
//  This file is part of the Robust Services Core (RSC).
//
//  RSC is free software: you can redistribute it and/or modify it under the
//  terms of the Lesser GNU General Public License as published by the Free
//  Software Foundation, either version 3 of the License, or (at your option)
//  any later version.
//
//  RSC is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
//  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
//  details.
//
//  You should have received a copy of the Lesser GNU General Public License
//  along with RSC.  If not, see <http://www.gnu.org/licenses/>.
//
#include "TestSessions.h"
#include "Event.h"
#include "EventHandler.h"
#include "State.h"
#include <ostream>
#include <string>
#include "Context.h"
#include "Debug.h"
#include "FactoryRegistry.h"
#include "GlobalAddress.h"
#include "IpPortRegistry.h"
#include "LocalAddress.h"
#include "MsgHeader.h"
#include "MsgPort.h"
#include "NwTypes.h"
#include "Registry.h"
#include "SbAppIds.h"
#include "SbEvents.h"
#include "SbTrace.h"
#include "Singleton.h"
#include "StTestData.h"
#include "SysTypes.h"

using namespace NetworkBase;
using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace SessionTools
{
//  Signal for test sessions.
//
class TestInjectSignal : public TestSignal
{
   friend class Singleton<TestInjectSignal>;

   TestInjectSignal();
   ~TestInjectSignal() = default;
};

//------------------------------------------------------------------------------
//
//  States for test sessions.
//
class TestState : public State
{
public:
   static const Id FTS = ServiceSM::Null;

   static const Id Null   = FTS + 0;
   static const Id Active = FTS + 1;
protected:
   explicit TestState(Id stid);
   virtual ~TestState();
};

class TestNull : public TestState
{
   friend class Singleton<TestNull>;

   TestNull();
   ~TestNull() = default;
};

class TestActive : public TestState
{
   friend class Singleton<TestActive>;

   TestActive();
   ~TestActive() = default;
};

//------------------------------------------------------------------------------
//
//  Events for test sessions.
//
class TestEvent : public Event
{
public:
   static const Id Inject = NextId + 0;
   static const Id Verify = NextId + 1;
   virtual ~TestEvent();
protected:
   TestEvent(Id eid, ServiceSM& owner);
};

class TestInjectEvent : public TestEvent
{
public:
   explicit TestInjectEvent(ServiceSM& owner);
   ~TestInjectEvent();
};

class TestVerifyEvent : public TestEvent
{
public:
   explicit TestVerifyEvent(ServiceSM& owner);
   ~TestVerifyEvent();
};

//------------------------------------------------------------------------------
//
//  Event handlers for test sessions.
//
class TestEventHandler : public EventHandler
{
public:
   static const Id AnalyzeUserMessage    = NextId + 0;
   static const Id AnalyzeNetworkMessage = NextId + 1;
   static const Id NuInject              = NextId + 2;
   static const Id NuVerify              = NextId + 3;
   static const Id AcInject              = NextId + 4;
   static const Id AcVerify              = NextId + 5;
protected:
   TestEventHandler() = default;
   virtual ~TestEventHandler() = default;
};

class TestAnalyzeUserMessage : public TestEventHandler
{
   friend class Singleton<TestAnalyzeUserMessage>;

   TestAnalyzeUserMessage() = default;
   ~TestAnalyzeUserMessage() = default;
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
};

class TestAnalyzeNetworkMessage : public TestEventHandler
{
   friend class Singleton<TestAnalyzeNetworkMessage>;

   TestAnalyzeNetworkMessage() = default;
   ~TestAnalyzeNetworkMessage() = default;
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
};

class TestNuInject : public TestEventHandler
{
   friend class Singleton<TestNuInject>;

   TestNuInject() = default;
   ~TestNuInject() = default;
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
};

class TestNuVerify : public TestEventHandler
{
   friend class Singleton<TestNuVerify>;

   TestNuVerify() = default;
   ~TestNuVerify() = default;
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
};

class TestAcInject : public TestEventHandler
{
   friend class Singleton<TestAcInject>;

   TestAcInject() = default;
   ~TestAcInject() = default;
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
};

class TestAcVerify : public TestEventHandler
{
   friend class Singleton<TestAcVerify>;

   TestAcVerify() = default;
   ~TestAcVerify() = default;
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
};

//==============================================================================

TestSession::TestSession(const StTestData* data, TestSessionId tid) :
   sbData_(data),
   tid_(tid),
   testPsm_(nullptr),
   appFid_(NIL_ID),
   appBid_(NIL_ID),
   lastMsg_(nullptr)
{
   Debug::ft("TestSession.ctor");
}

//------------------------------------------------------------------------------

TestSession::~TestSession()
{
   Debug::ftnt("TestSession.dtor");

   if(testPsm_ != nullptr) testPsm_->Kill();
}

//------------------------------------------------------------------------------

void TestSession::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Dynamic::Display(stream, prefix, options);

   stream << prefix << "sbData  : " << sbData_ << CRLF;
   stream << prefix << "tid     : " << tid_ << CRLF;
   stream << prefix << "testPsm : " << testPsm_ << CRLF;
   stream << prefix << "appFid  : " << appFid_ << CRLF;
   stream << prefix << "appBid  : " << appBid_ << CRLF;
   stream << prefix << "lastMsg : " << lastMsg_ << CRLF;
}

//------------------------------------------------------------------------------

fn_name TestSession_NextIcMsg = "TestSession.NextIcMsg";

Message* TestSession::NextIcMsg(FactoryId fid, SignalId sid, SkipInfo& skip)
{
   Debug::ft(TestSession_NextIcMsg);

   //  When InjectCommand creates a test session, it includes the session
   //  identifier in the Inject message.  But when an application message
   //  creates a test session, it has yet to be assigned an identifier.
   //  Therefore, when verification (of the initial application message)
   //  is the first action performed on a session, appFid_, appBid_, and
   //  testPsm_ all have nil values, and so they must be initialized here.
   //
   if(appFid_ == NIL_ID)
      appFid_ = fid;
   else if(appFid_ != fid)
      return nullptr;

   lastMsg_ = BuffTrace::NextIcMsg(lastMsg_, appFid_, sid, skip);

   if(lastMsg_ == nullptr) return nullptr;

   if(appBid_ == NIL_ID)
      appBid_ = lastMsg_->Header()->rxAddr.bid;
   else if(lastMsg_->Header()->rxAddr.bid != appBid_)
      return nullptr;

   if(testPsm_ == nullptr)
   {
      auto psm = MsgPort::Find(lastMsg_->Header()->rxAddr);

      //  If the PSM wasn't found, it has probably idled.
      //  Continue to verify its messages.
      //
      if(psm != nullptr)
      {
         testPsm_ = TestPsm::Find(*psm);

         if(testPsm_ == nullptr)
            Debug::SwLog(TestSession_NextIcMsg, "PSM not found", appFid_);
         else
            testPsm_->SetCliId(*sbData_->Cli(), tid_);
      }
   }

   return lastMsg_->Rewrap();
}

//------------------------------------------------------------------------------

void TestSession::SetAppPsm(ProtocolSM* psm)
{
   Debug::ft("TestSession.SetAppPsm");

   //  The application PSM's factory and identifier are never cleared.  This
   //  allows its messages to be found in the trace buffer even after the PSM
   //  itself has been deleted.  If the session identifier is reused, the new
   //  application PSM overwrites the previous one, after which VerifyCommand
   //  applies to the new PSM's messages.
   //
   if(psm != nullptr)
   {
      const auto& addr = psm->EnsurePort()->ObjAddr();
      appFid_ = addr.fid;
      appBid_ = addr.bid;
   }
}

//------------------------------------------------------------------------------

void TestSession::SetTestPsm(TestPsm* psm)
{
   Debug::ft("TestSession.SetTestPsm");

   testPsm_ = psm;
}

//==============================================================================

TestFactory::TestFactory() :
   SsmFactory(TestFactoryId, TestProtocolId, "Test Sessions")
{
   Debug::ft("TestFactory.ctor");

   AddIncomingSignal(Signal::Timeout);
   AddIncomingSignal(TestSignal::Inject);
}

//------------------------------------------------------------------------------

TestFactory::~TestFactory()
{
   Debug::ftnt("TestFactory.dtor");
}

//------------------------------------------------------------------------------

ProtocolSM* TestFactory::AllocIcPsm
   (const Message& msg, ProtocolLayer& lower) const
{
   Debug::ft("TestFactory.AllocIcPsm");

   return new TestPsm(lower, false);
}

//------------------------------------------------------------------------------

RootServiceSM* TestFactory::AllocRoot(const Message& msg, ProtocolSM& psm) const
{
   Debug::ft("TestFactory.AllocRoot");

   return new TestSsm(psm);
}

//==============================================================================

TestProtocol::TestProtocol() : TlvProtocol(TestProtocolId, TimerProtocolId)
{
   Debug::ft("TestProtocol.ctor");

   //  Create the test signals.
   //
   Singleton<TestInjectSignal>::Instance();
}

//------------------------------------------------------------------------------

TestProtocol::~TestProtocol()
{
   Debug::ftnt("TestProtocol.dtor");
}

//==============================================================================

TestSignal::TestSignal(Id sid) : Signal(TestProtocolId, sid) { }

//------------------------------------------------------------------------------

TestInjectSignal::TestInjectSignal() : TestSignal(Inject) { }

//==============================================================================

TestMessage::TestMessage(ProtocolSM* dest) : Message(nullptr, 0),
   appMsg_(nullptr),
   cli_(nullptr),
   tid_(NIL_ID)
{
   Debug::ft("TestMessage.ctor");

   SetProtocol(TestProtocolId);

   const auto& self = IpPortRegistry::LocalAddr();
   GlobalAddress addr(self, NilIpPort, TestFactoryId);
   SetSender(addr);

   if(dest != nullptr)
   {
      addr = GlobalAddress(addr, dest->EnsurePort()->LocAddr().SbAddr());
   }

   SetReceiver(addr);
}

//------------------------------------------------------------------------------

TestMessage::~TestMessage()
{
   Debug::ftnt("TestMessage.dtor");

   delete appMsg_;
   appMsg_ = nullptr;
}

//------------------------------------------------------------------------------

void TestMessage::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Message::Display(stream, prefix, options);

   stream << prefix << "appMsg : " << appMsg_ << CRLF;
   stream << prefix << "cli    : " << cli_ << CRLF;
   stream << prefix << "tid    : " << tid_ << CRLF;
}

//------------------------------------------------------------------------------

Message* TestMessage::GetAppMsg()
{
   Debug::ft("TestMessage.GetAppMsg");

   auto amsg = appMsg_;
   appMsg_ = nullptr;
   return amsg;
}

//------------------------------------------------------------------------------

void TestMessage::GetSubtended(std::vector<Base*>& objects) const
{
   Debug::ft("TestMessage.GetSubtended");

   Message::GetSubtended(objects);

   if(appMsg_ != nullptr) appMsg_->GetSubtended(objects);
}

//------------------------------------------------------------------------------

void TestMessage::SetAppMsg(Message& msg)
{
   Debug::ft("TestMessage.SetAppMsg");

   appMsg_ = &msg;
}

//------------------------------------------------------------------------------

bool TestMessage::SetCliId(CliThread& cli, TestSessionId tid)
{
   Debug::ft("TestMessage.SetCliId");

   if(tid_ != NIL_ID) return false;

   cli_ = &cli;
   tid_ = tid;
   return true;
}

//------------------------------------------------------------------------------

void TestMessage::UpdateTestPsm() const
{
   Debug::ft("TestMessage.UpdateTestPsm");

   auto tpsm = static_cast<TestPsm*>(Psm());

   tpsm->SetCliId(*cli_, tid_);
}

//==============================================================================

TestPsm::TestPsm() : ProtocolSM(TestFactoryId),
   cli_(nullptr),
   tid_(NIL_ID)
{
   Debug::ft("TestPsm.ctor(first)");

   SetState(Active);

   auto tssm = static_cast<TestSsm*>(RootSsm());
   tssm->SetTestPsm(this);
}

//------------------------------------------------------------------------------

TestPsm::TestPsm(ProtocolLayer& adj, bool upper) :
   ProtocolSM(TestFactoryId, adj, upper),
   cli_(nullptr),
   tid_(NIL_ID)
{
   Debug::ft("TestPsm.ctor(subseq)");

   SetState(Active);
}

//------------------------------------------------------------------------------

TestPsm::~TestPsm()
{
   Debug::ftnt("TestPsm.dtor");

   SendFinalMsg();
}

//------------------------------------------------------------------------------

void TestPsm::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   ProtocolSM::Display(stream, prefix, options);

   stream << prefix << "cli : " << cli_ << CRLF;
   stream << prefix << "tid : " << tid_ << CRLF;
}

//------------------------------------------------------------------------------

TestPsm* TestPsm::Find(const MsgPort& port)
{
   Debug::ft("TestPsm.Find");

   auto ctx = port.GetContext();

   if(ctx == nullptr) return nullptr;

   for(auto p = ctx->FirstPsm(); p != nullptr; ctx->NextPsm(p))
   {
      if(p->GetFactory() == TestFactoryId) return static_cast<TestPsm*>(p);
   }

   return nullptr;
}

//------------------------------------------------------------------------------

ProtocolSM::IncomingRc TestPsm::ProcessIcMsg(Message& msg, Event*& event)
{
   Debug::ft("TestPsm.ProcessIcMsg");

   event = new AnalyzeMsgEvent(msg);
   return EventRaised;
}

//------------------------------------------------------------------------------

fn_name TestPsm_ProcessOgMsg = "TestPsm.ProcessOgMsg";

ProtocolSM::OutgoingRc TestPsm::ProcessOgMsg(Message& msg)
{
   Debug::ft(TestPsm_ProcessOgMsg);

   //  A test PSM does not send messages.
   //
   Debug::SwLog(TestPsm_ProcessOgMsg, "unexpected invocation", msg.GetSignal());
   return PurgeMessage;
}

//------------------------------------------------------------------------------

fn_name TestPsm_Route = "TestPsm.Route";

Message::Route TestPsm::Route() const
{
   Debug::ft(TestPsm_Route);

   //  A test PSM does not send messages.
   //
   Debug::SwLog(TestPsm_Route, UnexpectedInvocation, 0);
   return Message::Internal;
}

//------------------------------------------------------------------------------

void TestPsm::SendFinalMsg()
{
   Debug::ft("TestPsm.SendFinalMsg");

   //  Deregister the PSM from its session.
   //
   if(tid_ != NIL_ID)
   {
      auto test = StTestData::Access(*cli_);
      if(test == nullptr) return;
      auto sess = test->AccessSession(tid_);
      if(sess != nullptr) sess->SetTestPsm(nullptr);
      tid_ = NIL_ID;
   }
}

//------------------------------------------------------------------------------

void TestPsm::SetAppPsm(ProtocolSM* psm) const
{
   Debug::ft("TestPsm.SetAppPsm");

   if(tid_ != NIL_ID)
   {
      auto test = StTestData::Access(*cli_);
      auto sess = test->AccessSession(tid_);
      if(sess != nullptr) sess->SetAppPsm(psm);
   }
}

//------------------------------------------------------------------------------

bool TestPsm::SetCliId(CliThread& cli, TestSessionId tid)
{
   Debug::ft("TestPsm.SetCliId");

   //  If the PSM is already assigned to a session, do nothing.
   //
   if(tid_ != NIL_ID) return false;

   //  Register the PSM with its session.
   //
   cli_ = &cli;
   tid_ = tid;

   auto test = StTestData::Access(*cli_);
   auto sess = test->AccessSession(tid_);
   if(sess != nullptr) sess->SetTestPsm(this);

   return true;
}

//------------------------------------------------------------------------------

void TestPsm::SetIdle()
{
   Debug::ft("TestPsm.SetIdle");

   SetState(Idle);
}

//==============================================================================

fixed_string TestInjectEventStr = "TestInjectEvent";
fixed_string TestVerifyEventStr = "TestVerifyEvent";

TestService::TestService() : Service(TestServiceId)
{
   Debug::ft("TestService.ctor");

   Singleton<TestNull>::Instance();
   Singleton<TestActive>::Instance();

   BindHandler(*Singleton<TestAnalyzeUserMessage>::Instance(),
      TestEventHandler::AnalyzeUserMessage);
   BindHandler(*Singleton<TestAnalyzeNetworkMessage>::Instance(),
      TestEventHandler::AnalyzeNetworkMessage);
   BindHandler(*Singleton<TestNuInject>::Instance(),
      TestEventHandler::NuInject);
   BindHandler(*Singleton<TestNuVerify>::Instance(),
      TestEventHandler::NuVerify);
   BindHandler(*Singleton<TestAcInject>::Instance(),
      TestEventHandler::AcInject);
   BindHandler(*Singleton<TestAcVerify>::Instance(),
      TestEventHandler::AcVerify);

   BindEventName(TestInjectEventStr, TestEvent::Inject);
   BindEventName(TestVerifyEventStr, TestEvent::Verify);
}

//------------------------------------------------------------------------------

TestService::~TestService()
{
   Debug::ftnt("TestService.dtor");
}

//==============================================================================

TestState::TestState(Id stid) : State(TestServiceId, stid)
{
   Debug::ft("TestState.ctor");
}

//------------------------------------------------------------------------------

TestState::~TestState()
{
   Debug::ftnt("TestState.dtor");
}

//------------------------------------------------------------------------------

TestNull::TestNull() : TestState(Null)
{
   Debug::ft("TestNull.ctor");

   BindMsgAnalyzer
      (TestEventHandler::AnalyzeUserMessage, Service::UserPort);
   BindMsgAnalyzer
      (TestEventHandler::AnalyzeNetworkMessage, Service::NetworkPort);
   BindEventHandler
      (TestEventHandler::NuInject, TestEvent::Inject);
   BindEventHandler
      (TestEventHandler::NuVerify, TestEvent::Verify);
}

//------------------------------------------------------------------------------

TestActive::TestActive() : TestState(TestState::Active)
{
   Debug::ft("TestActive.ctor");

   BindMsgAnalyzer
      (TestEventHandler::AnalyzeUserMessage, Service::UserPort);
   BindMsgAnalyzer
      (TestEventHandler::AnalyzeNetworkMessage, Service::NetworkPort);
   BindEventHandler
      (TestEventHandler::AcInject, TestEvent::Inject);
   BindEventHandler
      (TestEventHandler::AcVerify, TestEvent::Verify);
}

//==============================================================================

TestEvent::TestEvent(Id eid, ServiceSM& owner) : Event(eid, &owner)
{
   Debug::ft("TestEvent.ctor");
}

//------------------------------------------------------------------------------

TestEvent::~TestEvent()
{
   Debug::ftnt("TestEvent.dtor");
}

//------------------------------------------------------------------------------

TestInjectEvent::TestInjectEvent(ServiceSM& owner) :
   TestEvent(Inject, owner)
{
   Debug::ft("TestInjectEvent.ctor");
}

//------------------------------------------------------------------------------

TestInjectEvent::~TestInjectEvent()
{
   Debug::ftnt("TestInjectEvent.dtor");
}

//------------------------------------------------------------------------------

TestVerifyEvent::TestVerifyEvent(ServiceSM& owner) :
   TestEvent(Verify, owner)
{
   Debug::ft("TestVerifyEvent.ctor");
}

//------------------------------------------------------------------------------

TestVerifyEvent::~TestVerifyEvent()
{
   Debug::ftnt("TestVerifyEvent.dtor");
}

//==============================================================================

TestSsm::TestSsm(ProtocolSM& psm) : RootServiceSM(TestServiceId),
   testPsm_(nullptr),
   appPsm_(nullptr)
{
   Debug::ft("TestSsm.ctor");

   if(psm.GetFactory() == TestFactoryId)
      SetTestPsm(static_cast<TestPsm*>(&psm));
   else
      SetAppPsm(&psm);
}

//------------------------------------------------------------------------------

TestSsm::~TestSsm()
{
   Debug::ftnt("TestSsm.dtor");
}

//------------------------------------------------------------------------------

ServicePortId TestSsm::CalcPort(const AnalyzeMsgEvent& ame)
{
   Debug::ft("TestSsm.CalcPort");

   auto psm = ame.Msg()->Psm();

   if(testPsm_ == psm) return Service::UserPort;
   if(appPsm_ == psm) return Service::NetworkPort;

   if(appPsm_ == nullptr)
   {
      appPsm_ = psm;
      return Service::NetworkPort;
   }

   return NIL_ID;
}

//------------------------------------------------------------------------------

void TestSsm::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   RootServiceSM::Display(stream, prefix, options);

   stream << prefix << "testPsm : " << testPsm_ << CRLF;
   stream << prefix << "appPsm  : " << appPsm_ << CRLF;
}

//------------------------------------------------------------------------------

void TestSsm::PsmDeleted(const ProtocolSM& exPsm)
{
   Debug::ft("TestSsm.PsmDeleted");

   if(testPsm_ == &exPsm) testPsm_ = nullptr;
   if(appPsm_ == &exPsm) appPsm_ = nullptr;
   SetNextState(TestState::Null);
   RootServiceSM::PsmDeleted(exPsm);
}

//------------------------------------------------------------------------------

void TestSsm::SetAppPsm(ProtocolSM* psm)
{
   Debug::ft("TestSsm.SetAppPsm");

   appPsm_ = psm;
   UpdateTestPsm();
}

//------------------------------------------------------------------------------

void TestSsm::SetNextState(StateId stid)
{
   Debug::ft("TestSsm.SetNextState");

   RootServiceSM::SetNextState(stid);

   if((stid == TestState::Null) && (testPsm_ != nullptr))
   {
      testPsm_->SetIdle();
   }
}

//------------------------------------------------------------------------------

void TestSsm::SetTestPsm(TestPsm* psm)
{
   Debug::ft("TestSsm.SetTestPsm");

   testPsm_ = psm;
   UpdateTestPsm();
}

//------------------------------------------------------------------------------

void TestSsm::UpdateTestPsm()
{
   Debug::ft("TestSsm.UpdateTestPsm");

   if(testPsm_ != nullptr) testPsm_->SetAppPsm(appPsm_);
}

//------------------------------------------------------------------------------

EventHandler::Rc TestAnalyzeUserMessage::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("TestAnalyzeUserMessage.ProcessEvent");

   auto& ame = static_cast<AnalyzeMsgEvent&>(currEvent);
   auto tmsg = static_cast<TestMessage*>(ame.Msg());
   auto sid = tmsg->GetSignal();

   if(sid == TestSignal::Inject)
   {
      nextEvent = new TestInjectEvent(ssm);
      return Continue;
   }

   Context::Kill("invalid signal", sid);
   return Suspend;
}

//------------------------------------------------------------------------------

EventHandler::Rc TestAnalyzeNetworkMessage::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("TestAnalyzeNetworkMessage.ProcessEvent");

   nextEvent = new TestVerifyEvent(ssm);
   return Continue;
}

//------------------------------------------------------------------------------

EventHandler::Rc TestNuInject::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("TestNuInject.ProcessEvent");

   //  Update the test PSM with the CLI thread and test session identifier.
   //
   auto tmsg = static_cast<TestMessage*>(Context::ContextMsg());

   tmsg->UpdateTestPsm();

   //  Create the application PSM.
   //
   auto amsg = tmsg->GetAppMsg();
   auto afid = amsg->Header()->txAddr.fid;
   auto afac = Singleton<FactoryRegistry>::Instance()->Factories().At(afid);
   auto apsm = static_cast<SsmFactory*>(afac)->AllocOgPsm(*amsg);

   if(apsm == nullptr)
   {
      delete amsg;
      return Suspend;
   }

   //  Save the application PSM and queue the application message on its PSM.
   //  The message was built outside of a PSM, so set its receiver and sender.
   //
   auto& tssm = static_cast<TestSsm&>(ssm);
   tssm.SetAppPsm(apsm);
   apsm->EnqOgMsg(*amsg);
   tssm.SetNextState(TestState::Active);
   return Suspend;
}

//------------------------------------------------------------------------------

EventHandler::Rc TestNuVerify::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("TestNuVerify.ProcessEvent");

   //  Create the UPSM and enter the Active state.
   //
   auto& tssm = static_cast<TestSsm&>(ssm);
   new TestPsm;
   tssm.SetNextState(TestState::Active);
   return Suspend;
}

//------------------------------------------------------------------------------

EventHandler::Rc TestAcInject::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("TestAcInject.ProcessEvent");

   //  Queue the application message on its PSM.
   //
   auto tmsg = static_cast<TestMessage*>(Context::ContextMsg());
   auto& tssm = static_cast<TestSsm&>(ssm);
   auto amsg = tmsg->GetAppMsg();
   auto apsm = tssm.GetAppPsm();

   apsm->EnqOgMsg(*amsg);
   return Suspend;
}

//------------------------------------------------------------------------------

EventHandler::Rc TestAcVerify::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("TestAcVerify.ProcessEvent");

   //  Enter the Null state when the NPSM enters the Idle state.
   //
   auto apsm = Context::ContextPsm();

   if(apsm->GetState() == ProtocolSM::Idle)
   {
      ssm.SetNextState(TestState::Null);
   }

   return Suspend;
}
}
