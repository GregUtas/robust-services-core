//==============================================================================
//
//  MbPools.h
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
