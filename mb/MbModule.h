//==============================================================================
//
//  MbModule.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef MBMODULE_H_INCLUDED
#define MBMODULE_H_INCLUDED

#include "Module.h"
#include "NbTypes.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace MediaBase
{
//  Module for initializing MediaBase.
//
class MbModule : public Module
{
   friend class Singleton< MbModule >;

   //  Private because this singleton is not subclassed.
   //
   MbModule();

   //  Private because this singleton is not subclassed.
   //
   ~MbModule();

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
