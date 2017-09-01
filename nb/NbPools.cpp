//==============================================================================
//
//  NbPools.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "NbPools.h"
#include "Debug.h"
#include "MsgBuffer.h"
#include "NbAppIds.h"
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

MsgBufferPool::MsgBufferPool() :
   ObjectPool(MsgBufferObjPoolId, MemDyn, BlockSize, "MsgBuffers")
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

ThreadPool::ThreadPool() :
   ObjectPool(ThreadObjPoolId, MemPerm, BlockSize, "Threads")
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