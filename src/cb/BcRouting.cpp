//==============================================================================
//
//  BcRouting.cpp
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
#include "BcRouting.h"
#include <ostream>
#include "Debug.h"
#include "FactoryRegistry.h"
#include "Formatters.h"
#include "Registry.h"
#include "SbAppIds.h"
#include "Singleton.h"
#include "SysTypes.h"

using namespace NodeBase;
using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace CallBase
{
AnalysisResult::AnalysisResult() :
   selector(Address::Invalid),
   identifier(0)
{
   Debug::ft("AnalysisResult.ctor");
}

//------------------------------------------------------------------------------

AnalysisResult::AnalysisResult(const DigitString& ds) :
   selector(Address::Invalid),
   identifier(0)
{
   Debug::ft("AnalysisResult.ctor(digits)");

   auto dn = ds.ToDN();

   if(dn != Address::NilDN)
   {
      selector = Address::DnType;
      identifier = dn;
      return;
   }

   auto sc = ds.ToSC();

   if(sc != Address::NilSC)
   {
      selector = Address::ScType;
      identifier = sc;
      return;
   }
}

//------------------------------------------------------------------------------

void AnalysisResult::Display(ostream& stream, const string& prefix) const
{
   stream << prefix << "selector   : " << int(selector);
   stream << " (" << selector << ')' << CRLF;
   stream << prefix << "identifier : " << identifier << CRLF;
}

//==============================================================================

RouteResult::RouteResult() :
   selector(NIL_ID),
   identifier(0)
{
   Debug::ft("RouteResult.ctor");
}

//------------------------------------------------------------------------------

RouteResult::RouteResult(const AnalysisResult& ar) :
   selector(NIL_ID),
   identifier(0)
{
   Debug::ft("RouteResult.ctor(analysis)");

   switch(ar.selector)
   {
   case Address::DnType:
      identifier = ar.identifier;

      //b Temporary until DnProfile is created as a virtual base class
      //  for PotsProfile and a new CipProfile (for testing).
      //
      if(identifier < 90000)
         selector = PotsCallFactoryId;
      else
         selector = TestCallFactoryId;
      return;
   }
}

//------------------------------------------------------------------------------

void RouteResult::Display(ostream& stream, const string& prefix) const
{
   auto fac = Singleton<FactoryRegistry>::Instance()->Factories().At(selector);

   stream << prefix << "selector   : " << int(selector);
   stream << " (" << strClass(fac, false) << ')' << CRLF;
   stream << prefix << "identifier : " << identifier << CRLF;
}
}
