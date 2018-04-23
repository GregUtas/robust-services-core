//==============================================================================
//
//  PotsFeature.h
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
#ifndef POTSFEATURE_H_INCLUDED
#define POTSFEATURE_H_INCLUDED

#include "Protected.h"
#include <cstddef>
#include <cstdint>
#include "Parameter.h"
#include "RegCell.h"
#include "Signal.h"

namespace PotsBase
{
   class PotsFeatureProfile;
   class PotsProfile;
}

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace PotsBase
{
//  Each feature that can be assigned to a POTS subscriber provides a subclass,
//  which is a singleton registered with PotsFeatureRegistry.  Each subclass
//  implements behavior that is common to all instances of its feature.
//
class PotsFeature : public Protected
{
   friend class PotsFeatureRegistry;
   friend class Registry< PotsFeature >;
public:
   //  Identifier for a feature that can be assigned to a POTS subscriber.
   //
   typedef uint8_t Id;

   //> The maximum number of POTS features.
   //
   static const Id MaxId = 63;

   //  Returns the feature's identifier.
   //
   Id Fid() const { return Id(fid_.GetId()); }

   //  Returns a string that is an abbreviation for the feature.
   //
   const char* AbbrName() const { return abbr_; }

   //  Returns a string that provides the feature's full name.
   //
   const char* FullName() const { return name_; }

   //  Returns true if the feature can be activated and deactivated.
   //
   bool CanBeDeactivated() const { return deactivation_; }

   //  Returns true if the feature cannot be assigned to a subscriber
   //  who has already been assigned the feature identified by FID.
   //
   bool IsIncompatible(PotsFeature::Id fid) const
      { return incompatible_[fid]; }

   //  Creates an instance of the feature and adds it to PROFILE.  CLI
   //  is the thread from which the feature is being provisioned.
   //
   virtual PotsFeatureProfile* Subscribe
      (PotsProfile& profile, CliThread& cli) const = 0;  //d

   //  Returns the offset to fid_.
   //
   static ptrdiff_t CellDiff();

   //  Overridden to display member variables.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;
protected:
   //  Protected because this class is virtual.  FID is the feature's
   //  identifier, DEACTIVATION is true if it supports activation and
   //  deactivation, and ABBR and NAME are its abbreviation and full
   //  name.
   //
   PotsFeature(PotsFeature::Id fid, bool deactivation,
      const char* abbr, const char* name);

   //  Protected because subclasses should be singletons.
   //
   virtual ~PotsFeature();

   //  Makes the feature incompatible with the one identified by FID.
   //
   void SetIncompatible(PotsFeature::Id fid);
private:
   //  Deleted to prohibit copying.
   //
   PotsFeature(const PotsFeature& that) = delete;
   PotsFeature& operator=(const PotsFeature& that) = delete;

   //  Returns the parameters used to provision the feature.
   //
   virtual CliText* Attrs() const = 0;

   //  The feature's identifier in PotsFeatureRegistry.
   //
   RegCell fid_;

   //  Set if the feature can be activated and deactivated.
   //
   const bool deactivation_;

   //  The feature's abbreviation.
   //
   const char* abbr_;

   //  The feature's full name;
   //
   const char* name_;

   //  Flags that specify which features are incompatible with this one.
   //
   bool incompatible_[PotsFeature::MaxId + 1];
};
}
#endif
