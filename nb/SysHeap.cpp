//==============================================================================
//
//  SysHeap.cpp
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
#include "SysHeap.h"
#include <new>
#include <ostream>
#include <string>
#include "AllocationException.h"
#include "Debug.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
void SysHeap::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Object::Display(stream, prefix, options);

   stream << prefix << "heap     : " << heap_ << CRLF;
   stream << prefix << "type     : " << type_ << CRLF;
   stream << prefix << "inUse    : " << inUse_ << CRLF;
   stream << prefix << "allocs   : " << allocs_ << CRLF;
   stream << prefix << "fails    : " << fails_ << CRLF;
   stream << prefix << "frees    : " << frees_ << CRLF;
   stream << prefix << "maxInUse : " << maxInUse_ << CRLF;
}

//------------------------------------------------------------------------------

fn_name SysHeap_delete1 = "SysHeap.operator delete";

void SysHeap::operator delete(void* addr)
{
   Debug::ft(SysHeap_delete1);

   ::operator delete(addr);
}

//------------------------------------------------------------------------------

fn_name SysHeap_delete2 = "SysHeap.operator delete[]";

void SysHeap::operator delete[](void* addr)
{
   Debug::ft(SysHeap_delete2);

   ::operator delete[](addr);
}

//------------------------------------------------------------------------------

fn_name SysHeap_new1 = "SysHeap.operator new";

void* SysHeap::operator new(size_t size)
{
   Debug::ft(SysHeap_new1);

   auto addr = ::operator new(size, std::nothrow);
   if(addr != nullptr) return addr;
   throw AllocationException(MemPerm, size);
}

//------------------------------------------------------------------------------

fn_name SysHeap_new2 = "SysHeap.operator new[]";

void* SysHeap::operator new[](size_t size)
{
   Debug::ft(SysHeap_new2);

   auto addr = ::operator new[](size, std::nothrow);
   if(addr != nullptr) return addr;
   throw AllocationException(MemPerm, size);
}
}
