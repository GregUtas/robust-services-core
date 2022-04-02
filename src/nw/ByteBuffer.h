//==============================================================================
//
//  ByteBuffer.h
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
#ifndef BYTEBUFFER_H_INCLUDED
#define BYTEBUFFER_H_INCLUDED

#include "Pooled.h"
#include <cstddef>
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NetworkBase
{
//  Virtual base class for byte buffers.
//
class ByteBuffer : public NodeBase::Pooled
{
public:
   //  Virtual to allow subclassing.
   //
   virtual ~ByteBuffer() = default;

   //  Returns the location of the actual byte buffer.
   //
   virtual NodeBase::byte_t* Bytes() = 0;

   //  Returns the number of bytes that a buffer can hold.
   //
   virtual size_t Size() const = 0;
protected:
   //  Protected because this class is virtual.
   //
   ByteBuffer() = default;
};

//  The size of the virtual ByteBuffer base class.
//
constexpr size_t ByteBufferSize = sizeof(ByteBuffer);

//------------------------------------------------------------------------------
//
//  Tiny, small, medium, large, and huge byte buffers.
//
class TinyBuffer : public ByteBuffer
{
public:
   static const size_t ArraySize = 48 - ByteBufferSize;
   static void* operator new(size_t size);
private:
   NodeBase::byte_t* Bytes() override { return &bytes_[0]; }
   size_t Size() const override { return ArraySize; }
   NodeBase::byte_t bytes_[ArraySize];
};

class SmallBuffer : public ByteBuffer
{
public:
   static const size_t ArraySize = 128 - ByteBufferSize;
   static void* operator new(size_t size);
private:
   NodeBase::byte_t* Bytes() override { return &bytes_[0]; }
   size_t Size() const override { return ArraySize; }
   NodeBase::byte_t bytes_[ArraySize];
};

class MediumBuffer : public ByteBuffer
{
public:
   static const size_t ArraySize = 512 - ByteBufferSize;
   static void* operator new(size_t size);
private:
   NodeBase::byte_t* Bytes() override { return &bytes_[0]; }
   size_t Size() const override { return ArraySize; }
   NodeBase::byte_t bytes_[ArraySize];
};

class LargeBuffer : public ByteBuffer
{
public:
   static const size_t ArraySize = 2048 - ByteBufferSize;
   static void* operator new(size_t size);
private:
   NodeBase::byte_t* Bytes() override { return &bytes_[0]; }
   size_t Size() const override { return ArraySize; }
   NodeBase::byte_t bytes_[ArraySize];
};

class HugeBuffer : public ByteBuffer
{
public:
   static const size_t ArraySize = 8192 - ByteBufferSize;
   static void* operator new(size_t size);
private:
   NodeBase::byte_t* Bytes() override { return &bytes_[0]; }
   size_t Size() const override { return ArraySize; }
   NodeBase::byte_t bytes_[ArraySize];
};
}
#endif
