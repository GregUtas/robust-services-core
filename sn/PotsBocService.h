//==============================================================================
//
//  PotsBocService.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef POTSBOCSERVICE_H_INCLUDED
#define POTSBOCSERVICE_H_INCLUDED

#include "Initiator.h"
#include "NbTypes.h"
#include "Service.h"

using namespace NodeBase;
using namespace SessionBase;

//------------------------------------------------------------------------------

namespace PotsBase
{
class PotsBocInitiator : public Initiator
{
public:
   PotsBocInitiator();
private:
   virtual EventHandler::Rc ProcessEvent(const ServiceSM& parentSsm,
      Event& icEvent, Event*& ogEvent) const override;
};

class PotsBocService : public Service
{
   friend class Singleton< PotsBocService >;
private:
   PotsBocService();
   ~PotsBocService();
   virtual ServiceSM* AllocModifier() const override;
};
}
#endif
