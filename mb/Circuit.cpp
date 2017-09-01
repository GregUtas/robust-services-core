//==============================================================================
//
//  Circuit.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "Circuit.h"
#include <ostream>
#include "Algorithms.h"
#include "Debug.h"
#include "Singleton.h"
#include "SysTypes.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace MediaBase
{
fn_name Circuit_ctor = "Circuit.ctor";

Circuit::Circuit() : rxFrom_(Switch::SilentPort)
{
   Debug::ft(Circuit_ctor);

   Singleton< Switch >::Instance()->BindCircuit(*this);
}

//------------------------------------------------------------------------------

fn_name Circuit_dtor = "Circuit.dtor";

Circuit::~Circuit()
{
   Debug::ft(Circuit_dtor);

   if(TsPort() != NIL_ID)
   {
      Singleton< Switch >::Instance()->UnbindCircuit(*this);
   }
}

//------------------------------------------------------------------------------

ptrdiff_t Circuit::CellDiff()
{
   int local;
   auto fake = reinterpret_cast< const Circuit* >(&local);
   return ptrdiff(&fake->port_, fake);
}

//------------------------------------------------------------------------------

void Circuit::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Dynamic::Display(stream, prefix, options);

   stream << prefix << "port   : " << port_.to_str() << CRLF;
   stream << prefix << "rxFrom : " << rxFrom_ << CRLF;
}

//------------------------------------------------------------------------------

fn_name Circuit_MakeConn = "Circuit.MakeConn";

void Circuit::MakeConn(Switch::PortId rxFrom)
{
   Debug::ft(Circuit_MakeConn);

   if(Switch::IsValidPort(rxFrom)) rxFrom_ = rxFrom;
}

//------------------------------------------------------------------------------

fn_name Circuit_Name = "Circuit.Name";

string Circuit::Name() const
{
   Debug::ft(Circuit_Name);

   //  This is a pure virtual function.
   //
   Debug::SwErr(Circuit_Name, 0, 0, ErrorLog);
   return "Nil circuit";
}
}
