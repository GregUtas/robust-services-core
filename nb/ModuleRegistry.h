//==============================================================================
//
//  ModuleRegistry.h
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
#ifndef MODULEREGISTRY_H_INCLUDED
#define MODULEREGISTRY_H_INCLUDED

#include "Immutable.h"
#include <iosfwd>
#include "NbTypes.h"
#include "Registry.h"
#include "SysTypes.h"

namespace NodeBase
{
   class Module;
}

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Global registry for modules.
//
class ModuleRegistry : public Immutable
{
   friend class Singleton< ModuleRegistry >;
   friend class InitThread;
   friend class Module;
public:
   //  Returns the module registered against MID.
   //
   Module* GetModule(ModuleId mid) const;

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
   ModuleRegistry();

   //  Private because this singleton is not subclassed.
   //
   ~ModuleRegistry();

   //  Registers MODULE against its ModuleId.
   //
   void BindModule(Module& module);

   //  Removes MODULE from the registry.
   //
   void UnbindModule(Module& module);

   //  Restarts the system.
   //
   void Restart();

   //  Sets the reason for an upcoming restart.
   //
   void SetReason(reinit_t reason, debug32_t errval);

   //  Returns the next restart level when a restart fails.
   //
   static RestartLevel NextLevel();

   //  Determines the restart level when the system is in service.
   //
   RestartLevel CalcLevel() const;

   //  Overridden to start up all modules.
   //
   void Startup(RestartLevel level) override;

   //  Overridden to shut down all modules.
   //
   void Shutdown(RestartLevel level) override;

   //  Returns stream_, creating it if it doesn't exist.
   //
   std::ostringstream* Stream();

   //  The global registry of modules.
   //
   Registry< Module > modules_;

   //  The reason for a reinitialization or shutdown.
   //
   reinit_t reason_;

   //  An error value for debugging.
   //
   debug32_t errval_;

   //  A stream for recording the progress of system initialization.
   //
   ostringstreamPtr stream_;
};
}
#endif
