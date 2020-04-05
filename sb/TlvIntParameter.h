//==============================================================================
//
//  TlvIntParameter.h
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
#ifndef TLVINTPARAMETER_H_INCLUDED
#define TLVINTPARAMETER_H_INCLUDED

#include "TlvParameter.h"
#include <sstream>
#include "CliCommand.h"
#include "CliThread.h"
#include "Debug.h"
#include "SbCliParms.h"
#include "SbTypes.h"
#include "SysTypes.h"
#include "TlvMessage.h"

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
template< class T > class TlvIntParameter : public TlvParameter
{
protected:
   //  Protected because this class is virtual.
   //
   TlvIntParameter(ProtocolId prid, Id pid) : TlvParameter(prid, pid)
   {
      NodeBase::Debug::ft(TlvIntParameter_ctor());
   }

   //  Protected because subclasses should be singletons.
   //
   virtual ~TlvIntParameter() = default;

   //  Overridden to add an integer to MSG.
   //
   TestRc InjectMsg
      (NodeBase::CliThread& cli, Message& msg, Usage use) const override
   {
      NodeBase::Debug::ft(TlvIntParameter_InjectMsg());

      NodeBase::word value;
      auto& tlvmsg = static_cast< TlvMessage& >(msg);

      switch(cli.Command()->GetIntParmRc(value, cli))
      {
      case NodeBase::CliParm::None:
         if(use == Mandatory) return StreamMissingMandatoryParm;
         return Ok;
      case NodeBase::CliParm::Ok:
         break;
      default:
         return IllegalValueInStream;
      }

      T parmval = value;

      if(tlvmsg.AddType(parmval, Pid()) == nullptr)
      {
         *cli.obuf << ParameterNotAdded << NodeBase::CRLF;
         return MessageFailedToAddParm;
      }

      return Ok;
   }

   //  Overridden to check an integer in MSG against an expected value.
   //
   TestRc VerifyMsg
      (NodeBase::CliThread& cli, const Message& msg, Usage use) const override
   {
      NodeBase::Debug::ft(TlvIntParameter_VerifyMsg());

      TestRc rc;
      auto& tlvmsg = static_cast< const TlvMessage& >(msg);
      T* parmval;
      NodeBase::word value;
      auto exists = false;

      rc = tlvmsg.VerifyParm(Pid(), use, parmval);
      if(rc != Ok) return rc;
      if(use == Illegal) return Ok;

      switch(cli.Command()->GetIntParmRc(value, cli))
      {
      case NodeBase::CliParm::None:
         if(use == Mandatory) return StreamMissingMandatoryParm;
         break;
      case NodeBase::CliParm::Ok:
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
private:
   //  See the comment in Singleton.h about fn_name's in a template header.
   //
   inline static NodeBase::fn_name TlvIntParameter_ctor()
      { return "TlvIntParameter.ctor"; }
   inline static NodeBase::fn_name TlvIntParameter_InjectMsg()
      { return "TlvIntParameter.InjectMsg"; }
   inline static NodeBase::fn_name TlvIntParameter_VerifyMsg()
      { return "TlvIntParameter.VerifyMsg"; }
};
}
#endif
