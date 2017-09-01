//==============================================================================
//
//  FactoryRegistry.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef FACTORYREGISTRY_H_INCLUDED
#define FACTORYREGISTRY_H_INCLUDED

#include "Protected.h"
#include "NbTypes.h"
#include "Registry.h"
#include "SbTypes.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace SessionBase
{
//  Global registry for factories.
//
class FactoryRegistry : public Protected
{
   friend class Factory;
   friend class Singleton< FactoryRegistry >;
public:
   //  Returns the factory registered against FID.
   //
   Factory* GetFactory(FactoryId fid) const;

   //  Returns the registry of factories.  Used for iteration.
   //
   const Registry< Factory >& Factories() const { return factories_; }

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

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;
private:
   //  Private because this singleton is not subclassed.
   //
   FactoryRegistry();

   //  Private because this singleton is not subclassed.
   //
   ~FactoryRegistry();

   //  Registers FACTORY against its identifier.  Invoked by Factory's base
   //  class constructor.
   //
   bool BindFactory(Factory& factory);

   //  Removes FACTORY from the registry.  Invoked by Factory's base class
   //  destructor.
   //
   void UnbindFactory(Factory& factory);

   //  The global registry of factories.
   //
   Registry< Factory > factories_;

   //  The statistics group for factories.
   //
   StatisticsGroupPtr statsGroup_;
};
}
#endif
