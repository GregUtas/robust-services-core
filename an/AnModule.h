//==============================================================================
//
//  AnModule.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef ANMODULE_H_INCLUDED
#define ANMODULE_H_INCLUDED

#include "Module.h"
#include "NbTypes.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace AccessNode
{
//  Module for initializing AccessNode.
//
class AnModule : public Module
{
   friend class Singleton< AnModule >;
private:
   //  Private because this singleton is not subclassed.
   //
   AnModule();

   //  Private because this singleton is not subclassed.
   //
   ~AnModule();

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
