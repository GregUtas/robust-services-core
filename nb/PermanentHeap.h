//==============================================================================
//
//  PermanentHeap.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef PERMANENTHEAP_H_INCLUDED
#define PERMANENTHEAP_H_INCLUDED

#include "SysHeap.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  The default heap, which allocates memory of type MemPerm.  The heap for
//  each memory type is a singleton.  However, each singleton registers with
//  Singletons, which also needs to allocate memory from a heap.  This class
//  therefore resolves the circular dependency between heaps and singletons.
//
class PermanentHeap : public SysHeap
{
public:
   //  Returns the default heap.
   //
   static PermanentHeap* Instance();
private:
   //  Private to restrict creation to Instance.
   //
   PermanentHeap();

   //  Private to restrict deletion.
   //
   ~PermanentHeap();
};
}
#endif