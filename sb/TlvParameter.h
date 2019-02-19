//==============================================================================
//
//  TlvParameter.h
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
#ifndef TLVPARAMETER_H_INCLUDED
#define TLVPARAMETER_H_INCLUDED

#include "Parameter.h"
#include <cstddef>
#include <cstdint>
#include "MsgHeader.h"
#include "SbTypes.h"
#include "SysTypes.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace SessionBase
{
//  TLV (type-length-value) parameter header.
//
struct TlvParmHeader
{
   ParameterId pid : 16;  // parameter identifier
   uint16_t plen : 16;    // parameter length
};

//  The magic "4" must be sizeof(TlvParmHeader) or greater.
//
constexpr size_t MaxTlvParmSize = MaxSbMsgSize - sizeof(TlvParmHeader);

//------------------------------------------------------------------------------
//
//  TLV parameter layout.
//
struct TlvParmLayout
{
   TlvParmHeader header;              // parameter header
   byte_t bytes[MaxTlvParmSize - 1];  // parameter contents
};

typedef TlvParmLayout* TlvParmPtr;  // pointer to a parameter
typedef TlvParmPtr* TlvParmArray;   // array of pointers to parameters

//------------------------------------------------------------------------------
//
//  A TLV parameter is preceded by a header that contains its parameter
//  identifier and length.
//
class TlvParameter : public Parameter
{
public:
   //  Returns the parameter's identifier.
   //
   static Id ExtractPid(const TlvParmLayout& parm);

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;
protected:
   //  Passes the arguments to the base class constructor.  Protected because
   //  this class is virtual.
   //
   TlvParameter(ProtocolId prid, Id pid);

   //  Protected because subclasses should be singletons.
   //
   virtual ~TlvParameter();

   //  Looks for the parameter in MSG.  Returns the parameter's identifier if
   //  is present but illegal, or missing but mandatory.  Returns 0 otherwise.
   //  Parameters that support VerifyCommand must override this, but they will
   //  either invoke it or TlvMessage::VerifyParm first, before verifying each
   //  parameter field.
   //
   virtual TestRc VerifyMsg
      (CliThread& cli, const Message& msg, Usage use) const override;
};
}
#endif
