//==============================================================================
//
//  MbPools.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
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
   ObjectPool(MediaEndptObjPoolId, MemDyn, BlockSize, "MediaEndpts")
{
   Debug::ft(MediaEndptPool_ctor);
}

//------------------------------------------------------------------------------

fn_name MediaEndptPool_dtor = "MediaEndptPool.dtor";

MediaEndptPool::~MediaEndptPool()
{
   Debug::ft(MediaEndptPool_dtor);
}
}