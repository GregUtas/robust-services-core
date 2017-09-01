//==============================================================================
//
//  MbPools.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef MBPOOLS_H_INCLUDED
#define MBPOOLS_H_INCLUDED

#include "ObjectPool.h"
#include <cstddef>
#include "NbTypes.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace MediaBase
{
//  Pool for MediaEndpt objects.
//
class MediaEndptPool : public ObjectPool
{
   friend class Singleton< MediaEndptPool >;
public:
   //> The size of MediaEndpt blocks.
   //
   static const size_t BlockSize;
private:
   //  Private because this singleton is not subclassed.
   //
   MediaEndptPool();

   //  Private because this singleton is not subclassed.
   //
   ~MediaEndptPool();
};
}
#endif
