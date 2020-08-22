//==============================================================================
//
//  PooledClass.h
//
//  Copyright (C) 2013-2020  Greg Utas
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
#ifndef POOLEDCLASS_H_INCLUDED
#define POOLEDCLASS_H_INCLUDED

#include "Class.h"
#include <cstddef>

namespace NodeBase
{
   class ObjectPool;
}

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Subclass of Class that supports PooledObjects.
//
class PooledClass : public Class
{
public:
   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
protected:
   //  Protected because subclasses should be singletons.
   //
   PooledClass(ClassId cid, size_t size);

   //  Protected because this class is virtual.
   //
   virtual ~PooledClass();

   //  Sets pool_.  A subclass' Initialize function calls this to
   //  set its associated ObjectPool subclass.
   //    auto pool = Singleton< MyObjectPool >::Instance();
   //    auto obj1 = new MyPooledObject(...);
   //    auto obj2 = new MyPooledObject(...);
   //    SetPool(*pool);
   //    SetVptr(*obj1);
   //    SetTemplate(*obj1);
   //    SetQuasiSingleton(*obj2);
   //
   bool SetPool(ObjectPool& pool);

   //  Overridden to call pool_->DeqBlock.
   //
   Object* New(size_t size) override;
private:
   //  The pool that manages this class's objects.
   //
   ObjectPool* pool_;
};
}
#endif
