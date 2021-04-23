//==============================================================================
//
//  Circuit.h
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
#ifndef CIRCUIT_H_INCLUDED
#define CIRCUIT_H_INCLUDED

#include "Dynamic.h"
#include <cstddef>
#include <string>
#include "RegCell.h"
#include "SbTypes.h"
#include "Switch.h"

using namespace NodeBase;
using namespace SessionBase;

//------------------------------------------------------------------------------

namespace MediaBase
{
//  This is subclassed to define a type of circuit that can appear on a port
//  provided by Switch.  Each subclass instance represents an actual circuit
//  that can listen to one other circuit on the Switch at any given time.
//
class Circuit : public Dynamic
{
public:
   //  Virtual to allow subclassing.
   //
   virtual ~Circuit();

   //  Deleted to prohibit copying.
   //
   Circuit(const Circuit& that) = delete;

   //  Deleted to prohibit copy assignment.
   //
   Circuit& operator=(const Circuit& that) = delete;

   //  Returns the port to which the circuit is assigned.
   //
   Switch::PortId TsPort() const { return port_.GetId(); }

   //  Returns the port to which the circuit is listening.
   //
   Switch::PortId RxFrom() const { return rxFrom_; }

   //  Sets rxFrom as the port to which the circuit is listening.
   //
   void MakeConn(Switch::PortId rxFrom);

   //  Returns a string that identifies the circuit.
   //
   virtual std::string Name() const = 0;

   //  Returns true if the circuit supports PRID.
   //
   virtual bool Supports(ProtocolId prid) const { return false; }

   //  Returns the offset to port_.
   //
   static ptrdiff_t CellDiff();

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;
protected:
   //  Protected because this class is virtual.
   //
   Circuit();
private:
   //  The port where the circuit appears.
   //
   RegCell port_;

   //  The port to which the circuit is listening.
   //
   Switch::PortId rxFrom_;
};
}
#endif
