//==============================================================================
//
//  GlobalAddress.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "GlobalAddress.h"
#include <ostream>
#include <string>
#include "Debug.h"
#include "SysTypes.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace SessionBase
{
const GlobalAddress GlobalAddress::NilAddr = GlobalAddress();

//------------------------------------------------------------------------------

fn_name GlobalAddress_ctor1 = "GlobalAddress.ctor";

GlobalAddress::GlobalAddress() : sbAddr_(NilLocalAddress)
{
   Debug::ft(GlobalAddress_ctor1);
}

//------------------------------------------------------------------------------

fn_name GlobalAddress_ctor2 = "GlobalAddress.ctor(L3addr, factory)";

GlobalAddress::GlobalAddress(const SysIpL3Addr& l3Addr, FactoryId fid) :
   SysIpL3Addr(l3Addr),
   sbAddr_(NilLocalAddress)
{
   Debug::ft(GlobalAddress_ctor2);

   sbAddr_.fid = fid;
}

//------------------------------------------------------------------------------

fn_name GlobalAddress_ctor3 = "GlobalAddress.ctor(L2addr, port, factory)";

GlobalAddress::GlobalAddress
   (const SysIpL2Addr& l2Addr, ipport_t port, FactoryId fid) :
   SysIpL3Addr(l2Addr, port),
   sbAddr_(NilLocalAddress)
{
   Debug::ft(GlobalAddress_ctor3);

   sbAddr_.fid = fid;
}

//------------------------------------------------------------------------------

fn_name GlobalAddress_ctor4 = "GlobalAddress.ctor(L3addr, locaddr)";

GlobalAddress::GlobalAddress
   (const SysIpL3Addr& l3Addr, const LocalAddress& sbAddr) :
   SysIpL3Addr(l3Addr),
   sbAddr_(sbAddr)
{
   Debug::ft(GlobalAddress_ctor4);
}

//------------------------------------------------------------------------------

fn_name GlobalAddress_dtor = "GlobalAddress.dtor";

GlobalAddress::~GlobalAddress()
{
   Debug::ft(GlobalAddress_dtor);
}

//------------------------------------------------------------------------------

void GlobalAddress::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   SysIpL3Addr::Display(stream, prefix, options);

   stream << prefix << "sbAddr : " << sbAddr_.to_str() << CRLF;
}

//------------------------------------------------------------------------------

bool GlobalAddress::operator==(const GlobalAddress& that) const
{
   return ((sbAddr_ == that.sbAddr_) && (GetPort() == that.GetPort()) &&
      (GetIpV4Addr() == that.GetIpV4Addr()));
}

//------------------------------------------------------------------------------

bool GlobalAddress::operator!=(const GlobalAddress& that) const
{
   return !(*this == that);
}

//------------------------------------------------------------------------------

void GlobalAddress::Patch(sel_t selector, void* arguments)
{
   SysIpL3Addr::Patch(selector, arguments);
}
}
