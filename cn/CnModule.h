//==============================================================================
//
//  CnModule.h
//
//  Copyright (C) 2013-2022  Greg Utas
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

   //  Private because this is a singleton.
   //
   CnModule();

   //  Private because this is a singleton.
   //
   ~CnModule();

   //  Overridden for restarts.
   //
   void Shutdown(RestartLevel level) override;

   //  Overridden for restarts.
   //
   void Startup(RestartLevel level) override;
};
}
#endif
