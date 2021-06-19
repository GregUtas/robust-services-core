//==============================================================================
//
//  DaemonRegistry.h
//
//  Copyright (C) 2013-2021  Greg Utas
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
#ifndef DAEMONREGISTRY_H_INCLUDED
#define DAEMONREGISTRY_H_INCLUDED

#include "Permanent.h"
#include "NbTypes.h"
#include "Registry.h"
#include "SysTypes.h"

namespace NodeBase
{
   class Daemon;
}

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Global registry for daemons.
//
class DaemonRegistry : public Permanent
{
   friend class Singleton< DaemonRegistry >;
   friend class Daemon;
public:
   //  Deleted to prohibit copying.
   //
   DaemonRegistry(const DaemonRegistry& that) = delete;

   //  Deleted to prohibit copy assignment.
   //
   DaemonRegistry& operator=(const DaemonRegistry& that) = delete;

   //  Returns the daemon identified by NAME.
   //
   Daemon* FindDaemon(fixed_string name) const;

   //  Returns the daemons in the registry.
   //
   const Registry< Daemon >& Daemons() const { return daemons_; }

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;

   //  Overridden to invoke all daemons.
   //
   void Shutdown(RestartLevel level) override;

   //  Overridden to invoke all daemons.
   //
   void Startup(RestartLevel level) override;
private:
   //  Private because this is a singleton.
   //
   DaemonRegistry();

   //  Private because this is a singleton.
   //
   ~DaemonRegistry();

   //  Adds DAEMON to the registry.
   //
   bool BindDaemon(Daemon& daemon);

   //  Removes DAEMON from the registry.
   //
   void UnbindDaemon(Daemon& daemon);

   //  The daemons in the registry.
   //
   Registry< Daemon > daemons_;
};
}
#endif
