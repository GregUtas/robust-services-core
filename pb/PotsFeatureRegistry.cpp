//==============================================================================
//
//  PotsFeatureRegistry.cpp
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
#include "PotsFeatureRegistry.h"
#include "CliTextParm.h"
#include <ostream>
#include <string>
#include "CliText.h"
#include "Debug.h"
#include "Formatters.h"
#include "SysTypes.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace PotsBase
{
class WhichFeatureParm : public CliTextParm
{
public: WhichFeatureParm();
};

fixed_string WhichFeatureExpl = "feature abbreviation...";

WhichFeatureParm::WhichFeatureParm() :
   CliTextParm(WhichFeatureExpl, false, PotsFeature::MaxId + 1) { }

//------------------------------------------------------------------------------

fn_name PotsFeatureRegistry_ctor = "PotsFeatureRegistry.ctor";

PotsFeatureRegistry::PotsFeatureRegistry()
{
   Debug::ft(PotsFeatureRegistry_ctor);

   features_.Init(PotsFeature::MaxId, PotsFeature::CellDiff(), MemProt);
   featuresSubscribe_.reset(new WhichFeatureParm);
   featuresActivate_.reset(new WhichFeatureParm);
   featuresDeactivate_.reset(new WhichFeatureParm);
   featuresUnsubscribe_.reset(new WhichFeatureParm);
}

//------------------------------------------------------------------------------

fn_name PotsFeatureRegistry_dtor = "PotsFeatureRegistry.dtor";

PotsFeatureRegistry::~PotsFeatureRegistry()
{
   Debug::ft(PotsFeatureRegistry_dtor);
}

//------------------------------------------------------------------------------

fn_name PotsFeatureRegistry_Audit = "PotsFeatureRegistry.Audit";

void PotsFeatureRegistry::Audit()
{
   Debug::ft(PotsFeatureRegistry_Audit);

   for(auto f1 = features_.First(); f1 != nullptr; features_.Next(f1))
   {
      auto fid1 = f1->Fid();

      //  Create the CLI parameters that support each feature in the
      //  Subscribe, Activate, Deactivate, and Unsubscribe commands.
      //
      featuresSubscribe_->BindText(*f1->Attrs(), fid1);

      auto name = new CliText(f1->FullName(), f1->AbbrName());
      featuresUnsubscribe_->BindText(*name, fid1);

      if(f1->CanBeDeactivated())
      {
         featuresActivate_->BindText(*f1->Attrs(), fid1);
         name = new CliText(f1->FullName(), f1->AbbrName());
         featuresDeactivate_->BindText(*name, fid1);
      }

      //  Ensure that each pair of features agrees on their compatibility.
      //
      for(auto f2 = features_.First(); f2 != nullptr; features_.Next(f2))
      {
         auto fid2 = f2->Fid();

         if(f1->IsIncompatible(fid2) != f2->IsIncompatible(fid1))
         {
            Debug::SwLog(PotsFeatureRegistry_Audit, fid1, fid2);
            f1->SetIncompatible(fid2);
            f2->SetIncompatible(fid1);
         }
      }
   }
}

//------------------------------------------------------------------------------

fn_name PotsFeatureRegistry_BindFeature = "PotsFeatureRegistry.BindFeature";

bool PotsFeatureRegistry::BindFeature(PotsFeature& feature)
{
   Debug::ft(PotsFeatureRegistry_BindFeature);

   return features_.Insert(feature);
}

//------------------------------------------------------------------------------

void PotsFeatureRegistry::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Protected::Display(stream, prefix, options);

   stream << prefix << "featuresSubscribe   : ";
   stream << strObj(featuresSubscribe_.get());
   stream << prefix << "featuresActivate    : ";
   stream << strObj(featuresActivate_.get());
   stream << prefix << "featuresDeactivate  : ";
   stream << strObj(featuresDeactivate_.get());
   stream << prefix << "featuresUnsubscribe : ";
   stream << strObj(featuresUnsubscribe_.get());

   stream << prefix << "features [PotsFeature::Id]" << CRLF;
   features_.Display(stream, prefix + spaces(2), options);
}

//------------------------------------------------------------------------------

PotsFeature* PotsFeatureRegistry::Feature(PotsFeature::Id fid) const
{
   return features_.At(fid);
}

//------------------------------------------------------------------------------

fn_name PotsFeatureRegistry_UnbindFeature = "PotsFeatureRegistry.UnbindFeature";

void PotsFeatureRegistry::UnbindFeature(PotsFeature& feature)
{
   Debug::ft(PotsFeatureRegistry_UnbindFeature);

   features_.Erase(feature);
}
}
