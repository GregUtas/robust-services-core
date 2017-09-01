//==============================================================================
//
//  IpServiceRegistry.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef IPSERVICEREGISTRY_H_INCLUDED
#define IPSERVICEREGISTRY_H_INCLUDED

#include "Protected.h"
#include <string>
#include "NbTypes.h"
#include "Registry.h"

namespace NodeBase
{
   class IpService;
}

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Global registry for protocols supported over IP.
//
class IpServiceRegistry : public Protected
{
   friend class IpService;
   friend class Singleton< IpServiceRegistry >;
public:
   //  Returns the service registered against NAME.
   //
   IpService* GetService(const std::string& name) const;

   //  Overridden for restarts.
   //
   virtual void Startup(RestartLevel level) override;

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
   IpServiceRegistry();

   //  Private because this singleton is not subclassed.
   //
   ~IpServiceRegistry();

   //  Adds SERVICE to the registry.  Invoked by IpService's base class
   //  constructor.
   //
   bool BindService(IpService& service);

   //  Removes SERVICE from the registry.  Invoked by IpService's base
   //  class destructor.
   //
   void UnbindService(IpService& service);

   //  The global registry of IP services.
   //
   Registry< IpService > services_;
};
}
#endif
