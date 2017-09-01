//==============================================================================
//
//  PotsCfbService.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef POTSCFBSERVICE_H_INCLUDED
#define POTSCFBSERVICE_H_INCLUDED

#include "Initiator.h"
#include "NbTypes.h"
#include "Service.h"

using namespace NodeBase;
using namespace SessionBase;

//------------------------------------------------------------------------------

namespace PotsBase
{
class PotsCfbInitiator : public Initiator
{
public:
   PotsCfbInitiator();
private:
   virtual EventHandler::Rc ProcessEvent(const ServiceSM& parentSsm,
      Event& icEvent, Event*& ogEvent) const override;
};

//------------------------------------------------------------------------------

class PotsCfbService : public Service
{
   friend class Singleton< PotsCfbService >;
private:
   PotsCfbService();
   ~PotsCfbService();
   virtual ServiceSM* AllocModifier() const override;
};
}
#endif
