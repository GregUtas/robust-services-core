//==============================================================================
//
//  CliStack.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef CLISTACK_H_INCLUDED
#define CLISTACK_H_INCLUDED

#include "Temporary.h"
#include <cstddef>
#include <memory>
#include <string>

namespace NodeBase
{
   class CliCommand;
   class CliIncrement;
}

//------------------------------------------------------------------------------

namespace NodeBase
{
//  For tracking active CLI increments.
//
class CliStack : public Temporary
{
   friend std::unique_ptr< CliStack >::deleter_type;
   friend class CliThread;
public:
   //  Returns the increment on top of the stack.
   //
   CliIncrement* Top() const;

   //  Removes the top increment from the stack.  Returns false if the
   //  stack was empty except for NbIncrement.
   //
   bool Pop();

   //  Searches the stack for a increment that supports COMM.  On success,
   //  the second version updates INCR to the increment to which the command
   //  belongs.
   //
   const CliCommand* FindCommand
      (const std::string& comm) const;
   const CliCommand* FindCommand
      (const std::string& comm, const CliIncrement*& incr) const;

   //  Overridden to display member variables.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;
private:
   //  Searches the stack for an increment whose name matches NAME.
   //
   CliIncrement* FindIncrement(const std::string& name) const;

   //> The maximum number of increments that can be active simultaneously.
   //
   static const size_t MaxIncrDepth = 16;

   //  Not subclassed.  Only created by CliThread.
   //
   CliStack();

   //  Not subclassed.  Only deleted by CliThread.
   //
   ~CliStack();

   //  Overridden to prohibit copying.
   //
   CliStack(const CliStack& that);
   void operator=(const CliStack& that);

   //  Adds the CLI's NbIncrement to the stack.
   //
   void SetRoot(CliIncrement& root);

   //  Adds INCR to the set of active increments.
   //
   bool Push(CliIncrement& incr);

   //  The stack of active increments.
   //
   CliIncrement* increments_[MaxIncrDepth + 1];

   //  The index to the top of the stack.
   //
   int top_;
};
}
#endif
