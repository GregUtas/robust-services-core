//==============================================================================
//
//  PotsCfxService.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef POTSCFXSERVICE_H_INCLUDED
#define POTSCFXSERVICE_H_INCLUDED

#include "BcCause.h"
#include "EventHandler.h"
#include "NbTypes.h"
#include "SbTypes.h"
#include "Service.h"
#include "ServiceSM.h"

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
private:
   PotsCfxService();
   ~PotsCfxService();
};

class PotsCfxSsm : public ServiceSM
{
public:
   explicit PotsCfxSsm(ServiceId sid);
   EventHandler::Rc ForwardCall(Event*& nextEvent);
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;
protected:
   virtual ~PotsCfxSsm();
   virtual ServicePortId CalcPort(const AnalyzeMsgEvent& ame) override;
   virtual EventHandler::Rc ProcessSap
      (Event& currEvent, Event*& nextEvent) override;
   virtual EventHandler::Rc ProcessSnp
      (Event& currEvent, Event*& nextEvent) override;
private:
   void SetProfile(DnRouteFeatureProfile* cfxp) { cfxp_ = cfxp; }
   DnRouteFeatureProfile* Profile() const { return cfxp_; }
   EventHandler::Rc ReleaseCall
      (Event*& nextEvent, Cause::Ind cause, CipMessage* msg);
   void Cancel();
   virtual EventHandler::Rc ProcessInitAck
      (Event& currEvent, Event*& nextEvent) override;
   virtual EventHandler::Rc ProcessInitNack
      (Event& currEvent, Event*& nextEvent) override;
   virtual EventHandler::Rc ProcessSip
      (Event& currEvent, Event*& nextEvent) override;

   DnRouteFeatureProfile* cfxp_;
   bool timer_;
};
}
#endif
