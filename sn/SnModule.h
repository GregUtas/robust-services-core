//==============================================================================
//
//  SnModule.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef SNMODULE_H_INCLUDED
#define SNMODULE_H_INCLUDED

#include "Module.h"
#include "NbTypes.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace ServiceNode
{
//  Module for initializing ServiceNode.
//
class SnModule : public Module
{
   friend class Singleton< SnModule >;
private:
   //  Private because this singleton is not subclassed.
   //
   SnModule();

   //  Private because this singleton is not subclassed.
   //
   ~SnModule();

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
