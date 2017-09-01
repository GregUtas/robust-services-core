//==============================================================================
//
//  Signal.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef SIGNAL_H_INCLUDED
#define SIGNAL_H_INCLUDED

#include "Protected.h"
#include <cstddef>
#include "RegCell.h"
#include "SbTypes.h"
#include "SysTypes.h"

namespace NodeBase
{
   class CliText;
}

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace SessionBase
{
//  A signal is a message type defined by a protocol.  It governs the types
//  of parameters that may be present in its message. Each protocol defines
//  a singleton Signal subclass for each of its signals.
//
class Signal : public Protected
{
   friend class Registry< Signal >;
public:
   //  Allows "Id" to refer to a signal identifier in this class hierarchy.
   //
   typedef SignalId Id;

   //> Highest valid signal identifier.
   //
   static const Id MaxId = 63;

   //  Returns true if SID is a valid signal identifier.
   //
   static bool IsValidId(Id sid)
   {
      return ((sid != NIL_ID) && (sid <= MaxId));
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
   virtual CliText* CreateText() const;

   //  Returns the offset to sid_.
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
   //  Registers the signal with its protocol, which must already exist.
   //  Protected because this class is virtual.
   //
   Signal(ProtocolId prid, Id sid);

   //  Removes the signal from its protocol.  Protected because subclasses
   //  should be singletons.
   //
   virtual ~Signal();
private:
   //  Overridden to prohibit copying.
   //
   Signal(const Signal& that);
   void operator=(const Signal& that);

   //  The protocol to which the signal belongs.
   //
   ProtocolId prid_;

   //  The signal's identifier.
   //
   RegCell sid_;
};
}
#endif
