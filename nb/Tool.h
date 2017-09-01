//==============================================================================
//
//  Tool.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef TOOL_H_INCLUDED
#define TOOL_H_INCLUDED

#include "Immutable.h"
#include <cstddef>
#include <string>
#include "RegCell.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Base class for debugging tools.  See TraceRecord, TraceBufer, and NbTracer.
//
class Tool : public Immutable
{
   friend class Registry< Tool >;
public:
   //  Returns the tool's identifier.
   //
   id_t Tid() const { return tid_.GetId(); }

   //  Returns the character used in the >set tool command to enable or
   //  disable the tool.
   //
   char CliChar() const { return abbr_; }

   //  Returns true if the tool can safely be enabled.
   //
   bool IsSafe() const;

   //  Returns the tool's name.  Limited to 18 characters.
   //
   virtual const char* Name() const = 0;

   //  Returns an explanation of the tool's purpose.  Limited to 52 characters.
   //
   virtual const char* Expl() const = 0;

   //  Returns a string for displaying the tool's status.  The default version
   //  returns a string indicating whether the tool is on, and overrides must
   //  append to that string.
   //
   virtual std::string Status() const;

   //  Returns the offset to tid_.
   //
   static ptrdiff_t CellDiff();

   //  Overridden to display member variables.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;
protected:
   //  Adds the tool to ToolRegistry.  TID is the tool's identifier (see below).
   //  ABBR is the character for enabling and disabling the tool in the >set
   //  tool command.  If it is not printable, the CLI will not support the tool.
   //  SAFE is set if the tool is safe for use in the field.  Protected because
   //  this class is virtual.
   //
   Tool(FlagId tid, char abbr, bool safe);

   //  Removes the tool from ToolRegistry.  Virtual to allow subclassing.
   //  Protected because subclasses should be singletons.
   //
   virtual ~Tool();
private:
   //  Overridden to prohibit copying.
   //
   Tool(const Tool& that);
   void operator=(const Tool& that);

   //  The tool's identifier.
   //
   RegCell tid_;

   //  The character that enables or disables the tool in the >set tool command.
   //
   char abbr_;

   //  Whether the tool is safe for field use.
   //
   bool safe_;
};

//------------------------------------------------------------------------------
//
//  Trace filters determine what will be included in, or excluded from, a trace.
//
enum TraceFilter
{
   TraceAll,       // all activity
   TraceFaction,   // a specific faction
   TraceThread,    // a specific thread
   TracePeer,      // messages to/from a specific peer address
   TracePort,      // messages to/from a specific host port
   TraceFactory,   // incoming messages to a specific factory
   TraceProtocol,  // incoming messages in a specific protocol
   TraceSignal,    // incoming messages with a specific signal
   TraceService,   // creations of a specific ServiceSM
   TraceTimers     // work in timer registry
};
}
#endif
