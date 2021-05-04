//==============================================================================
//
//  PotsCwtService.h
//
//  Copyright (C) 2013-2021  Greg Utas
//
//  This file is part of the Robust Services Core (RSC).
//
//  RSC is free software: you can redistribute it and/or modify it under the
//  terms of the GNU General Public License as published by the Free Software
//  Foundation, either version 3 of the License, or (at your option) any later
//  version.
//
//  RSC is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
//  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
//  details.
//
//  You should have received a copy of the GNU General Public License along
//  with RSC.  If not, see <http://www.gnu.org/licenses/>.
//
#ifndef POTSCWTSERVICE_H_INCLUDED
#define POTSCWTSERVICE_H_INCLUDED

#include "Initiator.h"
#include "PotsProtocol.h"
#include "Service.h"
#include "NbTypes.h"

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
   EventHandler::Rc ProcessEvent(const ServiceSM& parentSsm,
      Event& currEvent, Event*& nextEvent) const override;
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

   PotsCwaService();
   ~PotsCwaService();
   ServiceSM* AllocModifier() const override;
};

class PotsCwbService : public PotsCwtService
{
   friend class Singleton< PotsCwbService >;

   PotsCwbService();
   ~PotsCwbService();
   ServiceSM* AllocModifier() const override;
};

class PotsCwmService : public Service
{
   friend class Singleton< PotsCwmService >;

   PotsCwmService();
   ~PotsCwmService();
   ServiceSM* AllocModifier() const override;
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
