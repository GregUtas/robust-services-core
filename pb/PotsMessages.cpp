//==============================================================================
//
//  PotsMessages.cpp
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
#include "PotsProtocol.h"
#include "BcAddress.h"
#include "BcProgress.h"
#include "Debug.h"
#include "GlobalAddress.h"
#include "IpPortRegistry.h"
#include "MediaParameter.h"
#include "NwTypes.h"
#include "SbAppIds.h"

using namespace NetworkBase;

//------------------------------------------------------------------------------

namespace PotsBase
{
PotsMessage::PotsMessage(SbIpBufferPtr& buff) : TlvMessage(buff)
{
   Debug::ft("PotsMessage.ctor(i/c)");
}

//------------------------------------------------------------------------------

PotsMessage::PotsMessage(ProtocolSM* psm, size_t size) : TlvMessage(psm, size)
{
   Debug::ft("PotsMessage.ctor(o/g)");
}

//------------------------------------------------------------------------------

PotsMessage::~PotsMessage()
{
   Debug::ftnt("PotsMessage.dtor");
}

//------------------------------------------------------------------------------

CauseInfo* PotsMessage::AddCause(const CauseInfo& cause)
{
   Debug::ft("PotsMessage.AddCause");

   return AddType(cause, PotsParameter::Cause);
}

//------------------------------------------------------------------------------

PotsFacilityInfo* PotsMessage::AddFacility(const PotsFacilityInfo& facility)
{
   Debug::ft("PotsMessage.AddFacility");

   return AddType(facility, PotsParameter::Facility);
}

//------------------------------------------------------------------------------

PotsHeaderInfo* PotsMessage::AddHeader(const PotsHeaderInfo& header)
{
   Debug::ft("PotsMessage.AddHeader");

   SetSignal(header.signal);
   return AddType(header, PotsParameter::Header);
}

//------------------------------------------------------------------------------

MediaInfo* PotsMessage::AddMedia(const MediaInfo& media)
{
   Debug::ft("PotsMessage.AddMedia");

   return AddType(media, PotsParameter::Media);
}

//------------------------------------------------------------------------------

ProgressInfo* PotsMessage::AddProgress(const ProgressInfo& progress)
{
   Debug::ft("PotsMessage.AddProgress");

   return AddType(progress, PotsParameter::Progress);
}

//==============================================================================

Pots_UN_Message::Pots_UN_Message(SbIpBufferPtr& buff) : PotsMessage(buff)
{
   Debug::ft("Pots_UN_Message.ctor(i/c)");
}

//------------------------------------------------------------------------------

Pots_UN_Message::Pots_UN_Message(ProtocolSM* psm, size_t size) :
   PotsMessage(psm, size)
{
   Debug::ft("Pots_UN_Message.ctor(o/g)");

   //  If there is no PSM, the message's header must be supplied here.
   //d Node-specific software needs to supply the IP layer 3 addresses.
   //
   if(psm == nullptr)
   {
      auto& self = IpPortRegistry::LocalAddr();
      auto& peer = IpPortRegistry::LocalAddr();

      SetProtocol(PotsProtocolId);
      GlobalAddress addr(self, PotsShelfIpPort, PotsShelfFactoryId);
      SetSender(addr);
      addr = GlobalAddress(peer, PotsCallIpPort, PotsCallFactoryId);
      SetReceiver(addr);
      return;
   }
}

//------------------------------------------------------------------------------

Pots_UN_Message::~Pots_UN_Message()
{
   Debug::ftnt("Pots_UN_Message.dtor");
}

//------------------------------------------------------------------------------

DigitString* Pots_UN_Message::AddDigits(const DigitString& digits)
{
   Debug::ft("Pots_UN_Message.AddDigits");

   return AddType(digits, PotsParameter::Digits);
}

//==============================================================================

Pots_NU_Message::Pots_NU_Message(SbIpBufferPtr& buff) : PotsMessage(buff)
{
   Debug::ft("Pots_NU_Message.ctor(i/c)");
}

//------------------------------------------------------------------------------

Pots_NU_Message::Pots_NU_Message(ProtocolSM* psm, size_t size) :
   PotsMessage(psm, size)
{
   Debug::ft("Pots_NU_Message.ctor(o/g)");

   //  If there is no PSM, the message's header must be supplied here.
   //d Node-specific software needs to supply the IP layer 3 addresses.
   //
   if(psm == nullptr)
   {
      auto& self = IpPortRegistry::LocalAddr();
      auto& peer = IpPortRegistry::LocalAddr();

      SetProtocol(PotsProtocolId);
      GlobalAddress addr(self, PotsCallIpPort, PotsCallFactoryId);
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
      SetPriority(IMMEDIATE);
   }
}

//------------------------------------------------------------------------------

Pots_NU_Message::~Pots_NU_Message()
{
   Debug::ftnt("Pots_NU_Message.dtor");
}

//------------------------------------------------------------------------------

PotsRingInfo* Pots_NU_Message::AddRing(const PotsRingInfo& ring)
{
   Debug::ft("Pots_NU_Message.AddRing");

   return AddType(ring, PotsParameter::Ring);
}

//------------------------------------------------------------------------------

PotsScanInfo* Pots_NU_Message::AddScan(const PotsScanInfo& scan)
{
   Debug::ft("Pots_NU_Message.AddScan");

   return AddType(scan, PotsParameter::Scan);
}
}
