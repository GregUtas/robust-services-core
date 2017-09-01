//==============================================================================
//
//  ToolRegistry.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef TOOLREGISTRY_H_INCLUDED
#define TOOLREGISTRY_H_INCLUDED

#include "Immutable.h"
#include <cstddef>
#include "NbTypes.h"
#include "Registry.h"
#include "SysTypes.h"

namespace NodeBase
{
   class Tool;
}

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Global registry for Tool subclasses.
//
class ToolRegistry : public Immutable
{
   friend class Singleton< ToolRegistry >;
public:
   //  Adds TOOL to the registry.
   //
   bool BindTool(Tool& tool);

   //  Removes TOOL from the registry.
   //
   void UnbindTool(Tool& tool);

   //  Returns the tool registered against ID.
   //
   Tool* GetTool(FlagId id) const;

   //  Returns the tool, if any, whose CLI character is ABBR.
   //
   Tool* FindTool(char abbr) const;

   //  Returns the registry of tools.  Used for iteration.
   //
   const Registry< Tool >& Tools() const { return tools_; }

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
   ToolRegistry();

   //  Private because this singleton is not subclassed.
   //
   ~ToolRegistry();

   //> The maximum number of increments that can register.
   //
   static const size_t MaxTools;

   //  The global registry of CLI increments.
   //
   Registry< Tool > tools_;
};
}
#endif
