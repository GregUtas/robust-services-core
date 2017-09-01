//==============================================================================
//
//  MscContextPair.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "MscContextPair.h"
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
   Debug::ft(MscContextPair_dtor);
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
   int local;
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
