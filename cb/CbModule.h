//==============================================================================
//
//  CbModule.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef CBMODULE_H_INCLUDED
#define CBMODULE_H_INCLUDED

#include "Module.h"
#include "NbTypes.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace CallBase
{
//  Module for initializing CallBase.
//
class CbModule : public Module
{
   friend class Singleton< CbModule >;
private:
   //  Private because this singleton is not subclassed.
   //
   CbModule();

   //  Private because this singleton is not subclassed.
   //
   ~CbModule();

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
