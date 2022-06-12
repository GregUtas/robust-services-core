//==============================================================================
//
//  NbPools.cpp
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
#include "NbPools.h"
#include <cstddef>
#include <iomanip>
#include "ClassRegistry.h"
#include "Debug.h"
#include "DeferredRegistry.h"
#include "MsgBuffer.h"
#include "NbAppIds.h"
#include "NbCliParms.h"
#include "Singleton.h"
#include "SysTypes.h"
#include "ThreadRegistry.h"
#include "TraceBuffer.h"

using std::ostream;
using std::setw;

//------------------------------------------------------------------------------

namespace NodeBase
{
constexpr size_t MsgBufferSize = sizeof(MsgBuffer) + (16 * BYTES_PER_WORD);

//------------------------------------------------------------------------------

MsgBufferPool::MsgBufferPool() :
   ObjectPool(MsgBufferObjPoolId, MemSlab, MsgBufferSize, "MsgBuffers")
{
   Debug::ft("MsgBufferPool.ctor");
}

//------------------------------------------------------------------------------

MsgBufferPool::~MsgBufferPool()
{
   Debug::ftnt("MsgBufferPool.dtor");
}

//------------------------------------------------------------------------------

void MsgBufferPool::ClaimBlocks()
{
   Debug::ft("MsgBufferPool.ClaimBlocks");

   Singleton<ThreadRegistry>::Instance()->ClaimBlocks();
   Singleton<DeferredRegistry>::Instance()->ClaimBlocks();
   Singleton<TraceBuffer>::Instance()->ClaimBlocks();

   //  Although subclasses of Class don't necessarily own MsgBuffers, they
   //  can own pooled objects for the purpose of supporting object templates
   //  and quasi-singletons.  One pool must therefore invoke ClaimBlocks on
   //  classes to have those blocks marked in use, so it might as well be
   //  this pool.
   //
   Singleton<ClassRegistry>::Instance()->ClaimBlocks();
}

//------------------------------------------------------------------------------

void MsgBufferPool::Patch(sel_t selector, void* arguments)
{
   ObjectPool::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fixed_string MsgBufferHeader = "RxTime(>>20)  Address";
//                             |          11..<address>

size_t MsgBufferPool::Summarize(ostream& stream, uint32_t selector) const
{
   stream << MsgBufferHeader << CRLF;

   auto items = GetUsed();

   if(items.empty())
   {
      stream << spaces(2) << NoBuffersExpl << CRLF;
      return 0;
   }

   for(auto obj = items.cbegin(); obj != items.cend(); ++obj)
   {
      if((*obj)->IsValid())
      {
         auto buff = static_cast<const MsgBuffer*>(*obj);
         stream << setw(11)
            << (buff->RxTime().time_since_epoch().count() >> 20);
         stream << spaces(2) << this << CRLF;
         ThisThread::PauseOver(90);
      }
   }

   return items.size();
}
}
