//==============================================================================
//
//  MbModule.h
//
//  Copyright (C) 2017  Greg Utas
//
//  This file is part of the Robust Services Core (RSC).
//
//  RSC is free software: you can redistribute it and/or modify it under the
//  terms of the GNU General Public License as published by the Free Software
//  Foundation, either version 3 of the License, or (at your option) any later
//  version.
//
//  RSC is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
//  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
//  details.
//
//  You should have received a copy of the GNU General Public License along
//  with RSC.  If not, see <http://www.gnu.org/licenses/>.
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
   void Startup(RestartLevel level) override;

   //  Overridden for restarts.
   //
   void Shutdown(RestartLevel level) override;

   //  Registers the module before main() is entered.
   //
   static bool Register();

   //  Initialized by invoking Register.
   //
   static bool Registered;
};
}
#endif
