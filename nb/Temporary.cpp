//==============================================================================
//
//  Temporary.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "Temporary.h"
#include "Debug.h"
#include "Memory.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
fn_name Temporary_ctor = "Temporary.ctor";

Temporary::Temporary()
{
   Debug::ft(Temporary_ctor);
}

//------------------------------------------------------------------------------

fn_name Temporary_new1 = "Temporary.operator new";

void* Temporary::operator new(size_t size)
{
   Debug::ft(Temporary_new1);

   return Memory::Alloc(size, MemTemp);
}

//------------------------------------------------------------------------------

fn_name Temporary_new2 = "Temporary.operator new[]";

void* Temporary::operator new[](size_t size)
{
   Debug::ft(Temporary_new2);

   return Memory::Alloc(size, MemTemp);
}

//------------------------------------------------------------------------------

void Temporary::Patch(sel_t selector, void* arguments)
{
   Object::Patch(selector, arguments);
}
}