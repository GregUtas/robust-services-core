//==============================================================================
//
//  PotsBicService.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef POTSBICSERVICE_H_INCLUDED
#define POTSBICSERVICE_H_INCLUDED

#include "Initiator.h"
#include "NbTypes.h"
#include "Service.h"

using namespace NodeBase;
using namespace SessionBase;

//------------------------------------------------------------------------------

namespace PotsBase
{
class PotsBicInitiator : public Initiator
{
public:
   PotsBicInitiator();
private:
   virtual EventHandler::Rc ProcessEvent(const ServiceSM& parentSsm,
      Event& icEvent, Event*& ogEvent) const override;
};

class PotsBicService : public Service
{
   friend class Singleton< PotsBicService >;
private:
   PotsBicService();
   ~PotsBicService();
   virtual ServiceSM* AllocModifier() const override;
};
}
#endif
