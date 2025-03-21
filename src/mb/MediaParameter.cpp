//==============================================================================
//
//  MediaParameter.cpp
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
#include "MediaParameter.h"
#include <ostream>
#include "CliIntParm.h"
#include "Debug.h"
#include "Singleton.h"
#include "SysTypes.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace MediaBase
{
MediaInfo::MediaInfo() : rxFrom(Switch::SilentPort)
{
   Debug::ft("MediaInfo.ctor");
}

//------------------------------------------------------------------------------

void MediaInfo::Display(ostream& stream, const string& prefix) const
{
   auto tsw = Singleton<Switch>::Instance();

   stream << prefix << "rxFrom : " << rxFrom;
   stream << " (" << tsw->CircuitName(rxFrom) << ')' << CRLF;
}

//------------------------------------------------------------------------------

bool MediaInfo::operator==(const MediaInfo& that) const
{
   return (rxFrom == that.rxFrom);
}

//------------------------------------------------------------------------------

bool MediaInfo::operator!=(const MediaInfo& that) const
{
   return !(*this == that);
}

//==============================================================================

MediaParameter::MediaParameter(ProtocolId prid, Id pid) :
   TlvIntParameter<Switch::PortId>(prid, pid)
{
   Debug::ft("MediaParameter.ctor");
}

//------------------------------------------------------------------------------

MediaParameter::~MediaParameter()
{
   Debug::ftnt("MediaParameter.dtor");
}

//------------------------------------------------------------------------------

fixed_string MediaParmExpl = "media.rxFrom: Switch::PortId";
fixed_string MediaTag = "m";

CliParm* MediaParameter::CreateCliParm(Usage use) const
{
   return (use == Mandatory ?
      new CliIntParm(MediaParmExpl, 0, Switch::MaxPortId) :
      new CliIntParm(MediaParmExpl, 0, Switch::MaxPortId, true, MediaTag));
}

//------------------------------------------------------------------------------

void MediaParameter::DisplayMsg(ostream& stream,
   const string& prefix, const byte_t* bytes, size_t count) const
{
   reinterpret_cast<const MediaInfo*>(bytes)->Display(stream, prefix);
}
}
