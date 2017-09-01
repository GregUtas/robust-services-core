//==============================================================================
//
//  Object.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "Object.h"
#include <ostream>
#include <string>
#include "Class.h"
#include "ClassRegistry.h"
#include "Debug.h"
#include "Formatters.h"
#include "Memory.h"
#include "Singleton.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
fn_name Object_ctor = "Object.ctor";

Object::Object() : patchArea_(0)
{
   Debug::ft(Object_ctor);
}

//------------------------------------------------------------------------------

void Object::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Base::Display(stream, prefix, options);

   stream << prefix << "patchArea : " << strHex(patchArea_) << CRLF;
}

//------------------------------------------------------------------------------

fn_name Object_GetClass = "Object.GetClass";

Class* Object::GetClass() const
{
   Debug::ft(Object_GetClass);

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
   //  Update C to its class and IID to its instance identifer.
   //
   ClassId cid = oid >> MaxInstanceIdLog2;
   cls = Singleton< ClassRegistry >::Instance()->Lookup(cid);
   if(cls == nullptr) return false;
   iid = oid & MaxInstanceId;
   return true;
}

//------------------------------------------------------------------------------

fn_name Object_GetInstanceId = "Object.GetInstanceId";

Object::InstanceId Object::GetInstanceId() const
{
   Debug::ft(Object_GetInstanceId);

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

fn_name Object_MemType = "Object.MemType";

MemoryType Object::MemType() const
{
   Debug::ft(Object_MemType);

   return Memory::Type(static_cast< const void* >(this));
}

//------------------------------------------------------------------------------

fn_name Object_MorphTo = "Object.MorphTo";

void Object::MorphTo(Class& target)
{
   Debug::ft(Object_MorphTo);

   //  Change this object's vptr to that of the target class.
   //
   auto obj = reinterpret_cast< ObjectStruct* >(this);
   obj->vptr = target.GetVptr();
}

//------------------------------------------------------------------------------

fn_name Object_delete1 = "Object.operator delete";

void Object::operator delete(void* addr)
{
   Debug::ft(Object_delete1);

   Memory::Free(addr);
}

//------------------------------------------------------------------------------

fn_name Object_delete2 = "Object.operator delete[]";

void Object::operator delete[](void* addr)
{
   Debug::ft(Object_delete2);

   Memory::Free(addr);
}

//------------------------------------------------------------------------------

fn_name Object_delete3 = "Object.operator delete(type)";

void Object::operator delete(void* addr, MemoryType type)
{
   Debug::ft(Object_delete3);

   Memory::Free(addr);
}

//------------------------------------------------------------------------------

fn_name Object_delete4 = "Object.operator delete[](type)";

void Object::operator delete[](void* addr, MemoryType type)
{
   Debug::ft(Object_delete4);

   Memory::Free(addr);
}

//------------------------------------------------------------------------------

fn_name Object_new1 = "Object.operator new";

void* Object::operator new(size_t size, MemoryType type)
{
   Debug::ft(Object_new1);

   return Memory::Alloc(size, type);
}

//------------------------------------------------------------------------------

fn_name Object_new2 = "Object.operator new[]";

void* Object::operator new[](size_t size, MemoryType type)
{
   Debug::ft(Object_new2);

   return Memory::Alloc(size, type);
}
}
