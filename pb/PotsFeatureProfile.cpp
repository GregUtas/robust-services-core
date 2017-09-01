//==============================================================================
//
//  PotsFeatureProfile.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "PotsFeatureProfile.h"
#include <ostream>
#include <string>
#include "Algorithms.h"
#include "Debug.h"
#include "PotsFeatureRegistry.h"
#include "Singleton.h"
#include "SysTypes.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace PotsBase
{
fn_name PotsFeatureProfile_ctor = "PotsFeatureProfile.ctor";

PotsFeatureProfile::PotsFeatureProfile(PotsFeature::Id fid) : fid_(fid)
{
   Debug::ft(PotsFeatureProfile_ctor);
}

//------------------------------------------------------------------------------

fn_name PotsFeatureProfile_dtor = "PotsFeatureProfile.dtor";

PotsFeatureProfile::~PotsFeatureProfile()
{
   Debug::ft(PotsFeatureProfile_dtor);
}

//------------------------------------------------------------------------------

fn_name PotsFeatureProfile_Activate = "PotsFeatureProfile.Activate";

bool PotsFeatureProfile::Activate(PotsProfile& profile, CliThread& cli)
{
   Debug::ft(PotsFeatureProfile_Activate);

   Debug::SwErr(PotsFeatureProfile_Activate, fid_, 0);
   return false;
}

//------------------------------------------------------------------------------

fn_name PotsFeatureProfile_Deactivate = "PotsFeatureProfile.Deactivate";

bool PotsFeatureProfile::Deactivate(PotsProfile& profile)
{
   Debug::ft(PotsFeatureProfile_Deactivate);

   Debug::SwErr(PotsFeatureProfile_Deactivate, fid_, 0);
   return false;
}

//------------------------------------------------------------------------------

void PotsFeatureProfile::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Protected::Display(stream, prefix, options);

   auto ftr = Singleton< PotsFeatureRegistry >::Instance()->Feature(fid_);

   stream << prefix << "fid  : " << int(fid_);
   stream << " (" << ftr->AbbrName() << ')' << CRLF;
   stream << prefix << "link : " << link_.to_str() << CRLF;
}

//------------------------------------------------------------------------------

ptrdiff_t PotsFeatureProfile::LinkDiff()
{
   int local;
   auto fake = reinterpret_cast< const PotsFeatureProfile* >(&local);
   return ptrdiff(&fake->link_, fake);
}

//------------------------------------------------------------------------------

fn_name PotsFeatureProfile_Unsubscribe = "PotsFeatureProfile.Unsubscribe";

bool PotsFeatureProfile::Unsubscribe(PotsProfile& profile)
{
   Debug::ft(PotsFeatureProfile_Unsubscribe);

   return true;
}
}
