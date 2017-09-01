//==============================================================================
//
//  Immutable.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "Immutable.h"
#include "Debug.h"
#include "Memory.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
fn_name Immutable_ctor = "Immutable.ctor";

Immutable::Immutable()
{
   Debug::ft(Immutable_ctor);
}

//------------------------------------------------------------------------------

fn_name Immutable_new1 = "Immutable.operator new";

void* Immutable::operator new(size_t size)
{
   Debug::ft(Immutable_new1);

   return Memory::Alloc(size, MemImm);
}

//------------------------------------------------------------------------------

fn_name Immutable_new2 = "Immutable.operator new[]";

void* Immutable::operator new[](size_t size)
{
   Debug::ft(Immutable_new2);

   return Memory::Alloc(size, MemImm);
}

//------------------------------------------------------------------------------

void Immutable::Patch(sel_t selector, void* arguments)
{
   Object::Patch(selector, arguments);
}
}