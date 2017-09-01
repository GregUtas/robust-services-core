//==============================================================================
//
//  Permanent.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "Permanent.h"
#include "Debug.h"
#include "Memory.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
fn_name Permanent_ctor = "Permanent.ctor";

Permanent::Permanent()
{
   Debug::ft(Permanent_ctor);
}

//------------------------------------------------------------------------------

fn_name Permanent_new1 = "Permanent.operator new";

void* Permanent::operator new(size_t size)
{
   Debug::ft(Permanent_new1);

   return Memory::Alloc(size, MemPerm);
}

//------------------------------------------------------------------------------

fn_name Permanent_new2 = "Permanent.operator new[]";

void* Permanent::operator new[](size_t size)
{
   Debug::ft(Permanent_new2);

   return Memory::Alloc(size, MemPerm);
}

//------------------------------------------------------------------------------

void Permanent::Patch(sel_t selector, void* arguments)
{
   Object::Patch(selector, arguments);
}
}