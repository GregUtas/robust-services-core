//==============================================================================
//
//  NwPools.h
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
#ifndef NWPOOLS_H_INCLUDED
#define NWPOOLS_H_INCLUDED

#include "ObjectPool.h"
#include "NbTypes.h"

//------------------------------------------------------------------------------

namespace NetworkBase
{
//  Pool for IpBuffer objects.
//
class IpBufferPool : public NodeBase::ObjectPool
{
   friend class NodeBase::Singleton<IpBufferPool>;
public:
   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
private:
   //  Private because this is a singleton.
   //
   IpBufferPool();

   //  Private because this is a singleton.
   //
   ~IpBufferPool();
};

//------------------------------------------------------------------------------
//
//  Pool for tiny byte buffers for IpBuffer payloads.
//
class TinyBufferPool : public NodeBase::ObjectPool
{
   friend class NodeBase::Singleton<TinyBufferPool>;
public:
   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
private:
   //  Private because this is a singleton.
   //
   TinyBufferPool();

   //  Private because this is a singleton.
   //
   ~TinyBufferPool();
};

//------------------------------------------------------------------------------
//
//  Pool for small byte buffers for IpBuffer payloads.
//
class SmallBufferPool : public NodeBase::ObjectPool
{
   friend class NodeBase::Singleton<SmallBufferPool>;
public:
   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
private:
   //  Private because this is a singleton.
   //
   SmallBufferPool();

   //  Private because this is a singleton.
   //
   ~SmallBufferPool();
};

//------------------------------------------------------------------------------
//
//  Pool for medium byte buffers for IpBuffer payloads.
//
class MediumBufferPool : public NodeBase::ObjectPool
{
   friend class NodeBase::Singleton<MediumBufferPool>;
public:
   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
private:
   //  Private because this is a singleton.
   //
   MediumBufferPool();

   //  Private because this is a singleton.
   //
   ~MediumBufferPool();
};

//------------------------------------------------------------------------------
//
//  Pool for large byte buffers for IpBuffer payloads.
//
class LargeBufferPool : public NodeBase::ObjectPool
{
   friend class NodeBase::Singleton<LargeBufferPool>;
public:
   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
private:
   //  Private because this is a singleton.
   //
   LargeBufferPool();

   //  Private because this is a singleton.
   //
   ~LargeBufferPool();
};

//------------------------------------------------------------------------------
//
//  Pool for huge byte buffers for IpBuffer payloads.
//
class HugeBufferPool : public NodeBase::ObjectPool
{
   friend class NodeBase::Singleton<HugeBufferPool>;
public:
   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
private:
   //  Private because this is a singleton.
   //
   HugeBufferPool();

   //  Private because this is a singleton.
   //
   ~HugeBufferPool();
};
}
#endif
