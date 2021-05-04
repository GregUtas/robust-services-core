//==============================================================================
//
//  Signal.cpp
//
//  Copyright (C) 2013-2021  Greg Utas
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
#include "Signal.h"
#include <bitset>
#include <cstdint>
#include <ostream>
#include <string>
#include "Algorithms.h"
#include "Debug.h"
#include "NbTypes.h"
#include "Protocol.h"
#include "ProtocolRegistry.h"
#include "Singleton.h"

using namespace NodeBase;
using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace SessionBase
{
fn_name Signal_ctor = "Signal.ctor";

Signal::Signal(ProtocolId prid, Id sid) : prid_(prid)
{
   Debug::ft(Signal_ctor);

   //  Register the signal with its protocol.
   //
   sid_.SetId(sid);
   auto pro = Singleton< ProtocolRegistry >::Instance()->GetProtocol(prid_);

   if(pro == nullptr)
   {
      Debug::SwLog(Signal_ctor, "protocol not found", pack2(prid_, sid));
      return;
   }

   pro->BindSignal(*this);
}

//------------------------------------------------------------------------------

fn_name Signal_dtor = "Signal.dtor";

Signal::~Signal()
{
   Debug::ftnt(Signal_dtor);

   Debug::SwLog(Signal_dtor, UnexpectedInvocation, 0);

   auto pro = Singleton< ProtocolRegistry >::Extant()->GetProtocol(prid_);
   if(pro != nullptr) pro->UnbindSignal(*this);
}

//------------------------------------------------------------------------------

ptrdiff_t Signal::CellDiff()
{
   uintptr_t local;
   auto fake = reinterpret_cast< const Signal* >(&local);
   return ptrdiff(&fake->sid_, fake);
}

//------------------------------------------------------------------------------

CliText* Signal::CreateText() const
{
   Debug::ft("Signal.CreateText");

   return nullptr;
}

//------------------------------------------------------------------------------

void Signal::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Immutable::Display(stream, prefix, options);

   if(!options.test(DispVerbose)) return;

   stream << prefix << "prid : " << prid_ << CRLF;
   stream << prefix << "sid  : " << sid_.to_str() << CRLF;
}

//------------------------------------------------------------------------------

void Signal::Patch(sel_t selector, void* arguments)
{
   Immutable::Patch(selector, arguments);
}
}
