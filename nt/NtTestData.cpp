//==============================================================================
//
//  NtTestData.cpp
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
#include "NtTestData.h"
#include <iosfwd>
#include <sstream>
#include "CliThread.h"
#include "Debug.h"
#include "Formatters.h"
#include "NbCliParms.h"
#include "Singleton.h"
#include "TestDatabase.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeTools
{
fn_name NtTestData_ctor = "NtTestData.ctor";

NtTestData::NtTestData(CliThread& cli) : CliAppData(cli, TestcaseAppId),
   failed_(false),
   passCount_(0),
   failCount_(0)
{
   Debug::ft(NtTestData_ctor);
}

//------------------------------------------------------------------------------

fn_name NtTestData_dtor = "NtTestData.dtor";

NtTestData::~NtTestData()
{
   Debug::ft(NtTestData_dtor);
}

//------------------------------------------------------------------------------

fn_name NtTestData_Access = "NtTestData.Access";

NtTestData* NtTestData::Access(CliThread& cli)
{
   Debug::ft(NtTestData_Access);

   auto data = cli.GetAppData(TestcaseAppId);
   if(data == nullptr) data = new NtTestData(cli);
   return static_cast< NtTestData* >(data);
}

//------------------------------------------------------------------------------

fn_name NtTestData_Conclude = "NtTestData.Conclude";

void NtTestData::Conclude()
{
   Debug::ft(NtTestData_Conclude);

   if(name_.empty()) return;

   auto cli = Cli();
   *cli->obuf << spaces(2) << SuccessExpl << CRLF;

   auto tdb = Singleton< TestDatabase >::Instance();

   if(failed_)
   {
      if(!recover_.empty())
      {
         auto command = string("read ") + recover_.c_str();
         cli->Execute(command);
      }
      else
      {
         if(!epilog_.empty())
         {
            auto command = string("read ") + epilog_.c_str();
            cli->Execute(command);
         }
      }

      ++failCount_;
      tdb->SetState(name_.c_str(), TestDatabase::Failed);
   }
   else
   {
      if(!epilog_.empty())
      {
         auto command = string("read ") + epilog_.c_str();
         cli->Execute(command);
      }

      passCount_++;
      tdb->SetState(name_.c_str(), TestDatabase::Passed);
   }

   cli->Notify(CliAppData::EndOfTest);
   name_.clear();
}

//------------------------------------------------------------------------------

void NtTestData::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   CliAppData::Display(stream, prefix, options);

   stream << prefix << "prolog    : " << prolog_ << CRLF;
   stream << prefix << "epilog    : " << epilog_ << CRLF;
   stream << prefix << "recover   : " << recover_ << CRLF;
   stream << prefix << "name      : " << name_ << CRLF;
   stream << prefix << "failed    : " << failed_ << CRLF;
   stream << prefix << "passCount : " << passCount_ << CRLF;
   stream << prefix << "failCount : " << failCount_ << CRLF;
}

//------------------------------------------------------------------------------

fn_name NtTestData_Initiate = "NtTestData.Initiate";

word NtTestData::Initiate(const string& test)
{
   Debug::ft(NtTestData_Initiate);

   //  If a testcase is currently running, wrap it up before starting
   //  the new one.
   //
   Conclude();

   name_ = test.c_str();
   failed_ = false;

   auto cli = Cli();
   auto command = string("symbols set testcase.name ") + name_.c_str();
   cli->Execute(command);

   if(!prolog_.empty())
   {
      auto command = string("read ") + prolog_.c_str();
      cli->Execute(command);
   }

   return 0;
}

//------------------------------------------------------------------------------

fn_name NtTestData_Query = "NtTestData.Query";

void NtTestData::Query(bool verbose, string& expl) const
{
   Debug::ft(NtTestData_Query);

   std::ostringstream stream;
   stream << "Current test session:" << CRLF;
   stream << spaces(2) << "Passed: " << passCount_ << CRLF;
   stream << spaces(2) << "Failed: " << failCount_ << CRLF;
   stream << "Testcase database:" << CRLF;

   string info;
   Singleton< TestDatabase >::Instance()->Query(verbose, info);
   stream << info;
   expl = stream.str();
}

//------------------------------------------------------------------------------

fn_name NtTestData_Reset = "NtTestData.Reset";

void NtTestData::Reset()
{
   Debug::ft(NtTestData_Reset);

   Cli()->SetAppData(nullptr, TestcaseAppId);
}

//------------------------------------------------------------------------------

fn_name NtTestData_SetFailed = "NtTestData.SetFailed";

word NtTestData::SetFailed(word rc, const string& expl)
{
   Debug::ft(NtTestData_SetFailed);

   failed_ = true;

   std::ostringstream stream;

   stream << TestFailedExpl << " (rc=" << rc << ')';
   if(!expl.empty()) stream << ": " << expl;
   return Cli()->Report(rc, stream.str());
}
}
