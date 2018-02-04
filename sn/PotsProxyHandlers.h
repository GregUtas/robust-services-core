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

using namespace SessionBase;

//------------------------------------------------------------------------------

namespace PotsBase
{
class PotsProxyNuAnalyzeLocalMessage : public EventHandler
{
   friend class Singleton< PotsProxyNuAnalyzeLocalMessage >;
protected:
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsProxyNuAnalyzeLocalMessage() { }
};

class PotsProxyNuOriginate : public EventHandler
{
   friend class Singleton< PotsProxyNuOriginate >;
protected:
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsProxyNuOriginate() { }
};

//------------------------------------------------------------------------------

class PotsProxyCiCollectInformation : public EventHandler
{
   friend class Singleton< PotsProxyCiCollectInformation >;
protected:
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsProxyCiCollectInformation() { }
};

//------------------------------------------------------------------------------

class PotsProxyScAnalyzeLocalMessage : public EventHandler
{
   friend class Singleton< PotsProxyScAnalyzeLocalMessage >;
protected:
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsProxyScAnalyzeLocalMessage() { }
};

class PotsProxyScSendCall : public EventHandler
{
   friend class Singleton< PotsProxyScSendCall >;
protected:
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsProxyScSendCall() { }
};

class PotsProxyScRemoteProgress : public EventHandler
{
   friend class Singleton< PotsProxyScRemoteProgress >;
protected:
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsProxyScRemoteProgress() { }
};

class PotsProxyScRemoteAlerting : public EventHandler
{
   friend class Singleton< PotsProxyScRemoteAlerting >;
protected:
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsProxyScRemoteAlerting() { }
};

//------------------------------------------------------------------------------

class PotsProxyPcAnalyzeLocalMessage : public EventHandler
{
   friend class Singleton< PotsProxyPcAnalyzeLocalMessage >;
protected:
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsProxyPcAnalyzeLocalMessage() { }
};

class PotsProxyPcLocalProgress : public EventHandler
{
   friend class Singleton< PotsProxyPcLocalProgress >;
protected:
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsProxyPcLocalProgress() { }
};

//------------------------------------------------------------------------------

class PotsProxyTaAnalyzeLocalMessage : public EventHandler
{
   friend class Singleton< PotsProxyTaAnalyzeLocalMessage >;
protected:
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsProxyTaAnalyzeLocalMessage() { }
};

//------------------------------------------------------------------------------

class PotsProxyAcAnalyzeLocalMessage : public EventHandler
{
   friend class Singleton< PotsProxyAcAnalyzeLocalMessage >;
protected:
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsProxyAcAnalyzeLocalMessage() { }
};

class PotsProxyAcLocalSuspend : public EventHandler
{
   friend class Singleton< PotsProxyAcLocalSuspend >;
protected:
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsProxyAcLocalSuspend() { }
};

class PotsProxyAcRemoteSuspend : public EventHandler
{
   friend class Singleton< PotsProxyAcRemoteSuspend >;
protected:
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsProxyAcRemoteSuspend() { }
};

//------------------------------------------------------------------------------

class PotsProxyLsLocalResume : public EventHandler
{
   friend class Singleton< PotsProxyLsLocalResume >;
protected:
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsProxyLsLocalResume() { }
};

//------------------------------------------------------------------------------

class PotsProxyRsRemoteResume : public EventHandler
{
   friend class Singleton< PotsProxyRsRemoteResume >;
protected:
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsProxyRsRemoteResume() { }
};

//------------------------------------------------------------------------------

class PotsProxyLocalAlerting : public EventHandler
{
   friend class Singleton< PotsProxyLocalAlerting >;
protected:
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsProxyLocalAlerting() { }
};

class PotsProxyLocalAnswer : public EventHandler
{
   friend class Singleton< PotsProxyLocalAnswer >;
protected:
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsProxyLocalAnswer() { }
};

class PotsProxyRemoteAnswer : public EventHandler
{
   friend class Singleton< PotsProxyRemoteAnswer >;
protected:
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsProxyRemoteAnswer() { }
};

class PotsProxyLocalRelease : public EventHandler
{
   friend class Singleton< PotsProxyLocalRelease >;
protected:
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsProxyLocalRelease() { }
};

class PotsProxyRemoteRelease : public EventHandler
{
   friend class Singleton< PotsProxyRemoteRelease >;
protected:
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsProxyRemoteRelease() { }
};

class PotsProxyReleaseCall : public EventHandler
{
   friend class Singleton< PotsProxyReleaseCall >;
protected:
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
private:
   PotsProxyReleaseCall() { }
};
}
#endif
