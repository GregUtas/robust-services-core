//==============================================================================
//
//  PotsFeatureRegistry.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef POTSFEATUREREGISTRY_H_INCLUDED
#define POTSFEATUREREGISTRY_H_INCLUDED

#include "Protected.h"
#include <memory>
#include "NbTypes.h"
#include "PotsFeature.h"
#include "Registry.h"

namespace NodeBase
{
   class CliTextParm;
}

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace PotsBase
{
//  Registry for the singleton instances of PotsFeature subclasses.
//
class PotsFeatureRegistry : public Protected
{
   friend class Singleton< PotsFeatureRegistry >;
   friend class ActivateCommand;
   friend class DeactivateCommand;
   friend class SubscribeCommand;
   friend class UnsubscribeCommand;
public:
   //  Visits all entries in the registry to build the CLI parameter trees
   //  that support the subscribe, activate, deactivate, and unsubscribe
   //  commands.  Also ensures that if feature A is defined as incompatible
   //  with feature B, that B is also defined as incompatible with A.
   //
   void Audit();

   //  Adds FEATURE to the registry.
   //
   bool BindFeature(PotsFeature& feature);

   //  Removes FEATURE from the registry.
   //
   void UnbindFeature(PotsFeature& feature);

   //  Returns the feature identified by FID.
   //
   PotsFeature* Feature(PotsFeature::Id fid) const;

   //  Overridden to display member variables.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;
private:
   //  Private because this singleton is not subclassed.
   //
   PotsFeatureRegistry();

   //  Private because this singleton is not subclassed.
   //
   ~PotsFeatureRegistry();

   //  The registry of PotsFeature subclasses.
   //
   Registry< PotsFeature > features_;

   //  The CLI parameter tree used when assigning a feature to a profile.
   //
   std::unique_ptr< CliTextParm > featuresSubscribe_;

   //  The CLI parameter tree used when activating a feature.
   //
   std::unique_ptr< CliTextParm > featuresActivate_;

   //  The CLI parameter tree used when deactivating a feature.
   //
   std::unique_ptr< CliTextParm > featuresDeactivate_;

   //  The CLI parameter tree used when removing a feature from a profile.
   //
   std::unique_ptr< CliTextParm > featuresUnsubscribe_;
};
}
#endif
