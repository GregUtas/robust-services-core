//==============================================================================
//
//  TestSessions.h
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
#ifndef TESTSESSIONS_H_INCLUDED
#define TESTSESSIONS_H_INCLUDED

#include "Dynamic.h"
#include <memory>
#include "Message.h"
#include "NbTypes.h"
#include "Parameter.h"
#include "ProtocolSM.h"
#include "RootServiceSM.h"
#include "SbTypes.h"
#include "Service.h"
#include "Signal.h"
#include "SsmFactory.h"
#include "TlvProtocol.h"

namespace SessionTools
{
   class StTestData;
}

using namespace NodeBase;
using namespace SessionBase;

//------------------------------------------------------------------------------

namespace SessionTools
{
//  The test protocol supports the injection and verification of messages sent
//  and received by PSMs.
//
class TestProtocol : public TlvProtocol
{
   friend class Singleton< TestProtocol >;
private:
   //  Private because this singleton is not subclassed.
   //
   TestProtocol();

   //  Private because this singleton is not subclassed.
   //
   ~TestProtocol();
};

//------------------------------------------------------------------------------
//
//  Signals sent from the test environment on the CLI thread to a test PSM.
//
class TestSignal : public Signal
{
public:
   //  Identifiers for test signals.
   //
   static const Id Inject = NextId;
protected:
   //  Protected because this class is virtual.
   //
   explicit TestSignal(Id sid);

   //  Protected because subclasses should be singletons.
   //
   virtual ~TestSignal();
};

//------------------------------------------------------------------------------
//
//  Each test PSM runs in a context that also includes an application's PSM.
//  After InjectCommand builds a message, a message is sent to the test PSM.
//  This message causes the test PSM to retrieve the message to be injected,
//  after which it can be queued on the application PSM and sent at the end
//  of the transaction.
//
class TestPsm : public ProtocolSM
{
public:
   //  The test PSM enters the Active state upon creation and stays there
   //  until the application PSM idles.
   //
   static const StateId Active = Idle + 1;

   //  Creates a PSM that will send an initial message.
   //
   TestPsm();

   //  Creates a PSM from an adjacent layer.  The arguments are the same
   //  as those for the base class.
   //
   TestPsm(ProtocolLayer& adj, bool upper);

   //  Finds the test PSM that is running in PORT's context.
   //
   static TestPsm* Find(const MsgPort& port);

   //  Invoked by the SSM when the application PSM is set or cleared.
   //
   void SetAppPsm(ProtocolSM* psm) const;

   //  Invoked when Verify finds an initial message received by the application
   //  PSM running in this test PSM's context.  Returns false if the test PSM
   //  already has a session identifier, which means that the wrong message was
   //  selected as a candidate for verification.  Also invoked after an Inject
   //  message created the test PSM.
   //
   bool SetCliId(CliThread& cli, TestSessionId tid);

   //  Invoked by the test SSM when the session ends.  The test SSM and PSM do
   //  not understand the application PSM's protocol, so they idle by watching
   //  the application PSM.
   //
   void SetIdle();

   //  Overridden to display member variables.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;
private:
   //  Private to restrict deletion.  Not subclassed.
   //
   ~TestPsm();

   //  Overridden to handle an incoming message.
   //
   virtual IncomingRc ProcessIcMsg(Message& msg, Event*& event) override;

   //  Overridden to handle an outoing message.
   //
   virtual OutgoingRc ProcessOgMsg(Message& msg) override;

   //  Overridden to return the route for outgoing messages.
   //
   virtual Message::Route Route() const override;

   //  Overridden to send a final message if the PSM's context dies.
   //
   virtual void SendFinalMsg() override;

   //  The CLI thread that is using this PSM to run tests.
   //
   CliThread* cli_;

   //  The session's identifier in InjectCommand and VerifyCommand.
   //
   TestSessionId tid_;
};

//------------------------------------------------------------------------------
//
//  Message sent from the test environment on the CLI thread to a test PSM.
//
class TestMessage : public Message
{
public:
   //  Creates an outgoing message that the CLI thread will send to DEST.
   //
   explicit TestMessage(ProtocolSM* dest);

   //  Frees any application message owned by the message.  Not subclassed.
   //
   ~TestMessage();

   //  Sets the application message to be injected.
   //
   void SetAppMsg(Message& appMsg);

   //  Invoked to provide TestPsm with its CLI thread and session identifier
   //  when injecting an initial message.
   //
   bool SetCliId(CliThread& cli, TestSessionId tid);

   //  Copies cli_ and tid_ to the test PSM after the initial message arrives.
   //
   void UpdateTestPsm() const;

   //  Returns and *clears* appMsg_, the application message to be injected.
   //  The message must therefore be explicitly deleted if it is not queued
   //  on a PSM.
   //
   Message* GetAppMsg();

   //  Overridden to enumerate all objects that the message owns.
   //
   virtual void GetSubtended(Base* objects[], size_t& count) const override;

   //  Overridden to display member variables.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;
private:
   //  The message to be injected.
   //
   Message* appMsg_;

   //  The CLI thread that is using this PSM to run tests.
   //
   CliThread* cli_;

   //  The session's identifier in InjectCommand and VerifyCommand.
   //
   TestSessionId tid_;
};

//------------------------------------------------------------------------------
//
//  Service for test sessions.
//
class TestService : public Service
{
   friend class Singleton< TestService >;
private:
   //  Private because this singleton is not subclassed.
   //
   TestService();

   //  Private because this singleton is not subclassed.
   //
   ~TestService();
};

//------------------------------------------------------------------------------
//
//  SSM for test sessions.
//
class TestSsm : public RootServiceSM
{
public:
   //  Public to allow creation.  PSM was just created by an incoming message.
   //
   explicit TestSsm(ProtocolSM& psm);

   //  Returns the application PSM.
   //
   ProtocolSM* GetAppPsm() const { return appPsm_; }

   //  Sets the test PSM.
   //
   void SetTestPsm(TestPsm* psm);

   //  Sets the appplication PSM.
   //
   void SetAppPsm(ProtocolSM* psm);

   //  Passes appPsm_ to testPsm_.
   //
   void UpdateTestPsm();

   //  Overridden to set the test PSM idle when entering the Null state.
   //
   virtual void SetNextState(StateId stid) override;

   //  Overridden to display member variables.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;
private:
   //  Private to restrict deletion.  Not subclassed.
   //
   ~TestSsm();

   //  Overridden to return Service::NetworkPort if the message arrived on the
   //  application PSM, and Service::UserPort if it arrived on the test PSM.
   //
   virtual ServicePortId CalcPort(const AnalyzeMsgEvent& ame) override;

   //  Overridden to handle deletion of the test or application PSM.
   //
   virtual void PsmDeleted(ProtocolSM& exPsm) override;

   //  The test PSM.
   //
   TestPsm* testPsm_;

   //  The application PSM.
   //
   ProtocolSM* appPsm_;
};

//------------------------------------------------------------------------------
//
//  Factory for test sessions.
//
class TestFactory : public SsmFactory
{
   friend class Singleton< TestFactory >;
private:
   //  Private because this singleton is not subclassed.
   //
   TestFactory();

   //  Private because this singleton is not subclassed.
   //
   ~TestFactory();

   //  Overridden to allocate a test SSM when a message arrives to create a
   //  new test session.
   //
   virtual RootServiceSM* AllocRoot
      (const Message& msg, ProtocolSM& psm) const override;

   //  Overridden to create a test PSM.
   //
   virtual ProtocolSM* AllocIcPsm
      (const Message& msg, ProtocolLayer& lower) const override;
};

//------------------------------------------------------------------------------
//
//  Information about a test session.
//
class TestSession : public Dynamic
{
   friend std::unique_ptr< TestSession >::deleter_type;
   friend class StTestData;
public:
   //> The maximum number of test PSMs that can be simultaneously active.
   //
   static const TestSessionId MaxId = 16;

   //  Returns the test PSM associated with this entry.
   //
   TestPsm* GetTestPsm() const { return testPsm_; }

   //  Finds the next incoming message that matches SID.  Updates SKIP with
   //  the first and last signals (if any) that were skipped before matching
   //  SID.  FID must match appFid_.
   //
   Message* NextIcMsg(FactoryId fid, SignalId sid, SkipInfo& skip);

   //  Sets the PSM associated with ID.
   //
   void SetTestPsm(TestPsm* psm);

   //  Invoked by the test PSM when the application PSM is set or cleared.
   //
   void SetAppPsm(ProtocolSM* psm);

   //  Overridden to display member variables.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;
private:
   //  Private to restrict creation to StTestData.
   //
   TestSession(StTestData* data, TestSessionId tid);

   //  Private to restrict deletion.  Not subclassed.
   //
   ~TestSession();

   //  Overridden to prohibit copying.
   //
   TestSession(const TestSession& that);
   void operator=(const TestSession& that);

   //  The instance of StTestData to which this session belongs.
   //
   const StTestData* const sbData_;

   //  The session's identifier.
   //
   const TestSessionId tid_;

   //  The test PSM.
   //
   TestPsm* testPsm_;

   //  The application PSM's factory.
   //
   FactoryId appFid_;

   //  The application PSM's identifier.
   //
   PooledObjectId appBid_;

   //  The last incoming message that was verified.
   //
   BuffTrace* lastMsg_;
};
}
#endif
