//==============================================================================
//
//  SbAppIds.h
//
//  Copyright (C) 2013-2021  Greg Utas
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
#ifndef SBAPPIDS_H_INCLUDED
#define SBAPPIDS_H_INCLUDED

#include "SbTypes.h"

//------------------------------------------------------------------------------

namespace SessionBase
{
//  Constants based on the following types should be defined here:
//
//  o ProtocolId
//  o FactoryId
//  o ServiceId
//
constexpr ProtocolId TimerProtocolId = 1;
constexpr ProtocolId TestProtocolId = 2;
constexpr ProtocolId CipProtocolId = 3;
constexpr ProtocolId PotsProtocolId = 4;

constexpr FactoryId TestFactoryId = 1;
constexpr FactoryId CipObcFactoryId = 2;
constexpr FactoryId CipTbcFactoryId = 3;
constexpr FactoryId ProxyCallFactoryId = 4;
constexpr FactoryId TestCallFactoryId = 5;
constexpr FactoryId PotsShelfFactoryId = 6;
constexpr FactoryId PotsCallFactoryId = 7;
constexpr FactoryId PotsMuxFactoryId = 8;

constexpr ServiceId TestServiceId = 1;
constexpr ServiceId PotsCallServiceId = 2;
constexpr ServiceId PotsSusServiceId = 3;
constexpr ServiceId PotsBocServiceId = 4;
constexpr ServiceId PotsBicServiceId = 5;
constexpr ServiceId PotsHtlServiceId = 6;
constexpr ServiceId PotsWmlActivation = 7;
constexpr ServiceId PotsWmlDeactivation = 8;
constexpr ServiceId PotsWmlServiceId = 9;
constexpr ServiceId PotsCcwServiceId = 10;
constexpr ServiceId PotsCwbServiceId = 11;
constexpr ServiceId PotsCwmServiceId = 12;
constexpr ServiceId PotsCwaServiceId = 13;
constexpr ServiceId PotsMuxServiceId = 14;
constexpr ServiceId PotsDiscServiceId = 15;
constexpr ServiceId PotsCfuActivation = 16;
constexpr ServiceId PotsCfuDeactivation = 17;
constexpr ServiceId PotsCfuServiceId = 18;
constexpr ServiceId PotsCfbServiceId = 19;
constexpr ServiceId PotsCfnServiceId = 20;
constexpr ServiceId PotsCfxServiceId = 21;
constexpr ServiceId PotsProxyServiceId = 22;
}
#endif
