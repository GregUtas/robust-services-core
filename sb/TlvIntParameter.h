//==============================================================================
//
//  TlvIntParameter.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef TLVINTPARAMETER_H_INCLUDED
#define TLVINTPARAMETER_H_INCLUDED

#include "TlvParameter.h"
#include <sstream>
#include "CliCommand.h"
#include "CliThread.h"
#include "Debug.h"
#include "SbCliParms.h"
#include "SbTypes.h"
#include "SysDecls.h"
#include "SysTypes.h"
#include "TlvMessage.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace SessionBase
{
//  Virtual base class for a TlvParameter that contains a single int field.
//  This is a template so that it can support ints of different byte sizes.
//  If it weren't a template, all int parameters would have to be the same
//  size, else the if(*parmval != value) check in VerifyMsg would fail when
//  it compared unused bytes.  Nevertheless, this class would need enhancing
//  to support ints that are not 1, 2, 4, or 8 bytes long.
//
template< typename T > class TlvIntParameter : public TlvParameter
{
protected:
   //  Protected because this class is virtual.
   //
   TlvIntParameter(ProtocolId prid, Id pid) : TlvParameter(prid, pid) { }

   //  Protected because subclasses should be singletons.
   //
   virtual ~TlvIntParameter() { }

   //  Overridden to add an integer to MSG.
   //
   virtual TestRc InjectMsg
      (CliThread& cli, Message& msg, Usage use) const override
   {
      Debug::ft(TlvIntParameter_InjectMsg());

      word value;
      T parmval;
      auto& tlvmsg = static_cast< TlvMessage& >(msg);

      switch(cli.Command()->GetIntParmRc(value, cli))
      {
      case CliParm::None:
         if(use == Mandatory) return StreamMissingMandatoryParm;
         return Ok;
         break;
      case CliParm::Ok:
         break;
      default:
         return IllegalValueInStream;
      }

      parmval = value;

      if(tlvmsg.AddType(parmval, Pid()) == nullptr)
      {
         *cli.obuf << ParameterNotAdded << CRLF;
         return MessageFailedToAddParm;
      }

      return Ok;
   }

   //  Overridden to check an integer in MSG against an expected value.
   //
   virtual TestRc VerifyMsg
      (CliThread& cli, const Message& msg, Usage use) const override
   {
      Debug::ft(TlvIntParameter_VerifyMsg());

      TestRc rc;
      auto& tlvmsg = static_cast< const TlvMessage& >(msg);
      T* parmval;
      word value;
      auto exists = false;

      rc = tlvmsg.VerifyParm(Pid(), use, parmval);
      if(rc != Ok) return rc;
      if(use == Illegal) return Ok;

      switch(cli.Command()->GetIntParmRc(value, cli))
      {
      case CliParm::None:
         if(use == Mandatory) return StreamMissingMandatoryParm;
         break;
      case CliParm::Ok:
         exists = true;
         break;
      default:
         if(use == Mandatory) return IllegalValueInStream;
         return Ok;
      }

      if(exists)
      {
         if(parmval == nullptr) return OptionalParmMissing;
         if(*parmval != value) return ParmValueMismatch;
      }
      else
      {
         if(parmval != nullptr) return OptionalParmPresent;
      }

      return Ok;
   }

   //  See the comment in Singleton.h about fn_name's in a template header.
   //
   inline static fn_name TlvIntParameter_InjectMsg()
      { return "TlvIntParameter.InjectMsg"; }
   inline static fn_name TlvIntParameter_VerifyMsg()
      { return "TlvIntParameter.VerifyMsg"; }
};
}
#endif
