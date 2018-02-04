//==============================================================================
//
//  PotsProxySessions.h
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
#ifndef POTSPROXYSESSIONS_H_INCLUDED
#define POTSPROXYSESSIONS_H_INCLUDED

#include "ProxyBcSessions.h"
#include "NbTypes.h"

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
