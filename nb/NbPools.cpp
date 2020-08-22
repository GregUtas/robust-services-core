//==============================================================================
//
//  NbPools.cpp
//
//  Copyright (C) 2013-2020  Greg Utas
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
#include "NbPools.h"
#include "ClassRegistry.h"
#include "Debug.h"
#include "MsgBuffer.h"
#include "NbAppIds.h"
#include "Singleton.h"
#include "SysTypes.h"
#include "ThreadRegistry.h"
#include "TraceBuffer.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
const size_t MsgBufferPool::BlockSize =
   sizeof(MsgBuffer) + (16 * BYTES_PER_WORD);

//------------------------------------------------------------------------------

fn_name MsgBufferPool_ctor = "MsgBufferPool.ctor";

MsgBufferPool::MsgBufferPool() :
   ObjectPool(MsgBufferObjPoolId, MemDynamic, BlockSize, "MsgBuffers")
{
   Debug::ft(MsgBufferPool_ctor);
}

//------------------------------------------------------------------------------

fn_name MsgBufferPool_dtor = "MsgBufferPool.dtor";

MsgBufferPool::~MsgBufferPool()
{
   Debug::ftnt(MsgBufferPool_dtor);
}

//------------------------------------------------------------------------------

fn_name MsgBufferPool_ClaimBlocks = "MsgBufferPool.ClaimBlocks";

void MsgBufferPool::ClaimBlocks()
{
   Debug::ft(MsgBufferPool_ClaimBlocks);

   Singleton< ThreadRegistry >::Instance()->ClaimBlocks();
   Singleton< TraceBuffer >::Instance()->ClaimBlocks();

   //  Although subclasses of Class don't necessarily own MsgBuffers, they
   //  can own pooled objects for the purpose of supporting object templates
   //  and quasi-singletons.  One pool must therefore invoke ClaimBlocks on
   //  classes to have those blocks marked in use, so it might as well be
   //  this pool.
   //
   Singleton< ClassRegistry >::Instance()->ClaimBlocks();
}

//------------------------------------------------------------------------------

void MsgBufferPool::Patch(sel_t selector, void* arguments)
{
   ObjectPool::Patch(selector, arguments);
}
}
