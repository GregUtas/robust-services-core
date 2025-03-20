//==============================================================================
//
//  PotsShelfIpService.cpp
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
#include "PotsShelf.h"
#include "CliText.h"
#include "Debug.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace PotsBase
{
PotsShelfIpService::PotsShelfIpService()
{
   Debug::ft("PotsShelfIpService.ctor");
}

//------------------------------------------------------------------------------

PotsShelfIpService::~PotsShelfIpService()
{
   Debug::ftnt("PotsShelfIpService.dtor");
}

//------------------------------------------------------------------------------

InputHandler* PotsShelfIpService::CreateHandler(IpPort* port) const
{
   Debug::ft("PotsShelfIpService.CreateHandler");

   return new PotsShelfHandler(port);
}

//------------------------------------------------------------------------------

fixed_string PotsShelfServiceStr = "POTS Shelf/UDP";
fixed_string PotsShelfServiceExpl = "POTS Shelf Protocol";

CliText* PotsShelfIpService::CreateText() const
{
   Debug::ft("PotsShelfIpService.CreateText");

   return new CliText(PotsShelfServiceStr, PotsShelfServiceExpl);
}
}
