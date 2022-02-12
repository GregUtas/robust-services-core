//==============================================================================
//
//  ByteBuffer.cpp
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
#include "ByteBuffer.h"
#include "Debug.h"
#include "NwPools.h"
#include "Singleton.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace NetworkBase
{
byte_t* ByteBuffer::Bytes()
{
   Debug::SwLog("ByteBuffer.Bytes", strOver(this), 0);
   return nullptr;
}

//------------------------------------------------------------------------------

size_t ByteBuffer::Size() const
{
   Debug::SwLog("ByteBuffer.Size", strOver(this), 0);
   return 0;
}

//==============================================================================

void* HugeBuffer::operator new(size_t size)
{
   Debug::ft("HugeBuffer.operator new");

   return Singleton< HugeBufferPool >::Instance()->DeqBlock(size);
}

//------------------------------------------------------------------------------

void* LargeBuffer::operator new(size_t size)
{
   Debug::ft("LargeBuffer.operator new");

   return Singleton< LargeBufferPool >::Instance()->DeqBlock(size);
}

//------------------------------------------------------------------------------

void* MediumBuffer::operator new(size_t size)
{
   Debug::ft("MediumBuffer.operator new");

   return Singleton< MediumBufferPool >::Instance()->DeqBlock(size);
}

//------------------------------------------------------------------------------

void* SmallBuffer::operator new(size_t size)
{
   Debug::ft("SmallBuffer.operator new");

   return Singleton< SmallBufferPool >::Instance()->DeqBlock(size);
}

//------------------------------------------------------------------------------

void* TinyBuffer::operator new(size_t size)
{
   Debug::ft("TinyBuffer.operator new");

   return Singleton< TinyBufferPool >::Instance()->DeqBlock(size);
}
}
