//==============================================================================
//
//  Switch.cpp
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
#include "Switch.h"
#include <ostream>
#include "Circuit.h"
#include "Debug.h"
#include "Formatters.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace MediaBase
{
fn_name Switch_ctor = "Switch.ctor";

Switch::Switch()
{
   Debug::ft(Switch_ctor);

   circuits_.Init(MaxPortId, Circuit::CellDiff(), MemDynamic);
}

//------------------------------------------------------------------------------

fn_name Switch_dtor = "Switch.dtor";

Switch::~Switch()
{
   Debug::ftnt(Switch_dtor);

   Debug::SwLog(Switch_dtor, UnexpectedInvocation, 0);
}

//------------------------------------------------------------------------------

fn_name Switch_BindCircuit = "Switch.BindCircuit";

bool Switch::BindCircuit(Circuit& circuit)
{
   Debug::ft(Switch_BindCircuit);

   return circuits_.Insert(circuit);
}

//------------------------------------------------------------------------------

fn_name Switch_CircuitName = "Switch.CircuitName";

string Switch::CircuitName(PortId pid) const
{
   Debug::ft(Switch_CircuitName);

   auto cct = GetCircuit(pid);
   return (cct != nullptr ? cct->Name() : "Unequipped");
}

//------------------------------------------------------------------------------

void Switch::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Dynamic::Display(stream, prefix, options);

   stream << prefix << "circuits [Switch::PortId]" << CRLF;
   circuits_.Display(stream, prefix + spaces(2), options);
}

//------------------------------------------------------------------------------

Circuit* Switch::GetCircuit(PortId pid) const
{
   return circuits_.At(pid);
}

//------------------------------------------------------------------------------

fn_name Switch_UnbindCircuit = "Switch.UnbindCircuit";

void Switch::UnbindCircuit(Circuit& circuit)
{
   Debug::ft(Switch_UnbindCircuit);

   circuits_.Erase(circuit);
}
}
