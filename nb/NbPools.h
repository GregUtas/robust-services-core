//==============================================================================
//
//  NbPools.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
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
