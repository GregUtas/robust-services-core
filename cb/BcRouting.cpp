//==============================================================================
//
//  BcRouting.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "BcRouting.h"
#include <ostream>
#include "Debug.h"
#include "Factory.h"
#include "FactoryRegistry.h"
#include "Formatters.h"
#include "SbAppIds.h"
#include "Singleton.h"
#include "SysTypes.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace CallBase
{
fn_name AnalysisResult_ctor1 = "AnalysisResult.ctor";

AnalysisResult::AnalysisResult() :
   selector(Address::Invalid),
   identifier(0)
{
   Debug::ft(AnalysisResult_ctor1);
}

//------------------------------------------------------------------------------

fn_name AnalysisResult_ctor2 = "AnalysisResult.ctor(digits)";

AnalysisResult::AnalysisResult(const DigitString& ds) :
   selector(Address::Invalid),
   identifier(0)
{
   Debug::ft(AnalysisResult_ctor2);

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

fn_name RouteResult_ctor1 = "RouteResult.ctor";

RouteResult::RouteResult() :
   selector(NIL_ID),
   identifier(0)
{
   Debug::ft(RouteResult_ctor1);
}

//------------------------------------------------------------------------------

fn_name RouteResult_ctor2 = "RouteResult.ctor(analysis)";

RouteResult::RouteResult(const AnalysisResult& ar) :
   selector(NIL_ID),
   identifier(0)
{
   Debug::ft(RouteResult_ctor2);

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
   auto fac = Singleton< FactoryRegistry >::Instance()->GetFactory(selector);

   stream << prefix << "selector   : " << int(selector);
   stream << " (" << strClass(fac, false) << ')' << CRLF;
   stream << prefix << "identifier : " << identifier << CRLF;
}
}
