//==============================================================================
//
//  PotsBcHandlers.h
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
#ifndef POTSBCHANDLERS_H_INCLUDED
#define POTSBCHANDLERS_H_INCLUDED

#include "EventHandler.h"
#include "NbTypes.h"

using namespace NodeBase;
using namespace SessionBase;

//------------------------------------------------------------------------------

namespace PotsBase
{
class PotsBcNuAnalyzeLocalMessage : public EventHandler
{
   friend class Singleton< PotsBcNuAnalyzeLocalMessage >;
protected:
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcNuAnalyzeLocalMessage() = default;
   ~PotsBcNuAnalyzeLocalMessage() = default;
};

class PotsBcNuOriginate : public EventHandler
{
   friend class Singleton< PotsBcNuOriginate >;
protected:
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcNuOriginate() = default;
   ~PotsBcNuOriginate() = default;
};

//------------------------------------------------------------------------------

class PotsBcAoAnalyzeLocalMessage : public EventHandler
{
   friend class Singleton< PotsBcAoAnalyzeLocalMessage >;
protected:
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcAoAnalyzeLocalMessage() = default;
   ~PotsBcAoAnalyzeLocalMessage() = default;
};

class PotsBcAoAuthorizeOrigination : public EventHandler
{
   friend class Singleton< PotsBcAoAuthorizeOrigination >;
protected:
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcAoAuthorizeOrigination() = default;
   ~PotsBcAoAuthorizeOrigination() = default;
};

class PotsBcAoOriginationDenied : public EventHandler
{
   friend class Singleton< PotsBcAoOriginationDenied >;
protected:
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcAoOriginationDenied() = default;
   ~PotsBcAoOriginationDenied() = default;
};

//------------------------------------------------------------------------------

class PotsBcCiAnalyzeLocalMessage : public EventHandler
{
   friend class Singleton< PotsBcCiAnalyzeLocalMessage >;
protected:
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcCiAnalyzeLocalMessage() = default;
   ~PotsBcCiAnalyzeLocalMessage() = default;
};

class PotsBcCiCollectInformation : public EventHandler
{
   friend class Singleton< PotsBcCiCollectInformation >;
protected:
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcCiCollectInformation() = default;
   ~PotsBcCiCollectInformation() = default;
};

class PotsBcCiLocalInformation : public EventHandler
{
   friend class Singleton< PotsBcCiLocalInformation >;
protected:
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcCiLocalInformation() = default;
   ~PotsBcCiLocalInformation() = default;
};

class PotsBcCiCollectionTimeout : public EventHandler
{
   friend class Singleton< PotsBcCiCollectionTimeout >;
protected:
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcCiCollectionTimeout() = default;
   ~PotsBcCiCollectionTimeout() = default;
};

//------------------------------------------------------------------------------

class PotsBcAiAnalyzeInformation : public EventHandler
{
   friend class Singleton< PotsBcAiAnalyzeInformation >;
protected:
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcAiAnalyzeInformation() = default;
   ~PotsBcAiAnalyzeInformation() = default;
};

class PotsBcAiInvalidInformation : public EventHandler
{
   friend class Singleton< PotsBcAiInvalidInformation >;
protected:
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcAiInvalidInformation() = default;
   ~PotsBcAiInvalidInformation() = default;
};

//------------------------------------------------------------------------------

class PotsBcSrSelectRoute : public EventHandler
{
   friend class Singleton< PotsBcSrSelectRoute >;
protected:
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcSrSelectRoute() = default;
   ~PotsBcSrSelectRoute() = default;
};

//------------------------------------------------------------------------------

class PotsBcAsAuthorizeCallSetup : public EventHandler
{
   friend class Singleton< PotsBcAsAuthorizeCallSetup >;
protected:
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcAsAuthorizeCallSetup() = default;
   ~PotsBcAsAuthorizeCallSetup() = default;
};

//------------------------------------------------------------------------------

class PotsBcScAnalyzeLocalMessage : public EventHandler
{
   friend class Singleton< PotsBcScAnalyzeLocalMessage >;
protected:
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcScAnalyzeLocalMessage() = default;
   ~PotsBcScAnalyzeLocalMessage() = default;
};

class PotsBcScSendCall : public EventHandler
{
   friend class Singleton< PotsBcScSendCall >;
protected:
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcScSendCall() = default;
   ~PotsBcScSendCall() = default;
};

class PotsBcScRemoteBusy : public EventHandler
{
   friend class Singleton< PotsBcScRemoteBusy >;
protected:
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcScRemoteBusy() = default;
   ~PotsBcScRemoteBusy() = default;
};

class PotsBcScRemoteProgress : public EventHandler
{
   friend class Singleton< PotsBcScRemoteProgress >;
protected:
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcScRemoteProgress() = default;
   ~PotsBcScRemoteProgress() = default;
};

class PotsBcScRemoteAlerting : public EventHandler
{
   friend class Singleton< PotsBcScRemoteAlerting >;
protected:
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcScRemoteAlerting() = default;
   ~PotsBcScRemoteAlerting() = default;
};

class PotsBcScRemoteRelease : public EventHandler
{
   friend class Singleton< PotsBcScRemoteRelease >;
protected:
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcScRemoteRelease() = default;
   ~PotsBcScRemoteRelease() = default;
};

//------------------------------------------------------------------------------

class PotsBcOaRemoteNoAnswer : public EventHandler
{
   friend class Singleton< PotsBcOaRemoteNoAnswer >;
protected:
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcOaRemoteNoAnswer() = default;
   ~PotsBcOaRemoteNoAnswer() = default;
};

//------------------------------------------------------------------------------

class PotsBcNuTerminate : public EventHandler
{
   friend class Singleton< PotsBcNuTerminate >;
protected:
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcNuTerminate() = default;
   ~PotsBcNuTerminate() = default;
};

//------------------------------------------------------------------------------

class PotsBcAtAuthorizeTermination : public EventHandler
{
   friend class Singleton< PotsBcAtAuthorizeTermination >;
protected:
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcAtAuthorizeTermination() = default;
   ~PotsBcAtAuthorizeTermination() = default;
};

class PotsBcAtTerminationDenied : public EventHandler
{
   friend class Singleton< PotsBcAtTerminationDenied >;
protected:
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcAtTerminationDenied() = default;
   ~PotsBcAtTerminationDenied() = default;
};

//------------------------------------------------------------------------------

class PotsBcSfAnalyzeLocalMessage : public EventHandler
{
   friend class Singleton< PotsBcSfAnalyzeLocalMessage >;
protected:
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcSfAnalyzeLocalMessage() = default;
   ~PotsBcSfAnalyzeLocalMessage() = default;
};

class PotsBcSfSelectFacility : public EventHandler
{
   friend class Singleton< PotsBcSfSelectFacility >;
protected:
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcSfSelectFacility() = default;
   ~PotsBcSfSelectFacility() = default;
};

class PotsBcSfLocalBusy : public EventHandler
{
   friend class Singleton< PotsBcSfLocalBusy >;
protected:
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcSfLocalBusy() = default;
   ~PotsBcSfLocalBusy() = default;
};

//------------------------------------------------------------------------------

class PotsBcPcAnalyzeLocalMessage : public EventHandler
{
   friend class Singleton< PotsBcPcAnalyzeLocalMessage >;
protected:
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcPcAnalyzeLocalMessage() = default;
   ~PotsBcPcAnalyzeLocalMessage() = default;
};

class PotsBcPcPresentCall : public EventHandler
{
   friend class Singleton< PotsBcPcPresentCall >;
protected:
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcPcPresentCall() = default;
   ~PotsBcPcPresentCall() = default;
};

class PotsBcPcFacilityFailure : public EventHandler
{
   friend class Singleton< PotsBcPcFacilityFailure >;
protected:
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcPcFacilityFailure() = default;
   ~PotsBcPcFacilityFailure() = default;
};

class PotsBcPcLocalAlerting : public EventHandler
{
   friend class Singleton< PotsBcPcLocalAlerting >;
protected:
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcPcLocalAlerting() = default;
   ~PotsBcPcLocalAlerting() = default;
};

class PotsBcPcRemoteRelease : public EventHandler
{
   friend class Singleton< PotsBcPcRemoteRelease >;
protected:
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcPcRemoteRelease() = default;
   ~PotsBcPcRemoteRelease() = default;
};

//------------------------------------------------------------------------------

class PotsBcTaAnalyzeLocalMessage : public EventHandler
{
   friend class Singleton< PotsBcTaAnalyzeLocalMessage >;
protected:
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcTaAnalyzeLocalMessage() = default;
   ~PotsBcTaAnalyzeLocalMessage() = default;
};

class PotsBcTaLocalNoAnswer : public EventHandler
{
   friend class Singleton< PotsBcTaLocalNoAnswer >;
protected:
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcTaLocalNoAnswer() = default;
   ~PotsBcTaLocalNoAnswer() = default;
};

class PotsBcTaRemoteRelease : public EventHandler
{
   friend class Singleton< PotsBcTaRemoteRelease >;
protected:
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcTaRemoteRelease() = default;
   ~PotsBcTaRemoteRelease() = default;
};

//------------------------------------------------------------------------------

class PotsBcAcAnalyzeLocalMessage : public EventHandler
{
   friend class Singleton< PotsBcAcAnalyzeLocalMessage >;
protected:
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcAcAnalyzeLocalMessage() = default;
   ~PotsBcAcAnalyzeLocalMessage() = default;
};

class PotsBcAcLocalSuspend : public EventHandler
{
   friend class Singleton< PotsBcAcLocalSuspend >;
protected:
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcAcLocalSuspend() = default;
   ~PotsBcAcLocalSuspend() = default;
};

class PotsBcAcRemoteSuspend : public EventHandler
{
   friend class Singleton< PotsBcAcRemoteSuspend >;
protected:
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcAcRemoteSuspend() = default;
   ~PotsBcAcRemoteSuspend() = default;
};

//------------------------------------------------------------------------------

class PotsBcLsLocalResume : public EventHandler
{
   friend class Singleton< PotsBcLsLocalResume >;
protected:
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcLsLocalResume() = default;
   ~PotsBcLsLocalResume() = default;
};

class PotsBcLsRemoteRelease : public EventHandler
{
   friend class Singleton< PotsBcLsRemoteRelease >;
protected:
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcLsRemoteRelease() = default;
   ~PotsBcLsRemoteRelease() = default;
};

//------------------------------------------------------------------------------

class PotsBcRsRemoteResume : public EventHandler
{
   friend class Singleton< PotsBcRsRemoteResume >;
protected:
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcRsRemoteResume() = default;
   ~PotsBcRsRemoteResume() = default;
};

//------------------------------------------------------------------------------

class PotsBcExAnalyzeLocalMessage : public EventHandler
{
   friend class Singleton< PotsBcExAnalyzeLocalMessage >;
protected:
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcExAnalyzeLocalMessage() = default;
   ~PotsBcExAnalyzeLocalMessage() = default;
};

class PotsBcExApplyTreatment : public EventHandler
{
   friend class Singleton< PotsBcExApplyTreatment >;
protected:
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcExApplyTreatment() = default;
   ~PotsBcExApplyTreatment() = default;
};

//------------------------------------------------------------------------------

class PotsBcLocalAnswer : public EventHandler
{
   friend class Singleton< PotsBcLocalAnswer >;
protected:
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcLocalAnswer() = default;
   ~PotsBcLocalAnswer() = default;
};

class PotsBcRemoteAnswer : public EventHandler
{
   friend class Singleton< PotsBcRemoteAnswer >;
protected:
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcRemoteAnswer() = default;
   ~PotsBcRemoteAnswer() = default;
};

class PotsBcLocalRelease : public EventHandler
{
   friend class Singleton< PotsBcLocalRelease >;
protected:
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcLocalRelease() = default;
   ~PotsBcLocalRelease() = default;
};

class PotsBcReleaseCall : public EventHandler
{
   friend class Singleton< PotsBcReleaseCall >;
protected:
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcReleaseCall() = default;
   ~PotsBcReleaseCall() = default;
};

class PotsBcReleaseUser : public EventHandler
{
   friend class Singleton< PotsBcReleaseUser >;
protected:
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsBcReleaseUser() = default;
   ~PotsBcReleaseUser() = default;
};
}
#endif
