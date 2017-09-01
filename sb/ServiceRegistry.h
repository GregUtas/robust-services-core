//==============================================================================
//
//  ServiceRegistry.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef SERVICEREGISTRY_H_INCLUDED
#define SERVICEREGISTRY_H_INCLUDED

#include "Protected.h"
#include "NbTypes.h"
#include "Registry.h"
#include "SbTypes.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace SessionBase
{
//  Global registry for services.
//
class ServiceRegistry : public Protected
{
   friend class Service;
   friend class Singleton< ServiceRegistry >;
public:
   //  Returns the service registered against SID.
   //
   Service* GetService(ServiceId sid) const;

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
   ServiceRegistry();

   //  Private because this singleton is not subclassed.
   //
   ~ServiceRegistry();

   //  Registers SERVICE against its service identifier.
   //
   bool BindService(Service& service);

   //  Removes SERVICE from the registry.
   //
   void UnbindService(Service& service);

   //  The global registry of services.
   //
   Registry< Service > services_;
};
}
#endif
