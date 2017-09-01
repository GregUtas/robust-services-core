//==============================================================================
//
//  PbModule.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef PBMODULE_H_INCLUDED
#define PBMODULE_H_INCLUDED

#include "Module.h"
#include "NbTypes.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace PotsBase
{
//  Module for initializing PotsBase.
//
class PbModule : public Module
{
   friend class Singleton< PbModule >;
private:
   //  Private because this singleton is not subclassed.
   //
   PbModule();

   //  Private because this singleton is not subclassed.
   //
   ~PbModule();

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
