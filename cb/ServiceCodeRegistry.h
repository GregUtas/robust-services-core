//==============================================================================
//
//  ServiceCodeRegistry.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef SERVICECODEREGISTRY_H_INCLUDED
#define SERVICECODEREGISTRY_H_INCLUDED

#include "Protected.h"
#include "BcAddress.h"
#include "NbTypes.h"
#include "SbTypes.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace CallBase
{
//  Registry for service codes (*nn digit strings used to control services).
//
class ServiceCodeRegistry : public Protected
{
   friend class Singleton< ServiceCodeRegistry >;
public:
   //  Associates the service identified by SID when the service code
   //  identified by SC.
   //
   void SetService(Address::SC sc, ServiceId sid);

   //  Returns the service associated with SC.
   //
   ServiceId GetService(Address::SC sc) const;

   //  Overridden for restarts.
   //
   virtual void Startup(RestartLevel level) override;

   //  Overridden to display member variables.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;
private:
   //  Private because this singleton is not subclassed.
   //
   ServiceCodeRegistry();

   //  Private because this singleton is not subclassed.
   //
   ~ServiceCodeRegistry();

   //  The table that maps service codes to service identifiers.
   //
   ServiceId codeToService_[Address::LastSC + 1];
};
}
#endif
