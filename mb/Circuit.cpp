//==============================================================================
//
//  Circuit.cpp
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

   Singleton< Switch >::Instance()->UnbindCircuit(*this);
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

   Debug::SwLog(Circuit_Name, strOver(this), 0);
   return "Unknown circuit";
}
}
