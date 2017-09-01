//==============================================================================
//
//  PotsWmlService.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef POTSWMLSERVICE_H_INCLUDED
#define POTSWMLSERVICE_H_INCLUDED

#include "Initiator.h"
#include "NbTypes.h"
#include "Service.h"

using namespace NodeBase;
using namespace SessionBase;

//------------------------------------------------------------------------------

namespace PotsBase
{
class PotsWmlInitiator : public Initiator
{
public:
   PotsWmlInitiator();
private:
   virtual EventHandler::Rc ProcessEvent(const ServiceSM& parentSsm,
      Event& icEvent, Event*& ogEvent) const override;
};

class PotsWmlService : public Service
{
   friend class Singleton< PotsWmlService >;
private:
   PotsWmlService();
   ~PotsWmlService();
   virtual ServiceSM* AllocModifier() const override;
};

class PotsWmlActivate : public Service
{
   friend class Singleton< PotsWmlActivate >;
private:
   PotsWmlActivate();
   ~PotsWmlActivate();
   virtual ServiceSM* AllocModifier() const override;
};

class PotsWmlDeactivate : public Service
{
   friend class Singleton< PotsWmlDeactivate >;
private:
   PotsWmlDeactivate();
   ~PotsWmlDeactivate();
   virtual ServiceSM* AllocModifier() const override;
};
}
#endif
