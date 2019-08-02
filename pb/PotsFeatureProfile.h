//==============================================================================
//
//  PotsFeatureProfile.h
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
#ifndef POTSFEATUREPROFILE_H_INCLUDED
#define POTSFEATUREPROFILE_H_INCLUDED

#include "Protected.h"
#include <cstddef>
#include "Parameter.h"
#include "PotsFeature.h"
#include "Q1Link.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace PotsBase
{
//  Each PotsFeature subclass also defines a profile, which is created when
//  a POTS user subscribes to the feature.  The instance is queued against
//  the user's profile.  It contains data that persists across sessions and
//  that is specific to the user's subscription to the feature.
//
class PotsFeatureProfile : public Protected
{
   friend class PotsProfile;
   friend class Q1Way< PotsFeatureProfile >;
public:
   //  Activates the feature.  The default version generates a log and must
   //  be overridden by features that can be activated and deactivated.
   //
   virtual bool Activate(PotsProfile& profile, CliThread& cli);  //d

   //  Deactivates the feature.  The default version generates a log and must
   //  be overridden by features that can be activated and deactivated.
   //
   virtual bool Deactivate(PotsProfile& profile);

   //  Returns the feature's identifier.
   //
   PotsFeature::Id Fid() const { return fid_; }

   //  Returns the offset to link_.
   //
   static ptrdiff_t LinkDiff();

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;
protected:
   //  Protected because this class is virtual.
   //
   explicit PotsFeatureProfile(PotsFeature::Id fid);

   //  Protected because subclasses should be singletons.
   //
   virtual ~PotsFeatureProfile();
private:
   //  Deleted to prohibit copying.
   //
   PotsFeatureProfile(const PotsFeatureProfile& that) = delete;
   PotsFeatureProfile& operator=(const PotsFeatureProfile& that) = delete;

   //  Deletes the user's subscription to the feature.  Deletion is actually
   //  performed by PotsProfile.Unsubscribe (see below), which also invokes
   //  this function.  The default version does nothing but may be overridden
   //  by subclasses that need to perform feature-specific work when a feature
   //  is unsubscribed.
   //
   virtual bool Unsubscribe(PotsProfile& profile);

   //  The feature's identifier.
   //
   const PotsFeature::Id fid_;

   //  The next feature assigned to the profile.
   //
   Q1Link link_;
};
}
#endif
