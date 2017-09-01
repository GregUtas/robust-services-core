//==============================================================================
//
//  Module.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "Module.h"
#include <ostream>
#include <string>
#include "Algorithms.h"
#include "Debug.h"
#include "ModuleRegistry.h"
#include "Singleton.h"
#include "SysTypes.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
fn_name Module_ctor = "Module.ctor";

Module::Module(ModuleId mid)
{
   Debug::ft(Module_ctor);

   mid_.SetId(mid);
   Singleton< ModuleRegistry >::Instance()->BindModule(*this);
}

//------------------------------------------------------------------------------

fn_name Module_dtor = "Module.dtor";

Module::~Module()
{
   Debug::ft(Module_dtor);

   Singleton< ModuleRegistry >::Instance()->UnbindModule(*this);
}

//------------------------------------------------------------------------------

ptrdiff_t Module::CellDiff()
{
   int local;
   auto fake = reinterpret_cast< const Module* >(&local);
   return ptrdiff(&fake->mid_, fake);
}

//------------------------------------------------------------------------------

fn_name Module_Dependencies = "Module.Dependencies";

ModuleId* Module::Dependencies(size_t& count) const
{
   Debug::ft(Module_Dependencies);

   count = 0;
   return nullptr;
}

//------------------------------------------------------------------------------

void Module::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Immutable::Display(stream, prefix, options);

   stream << prefix << "mid : " << mid_.to_str() << CRLF;
}

//------------------------------------------------------------------------------

void Module::Patch(sel_t selector, void* arguments)
{
   Immutable::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name Module_Shutdown = "Module.Shutdown";

void Module::Shutdown(RestartLevel level)
{
   Debug::ft(Module_Shutdown);
}

//------------------------------------------------------------------------------

fn_name Module_Startup = "Module.Startup";

void Module::Startup(RestartLevel level)
{
   Debug::ft(Module_Startup);
}
}
