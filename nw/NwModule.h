//==============================================================================
//
//  NwModule.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef NWMODULE_H_INCLUDED
#define NWMODULE_H_INCLUDED

#include "Module.h"
#include "NbTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Module for initializing NodeBase.
//
class NwModule : public Module
{
   friend class Singleton< NwModule >;
public:
   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;
private:
   //  Private because this singleton is not subclassed.
   //
   NwModule();

   //  Private because this singleton is not subclassed.
   //
   ~NwModule();

   //  Overridden for restarts.
   //
   virtual void Startup(RestartLevel level) override;

   //  Overridden for restarts.
   //
   virtual void Shutdown(RestartLevel level) override;

   //  Registers the module before main() is entered.
   //
   static bool Register();

   //  Initialized by invoking Register.
   //
   static bool Registered;
};
}
#endif
