//==============================================================================
//
//  MscAddress.h
//
//  Copyright (C) 2013-2021  Greg Utas
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
#ifndef MSCADDRESS_H_INCLUDED
#define MSCADDRESS_H_INCLUDED

#include "Temporary.h"
#include <cstddef>
#include "LocalAddress.h"
#include "Q1Link.h"
#include "SbTypes.h"

namespace SessionBase
{
   class MsgTrace;
}

namespace SessionTools
{
   class MscContext;
}

using namespace NodeBase;
using namespace SessionBase;

//------------------------------------------------------------------------------

namespace SessionTools
{
//  A message sequence chart assigns an MscAddress to each message sender or
//  receiver (factory, PSM, or SSM).  Each MscAddress belongs to an MscContext.
//  An MscContext for a PSM or factory has one MscAddress, whereas a context
//  for an SSM may have several.
//
class MscAddress : public Temporary
{
public:
   //  Creates an address for MT's local address.  CONTEXT is the
   //  context to which the address belongs.
   //
   MscAddress(const MsgTrace& mt, MscContext* context);

   //  Not subclassed.
   //
   ~MscAddress();

   //  Returns the local object associated with this address.
   //
   const LocalAddress& LocAddr() const { return locAddr_; }

   //  Returns the remote object associated with this address.
   //
   const LocalAddress& RemAddr() const { return remAddr_; }

   //  Returns the context to which this address belongs.
   //
   MscContext* Context() const { return context_; }

   //  Invoked when MT contains a local address that is already known.  The
   //  identity of its peer is updated if the current peer is a factory and
   //  MT's remote address is a specific PSM.  If the address was originally
   //  added as a remote address, its context may still be nullptr and can
   //  now be updated.
   //
   void SetPeer(const MsgTrace& mt, MscContext* context);

   //  Returns true if the address was involved in an external dialog, in
   //  which case FID is set to the factory involved in that dialog.
   //
   bool ExternalFid(FactoryId& fid) const;

   //  Returns the offset to link_.
   //
   static ptrdiff_t LinkDiff();

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;
private:
   //  The address captured by this record, which is
   //  o a PSM or factory in this processor
   //  o a factory in another processor (a PSM in another processor
   //    is only represented by its factory)
   //
   const LocalAddress locAddr_;

   //  If the local object is a PSM, the identity of its peer PSM
   //  if intraprocessor.  This address must be tracked so that an
   //  initial message from this PSM can be drawn to the peer rather
   //  than to the factory that created it.
   //
   LocalAddress remAddr_;

   //  The context to which the address belongs.
   //
   MscContext* context_;

   //  Set if this address was involved in an external dialog.
   //
   bool external_;

   //  If external_ is set, the FactoryId associated with the
   //  external dialog.
   //
   FactoryId extFid_;

   //  The next address in the message sequence chart.
   //
   Q1Link link_;
};
}
#endif
