//==============================================================================
//
//  NtModule.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef NTMODULE_H_INCLUDED
#define NTMODULE_H_INCLUDED

#include "Module.h"
#include "NbTypes.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace NodeTools
{
//  Module for initializing NodeTools.
//
class NtModule : public Module
{
   friend class Singleton< NtModule >;
private:
   //  Private because this singleton is not subclassed.
   //
   NtModule();

   //  Private because this singleton is not subclassed.
   //
   ~NtModule();

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
