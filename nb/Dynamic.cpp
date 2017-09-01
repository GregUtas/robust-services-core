//==============================================================================
//
//  Dynamic.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "Dynamic.h"
#include "Debug.h"
#include "Memory.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
fn_name Dynamic_ctor = "Dynamic.ctor";

Dynamic::Dynamic()
{
   Debug::ft(Dynamic_ctor);
}

//------------------------------------------------------------------------------

fn_name Dynamic_new1 = "Dynamic.operator new";

void* Dynamic::operator new(size_t size)
{
   Debug::ft(Dynamic_new1);

   return Memory::Alloc(size, MemDyn);
}

//------------------------------------------------------------------------------

fn_name Dynamic_new2 = "Dynamic.operator new[]";

void* Dynamic::operator new[](size_t size)
{
   Debug::ft(Dynamic_new2);

   return Memory::Alloc(size, MemDyn);
}

//------------------------------------------------------------------------------

void Dynamic::Patch(sel_t selector, void* arguments)
{
   Object::Patch(selector, arguments);
}
}