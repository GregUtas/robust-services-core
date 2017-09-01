//==============================================================================
//
//  TlvParameter.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef TLVPARAMETER_H_INCLUDED
#define TLVPARAMETER_H_INCLUDED

#include "Parameter.h"
#include "MsgHeader.h"
#include "NbTypes.h"
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
   MsgSize plen : 16;     // parameter length

   //  The magic "8" must be sizeof(TlvParmHeader) or greater.
   //
   static const MsgSize MaxParmSize = (MsgHeader::MaxMsgSize - 4);
};

//------------------------------------------------------------------------------
//
//  TLV parameter layout.
//
struct TlvParmLayout
{
   TlvParmHeader header;                          // parameter header
   byte_t bytes[TlvParmHeader::MaxParmSize - 1];  // parameter contents
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
