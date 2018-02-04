//==============================================================================
//
//  PotsMessages.cpp
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
#include "PotsProtocol.h"
#include "BcAddress.h"
#include "BcProgress.h"
#include "Debug.h"
#include "GlobalAddress.h"
#include "IpPortRegistry.h"
#include "MediaParameter.h"
#include "NwTypes.h"
#include "ProtocolSM.h"
#include "SbAppIds.h"
#include "SysTypes.h"

using namespace SessionBase;

//------------------------------------------------------------------------------

namespace PotsBase
{
fn_name PotsMessage_ctor1 = "PotsMessage.ctor(i/c)";

PotsMessage::PotsMessage(SbIpBufferPtr& buff) : TlvMessage(buff)
{
   Debug::ft(PotsMessage_ctor1);
}

//------------------------------------------------------------------------------

fn_name PotsMessage_ctor2 = "PotsMessage.ctor(o/g)";

PotsMessage::PotsMessage(ProtocolSM* psm, MsgSize size) : TlvMessage(psm, size)
{
   Debug::ft(PotsMessage_ctor2);
}

//------------------------------------------------------------------------------

fn_name PotsMessage_dtor = "PotsMessage.dtor";

PotsMessage::~PotsMessage()
{
   Debug::ft(PotsMessage_dtor);
}

//------------------------------------------------------------------------------

fn_name PotsMessage_AddCause = "PotsMessage.AddCause";

CauseInfo* PotsMessage::AddCause(const CauseInfo& cause)
{
   Debug::ft(PotsMessage_AddCause);

   return AddType(cause, PotsParameter::Cause);
}

//------------------------------------------------------------------------------

fn_name PotsMessage_AddFacility = "PotsMessage.AddFacility";

PotsFacilityInfo* PotsMessage::AddFacility(const PotsFacilityInfo& facility)
{
   Debug::ft(PotsMessage_AddFacility);

   return AddType(facility, PotsParameter::Facility);
}

//------------------------------------------------------------------------------

fn_name PotsMessage_AddHeader = "PotsMessage.AddHeader";

PotsHeaderInfo* PotsMessage::AddHeader(const PotsHeaderInfo& header)
{
   Debug::ft(PotsMessage_AddHeader);

   SetSignal(header.signal);
   return AddType(header, PotsParameter::Header);
}

//------------------------------------------------------------------------------

fn_name PotsMessage_AddMedia = "PotsMessage.AddMedia";

MediaInfo* PotsMessage::AddMedia(const MediaInfo& media)
{
   Debug::ft(PotsMessage_AddMedia);

   return AddType(media, PotsParameter::Media);
}

//------------------------------------------------------------------------------

fn_name PotsMessage_AddProgress = "PotsMessage.AddProgress";

ProgressInfo* PotsMessage::AddProgress(const ProgressInfo& progress)
{
   Debug::ft(PotsMessage_AddProgress);

   return AddType(progress, PotsParameter::Progress);
}

//==============================================================================

fn_name Pots_UN_Message_ctor1 = "Pots_UN_Message.ctor(i/c)";

Pots_UN_Message::Pots_UN_Message(SbIpBufferPtr& buff) : PotsMessage(buff)
{
   Debug::ft(Pots_UN_Message_ctor1);
}

//------------------------------------------------------------------------------

fn_name Pots_UN_Message_ctor2 = "Pots_UN_Message.ctor(o/g)";

Pots_UN_Message::Pots_UN_Message(ProtocolSM* psm, MsgSize size) :
   PotsMessage(psm, size)
{
   Debug::ft(Pots_UN_Message_ctor2);

   //  If there is no PSM, the message's header must be supplied here.
   //d Node-specific software needs to supply the IP layer 3 addresses.
   //
   if(psm == nullptr)
   {
      auto host = IpPortRegistry::HostAddress();
      auto peer = IpPortRegistry::HostAddress();

      SetProtocol(PotsProtocolId);
      auto addr = GlobalAddress(host, PotsShelfIpPort, PotsShelfFactoryId);
      SetSender(addr);
      addr = GlobalAddress(peer, PotsCallIpPort, PotsCallFactoryId);
      SetReceiver(addr);
      return;
   }
}

//------------------------------------------------------------------------------

fn_name Pots_UN_Message_dtor = "Pots_UN_Message.dtor";

Pots_UN_Message::~Pots_UN_Message()
{
   Debug::ft(Pots_UN_Message_dtor);
}

//------------------------------------------------------------------------------

fn_name Pots_UN_Message_AddDigits = "Pots_UN_Message.AddDigits";

DigitString* Pots_UN_Message::AddDigits(const DigitString& digits)
{
   Debug::ft(Pots_UN_Message_AddDigits);

   return AddType(digits, PotsParameter::Digits);
}

//==============================================================================

fn_name Pots_NU_Message_ctor1 = "Pots_NU_Message.ctor(i/c)";

Pots_NU_Message::Pots_NU_Message(SbIpBufferPtr& buff) : PotsMessage(buff)
{
   Debug::ft(Pots_NU_Message_ctor1);
}

//------------------------------------------------------------------------------

fn_name Pots_NU_Message_ctor2 = "Pots_NU_Message.ctor(o/g)";

Pots_NU_Message::Pots_NU_Message(ProtocolSM* psm, MsgSize size) :
   PotsMessage(psm, size)
{
   Debug::ft(Pots_NU_Message_ctor2);

   //  If there is no PSM, the message's header must be supplied here.
   //d Node-specific software needs to supply the IP layer 3 addresses.
   //
   if(psm == nullptr)
   {
      auto host = IpPortRegistry::HostAddress();
      auto peer = IpPortRegistry::HostAddress();

      SetProtocol(PotsProtocolId);
      auto addr = GlobalAddress(host, PotsCallIpPort, PotsCallFactoryId);
      SetSender(addr);
      addr = GlobalAddress(peer, PotsShelfIpPort, PotsShelfFactoryId);
      SetReceiver(addr);
      return;
   }

   //  If the message will be sent to a multiplexer's network-side PSM,
   //  it must have immediate priority.
   //
   if(psm->PeerFactory() == PotsMuxFactoryId)
   {
      SetPriority(Immediate);
   }
}

//------------------------------------------------------------------------------

fn_name Pots_NU_Message_dtor = "Pots_NU_Message.dtor";

Pots_NU_Message::~Pots_NU_Message()
{
   Debug::ft(Pots_NU_Message_dtor);
}

//------------------------------------------------------------------------------

fn_name Pots_NU_Message_AddRing = "Pots_NU_Message.AddRing";

PotsRingInfo* Pots_NU_Message::AddRing(const PotsRingInfo& ring)
{
   Debug::ft(Pots_NU_Message_AddRing);

   return AddType(ring, PotsParameter::Ring);
}

//------------------------------------------------------------------------------

fn_name Pots_NU_Message_AddScan = "Pots_NU_Message.AddScan";

PotsScanInfo* Pots_NU_Message::AddScan(const PotsScanInfo& scan)
{
   Debug::ft(Pots_NU_Message_AddScan);

   return AddType(scan, PotsParameter::Scan);
}
}
