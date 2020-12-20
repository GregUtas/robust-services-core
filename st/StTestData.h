//==============================================================================
//
//  StTestData.h
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
//  Data used by the CLI >tests command and related commands.
//
class StTestData : public CliAppData
{
public:
   //  Deleted to prohibit copying.
   //
   StTestData(const StTestData& that) = delete;
   StTestData& operator=(const StTestData& that) = delete;

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
   void EventOccurred(Event event) override;

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;
private:
   //  Private to restrict creation to the Access function.
   //
   explicit StTestData(CliThread& cli);

   //  Private to restrict deletion.  Not subclassed.
   //
   ~StTestData();

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
