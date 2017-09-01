//==============================================================================
//
//  ClassRegistry.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef CLASSREGISTRY_H_INCLUDED
#define CLASSREGISTRY_H_INCLUDED

#include "Protected.h"
#include "NbTypes.h"
#include "Registry.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Global registry for classes.
//
class ClassRegistry : public Protected
{
   friend class Singleton< ClassRegistry >;
public:
   //  Adds CLS to the registry.
   //
   bool BindClass(Class& cls);

   //  Removes CLS from the registry.
   //
   void UnbindClass(Class& cls);

   //  Returns the class registered against CID.
   //
   Class* Lookup(ClassId cid) const;

   //  Overridden to display member variables.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;
private:
   //  Private because this singleton is not subclassed.
   //
   ClassRegistry();

   //  Private because this singleton is not subclassed.
   //
   ~ClassRegistry();

   //  The global registry of classes.
   //
   Registry< Class > classes_;
};
}
#endif
