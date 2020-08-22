//==============================================================================
//
//  PotsCfnFeature.cpp
//
//  Copyright (C) 2013-2020  Greg Utas
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
#include "PotsCfnFeature.h"
#include "CliIntParm.h"
#include "CliText.h"
#include <sstream>
#include <string>
#include "CliCommand.h"
#include "CliThread.h"
#include "Debug.h"
#include "Formatters.h"
#include "FunctionGuard.h"
#include "PotsCliParms.h"
#include "PotsProfile.h"
#include "PotsProfileRegistry.h"
#include "PotsProtocol.h"
#include "Singleton.h"
#include "SysTypes.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace PotsBase
{
class PotsCfnTimerOptParm : public CliIntParm
{
public: PotsCfnTimerOptParm();
};

class PotsCfnAttrs : public CliText
{
public: PotsCfnAttrs();
};

//------------------------------------------------------------------------------

fixed_string PotsCfnTimerOptExpl = "timeout (default=30)";
fixed_string PotsCfnTimerTag = "to";

PotsCfnTimerOptParm::PotsCfnTimerOptParm() :
   CliIntParm(PotsCfnTimerOptExpl, 2*PotsProtocol::RingingCycleTime,
   7*PotsProtocol::RingingCycleTime, true, PotsCfnTimerTag) { }

fixed_string PotsCfnAbbrName = "cfn";
fixed_string PotsCfnFullName = "Call Forwarding No Answer";

PotsCfnAttrs::PotsCfnAttrs() : CliText(PotsCfnFullName, PotsCfnAbbrName)
{
   BindParm(*new DnTagParm);
   BindParm(*new PotsCfnTimerOptParm);
}

//==============================================================================

fn_name PotsCfnFeature_ctor = "PotsCfnFeature.ctor";

PotsCfnFeature::PotsCfnFeature() :
   PotsFeature(CFN, true, PotsCfnAbbrName, PotsCfnFullName)
{
   Debug::ft(PotsCfnFeature_ctor);

   SetIncompatible(BOC);
   SetIncompatible(BIC);
}

//------------------------------------------------------------------------------

fn_name PotsCfnFeature_dtor = "PotsCfnFeature.dtor";

PotsCfnFeature::~PotsCfnFeature()
{
   Debug::ftnt(PotsCfnFeature_dtor);
}

//------------------------------------------------------------------------------

CliText* PotsCfnFeature::Attrs() const { return new PotsCfnAttrs; }

//------------------------------------------------------------------------------

fn_name PotsCfnFeature_Subscribe = "PotsCfnFeature.Subscribe";

PotsFeatureProfile* PotsCfnFeature::Subscribe
   (PotsProfile& profile, CliThread& cli) const
{
   Debug::ft(PotsCfnFeature_Subscribe);

   word dn, timeout = 0;
   bool dnwarn = false, towarn = false;

   if(cli.Command()->GetIntParmRc(dn, cli) == CliParm::Ok)
   {
      auto reg = Singleton< PotsProfileRegistry >::Instance();

      dnwarn = (reg->Profile(dn) == nullptr);

      if(cli.Command()->GetIntParmRc(timeout, cli) != CliParm::Ok)
      {
         timeout = 0;
         towarn = true;
      }

      if(!cli.EndOfInput()) return nullptr;
      if(dnwarn) *cli.obuf << spaces(2) << UnregisteredDnWarning << CRLF;
      if(towarn) *cli.obuf << spaces(2) << DefaultTimeoutWarning << CRLF;
   }
   else
   {
      dn = PotsProfile::NilDN;
      *cli.obuf << spaces(2) << NoDestinationWarning << CRLF;
   }

   FunctionGuard guard(Guard_MemUnprotect);
   return new PotsCfnFeatureProfile(dn, timeout);
}

//==============================================================================

fn_name PotsCfnFeatureProfile_ctor = "PotsCfnFeatureProfile.ctor";

PotsCfnFeatureProfile::PotsCfnFeatureProfile(Address::DN dn, secs_t timeout) :
   DnRouteFeatureProfile(CFN, dn),
   timeout_(timeout)
{
   Debug::ft(PotsCfnFeatureProfile_ctor);

   if(timeout_ == 0) timeout_ = 5 * PotsProtocol::RingingCycleTime;
}

//------------------------------------------------------------------------------

fn_name PotsCfnFeatureProfile_dtor = "PotsCfnFeatureProfile.dtor";

PotsCfnFeatureProfile::~PotsCfnFeatureProfile()
{
   Debug::ftnt(PotsCfnFeatureProfile_dtor);
}

//------------------------------------------------------------------------------

fn_name PotsCfnFeatureProfile_Activate = "PotsCfnFeatureProfile.Activate";

bool PotsCfnFeatureProfile::Activate(PotsProfile& profile, CliThread& cli)
{
   Debug::ft(PotsCfnFeatureProfile_Activate);

   word timeout;

   if(DnRouteFeatureProfile::Activate(profile, cli))
   {
      if(cli.Command()->GetIntParmRc(timeout, cli) == CliParm::Ok)
      {
         FunctionGuard guard(Guard_MemUnprotect);
         timeout_ = timeout;
      }
      if(!cli.EndOfInput()) return false;

      auto reg = Singleton< PotsProfileRegistry >::Instance();

      if(reg->Profile(GetDN()) == nullptr)
      {
         *cli.obuf << spaces(2) << UnregisteredDnWarning << CRLF;
      }
      return true;
   }

   return false;
}

//------------------------------------------------------------------------------

void PotsCfnFeatureProfile::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   DnRouteFeatureProfile::Display(stream, prefix, options);

   stream << prefix << "timeout : " << int(timeout_) << CRLF;
}
}
