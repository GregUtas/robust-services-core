//==============================================================================
//
//  Debug.h
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
#ifndef DEBUG_H_INCLUDED
#define DEBUG_H_INCLUDED

#include <string>
#include "SysTypes.h"

namespace NodeBase
{
   class Base;
}

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Severity of software logs.
//
enum SwLogLevel
{
   SwInfo,     // a basic debug log
   SwWarning,  // a log that includes a stack trace
   SwError     // throws an exception (which includes a stack trace)
};

//  Used in software logs when a function invocation was unexpected.
//
extern fixed_string UnexpectedInvocation;

//  Used in software logs when a function override should have been provided.
//  Returns the string "override not found in " + strClass(obj, ns).
//
std::string strOver(const Base* obj, bool ns = true);

//  Interface for generating debug information.  This interface must restrict
//  its use of headers so that it can be used by low level class templates.
//
class Debug
{
   friend class Thread;
   friend class ThreadAdmin;
   friend class TraceBuffer;
public:
   //  Deleted because this class only has static members.
   //
   Debug() = delete;

   //  Any function that wishes to be included in a function trace must call
   //  Debug::ft(ClassName_FunctionName) as its first line of code, where the
   //  argument is defined as
   //
   //    fn_name ClassName_FunctionName = "ClassName.FunctionName";
   //
   //  at FILE SCOPE.  The pointer to this string is saved, so it must remain
   //  allocated!  DO NOT PASS SOMETHING THAT CAN BE DELETED.
   //
   //  Even small, "trivial" functions should be included in a function trace,
   //  as this can help to determine the execution flow when analyzing a trace.
   //  A function should only be omitted if it is invoked so frequently that it
   //  would fill the trace with noise.  "Get" functions usually fall into this
   //  category.
   //
   static void ft(fn_name_arg func);

   //  Generates a software log.  FUNC is the function's exact name, in the
   //  same form as that used for Debug::ft above.  ERRVAL/ERRSTR provides
   //  debug information.  OFFSET is often a sequence number within a function,
   //  which makes it easy to see which occurrence of SwLog was invoked, but in
   //  some cases it provides debug information instead.  LEVEL specifies the
   //  severity of the log:
   //  o SwInfo generates a basic log
   //  o SwWarning includes a stack trace in the log
   //  o SwError throws an exception to clean up the work in progress
   //
   static void SwLog(fn_name_arg func, debug64_t errval,
      debug64_t offset, SwLogLevel level = SwWarning);
   static void SwLog(fn_name_arg func, const std::string& errstr,
      debug64_t offset, SwLogLevel level = SwWarning);

   //  Throws an exception if CONDITION is false.  ERRVAL is for debugging.
   //
   static void Assert(bool condition, debug64_t errval = 0);

   //  If FORCE or the ShowToolProgress is set, invokes CoutThread::Spool(s)
   //  followed by Thread::Pause(10).
   //
   static void Progress(const std::string& s, bool force = false);

   //  Does nothing.  Useful for defining a breakpoint or tracepoint.
   //
   static void noop();

   //  Invoked by functions that are (transitively) invoked by
   //  Debug::ft.  Such functions must *not* invoke Debug::ft;
   //  doing so will definitely cause a stack overflow.
   //
   static void noft() { }

   //  Returns true if a trace tool is currently active.
   //
   static bool TraceOn() { return FcFlags_.test(TracingActive); }

   //  Returns true if the software flag identified by FID is on.
   //  Always returns false unless Element::RunningInLab() is true.
   //
   static bool SwFlagOn(FlagId fid);

   //  Sets the software flag identified by FID to VALUE.
   //
   static void SetSwFlag(FlagId fid, bool value);

   //  Clears all flags.
   //
   static void ResetSwFlags();

   //  Returns the entire set of flags.  Note that this is a copy.
   //
   static Flags GetSwFlags();
private:
   //  Flags that define actions performed when a function is invoked.
   //
   enum InvocationFlag
   {
      TracingActive,  // set when tracing is active
      TrapPending,    // set when a Raise() is pending on any thread
      StackChecking   // set when stack overflow prevention is active
   };

   //  Used by the various versions of SwLog.
   //
   static void GenerateSwLog(fn_name_arg func, const std::string& errstr,
      debug64_t offset, SwLogLevel level);

   //  Flags for controlling the behavior of software during testing.
   //
   static Flags SwFlags_;

   //  Flags associated with a function call.
   //
   static Flags FcFlags_;
};
}
#endif
