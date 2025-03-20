//==============================================================================
//
//  MscAddress.cpp
//
//  Copyright (C) 2013-2025  Greg Utas
//
//  This file is part of the Robust Services Core (RSC).
//
//  RSC is free software: you can redistribute it and/or modify it under the
//  terms of the Lesser GNU General Public License as published by the Free
//  Software Foundation, either version 3 of the License, or (at your option)
//  any later version.
//
//  RSC is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
//  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
//  details.
//
//  You should have received a copy of the Lesser GNU General Public License
//  along with RSC.  If not, see <http://www.gnu.org/licenses/>.
//
#include "MscAddress.h"
#include <cstdint>
#include <ostream>
#include <string>
#include "Algorithms.h"
#include "Debug.h"
#include "Message.h"
#include "SbTrace.h"
#include "SysTypes.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace SessionTools
{
MscAddress::MscAddress(const MsgTrace& mt, MscContext* context) :
   locAddr_(mt.LocAddr()),
   context_(context),
   external_(false),
   extFid_(NIL_ID)
{
   Debug::ft("MscAddress.ctor");

   SetPeer(mt, context);
}

//------------------------------------------------------------------------------

MscAddress::~MscAddress()
{
   Debug::ftnt("MscAddress.dtor");
}

//------------------------------------------------------------------------------

void MscAddress::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Temporary::Display(stream, prefix, options);

   stream << prefix << "locAddr  : " << locAddr_.to_str() << CRLF;
   stream << prefix << "remAddr  : " << remAddr_.to_str() << CRLF;
   stream << prefix << "context  : " << context_ << CRLF;
   stream << prefix << "external : " << external_ << CRLF;
   stream << prefix << "extFid   : " << extFid_ << CRLF;
}

//------------------------------------------------------------------------------

bool MscAddress::ExternalFid(FactoryId& fid) const
{
   if(!external_) return false;
   fid = extFid_;
   return true;
}

//------------------------------------------------------------------------------

ptrdiff_t MscAddress::LinkDiff()
{
   uintptr_t local;
   auto fake = reinterpret_cast<const MscAddress*>(&local);
   return ptrdiff(&fake->link_, fake);
}

//------------------------------------------------------------------------------

fn_name MscAddress_SetPeer = "MscAddress.SetPeer";

void MscAddress::SetPeer(const MsgTrace& mt, MscContext* context)
{
   Debug::ft(MscAddress_SetPeer);

   if(context_ == nullptr) context_ = context;

   if(mt.Route() == Message::Internal)
   {
      if(locAddr_.bid == mt.LocAddr().bid)
      {
         if(remAddr_.bid == NIL_ID)
         {
            remAddr_ = mt.RemAddr();
         }

         return;
      }

      if(locAddr_.bid == mt.RemAddr().bid)
      {
         if(remAddr_.bid == NIL_ID)
         {
            remAddr_ = mt.LocAddr();
         }

         return;
      }
   }
   else if(!external_)
   {
      external_ = true;
      extFid_ = mt.RemAddr().fid;
   }
   else if(extFid_ != mt.RemAddr().fid)
   {
      Debug::SwLog(MscAddress_SetPeer, "unexpected factory",
         pack3(locAddr_.fid, extFid_, mt.RemAddr().fid));
   }
}
}
