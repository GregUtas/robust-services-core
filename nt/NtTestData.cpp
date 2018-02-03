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
#include "NbCliParms.h"

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

fn_name NtTestData_SetFailed = "NtTestData.SetFailed";

word NtTestData::SetFailed(word rc, const string& expl)
{
   Debug::ft(NtTestData_SetFailed);

   failed_ = true;

   std::ostringstream stream;

   stream << TestFailedExpl << " (rc=" << rc << ')';
   if(expl.empty())
      stream << '.';
   else
      stream << ": " << expl;
   return Cli()->Report(rc, stream.str());
}
}
