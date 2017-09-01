//==============================================================================
//
//  OnModule.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef ONMODULE_H_INCLUDED
#define ONMODULE_H_INCLUDED

#include "Module.h"
#include "NbTypes.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace OperationsNode
{
//  Module for initializing OperationsNode.
//
class OnModule : public Module
{
   friend class Singleton< OnModule >;
private:
   //  Private because this singleton is not subclassed.
   //
   OnModule();

   //  Private because this singleton is not subclassed.
   //
   ~OnModule();

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
