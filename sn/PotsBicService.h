//==============================================================================
//
//  PotsBicService.h
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
#ifndef POTSBICSERVICE_H_INCLUDED
#define POTSBICSERVICE_H_INCLUDED

#include "Initiator.h"
#include "Service.h"
#include "NbTypes.h"

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
   EventHandler::Rc ProcessEvent(const ServiceSM& parentSsm,
      Event& currEvent, Event*& nextEvent) const override;
};

class PotsBicService : public Service
{
   friend class Singleton< PotsBicService >;

   PotsBicService();
   ~PotsBicService();
   ServiceSM* AllocModifier() const override;
};
}
#endif
