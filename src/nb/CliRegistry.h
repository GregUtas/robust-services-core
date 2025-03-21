//==============================================================================
//
//  CliRegistry.h
//
//  Copyright (C) 2013-2025  Greg Utas
//
//  This file is part of the Robust Services Core (RSC).
//
//  RSC is free software: you can redistribute it and/or modify it under the
//  terms of the Lesser GNU General Public License as published by the Free
//  Software Foundation, either version 3 of the License, or (at your option)
//  any later version.
//
//  RSC is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
//  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
//  details.
//
//  You should have received a copy of the Lesser GNU General Public License
//  along with RSC.  If not, see <http://www.gnu.org/licenses/>.
//
#ifndef CLIREGISTRY_H_INCLUDED
#define CLIREGISTRY_H_INCLUDED

#include "Immutable.h"
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
class CliRegistry : public Immutable
{
   friend class Singleton<CliRegistry>;
   friend class CliIncrement;
public:
   //  Deleted to prohibit copying.
   //
   CliRegistry(const CliRegistry& that) = delete;

   //  Deleted to prohibit copy assignment.
   //
   CliRegistry& operator=(const CliRegistry& that) = delete;

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
   //  Private because this is a singleton.
   //
   CliRegistry();

   //  Private because this is a singleton.
   //
   ~CliRegistry();

   //  Adds INCR to the registry.
   //
   bool BindIncrement(CliIncrement& incr);

   //  Removes INCR from the registry.
   //
   void UnbindIncrement(CliIncrement& incr);

   //  The global registry of CLI increments.
   //
   Registry<CliIncrement> increments_;
};
}
#endif
