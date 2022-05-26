//==============================================================================
//
//  Object.cpp
//
//  Copyright (C) 2013-2022  Greg Utas
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
#include "Object.h"
#include <ostream>
#include <string>
#include "Class.h"
#include "ClassRegistry.h"
#include "Debug.h"
#include "Formatters.h"
#include "Singleton.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
void Object::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Base::Display(stream, prefix, options);

   stream << prefix << "patchArea : " << strHex(patchArea_) << CRLF;
}

//------------------------------------------------------------------------------

Class* Object::GetClass() const
{
   Debug::ft("Object.GetClass");

   //  This is overridden by objects that belong to a Class.
   //
   return nullptr;
}

//------------------------------------------------------------------------------

Object::ClassId Object::GetClassId() const
{
   //  Look up this object's class and return its identifier.
   //
   auto c = GetClass();
   if(c == nullptr) return NIL_ID;
   return c->Cid();
}

//------------------------------------------------------------------------------

bool Object::GetClassInstanceId(ObjectId oid, Class*& cls, InstanceId& iid)
{
   //  OID contains an object's class and instance identifiers.
   //  Update C to its class and IID to its instance identifier.
   //
   ClassId cid = oid >> MaxInstanceIdLog2;
   cls = Singleton<ClassRegistry>::Instance()->Lookup(cid);
   if(cls == nullptr) return false;
   iid = oid & MaxInstanceId;
   return true;
}

//------------------------------------------------------------------------------

Object::InstanceId Object::GetInstanceId() const
{
   Debug::ft("Object.GetInstanceId");

   //  This is overridden by objects that have identifiers.
   //
   return NIL_ID;
}

//------------------------------------------------------------------------------

Object::ObjectId Object::GetObjectId() const
{
   //  Look up this object's class and return its identifier.
   //
   auto id = GetInstanceId();
   if(id == NIL_ID) return NIL_ID;
   auto c = GetClass();
   if(c == nullptr) return NIL_ID;
   return (c->Cid() << MaxInstanceIdLog2) + id;
}

//------------------------------------------------------------------------------

void Object::MorphTo(const Class& target)
{
   Debug::ft("Object.MorphTo");

   //  Change this object's vptr to that of the target class.
   //
   auto obj = reinterpret_cast<ObjectStruct*>(this);
   obj->vptr = target.GetVptr();
}
}
