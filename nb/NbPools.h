//==============================================================================
//
//  NbPools.h
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
#ifndef NBPOOLS_H_INCLUDED
#define NBPOOLS_H_INCLUDED

#include "ObjectPool.h"
#include <cstddef>
#include "NbTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Pool for MsgBuffer objects.
//
class MsgBufferPool : public ObjectPool
{
   friend class Singleton< MsgBufferPool >;
public:
   //> The size of MsgBuffer blocks.
   //
   static const size_t BlockSize;

   //  Overridden to claim blocks held by the trace buffer.
   //
   virtual void ClaimBlocks() override;

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;
private:
   //  Private because this singleton is not subclassed.
   //
   MsgBufferPool();

   //  Private because this singleton is not subclassed.
   //
   ~MsgBufferPool();
};

//  Pool for Thread objects.
//
class ThreadPool : public ObjectPool
{
   friend class Singleton< ThreadPool >;
public:
   //> The size of Thread blocks.
   //
   static const size_t BlockSize;

   //  Overridden to claim blocks held by NbTracer.
   //
   virtual void ClaimBlocks() override;

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;
private:
   //  Private because this singleton is not subclassed.
   //
   ThreadPool();

   //  Private because this singleton is not subclassed.
   //
   ~ThreadPool();
};
}
#endif
