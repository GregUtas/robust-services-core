//==============================================================================
//
//  PotsProxySessions.h
//
//  Copyright (C) 2013-2022  Greg Utas
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
using namespace NodeBase;

//------------------------------------------------------------------------------

namespace PotsBase
{
//  POTS proxy call service.
//
class PotsProxyService : public ProxyBcService
{
   friend class Singleton< PotsProxyService >;

   //  Private because this is a singleton.  Registers all
   //  POTS states, event handlers, and triggers.
   //
   PotsProxyService();

   //  Private because this is a singleton.
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

   PotsProxyNull();
   ~PotsProxyNull() = default;
};

class PotsProxyAuthorizingOrigination : public ProxyBcAuthorizingOrigination
{
   friend class Singleton< PotsProxyAuthorizingOrigination >;

   PotsProxyAuthorizingOrigination();
   ~PotsProxyAuthorizingOrigination() = default;
};

class PotsProxyCollectingInformation : public ProxyBcCollectingInformation
{
   friend class Singleton< PotsProxyCollectingInformation >;

   PotsProxyCollectingInformation();
   ~PotsProxyCollectingInformation() = default;
};

class PotsProxyAnalyzingInformation : public ProxyBcAnalyzingInformation
{
   friend class Singleton< PotsProxyAnalyzingInformation >;

   PotsProxyAnalyzingInformation();
   ~PotsProxyAnalyzingInformation() = default;
};

class PotsProxySelectingRoute : public ProxyBcSelectingRoute
{
   friend class Singleton< PotsProxySelectingRoute >;

   PotsProxySelectingRoute();
   ~PotsProxySelectingRoute() = default;
};

class PotsProxyAuthorizingCallSetup : public ProxyBcAuthorizingCallSetup
{
   friend class Singleton< PotsProxyAuthorizingCallSetup >;

   PotsProxyAuthorizingCallSetup();
   ~PotsProxyAuthorizingCallSetup() = default;
};

class PotsProxySendingCall : public ProxyBcSendingCall
{
   friend class Singleton< PotsProxySendingCall >;

   PotsProxySendingCall();
   ~PotsProxySendingCall() = default;
};

class PotsProxyOrigAlerting : public ProxyBcOrigAlerting
{
   friend class Singleton< PotsProxyOrigAlerting >;

   PotsProxyOrigAlerting();
   ~PotsProxyOrigAlerting() = default;
};

class PotsProxyAuthorizingTermination : public ProxyBcAuthorizingTermination
{
   friend class Singleton< PotsProxyAuthorizingTermination >;

   PotsProxyAuthorizingTermination();
   ~PotsProxyAuthorizingTermination() = default;
};

class PotsProxySelectingFacility : public ProxyBcSelectingFacility
{
   friend class Singleton< PotsProxySelectingFacility >;

   PotsProxySelectingFacility();
   ~PotsProxySelectingFacility() = default;
};

class PotsProxyPresentingCall : public ProxyBcPresentingCall
{
   friend class Singleton< PotsProxyPresentingCall >;

   PotsProxyPresentingCall();
   ~PotsProxyPresentingCall() = default;
};

class PotsProxyTermAlerting : public ProxyBcTermAlerting
{
   friend class Singleton< PotsProxyTermAlerting >;

   PotsProxyTermAlerting();
   ~PotsProxyTermAlerting() = default;
};

class PotsProxyActive : public ProxyBcActive
{
   friend class Singleton< PotsProxyActive >;

   PotsProxyActive();
   ~PotsProxyActive() = default;
};

class PotsProxyLocalSuspending : public ProxyBcLocalSuspending
{
   friend class Singleton< PotsProxyLocalSuspending >;

   PotsProxyLocalSuspending();
   ~PotsProxyLocalSuspending() = default;
};

class PotsProxyRemoteSuspending : public ProxyBcRemoteSuspending
{
   friend class Singleton< PotsProxyRemoteSuspending >;

   PotsProxyRemoteSuspending();
   ~PotsProxyRemoteSuspending() = default;
};

class PotsProxyException : public ProxyBcException
{
   friend class Singleton< PotsProxyException >;

   PotsProxyException();
   ~PotsProxyException() = default;
};
}
#endif
