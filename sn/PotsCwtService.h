//==============================================================================
//
//  PotsCwtService.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef POTSCWTSERVICE_H_INCLUDED
#define POTSCWTSERVICE_H_INCLUDED

#include "Initiator.h"
#include "NbTypes.h"
#include "PotsProtocol.h"
#include "Service.h"

using namespace NodeBase;
using namespace SessionBase;

//------------------------------------------------------------------------------

namespace PotsBase
{
class PotsCwtInitiator : public Initiator
{
public:
   PotsCwtInitiator();
private:
   virtual EventHandler::Rc ProcessEvent(const ServiceSM& parentSsm,
      Event& icEvent, Event*& ogEvent) const override;
};

class PotsCwtService : public Service
{
protected:
   explicit PotsCwtService(Id sid);
   virtual ~PotsCwtService();
};

class PotsCwaService : public PotsCwtService
{
   friend class Singleton< PotsCwaService >;
private:
   PotsCwaService();
   ~PotsCwaService();
   virtual ServiceSM* AllocModifier() const override;
};

class PotsCwbService : public PotsCwtService
{
   friend class Singleton< PotsCwbService >;
private:
   PotsCwbService();
   ~PotsCwbService();
   virtual ServiceSM* AllocModifier() const override;
};

class PotsCwmService : public Service
{
   friend class Singleton< PotsCwmService >;
private:
   PotsCwmService();
   ~PotsCwmService();
   virtual ServiceSM* AllocModifier() const override;
};

class PotsCwtFacility : public Facility
{
public:
   static const Ind InitiationTimeout = NextInd;
   static const Ind Unanswered        = NextInd + 1;
   static const Ind Answered          = NextInd + 2;
   static const Ind Retrieved         = NextInd + 3;
   static const Ind Reconnected       = NextInd + 4;
   static const Ind Reanswered        = NextInd + 5;
   static const Ind InactiveReleased  = NextInd + 6;
   static const Ind Alerted           = NextInd + 7;
};
}
#endif
