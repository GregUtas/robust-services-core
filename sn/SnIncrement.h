//==============================================================================
//
//  SnIncrement.h
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
#ifndef SNINCREMENT_H_INCLUDED
#define SNINCREMENT_H_INCLUDED

#include "CliIncrement.h"

using namespace NodeBase;
#include "NbTypes.h"

//------------------------------------------------------------------------------

namespace PotsBase
{
//  The increment that provides POTS commands.
//
class SnIncrement : public CliIncrement
{
   friend class Singleton< SnIncrement >;
private:
   //  Private because this singleton is not subclassed.
   //
   SnIncrement();

   //  Private because this singleton is not subclassed.
   //
   ~SnIncrement();
};
}
#endif