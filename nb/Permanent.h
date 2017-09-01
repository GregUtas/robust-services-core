//==============================================================================
//
//  Permanent.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef PERMANENT_H_INCLUDED
#define PERMANENT_H_INCLUDED

#include "Object.h"
#include <cstddef>
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Virtual base class for objects allocated on a heap that survives all
//  restarts.  This capability is not often required.  An example is logs
//  that could help to determine why the restart occurred.  Threads also
//  use it because some of them must survive all restarts.  However, most
//  threads exit during restarts.
//
class Permanent : public Object
{
public:
   //  Virtual to allow subclassing.
   //
   virtual ~Permanent() { }

   //  Overridden to return the type of memory used by subclasses.
   //
   virtual MemoryType MemType() const override { return MemPerm; }

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;

   //  Overridden to allocate memory from the permanent heap.
   //
   static void* operator new(size_t size);
   static void* operator new[](size_t size);
protected:
   //  Protected because this class is virtual.
   //
   Permanent();
};
}
#endif
