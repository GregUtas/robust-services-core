//==============================================================================
//
//  PotsBcTriggers.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "PotsSessions.h"

//------------------------------------------------------------------------------

namespace PotsBase
{
PotsAuthorizeOriginationSap::PotsAuthorizeOriginationSap() :
   BcTrigger(AuthorizeOriginationSap) { }

PotsAuthorizeOriginationSap::~PotsAuthorizeOriginationSap() { }

//------------------------------------------------------------------------------

PotsCollectInformationSap::PotsCollectInformationSap() :
   BcTrigger(CollectInformationSap) { }

PotsCollectInformationSap::~PotsCollectInformationSap() { }

//------------------------------------------------------------------------------

PotsAuthorizeTerminationSap::PotsAuthorizeTerminationSap() :
   BcTrigger(AuthorizeTerminationSap) { }

PotsAuthorizeTerminationSap::~PotsAuthorizeTerminationSap() { }

//------------------------------------------------------------------------------

PotsLocalBusySap::PotsLocalBusySap() :
   BcTrigger(LocalBusySap) { }

PotsLocalBusySap::~PotsLocalBusySap() { }

//------------------------------------------------------------------------------

PotsLocalAlertingSnp::PotsLocalAlertingSnp() :
   BcTrigger(LocalAlertingSnp) { }

PotsLocalAlertingSnp::~PotsLocalAlertingSnp() { }
}