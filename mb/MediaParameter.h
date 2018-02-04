//==============================================================================
//
//  MediaParameter.h
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
#ifndef MEDIAPARAMETER_H_INCLUDED
#define MEDIAPARAMETER_H_INCLUDED

#include "TlvIntParameter.h"
#include <iosfwd>
#include <string>
#include "SbTypes.h"
#include "Switch.h"

using namespace SessionBase;

//------------------------------------------------------------------------------

namespace MediaBase
{
//  To set up a media stream, two endpoints exchange their media addresses.
//
struct MediaInfo
{
public:
   //  Sets rxFrom to the address for silent tone.
   //
   MediaInfo();

   //  Outputs the struct in STREAM, preceded by INDENT spaces.
   //
   void Display(std::ostream& stream, const std::string& prefix) const;

   //  Returns true if all fields in both addresses match.
   //
   bool operator==(const MediaInfo& that) const;

   //  Returns the inverse of the == operator.
   //
   bool operator!=(const MediaInfo& that) const;

   //  The media address.
   //
   Switch::PortId rxFrom;
};

//------------------------------------------------------------------------------
//
//  Virtual base class for supporting a MediaInfo parameter.
//
class MediaParameter : public TlvIntParameter< Switch::PortId >
{
protected:
   //  Protected because this class is virtual.
   //
   MediaParameter(ProtocolId prid, Id pid);

   //  Protected because subclasses should be singletons.
   //
   virtual ~MediaParameter();

   //  Overridden to invoke MediaInfo::Display.
   //
   virtual void DisplayMsg(std::ostream& stream, const std::string& prefix,
      const byte_t* bytes, size_t count) const override;

   //  Overridden to create a CLI parameter for MediaInfo.
   //
   virtual CliParm* CreateCliParm(Usage use) const override;
};
}
#endif
