//==============================================================================
//
//  BcProgress.h
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
#ifndef BCPROGRESS_H_INCLUDED
#define BCPROGRESS_H_INCLUDED

#include "TlvIntParameter.h"
#include <cstdint>
#include <iosfwd>
#include <string>
#include "SbTypes.h"
#include "SysTypes.h"

using namespace NodeBase;
using namespace SessionBase;

//------------------------------------------------------------------------------

namespace CallBase
{
//  Progress indicators.
//
namespace Progress
{
   //  Type for progress indicators.
   //
   typedef uint8_t Ind;

   constexpr Ind NilInd = 0;          // default value
   constexpr Ind EndOfSelection = 1;  // facility for incoming call chosen
   constexpr Ind Alerting = 2;        // facility acknowledged incoming call
   constexpr Ind Suspend = 3;         // will clear call if timer expires
   constexpr Ind Resume = 4;          // resumed call before timer expired
   constexpr Ind MediaUpdate = 5;     // sending media from a new address
   constexpr Ind MaxInd = 5;          // range constant

   //  Returns a string for displaying ID.
   //
   c_string strInd(Ind ind);
}

//------------------------------------------------------------------------------
//
//  Progress indicator parameter.
//
struct ProgressInfo
{
   //  Constructs the nil instance.
   //
   ProgressInfo();

   //  Displays member variables, similar to Base::Display.
   //
   void Display(std::ostream& stream, const std::string& prefix) const;

   //  The progress indicator.
   //
   Progress::Ind progress;
};

//------------------------------------------------------------------------------
//
//  Virtual base class for supporting a ProgressInfo parameter.
//
class ProgressParameter : public TlvIntParameter<Progress::Ind>
{
protected:
   //  Protected because this class is virtual.
   //
   ProgressParameter(ProtocolId prid, Id pid);

   //  Protected because subclasses should be singletons.
   //
   virtual ~ProgressParameter();

   //  Overridden to create a CLI parameter for ProgressInfo.
   //
   CliParm* CreateCliParm(Usage use) const override;

   //  Overridden to invoke Info::Display.
   //
   void DisplayMsg(std::ostream& stream, const std::string& prefix,
      const byte_t* bytes, size_t count) const override;
};
}
#endif
