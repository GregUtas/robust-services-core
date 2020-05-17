//==============================================================================
//
//  MediaParameter.cpp
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
#include "MediaParameter.h"
#include "CliIntParm.h"
#include <ostream>
#include "Debug.h"
#include "Singleton.h"
#include "SysTypes.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace MediaBase
{
fn_name MediaInfo_ctor = "MediaInfo.ctor";

MediaInfo::MediaInfo() : rxFrom(Switch::SilentPort)
{
   Debug::ft(MediaInfo_ctor);
}

//------------------------------------------------------------------------------

void MediaInfo::Display(ostream& stream, const string& prefix) const
{
   auto tsw = Singleton< Switch >::Instance();

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

fn_name MediaParameter_ctor = "MediaParameter.ctor";

MediaParameter::MediaParameter(ProtocolId prid, Id pid) :
   TlvIntParameter< Switch::PortId >(prid, pid)
{
   Debug::ft(MediaParameter_ctor);
}

//------------------------------------------------------------------------------

fn_name MediaParameter_dtor = "MediaParameter.dtor";

MediaParameter::~MediaParameter()
{
   Debug::ftnt(MediaParameter_dtor);
}

//------------------------------------------------------------------------------

class MediaMandParm : public CliIntParm
{
public: MediaMandParm();
};

class MediaOptParm : public CliIntParm
{
public: MediaOptParm();
};

fixed_string MediaParmExpl = "media.rxFrom: Switch::PortId";
fixed_string MediaTag = "m";

MediaMandParm::MediaMandParm() :
   CliIntParm(MediaParmExpl, 0, Switch::MaxPortId) { }

MediaOptParm::MediaOptParm() :
   CliIntParm(MediaParmExpl, 0, Switch::MaxPortId, true, MediaTag) { }

CliParm* MediaParameter::CreateCliParm(Usage use) const
{
   if(use == Mandatory) return new MediaMandParm;
   return new MediaOptParm;
}

//------------------------------------------------------------------------------

void MediaParameter::DisplayMsg(ostream& stream,
   const string& prefix, const byte_t* bytes, size_t count) const
{
   reinterpret_cast< const MediaInfo* >(bytes)->Display(stream, prefix);
}
}
