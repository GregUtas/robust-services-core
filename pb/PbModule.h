//==============================================================================
//
//  PbModule.h
//
//  Copyright (C) 2013-2020  Greg Utas
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

   //  Private because this is a singleton.
   //
   PbModule();

   //  Private because this is a singleton.
   //
   ~PbModule();

   //  Overridden for restarts.
   //
   void Startup(RestartLevel level) override;

   //  Overridden for restarts.
   //
   void Shutdown(RestartLevel level) override;
};
}
#endif
