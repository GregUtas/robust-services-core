//==============================================================================
//
//  Switch.h
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
#ifndef SWITCH_H_INCLUDED
#define SWITCH_H_INCLUDED

#include "Dynamic.h"
#include <cstdint>
#include <string>
#include "NbTypes.h"
#include "Registry.h"
#include "SysTypes.h"

namespace MediaBase
{
   class Circuit;
}

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace MediaBase
{
//  The Switch singleton represents a timeswitch with subclasses of Circuit
//  registered against (connected to) its ports.  Any port on the switch can
//  listen to any other port.
//
class Switch : public Dynamic
{
   friend class Singleton< Switch >;
   friend class Circuit;
public:
   //  Allows PortId to be used within this class.
   //
   typedef int32_t PortId;

   //  A hard-coded port that transmits silence.
   //
   static const PortId SilentPort = 1;

   //  The maximum valid port number.
   //
   static const PortId MaxPortId = 100000;

   //  Returns true if PID is a valid port identifier.
   //
   static bool IsValidPort(PortId pid)
   {
      return ((pid != NIL_ID) && (pid < MaxPortId));
   }

   //  Returns a string identifying the circuit assigned to PID.
   //
   std::string CircuitName(PortId pid) const;

   //  Returns the circuit assigned to PID.
   //
   Circuit* GetCircuit(PortId pid) const;

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;
private:
   //  Private because this singleton is not subclassed.
   //
   Switch();

   //  Private because this singleton is not subclassed.
   //
   ~Switch();

   //  Adds CIRCUIT to the switch by assigning it to an available port.
   //
   bool BindCircuit(Circuit& circuit);

   //  Removes CIRCUIT from the switch.
   //
   void UnbindCircuit(Circuit& circuit);

   //  The registry of circuits, indexed by PortId.
   //
   Registry< Circuit > circuits_;
};
}
#endif
