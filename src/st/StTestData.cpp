//==============================================================================
//
//  StTestData.cpp
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
#include "StTestData.h"
#include <cstddef>
#include <ostream>
#include <string>
#include "CliThread.h"
#include "Debug.h"
#include "Formatters.h"
#include "MsgHeader.h"
#include "SbTrace.h"
#include "SysTypes.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace SessionTools
{
StTestData::StTestData(CliThread& cli) : CliAppData(cli, TestSessionAppId),
   verify_(true)
{
   Debug::ft("StTestData.ctor");

   for(size_t i = 0; i <= Factory::MaxId; ++i) lastMsg_[i] = nullptr;
   for(size_t i = 0; i <= TestSession::MaxId; ++i) session_[i] = nullptr;
}

//------------------------------------------------------------------------------

StTestData::~StTestData()
{
   Debug::ftnt("StTestData.dtor");
}

//------------------------------------------------------------------------------

StTestData* StTestData::Access(CliThread& cli)
{
   Debug::ft("StTestData.Access");

   auto data = cli.GetAppData(TestSessionAppId);
   if(data == nullptr) data = new StTestData(cli);
   return static_cast<StTestData*>(data);
}

//------------------------------------------------------------------------------

fn_name StTestData_AccessSession = "StTestData.AccessSession";

TestSession* StTestData::AccessSession(TestSessionId tid)
{
   Debug::ft(StTestData_AccessSession);

   if((tid >= 1) && (tid <= TestSession::MaxId))
   {
      if(session_[tid] == nullptr)
      {
         session_[tid].reset(new TestSession(this, tid));
      }

      return session_[tid].get();
   }

   Debug::SwLog(StTestData_AccessSession, "invalid TestSessionId", tid);
   return nullptr;
}

//------------------------------------------------------------------------------

void StTestData::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   CliAppData::Display(stream, prefix, options);

   stream << prefix << "verify : " << verify_ << CRLF;
   stream << prefix << "lastMsg : " << CRLF;

   auto lead1 = prefix + spaces(2);
   auto lead2 = prefix + spaces(4);

   for(size_t i = 0; i <= Factory::MaxId; ++i)
   {
      if(lastMsg_[i] != nullptr)
      {
         stream << lead1 << strIndex(i) << lastMsg_[i] << CRLF;
      }
   }

   stream << prefix << "session : " << CRLF;

   for(size_t i = 0; i <= TestSession::MaxId; ++i)
   {
      if(session_[i] != nullptr)
      {
         stream << lead1 << strIndex(i) << CRLF;
         session_[i]->Display(stream, lead2, options);
      }
   }
}

//------------------------------------------------------------------------------

void StTestData::EventOccurred(Event event)
{
   Debug::ft("StTestData.EventOccurred");

   if(event == EndOfTest)
   {
      for(size_t i = 0; i <= Factory::MaxId; ++i) lastMsg_[i] = nullptr;

      for(size_t i = 0; i <= TestSession::MaxId; ++i)
      {
         session_[i].reset();
      }
   }
}

//------------------------------------------------------------------------------

bool StTestData::InjectMsg(Message& appMsg, TestSessionId tid)
{
   Debug::ft("StTestData.InjectMsg");

   auto sdata = AccessSession(tid);

   if(sdata != nullptr)
   {
      auto dest = sdata->GetTestPsm();
      auto msg = new TestMessage(dest);

      msg->SetSignal(TestSignal::Inject);
      msg->SetAppMsg(appMsg);
      msg->SetCliId(*Cli(), tid);

      if(dest != nullptr)
      {
         msg->SetPriority(PROGRESS);
      }
      else
      {
         msg->SetPriority(INGRESS);
         msg->Header()->initial = true;
      }

      return msg->Send(Message::Internal);
   }

   return false;
}

//------------------------------------------------------------------------------

Message* StTestData::NextIcMsg(FactoryId fid, SignalId sid, SkipInfo& skip)
{
   Debug::ft("StTestData.NextIcMsg");

   lastMsg_[fid] = BuffTrace::NextIcMsg(lastMsg_[fid], fid, sid, skip);

   if(lastMsg_[fid] != nullptr)
   {
      return lastMsg_[fid]->Rewrap();
   }

   return nullptr;
}
}
