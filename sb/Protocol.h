//==============================================================================
//
//  Protocol.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef PROTOCOL_H_INCLUDED
#define PROTOCOL_H_INCLUDED

#include "Protected.h"
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

using namespace NodeBase;

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
class Protocol : public Protected
{
   friend class Parameter;
   friend class Registry< Protocol >;
   friend class Signal;
public:
   //  Allows "Id" to refer to a protocol identifier in this class hierarchy.
   //
   typedef ProtocolId Id;

   //> Highest valid protocol identifier.
   //
   static const Id MaxId = UINT8_MAX;

   //  Returns the protocol's identifier.
   //
   Id Prid() const { return Id(prid_.GetId()); }

   //  Returns true if PRID1 understands PRID2 (that is, if PRID1 = PRID2
   //  or PRID2 is a base for PRID1).
   //
   static bool Understands(Id prid1, Id prid2);

   //  Returns the signal within BUFF.
   //
   virtual SignalId ExtractSignal(const SbIpBuffer& buff) const = 0;

   //  Returns the signal registered against SID.
   //
   Signal* GetSignal(SignalId sid) const;

   //  Returns the parameter registered against PID.
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

   //  Displays BUFF in text format.  The default version outputs
   //  a string stating that symbolic display is not supported and
   //  should be overridden by protocols that uses binary encoding.
   //
   virtual void DisplayMsg(std::ostream& stream,
      const std::string& prefix, const SbIpBuffer& buff) const;

   //  Returns the offset to prid_.
   //
   static ptrdiff_t CellDiff();

   //  Overridden to display member variables.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;
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
   //  Overridden to prohibit copying.
   //
   Protocol(const Protocol& that);
   void operator=(const Protocol& that);

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
   RegCell prid_;

   //  The identifier of the protocol's base class.
   //
   const Id base_;

   //  The signals registered with the protocol.
   //
   Registry< Signal > signals_;

   //  The parameters registered with the protocol.
   //
   Registry< Parameter > parameters_;
};
}
#endif
