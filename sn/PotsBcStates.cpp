//==============================================================================
//
//  PotsBcStates.cpp
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
#include "PotsSessions.h"
#include "SbAppIds.h"

using namespace SessionBase;

//------------------------------------------------------------------------------

namespace PotsBase
{
PotsBcNull::PotsBcNull() :
   ProxyBcNull(PotsCallServiceId) { }

PotsBcNull::~PotsBcNull() { }

PotsBcAuthorizingOrigination::PotsBcAuthorizingOrigination() :
   ProxyBcAuthorizingOrigination(PotsCallServiceId) { }

PotsBcAuthorizingOrigination::~PotsBcAuthorizingOrigination() { }

PotsBcCollectingInformation::PotsBcCollectingInformation() :
   ProxyBcCollectingInformation(PotsCallServiceId) { }

PotsBcCollectingInformation::~PotsBcCollectingInformation() { }

PotsBcAnalyzingInformation::PotsBcAnalyzingInformation() :
   ProxyBcAnalyzingInformation(PotsCallServiceId) { }

PotsBcAnalyzingInformation::~PotsBcAnalyzingInformation() { }

PotsBcSelectingRoute::PotsBcSelectingRoute() :
   ProxyBcSelectingRoute(PotsCallServiceId) { }

PotsBcSelectingRoute::~PotsBcSelectingRoute() { }

PotsBcAuthorizingCallSetup::PotsBcAuthorizingCallSetup() :
   ProxyBcAuthorizingCallSetup(PotsCallServiceId) { }

PotsBcAuthorizingCallSetup::~PotsBcAuthorizingCallSetup() { }

PotsBcSendingCall::PotsBcSendingCall() :
   ProxyBcSendingCall(PotsCallServiceId) { }

PotsBcSendingCall::~PotsBcSendingCall() { }

PotsBcOrigAlerting::PotsBcOrigAlerting() :
   ProxyBcOrigAlerting(PotsCallServiceId) { }

PotsBcOrigAlerting::~PotsBcOrigAlerting() { }

PotsBcAuthorizingTermination::PotsBcAuthorizingTermination() :
   ProxyBcAuthorizingTermination(PotsCallServiceId) { }

PotsBcAuthorizingTermination::~PotsBcAuthorizingTermination() { }

PotsBcSelectingFacility::PotsBcSelectingFacility() :
   ProxyBcSelectingFacility(PotsCallServiceId) { }

PotsBcSelectingFacility::~PotsBcSelectingFacility() { }

PotsBcPresentingCall::PotsBcPresentingCall() :
   ProxyBcPresentingCall(PotsCallServiceId) { }

PotsBcPresentingCall::~PotsBcPresentingCall() { }

PotsBcTermAlerting::PotsBcTermAlerting() :
   ProxyBcTermAlerting(PotsCallServiceId) { }

PotsBcTermAlerting::~PotsBcTermAlerting() { }

PotsBcActive::PotsBcActive() :
   ProxyBcActive(PotsCallServiceId) { }

PotsBcActive::~PotsBcActive() { }

PotsBcLocalSuspending::PotsBcLocalSuspending() :
   ProxyBcLocalSuspending(PotsCallServiceId) { }

PotsBcLocalSuspending::~PotsBcLocalSuspending() { }

PotsBcRemoteSuspending::PotsBcRemoteSuspending() :
   ProxyBcRemoteSuspending(PotsCallServiceId) { }

PotsBcRemoteSuspending::~PotsBcRemoteSuspending() { }

PotsBcException::PotsBcException() :
   ProxyBcException(PotsCallServiceId) { }

PotsBcException::~PotsBcException() { }
}
