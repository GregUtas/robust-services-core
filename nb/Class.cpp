//==============================================================================
//
//  Class.cpp
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
#include "Class.h"
#include <bitset>
#include <new>
#include <ostream>
#include <string>
#include "Algorithms.h"
#include "AllocationException.h"
#include "ClassRegistry.h"
#include "Debug.h"
#include "Formatters.h"
#include "Memory.h"
#include "NbTypes.h"
#include "Singleton.h"
#include "SysTypes.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
fn_name Class_ctor = "Class.ctor";

Class::Class(ClassId cid, size_t size) :
   size_(size),
   vptr_(BAD_POINTER),
   template_(nullptr),
   quasi_(false),
   singleton_(nullptr)
{
   Debug::ft(Class_ctor);

   cid_.SetId(cid);
   Singleton< ClassRegistry >::Instance()->BindClass(*this);
}

//------------------------------------------------------------------------------

fn_name Class_dtor = "Class.dtor";

Class::~Class()
{
   Debug::ft(Class_dtor);

   Singleton< ClassRegistry >::Instance()->UnbindClass(*this);
}

//------------------------------------------------------------------------------

ptrdiff_t Class::CellDiff()
{
   int local;
   auto fake = reinterpret_cast< const Class* >(&local);
   return ptrdiff(&fake->cid_, fake);
}

//------------------------------------------------------------------------------

fn_name Class_Create = "Class.Create";

Object* Class::Create()
{
   Debug::ft(Class_Create);

   Object* obj;

   //  Get a block for a new object and use its template to initialize it.
   //  Call PostInitialize to set any members that depend on run-time data.
   //
   if(template_ == nullptr)
   {
      Debug::SwLog(Class_Create, Cid(), 0);
      return nullptr;
   }

   if(quasi_)
      obj = GetQuasiSingleton();
   else
      obj = New(size_);

   if(obj != nullptr)
   {
      Memory::Copy(obj, template_.get(), size_);
      obj->PostInitialize();
   }

   return obj;
}

//------------------------------------------------------------------------------

void Class::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Protected::Display(stream, prefix, options);

   stream << prefix << "cid   : " << cid_.to_str() << CRLF;
   stream << prefix << "size  : " << size_ << CRLF;
   stream << prefix << "vptr  : " << vptr_ << CRLF;
   stream << prefix << "quasi : " << quasi_ << CRLF;

   stream << prefix << "template : ";

   if((template_ != nullptr) && options.test(DispVerbose))
   {
      stream << CRLF;
      template_->Display(stream, prefix + spaces(2), options);
   }
   else
   {
      stream << strObj(template_.get()) << CRLF;
   }

   stream << prefix << "singleton : ";

   if((singleton_ != nullptr) && options.test(DispVerbose))
   {
      stream << CRLF;
      singleton_->Display(stream, prefix + spaces(2), options);
   }
   else
   {
      stream << strObj(singleton_.get()) << CRLF;
   }
}

//------------------------------------------------------------------------------

fn_name Class_FreeQuasiSingleton = "Class.FreeQuasiSingleton";

void Class::FreeQuasiSingleton(Object* obj)
{
   Debug::ft(Class_FreeQuasiSingleton);

   //  If a quasi-singleton is already available, return OBJ to its pool,
   //  else make it the quasi-singleton.
   //
   if(singleton_ == nullptr)
      singleton_.reset(obj);
   else
      delete obj;
}

//------------------------------------------------------------------------------

fn_name Class_GetQuasiSingleton = "Class.GetQuasiSingleton";

Object* Class::GetQuasiSingleton()
{
   Debug::ft(Class_GetQuasiSingleton);

   //  If the quasi-singleton is available, return it, else allocate a block
   //  from the pool associated with this class.
   //
   if(singleton_ != nullptr) return singleton_.release();
   return New(size_);
}

//------------------------------------------------------------------------------

fn_name Class_GetSubtended = "Class.GetSubtended";

void Class::GetSubtended(Base* objects[], size_t& count) const
{
   Debug::ft(Class_GetSubtended);

   Protected::GetSubtended(objects, count);

   if(template_ != nullptr) template_->GetSubtended(objects, count);
   if(singleton_ != nullptr) singleton_->GetSubtended(objects, count);
}

//------------------------------------------------------------------------------

fn_name Class_Initialize = "Class.Initialize";

void Class::Initialize()
{
   Debug::ft(Class_Initialize);

   //  Subclasses override this to call Set functions.  Consequently,
   //  it should not be called.
   //
   Debug::SwLog(Class_Initialize, Cid(), 0);
}

//------------------------------------------------------------------------------

fn_name Class_New = "Class.New";

Object* Class::New(size_t size)
{
   Debug::ft(Class_New);

   //e This needs to support memory types.
   //
   auto addr = ::operator new(size, std::nothrow);
   if(addr != nullptr) return (Object*) addr;
   throw AllocationException(MemPerm, size);
}

//------------------------------------------------------------------------------

void Class::Patch(sel_t selector, void* arguments)
{
   Protected::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name Class_SetQuasiSingleton = "Class.SetQuasiSingleton";

bool Class::SetQuasiSingleton(Object& obj)
{
   Debug::ft(Class_SetQuasiSingleton);

   //  Verify OBJ and make it the quasi-singleton if it wasn't already
   //  registered as this class's template.
   //
   if(!VerifyClass(obj)) return false;

   if(&obj == template_.get())
   {
      Debug::SwLog(Class_SetQuasiSingleton, Cid(), 0);
      return false;
   }

   singleton_.reset(&obj);
   quasi_ = true;
   return true;
}

//------------------------------------------------------------------------------

fn_name Class_SetTemplate = "Class.SetTemplate";

bool Class::SetTemplate(Object& obj)
{
   Debug::ft(Class_SetTemplate);

   //  Verify OBJ and make it the template if it wasn't already registered
   //  as this class's quasi-singleton.
   //
   if(!VerifyClass(obj)) return false;

   if(&obj == singleton_.get())
   {
      Debug::SwLog(Class_SetTemplate, Cid(), 0);
      return false;
   }

   template_.reset(&obj);
   return true;
}

//------------------------------------------------------------------------------

fn_name Class_SetVptr = "Class.SetVptr";

bool Class::SetVptr(Object& obj)
{
   Debug::ft(Class_SetVptr);

   //  Verify OBJ and save its vptr.
   //
   if(!VerifyClass(obj)) return false;

   vptr_ = obj.Vptr();
   return true;
}

//------------------------------------------------------------------------------

fn_name Class_VerifyClass = "Class.VerifyClass";

bool Class::VerifyClass(const Object& obj) const
{
   Debug::ft(Class_VerifyClass);

   auto c = obj.GetClass();

   if(c == nullptr)
   {
      Debug::SwLog(Class_VerifyClass, Cid(), 0);
      return false;
   }

   if(c != this)
   {
      Debug::SwLog(Class_VerifyClass, Cid(), 1);
      return false;
   }

   return true;
}
}
