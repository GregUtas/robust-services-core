//==============================================================================
//
//  ClassRegistry.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "ClassRegistry.h"
#include <ostream>
#include <string>
#include "Class.h"
#include "Debug.h"
#include "Formatters.h"
#include "SysTypes.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
fn_name ClassRegistry_ctor = "ClassRegistry.ctor";

ClassRegistry::ClassRegistry()
{
   Debug::ft(ClassRegistry_ctor);

   classes_.Init(MaxClassId + 1, Class::CellDiff(), MemProt);
}

//------------------------------------------------------------------------------

fn_name ClassRegistry_dtor = "ClassRegistry.dtor";

ClassRegistry::~ClassRegistry()
{
   Debug::ft(ClassRegistry_dtor);
}

//------------------------------------------------------------------------------

fn_name ClassRegistry_BindClass = "ClassRegistry.BindClass";

bool ClassRegistry::BindClass(Class& cls)
{
   Debug::ft(ClassRegistry_BindClass);

   return classes_.Insert(cls);
}

//------------------------------------------------------------------------------

void ClassRegistry::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Protected::Display(stream, prefix, options);

   stream << prefix << "classes [Object::ClassId]" << CRLF;
   classes_.Display(stream, prefix + spaces(2), options);
}

//------------------------------------------------------------------------------

Class* ClassRegistry::Lookup(ClassId cid) const
{
   return classes_.At(cid);
}

//------------------------------------------------------------------------------

void ClassRegistry::Patch(sel_t selector, void* arguments)
{
   Protected::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name ClassRegistry_UnbindClass = "ClassRegistry.UnbindClass";

void ClassRegistry::UnbindClass(Class& cls)
{
   Debug::ft(ClassRegistry_UnbindClass);

   classes_.Erase(cls);
}
}
