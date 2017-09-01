//==============================================================================
//
//  Signal.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "Signal.h"
#include <bitset>
#include <ostream>
#include <string>
#include "Algorithms.h"
#include "Debug.h"
#include "NbTypes.h"
#include "Protocol.h"
#include "ProtocolRegistry.h"
#include "Singleton.h"

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
      Debug::SwErr(Signal_ctor, prid_, sid);
      return;
   }

   pro->BindSignal(*this);
}

//------------------------------------------------------------------------------

fn_name Signal_dtor = "Signal.dtor";

Signal::~Signal()
{
   Debug::ft(Signal_dtor);

   auto pro = Singleton< ProtocolRegistry >::Instance()->GetProtocol(prid_);
   if(pro != nullptr) pro->UnbindSignal(*this);
}

//------------------------------------------------------------------------------

ptrdiff_t Signal::CellDiff()
{
   int local;
   auto fake = reinterpret_cast< const Signal* >(&local);
   return ptrdiff(&fake->sid_, fake);
}

//------------------------------------------------------------------------------

fn_name Signal_CreateText = "Signal.CreateText";

CliText* Signal::CreateText() const
{
   Debug::ft(Signal_CreateText);

   return nullptr;
}

//------------------------------------------------------------------------------

void Signal::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Protected::Display(stream, prefix, options);

   if(!options.test(DispVerbose)) return;

   stream << prefix << "prid : " << prid_ << CRLF;
   stream << prefix << "sid  : " << sid_.to_str() << CRLF;
}

//------------------------------------------------------------------------------

void Signal::Patch(sel_t selector, void* arguments)
{
   Protected::Patch(selector, arguments);
}
}
