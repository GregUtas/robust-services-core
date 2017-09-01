//==============================================================================
//
//  PotsProxySessions.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef POTSPROXYSESSIONS_H_INCLUDED
#define POTSPROXYSESSIONS_H_INCLUDED

#include "ProxyBcSessions.h"
#include "NbTypes.h"

using namespace NodeBase;
using namespace CallBase;

//------------------------------------------------------------------------------

namespace PotsBase
{
//  POTS proxy call service.
//
class PotsProxyService : public ProxyBcService
{
   friend class Singleton< PotsProxyService >;
private:
   //  Private because this singleton is not subclassed.  Registers all
   //  POTS states, event handlers, and triggers.
   //
   PotsProxyService();

   //  Private because this singleton is not subclassed.
   //
   ~PotsProxyService();
};

//------------------------------------------------------------------------------
//
//  POTS basic call states.
//
class PotsProxyNull : public ProxyBcNull
{
   friend class Singleton< PotsProxyNull >;
private:
   PotsProxyNull();
   ~PotsProxyNull();
};

class PotsProxyAuthorizingOrigination : public ProxyBcAuthorizingOrigination
{
   friend class Singleton< PotsProxyAuthorizingOrigination >;
private:
   PotsProxyAuthorizingOrigination();
   ~PotsProxyAuthorizingOrigination();
};

class PotsProxyCollectingInformation : public ProxyBcCollectingInformation
{
   friend class Singleton< PotsProxyCollectingInformation >;
private:
   PotsProxyCollectingInformation();
   ~PotsProxyCollectingInformation();
};

class PotsProxyAnalyzingInformation : public ProxyBcAnalyzingInformation
{
   friend class Singleton< PotsProxyAnalyzingInformation >;
private:
   PotsProxyAnalyzingInformation();
   ~PotsProxyAnalyzingInformation();
};

class PotsProxySelectingRoute : public ProxyBcSelectingRoute
{
   friend class Singleton< PotsProxySelectingRoute >;
private:
   PotsProxySelectingRoute();
   ~PotsProxySelectingRoute();
};

class PotsProxyAuthorizingCallSetup : public ProxyBcAuthorizingCallSetup
{
   friend class Singleton< PotsProxyAuthorizingCallSetup >;
private:
   PotsProxyAuthorizingCallSetup();
   ~PotsProxyAuthorizingCallSetup();
};

class PotsProxySendingCall : public ProxyBcSendingCall
{
   friend class Singleton< PotsProxySendingCall >;
private:
   PotsProxySendingCall();
   ~PotsProxySendingCall();
};

class PotsProxyOrigAlerting : public ProxyBcOrigAlerting
{
   friend class Singleton< PotsProxyOrigAlerting >;
private:
   PotsProxyOrigAlerting();
   ~PotsProxyOrigAlerting();
};

class PotsProxyAuthorizingTermination : public ProxyBcAuthorizingTermination
{
   friend class Singleton< PotsProxyAuthorizingTermination >;
private:
   PotsProxyAuthorizingTermination();
   ~PotsProxyAuthorizingTermination();
};

class PotsProxySelectingFacility : public ProxyBcSelectingFacility
{
   friend class Singleton< PotsProxySelectingFacility >;
private:
   PotsProxySelectingFacility();
   ~PotsProxySelectingFacility();
};

class PotsProxyPresentingCall : public ProxyBcPresentingCall
{
   friend class Singleton< PotsProxyPresentingCall >;
private:
   PotsProxyPresentingCall();
   ~PotsProxyPresentingCall();
};

class PotsProxyTermAlerting : public ProxyBcTermAlerting
{
   friend class Singleton< PotsProxyTermAlerting >;
private:
   PotsProxyTermAlerting();
   ~PotsProxyTermAlerting();
};

class PotsProxyActive : public ProxyBcActive
{
   friend class Singleton< PotsProxyActive >;
private:
   PotsProxyActive();
   ~PotsProxyActive();
};

class PotsProxyLocalSuspending : public ProxyBcLocalSuspending
{
   friend class Singleton< PotsProxyLocalSuspending >;
private:
   PotsProxyLocalSuspending();
   ~PotsProxyLocalSuspending();
};

class PotsProxyRemoteSuspending : public ProxyBcRemoteSuspending
{
   friend class Singleton< PotsProxyRemoteSuspending >;
private:
   PotsProxyRemoteSuspending();
   ~PotsProxyRemoteSuspending();
};

class PotsProxyException : public ProxyBcException
{
   friend class Singleton< PotsProxyException >;
private:
   PotsProxyException();
   ~PotsProxyException();
};
}
#endif
