//==============================================================================
//
//  BcRouting.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef BCROUTING_H_INCLUDED
#define BCROUTING_H_INCLUDED

#include "BcAddress.h"
#include <cstdint>
#include <iosfwd>
#include <string>
#include "SbTypes.h"

using namespace SessionBase;

//------------------------------------------------------------------------------

namespace CallBase
{
//  The result of mapping a digit string to an Address.
//
struct AnalysisResult
{
public:
   //  Constructs the nil instance.
   //
   AnalysisResult();

   //  Constructs a result based on DS.
   //
   explicit AnalysisResult(const DigitString& ds);

   //  Displays member variables, similar to Base::Display.
   //
   void Display(std::ostream& stream, const std::string& prefix) const;

   //  The type of address.
   //
   Address::Type selector;

   //  The actual address within SELECTOR's domain.
   //
   uint32_t identifier;
};

//------------------------------------------------------------------------------
//
//  The result of mapping an AnalysisResult to a destination that should
//  receive a session.
//
struct RouteResult
{
public:
   //  The type for identifying a destination that can receive a session.
   //
   typedef uint32_t Id;

   //  Constructs the nil instance.
   //
   RouteResult();

   //  Constructs a result based on AR.
   //
   explicit RouteResult(const AnalysisResult& ar);

   //  Displays member variables, similar to Base::Display.
   //
   void Display(std::ostream& stream, const std::string& prefix) const;

   //  The factory that will receive the session.
   //
   FactoryId selector;

   //  The destination that will receive the session.  It is interpreted
   //  in the context of SELECTOR (that is, it is factory specific).
   //
   Id identifier;
};
}
#endif
