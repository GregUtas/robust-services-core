//==============================================================================
//
//  PotsProxyHandlers.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
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
