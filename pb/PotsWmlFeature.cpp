//==============================================================================
//
//  PotsWmlFeature.cpp
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
#include "PotsWmlFeature.h"
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
class PotsWmlTimerOptParm : public CliIntParm
{
public: PotsWmlTimerOptParm();
};

class PotsWmlAttrs : public CliText
{
public: PotsWmlAttrs();
};

//------------------------------------------------------------------------------

fixed_string PotsWmlTimerOptExpl = "timeout (default=5)";
fixed_string PotsWmlTimerTag = "to";

PotsWmlTimerOptParm::PotsWmlTimerOptParm() : CliIntParm(PotsWmlTimerOptExpl, 3,
   PotsProtocol::FirstDigitTimeout - 1, true, PotsWmlTimerTag) { }

fixed_string PotsWmlAbbrName = "wml";
fixed_string PotsWmlFullName = "Warm Line";

PotsWmlAttrs::PotsWmlAttrs() : CliText(PotsWmlFullName, PotsWmlAbbrName)
{
   BindParm(*new DnTagParm);
   BindParm(*new PotsWmlTimerOptParm);
}

//==============================================================================

fn_name PotsWmlFeature_ctor = "PotsWmlFeature.ctor";

PotsWmlFeature::PotsWmlFeature() :
   PotsFeature(WML, true, PotsWmlAbbrName, PotsWmlFullName)
{
   Debug::ft(PotsWmlFeature_ctor);

   SetIncompatible(BOC);
   SetIncompatible(HTL);
}

//------------------------------------------------------------------------------

fn_name PotsWmlFeature_dtor = "PotsWmlFeature.dtor";

PotsWmlFeature::~PotsWmlFeature()
{
   Debug::ft(PotsWmlFeature_dtor);
}

//------------------------------------------------------------------------------

CliText* PotsWmlFeature::Attrs() const { return new PotsWmlAttrs; }

//------------------------------------------------------------------------------

fn_name PotsWmlFeature_Subscribe = "PotsWmlFeature.Subscribe";

PotsFeatureProfile* PotsWmlFeature::Subscribe
   (PotsProfile& profile, CliThread& cli) const
{
   Debug::ft(PotsWmlFeature_Subscribe);

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

      cli.EndOfInput(false);
      if(dnwarn) *cli.obuf << spaces(2) << UnregisteredDnWarning << CRLF;
      if(towarn) *cli.obuf << spaces(2) << DefaultTimeoutWarning << CRLF;
   }
   else
   {
      dn = PotsProfile::NilDN;
      *cli.obuf << spaces(2) << NoDestinationWarning << CRLF;
   }

   FunctionGuard guard(Guard_MemUnprotect);
   return new PotsWmlFeatureProfile(dn, timeout);
}

//==============================================================================

fn_name PotsWmlFeatureProfile_ctor = "PotsWmlFeatureProfile.ctor";

PotsWmlFeatureProfile::PotsWmlFeatureProfile(Address::DN dn, secs_t timeout) :
   DnRouteFeatureProfile(WML, dn),
   timeout_(timeout)
{
   Debug::ft(PotsWmlFeatureProfile_ctor);

   if(timeout_ == 0) timeout_ = PotsProtocol::FirstDigitTimeout - 5;
}

//------------------------------------------------------------------------------

fn_name PotsWmlFeatureProfile_dtor = "PotsWmlFeatureProfile.dtor";

PotsWmlFeatureProfile::~PotsWmlFeatureProfile()
{
   Debug::ft(PotsWmlFeatureProfile_dtor);
}

//------------------------------------------------------------------------------

fn_name PotsWmlFeatureProfile_Activate = "PotsWmlFeatureProfile.Activate";

bool PotsWmlFeatureProfile::Activate(PotsProfile& profile, CliThread& cli)
{
   Debug::ft(PotsWmlFeatureProfile_Activate);

   FunctionGuard guard(Guard_MemUnprotect);

   word timeout;
   bool dnwarn = false;

   if(DnRouteFeatureProfile::Activate(profile, cli))
   {
      auto reg = Singleton< PotsProfileRegistry >::Instance();
      dnwarn = (reg->Profile(GetDN()) == nullptr);

      if(cli.Command()->GetIntParmRc(timeout, cli) == CliParm::Ok)
      {
         timeout_ = timeout;
      }
      cli.EndOfInput(false);
      if(dnwarn) *cli.obuf << spaces(2) << UnregisteredDnWarning << CRLF;
      return true;
   }

   return false;
}

//------------------------------------------------------------------------------

void PotsWmlFeatureProfile::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   DnRouteFeatureProfile::Display(stream, prefix, options);

   stream << prefix << "timeout : " << int(timeout_) << CRLF;
}
}
