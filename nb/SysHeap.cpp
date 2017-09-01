//==============================================================================
//
//  SysHeap.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
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
