//==============================================================================
//
//  Immutable.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef IMMUTABLE_H_INCLUDED
#define IMMUTABLE_H_INCLUDED

#include "Object.h"
#include <cstddef>
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Virtual base class for objects allocated on a heap that is write-protected
//  at run-time and that survives all restarts.  Subclasses typically contain
//  data that can only be recreated by rebooting.
//
class Immutable : public Object
{
public:
   //  Virtual to allow subclassing.
   //
   virtual ~Immutable() { }

   //  Overridden to return the type of memory used by subclasses.
   //
   virtual MemoryType MemType() const override { return MemImm; }

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;

   //  Overridden to allocate memory from the immutable heap.
   //
   static void* operator new(size_t size);
   static void* operator new[](size_t size);
protected:
   //  Protected because this class is virtual.
   //
   Immutable();
};
}
#endif
