//==============================================================================
//
//  TlvProtocol.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef TLVPROTOCOL_H_INCLUDED
#define TLVPROTOCOL_H_INCLUDED

#include "Protocol.h"

//------------------------------------------------------------------------------

namespace SessionBase
{
//  A protocol whose parameters are subclasses of TlvParameter must subclass
//  from this rather than from Protocol.
//
class TlvProtocol : public Protocol
{
public:
   //  Overridden to return the signal in BUFF's message header.
   //
   virtual SignalId ExtractSignal(const SbIpBuffer& buff) const override;

   //  Overridden to display BUFF's contents.
   //
   virtual void DisplayMsg(std::ostream& stream,
      const std::string& prefix, const SbIpBuffer& buff) const override;

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;
protected:
   //  The arguments are for the base class constructor.  Protected because
   //  this class is virtual.
   //
   TlvProtocol(Id prid, Id base);

   //  Protected because subclasses should be singletons.
   //
   virtual ~TlvProtocol();
};
}
#endif
