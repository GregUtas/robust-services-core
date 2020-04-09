//==============================================================================
//
//  ClassRegistry.cpp
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

   classes_.Init(MaxClassId, Class::CellDiff(), MemPersistent);
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
   Persistent::Display(stream, prefix, options);

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
   Persistent::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name ClassRegistry_UnbindClass = "ClassRegistry.UnbindClass";

void ClassRegistry::UnbindClass(Class& cls)
{
   Debug::ft(ClassRegistry_UnbindClass);

   classes_.Erase(cls);
}
}
