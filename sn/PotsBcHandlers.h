//==============================================================================
//
//  PotsBcHandlers.h
//
//  Copyright (C) 2017  Greg Utas
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
#ifndef POTSBCHANDLERS_H_INCLUDED
#define POTSBCHANDLERS_H_INCLUDED

#include "EventHandler.h"
#include "NbTypes.h"

using namespace SessionBase;

//------------------------------------------------------------------------------

namespace PotsBase
{
class PotsBcNuAnalyzeLocalMessage : public EventHandler
{
   friend class Singleton< PotsBcNuAnalyzeLocalMessage >;
protected:
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcNuAnalyzeLocalMessage() { }
};

class PotsBcNuOriginate : public EventHandler
{
   friend class Singleton< PotsBcNuOriginate >;
protected:
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcNuOriginate() { }
};

//------------------------------------------------------------------------------

class PotsBcAoAnalyzeLocalMessage : public EventHandler
{
   friend class Singleton< PotsBcAoAnalyzeLocalMessage >;
protected:
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcAoAnalyzeLocalMessage() { }
};

class PotsBcAoAuthorizeOrigination : public EventHandler
{
   friend class Singleton< PotsBcAoAuthorizeOrigination >;
protected:
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcAoAuthorizeOrigination() { }
};

class PotsBcAoOriginationDenied : public EventHandler
{
   friend class Singleton< PotsBcAoOriginationDenied >;
protected:
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcAoOriginationDenied() { }
};

//------------------------------------------------------------------------------

class PotsBcCiAnalyzeLocalMessage : public EventHandler
{
   friend class Singleton< PotsBcCiAnalyzeLocalMessage >;
protected:
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcCiAnalyzeLocalMessage() { }
};

class PotsBcCiCollectInformation : public EventHandler
{
   friend class Singleton< PotsBcCiCollectInformation >;
protected:
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcCiCollectInformation() { }
};

class PotsBcCiLocalInformation : public EventHandler
{
   friend class Singleton< PotsBcCiLocalInformation >;
protected:
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcCiLocalInformation() { }
};

class PotsBcCiCollectionTimeout : public EventHandler
{
   friend class Singleton< PotsBcCiCollectionTimeout >;
protected:
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcCiCollectionTimeout() { }
};

//------------------------------------------------------------------------------

class PotsBcAiAnalyzeInformation : public EventHandler
{
   friend class Singleton< PotsBcAiAnalyzeInformation >;
protected:
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcAiAnalyzeInformation() { }
};

class PotsBcAiInvalidInformation : public EventHandler
{
   friend class Singleton< PotsBcAiInvalidInformation >;
protected:
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcAiInvalidInformation() { }
};

//------------------------------------------------------------------------------

class PotsBcSrSelectRoute : public EventHandler
{
   friend class Singleton< PotsBcSrSelectRoute >;
protected:
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcSrSelectRoute() { }
};

//------------------------------------------------------------------------------

class PotsBcAsAuthorizeCallSetup : public EventHandler
{
   friend class Singleton< PotsBcAsAuthorizeCallSetup >;
protected:
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcAsAuthorizeCallSetup() { }
};

//------------------------------------------------------------------------------

class PotsBcScAnalyzeLocalMessage : public EventHandler
{
   friend class Singleton< PotsBcScAnalyzeLocalMessage >;
protected:
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcScAnalyzeLocalMessage() { }
};

class PotsBcScSendCall : public EventHandler
{
   friend class Singleton< PotsBcScSendCall >;
protected:
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcScSendCall() { }
};

class PotsBcScRemoteBusy : public EventHandler
{
   friend class Singleton< PotsBcScRemoteBusy >;
protected:
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcScRemoteBusy() { }
};

class PotsBcScRemoteProgress : public EventHandler
{
   friend class Singleton< PotsBcScRemoteProgress >;
protected:
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcScRemoteProgress() { }
};

class PotsBcScRemoteAlerting : public EventHandler
{
   friend class Singleton< PotsBcScRemoteAlerting >;
protected:
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcScRemoteAlerting() { }
};

class PotsBcScRemoteRelease : public EventHandler
{
   friend class Singleton< PotsBcScRemoteRelease >;
protected:
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcScRemoteRelease() { }
};

//------------------------------------------------------------------------------

class PotsBcOaRemoteNoAnswer : public EventHandler
{
   friend class Singleton< PotsBcOaRemoteNoAnswer >;
protected:
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcOaRemoteNoAnswer() { }
};

//------------------------------------------------------------------------------

class PotsBcNuTerminate : public EventHandler
{
   friend class Singleton< PotsBcNuTerminate >;
protected:
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcNuTerminate() { }
};

//------------------------------------------------------------------------------

class PotsBcAtAuthorizeTermination : public EventHandler
{
   friend class Singleton< PotsBcAtAuthorizeTermination >;
protected:
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcAtAuthorizeTermination() { }
};

class PotsBcAtTerminationDenied : public EventHandler
{
   friend class Singleton< PotsBcAtTerminationDenied >;
protected:
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcAtTerminationDenied() { }
};

//------------------------------------------------------------------------------

class PotsBcSfAnalyzeLocalMessage : public EventHandler
{
   friend class Singleton< PotsBcSfAnalyzeLocalMessage >;
protected:
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcSfAnalyzeLocalMessage() { }
};

class PotsBcSfSelectFacility : public EventHandler
{
   friend class Singleton< PotsBcSfSelectFacility >;
protected:
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcSfSelectFacility() { }
};

class PotsBcSfLocalBusy : public EventHandler
{
   friend class Singleton< PotsBcSfLocalBusy >;
protected:
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcSfLocalBusy() { }
};

//------------------------------------------------------------------------------

class PotsBcPcAnalyzeLocalMessage : public EventHandler
{
   friend class Singleton< PotsBcPcAnalyzeLocalMessage >;
protected:
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcPcAnalyzeLocalMessage() { }
};

class PotsBcPcPresentCall : public EventHandler
{
   friend class Singleton< PotsBcPcPresentCall >;
protected:
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcPcPresentCall() { }
};

class PotsBcPcFacilityFailure : public EventHandler
{
   friend class Singleton< PotsBcPcFacilityFailure >;
protected:
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcPcFacilityFailure() { }
};

class PotsBcPcLocalAlerting : public EventHandler
{
   friend class Singleton< PotsBcPcLocalAlerting >;
protected:
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcPcLocalAlerting() { }
};

class PotsBcPcRemoteRelease : public EventHandler
{
   friend class Singleton< PotsBcPcRemoteRelease >;
protected:
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcPcRemoteRelease() { }
};

//------------------------------------------------------------------------------

class PotsBcTaAnalyzeLocalMessage : public EventHandler
{
   friend class Singleton< PotsBcTaAnalyzeLocalMessage >;
protected:
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcTaAnalyzeLocalMessage() { }
};

class PotsBcTaLocalNoAnswer : public EventHandler
{
   friend class Singleton< PotsBcTaLocalNoAnswer >;
protected:
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcTaLocalNoAnswer() { }
};

class PotsBcTaRemoteRelease : public EventHandler
{
   friend class Singleton< PotsBcTaRemoteRelease >;
protected:
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcTaRemoteRelease() { }
};

//------------------------------------------------------------------------------

class PotsBcAcAnalyzeLocalMessage : public EventHandler
{
   friend class Singleton< PotsBcAcAnalyzeLocalMessage >;
protected:
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcAcAnalyzeLocalMessage() { }
};

class PotsBcAcLocalSuspend : public EventHandler
{
   friend class Singleton< PotsBcAcLocalSuspend >;
protected:
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcAcLocalSuspend() { }
};

class PotsBcAcRemoteSuspend : public EventHandler
{
   friend class Singleton< PotsBcAcRemoteSuspend >;
protected:
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcAcRemoteSuspend() { }
};

//------------------------------------------------------------------------------

class PotsBcLsLocalResume : public EventHandler
{
   friend class Singleton< PotsBcLsLocalResume >;
protected:
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcLsLocalResume() { }
};

class PotsBcLsRemoteRelease : public EventHandler
{
   friend class Singleton< PotsBcLsRemoteRelease >;
protected:
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcLsRemoteRelease() { }
};

//------------------------------------------------------------------------------

class PotsBcRsRemoteResume : public EventHandler
{
   friend class Singleton< PotsBcRsRemoteResume >;
protected:
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcRsRemoteResume() { }
};

//------------------------------------------------------------------------------

class PotsBcExAnalyzeLocalMessage : public EventHandler
{
   friend class Singleton< PotsBcExAnalyzeLocalMessage >;
protected:
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcExAnalyzeLocalMessage() { }
};

class PotsBcExApplyTreatment : public EventHandler
{
   friend class Singleton< PotsBcExApplyTreatment >;
protected:
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcExApplyTreatment() { }
};

//------------------------------------------------------------------------------

class PotsBcLocalAnswer : public EventHandler
{
   friend class Singleton< PotsBcLocalAnswer >;
protected:
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcLocalAnswer() { }
};

class PotsBcRemoteAnswer : public EventHandler
{
   friend class Singleton< PotsBcRemoteAnswer >;
protected:
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcRemoteAnswer() { }
};

class PotsBcLocalRelease : public EventHandler
{
   friend class Singleton< PotsBcLocalRelease >;
protected:
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcLocalRelease() { }
};

class PotsBcReleaseCall : public EventHandler
{
   friend class Singleton< PotsBcReleaseCall >;
protected:
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcReleaseCall() { }
};

class PotsBcReleaseUser : public EventHandler
{
   friend class Singleton< PotsBcReleaseUser >;
protected:
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcReleaseUser() { }
};
}
#endif
