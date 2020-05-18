//==============================================================================
//
//  MbPools.cpp
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
#include "MbPools.h"
#include "Debug.h"
#include "MediaEndpt.h"
#include "NbAppIds.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace MediaBase
{
const size_t MediaEndptPool::BlockSize =
   sizeof(MediaEndpt) + (40 * BYTES_PER_WORD);

//------------------------------------------------------------------------------

fn_name MediaEndptPool_ctor = "MediaEndptPool.ctor";

MediaEndptPool::MediaEndptPool() :
   ObjectPool(MediaEndptObjPoolId, MemDynamic, BlockSize, "MediaEndpts")
{
   Debug::ft(MediaEndptPool_ctor);
}

//------------------------------------------------------------------------------

fn_name MediaEndptPool_dtor = "MediaEndptPool.dtor";

MediaEndptPool::~MediaEndptPool()
{
   Debug::ftnt(MediaEndptPool_dtor);
}
}
