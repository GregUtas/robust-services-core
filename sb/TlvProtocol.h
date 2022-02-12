//==============================================================================
//
//  TlvProtocol.h
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
   //  Overridden to display BUFF's contents.
   //
   void DisplayMsg(std::ostream& stream,
      const std::string& prefix, const SbIpBuffer& buff) const override;

   //  Overridden to return the signal in BUFF's message header.
   //
   SignalId ExtractSignal(const SbIpBuffer& buff) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
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
