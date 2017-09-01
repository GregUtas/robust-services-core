//==============================================================================
//
//  PotsCfuService.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef POTSCFUSERVICE_H_INCLUDED
#define POTSCFUSERVICE_H_INCLUDED

#include "Initiator.h"
#include "NbTypes.h"
#include "Service.h"

using namespace NodeBase;
using namespace SessionBase;

//------------------------------------------------------------------------------

namespace PotsBase
{
class PotsCfuInitiator : public Initiator
{
public:
   PotsCfuInitiator();
private:
   virtual EventHandler::Rc ProcessEvent(const ServiceSM& parentSsm,
      Event& icEvent, Event*& ogEvent) const override;
};

//------------------------------------------------------------------------------

class PotsCfuActivate : public Service
{
   friend class Singleton< PotsCfuActivate >;
private:
   PotsCfuActivate();
   ~PotsCfuActivate();
   virtual ServiceSM* AllocModifier() const override;
};

//------------------------------------------------------------------------------

class PotsCfuDeactivate : public Service
{
   friend class Singleton< PotsCfuDeactivate >;
private:
   PotsCfuDeactivate();
   ~PotsCfuDeactivate();
   virtual ServiceSM* AllocModifier() const override;
};

//------------------------------------------------------------------------------

class PotsCfuService : public Service
{
   friend class Singleton< PotsCfuService >;
private:
   PotsCfuService();
   ~PotsCfuService();
   virtual ServiceSM* AllocModifier() const override;
};
}
#endif
