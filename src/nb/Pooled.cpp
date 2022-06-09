//==============================================================================
//
//  Pooled.cpp
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
#include "Pooled.h"
#include <ostream>
#include <string>
#include "Algorithms.h"
#include "Debug.h"
#include "ObjectPool.h"
#include "ObjectPoolRegistry.h"
#include "ObjectPoolTrace.h"
#include "Registry.h"
#include "Singleton.h"
#include "SysTypes.h"
#include "ToolTypes.h"
#include "TraceBuffer.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
Pooled::Pooled() :
   assigned_(true),
   orphaned_(0),
   corrupt_(false),
   logged_(false)
{
   Debug::ft("Pooled.ctor");
}

//------------------------------------------------------------------------------

void Pooled::Claim()
{
   Debug::ft("Pooled.Claim");

   orphaned_ = 0;

   if(Debug::TraceOn())
   {
      auto buff = Singleton<TraceBuffer>::Instance();

      if(buff->ToolIsOn(ObjPoolTracer))
      {
         auto rec = new ObjectPoolTrace(ObjectPoolTrace::Claimed, *this);
         buff->Insert(rec);
      }
   }
}

//------------------------------------------------------------------------------

void Pooled::ClaimBlocks()
{
   Debug::ft("Pooled.ClaimBlocks");

   //  If this block is corrupt, let the audit recover it.
   //
   if(corrupt_) return;

   //  Mark the block corrupt so that it will be avoided in the future if
   //  turns out to be corrupt.  Claim it and all of the objects that it
   //  owns.  If this succeeds, then it isn't corrupt.
   //
   corrupt_ = true;
   Object::ClaimBlocks();
   corrupt_ = false;
}

//------------------------------------------------------------------------------

void Pooled::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Object::Display(stream, prefix, options);

   auto pid = ObjectPool::ObjPid(this);
   auto seq = ObjectPool::ObjSeq(this);

   stream << prefix << "pid      : " << int(pid) << CRLF;
   stream << prefix << "seq      : " << int(seq) << CRLF;
   stream << prefix << "link     : " << link_.to_str() << CRLF;
   stream << prefix << "assigned : " << assigned_ << CRLF;
   stream << prefix << "orphaned : " << int(orphaned_) << CRLF;
   stream << prefix << "corrupt  : " << corrupt_ << CRLF;
   stream << prefix << "logged   : " << logged_ << CRLF;
}

//------------------------------------------------------------------------------

ptrdiff_t Pooled::LinkDiff()
{
   uintptr_t local;
   auto fake = reinterpret_cast<const Pooled*>(&local);
   return ptrdiff(&fake->link_, fake);
}

//------------------------------------------------------------------------------

MemoryType Pooled::MemType() const
{
   Debug::ft("Pooled.MemType");

   auto pid = ObjectPool::ObjPid(this);
   auto pool = Singleton<ObjectPoolRegistry>::Instance()->Pools().At(pid);
   if(pool != nullptr) return pool->BlockType();
   return MemNull;
}

//------------------------------------------------------------------------------

void Pooled::operator delete(void* addr)
{
   Debug::ftnt("Pooled.operator delete");

   auto obj = (Pooled*) addr;
   auto pid = ObjectPool::ObjPid(obj);
   auto pool = Singleton<ObjectPoolRegistry>::Extant()->Pools().At(pid);
   if(pool != nullptr) pool->EnqBlock(obj, true);
}

//------------------------------------------------------------------------------

void Pooled::Patch(sel_t selector, void* arguments)
{
   Object::Patch(selector, arguments);
}
}
