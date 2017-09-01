//==============================================================================
//
//  PotsBcStates.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "PotsSessions.h"
#include "SbAppIds.h"

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