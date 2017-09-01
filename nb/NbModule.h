//==============================================================================
//
//  NbModule.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef NBMODULE_H_INCLUDED
#define NBMODULE_H_INCLUDED

#include "Module.h"
#include "NbTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Module for initializing NodeBase.
//
class NbModule : public Module
{
   friend class Singleton< NbModule >;
public:
   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;
private:
   //  Private because this singleton is not subclassed.
   //
   NbModule();

   //  Private because this singleton is not subclassed.
   //
   ~NbModule();

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
