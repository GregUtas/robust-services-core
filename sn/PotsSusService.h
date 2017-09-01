//==============================================================================
//
//  PotsSusService.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef POTSSUSSERVICE_H_INCLUDED
#define POTSSUSSERVICE_H_INCLUDED

#include "Initiator.h"
#include "NbTypes.h"
#include "SbTypes.h"
#include "Service.h"

using namespace NodeBase;
using namespace SessionBase;

//------------------------------------------------------------------------------

namespace PotsBase
{
class PotsSusInitiator : public Initiator
{
protected:
   PotsSusInitiator(TriggerId tid, Initiator::Priority prio);
   virtual ~PotsSusInitiator() { }
private:
   virtual EventHandler::Rc ProcessEvent(const ServiceSM& parentSsm,
      Event& icEvent, Event*& ogEvent) const override;
};

class PotsOSusInitiator : public PotsSusInitiator
{
public:
   PotsOSusInitiator();
};

class PotsTSusInitiator : public PotsSusInitiator
{
public:
   PotsTSusInitiator();
};

class PotsSusService : public Service
{
   friend class Singleton< PotsSusService >;
private:
   PotsSusService();
   ~PotsSusService();
   virtual ServiceSM* AllocModifier() const override;
};
}
#endif
