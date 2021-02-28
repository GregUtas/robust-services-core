//==============================================================================
//
//  PotsCfxService.h
//
//  Copyright (C) 2013-2020  Greg Utas
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
#ifndef POTSCFXSERVICE_H_INCLUDED
#define POTSCFXSERVICE_H_INCLUDED

#include "Service.h"
#include "ServiceSM.h"
#include "BcCause.h"
#include "EventHandler.h"
#include "NbTypes.h"
#include "SbTypes.h"

namespace CallBase
{
   class CipMessage;
}

namespace PotsBase
{
   class DnRouteFeatureProfile;
}

using namespace NodeBase;
using namespace SessionBase;
using namespace CallBase;

//------------------------------------------------------------------------------

namespace PotsBase
{
class PotsCfxService : public Service
{
   friend class Singleton< PotsCfxService >;

   PotsCfxService();
   ~PotsCfxService();
};

class PotsCfxSsm : public ServiceSM
{
public:
   explicit PotsCfxSsm(ServiceId sid);
   EventHandler::Rc ForwardCall(Event*& nextEvent);
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;
protected:
   virtual ~PotsCfxSsm();
   ServicePortId CalcPort(const AnalyzeMsgEvent& ame) override;
   EventHandler::Rc ProcessSap(Event& currEvent, Event*& nextEvent) override;
   EventHandler::Rc ProcessSnp(Event& currEvent, Event*& nextEvent) override;
private:
   void SetProfile(DnRouteFeatureProfile* cfxp) { cfxp_ = cfxp; }
   DnRouteFeatureProfile* Profile() const { return cfxp_; }
   EventHandler::Rc ReleaseCall
      (Event*& nextEvent, Cause::Ind cause, CipMessage* msg);
   void Cancel();
   EventHandler::Rc ProcessInitAck
      (Event& currEvent, Event*& nextEvent) override;
   EventHandler::Rc ProcessInitNack
      (Event& currEvent, Event*& nextEvent) override;
   EventHandler::Rc ProcessSip(Event& currEvent, Event*& nextEvent) override;

   DnRouteFeatureProfile* cfxp_;
   bool timer_;
};
}
#endif
