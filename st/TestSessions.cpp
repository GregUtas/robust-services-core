//==============================================================================
//
//  TestSessions.cpp
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
#include "SbAppIds.h"
#include "SbEvents.h"
#include "SbTrace.h"
#include "Singleton.h"
#include "StTestData.h"
#include "SysTypes.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace SessionTools
{
//  Signal for test sessions.
//
class TestInjectSignal : public TestSignal
{
   friend class Singleton< TestInjectSignal >;
private:
   TestInjectSignal();
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
   friend class Singleton< TestNull >;
private:
   TestNull();
};

class TestActive : public TestState
{
   friend class Singleton< TestActive >;
private:
   TestActive();
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
   TestEventHandler();
   virtual ~TestEventHandler();
};

class TestAnalyzeUserMessage : public TestEventHandler
{
   friend class Singleton< TestAnalyzeUserMessage >;
private:
   TestAnalyzeUserMessage() = default;
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
};

class TestAnalyzeNetworkMessage : public TestEventHandler
{
   friend class Singleton< TestAnalyzeNetworkMessage >;
private:
   TestAnalyzeNetworkMessage() = default;
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
};

class TestNuInject : public TestEventHandler
{
   friend class Singleton< TestNuInject >;
private:
   TestNuInject() = default;
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
};

class TestNuVerify : public TestEventHandler
{
   friend class Singleton< TestNuVerify >;
private:
   TestNuVerify() = default;
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
};

class TestAcInject : public TestEventHandler
{
   friend class Singleton< TestAcInject >;
private:
   TestAcInject() = default;
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
};

class TestAcVerify : public TestEventHandler
{
   friend class Singleton< TestAcVerify >;
private:
   TestAcVerify() = default;
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
};

//==============================================================================

fn_name TestSession_ctor = "TestSession.ctor";

TestSession::TestSession(const StTestData* data, TestSessionId tid) :
   sbData_(data),
   tid_(tid),
   testPsm_(nullptr),
   appFid_(NIL_ID),
   appBid_(NIL_ID),
   lastMsg_(nullptr)
{
   Debug::ft(TestSession_ctor);
}

//------------------------------------------------------------------------------

fn_name TestSession_dtor = "TestSession.dtor";

TestSession::~TestSession()
{
   Debug::ft(TestSession_dtor);

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

   while(true)
   {
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
               Debug::SwLog(TestSession_NextIcMsg, appFid_, 0);
            else
               testPsm_->SetCliId(*sbData_->Cli(), tid_);
         }
      }

      return lastMsg_->Rewrap();
   }

   return nullptr;
}

//------------------------------------------------------------------------------

fn_name TestSession_SetAppPsm = "TestSession.SetAppPsm";

void TestSession::SetAppPsm(ProtocolSM* psm)
{
   Debug::ft(TestSession_SetAppPsm);

   //  The application PSM's factory and identifier are never cleared.  This
   //  allows its messages to be found in the trace buffer even after the PSM
   //  itself has been deleted.  If the session identifier is reused, the new
   //  application PSM overwrites the previous one, after which VerifyCommand
   //  applies to the new PSM's messages.
   //
   if(psm != nullptr)
   {
      auto& addr = psm->EnsurePort()->ObjAddr();

      appFid_ = addr.fid;
      appBid_ = addr.bid;
   }
}

//------------------------------------------------------------------------------

fn_name TestSession_SetTestPsm = "TestSession.SetTestPsm";

void TestSession::SetTestPsm(TestPsm* psm)
{
   Debug::ft(TestSession_SetTestPsm);

   testPsm_ = psm;
}

//==============================================================================

fn_name TestFactory_ctor = "TestFactory.ctor";

TestFactory::TestFactory() :
   SsmFactory(TestFactoryId, TestProtocolId, "Test Sessions")
{
   Debug::ft(TestFactory_ctor);

   AddIncomingSignal(Signal::Timeout);
   AddIncomingSignal(TestSignal::Inject);
}

//------------------------------------------------------------------------------

fn_name TestFactory_dtor = "TestFactory.dtor";

TestFactory::~TestFactory()
{
   Debug::ft(TestFactory_dtor);
}

//------------------------------------------------------------------------------

fn_name TestFactory_AllocIcPsm = "TestFactory.AllocIcPsm";

ProtocolSM* TestFactory::AllocIcPsm
   (const Message& msg, ProtocolLayer& lower) const
{
   Debug::ft(TestFactory_AllocIcPsm);

   return new TestPsm(lower, false);
}

//------------------------------------------------------------------------------

fn_name TestFactory_AllocRoot = "TestFactory.AllocRoot";

RootServiceSM* TestFactory::AllocRoot
   (const Message& msg, ProtocolSM& psm) const
{
   Debug::ft(TestFactory_AllocRoot);

   return new TestSsm(psm);
}

//==============================================================================

fn_name TestProtocol_ctor = "TestProtocol.ctor";

TestProtocol::TestProtocol() : TlvProtocol(TestProtocolId, TimerProtocolId)
{
   Debug::ft(TestProtocol_ctor);

   //  Create the test signals.
   //
   Singleton< TestInjectSignal >::Instance();
}

//------------------------------------------------------------------------------

fn_name TestProtocol_dtor = "TestProtocol.dtor";

TestProtocol::~TestProtocol()
{
   Debug::ft(TestProtocol_dtor);
}

//==============================================================================

TestSignal::TestSignal(Id sid) : Signal(TestProtocolId, sid) { }

//------------------------------------------------------------------------------

TestSignal::~TestSignal() { }

//------------------------------------------------------------------------------

TestInjectSignal::TestInjectSignal() : TestSignal(Inject) { }

//==============================================================================

fn_name TestMessage_ctor = "TestMessage.ctor";

TestMessage::TestMessage(ProtocolSM* dest) : Message(nullptr, 0),
   appMsg_(nullptr),
   cli_(nullptr),
   tid_(NIL_ID)
{
   Debug::ft(TestMessage_ctor);

   SetProtocol(TestProtocolId);

   auto host = IpPortRegistry::HostAddress();
   GlobalAddress addr(host, NilIpPort, TestFactoryId);
   SetSender(addr);

   if(dest != nullptr)
   {
      addr = GlobalAddress(addr, dest->EnsurePort()->LocAddr().SbAddr());
   }

   SetReceiver(addr);
}

//------------------------------------------------------------------------------

fn_name TestMessage_dtor = "TestMessage.dtor";

TestMessage::~TestMessage()
{
   Debug::ft(TestMessage_dtor);

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

fn_name TestMessage_GetAppMsg = "TestMessage.GetAppMsg";

Message* TestMessage::GetAppMsg()
{
   Debug::ft(TestMessage_GetAppMsg);

   auto amsg = appMsg_;
   appMsg_ = nullptr;
   return amsg;
}

//------------------------------------------------------------------------------

fn_name TestMessage_GetSubtended = "TestMessage.GetSubtended";

void TestMessage::GetSubtended(Base* objects[], size_t& count) const
{
   Debug::ft(TestMessage_GetSubtended);

   Message::GetSubtended(objects, count);

   if(appMsg_ != nullptr) appMsg_->GetSubtended(objects, count);
}

//------------------------------------------------------------------------------

fn_name TestMessage_SetAppMsg = "TestMessage.SetAppMsg";

void TestMessage::SetAppMsg(Message& msg)
{
   Debug::ft(TestMessage_SetAppMsg);

   appMsg_ = &msg;
}

//------------------------------------------------------------------------------

fn_name TestMessage_SetCliId = "TestMessage.SetCliId";

bool TestMessage::SetCliId(CliThread& cli, TestSessionId tid)
{
   Debug::ft(TestMessage_SetCliId);

   if(tid_ != NIL_ID) return false;

   cli_ = &cli;
   tid_ = tid;
   return true;
}

//------------------------------------------------------------------------------

fn_name TestMessage_UpdateTestPsm = "TestMessage.UpdateTestPsm";

void TestMessage::UpdateTestPsm() const
{
   Debug::ft(TestMessage_UpdateTestPsm);

   auto tpsm = static_cast< TestPsm* >(Psm());

   tpsm->SetCliId(*cli_, tid_);
}

//==============================================================================

fn_name TestPsm_ctor1 = "TestPsm.ctor(first)";

TestPsm::TestPsm() : ProtocolSM(TestFactoryId),
   cli_(nullptr),
   tid_(NIL_ID)
{
   Debug::ft(TestPsm_ctor1);

   SetState(Active);

   auto tssm = static_cast< TestSsm* >(RootSsm());
   tssm->SetTestPsm(this);
}

//------------------------------------------------------------------------------

fn_name TestPsm_ctor2 = "TestPsm.ctor(subseq)";

TestPsm::TestPsm(ProtocolLayer& adj, bool upper) :
   ProtocolSM(TestFactoryId, adj, upper),
   cli_(nullptr),
   tid_(NIL_ID)
{
   Debug::ft(TestPsm_ctor2);

   SetState(Active);
}

//------------------------------------------------------------------------------

fn_name TestPsm_dtor = "TestPsm.dtor";

TestPsm::~TestPsm()
{
   Debug::ft(TestPsm_dtor);

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

fn_name TestPsm_Find = "TestPsm.Find";

TestPsm* TestPsm::Find(const MsgPort& port)
{
   Debug::ft(TestPsm_Find);

   auto ctx = port.GetContext();

   if(ctx == nullptr) return nullptr;

   for(auto p = ctx->FirstPsm(); p != nullptr; ctx->NextPsm(p))
   {
      if(p->GetFactory() == TestFactoryId) return static_cast< TestPsm* >(p);
   }

   return nullptr;
}

//------------------------------------------------------------------------------

fn_name TestPsm_ProcessIcMsg = "TestPsm.ProcessIcMsg";

ProtocolSM::IncomingRc TestPsm::ProcessIcMsg(Message& msg, Event*& event)
{
   Debug::ft(TestPsm_ProcessIcMsg);

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
   Debug::SwLog(TestPsm_ProcessOgMsg, msg.GetSignal(), 0);
   return PurgeMessage;
}

//------------------------------------------------------------------------------

fn_name TestPsm_Route = "TestPsm.Route";

Message::Route TestPsm::Route() const
{
   Debug::ft(TestPsm_Route);

   //  A test PSM does not send messages.
   //
   Debug::SwLog(TestPsm_Route, 0, 0);
   return Message::Internal;
}

//------------------------------------------------------------------------------

fn_name TestPsm_SendFinalMsg = "TestPsm.SendFinalMsg";

void TestPsm::SendFinalMsg()
{
   Debug::ft(TestPsm_SendFinalMsg);

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

fn_name TestPsm_SetAppPsm = "TestPsm.SetAppPsm";

void TestPsm::SetAppPsm(ProtocolSM* psm) const
{
   Debug::ft(TestPsm_SetAppPsm);

   if(tid_ != NIL_ID)
   {
      auto test = StTestData::Access(*cli_);
      auto sess = test->AccessSession(tid_);
      if(sess != nullptr) sess->SetAppPsm(psm);
   }
}

//------------------------------------------------------------------------------

fn_name TestPsm_SetCliId = "TestPsm.SetCliId";

bool TestPsm::SetCliId(CliThread& cli, TestSessionId tid)
{
   Debug::ft(TestPsm_SetCliId);

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

fn_name TestPsm_SetIdle = "TestPsm.SetIdle";

void TestPsm::SetIdle()
{
   Debug::ft(TestPsm_SetIdle);

   SetState(Idle);
}

//==============================================================================

fixed_string TestInjectEventStr = "TestInjectEvent";
fixed_string TestVerifyEventStr = "TestVerifyEvent";

fn_name TestService_ctor = "TestService.ctor";

TestService::TestService() : Service(TestServiceId)
{
   Debug::ft(TestService_ctor);

   Singleton< TestNull >::Instance();
   Singleton< TestActive >::Instance();

   BindHandler(*Singleton< TestAnalyzeUserMessage >::Instance(),
      TestEventHandler::AnalyzeUserMessage);
   BindHandler(*Singleton< TestAnalyzeNetworkMessage >::Instance(),
      TestEventHandler::AnalyzeNetworkMessage);
   BindHandler(*Singleton< TestNuInject >::Instance(),
      TestEventHandler::NuInject);
   BindHandler(*Singleton< TestNuVerify >::Instance(),
      TestEventHandler::NuVerify);
   BindHandler(*Singleton< TestAcInject >::Instance(),
      TestEventHandler::AcInject);
   BindHandler(*Singleton< TestAcVerify >::Instance(),
      TestEventHandler::AcVerify);

   BindEventName(TestInjectEventStr, TestEvent::Inject);
   BindEventName(TestVerifyEventStr, TestEvent::Verify);
}

//------------------------------------------------------------------------------

fn_name TestService_dtor = "TestService.dtor";

TestService::~TestService()
{
   Debug::ft(TestService_dtor);
}

//==============================================================================

fn_name TestState_ctor = "TestState.ctor";

TestState::TestState(Id stid) : State(TestServiceId, stid)
{
   Debug::ft(TestState_ctor);
}

//------------------------------------------------------------------------------

fn_name TestState_dtor = "TestState.dtor";

TestState::~TestState()
{
   Debug::ft(TestState_dtor);
}

//------------------------------------------------------------------------------

fn_name TestNull_ctor = "TestNull.ctor";

TestNull::TestNull() : TestState(TestNull::Null)
{
   Debug::ft(TestNull_ctor);

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

fn_name TestActive_ctor = "TestActive.ctor";

TestActive::TestActive() : TestState(TestState::Active)
{
   Debug::ft(TestActive_ctor);

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

fn_name TestEvent_ctor = "TestEvent.ctor";

TestEvent::TestEvent(Id eid, ServiceSM& owner) : Event(eid, &owner)
{
   Debug::ft(TestEvent_ctor);
}

//------------------------------------------------------------------------------

fn_name TestEvent_dtor = "TestEvent.dtor";

TestEvent::~TestEvent()
{
   Debug::ft(TestEvent_dtor);
}

//------------------------------------------------------------------------------

fn_name TestInjectEvent_ctor = "TestInjectEvent.ctor";

TestInjectEvent::TestInjectEvent(ServiceSM& owner) :
   TestEvent(Inject, owner)
{
   Debug::ft(TestInjectEvent_ctor);
}

//------------------------------------------------------------------------------

fn_name TestInjectEvent_dtor = "TestInjectEvent.dtor";

TestInjectEvent::~TestInjectEvent()
{
   Debug::ft(TestInjectEvent_dtor);
}

//------------------------------------------------------------------------------

fn_name TestVerifyEvent_ctor = "TestVerifyEvent.ctor";

TestVerifyEvent::TestVerifyEvent(ServiceSM& owner) :
   TestEvent(Verify, owner)
{
   Debug::ft(TestVerifyEvent_ctor);
}

//------------------------------------------------------------------------------

fn_name TestVerifyEvent_dtor = "TestVerifyEvent.dtor";

TestVerifyEvent::~TestVerifyEvent()
{
   Debug::ft(TestVerifyEvent_dtor);
}

//==============================================================================

fn_name TestSsm_ctor = "TestSsm.ctor";

TestSsm::TestSsm(ProtocolSM& psm) : RootServiceSM(TestServiceId),
   testPsm_(nullptr),
   appPsm_(nullptr)
{
   Debug::ft(TestSsm_ctor);

   if(psm.GetFactory() == TestFactoryId)
      SetTestPsm(static_cast< TestPsm* >(&psm));
   else
      SetAppPsm(&psm);
}

//------------------------------------------------------------------------------

fn_name TestSsm_dtor = "TestSsm.dtor";

TestSsm::~TestSsm()
{
   Debug::ft(TestSsm_dtor);
}

//------------------------------------------------------------------------------

fn_name TestSsm_CalcPort = "TestSsm.CalcPort";

ServicePortId TestSsm::CalcPort(const AnalyzeMsgEvent& ame)
{
   Debug::ft(TestSsm_CalcPort);

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

fn_name TestSsm_PsmDeleted = "TestSsm.PsmDeleted";

void TestSsm::PsmDeleted(ProtocolSM& exPsm)
{
   Debug::ft(TestSsm_PsmDeleted);

   if(testPsm_ == &exPsm) testPsm_ = nullptr;
   if(appPsm_ == &exPsm) appPsm_ = nullptr;
   SetNextState(TestState::Null);
   RootServiceSM::PsmDeleted(exPsm);
}

//------------------------------------------------------------------------------

fn_name TestSsm_SetAppPsm = "TestSsm.SetAppPsm";

void TestSsm::SetAppPsm(ProtocolSM* psm)
{
   Debug::ft(TestSsm_SetAppPsm);

   appPsm_ = psm;
   UpdateTestPsm();
}

//------------------------------------------------------------------------------

fn_name TestSsm_SetNextState = "TestSsm.SetNextState";

void TestSsm::SetNextState(StateId stid)
{
   Debug::ft(TestSsm_SetNextState);

   RootServiceSM::SetNextState(stid);

   if((stid == TestState::Null) && (testPsm_ != nullptr))
   {
      testPsm_->SetIdle();
   }
}

//------------------------------------------------------------------------------

fn_name TestSsm_SetTestPsm = "TestSsm.SetTestPsm";

void TestSsm::SetTestPsm(TestPsm* psm)
{
   Debug::ft(TestSsm_SetTestPsm);

   testPsm_ = psm;
   UpdateTestPsm();
}

//------------------------------------------------------------------------------

fn_name TestSsm_UpdateTestPsm = "TestSsm.UpdateTestPsm";

void TestSsm::UpdateTestPsm()
{
   Debug::ft(TestSsm_UpdateTestPsm);

   if(testPsm_ != nullptr) testPsm_->SetAppPsm(appPsm_);
}

//==============================================================================

TestEventHandler::TestEventHandler() { }

TestEventHandler::~TestEventHandler() { }

//------------------------------------------------------------------------------

fn_name TestAnalyzeUserMessage_ProcessEvent =
   "TestAnalyzeUserMessage.ProcessEvent";

EventHandler::Rc TestAnalyzeUserMessage::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(TestAnalyzeUserMessage_ProcessEvent);

   auto& ame = static_cast< AnalyzeMsgEvent& >(currEvent);
   auto tmsg = static_cast< TestMessage* >(ame.Msg());
   auto sid = tmsg->GetSignal();

   if(sid == TestSignal::Inject)
   {
      nextEvent = new TestInjectEvent(ssm);
      return Continue;
   }

   Context::Kill(TestAnalyzeUserMessage_ProcessEvent, sid, 0);
   return Suspend;
}

//------------------------------------------------------------------------------

fn_name TestAnalyzeNetworkMessage_ProcessEvent =
   "TestAnalyzeNetworkMessage.ProcessEvent";

EventHandler::Rc TestAnalyzeNetworkMessage::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(TestAnalyzeNetworkMessage_ProcessEvent);

   nextEvent = new TestVerifyEvent(ssm);
   return Continue;
}

//------------------------------------------------------------------------------

fn_name TestNuInject_ProcessEvent = "TestNuInject.ProcessEvent";

EventHandler::Rc TestNuInject::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(TestNuInject_ProcessEvent);

   //  Update the test PSM with the CLI thread and test session identifier.
   //
   auto tmsg = static_cast< TestMessage* >(Context::ContextMsg());

   tmsg->UpdateTestPsm();

   //  Create the application PSM.
   //
   auto amsg = tmsg->GetAppMsg();
   auto afid = amsg->Header()->txAddr.fid;
   auto afac = Singleton< FactoryRegistry >::Instance()->GetFactory(afid);
   auto apsm = static_cast< SsmFactory* >(afac)->AllocOgPsm(*amsg);

   if(apsm == nullptr)
   {
      delete amsg;
      return Suspend;
   }

   //  Save the application PSM and queue the application message on its PSM.
   //  The message was built outside of a PSM, so set its receiver and sender.
   //
   auto& tssm = static_cast< TestSsm& >(ssm);
   tssm.SetAppPsm(apsm);
   apsm->EnqOgMsg(*amsg);
   tssm.SetNextState(TestState::Active);
   return Suspend;
}

//------------------------------------------------------------------------------

fn_name TestNuVerify_ProcessEvent = "TestNuVerify.ProcessEvent";

EventHandler::Rc TestNuVerify::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(TestNuVerify_ProcessEvent);

   //  Create the UPSM and enter the Active state.
   //
   auto& tssm = static_cast< TestSsm& >(ssm);
   auto tpsm = new TestPsm;

   if(tpsm == nullptr) return Suspend;
   tssm.SetNextState(TestState::Active);
   return Suspend;
}

//------------------------------------------------------------------------------

fn_name TestAcInject_ProcessEvent = "TestAcInject.ProcessEvent";

EventHandler::Rc TestAcInject::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(TestAcInject_ProcessEvent);

   //  Queue the application message on its PSM.
   //
   auto tmsg = static_cast< TestMessage* >(Context::ContextMsg());
   auto& tssm = static_cast< TestSsm& >(ssm);
   auto amsg = tmsg->GetAppMsg();
   auto apsm = tssm.GetAppPsm();

   apsm->EnqOgMsg(*amsg);
   return Suspend;
}

//------------------------------------------------------------------------------

fn_name TestAcVerify_ProcessEvent = "TestAcVerify.ProcessEvent";

EventHandler::Rc TestAcVerify::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(TestAcVerify_ProcessEvent);

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
