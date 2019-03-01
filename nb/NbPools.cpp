//==============================================================================
//
//  NbPools.cpp
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
#include "NbPools.h"
#include "Debug.h"
#include "MsgBuffer.h"
#include "Singleton.h"
#include "SysTypes.h"
#include "Thread.h"
#include "ThreadRegistry.h"
#include "TraceBuffer.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
const size_t MsgBufferPool::BlockSize =
   sizeof(MsgBuffer) + (16 * BYTES_PER_WORD);

//------------------------------------------------------------------------------

fn_name MsgBufferPool_ctor = "MsgBufferPool.ctor";

MsgBufferPool::MsgBufferPool() : ObjectPool(MemDyn, BlockSize, "MsgBuffers")
{
   Debug::ft(MsgBufferPool_ctor);
}

//------------------------------------------------------------------------------

fn_name MsgBufferPool_dtor = "MsgBufferPool.dtor";

MsgBufferPool::~MsgBufferPool()
{
   Debug::ft(MsgBufferPool_dtor);
}

//------------------------------------------------------------------------------

fn_name MsgBufferPool_ClaimBlocks = "MsgBufferPool.ClaimBlocks";

void MsgBufferPool::ClaimBlocks()
{
   Debug::ft(MsgBufferPool_ClaimBlocks);

   Singleton< TraceBuffer >::Instance()->ClaimBlocks();
}

//------------------------------------------------------------------------------

void MsgBufferPool::Patch(sel_t selector, void* arguments)
{
   ObjectPool::Patch(selector, arguments);
}

//==============================================================================

const size_t ThreadPool::BlockSize = sizeof(Thread) + (60 * BYTES_PER_WORD);

//------------------------------------------------------------------------------

fn_name ThreadPool_ctor = "ThreadPool.ctor";

ThreadPool::ThreadPool() : ObjectPool(MemPerm, BlockSize, "Threads")
{
   Debug::ft(ThreadPool_ctor);
}

//------------------------------------------------------------------------------

fn_name ThreadPool_dtor = "ThreadPool.dtor";

ThreadPool::~ThreadPool()
{
   Debug::ft(ThreadPool_dtor);
}

//------------------------------------------------------------------------------

fn_name ThreadPool_ClaimBlocks = "ThreadPool.ClaimBlocks";

void ThreadPool::ClaimBlocks()
{
   Debug::ft(ThreadPool_ClaimBlocks);

   Singleton< ThreadRegistry >::Instance()->ClaimBlocks();
}

//------------------------------------------------------------------------------

void ThreadPool::Patch(sel_t selector, void* arguments)
{
   ObjectPool::Patch(selector, arguments);
}
}
