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
#include "Permanent.h"
#include <bitset>
#include <cstdint>
#include <ostream>
#include <string>
#include "Algorithms.h"
#include "ClassRegistry.h"
#include "Debug.h"
#include "Formatters.h"
#include "FunctionGuard.h"
#include "Memory.h"
#include "NbTypes.h"
#include "Restart.h"
#include "Singleton.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Data that changes too frequently to unprotect and reprotect memory
//  when it needs to be modified.
//
struct ClassDynamic : public Permanent
{
   //  Constructor.
   //
   ClassDynamic() = default;

   //  Used by Create to block-initialize a new object.
   //
   std::unique_ptr< Object > template_;

   //  The quasi-singleton instance.
   //
   std::unique_ptr< Object > singleton_;
};

//==============================================================================

fn_name Class_ctor = "Class.ctor";

Class::Class(ClassId cid, size_t size) :
   size_(size),
   vptr_(BAD_POINTER)
{
   Debug::ft(Class_ctor);

   dyn_.reset(new ClassDynamic);
   cid_.SetId(cid);
   Singleton< ClassRegistry >::Instance()->BindClass(*this);
}

//------------------------------------------------------------------------------

fn_name Class_dtor = "Class.dtor";

Class::~Class()
{
   Debug::ft(Class_dtor);

   Debug::SwLog(Class_dtor, UnexpectedInvocation, 0);
   Singleton< ClassRegistry >::Instance()->UnbindClass(*this);
}

//------------------------------------------------------------------------------

ptrdiff_t Class::CellDiff()
{
   uintptr_t local;
   auto fake = reinterpret_cast< const Class* >(&local);
   return ptrdiff(&fake->cid_, fake);
}

//------------------------------------------------------------------------------

fn_name Class_ClaimBlocks = "Class.ClaimBlocks";

void Class::ClaimBlocks()
{
   Debug::ft(Class_ClaimBlocks);

   if(dyn_->template_ != nullptr) dyn_->template_->ClaimBlocks();
   if(dyn_->singleton_ != nullptr) dyn_->singleton_->ClaimBlocks();
}

//------------------------------------------------------------------------------

fn_name Class_Create = "Class.Create";

Object* Class::Create()
{
   Debug::ft(Class_Create);

   //  Get a block for a new object and use its template to initialize it.
   //  Call PostInitialize to set any members that depend on run-time data.
   //
   if(dyn_->template_ == nullptr)
   {
      Debug::SwLog(Class_Create, "null template", Cid());
      return nullptr;
   }

   Object* obj;

   if(dyn_->singleton_ != nullptr)
      obj = dyn_->singleton_.release();
   else
      obj = New(size_);

   if(obj != nullptr)
   {
      Memory::Copy(obj, dyn_->template_.get(), size_);
      obj->PostInitialize();
   }

   return obj;
}

//------------------------------------------------------------------------------

void Class::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Immutable::Display(stream, prefix, options);

   stream << prefix << "cid  : " << cid_.to_str() << CRLF;
   stream << prefix << "size : " << size_ << CRLF;
   stream << prefix << "vptr : " << vptr_ << CRLF;

   stream << prefix << "template : ";

   if((dyn_->template_ != nullptr) && options.test(DispVerbose))
   {
      stream << CRLF;
      dyn_->template_->Display(stream, prefix + spaces(2), options);
   }
   else
   {
      stream << strObj(dyn_->template_.get()) << CRLF;
   }

   stream << prefix << "singleton : ";

   if((dyn_->singleton_ != nullptr) && options.test(DispVerbose))
   {
      stream << CRLF;
      dyn_->singleton_->Display(stream, prefix + spaces(2), options);
   }
   else
   {
      stream << strObj(dyn_->singleton_.get()) << CRLF;
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
   if(dyn_->singleton_ == nullptr)
      dyn_->singleton_.reset(obj);
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
   if(dyn_->singleton_ != nullptr)
   {
      return dyn_->singleton_.release();
   }

   return New(size_);
}

//------------------------------------------------------------------------------

fn_name Class_Initialize = "Class.Initialize";

void Class::Initialize()
{
   Debug::ft(Class_Initialize);

   Debug::SwLog(Class_Initialize, strOver(this), Cid());
}

//------------------------------------------------------------------------------

fn_name Class_New = "Class.New";

Object* Class::New(size_t size)
{
   Debug::ft(Class_New);

   auto type = MemType();
   auto addr = Memory::Alloc(size, type);
   return (Object*) addr;
}

//------------------------------------------------------------------------------

fn_name Class_ObjType = "Class.ObjType";

MemoryType Class::ObjType() const
{
   Debug::ft(Class_ObjType);

   Debug::SwLog(Class_ObjType, strOver(this), 0);
   return MemNull;
}

//------------------------------------------------------------------------------

void Class::Patch(sel_t selector, void* arguments)
{
   Immutable::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name Class_SetQuasiSingleton = "Class.SetQuasiSingleton";

bool Class::SetQuasiSingleton(Object& obj)
{
   Debug::ft(Class_SetQuasiSingleton);

   //  Verify OBJ and make it the quasi-singleton if it wasn't already
   //  registered as this class's template.
   //
   if(ObjType() != MemDynamic) return false;

   if(!VerifyClass(obj)) return false;

   if(&obj == dyn_->template_.get())
   {
      Debug::SwLog(Class_SetQuasiSingleton, "object is template", Cid());
      return false;
   }

   dyn_->singleton_.reset(&obj);
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

   if(&obj == dyn_->singleton_.get())
   {
      Debug::SwLog(Class_SetTemplate, "object is quasi-singleton", Cid());
      return false;
   }

   dyn_->template_.reset(&obj);
   return true;
}

//------------------------------------------------------------------------------

fn_name Class_SetVptr = "Class.SetVptr";

bool Class::SetVptr(const Object& obj)
{
   Debug::ft(Class_SetVptr);

   //  Verify OBJ and save its vptr.
   //
   if(!VerifyClass(obj)) return false;

   FunctionGuard guard(Guard_ImmUnprotect);
   vptr_ = obj.Vptr();
   return true;
}

//------------------------------------------------------------------------------

fn_name Class_Shutdown = "Class.Shutdown";

void Class::Shutdown(RestartLevel level)
{
   Debug::ft(Class_Shutdown);

   Restart::Release(dyn_->template_);
   Restart::Release(dyn_->singleton_);
}

//------------------------------------------------------------------------------

fn_name Class_VerifyClass = "Class.VerifyClass";

bool Class::VerifyClass(const Object& obj) const
{
   Debug::ft(Class_VerifyClass);

   auto c = obj.GetClass();

   if(c == nullptr)
   {
      Debug::SwLog(Class_VerifyClass, "class not found", Cid());
      return false;
   }

   if(c != this)
   {
      Debug::SwLog(Class_VerifyClass, "unexpected class", Cid());
      return false;
   }

   return true;
}
}
