//==============================================================================
//
//  Module.h
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
#ifndef MODULE_H_INCLUDED
#define MODULE_H_INCLUDED

#include "Immutable.h"
#include <cstddef>
#include <string>
#include "NbTypes.h"
#include "RegCell.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  The purpose of modules is to avoid the type of totally unstructured
//  main() that plagues so many systems.
//
//  A module--the term as used here predates C++20 and is unrelated to its
//  usage--consists of interrelated software that provides some logical
//  capability.  The capability is often implemented in its own namespace
//  and should have a separate .h/.cpp pair for each major class.  One such
//  pair is a Module subclass that is invoked during restarts.  The term
//  "restart" refers to both system initialization (when the executable is
//  first launched) and reinitialization (to recover from a serious error).
//
//  Each module implements its singleton subclass of Module as follows:
//
//    class SomeModule : public Module
//    {
//       friend class Singleton<SomeModule>;
//    public:
//       virtual void Patch(sel_t selector, void* arguments) override;
//    private:
//       SomeModule() : Module()
//       {
//          //  Modules 1 to N are the ones that this module requires.
//          //  Creating their singletons ensures that they will exist in
//          //  the module registry when the system initializes.  Because
//          //  each module creates the modules on which it depends before
//          //  it adds itself to the registry, the registry will contain
//          //  modules in the (partial) ordering of their dependencies.
//          //
//          Singleton<Module1>::Instance();
//          //  ...
//          Singleton<ModuleN>::Instance();
//          Singleton<ModuleRegistry>::Instance()->BindModule(*this);
//       }
//
//       ~SomeModule() = default;
//
//       void Enable() override
//       {
//          //  Enable the modules that this one requires, followed by this
//          //  module.  This must be public if other modules require this
//          //  one.  The outline is similar to the constructor.
//          //
//          Singleton<Module1>::Instance()->Enable();
//          //  ...
//          Singleton<ModuleN>::Instance()->Enable();
//          Module::Enable();
//       }
//
//       void Startup(RestartLevel level) override;
//       void Shutdown(RestartLevel level) override;
//    };
//
//  Later during initialization, ModuleRegistry::Startup handles most of the
//  system's initialization by invoking Startup on each module that has been
//  enabled (see the Enable function, below).  A Startup function initializes
//  the data required by its module when the system initializes or restarts.
//
class Module : public Immutable
{
   friend class ModuleRegistry;
   friend class Registry<Module>;
public:
   //  Deleted to prohibit copying.
   //
   Module(const Module& that) = delete;

   //  Deleted to prohibit copy assignment.
   //
   Module& operator=(const Module& that) = delete;

   //> Highest valid module identifier.
   //
   static const ModuleId MaxId;

   //  Enables the module.  Overridden to invoke this function on modules
   //  that this one requires (the same ones that its constructor created),
   //  after which this version must be invoked.  Only invoked during a
   //  RestartReboot (initial launch), when NodeBase (the lowest layer)
   //  has initialized.  This function is invoked on each module whose
   //  Symbol() appears in the configuration parameter OptionalModules,
   //  which in turn causes it to enable the modules that it requires.
   //  If a module is not enabled as the result of this procedure, its
   //  Startup function is not invoked.  This allows a single executable
   //  with various optional capabilities to be built, and a subset of those
   //  capabilities to be enabled by the OptionalModules parameter.  The
   //  executable's capabilities can later be modified by editing that
   //  parameter in the configuration file and performing a reboot restart.
   //
   virtual void Enable();

   //  Returns the module's identifier.
   //
   ModuleId Mid() const { return mid_.GetId(); }

   //  Returns the module's symbol.
   //
   const std::string& Symbol() const { return symbol_; }

   //  Returns true if the module is enabled.
   //
   bool IsEnabled() const { return enabled_; }

   //  Returns the offset to mid_.
   //
   static ptrdiff_t CellDiff();

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;

   //  Overridden but does nothing.  Provided for tracing only.  Each subclass
   //  overrides this to deal with objects that will not survive the restart.
   //
   void Shutdown(RestartLevel level) override;

   //  Overridden but does nothing.  Provided for tracing only.  Each subclass
   //  overrides this to create objects that need to exist before the system
   //  starts to perform work.  These are made ready for use so that initial
   //  payload transactions do not take more time than subsequent transactions.
   //
   void Startup(RestartLevel level) override;
protected:
   //  Protected because this class is virtual.  SYMBOL is used by an optional
   //  module so that the OptionalModules configuration parameter can specify
   //  the subset of modules to enable in a build that includes all modules.
   //
   Module(c_string symbol = EMPTY_STR);

   //  Removes the module from the global module registry.  Protected
   //  because subclasses should be singletons.
   //
   virtual ~Module();
private:
   //  The module's identifier (location in ModuleRegistry).
   //
   RegCell mid_;

   //  The symbol for enabling the module if it is optional.
   //
   const std::string symbol_;

   //  Set if the module is enabled.
   //
   bool enabled_;
};
}
#endif
