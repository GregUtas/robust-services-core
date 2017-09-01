//==============================================================================
//
//  PotsHtlService.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef POTSHTLSERVICE_H_INCLUDED
#define POTSHTLSERVICE_H_INCLUDED

#include "Initiator.h"
#include "NbTypes.h"
#include "Service.h"

using namespace NodeBase;
using namespace SessionBase;

//------------------------------------------------------------------------------

namespace PotsBase
{
class PotsHtlInitiator : public Initiator
{
public:
   PotsHtlInitiator();
private:
   virtual EventHandler::Rc ProcessEvent(const ServiceSM& parentSsm,
      Event& icEvent, Event*& ogEvent) const override;
};

class PotsHtlService : public Service
{
   friend class Singleton< PotsHtlService >;
private:
   PotsHtlService();
   ~PotsHtlService();
   virtual ServiceSM* AllocModifier() const override;
};
}
#endif
