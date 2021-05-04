//==============================================================================
//
//  Singleton.h
//
//  Copyright (C) 2013-2021  Greg Utas
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
//  MyClass must define its constructor or destructor as private.  That way, it
//  can only be created via its singleton template.  It must make this template
//  a friend class to enable access to the private constructor and destructor:
//
//    class MyClass : public Base  // actually a *subclass* of Base: see below
//    {
//       friend class Singleton< MyClass >;
//    public:
//       // interface for clients
//    private:
//       MyClass();   // cannot have any arguments
//       ~MyClass();
//    };
//
//  The type of memory that a singleton wishes to use determines it ultimate
//  base class:
//    o MemTemporary:  Temporary
//    o MemDynamic:    Dynamic
//    o MemPersistent: Persistent
//    o MemProtected:  Protected
//    o MemPermanent:  Permanent
//    o MemImmutable:  Immutable
//
//  Singletons should be created during system initialization and restarts.
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
      //  The TraceBuffer singleton is created during initialization.
      //  If initialization is being traced when this code is entered
      //  for that purpose, invoking Debug::ft will create TraceBuffer,
      //  so it will have magically appeared when the original call to
      //  this function resumes execution.  We must therefore recheck
      //  for the singleton.
      //
      if(Instance_ != nullptr) return Instance_;
      Debug::ft(Singleton_Instance());
      if(Instance_ != nullptr) return Instance_;
      Instance_ = new T;
      auto reg = Singletons::Instance();
      auto type = Instance_->MemType();
      reg->BindInstance((const Base**) &Instance_, type);
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

   //  Declaring an fn_name at file scope in a template header causes an
   //  avalanche of link errors for multiply defined symbols.  Returning
   //  an fn_name from an inline function limits the string constant to a
   //  single occurrence, no matter how many template instances exist.
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
