//==============================================================================
//
//  PotsFeatureRegistry.h
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
#ifndef POTSFEATUREREGISTRY_H_INCLUDED
#define POTSFEATUREREGISTRY_H_INCLUDED

#include "Immutable.h"
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
//  Registry for POTS features.
//
class PotsFeatureRegistry : public Immutable
{
   friend class Singleton< PotsFeatureRegistry >;
   friend class PotsFeature;
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

   //  Returns the feature identified by FID.
   //
   PotsFeature* Feature(PotsFeature::Id fid) const;

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;
private:
   //  Private because this singleton is not subclassed.
   //
   PotsFeatureRegistry();

   //  Private because this singleton is not subclassed.
   //
   ~PotsFeatureRegistry();

   //  Adds FEATURE to the registry.
   //
   bool BindFeature(PotsFeature& feature);

   //  Removes FEATURE from the registry.
   //
   void UnbindFeature(PotsFeature& feature);

   //  The registry of POTS features.
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
