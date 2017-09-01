//==============================================================================
//
//  Temporary.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef TEMPORARY_H_INCLUDED
#define TEMPORARY_H_INCLUDED

#include "Object.h"
#include <cstddef>
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Virtual base class for objects allocated on a heap that is destroyed during
//  all restarts.  Subclasses contain data that does not need to preserved over
//  a restart or that can easily be recreated.
//
class Temporary : public Object
{
public:
   //  Virtual to allow subclassing.
   //
   virtual ~Temporary() { }

   //  Overridden to return the type of memory used by subclasses.
   //
   virtual MemoryType MemType() const override { return MemTemp; }

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;

   //  Overridden to allocate memory from the temporary heap.
   //
   static void* operator new(size_t size);
   static void* operator new[](size_t size);
protected:
   //  Protected because this class is virtual.
   //
   Temporary();
};
}
#endif
