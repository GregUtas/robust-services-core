//==============================================================================
//
//  Protocol.h
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
#ifndef PROTOCOL_H_INCLUDED
#define PROTOCOL_H_INCLUDED

#include "Immutable.h"
#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <string>
#include "RegCell.h"
#include "Registry.h"
#include "SbTypes.h"

namespace SessionBase
{
   class Parameter;
   class Signal;
}

//------------------------------------------------------------------------------

namespace SessionBase
{
//  Each protocol defines a singleton subclass for registering its signals
//  and parameters.  If a protocol's parameters use TLV (type-length-value)
//  format, it should subclass from TlvProtocol.
//
//  Protocol inheritance is supported using delegation to a base class rather
//  than by actual inheritance.  This avoids cloning each of the base class
//  signals and parameters into the subclass.  Each protocol should ultimately
//  inherit from TimerProtocol, which defines TimeoutSignal.
//
class Protocol : public NodeBase::Immutable
{
   friend class NodeBase::Registry<Protocol>;
   friend class Parameter;
   friend class Signal;
public:
   //  Allows "Id" to refer to a protocol identifier in this class hierarchy.
   //
   typedef ProtocolId Id;

   //  Deleted to prohibit copying.
   //
   Protocol(const Protocol& that) = delete;

   //  Deleted to prohibit copy assignment.
   //
   Protocol& operator=(const Protocol& that) = delete;

   //> Highest valid protocol identifier.
   //
   static const Id MaxId = UINT8_MAX;

   //  Returns the signal within BUFF.
   //
   virtual SignalId ExtractSignal(const SbIpBuffer& buff) const = 0;

   //  Displays BUFF in text format.  The default version outputs
   //  a string stating that symbolic display is not supported and
   //  should be overridden by protocols that uses binary encoding.
   //
   virtual void DisplayMsg(std::ostream& stream,
      const std::string& prefix, const SbIpBuffer& buff) const;

   //  Returns the signal registered against SID, whether in this
   //  protocol or its base protocol.
   //
   Signal* GetSignal(SignalId sid) const;

   //  Returns the parameter registered against PID, whether in this
   //  protocol or its base protocol.
   //
   Parameter* GetParameter(ParameterId pid) const;

   //  Returns the first signal in the protocol.
   //
   Signal* FirstSignal() const;

   //  Updates SIG to the next signal in the protocol, or nullptr if
   //  there is no next signal.
   //
   void NextSignal(Signal*& sig) const;

   //  Returns the first parameter the protocol.
   //
   Parameter* FirstParm() const;

   //  Updates PARM to the next parameter in the protocol, or nullptr
   //  if there is no next parameter.
   //
   void NextParm(Parameter*& parm) const;

   //  Returns the protocol's signals.
   //
   const NodeBase::Registry<Signal>& Signals() const { return signals_; }

   //  Returns the protocol's parameters.
   //
   const NodeBase::Registry<Parameter>& Parameters() const
      { return parameters_; }

   //  Returns true if PRID1 understands PRID2 (that is, if PRID1 = PRID2
   //  or PRID2 is a base for PRID1).
   //
   static bool Understands(Id prid1, Id prid2);

   //  Returns the protocol's identifier in the global ProtocolRegistry.
   //
   Id Prid() const { return prid_.GetId(); }

   //  Returns the identifier of the protocol's base protocol, if any
   //
   Id GetBase() const { return base_; }

   //  Returns the offset to prid_.
   //
   static ptrdiff_t CellDiff();

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;

   //  Overridden to display, based on INDEX, each signal or parameter.
   //
   void Summarize(std::ostream& stream, uint8_t index) const override;

   //  Constants for Summarize's INDEX parameter.
   //
   static const uint8_t SummarizeSignals = 1;
   static const uint8_t SummarizeParameters = 2;
protected:
   //  Sets the corresponding member variables and adds the protocol to
   //  ProtocolRegistry.  Protected because this class is virtual.
   //
   Protocol(Id prid, Id base);

   //  Deletes all signals and parameters before removing the protocol from
   //  ProtocolRegistry.  Protected because subclasses should be singletons.
   //
   virtual ~Protocol();
private:
   //  Adds SIGNAL to the protocol.  Invoked by Signal's base class
   //  constructor.
   //
   bool BindSignal(Signal& signal);

   //  Removes SIGNAL from the protocol.  Invoked by Signal's base class
   //  destructor.
   //
   void UnbindSignal(Signal& signal);

   //  Adds PARAMETER to the protocol.  Invoked by Parameter's base class
   //  class constructor.
   //
   bool BindParameter(Parameter& parameter);

   //  Removes PARAMETER from the protocol.  Invoked by Parameter's base
   //  class destructor.
   //
   void UnbindParameter(Parameter& parameter);

   //  The protocol's identifier.
   //
   NodeBase::RegCell prid_;

   //  The identifier of the protocol's base class.
   //
   const Id base_;

   //  The signals registered with the protocol.
   //
   NodeBase::Registry<Signal> signals_;

   //  The parameters registered with the protocol.
   //
   NodeBase::Registry<Parameter> parameters_;
};
}
#endif
