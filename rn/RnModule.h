//==============================================================================
//
//  RnModule.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef RNMODULE_H_INCLUDED
#define RNMODULE_H_INCLUDED

#include "Module.h"
#include "NbTypes.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace RoutingNode
{
//  Module for initializing RoutingNode.
//
class RnModule : public Module
{
   friend class Singleton< RnModule >;
private:
   //  Private because this singleton is not subclassed.
   //
   RnModule();

   //  Private because this singleton is not subclassed.
   //
   ~RnModule();

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
