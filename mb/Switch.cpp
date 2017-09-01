//==============================================================================
//
//  Switch.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "Switch.h"
#include <iosfwd>
#include <sstream>
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

   circuits_.Init(MaxPortId + 1, Circuit::CellDiff(), MemDyn);
}

//------------------------------------------------------------------------------

fn_name Switch_dtor = "Switch.dtor";

Switch::~Switch()
{
   Debug::ft(Switch_dtor);
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

   std::ostringstream name;
   auto cct = GetCircuit(pid);

   if(cct != nullptr)
      name << cct->Name();
   else
      name << "Unequipped";

   return name.str();
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
