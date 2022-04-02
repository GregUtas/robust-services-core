//==============================================================================
//
//  Circuit.cpp
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
#include "Circuit.h"
#include <cstdint>
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
Circuit::Circuit() : rxFrom_(Switch::SilentPort)
{
   Debug::ft("Circuit.ctor");

   Singleton< Switch >::Instance()->BindCircuit(*this);
}

//------------------------------------------------------------------------------

Circuit::~Circuit()
{
   Debug::ftnt("Circuit.dtor");

   Singleton< Switch >::Extant()->UnbindCircuit(*this);
}

//------------------------------------------------------------------------------

ptrdiff_t Circuit::CellDiff()
{
   uintptr_t local;
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

void Circuit::MakeConn(Switch::PortId rxFrom)
{
   Debug::ft("Circuit.MakeConn");

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
