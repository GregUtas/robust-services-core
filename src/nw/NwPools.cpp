//==============================================================================
//
//  NwPools.cpp
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
#include "NwPools.h"
#include <cstddef>
#include "ByteBuffer.h"
#include "Debug.h"
#include "IpBuffer.h"
#include "NbAppIds.h"
#include "SysTypes.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace NetworkBase
{
constexpr size_t IpBufferSize = sizeof(IpBuffer);

//------------------------------------------------------------------------------

IpBufferPool::IpBufferPool() :
   ObjectPool(IpBufferObjPoolId, MemSlab, IpBufferSize, "IpBuffers")
{
   Debug::ft("IpBufferPool.ctor");
}

//------------------------------------------------------------------------------

IpBufferPool::~IpBufferPool()
{
   Debug::ftnt("IpBufferPool.dtor");
}

//------------------------------------------------------------------------------

void IpBufferPool::Patch(sel_t selector, void* arguments)
{
   ObjectPool::Patch(selector, arguments);
}

//==============================================================================

constexpr size_t TinyBufferSize = sizeof(TinyBuffer);

//------------------------------------------------------------------------------

TinyBufferPool::TinyBufferPool() :
   ObjectPool(TinyBufferObjPoolId, MemSlab, TinyBufferSize, "TinyBuffers")
{
   Debug::ft("TinyBufferPool.ctor");
}

//------------------------------------------------------------------------------

TinyBufferPool::~TinyBufferPool()
{
   Debug::ftnt("TinyBufferPool.dtor");
}

//------------------------------------------------------------------------------

void TinyBufferPool::Patch(sel_t selector, void* arguments)
{
   ObjectPool::Patch(selector, arguments);
}

//==============================================================================

constexpr size_t SmallBufferSize = sizeof(SmallBuffer);

//------------------------------------------------------------------------------

SmallBufferPool::SmallBufferPool() :
   ObjectPool(SmallBufferObjPoolId, MemSlab, SmallBufferSize, "SmallBuffers")
{
   Debug::ft("SmallBufferPool.ctor");
}

//------------------------------------------------------------------------------

SmallBufferPool::~SmallBufferPool()
{
   Debug::ftnt("SmallBufferPool.dtor");
}

//------------------------------------------------------------------------------

void SmallBufferPool::Patch(sel_t selector, void* arguments)
{
   ObjectPool::Patch(selector, arguments);
}

//==============================================================================

constexpr size_t MediumBufferSize = sizeof(MediumBuffer);

//------------------------------------------------------------------------------

MediumBufferPool::MediumBufferPool() :
   ObjectPool(MediumBufferObjPoolId, MemSlab, MediumBufferSize, "MediumBuffers")
{
   Debug::ft("MediumBufferPool.ctor");
}

//------------------------------------------------------------------------------

MediumBufferPool::~MediumBufferPool()
{
   Debug::ftnt("MediumBufferPool.dtor");
}

//------------------------------------------------------------------------------

void MediumBufferPool::Patch(sel_t selector, void* arguments)
{
   ObjectPool::Patch(selector, arguments);
}

//==============================================================================

constexpr size_t LargeBufferSize = sizeof(LargeBuffer);

//------------------------------------------------------------------------------

LargeBufferPool::LargeBufferPool() :
   ObjectPool(LargeBufferObjPoolId, MemSlab, LargeBufferSize, "LargeBuffers")
{
   Debug::ft("LargeBufferPool.ctor");
}

//------------------------------------------------------------------------------

LargeBufferPool::~LargeBufferPool()
{
   Debug::ftnt("LargeBufferPool.dtor");
}

//------------------------------------------------------------------------------

void LargeBufferPool::Patch(sel_t selector, void* arguments)
{
   ObjectPool::Patch(selector, arguments);
}

//==============================================================================

constexpr size_t HugeBufferSize = sizeof(HugeBuffer);

//------------------------------------------------------------------------------

HugeBufferPool::HugeBufferPool() :
   ObjectPool(HugeBufferObjPoolId, MemSlab, HugeBufferSize, "HugeBuffers")
{
   Debug::ft("HugeBufferPool.ctor");
}

//------------------------------------------------------------------------------

HugeBufferPool::~HugeBufferPool()
{
   Debug::ftnt("HugeBufferPool.dtor");
}

//------------------------------------------------------------------------------

void HugeBufferPool::Patch(sel_t selector, void* arguments)
{
   ObjectPool::Patch(selector, arguments);
}
}
