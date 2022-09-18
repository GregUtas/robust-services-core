//==============================================================================
//
//  ModuleRegistry.h
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
#ifndef MODULEREGISTRY_H_INCLUDED
#define MODULEREGISTRY_H_INCLUDED

#include "Immutable.h"
#include <memory>
#include <string>
#include "NbTypes.h"
#include "Registry.h"
#include "SysTypes.h"

namespace NodeBase
{
   class Module;
   class ModulesCfg;
}

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Global registry for modules.
//
class ModuleRegistry : public Immutable
{
   friend class Singleton<ModuleRegistry>;
   friend class Module;
   friend class NbModule;
   friend class InitThread;
   friend class RootThread;
public:
   //  Deleted to prohibit copying.
   //
   ModuleRegistry(const ModuleRegistry& that) = delete;

   //  Deleted to prohibit copy assignment.
   //
   ModuleRegistry& operator=(const ModuleRegistry& that) = delete;

   //  Registers MODULE against its ModuleId.
   //
   void BindModule(Module& module);

   //  Returns the modules in the registry.
   //
   const Registry<Module>& Modules() const { return modules_; }

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;

   //  Overridden to display each module.
   //
   size_t Summarize(std::ostream& stream, uint32_t selector) const override;
private:
   //  Private because this is a singleton.
   //
   ModuleRegistry();

   //  Private because this is a singleton.
   //
   ~ModuleRegistry();

   //  Removes MODULE from the registry.
   //
   void UnbindModule(Module& module);

   //  Restarts the system.
   //
   void Restart();

   //  Sets the level for an upcoming restart.
   //
   static void SetLevel(RestartLevel level);

   //  Returns the level for an upcoming restart.
   //
   static RestartLevel GetLevel();

   //  Returns the next restart level when a restart fails.
   //
   static RestartLevel NextLevel();

   //  Handles system shutdown.
   //
   static void CheckForExit();

   //  Enables modules from the OptionalModules configuration parameter
   //  when the system first initializes.
   //
   void EnableModules() const;

   //  Returns the module identified by SYMBOL.
   //
   Module* FindModule(const std::string& symbol) const;

   //  Overridden to shut down all modules.
   //
   void Shutdown(RestartLevel level) override;

   //  Overridden to start up all modules.
   //
   void Startup(RestartLevel level) override;

   //  The global registry of modules.
   //
   Registry<Module> modules_;

   //  The configuration parameter for the optional modules to be enabled.
   //
   std::unique_ptr<ModulesCfg> modulesCfg_;
};
}
#endif
