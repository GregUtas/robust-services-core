//==============================================================================
//
//  Parameter.cpp
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
#include "Parameter.h"
#include <bitset>
#include <cstdint>
#include <ostream>
#include "Algorithms.h"
#include "Debug.h"
#include "Formatters.h"
#include "NbTypes.h"
#include "Protocol.h"
#include "ProtocolRegistry.h"
#include "SbCliParms.h"
#include "Singleton.h"

using namespace NodeBase;
using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace SessionBase
{
const Parameter::Id Parameter::MaxId = 63;

//------------------------------------------------------------------------------

fn_name Parameter_ctor = "Parameter.ctor";

Parameter::Parameter(ProtocolId prid, Id pid) : prid_(prid)
{
   Debug::ft(Parameter_ctor);

   pid_.SetId(pid);

   for(auto i = 0; i <= Signal::MaxId; ++i) usage_[i] = Illegal;

   //  Register the parameter with its protocol.
   //
   auto pro = Singleton< ProtocolRegistry >::Instance()->GetProtocol(prid_);

   if(pro == nullptr)
   {
      Debug::SwLog(Parameter_ctor, "protocol not found", pack2(prid_, pid));
      return;
   }

   pro->BindParameter(*this);
}

//------------------------------------------------------------------------------

fn_name Parameter_dtor = "Parameter.dtor";

Parameter::~Parameter()
{
   Debug::ftnt(Parameter_dtor);

   Debug::SwLog(Parameter_dtor, UnexpectedInvocation, 0);

   auto pro = Singleton< ProtocolRegistry >::Extant()->GetProtocol(prid_);
   if(pro == nullptr) return;
   pro->UnbindParameter(*this);
}

//------------------------------------------------------------------------------

fn_name Parameter_BindUsage = "Parameter.BindUsage";

bool Parameter::BindUsage(SignalId sid, Usage usage)
{
   Debug::ft(Parameter_BindUsage);

   if(!Signal::IsValidId(sid))
   {
      Debug::SwLog(Parameter_BindUsage, "invalid signal", Pid());
      return false;
   }

   usage_[sid] = usage;
   return true;
}

//------------------------------------------------------------------------------

ptrdiff_t Parameter::CellDiff()
{
   uintptr_t local;
   auto fake = reinterpret_cast< const Parameter* >(&local);
   return ptrdiff(&fake->pid_, fake);
}

//------------------------------------------------------------------------------

fn_name Parameter_CreateCliParm = "Parameter.CreateCliParm";

CliParm* Parameter::CreateCliParm(Usage use) const
{
   Debug::ft(Parameter_CreateCliParm);

   return nullptr;
}

//------------------------------------------------------------------------------

void Parameter::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Immutable::Display(stream, prefix, options);

   if(!options.test(DispVerbose)) return;

   stream << prefix << "prid : " << prid_ << CRLF;
   stream << prefix << "pid  : " << pid_.to_str() << CRLF;

   stream << prefix << "usage [SignalId]" << CRLF;

   auto lead = prefix + spaces(2);

   for(auto i = 0; i <= Signal::MaxId; ++i)
   {
      char c;

      if((i % 30) == 0) stream << lead;

      switch(usage_[i])
      {
      case Mandatory:
         c = 'M';
         break;
      case Optional:
         c = 'O';
         break;
      default:
         c = '-';
      }

      stream << c;

      if((i % 30) == 29) stream << CRLF;
   }

   stream << CRLF;
}

//------------------------------------------------------------------------------

void Parameter::DisplayMsg(ostream& stream,
   const string& prefix, const byte_t* bytes, size_t count) const
{
   stream << prefix << NoParameterDisplay << CRLF;
   strBytes(stream, prefix + spaces(2), bytes, count);
}

//------------------------------------------------------------------------------

fixed_string TestRcStrings[Parameter::TestRc_N + 1] =
{
   "OK",
   "Parameter not yet supported",
   "Mandatory parameter missing in message",
   "Illegal parameter present in message",
   "Message failed to add parameter",
   "Illegal value in stream",
   "Mandatory parameter missing in stream",
   "Illegal parameter present in stream",
   "Optional parameter missing when expected",
   "Optional parameter present when not expected",
   "Expected and actual values differ",
   ERROR_STR
};

c_string Parameter::ExplainRc(TestRc rc)
{
   if((rc >= 0) && (rc < TestRc_N)) return TestRcStrings[rc];
   return TestRcStrings[TestRc_N];
}

//------------------------------------------------------------------------------

Parameter::Usage Parameter::GetUsage(SignalId sid) const
{
   if(Signal::IsValidId(sid)) return usage_[sid];
   return Illegal;
}

//------------------------------------------------------------------------------

fn_name Parameter_InjectMsg = "Parameter.InjectMsg";

Parameter::TestRc Parameter::InjectMsg
   (CliThread& cli, Message& msg, Usage use) const
{
   Debug::ft(Parameter_InjectMsg);

   Debug::SwLog(Parameter_InjectMsg, strOver(this), pack2(prid_, Pid()));
   return NotImplemented;
}

//------------------------------------------------------------------------------

void Parameter::Patch(sel_t selector, void* arguments)
{
   Immutable::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name Parameter_VerifyMsg = "Parameter.VerifyMsg";

Parameter::TestRc Parameter::VerifyMsg
   (CliThread& cli, const Message& msg, Usage use) const
{
   Debug::ft(Parameter_VerifyMsg);

   Debug::SwLog(Parameter_VerifyMsg, strOver(this), pack2(prid_, Pid()));
   return NotImplemented;
}
}
