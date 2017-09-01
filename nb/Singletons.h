//==============================================================================
//
//  Singletons.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef SINGLETONS_H_INCLUDED
#define SINGLETONS_H_INCLUDED

#include "Permanent.h"
#include <cstddef>
#include "Array.h"
#include "SysTypes.h"

namespace NodeBase
{
   struct SingletonTuple;
}

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Global registry for singletons.  This simplifies restart software because
//  Shutdown functions do not have to nullify a singleton's Instance_ pointer
//  when a restart frees the heap in which the singleton was created.  When
//  Singleton.Instance creates a singleton, it adds it to this registry, which
//  saves the location of the Instance_ pointer and the type of memory used by
//  the singleton.  This allows all affected singleton Instance_ pointers to
//  be cleared by this registry's Shutdown function.
//
class Singletons : public Permanent
{
public:
   //  Returns the registry of singletons.
   //
   static Singletons* Instance();

   //  Adds INSTANCE, which uses TYPE, to the registry.
   //
   void BindInstance(const Base** addr, MemoryType type);

   //  Removes INSTANCE from the registry.
   //
   void UnbindInstance(const Base** addr);

   //  Overridden to display member variables.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for restarts.
   //
   virtual void Shutdown(RestartLevel level) override;

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;
private:
   //> The maximum size of the registry.
   //
   static const size_t MaxSingletons;

   //  Private because this singleton is not subclassed.
   //
   Singletons();

   //  Private because this singleton is not subclassed.
   //
   ~Singletons();

   //  Overridden to prohibit copying.
   //
   Singletons(const Singletons& that);
   void operator=(const Singletons& that);

   //  Information about each singleton.
   //
   Array< SingletonTuple > registry_;

   //  The registry of singletons.
   //
   static Singletons* Instance_;
};
}
#endif
