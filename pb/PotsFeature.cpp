//==============================================================================
//
//  PotsFeature.cpp
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
#include "PotsFeature.h"
#include <ostream>
#include <string>
#include "Algorithms.h"
#include "Debug.h"
#include "PotsFeatureRegistry.h"
#include "Singleton.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace PotsBase
{
fn_name PotsFeature_ctor = "PotsFeature.ctor";

PotsFeature::PotsFeature(PotsFeature::Id fid, bool deactivation,
   c_string abbr, c_string name) :
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

   Debug::SwLog(PotsFeature_Attrs, strOver(this), 0);
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

   Debug::SwLog(PotsFeature_Subscribe, strOver(this), 0);
   return nullptr;
}
}
