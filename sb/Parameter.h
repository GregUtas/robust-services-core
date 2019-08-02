//==============================================================================
//
//  Parameter.h
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
#ifndef PARAMETER_H_INCLUDED
#define PARAMETER_H_INCLUDED

#include "Protected.h"
#include <cstddef>
#include <iosfwd>
#include <string>
#include "RegCell.h"
#include "SbTypes.h"
#include "Signal.h"
#include "SysTypes.h"

namespace NodeBase
{
   class CliParm;
   class CliThread;
}

//------------------------------------------------------------------------------

namespace SessionBase
{
//  Each protocol defines a singleton subclass for each of its parameters.
//  A parameter that uses the TLV format should subclass from TlvParameter.
//
class Parameter : public NodeBase::Protected
{
   friend class NodeBase::Registry< Parameter >;
public:
   //  Allows "Id" to refer to a parameter identifier in this class hierarchy.
   //
   typedef ParameterId Id;

   //> Highest valid parameter identifier.
   //
   static const Id MaxId;

   //  Identifier for timeout (timer expiry) parameter.
   //
   static const Id Timeout = 1;

   //  Applications start to number their parameters from here.
   //
   static const Id NextId = 2;

   //  Parameter usage in the context of a particular signal.
   //
   typedef int Usage;

   static const Usage Illegal   = 0;  // parameter illegal for signal
   static const Usage Optional  = 1;  // parameter optional for signal
   static const Usage Mandatory = 2;  // parameter mandatory for signal

   //  Returns the parameter's identifier.
   //
   Id Pid() const { return Id(pid_.GetId()); }

   //  Returns the protocol to which the parameter belongs.
   //
   ProtocolId Prid() const { return prid_; }

   //  Returns the parameter's usage within SID.
   //
   Usage GetUsage(SignalId sid) const;

   //  Displays a parameter within a message symbolically.  The parameter is
   //  located at bytes[0 to count-1].  The default version outputs a string
   //  stating that symbolic display is not supported.
   //
   virtual void DisplayMsg(std::ostream& stream, const std::string& prefix,
      const NodeBase::byte_t* bytes, size_t count) const;

   //  Creates a subclass of CliParm that allows the parameter's field(s) to
   //  be entered through the CLI.  USE indicates whether the parameter is
   //  mandatory or optional.  Invoked by CLI commands such as Inject and
   //  Verify.  The default version returns nullptr and must be overridden
   //  by parameters that support CLI commands.
   //
   virtual NodeBase::CliParm* CreateCliParm(Usage use) const;

   //  Return codes for InjectMsg and VerifyMsg.
   //
   enum TestRc
   {
      Ok,
      NotImplemented,
      MessageMissingMandatoryParm,
      MessageContainsIllegalParm,
      MessageFailedToAddParm,
      IllegalValueInStream,
      StreamMissingMandatoryParm,
      StreamContainsIllegalParm,
      OptionalParmMissing,
      OptionalParmPresent,
      ParmValueMismatch,
      TestRc_N
   };

   //  Invoked by InjectCommand.  It gets CLI parameters that specify
   //  whether the parameter should be added to MSG and, if so, the
   //  value of its field(s).  It adds the parameter if appropriate.
   //  USE indicates how MSG's signal uses the parameter.  Returns 0
   //  on success.  Any other result indicates a failure, such as a
   //  faulty parameter specification.  The default version generates
   //  a log and returns NotImplemented, and must be overridden by
   //  parameters that support InjectCommand.
   //
   virtual TestRc InjectMsg
      (NodeBase::CliThread& cli, Message& msg, Usage use) const;

   //  Invoked by VerifyCommand.  It gets CLI parameters that specify
   //  whether the parameter should be present in MSG and, if so, the
   //  value of its field(s).  It then searches MSG to confirm that
   //  the parameter is present (or absent) and that its fields are
   //  correct.  USE indicates how MSG's signal uses the parameter.
   //  Returns 0 if MSG passes.  Any other result means that MSG was
   //  incorrect.  The default version generates a log and returns
   //  NotImplemented, and must be overridden by parameters that
   //  support VerifyCommand.
   //
   virtual TestRc VerifyMsg
      (NodeBase::CliThread& cli, const Message& msg, Usage use) const;

   //  Returns a string that explains the above return codes.
   //
   static const char* ExplainRc(TestRc rc);

   //  Returns the offset to pid_.
   //
   static ptrdiff_t CellDiff();

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
protected:
   //  Sets the corresponding member variables.  Registers the parameter with
   //  its protocol, which must already exist.  Protected because this class
   //  is virtual.
   //
   Parameter(ProtocolId prid, Id pid);

   //  Removes the parameter from its protocol.  Protected because subclasses
   //  should be singletons.
   //
   virtual ~Parameter();

   //  Specifies that SID uses this parameter according to USAGE.  Invoked
   //  by a subclass constructor.
   //
   bool BindUsage(SignalId sid, Usage usage);
private:
   //  Deleted to prohibit copying.
   //
   Parameter(const Parameter& that) = delete;
   Parameter& operator=(const Parameter& that) = delete;

   //  The protocol to which the parameter belongs.
   //
   const ProtocolId prid_;

   //  The parameter's identifier.
   //
   NodeBase::RegCell pid_;

   //  The parameter's usage with respect to each signal.
   //
   Usage usage_[Signal::MaxId + 1];
};
}
#endif
