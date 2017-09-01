//==============================================================================
//
//  PotsCliParms.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "PotsCliParms.h"
#include "PotsProfile.h"

//------------------------------------------------------------------------------

namespace PotsBase
{
fixed_string AlreadyRegistered      = "That DN is already registered.";
fixed_string AlreadySubscribed      = "That feature is already subscribed.";
fixed_string DefaultTimeoutWarning  = "WARNING: Default timeout used.";
fixed_string FeatureNotInstalled    = "That feature is not installed.";
fixed_string IllegalScanChar        = "Illegal scan character: ";
fixed_string IncompatibleFeature    = "Incompatible with subscribed feature ";
fixed_string InvalidDestination     = "The destination DN is invalid.";
fixed_string NoCircuitExpl          = "That DN is not equipped with a circuit.";
fixed_string NoCircuitsExpl         = "There were no circuits to display.";
fixed_string NoDestinationWarning   = "WARNING: No destination DN. Feature is inactive.";
fixed_string NoDnsExpl              = "There were no DNs to display.";
fixed_string NoFeatureExpl          = "There is no feature with that identifier.";
fixed_string NoToneExpl             = "There is no tone with that identifier.";
fixed_string NoTreatmentExpl        = "There is no treatment queue with that identifier.";
fixed_string NotRegisteredExpl      = "That DN is not registered.";
fixed_string NotSubscribedExpl      = "That feature is not subscribed.";
fixed_string UnregisteredDnWarning  = "WARNING: The destination DN is not registered.";

//------------------------------------------------------------------------------

fixed_string DnExpl = "DN";

DnMandParm::DnMandParm() :
   CliIntParm(DnExpl, PotsProfile::FirstDN, PotsProfile::LastDN) { }

DnOptParm::DnOptParm() :
   CliIntParm(DnExpl, PotsProfile::FirstDN, PotsProfile::LastDN, true) { }

fixed_string DnTag = "dn";
fixed_string DnTagExpl = "DN (must be valid to activate feature)";

DnTagParm::DnTagParm() : CliIntParm(DnTagExpl,
   PotsProfile::FirstDN, PotsProfile::LastDN, true, DnTag) { }
}