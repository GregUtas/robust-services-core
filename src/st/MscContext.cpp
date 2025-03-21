//==============================================================================
//
//  MscContext.cpp
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
#include "MscContext.h"
#include <ostream>
#include "Algorithms.h"
#include "Debug.h"
#include "FactoryRegistry.h"
#include "Formatters.h"
#include "Registry.h"
#include "ServiceRegistry.h"
#include "Singleton.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace SessionTools
{
MscContext::MscContext(const void* rcvr, ContextType type, uint16_t cid) :
   rcvr_(rcvr),
   type_(type),
   cid_(cid),
   col_(NilMscColumn),
   group_(0)
{
   Debug::ft("MscContext.ctor");
}

//------------------------------------------------------------------------------

MscContext::~MscContext()
{
   Debug::ftnt("MscContext.dtor");
}

//------------------------------------------------------------------------------

void MscContext::ClearGroup()
{
   Debug::ft("MscContext.ClearGroup");

   if(rcvr_ == nullptr) group_ = 0;
}

//------------------------------------------------------------------------------

void MscContext::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Temporary::Display(stream, prefix, options);

   stream << prefix << "rcvr  : " << rcvr_ << CRLF;
   stream << prefix << "type  : " << type_ << CRLF;
   stream << prefix << "cid   : " << cid_ << CRLF;
   stream << prefix << "col   : " << col_ << CRLF;
   stream << prefix << "group : " << group_ << CRLF;
}

//------------------------------------------------------------------------------

bool MscContext::IsEqualTo(const void* rcvr, uint16_t cid) const
{
   if(rcvr == nullptr) return ((rcvr_ == nullptr) && (cid_ == cid));
   return (rcvr_ == rcvr);
}

//------------------------------------------------------------------------------

ptrdiff_t MscContext::LinkDiff()
{
   uintptr_t local;
   auto fake = reinterpret_cast<const MscContext*>(&local);
   return ptrdiff(&fake->link_, fake);
}

//------------------------------------------------------------------------------

void MscContext::Names(string& text1, string& text2) const
{
   Debug::ft("MscContext.Names");

   if((rcvr_ == nullptr) && (cid_ == NIL_ID))
   {
      text1 = "External";
      text2 = "Contexts";
      return;
   }

   if(type_ == MultiPort)
   {
      auto reg = Singleton<ServiceRegistry>::Instance();
      text1 = strClass(reg->Services().At(cid_), false);
   }
   else
   {
      auto reg = Singleton<FactoryRegistry>::Instance();
      text1 = strClass(reg->Factories().At(FactoryId(cid_)), false);
   }

   text2 = strContextType(type_);
   text2.push_back(':');

   if(rcvr_ != nullptr)
      text2 += strPtr(rcvr_);
   else
      text2 += "external";
}

//------------------------------------------------------------------------------

MscColumn MscContext::SetColumn(MscColumn col)
{
   Debug::ft("MscContext.SetColumn");

   col_ = col;
   return col + ColWidth;
}

//------------------------------------------------------------------------------

bool MscContext::SetGroup(int group)
{
   Debug::ft("MscContext.SetGroup");

   if(rcvr_ == nullptr)
   {
      //  An external context is always assigned to the current group,
      //  but it cannot begin a new group.
      //
      group_ = group;
      return false;
   }

   if(group_ == 0)
   {
      //  An internal context is only assigned to the current group if
      //  it does not yet have a group, and this can begin a new group.
      //
      group_ = group;
      return true;
   }

   //  This internal context is already assigned to a group.
   //
   return false;
}
}
