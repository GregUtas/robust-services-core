//==============================================================================
//
//  SbAppIds.h
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
#ifndef SBAPPIDS_H_INCLUDED
#define SBAPPIDS_H_INCLUDED

//------------------------------------------------------------------------------

namespace SessionBase
{
//  Constants based on the following types should be defined here:
//
//  o ProtocolId
//  o FactoryId
//  o ServiceId
//
enum ProtocolIds
{
   TimerProtocolId = 1,
   TestProtocolId = 2,
   CipProtocolId = 3,
   PotsProtocolId = 4
};

enum FactoryIds
{
   TestFactoryId = 1,
   CipObcFactoryId = 2,
   CipTbcFactoryId = 3,
   ProxyCallFactoryId = 4,
   TestCallFactoryId = 5,
   PotsShelfFactoryId = 6,
   PotsCallFactoryId = 7,
   PotsMuxFactoryId = 8
};

enum ServiceIds
{
   TestServiceId = 1,
   PotsCallServiceId = 2,
   PotsSusServiceId = 3,
   PotsBocServiceId = 4,
   PotsBicServiceId = 5,
   PotsHtlServiceId = 6,
   PotsWmlActivation = 7,
   PotsWmlDeactivation = 8,
   PotsWmlServiceId = 9,
   PotsCcwServiceId = 10,
   PotsCwbServiceId = 11,
   PotsCwmServiceId = 12,
   PotsCwaServiceId = 13,
   PotsMuxServiceId = 14,
   PotsDiscServiceId = 15,
   PotsCfuActivation = 16,
   PotsCfuDeactivation = 17,
   PotsCfuServiceId = 18,
   PotsCfbServiceId = 19,
   PotsCfnServiceId = 20,
   PotsCfxServiceId = 21,
   PotsProxyServiceId = 22
};
}
#endif
