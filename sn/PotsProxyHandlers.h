//==============================================================================
//
//  PotsProxyHandlers.h
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
#ifndef POTSPROXYHANDLERS_H_INCLUDED
#define POTSPROXYHANDLERS_H_INCLUDED

#include "EventHandler.h"
#include "NbTypes.h"

using namespace NodeBase;
using namespace SessionBase;

//------------------------------------------------------------------------------

namespace PotsBase
{
class PotsProxyNuAnalyzeLocalMessage : public EventHandler
{
   friend class Singleton< PotsProxyNuAnalyzeLocalMessage >;
protected:
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsProxyNuAnalyzeLocalMessage() = default;
};

class PotsProxyNuOriginate : public EventHandler
{
   friend class Singleton< PotsProxyNuOriginate >;
protected:
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsProxyNuOriginate() = default;
};

//------------------------------------------------------------------------------

class PotsProxyCiCollectInformation : public EventHandler
{
   friend class Singleton< PotsProxyCiCollectInformation >;
protected:
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsProxyCiCollectInformation() = default;
};

//------------------------------------------------------------------------------

class PotsProxyScAnalyzeLocalMessage : public EventHandler
{
   friend class Singleton< PotsProxyScAnalyzeLocalMessage >;
protected:
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsProxyScAnalyzeLocalMessage() = default;
};

class PotsProxyScSendCall : public EventHandler
{
   friend class Singleton< PotsProxyScSendCall >;
protected:
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsProxyScSendCall() = default;
};

class PotsProxyScRemoteProgress : public EventHandler
{
   friend class Singleton< PotsProxyScRemoteProgress >;
protected:
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsProxyScRemoteProgress() = default;
};

class PotsProxyScRemoteAlerting : public EventHandler
{
   friend class Singleton< PotsProxyScRemoteAlerting >;
protected:
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsProxyScRemoteAlerting() = default;
};

//------------------------------------------------------------------------------

class PotsProxyPcAnalyzeLocalMessage : public EventHandler
{
   friend class Singleton< PotsProxyPcAnalyzeLocalMessage >;
protected:
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsProxyPcAnalyzeLocalMessage() = default;
};

class PotsProxyPcLocalProgress : public EventHandler
{
   friend class Singleton< PotsProxyPcLocalProgress >;
protected:
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsProxyPcLocalProgress() = default;
};

//------------------------------------------------------------------------------

class PotsProxyTaAnalyzeLocalMessage : public EventHandler
{
   friend class Singleton< PotsProxyTaAnalyzeLocalMessage >;
protected:
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsProxyTaAnalyzeLocalMessage() = default;
};

//------------------------------------------------------------------------------

class PotsProxyAcAnalyzeLocalMessage : public EventHandler
{
   friend class Singleton< PotsProxyAcAnalyzeLocalMessage >;
protected:
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsProxyAcAnalyzeLocalMessage() = default;
};

class PotsProxyAcLocalSuspend : public EventHandler
{
   friend class Singleton< PotsProxyAcLocalSuspend >;
protected:
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsProxyAcLocalSuspend() = default;
};

class PotsProxyAcRemoteSuspend : public EventHandler
{
   friend class Singleton< PotsProxyAcRemoteSuspend >;
protected:
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsProxyAcRemoteSuspend() = default;
};

//------------------------------------------------------------------------------

class PotsProxyLsLocalResume : public EventHandler
{
   friend class Singleton< PotsProxyLsLocalResume >;
protected:
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsProxyLsLocalResume() = default;
};

//------------------------------------------------------------------------------

class PotsProxyRsRemoteResume : public EventHandler
{
   friend class Singleton< PotsProxyRsRemoteResume >;
protected:
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsProxyRsRemoteResume() = default;
};

//------------------------------------------------------------------------------

class PotsProxyLocalAlerting : public EventHandler
{
   friend class Singleton< PotsProxyLocalAlerting >;
protected:
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsProxyLocalAlerting() = default;
};

class PotsProxyLocalAnswer : public EventHandler
{
   friend class Singleton< PotsProxyLocalAnswer >;
protected:
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsProxyLocalAnswer() = default;
};

class PotsProxyRemoteAnswer : public EventHandler
{
   friend class Singleton< PotsProxyRemoteAnswer >;
protected:
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsProxyRemoteAnswer() = default;
};

class PotsProxyLocalRelease : public EventHandler
{
   friend class Singleton< PotsProxyLocalRelease >;
protected:
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsProxyLocalRelease() = default;
};

class PotsProxyRemoteRelease : public EventHandler
{
   friend class Singleton< PotsProxyRemoteRelease >;
protected:
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsProxyRemoteRelease() = default;
};

class PotsProxyReleaseCall : public EventHandler
{
   friend class Singleton< PotsProxyReleaseCall >;
protected:
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsProxyReleaseCall() = default;
};
}
#endif
