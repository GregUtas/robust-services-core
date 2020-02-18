//==============================================================================
//
//  PermanentHeap.h
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

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
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
