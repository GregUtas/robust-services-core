//==============================================================================
//
//  Protected.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "Protected.h"
#include "Debug.h"
#include "Memory.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
fn_name Protected_ctor = "Protected.ctor";

Protected::Protected()
{
   Debug::ft(Protected_ctor);
}

//------------------------------------------------------------------------------

fn_name Protected_new1 = "Protected.operator new";

void* Protected::operator new(size_t size)
{
   Debug::ft(Protected_new1);

   return Memory::Alloc(size, MemProt);
}

//------------------------------------------------------------------------------

fn_name Protected_new2 = "Protected.operator new[]";

void* Protected::operator new[](size_t size)
{
   Debug::ft(Protected_new2);

   return Memory::Alloc(size, MemProt);
}

//------------------------------------------------------------------------------

void Protected::Patch(sel_t selector, void* arguments)
{
   Object::Patch(selector, arguments);
}
}