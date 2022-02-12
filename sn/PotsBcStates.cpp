//==============================================================================
//
//  PotsBcStates.cpp
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
#include "PotsSessions.h"
#include "SbAppIds.h"

//------------------------------------------------------------------------------

namespace PotsBase
{
PotsBcNull::PotsBcNull() :
   ProxyBcNull(PotsCallServiceId) { }

PotsBcAuthorizingOrigination::PotsBcAuthorizingOrigination() :
   ProxyBcAuthorizingOrigination(PotsCallServiceId) { }

PotsBcCollectingInformation::PotsBcCollectingInformation() :
   ProxyBcCollectingInformation(PotsCallServiceId) { }

PotsBcAnalyzingInformation::PotsBcAnalyzingInformation() :
   ProxyBcAnalyzingInformation(PotsCallServiceId) { }

PotsBcSelectingRoute::PotsBcSelectingRoute() :
   ProxyBcSelectingRoute(PotsCallServiceId) { }

PotsBcAuthorizingCallSetup::PotsBcAuthorizingCallSetup() :
   ProxyBcAuthorizingCallSetup(PotsCallServiceId) { }

PotsBcSendingCall::PotsBcSendingCall() :
   ProxyBcSendingCall(PotsCallServiceId) { }

PotsBcOrigAlerting::PotsBcOrigAlerting() :
   ProxyBcOrigAlerting(PotsCallServiceId) { }

PotsBcAuthorizingTermination::PotsBcAuthorizingTermination() :
   ProxyBcAuthorizingTermination(PotsCallServiceId) { }

PotsBcSelectingFacility::PotsBcSelectingFacility() :
   ProxyBcSelectingFacility(PotsCallServiceId) { }

PotsBcPresentingCall::PotsBcPresentingCall() :
   ProxyBcPresentingCall(PotsCallServiceId) { }

PotsBcTermAlerting::PotsBcTermAlerting() :
   ProxyBcTermAlerting(PotsCallServiceId) { }

PotsBcActive::PotsBcActive() :
   ProxyBcActive(PotsCallServiceId) { }

PotsBcLocalSuspending::PotsBcLocalSuspending() :
   ProxyBcLocalSuspending(PotsCallServiceId) { }

PotsBcRemoteSuspending::PotsBcRemoteSuspending() :
   ProxyBcRemoteSuspending(PotsCallServiceId) { }

PotsBcException::PotsBcException() :
   ProxyBcException(PotsCallServiceId) { }
}
