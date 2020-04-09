//==============================================================================
//
//  Signal.h
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
#ifndef SIGNAL_H_INCLUDED
#define SIGNAL_H_INCLUDED

#include "Persistent.h"
#include <cstddef>
#include "RegCell.h"
#include "SbTypes.h"
#include "SysTypes.h"

namespace NodeBase
{
   class CliText;
}

//------------------------------------------------------------------------------

namespace SessionBase
{
//  A signal is a message type defined by a protocol.  It governs the types
//  of parameters that may be present in its message. Each protocol defines
//  a singleton Signal subclass for each of its signals.
//
class Signal : public NodeBase::Persistent
{
   friend class NodeBase::Registry< Signal >;
public:
   //  Allows "Id" to refer to a signal identifier in this class hierarchy.
   //
   typedef SignalId Id;

   //> Highest valid signal identifier.
   //
   static const Id MaxId = 63;

   //  Deleted to prohibit copying.
   //
   Signal(const Signal& that) = delete;
   Signal& operator=(const Signal& that) = delete;

   //  Returns true if SID is a valid signal identifier.
   //
   static bool IsValidId(Id sid)
   {
      return ((sid != NodeBase::NIL_ID) && (sid <= MaxId));
   }

   //  Identifier for timeout (timer expiry) signal.
   //
   static const Id Timeout = 1;

   //  Applications start to number their signals from here.
   //
   static const Id NextId = 2;

   //  Returns the signal's identifier.
   //
   Id Sid() const { return Id(sid_.GetId()); }

   //  Returns the protocol to which the signal belongs.
   //
   ProtocolId Prid() const { return prid_; }

   //  Creates a subclass of CliText that allows the signal to be specified
   //  using a string.  Invoked by InjectCommand and VerifyCommand.  The
   //  default version returns nullptr and must be overridden by signals
   //  that support these CLI commands.
   //
   virtual NodeBase::CliText* CreateText() const;

   //  Returns the offset to sid_.
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
   //  Registers the signal with its protocol, which must already exist.
   //  Protected because this class is virtual.
   //
   Signal(ProtocolId prid, Id sid);

   //  Removes the signal from its protocol.  Protected because subclasses
   //  should be singletons.
   //
   virtual ~Signal();
private:
   //  The protocol to which the signal belongs.
   //
   const ProtocolId prid_;

   //  The signal's identifier.
   //
   NodeBase::RegCell sid_;
};
}
#endif
