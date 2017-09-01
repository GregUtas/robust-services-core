//==============================================================================
//
//  PotsCfnService.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef POTSCFNSERVICE_H_INCLUDED
#define POTSCFNSERVICE_H_INCLUDED

#include "Initiator.h"
#include "NbTypes.h"
#include "Service.h"

using namespace NodeBase;
using namespace SessionBase;

//------------------------------------------------------------------------------

namespace PotsBase
{
class PotsCfnInitiator : public Initiator
{
public:
   PotsCfnInitiator();
private:
   virtual EventHandler::Rc ProcessEvent(const ServiceSM& parentSsm,
      Event& icEvent, Event*& ogEvent) const override;
};

//------------------------------------------------------------------------------

class PotsCfnService : public Service
{
   friend class Singleton< PotsCfnService >;
private:
   PotsCfnService();
   ~PotsCfnService();
   virtual ServiceSM* AllocModifier() const override;
};
}
#endif
