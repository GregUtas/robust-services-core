//==============================================================================
//
//  Dynamic.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef DYNAMIC_H_INCLUDED
#define DYNAMIC_H_INCLUDED

#include "Object.h"
#include <cstddef>
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Virtual base class for objects allocated on a heap that survives a warm
//  restart.  Subclasses typically contain important data that changes too
//  frequently to be write-protected.  Such classes usually contain state
//  information for system components or payload applications.
//
class Dynamic : public Object
{
public:
   //  Virtual to allow subclassing.
   //
   virtual ~Dynamic() { }

   //  Overridden to return the type of memory used by subclasses.
   //
   virtual MemoryType MemType() const override { return MemDyn; }

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;

   //  Overridden to allocate memory from the dynamic heap.
   //
   static void* operator new(size_t size);
   static void* operator new[](size_t size);
protected:
   //  Protected because this class is virtual.
   //
   Dynamic();
};
}
#endif
