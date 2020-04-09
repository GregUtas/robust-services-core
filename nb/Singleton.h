//==============================================================================
//
//  Singleton.h
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
#ifndef SINGLETON_H_INCLUDED
#define SINGLETON_H_INCLUDED

#include "Debug.h"
#include "Singletons.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Class template for singletons.  A singleton for MyClass is created and/or
//  accessed by
//    auto c = Singleton< MyClass >::Instance();
//  This has the side effect of creating the singleton if it doesn't yet exist.
//
//  MyClass must not define its constructor or destructor as public.  In this
//  way, it can only be created via its singleton template.  It must make this
//  template a friend class to enable access to the non-public constructor and
//  destructor:
//
//    class MyClass : public Base  // or some subclass of Base
//    {
//       friend class Singleton< MyClass >;
//    public:
//    private:        // or protected, if subclassing is allowed
//       MyClass();   // cannot have any arguments
//       ~MyClass();  // or virtual, if subclassing is allowed
//    };
//
//  The type of memory that a singleton wishes to use determines it ultimate
//  base class:
//    o MemTemporary: Temporary
//    o MemDynamic:   Dynamic
//    o MemProtected: Protected
//    o MemPermanent: Base, Object, or Permanent
//    o MemImmutable: Immutable
//  The reason for this is that the only other way to specify an object's memory
//  type is to use the override of Object.operator new that includes MemoryType.
//  This is not possible for a singleton, however, because Singleton.Instance
//  invokes the standard form of new, which means that the memory type must be
//  determine by one of the singleton's base classes.
//
template< class T > class Singleton
{
public:
   //  Creates the singleton if necessary and returns a pointer to it.
   //  An exception occurs if allocation fails, since most singletons
   //  are created during system initialization.
   //
   static T* Instance()
   {
      //  When initialization is being traced, tracing this function
      //  will create singletons used by TraceBuffer, so recheck for
      //  the singleton.
      //
      if(Instance_ != nullptr) return Instance_;
      Debug::ft(Singleton_Instance());
      if(Instance_ != nullptr) return Instance_;
      Instance_ = new T;
      auto reg = Singletons::Instance();
      reg->BindInstance((const Base**) &Instance_, Instance_->MemType());
      return Instance_;
   }

   //  Deletes the singleton if it exists.  In some cases, this may be
   //  invoked because the singleton is corrupt, with the intention of
   //  recreating it.  This will fail, however, if the call to delete
   //  traps and our static pointer is not cleared.  Even worse, this
   //  would leave a partially destructed object as the singleton.  It
   //  is therefore necessary to nullify the static pointer *before*
   //  calling delete, so that a new singleton can be created even if
   //  a trap occurs during deletion.
   //
   static void Destroy()
   {
      Debug::ft(Singleton_Destroy());
      if(Instance_ == nullptr) return;
      auto singleton = Instance_;
      auto reg = Singletons::Instance();
      reg->UnbindInstance((const Base**) &Instance_);
      Instance_ = nullptr;
      delete singleton;
   }

   //  Returns a pointer to the current singleton instance but does not
   //  create it.  This allows the premature creation of a singleton to
   //  be avoided during system initialization and restarts.
   //
   static T* Extant() { return Instance_; }
private:
   //  Creates the singleton.
   //
   Singleton() { Instance(); }

   //  Deletes the singleton.
   //
   ~Singleton() { Destroy(); }

   //  Declaring fn_name's at file scope in a template header causes an
   //  avalanche of link errors for multiply defined symbols.  Inlining
   //  these functions limits them to one instantiation.
   //
   inline static fn_name
      Singleton_Instance() { return "Singleton.Instance"; }
   inline static fn_name
      Singleton_Destroy()  { return "Singleton.Destroy"; }

   //  Pointer to the singleton instance.
   //
   static T* Instance_;
};
}

//  Initialization of the singleton instance.
//
template< class T > T* NodeBase::Singleton< T >::Instance_ = nullptr;

#endif
