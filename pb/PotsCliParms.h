//==============================================================================
//
//  PotsCliParms.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef POTSCLIPARMS_H_INCLUDED
#define POTSCLIPARMS_H_INCLUDED

#include "CliIntParm.h"
#include "SysTypes.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace PotsBase
{
//  Strings used by commands in the POTS increment.
//
extern fixed_string AlreadyRegistered;
extern fixed_string AlreadySubscribed;
extern fixed_string DefaultTimeoutWarning;
extern fixed_string FeatureNotInstalled;
extern fixed_string IllegalScanChar;
extern fixed_string IncompatibleFeature;
extern fixed_string InvalidDestination;
extern fixed_string NoCircuitExpl;
extern fixed_string NoCircuitsExpl;
extern fixed_string NoDestinationWarning;
extern fixed_string NoDnsExpl;
extern fixed_string NoFeatureExpl;
extern fixed_string NoToneExpl;
extern fixed_string NoTreatmentExpl;
extern fixed_string NotRegisteredExpl;
extern fixed_string NotSubscribedExpl;
extern fixed_string UnregisteredDnWarning;

//------------------------------------------------------------------------------
//
//  Parameter for a mandatory Address::DN.
//
class DnMandParm : public CliIntParm
{
public: DnMandParm();
};

//  Parameter for an optional Address::DN.
//
class DnOptParm : public CliIntParm
{
public: DnOptParm();
};

//  Parameter for a tagged ("dn=") Address::DN.
//
class DnTagParm : public CliIntParm
{
public: DnTagParm();
};
}
#endif
