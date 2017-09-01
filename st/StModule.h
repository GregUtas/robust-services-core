//==============================================================================
//
//  StModule.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef STMODULE_H_INCLUDED
#define STMODULE_H_INCLUDED

#include "Module.h"
#include "NbTypes.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace SessionTools
{
//  Module for initializing SessionTools.
//
class StModule : public Module
{
   friend class Singleton< StModule >;
private:
   //  Private because this singleton is not subclassed.
   //
   StModule();

   //  Private because this singleton is not subclassed.
   //
   ~StModule();

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
