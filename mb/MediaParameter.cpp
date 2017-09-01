//==============================================================================
//
//  MediaParameter.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
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
   Debug::ft(MediaParameter_dtor);
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
