//==============================================================================
//
//  DipModule.h
//
//  Copyright (C) 2019  Greg Utas
//
//  Diplomacy AI Client - Part of the DAIDE project (www.daide.org.uk).
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
#ifndef DIPMODULE_H_INCLUDED
#define DIPMODULE_H_INCLUDED

#include "Module.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace Diplomacy
{
//  Module for initialising Diplomacy.
//
class DipModule : public Module
{
   friend class Singleton< DipModule >;

   //  Private because this singleton is not subclassed.
   //
   DipModule();

   //  Private because this singleton is not subclassed.
   //
   ~DipModule() = default;

   //  Overridden for restarts.
   //
   void Startup(RestartLevel level) override;

   //  Overridden for restarts.
   //
   void Shutdown(RestartLevel level) override;
};
}
#endif
