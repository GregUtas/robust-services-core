//==============================================================================
//
//  PotsProfileRegistry.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef POTSPROFILEREGISTRY_H_INCLUDED
#define POTSPROFILEREGISTRY_H_INCLUDED

#include "Protected.h"
#include "BcAddress.h"
#include "NbTypes.h"
#include "Registry.h"

namespace PotsBase
{
   class PotsProfile;
}

using namespace NodeBase;
using namespace CallBase;

//------------------------------------------------------------------------------

namespace PotsBase
{
//  Registry for PotsProfile instances.
//
class PotsProfileRegistry : public Protected
{
   friend class Singleton< PotsProfileRegistry >;
public:
   //  Adds PROFILE to the registry.
   //
   bool BindProfile(PotsProfile& profile);

   //  Removes PROFILE from the registry.
   //
   void UnbindProfile(PotsProfile& profile);

   //  Returns DN's profile.
   //
   PotsProfile* Profile(Address::DN dn) const;

   //  Returns DN's profile.  If DN is not registered, searches sequentially
   //  starting at DN and returns the profile of the next registered DN.
   //
   PotsProfile* FirstProfile(Address::DN dn) const;

   //  Returns the profile that follows PROFILE.
   //
   PotsProfile* NextProfile(const PotsProfile& profile) const;

   //  Overridden for restarts.
   //
   virtual void Startup(RestartLevel level) override;

   //  Overridden for restarts.
   //
   virtual void Shutdown(RestartLevel level) override;

   //  Overridden to display member variables.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;
private:
   //  Private because this singleton is not subclassed.
   //
   PotsProfileRegistry();

   //  Private because this singleton is not subclassed.
   //
   ~PotsProfileRegistry();

   //  The registry of POTS profiles.
   //
   Registry< PotsProfile > profiles_;
};
}
#endif
