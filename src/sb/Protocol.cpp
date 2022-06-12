//==============================================================================
//
//  Protocol.cpp
//
//  Copyright (C) 2013-2022  Greg Utas
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
#include "Protocol.h"
#include <iomanip>
#include <ostream>
#include "Algorithms.h"
#include "Debug.h"
#include "Formatters.h"
#include "Parameter.h"
#include "ProtocolRegistry.h"
#include "SbCliParms.h"
#include "Signal.h"
#include "Singleton.h"
#include "SysTypes.h"

using namespace NodeBase;
using std::ostream;
using std::setw;
using std::string;

//------------------------------------------------------------------------------

namespace SessionBase
{
Protocol::Protocol(Id prid, Id base) : base_(base)
{
   Debug::ft("Protocol.ctor");

   signals_.Init(Signal::MaxId, Signal::CellDiff(), MemImmutable);
   parameters_.Init(Parameter::MaxId, Parameter::CellDiff(), MemImmutable);

   prid_.SetId(prid);
   Singleton<ProtocolRegistry>::Instance()->BindProtocol(*this);
}

//------------------------------------------------------------------------------

fn_name Protocol_dtor = "Protocol.dtor";

Protocol::~Protocol()
{
   Debug::ftnt(Protocol_dtor);

   Debug::SwLog(Protocol_dtor, UnexpectedInvocation, 0);
   Singleton<ProtocolRegistry>::Extant()->UnbindProtocol(*this);
}

//------------------------------------------------------------------------------

bool Protocol::BindParameter(Parameter& parameter)
{
   Debug::ft("Protocol.BindParameter");

   return parameters_.Insert(parameter);
}

//------------------------------------------------------------------------------

bool Protocol::BindSignal(Signal& signal)
{
   Debug::ft("Protocol.BindSignal");

   return signals_.Insert(signal);
}

//------------------------------------------------------------------------------

ptrdiff_t Protocol::CellDiff()
{
   uintptr_t local;
   auto fake = reinterpret_cast<const Protocol*>(&local);
   return ptrdiff(&fake->prid_, fake);
}

//------------------------------------------------------------------------------

void Protocol::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Immutable::Display(stream, prefix, options);

   stream << prefix << "prid : " << prid_.to_str() << CRLF;
   stream << prefix << "base : " << base_ << CRLF;
   stream << prefix << "signals [SignalId]" << CRLF;
   signals_.Display(stream, prefix + spaces(2), options);
   stream << prefix << "parameters [ParameterId]" << CRLF;
   parameters_.Display(stream, prefix + spaces(2), options);
}

//------------------------------------------------------------------------------

void Protocol::DisplayMsg(ostream& stream,
   const string& prefix, const SbIpBuffer& buff) const
{
   stream << prefix << NoProtocolDisplay << CRLF;
}

//------------------------------------------------------------------------------

fn_name Protocol_ExtractSignal = "Protocol.ExtractSignal";

SignalId Protocol::ExtractSignal(const SbIpBuffer& buff) const
{
   Debug::ft(Protocol_ExtractSignal);

   Debug::SwLog(Protocol_ExtractSignal, strOver(this), 0);
   return NIL_ID;
}

//------------------------------------------------------------------------------

Parameter* Protocol::FirstParm() const
{
   for(auto pid = 1; pid <= Parameter::MaxId; ++pid)
   {
      auto parm = GetParameter(pid);
      if(parm != nullptr) return parm;
   }

   return nullptr;
}

//------------------------------------------------------------------------------

Signal* Protocol::FirstSignal() const
{
   for(auto sid = 1; sid <= Signal::MaxId; ++sid)
   {
      auto sig = GetSignal(sid);
      if(sig != nullptr) return sig;
   }

   return nullptr;
}

//------------------------------------------------------------------------------

Parameter* Protocol::GetParameter(ParameterId pid) const
{
   auto parm = parameters_.At(pid);
   if(parm != nullptr) return parm;
   if(base_ == NIL_ID) return nullptr;
   auto pro = Singleton<ProtocolRegistry>::Instance()->Protocols().At(base_);
   if(pro == nullptr) return nullptr;
   return pro->GetParameter(pid);
}

//------------------------------------------------------------------------------

Signal* Protocol::GetSignal(SignalId sid) const
{
   auto sig = signals_.At(sid);
   if(sig != nullptr) return sig;
   if(base_ == NIL_ID) return nullptr;
   auto pro = Singleton<ProtocolRegistry>::Instance()->Protocols().At(base_);
   if(pro == nullptr) return nullptr;
   return pro->GetSignal(sid);
}

//------------------------------------------------------------------------------

void Protocol::NextParm(Parameter*& parm) const
{
   if(parm == nullptr) return;

   for(auto pid = parm->Pid() + 1; pid <= Parameter::MaxId; ++pid)
   {
      parm = GetParameter(pid);
      if(parm != nullptr) return;
   }
}

//------------------------------------------------------------------------------

void Protocol::NextSignal(Signal*& sig) const
{
   if(sig == nullptr) return;

   for(auto sid = sig->Sid() + 1; sid <= Signal::MaxId; ++sid)
   {
      sig = GetSignal(sid);
      if(sig != nullptr) return;
   }
}

//------------------------------------------------------------------------------

void Protocol::Patch(sel_t selector, void* arguments)
{
   Immutable::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fixed_string ItemHeader = " Id  Name";
//                        |  3..<name>

size_t Protocol::Summarize(ostream& stream, uint32_t selector) const
{
   size_t count = 0;

   switch(selector)
   {
   case SummarizeSignals:
      stream << "Signals for " << strClass(this) << ':' << CRLF;
      stream << ItemHeader << CRLF;

      for(auto s = FirstSignal(); s != nullptr; NextSignal(s), ++count)
      {
         stream << setw(3) << s->Sid();
         stream << spaces(2) << strClass(s) << CRLF;
      }
      break;

   case SummarizeParameters:
      stream << "Parameters for " << strClass(this) << ':' << CRLF;
      stream << ItemHeader << CRLF;

      for(auto p = FirstParm(); p != nullptr; NextParm(p), ++count)
      {
         stream << setw(3) << p->Pid();
         stream << spaces(2) << strClass(p) << CRLF;
      }
      break;
   }

   return count;
}

//------------------------------------------------------------------------------

void Protocol::UnbindParameter(Parameter& parameter)
{
   Debug::ftnt("Protocol.UnbindParameter");

   parameters_.Erase(parameter);
}

//------------------------------------------------------------------------------

void Protocol::UnbindSignal(Signal& signal)
{
   Debug::ftnt("Protocol.UnbindSignal");

   signals_.Erase(signal);
}

//------------------------------------------------------------------------------

bool Protocol::Understands(Id prid1, Id prid2)
{
   Debug::ft("Protocol.Understands");

   if(prid1 == prid2) return true;

   auto reg = Singleton<ProtocolRegistry>::Instance();
   auto pro = reg->Protocols().At(prid1);

   while(pro != nullptr)
   {
      if(pro->base_ == prid2) return true;
      pro = reg->Protocols().At(pro->base_);
   }

   return false;
}
}
