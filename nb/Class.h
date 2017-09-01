//==============================================================================
//
//  Class.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef CLASS_H_INCLUDED
#define CLASS_H_INCLUDED

#include "Protected.h"
#include <cstddef>
#include <memory>
#include "RegCell.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Each subclass of Class is a singleton for a specific C++ class.  Each
//  singleton supports techniques such as Object Template, Quasi-Singleton,
//  Object Morphing, and a Concrete Factory for deserialization.  A class
//  that uses any of these techniques implements a subclass of Class.  The
//  framework provides PooledClass to support the techniques mentioned for
//  objects subclassed from Pooled.
//
class Class : public Protected
{
   friend class Registry< Class >;
public:
   //  Overridden by a subclass to call any of SetVptr, SetTemplate, and
   //  SetQuasiSingleton after its singleton is created.  The singleton is
   //  created during system initialization by
   //    Singleton< MyClass >::Instance()->Initialize();
   //  If a subclass uses all of these techniques, its Initialize function
   //  looks like this:
   //    auto obj1 = new MyObject(...);
   //    auto obj2 = new MyObject(...);
   //    SetVptr(*obj1);
   //    SetTemplate(*obj1);
   //    SetQuasiSingleton(*obj2);
   //
   virtual void Initialize();

   //  Creates an object using the Object Template technique.
   //  The Initialize function must have called SetTemplate.
   //
   virtual Object* Create();

   //  Called by a quasi-singleton's override of operator new.
   //
   virtual Object* GetQuasiSingleton();

   //  Called by a quasi-singleton's override of operator delete.
   //
   virtual void FreeQuasiSingleton(Object* obj);

   //  Returns the class's identifier.
   //
   ClassId Cid() const { return cid_.GetId(); }

   //  Returns the vptr_ for the objects that the class supports.
   //
   vptr_t GetVptr() const { return vptr_; }

   //  Returns the offset to cid_.
   //
   static ptrdiff_t CellDiff();

   //  Overridden to enumerate all objects that the class owns.
   //
   virtual void GetSubtended(Base* objects[], size_t& count) const override;

   //  Overridden to display member variables.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;
protected:
   //  Sets cid_ and size_.  SIZE is the size of the type of object that
   //  a subclass supports:
   //    MyClass::MyClass() : Class(MyClassId, sizeof(MyPooledObject)) { }
   //  Protected because this class is virtual.
   //
   Class(ClassId cid, size_t size);

   //  Deletes template_ and singleton_.  Protected because subclasses
   //  should be singletons.
   //
   virtual ~Class();

   //  Performs the equivalent of operator new on the object's class
   //  to allocate SIZE bytes when creating an object.  The default
   //  version invokes the global operator new.
   //
   virtual Object* New(size_t size);

   //  Sets vptr_. An Initialize function calls this to register its
   //  vptr if it supports morphing or deserialization. The vptr is
   //  read from OBJ, which may be deleted after calling this function.
   //
   bool SetVptr(Object& obj);

   //  Sets template_.  An Initialize function calls this to register
   //  its template if it uses Create to instantiate objects.  OBJ must
   //  not be deleted after calling this function, and it must not be
   //  the same object passed to SetQuasiSingleton.
   //
   bool SetTemplate(Object& obj);

   //  Sets quasi_ and singleton_.  An Initialize function calls this
   //  to create its initial quasi-singleton. OBJ must not be deleted
   //  after calling this function, and it must not be the same object
   //  passed to SetTemplate.
   //
   bool SetQuasiSingleton(Object& obj);
private:
   //   Checks that OBJ belongs to this class.
   //
   bool VerifyClass(const Object& obj) const;

   //  Overridden to prohibit copying.
   //
   Class(const Class& that);
   void operator=(const Class& that);

   //  The class's identifier.
   //
   RegCell cid_;

   //  The size of the class's objects.
   //
   const size_t size_;

   //  The vptr for this class's objects.
   //
   vptr_t vptr_;

   //  Used by Create to block-initialize a new object.
   //
   std::unique_ptr< Object > template_;

   //  Set if the class uses quasi-singletons.
   //
   bool quasi_;

   //  The quasi-singleton instance.
   //
   std::unique_ptr< Object > singleton_;
};
}
#endif
