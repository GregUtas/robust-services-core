//==============================================================================
//
//  CxxExecute.h
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
#ifndef CXXEXECUTE_H_INCLUDED
#define CXXEXECUTE_H_INCLUDED

#include "TraceRecord.h"
#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <set>
#include <string>
#include "CodeTypes.h"
#include "Cxx.h"
#include "CxxFwd.h"
#include "Debug.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace CodeTools
{
//  Base class for tracing execution.
//
class CxxTrace : public NodeBase::TraceRecord
{
public:
   //  Actions that are traced during execution.
   //
   enum Action
   {
      START_FILE,
      START_SCOPE,
      START_TEMPLATE,
      END_TEMPLATE,
      PUSH_OP,
      POP_OP,
      PUSH_ARG,
      POP_ARG,
      SET_AUTO,
      INCR_READS,
      INCR_WRITES,
      INCR_CALLS,
      EXECUTE,
      CLEAR,
      ERROR,
      Action_N
   };

   //  Virtual to allow subclassing.
   //
   virtual ~CxxTrace() = default;
protected:
   //  Creates a trace record of SIZE, for ACTION.  Protected because this
   //  class is virtual.
   //
   CxxTrace(size_t size, Action action);

   //  Overridden to display the trace record.
   //
   bool Display(std::ostream& stream, bool diff) override;
private:
   //  The line number associated with the trace record.
   //
   uint16_t line_;

   //  The last line number displayed.
   //
   static uint16_t Last_;
};

//------------------------------------------------------------------------------
//
//  A stack argument when "executing code" to track usages.
//
class StackArg
{
public:
   //  Constructs an argument for T.  Constructs a pointer to P if T is 1.
   //  CTOR indicates that the argument was created by a constructor call.
   //
   StackArg(CxxToken* t, TagCount p, bool ctor);

   //  Constructs an argument for T, which was just accessed by NAME.
   //
   StackArg(CxxToken* t, TypeName* name);

   //  Constructs an argument for T, which was accessed as VIA.NAME or
   //  VIA->NAME.  OP is the operator ("." or "->") that was used.
   //
   StackArg(CxxToken* t, TypeName* name, const StackArg& via, Cxx::Operator op);

   //  Constructs an argument for a function that will be invoked and
   //  that was accessed by NAME.
   //
   StackArg(Function* f, TypeName* name);

   //  Copy constructor.
   //
   StackArg(const StackArg& that) = default;

   //  Copy operator.
   //
   StackArg& operator=(const StackArg& that) = default;

   //  Invokes TypeString on ITEM and adjusts the result based on PTRS.
   //
   std::string TypeString(bool arg) const;

   //  Adjusts the level of indirection to the argument.
   //
   void DecrPtrs() { --ptrs_; }
   void IncrPtrs() { ++ptrs_; }

   //  Returns true if the indirection, address of, or array subscript
   //  operator was applied to the argument.
   //
   bool UsedIndirectly() const { return ptrs_ != 0; }

   //  Returns the level of indirection to the argument's underlying type.
   //  If ARRAYS is set, each array specification is treated as a pointer.
   //
   size_t Ptrs(bool arrays) const;

   //  Returns the number of arrays attached to the argument's underlying
   //  type.
   //
   size_t Arrays() const;

   //  Returns the number of reference tags attached to the underlying type.
   //
   size_t Refs() const;

   //  Returns true if the argument is indirect.
   //
   bool IsIndirect() const { return ((Ptrs(true) > 0) || (Refs() > 0)); }

   //  Clears name_ so that the argument can be pushed again.
   //
   StackArg& EraseName() { name = nullptr; return *this; }

   //  Records that name_ (if it exists) was used directly.
   //
   void SetAsDirect() const;

   //  Tags the argument as a member of the context class (the class
   //  whose non-static member function is currently being executed).
   //
   void SetAsMember() { member_ = true; }

   //  Sets the number of reference tags on the argument.
   //
   void SetRefs(TagCount r) { refs_ = r; }

   //  Tags the argument const.
   //
   void SetAsConst() { const_ = true; }

   //  Returns true if the argument is const.
   //
   bool IsConst() const { return const_; }

   //  Tags the argument as a const pointer.
   //
   void SetAsConstPtr() { constptr_ = true; }

   //  Makes the argument read-only, setting constptr_ instead
   //  of const_ if it is a pointer.
   //
   void SetAsReadOnly();

   //  Makes the argument writeable, resetting constptr_ instead
   //  of const_ if it is a pointer.
   //
   void SetAsWriteable();

   //  Tags the argument as mutable.
   //
   void SetAsMutable() { mutable_ = true; }

   //  Tags the argument as a temporary.
   //
   void SetAsTemporary();

   //  Returns true if the argument is read-only.  PASSED is set if the
   //  argument is being passed to a non-const reference or pointer.
   //  Returns const_ if PASSED is set or the argument is not a pointer,
   //  else returns constptr_.
   //
   bool IsReadOnly(bool passed) const;

   //  Tags the argument as a function to be invoked unless it
   //  was previously tagged as a "this" argument.
   //
   void SetInvoke() { if(!this_) invoke_ = true; }

   //  Returns true if the argument is a function to be invoked.
   //
   bool InvokeSet() const { return invoke_; }

   //  Specifies whether the argument as a "this" argument.
   //
   void SetAsThis(bool value) { this_ = value; }

   //  Tags the argument as an implicit "this" argument.
   //
   void SetAsImplicitThis() { this_ = true; implicit_ = true; }

   //  Returns true if the argument is a "this" argument.
   //
   bool IsThis() const { return this_; }

   //  Returns true if the argument is an implicit "this" argument.
   //
   bool IsImplicitThis() const { return (this_ && implicit_); }

   //  Returns true if the argument was created by a constructor.
   //
   bool WasConstructed() const { return ctor_; }

   //  Returns the numeric type associated with the item's root type.
   //
   Numeric NumericType() const;

   //  Returns true if the argument can be overloaded.
   //
   bool CanBeOverloaded() const;

   //  Returns true if the argument is a default constructor.  ARGS are
   //  the arguments being passed to the would-be constructor.
   //
   bool IsDefaultCtor(const StackArgVector& args) const;

   //  Sets this argument as the result of the current expression, which
   //  can then be used to determine the type for data declared as "auto".
   //
   void SetAsAutoType() const;

   //  If this item has the type "auto", sets its type to that which was
   //  most recently set by SetAsAutoType.  Returns false if that type is
   //  invalid.
   //
   void SetAutoType();

   //  Updates DATA, which has the type "auto", to the type most recently
   //  set by SetAsAutoType.  Returns false if that type is invalid.
   //
   static bool SetAutoTypeFor(const FuncData& data);

   //  Returns the level of compatibility when assigning THAT to this item.
   //  The TypeString for each item must be provided.
   //
   TypeMatch CalcMatchWith(const StackArg& that,
      const std::string& thisType, const std::string& thatType) const;

   //  Logs a warning if the item is not a bool.  Invoked when an operator
   //  or conditional expression expects a bool.
   //
   void CheckIfBool() const;

   //  Invokes WasRead on the actual item.
   //
   void WasRead() const;

   //  Invokes WasWritten on the actual item.
   //
   void WasWritten() const;

   //  Invokes DecrPtrs.  If the item is a member, handles constness
   //  for arrays as opposed pointers.
   //
   void WasIndexed();

   //  Invoked when THIS is assigned to THAT.  TYPE is the type of
   //  assignment that occurred.
   //
   void AssignedTo(const StackArg& that, AssignmentType type) const;

   //  Invokes SetNonConst on this->item (if INDEX is 0) or this->via_
   //  (if INDEX is 1).  Generates a log if item/via_ was const.
   //
   void SetNonConst(size_t index) const;

   //  Returns true if ITEM and PTRS match.
   //
   bool operator==(const StackArg& that) const;
   bool operator!=(const StackArg& that) const;

   //  Returns a string that can be written to the trace file or a log.
   //
   std::string Trace() const;

   //  What the argument refers to.
   //
   CxxToken* item;

   //  The name through which the argument was accessed.
   //
   TypeName* name;
private:
   //  Sets DATA's referent to this argument.  Returns FALSE on failure.
   //
   bool SetAutoTypeOn(const FuncData& data) const;

   //  Invoked by CalcMatchWith, once for THAT, and once for any types to
   //  which THAT can be converted.
   //
   TypeMatch MatchWith(const StackArg& that,
      const std::string& thisType, const std::string& thatType) const;

   //  Invoked to assess const compatibility when THAT is already known to
   //  be compatible with this argument.  Returns MATCH if the items are
   //  const compatible, and Adaptable if they are not.
   //
   TypeMatch MatchConst(const StackArg& that, TypeMatch match) const;

   //  Invoked to record that the context function cannot be const.
   //
   static void ContextFunctionIsNonConst();

   //  The item through which this one was accessed.
   //
   CxxToken* via_;

   //  The level of pointer indirection to the argument, which can actually
   //  be negative (see usages of DecrPtrs).  The net level of indirection
   //  to ITEM's underlying type is returned by Ptrs (above), which cannot
   //  return a negative count.
   //
   TagCount ptrs_ : 8;

   //  The number of reference tags attached to the argument.
   //
   TagCount refs_ : 8;

   //  Set if the argument was accessed via "this".
   //
   bool member_ : 1;

   //  Set if the argument is const.
   //
   bool const_ : 1;

   //  Set if the argument is a const pointer.
   //
   bool constptr_ : 1;

   //  Set if the argument is mutable.
   //
   bool mutable_ : 1;

   //  Set if the argument is a function to be invoked.
   //
   bool invoke_ : 1;

   //  Set if the argument will be passed to a "this" argument.
   //
   bool this_ : 1;

   //  Set if the argument was created as an implicit "this".
   //
   bool implicit_ : 1;

   //  Set if the argument was created by a constructor.
   //
   bool ctor_ : 1;

   //  Set if WasRead has been invoked on the argument.  This prevents further
   //  invocations of WasRead when the argument is reused by pushing it as the
   //  result of an expression.
   //
   mutable bool read_ : 1;

   //  The result of an expression that can be used to resolve the type "auto".
   //
   static StackArg AutoType_;
};

extern const StackArg NilStackArg;

//------------------------------------------------------------------------------
//
//  Options for the CLI >parse command.
//
constexpr char TraceParse = 'p';
constexpr char SaveParseTrace = 's';
constexpr char TraceExecution = 'x';
constexpr char TraceFunctions = 'f';
constexpr char TraceImmediate = 'i';

//------------------------------------------------------------------------------
//
//  Used when parsing and executing code.
//
class ParseFrame
{
public:
   //  Constructs a new frame for PARSER.
   //
   explicit ParseFrame(const Parser* parser);

   //  Returns the parser associated with this frame.
   //
   const Parser* GetParser() const { return parser_; }

   //  Enters conditional compilation.
   //
   void PushOptional(OptionalCode* code);

   //  Exits conditional compilation.
   //
   bool PopOptional();

   //  Returns the current conditional compilation.
   //
   OptionalCode* Optional() const;

   //  Enters a scope.
   //
   void PushScope(CxxScope* scope);

   //  Exits the current scope.
   //
   void PopScope();

   //  Returns the current scope.
   //
   CxxScope* Scope() const;

   //  Pushes an operator.
   //
   void PushOp(const Operation* op);

   //  Returns the operator on top of the stack.
   //
   const Operation* TopOp() const;

   //  Pops an operator.
   //
   const Operation* PopOp();

   //  Pushes an argument.
   //
   void PushArg(const StackArg& arg);

   //  Pops an argument and invokes WasRead if READ is set.
   //
   StackArg PopArg(bool read);

   //  Pops an argument into ARG and returns true.  Returns
   //  false if the argument stack was empty.
   //
   bool PopArg(StackArg& arg);

   //  Peeks at the argument on top of the stack.
   //
   StackArg* TopArg();

   //  Returns the current execution position.
   //
   size_t GetPos() const { return pos_; }

   //  Sets the current execution position.
   //
   void SetPos(size_t pos) { pos_ = pos; }

   //  Executes pending operations before evaluating a new
   //  statement or expression.
   //
   void Execute();

   //  Clears the argument and operator stacks.
   //
   void Clear(NodeBase::word from);

   //  Reinitializes members in preparation for parsing a new file.
   //
   void Reset();
private:
   //  The parser associated with this frame.
   //
   const Parser* const parser_;

   //  Any conditional compilation (can be stacked).
   //
   std::vector< OptionalCode* > opts_;

   //  The scopes in which execution is occurring.
   //
   std::vector< CxxScope* > scopes_;

   //  The stack of arguments.
   //
   StackArgVector args_;

   //  The stack of operators.
   //
   std::vector< const Operation* > ops_;

   //  The current position in the source code.
   //
   size_t pos_;
};

//------------------------------------------------------------------------------
//
//  A source code tracepoint used to debug the >parse command.
//
class Tracepoint
{
public:
   //  What to do when reaching the tracepoint.
   //
   enum Action
   {
      Break = 1,  // break (at Debug::noop in Tracepoint::Hit)
      Start,      // start trace
      Stop,       // stop trace
      Action_N    // out of bounds value
   };

   //  Constructs a tracepoint to perform ACT when reaching FILE/LINE.
   //
   Tracepoint(const CodeFile* file, size_t line, Action act);

   //  For supporting Context::Tracepoints_.
   //
   bool operator<(const Tracepoint& that) const;

   //  Returns the file associated with the tracepoint.
   //
   const CodeFile* File() const { return file_; }

   //  Invoked when FILE/LINE is reached.  EXECUTING is false during
   //  parsing and true during execution.
   //
   void OnLine(const CodeFile* file, size_t line, bool executing) const;

   //  Displays the tracepoint in STREAM, starting each line with PREFIX.
   //
   void Display(std::ostream& stream, const std::string& prefix) const;
private:
   //  The source code file where the tracepoint is set.
   //
   const CodeFile* const file_;

   //  The line number within FILE (first line is 0).
   //
   const size_t line_;

   //  What to do upon reaching FILE/LINE.
   //
   const Action action_;

   //  Set if the tracepoint has been handled during parsing.
   //
   mutable bool parsed_;

   //  Set if the tracepoint has been handled during execution.
   //
   mutable bool executed_;
};

std::ostream& operator<<(std::ostream& stream, Tracepoint::Action action);

//------------------------------------------------------------------------------
//
//  For tracking all parser instances and accessing the foreground parser.
//
class Context
{
   friend class Parser;
public:
   //  Enters a scope.
   //
   static void PushScope(CxxScope* scope) { Frame_->PushScope(scope); }

   //  Exits the current scope.
   //
   static void PopScope() { Frame_->PopScope(); }

   //  Returns the current scope.
   //
   static CxxScope* Scope() { return Frame_->Scope(); }

   //  If parsing a template instance, returns the scope in which its
   //  instantiation occured, else returns the current scope.
   //
   static CxxScope* PrevScope();

   //  Enters conditional compilation.
   //
   static void PushOptional(OptionalCode* opt) { Frame_->PushOptional(opt); }

   //  Returns the current conditional compilation.
   //
   static OptionalCode* Optional() { return Frame_->Optional(); }

   //  Returns the current parser.
   //
   static const Parser* GetParser();

   //  Sets the item in which execution is about to occur.  It is
   //  recorded in the trace, and SetPos(owner) is invoked.
   //
   static void Enter(const CxxScoped* owner);

   //  Pushes an operator.
   //
   static void PushOp(const Operation* op) { Frame_->PushOp(op); }

   //  Returns the operator on top of the stack.
   //
   static const Operation* TopOp() { return Frame_->TopOp(); }

   //  Pops an operator.
   //
   static const Operation* PopOp() { return Frame_->PopOp(); }

   //  Pushes an argument.
   //
   static void PushArg(const StackArg& arg) { Frame_->PushArg(arg); }

   //  Pops an argument and invokes WasRead if READ is set.
   //
   static StackArg PopArg(bool read) { return Frame_->PopArg(read); }

   //  Pops an argument into ARG and returns true.  Returns
   //  false if the argument stack was empty.
   //
   static bool PopArg(StackArg& arg) { return Frame_->PopArg(arg); }

   //  Peeks at the argument on top of the stack.
   //
   static StackArg* TopArg() { return Frame_->TopArg(); }

   //  Invokes WasCalled on FUNC.
   //
   static void WasCalled(Function* func);

   //  Returns the current execution position in the source code.
   //
   static size_t GetPos() { return Frame_->GetPos(); }

   //  Sets the current execution position in the source code.
   //
   static void SetPos(const CxxLocation& loc);
   static void SetPos(size_t pos);

   //  Executes pending operations before evaluating a new
   //  statement or expression.
   //
   static void Execute() { Frame_->Execute(); }

   //  Clears the stack before evaluating a new statement.
   //  o Returns if the stack is empty.
   //  o Discards any unused arguments.
   //  o Logs an error if any operators were not processed.
   //
   static void Clear(NodeBase::word from) { Frame_->Clear(from); }

   //  Returns the file in which execution is occurring.
   //
   static CodeFile* File() { return File_; }

   //  Returns true if source code is being parsed.
   //
   static bool ParsingSourceCode() { return (File_ != nullptr); }

   //  Returns true if a template instance is currently being parsed.
   //
   static bool ParsingTemplateInstance();

   //  Returns true if the option identified by OPT is on.
   //
   static bool OptionIsOn(char opt);

   //  Logs ACT, which is associated with TOKEN, ARG, or FILE.  If ERR
   //  is not zero or EXPL is not empty, it indicates why ACT failed.
   //
   static void Trace(CxxTrace::Action act);
   static void Trace(CxxTrace::Action act, const CxxToken* token);
   static void Trace(CxxTrace::Action act, const StackArg& arg);
   static void Trace(CxxTrace::Action act, const CodeFile& file);
   static void Trace(CxxTrace::Action act, NodeBase::word err,
      const std::string& expl = NodeBase::EMPTY_STR);

   //  Inserts a tracepoint to perform ACTION at FILE and LINE.
   //
   static void InsertTracepoint
      (const CodeFile* file, size_t line, Tracepoint::Action action);

   //  Removes the tracepoint to perform ACTION at FILE and LINE.
   //
   static void EraseTracepoint
      (const CodeFile* file, size_t line, Tracepoint::Action action);

   //  Removes all tracepoint.
   //
   static void ClearTracepoints();

   //  Displays current tracepoint in STREAM.  Each line starts with PREFIX.
   //
   static void DisplayTracepoints
      (std::ostream& stream, const std::string& prefix);

   //  Logs WARNING at the current execution position.  ITEM and OFFSET
   //  are included as additional information for the log.
   //
   static void Log(Warning warning,
      const CxxNamed* item = nullptr, NodeBase::word offset = 0);

   //  The following invokes its Debug counterpart but also inserts
   //  the log in the execution trace to make it easier to see where
   //  the error occurred.
   //
   static void SwLog(NodeBase::fn_name_arg func, const std::string& expl,
      NodeBase::word errval, NodeBase::SwLogLevel level = NodeBase::SwInfo);

   //  Resets static data members when entering a restart at LEVEL.
   //
   static void Shutdown(NodeBase::RestartLevel level);

   //  Provided for symmetry with Shutdown.
   //
   static void Startup(NodeBase::RestartLevel level) { }

   //  Set if execution is being traced.
   //
   static bool Tracing;
private:
   //  Deleted because this class (basically a singleton) only has
   //  static members.
   //
   Context() = delete;

   //  Pushes a parser onto the stack.
   //
   static void PushParser(const Parser* parser);

   //  Pops PARSER if it is at the top of the stack.
   //
   static void PopParser(const Parser* parser);

   //  Returns the parser on the bottom of the stack.
   //
   static const Parser* RootParser() { return Frames_.front()->GetParser(); }

   //  Returns the parse depth (the number of stacked parsers).
   //
   static size_t ParseDepth() { return Frames_.size(); }

   //  Sets the options for the current parser when it starts to run.
   //
   static void SetOptions(const std::string& opts) { Options_ = opts; }

   //  Returns the current parser options.
   //
   static std::string GetOptions() { return Options_; }

   //  Sets the current execution position in the source code
   //  for a SCOPE that has both a declaration and definition.
   //
   static void SetPos(const CxxScoped* scope);

   //  Exits conditional compilation.
   //
   static bool PopOptional() { return Frame_->PopOptional(); }

   //  Configures the trace environment and returns true if tracing has
   //  been started.
   //
   static bool StartTracing();

   //  Invoked when parsing or executing LINE.
   //
   static void OnLine(size_t line, bool executing);

   //  Reinitializes all members.
   //
   static void Reset();

   //  Sets the file in which execution is occurring and invokes Reset.
   //
   static void SetFile(CodeFile* file);

   //  Returns a string containing File_ and the line number/offset for Pos_.
   //
   static std::string Location();

   //  The options set by the compiler.
   //
   static std::string Options_;

   //  The file in which execution is occurring.
   //
   static CodeFile* File_;

   //  The parse frames in which execution is occurring.
   //
   static std::vector< ParseFramePtr > Frames_;

   //  The parse frame that is currently active.
   //
   static ParseFrame* Frame_;

   //  The source code location of the last log generated by SwLog.
   //
   static std::string LastLogLoc_;

   //  Execution tracepoints.
   //
   static std::set< Tracepoint > Tracepoints_;

   //  Whether invocations of SetPos should be checked for a breakpoint.
   //
   static bool CheckPos_;
};
}
#endif
