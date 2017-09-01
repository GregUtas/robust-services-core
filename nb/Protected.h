//==============================================================================
//
//  Protected.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef PROTECTED_H_INCLUDED
#define PROTECTED_H_INCLUDED

#include "Object.h"
#include <cstddef>
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Virtual base class for objects allocated on a heap that is write-protected
//  at run-time and that survives both warm and cold restarts.  Subclasses
//  contain critical data that changes infrequently, typically configuration
//  data.
//
class Protected : public Object
{
public:
   //  Virtual to allow subclassing.
   //
   virtual ~Protected() { }

   //  Overridden to return the type of memory used by subclasses.
   //
   virtual MemoryType MemType() const override { return MemProt; }

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;

   //  Overridden to allocate memory from the protected heap.
   //
   static void* operator new(size_t size);
   static void* operator new[](size_t size);
protected:
   //  Protected because this class is virtual.
   //
   Protected();
};
}
#endif
