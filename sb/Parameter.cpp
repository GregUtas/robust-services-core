//==============================================================================
//
//  Parameter.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "Parameter.h"
#include <bitset>
#include <ostream>
#include "Algorithms.h"
#include "Debug.h"
#include "Formatters.h"
#include "NbTypes.h"
#include "Protocol.h"
#include "ProtocolRegistry.h"
#include "SbCliParms.h"
#include "Singleton.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace SessionBase
{
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
      Debug::SwErr(Parameter_ctor, pack2(prid_, pid), 0);
      return;
   }

   pro->BindParameter(*this);
}

//------------------------------------------------------------------------------

fn_name Parameter_dtor = "Parameter.dtor";

Parameter::~Parameter()
{
   Debug::ft(Parameter_dtor);

   auto pro = Singleton< ProtocolRegistry >::Instance()->GetProtocol(prid_);

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
      Debug::SwErr(Parameter_BindUsage, Pid(), sid);
      return false;
   }

   usage_[sid] = usage;
   return true;
}

//------------------------------------------------------------------------------

ptrdiff_t Parameter::CellDiff()
{
   int local;
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
   Protected::Display(stream, prefix, options);

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

const char* Parameter::ExplainRc(TestRc rc)
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

   Debug::SwErr(Parameter_InjectMsg, prid_, Pid());
   return NotImplemented;
}

//------------------------------------------------------------------------------

void Parameter::Patch(sel_t selector, void* arguments)
{
   Protected::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name Parameter_VerifyMsg = "Parameter.VerifyMsg";

Parameter::TestRc Parameter::VerifyMsg
   (CliThread& cli, const Message& msg, Usage use) const
{
   Debug::ft(Parameter_VerifyMsg);

   Debug::SwErr(Parameter_VerifyMsg, prid_, Pid());
   return NotImplemented;
}
}
