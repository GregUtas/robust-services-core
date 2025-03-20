//==============================================================================
//
//  PotsCliParms.h
//
//  Copyright (C) 2013-2025  Greg Utas
//
//  This file is part of the Robust Services Core (RSC).
//
//  RSC is free software: you can redistribute it and/or modify it under the
//  terms of the Lesser GNU General Public License as published by the Free
//  Software Foundation, either version 3 of the License, or (at your option)
//  any later version.
//
//  RSC is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
//  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
//  details.
//
//  You should have received a copy of the Lesser GNU General Public License
//  along with RSC.  If not, see <http://www.gnu.org/licenses/>.
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
