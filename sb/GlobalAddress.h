//==============================================================================
//
//  GlobalAddress.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef GLOBALADDRESS_H_INCLUDED
#define GLOBALADDRESS_H_INCLUDED

#include "SysIpL3Addr.h"
#include "LocalAddress.h"
#include "NwTypes.h"
#include "SbTypes.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace SessionBase
{
//  Address for a SessionBase interprocessor (but intrasystem) message, which
//  includes an IP address and port in addition to a LocalAddress.
//
class GlobalAddress : public SysIpL3Addr
{
   friend class MsgPort;
public:
   //  Constructs the nil address.
   //
   GlobalAddress();

   //  The nil global address.
   //
   static const GlobalAddress NilAddr;

   //  Specifies an IP layer 3 address and factory.
   //
   GlobalAddress(const SysIpL3Addr& l3Addr, FactoryId fid);

   //  Specifies an IP layer 2 address, port, and factory.
   //
   GlobalAddress(const SysIpL2Addr& l2Addr, ipport_t port, FactoryId fid);

   //  Specifies an IP layer 3 address and pooled object.
   //
   GlobalAddress(const SysIpL3Addr& l3Addr, const LocalAddress& sbAddr);

   //  Not subclassed.
   //
   ~GlobalAddress();

   //  Returns the local address.
   //
   const LocalAddress& SbAddr() const { return sbAddr_; }

   //  Returns the factory identifier.
   //
   FactoryId Fid() const { return sbAddr_.fid; }

   //  Returns true if all fields in both addresses match.
   //
   bool operator==(const GlobalAddress& that) const;

   //  Returns the inverse of the == operator.
   //
   bool operator!=(const GlobalAddress& that) const;

   //  Overridden to display member variables.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;
private:
   //  Identifiers for the sending or receiving object and factory.
   //
   LocalAddress sbAddr_;
};
}
#endif
