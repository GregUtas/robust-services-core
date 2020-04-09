//==============================================================================
//
//  CliRegistry.h
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
#ifndef CLIREGISTRY_H_INCLUDED
#define CLIREGISTRY_H_INCLUDED

#include "Persistent.h"
#include <cstddef>
#include <iosfwd>
#include <string>
#include "NbTypes.h"
#include "Registry.h"

namespace NodeBase
{
   class CliIncrement;
}

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Global registry for CLI increments.
//
class CliRegistry : public Persistent
{
   friend class Singleton< CliRegistry >;
public:
   //  Adds INCR to the registry.
   //
   bool BindIncrement(CliIncrement& incr);

   //  Removes INCR from the registry.
   //
   void UnbindIncrement(CliIncrement& incr);

   //  Returns the increment registered against NAME, if any.
   //
   CliIncrement* FindIncrement(const std::string& name) const;

   //  Called by the CLI's INCRS command to show all registered increments.
   //
   void ListIncrements(std::ostream& stream) const;

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
private:
   //  Private because this singleton is not subclassed.
   //
   CliRegistry();

   //  Private because this singleton is not subclassed.
   //
   ~CliRegistry();

   //> The maximum number of increments that can register.
   //
   static const size_t MaxIncrements;

   //  The global registry of CLI increments.
   //
   Registry< CliIncrement > increments_;
};
}
#endif
