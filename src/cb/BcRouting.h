//==============================================================================
//
//  BcRouting.h
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
#ifndef BCROUTING_H_INCLUDED
#define BCROUTING_H_INCLUDED

#include <cstdint>
#include <iosfwd>
#include <string>
#include "BcAddress.h"
#include "SbTypes.h"

using namespace SessionBase;

//------------------------------------------------------------------------------

namespace CallBase
{
//  The result of mapping a digit string to an Address.
//
struct AnalysisResult
{
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
