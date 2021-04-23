//==============================================================================
//
//  Module.h
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
#ifndef MODULE_H_INCLUDED
#define MODULE_H_INCLUDED

#include "Immutable.h"
#include <cstddef>
#include "NbTypes.h"
#include "RegCell.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  A module consists of interrelated software that provides some logical
//  capability.  It is implemented within its own namespace, which should
//  consist of a separate .h/.cpp pair for each major class.  One of these
//  pairs is a Module subclass that is invoked during restarts.  The term
//  "restart" refers to both system initialization (when the executable is
//  first launched) and reinitialization (to recover from a serious error).
//
//  Each module implements its singleton subclass of Module as follows:
//
//    class SomeModule : public Module
//    {
//       friend class Singleton< SomeModule >;
//    public:
//       virtual void Patch(sel_t selector, void* arguments) override;
//    private:
//       SomeModule() : Module()
//       {
//          //  Modules 1 to N are the ones on which this module depends.
//          //  Creating their singletons ensures that they will exist in
//          //  the module registry when the system initializes.  Because
//          //  each module creates the modules on which it depends before
//          //  it adds itself to the registry, the registry will contain
//          //  modules in the (partial) ordering of their dependencies.
//          //
//          Singleton< Module1 >::Instance();
//          //  ...
//          Singleton< ModuleN >::Instance();
//          Singleton< ModuleRegistry >::Instance()->BindModule(*this);
//       }
//
//       ~SomeModule() = default;
//       void Startup(RestartLevel level) override;
//       void Shutdown(RestartLevel level) override;
//    };
//
//  Later during initialization, ModuleRegistry::Startup handles most of
//  the system's initialization by invoking Startup on each module.  The
//  Startup function initializes the data required by the module when
//  the system starts to run.
//
//  The purpose of modules is to avoid the type of totally unstructured
//  main() that plagues so many systems.
//
class Module : public Immutable
{
   friend class Registry< Module >;
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

   //  Returns the module's identifier.
   //
   ModuleId Mid() const { return mid_.GetId(); }

   //  Overridden but does nothing.  Provided for tracing only.  Each subclass
   //  overrides this to create objects that need to exist before the system
   //  starts to perform work.  These are made ready for use so that initial
   //  payload transactions do not take more time than subsequent transactions.
   //
   void Startup(RestartLevel level) override;

   //  Overridden but does nothing.  Provided for tracing only.  Each subclass
   //  overrides this to deal with objects that will not survive the restart.
   //
   void Shutdown(RestartLevel level) override;

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
protected:
   //  Protected because this class is virtual.
   //
   Module();

   //  Removes the module from the global module registry.  Protected
   //  because subclasses should be singletons.
   //
   virtual ~Module();
private:
   //  The module's identifier.
   //
   RegCell mid_;
};
}
#endif
