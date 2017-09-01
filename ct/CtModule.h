//==============================================================================
//
//  CtModule.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef CTMODULE_H_INCLUDED
#define CTMODULE_H_INCLUDED

#include "Module.h"
#include "NbTypes.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace CodeTools
{
//  Module for initializing CodeTools.
//
class CtModule : public Module
{
   friend class Singleton< CtModule >;
public:
   //  Overridden for restarts.
   //
   virtual void Shutdown(RestartLevel level) override;

   //  Overridden for restarts.
   //
   virtual void Startup(RestartLevel level) override;
private:
   //  Private because this singleton is not subclassed.
   //
   CtModule();

   //  Private because this singleton is not subclassed.
   //
   ~CtModule();

   //  Registers the module before main() is entered.
   //
   static bool Register();

   //  Initialized by invoking Register.
   //
   static bool Registered;
};
}
#endif
