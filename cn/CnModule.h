//==============================================================================
//
//  CnModule.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef CNMODULE_H_INCLUDED
#define CNMODULE_H_INCLUDED

#include "Module.h"
#include "NbTypes.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace ControlNode
{
//  Module for initializing ControlNode.
//
class CnModule : public Module
{
   friend class Singleton< CnModule >;
private:
   //  Private because this singleton is not subclassed.
   //
   CnModule();

   //  Private because this singleton is not subclassed.
   //
   ~CnModule();

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
