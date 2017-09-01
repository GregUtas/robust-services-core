//==============================================================================
//
//  PotsFeature.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "PotsFeature.h"
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
fn_name PotsFeature_ctor = "PotsFeature.ctor";

PotsFeature::PotsFeature(PotsFeature::Id fid, bool deactivation,
   const char* abbr, const char* name) :
   deactivation_(deactivation),
   abbr_(abbr),
   name_(name)
{
   Debug::ft(PotsFeature_ctor);

   Debug::Assert(abbr_ != nullptr);
   Debug::Assert(name_ != nullptr);

   fid_.SetId(fid);
   for(auto i = 0; i <= MaxId; ++i) incompatible_[i] = false;
   incompatible_[fid] = true;

   Singleton< PotsFeatureRegistry >::Instance()->BindFeature(*this);
}

//------------------------------------------------------------------------------

fn_name PotsFeature_dtor = "PotsFeature.dtor";

PotsFeature::~PotsFeature()
{
   Debug::ft(PotsFeature_dtor);

   Singleton< PotsFeatureRegistry >::Instance()->UnbindFeature(*this);
}

//------------------------------------------------------------------------------

fn_name PotsFeature_Attrs = "PotsFeature.Attrs";

CliText* PotsFeature::Attrs() const
{
   Debug::ft(PotsFeature_Attrs);

   //  This is a pure virtual function.
   //
   Debug::SwErr(PotsFeature_Attrs, 0, 0, ErrorLog);
   return nullptr;
}

//------------------------------------------------------------------------------

ptrdiff_t PotsFeature::CellDiff()
{
   int local;
   auto fake = reinterpret_cast< const PotsFeature* >(&local);
   return ptrdiff(&fake->fid_, fake);
}

//------------------------------------------------------------------------------

void PotsFeature::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Protected::Display(stream, prefix, options);

   stream << prefix << "fid          : " << fid_.to_str() << CRLF;
   stream << prefix << "deactivation : " << deactivation_ << CRLF;
   stream << prefix << "abbr         : " << abbr_ << CRLF;
   stream << prefix << "name         : " << name_ << CRLF;
   stream << prefix << "incompatible : ";

   for(auto i = 0; i <= MaxId; ++i)
   {
      if(incompatible_[i] && (i != Fid()))
      {
         auto ftr = Singleton< PotsFeatureRegistry >::Instance()->Feature(i);
         stream << ftr->AbbrName() << SPACE;
      }
   }

   stream << CRLF;
}

//------------------------------------------------------------------------------

fn_name PotsFeature_SetIncompatible = "PotsFeature.SetIncompatible";

void PotsFeature::SetIncompatible(PotsFeature::Id fid)
{
   Debug::ft(PotsFeature_SetIncompatible);

   if(fid <= MaxId) incompatible_[fid] = true;
}

//------------------------------------------------------------------------------

fn_name PotsFeature_Subscribe = "PotsFeature.Subscribe";

PotsFeatureProfile* PotsFeature::Subscribe
   (PotsProfile& profile, CliThread& cli) const
{
   Debug::ft(PotsFeature_Subscribe);

   //  This is a pure virtual function.
   //
   Debug::SwErr(PotsFeature_Subscribe, 0, 0, ErrorLog);
   return nullptr;
}
}
