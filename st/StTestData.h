//==============================================================================
//
//  StTestData.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef STTESTDATA_H_INCLUDED
#define STTESTDATA_H_INCLUDED

#include "CliAppData.h"
#include <memory>
#include "Factory.h"
#include "Message.h"
#include "SbTypes.h"
#include "TestSessions.h"

using namespace NodeBase;
using namespace SessionBase;

//------------------------------------------------------------------------------

namespace SessionTools
{
//  Data used by the CLI Testcase command and related commands.
//
class StTestData : public CliAppData
{
public:
   //  Returns the test data registered against CLI.  If the data
   //  does not exist, it is created.
   //
   static StTestData* Access(CliThread& cli);

   //  Returns the data associated with TID.  If the data does not
   //  exist, it is created.
   //
   TestSession* AccessSession(TestSessionId tid);

   //  Enables or disables the >verify command.
   //
   void SetVerify(bool on) { verify_ = on; }

   //  Returns true if the >verify command is enabled.
   //
   bool VerifyOn() const { return verify_; }

   //  Sends appMsg from the application PSM running in the test
   //  session identified by TID.
   //
   bool InjectMsg(Message& appMsg, TestSessionId tid);

   //  Finds the next message whose signal matches SID and that was
   //  received by the factory identified by FID or one of its PSMs.
   //
   Message* NextIcMsg(FactoryId fid, SignalId sid, SkipInfo& skip);

   //  Overridden to clean up at the end of a test.
   //
   virtual void EventOccurred(Event evt) override;

   //  Overridden to display member variables.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;
private:
   //  Private to restrict creation to the Access function.
   //
   explicit StTestData(CliThread& cli);

   //  Private to restrict deletion.  Not subclassed.
   //
   ~StTestData();

   //  Overridden to prohibit copying.
   //
   StTestData(const StTestData& that);
   void operator=(const StTestData& that);

   //  Whether the >verify command is currently enabled.
   //
   bool verify_;

   //  The most recent message that was verified during testing
   //  and that was received by a factory or one of its PSMs.
   //
   BuffTrace* lastMsg_[Factory::MaxId + 1];

   //  Data associated with active test sessions.
   //
   std::unique_ptr< TestSession > session_[TestSession::MaxId + 1];
};
}
#endif
