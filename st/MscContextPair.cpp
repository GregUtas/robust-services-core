//==============================================================================
//
//  MscContextPair.cpp
//
//  Copyright (C) 2013-2020  Greg Utas
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
#include "MscContextPair.h"
#include <cstdint>
#include <ostream>
#include <string>
#include "Algorithms.h"
#include "Debug.h"
#include "SysTypes.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace SessionTools
{
fn_name MscContextPair_ctor = "MscContextPair.ctor";

MscContextPair::MscContextPair(MscContext& ctx1, MscContext& ctx2) :
   ctx1_(&ctx1),
   ctx2_(&ctx2)
{
   Debug::ft(MscContextPair_ctor);
}

//------------------------------------------------------------------------------

fn_name MscContextPair_dtor = "MscContextPair.dtor";

MscContextPair::~MscContextPair()
{
   Debug::ftnt(MscContextPair_dtor);
}

//------------------------------------------------------------------------------

void MscContextPair::Contexts(MscContext*& ctx1, MscContext*& ctx2) const
{
   ctx1 = ctx1_;
   ctx2 = ctx2_;
}

//------------------------------------------------------------------------------

void MscContextPair::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Temporary::Display(stream, prefix, options);

   stream << prefix << "ctx1 : " << ctx1_ << CRLF;
   stream << prefix << "ctx2 : " << ctx2_ << CRLF;
}

//------------------------------------------------------------------------------

fn_name MscContextPair_IsEqualTo = "MscContextPair.IsEqualTo";

bool MscContextPair::IsEqualTo
   (const MscContext& ctx1, const MscContext& ctx2) const
{
   Debug::ft(MscContextPair_IsEqualTo);

   if(ctx1_ == &ctx1) return (ctx2_ == &ctx2);
   if(ctx1_ == &ctx2) return (ctx2_ == &ctx1);
   return false;
}

//------------------------------------------------------------------------------

ptrdiff_t MscContextPair::LinkDiff()
{
   uintptr_t local;
   auto fake = reinterpret_cast< const MscContextPair* >(&local);
   return ptrdiff(&fake->link_, fake);
}

//------------------------------------------------------------------------------

fn_name MscContextPair_Peer = "MscContextPair.Peer";

MscContext* MscContextPair::Peer(const MscContext& context) const
{
   Debug::ft(MscContextPair_Peer);

   if(ctx1_ == &context) return ctx2_;
   if(ctx2_ == &context) return ctx1_;
   return nullptr;
}
}
