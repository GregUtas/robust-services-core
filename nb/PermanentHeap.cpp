//==============================================================================
//
//  PermanentHeap.cpp
//
//  Copyright (C) 2013-2020  Greg Utas
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
#include "PermanentHeap.h"
#include "Debug.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
PermanentHeap::PermanentHeap() : SysHeap()
{
   Debug::ftnt("PermanentHeap.ctor");
}

//------------------------------------------------------------------------------

PermanentHeap::~PermanentHeap()
{
   Debug::ftnt("PermanentHeap.dtor");
}

//------------------------------------------------------------------------------

PermanentHeap* PermanentHeap::Instance()
{
   static PermanentHeap heap;

   return &heap;
}

//------------------------------------------------------------------------------

void PermanentHeap::Patch(sel_t selector, void* arguments)
{
   SysHeap::Patch(selector, arguments);
}
}
