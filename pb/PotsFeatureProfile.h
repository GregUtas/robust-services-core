//==============================================================================
//
//  PotsFeatureProfile.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
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
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;
protected:
   //  Protected because this class is virtual.
   //
   explicit PotsFeatureProfile(PotsFeature::Id fid);

   //  Protected because subclasses should be singletons.
   //
   virtual ~PotsFeatureProfile();
private:
   //  Overridden to prohibit copying.
   //
   PotsFeatureProfile(const PotsFeatureProfile& that);
   void operator=(const PotsFeatureProfile& that);

   //  Deletes the user's subscription to the feature.  Deletion is actually
   //  performed by PotsProfile.Unsubscribe (see below), which also invokes
   //  this function.  The default version does nothing but may be overridden
   //  by subclasses that need to perform feature-specific work when a feature
   //  is unsubscribed.
   //
   virtual bool Unsubscribe(PotsProfile& profile);

   //  The feature's identifier.
   //
   PotsFeature::Id fid_;

   //  The next feature assigned to the profile.
   //
   Q1Link link_;
};
}
#endif
