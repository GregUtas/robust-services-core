//==============================================================================
//
//  Switch.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
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
public:
   //  Allows PortId to be used within this class.
   //
   typedef uint16_t PortId;

   //  A hard-coded port that transmits silence.
   //
   static const PortId SilentPort = 1;

   //  The maximum valid port number.
   //
   static const PortId MaxPortId  = 65000;

   //  Returns true if PID is a valid port identifier.
   //
   static bool IsValidPort(PortId pid)
   {
      return ((pid != NIL_ID) && (pid < MaxPortId));
   }

   //  Adds CIRCUIT to the switch by assigning it to an available port.
   //
   bool BindCircuit(Circuit& circuit);

   //  Removes CIRCUIT from the switch.
   //
   void UnbindCircuit(Circuit& circuit);

   //  Returns a string identifying the circuit assigned to PID.
   //
   std::string CircuitName(PortId pid) const;

   //  Returns the circuit assigned to PID.
   //
   Circuit* GetCircuit(PortId pid) const;

   //  Overridden to display member variables.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;
private:
   //  Private because this singleton is not subclassed.
   //
   Switch();

   //  Private because this singleton is not subclassed.
   //
   ~Switch();

   //  The registry of circuits, indexed by PortId.
   //
   Registry< Circuit > circuits_;
};
}
#endif
