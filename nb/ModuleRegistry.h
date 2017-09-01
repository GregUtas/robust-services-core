//==============================================================================
//
//  ModuleRegistry.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
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
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;
private:
   //  Private because this singleton is not subclassed.
   //
   ModuleRegistry();

   //  Private because this singleton is not subclassed.
   //
   ~ModuleRegistry();

   //  Registers MODULE against MID.
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
   virtual void Startup(RestartLevel level) override;

   //  Overridden to shut down all modules.
   //
   virtual void Shutdown(RestartLevel level) override;

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
