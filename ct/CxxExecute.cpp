//==============================================================================
//
//  CxxExecute.cpp
//
//  Copyright (C) 2013-2021  Greg Utas
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
#include "CxxExecute.h"
#include <cstring>
#include <iomanip>
#include <set>
#include <sstream>
#include <utility>
#include "CodeFile.h"
#include "CxxArea.h"
#include "CxxDirective.h"
#include "CxxLocation.h"
#include "CxxNamed.h"
#include "CxxScope.h"
#include "CxxString.h"
#include "CxxSymbols.h"
#include "CxxToken.h"
#include "Debug.h"
#include "Formatters.h"
#include "Lexer.h"
#include "Parser.h"
#include "Singleton.h"
#include "ThisThread.h"
#include "ToolTypes.h"
#include "TraceBuffer.h"
#include "TraceDump.h"

using namespace NodeBase;
using std::ostream;
using std::setw;
using std::string;

//------------------------------------------------------------------------------

namespace CodeTools
{
//  Returns true if a function in a template (not in a template
//  *instance*) is being compiled.
//
static bool CompilingTemplateFunction()
{
   auto scope = Context::Scope();
   if(scope == nullptr) return false;

   auto func = Context::Scope()->GetFunction();
   if(func == nullptr) return false;

   return (func->GetTemplateType() != NonTemplate);
}

//------------------------------------------------------------------------------
//
//  Invoked to record that the context function cannot be const.
//
static void ContextFunctionIsNonConst()
{
   Debug::ft("CodeTools.ContextFunctionIsNonConst");

   auto func = Context::Scope()->GetFunction();
   if(func != nullptr) func->IncrThisWrites();
}

//------------------------------------------------------------------------------
//
//  Returns a string containing File_ and the line number/offset for Pos_.
//
static string Location()
{
   auto parser = Context::GetParser();
   if(parser == nullptr) return "unknown location";

   std::ostringstream stream;
   stream << parser->GetVenue();
   stream << ", line " << parser->GetLineNum(Context::GetPos()) + 1;

   if(parser->ParsingSourceCode())
   {
      auto scope = Context::Scope();

      if(scope != nullptr)
      {
         auto name = scope->ScopedName(true);
         string locals(SCOPE_STR);
         locals += LOCALS_STR;
         auto pos = name.find(locals);
         if(pos != string::npos) name.erase(pos);
         if(!name.empty()) stream << ", scope " << name;
      }
   }

   return stream.str();
}

//------------------------------------------------------------------------------
//
//  Concrete classes for tracing compilation.
//
class ActTrace : public CxxTrace
{
public:
   //  Creates a trace record for ACTION, which has no additional data.
   //
   explicit ActTrace(Action action);

   //  Overridden to display the trace record.
   //
   bool Display(ostream& stream, const string& opts) override;
};

class ArgTrace : public CxxTrace
{
public:
   //  Creates a trace record for ACTION, which is associated with ARG.
   //
   ArgTrace(Action action, const StackArg& arg);

   //  Overridden to display the trace record.
   //
   bool Display(ostream& stream, const string& opts) override;
private:
   //  The argument associated with the action.
   //
   const StackArg arg_;
};

class TokenTrace : public CxxTrace
{
public:
   //  Creates a trace record for ACTION, which is associated with TOKEN.
   //
   TokenTrace(Action action, const CxxToken* token);

   //  Overridden to display the trace record.
   //
   bool Display(ostream& stream, const string& opts) override;
private:
   //  The token associated with the action.
   //
   const CxxToken* const token_;
};

class FileTrace : public CxxTrace
{
public:
   //  Creates a trace record for ACTION, which is associated with FILE.
   //
   FileTrace(Action action, const CodeFile& file);

   //  Overridden to display the trace record.
   //
   bool Display(ostream& stream, const string& opts) override;
private:
   //  The file associated with the action.
   //
   const CodeFile* const file_;
};

class ErrTrace : public CxxTrace
{
public:
   //  Creates a trace record for ACTION, which is explained by ERR and/or EXPL.
   //
   ErrTrace(Action action, word err = 0, const string& expl = EMPTY_STR);

   //  Not subclassed.
   //
   ~ErrTrace();

   //  Deleted to prohibit copying.
   //
   ErrTrace(const ErrTrace& that) = delete;

   //  Deleted to prohibit copy assignment.
   //
   ErrTrace& operator=(const ErrTrace& that) = delete;

   //  Overridden to display the trace record.
   //
   bool Display(ostream& stream, const string& opts) override;
private:
   //  If non-zero, the error associated with the action.
   //
   const word err_;

   //  Any explanation associated with the error.
   //
   stringPtr expl_;
};

//==============================================================================

ActTrace::ActTrace(Action action) : CxxTrace(action) { }

//------------------------------------------------------------------------------

bool ActTrace::Display(ostream& stream, const string& opts)
{
   CxxTrace::Display(stream, opts);
   return true;
}

//==============================================================================

ArgTrace::ArgTrace(Action action, const StackArg& arg) :
   CxxTrace(action),
   arg_(arg)
{
}

//------------------------------------------------------------------------------

bool ArgTrace::Display(ostream& stream, const string& opts)
{
   CxxTrace::Display(stream, opts);
   stream << arg_.Trace();
   return true;
}

//==============================================================================
//
//  Set if compilation is being traced.
//
static bool Tracing_ = false;

string Context::Options_ = EMPTY_STR;
CodeFile* Context::File_ = nullptr;
std::vector< ParseFramePtr > Context::Frames_ = std::vector< ParseFramePtr >();
ParseFrame* Context::Frame_ = nullptr;

//  The source code location of the last log generated by SwLog.
//
static std::string LastLogLoc_ = EMPTY_STR;

//  Parser/compiler tracepoints.
//
static std::set<Tracepoint> Tracepoints_ = std::set< Tracepoint >();

bool Context::CheckPos_ = false;

//  Information for the function(s) that are updating the cross-reference.
//  This acts as a stack to allow nesting.
//
static std::vector<XrefFrame> XrefFrames_ = std::vector< XrefFrame >();

//------------------------------------------------------------------------------

bool Context::AtFileScope()
{
   Debug::ft("Context.AtFileScope");

   auto scope = Scope();
   if(scope == nullptr) return false;
   return (scope->Type() == Cxx::Namespace);
}

//------------------------------------------------------------------------------

void Context::ClearTracepoints()
{
   Debug::ft("Context.ClearTracepoints");

   Tracepoints_.clear();
}

//------------------------------------------------------------------------------

void Context::DisplayTracepoints(ostream& stream, const string& prefix)
{
   if(Tracepoints_.empty())
   {
      stream << prefix << "No tracepoints inserted." << CRLF;
      return;
   }

   for(auto b = Tracepoints_.cbegin(); b != Tracepoints_.cend(); ++b)
   {
      b->Display(stream, prefix);
   }
}

//------------------------------------------------------------------------------

void Context::Enter(const CxxScoped* owner)
{
   Debug::ft("Context.Enter");

   Trace(CxxTrace::START_SCOPE, 0, owner->ScopedName(true));
   SetPos(owner);
}

//------------------------------------------------------------------------------

void Context::EraseTracepoint
   (const CodeFile* file, size_t line, Tracepoint::Action action)
{
   Debug::ft("Context.EraseTracepoint");

   Tracepoint loc(file, line, action);
   Tracepoints_.erase(loc);
}

//------------------------------------------------------------------------------

TypeName* Context::FindXrefItem(const string& name)
{
   //  The top frame is currently an InstanceFunction that is resolving any
   //  items pushed by the TemplateFunction in the frame below it.
   //
   auto size = XrefFrames_.size();
   if(size < 2) return nullptr;
   return XrefFrames_.at(size - 2).FindItem(name);
}

//------------------------------------------------------------------------------

const Parser* Context::GetParser()
{
   if(Frame_ == nullptr) return nullptr;
   return Frame_->GetParser();
}

//------------------------------------------------------------------------------

XrefUpdater Context::GetXrefUpdater()
{
   if(XrefFrames_.empty()) return NotAFunction;
   return XrefFrames_.back().Updater();
}

//------------------------------------------------------------------------------

void Context::InsertTracepoint
   (const CodeFile* file, size_t line, Tracepoint::Action action)
{
   Debug::ft("Context.InsertTracepoint");

   Tracepoint loc(file, line, action);
   Tracepoints_.insert(loc);
}

//------------------------------------------------------------------------------

void Context::Log(Warning warning, const CxxNamed* item, word offset)
{
   Debug::ft("Context.Log");

   if(File_ == nullptr) return;
   File_->LogPos(GetPos(), warning, item, offset);
}

//------------------------------------------------------------------------------

void Context::OnLine(size_t line, bool compiling)
{
   auto parser = GetParser();
   if(parser == nullptr) return;
   if(!parser->ParsingSourceCode()) return;

   for(auto b = Tracepoints_.cbegin(); b != Tracepoints_.cend(); ++b)
   {
      b->OnLine(File_, line, compiling);
   }
}

//------------------------------------------------------------------------------

bool Context::OptionIsOn(char opt)
{
   return (GetOptions().find(opt) != string::npos);
}

//------------------------------------------------------------------------------

const ParseFrame* Context::OuterFrame()
{
   Debug::ft("Context.OuterFrame");

   auto size = Frames_.size();
   if(size <= 1) return nullptr;
   return Frames_.at(size - 2).get();
}

//------------------------------------------------------------------------------

CxxScope* Context::OuterScope()
{
   Debug::ft("Context.OuterScope");

   if(Frames_.empty()) return nullptr;
   return Frames_.back()->OuterScope();
}

//------------------------------------------------------------------------------

bool Context::ParsingSourceCode()
{
   if(Frame_ == nullptr) return false;
   return Frame_->GetParser()->ParsingSourceCode();
}

//------------------------------------------------------------------------------

bool Context::ParsingTemplateInstance()
{
   if(Frame_ == nullptr) return false;
   return Frame_->GetParser()->ParsingTemplateInstance();
}

//------------------------------------------------------------------------------

void Context::PopParser(const Parser* parser)
{
   Debug::ft("Context.PopParser");

   //  If PARSER is on top of the stack, remove it.
   //
   if(Frame_->GetParser() == parser)
   {
      Clear(0);
      Frames_.pop_back();
      Frame_ = (Frames_.empty() ? nullptr : Frames_.back().get());
      return;
   }
}

//------------------------------------------------------------------------------

void Context::PopXrefFrame()
{
   XrefFrames_.pop_back();
}

//------------------------------------------------------------------------------

void Context::PushParser(const Parser* parser)
{
   Debug::ft("Context.PushParser");

   ParseFramePtr frame(new ParseFrame(parser));
   Frame_ = frame.get();
   Frames_.push_back(std::move(frame));
}

//------------------------------------------------------------------------------

void Context::PushScope(CxxScope* scope, bool hidden)
{
   auto access = (hidden ? Cxx::Private : scope->GetAccess());
   ActiveScope active(scope, access);
   Frame_->PushScope(active);
}

//------------------------------------------------------------------------------

void Context::PushXrefFrame(XrefUpdater updater)
{
   XrefFrames_.push_back(XrefFrame(updater));
}

//------------------------------------------------------------------------------

void Context::PushXrefItem(TypeName* item)
{
   if(!XrefFrames_.empty())
   {
      XrefFrames_.back().PushItem(item);
   }
}

//------------------------------------------------------------------------------

void Context::Reset()
{
   Debug::ft("Context.Reset");

   Frame_->Reset();
   File_ = nullptr;
   CheckPos_ = false;
   Block::ResetUsings();
}

//------------------------------------------------------------------------------

CxxScope* Context::Scope()
{
   return (Frame_ != nullptr ? Frame_->Scope() : nullptr);
}

//------------------------------------------------------------------------------

Cxx::Access Context::ScopeVisibility()
{
   Debug::ft("Context.ScopeVisibility");

   if(ScopeAccess() == Cxx::Private) return Cxx::Private;
   if(!ParsingSourceCode()) return Cxx::Private;

   //  If the scope is in a class, that class (and its chain of outer classes,
   //  if any) also affect its visibility.
   //
   auto scope = Scope();
   auto access = scope->GetAccess();

   for(auto c = scope->GetClass(); c != nullptr; c = c->OuterClass())
   {
      if(c->GetAccess() < access) access = c->GetAccess();
   }

   return access;
}

//------------------------------------------------------------------------------

void Context::SetFile(CodeFile* file)
{
   Debug::ft("Context.SetFile");

   //  This is the start of a new parse, so reinitialize the context.
   //
   Reset();
   File_ = file;

   for(auto b = Tracepoints_.cbegin(); b != Tracepoints_.cend(); ++b)
   {
      if(b->File() == file)
      {
         CheckPos_ = true;
         return;
      }
   }
}

//------------------------------------------------------------------------------

void Context::SetPos(size_t pos)
{
   //  This can be invoked when the Editor adds code, in which case
   //  there will be no parse frame.
   //
   if(Frame_ != nullptr) Frame_->SetPos(pos);

   if(CheckPos_ && (File_ != nullptr))
   {
      OnLine(File_->GetLexer().GetLineNum(GetPos()), true);
   }
}

//------------------------------------------------------------------------------

void Context::SetPos(const CxxLocation& loc)
{
   SetPos(loc.GetPos());
}

//------------------------------------------------------------------------------

fn_name Context_SetPos = "Context.SetPos";

void Context::SetPos(const CxxScoped* scope)
{
   Debug::ft(Context_SetPos);

   if(scope->GetFile() == File_)
      SetPos(scope->GetPos());
   else
      SwLog(Context_SetPos, scope->Trace(), 0);
}

//------------------------------------------------------------------------------

void Context::Shutdown(RestartLevel level)
{
   Debug::ft("Context.Shutdown");

   Tracing_ = false;
   Options_ = EMPTY_STR;
   LastLogLoc_ = EMPTY_STR;
   File_ = nullptr;
   Frames_.clear();
   Frame_ = nullptr;
   CheckPos_ = false;
}

//------------------------------------------------------------------------------

bool Context::StartTracing()
{
   Debug::ft("Context.StartTracing");

   auto x = OptionIsOn(TraceCompilation);
   auto f = OptionIsOn(TraceFunctions);
   if(!x && !f) return false;

   auto buff = Singleton< TraceBuffer >::Instance();

   if(x)
   {
      buff->SetTool(ParserTracer, true);
      Tracing_ = true;
   }

   if(f)
   {
      buff->SetTool(FunctionTracer, true);
      ThisThread::IncludeInTrace();
   }

   ThisThread::StartTracing(EMPTY_STR);
   return true;
}

//------------------------------------------------------------------------------

void Context::SwLog
   (fn_name_arg func, const string& expl, word errval, bool stack)
{
   Debug::ft("Context.SwLog");

   //  Logs are usually suppressed when compiling a function in a template.
   //
   if(CompilingTemplateFunction() && !OptionIsOn(TemplateLogs)) return;

   //  Suppress noise that occurs after logging another error.
   //
   auto loc = Location();

   if(loc == LastLogLoc_)
   {
      if(expl == "Empty argument stack") return;
      if(expl.find("is incompatible with #ERR!") != string::npos) return;
   }

   LastLogLoc_ = loc;
   auto info = loc + ": " + expl;
   Trace(CxxTrace::ERROR, errval, info);
   if(Tracing_ && !stack) return;  //@
   Debug::SwLog(func, info, errval, stack);
}

//------------------------------------------------------------------------------

void Context::Trace(CxxTrace::Action act)
{
   if(!Tracing_) return;
   auto rec = new ActTrace(act);
   Singleton< TraceBuffer >::Instance()->Insert(rec);
}

//------------------------------------------------------------------------------

void Context::Trace(CxxTrace::Action act, const StackArg& arg)
{
   if(!Tracing_) return;
   auto rec = new ArgTrace(act, arg);
   Singleton< TraceBuffer >::Instance()->Insert(rec);
}

//------------------------------------------------------------------------------

void Context::Trace(CxxTrace::Action act, word err, const string& expl)
{
   if(!Tracing_) return;
   auto rec = new ErrTrace(act, err, expl);
   Singleton< TraceBuffer >::Instance()->Insert(rec);
}

//------------------------------------------------------------------------------

void Context::Trace(CxxTrace::Action act, const CodeFile& file)
{
   if(!Tracing_) return;
   auto rec = new FileTrace(act, file);
   Singleton< TraceBuffer >::Instance()->Insert(rec);
}

//------------------------------------------------------------------------------

void Context::Trace(CxxTrace::Action act, const CxxToken* token)
{
   if(!Tracing_) return;
   auto rec = new TokenTrace(act, token);
   Singleton< TraceBuffer >::Instance()->Insert(rec);
}

//------------------------------------------------------------------------------

void Context::WasCalled(Function* func)
{
   Debug::ft("Context.WasCalled");

   if(func == nullptr) return;
   func->WasCalled();
   StackArg arg(func, 0, false);
   Trace(CxxTrace::INCR_CALLS, arg);
}

//==============================================================================

fixed_string ActionStrings[CxxTrace::Action_N] =
{
   "FILE",
   "SCOPE",
   "TMPLT...",
   "...TMPLT",
   "push_op",
   "pop_op",
   "push_arg",
   "pop_arg",
   "set_auto",
   "incr_r",
   "incr_w",
   "incr_c",
   "execute",
   "clear",
   ERROR_STR  // may actually appear in compilation traces
};

//  The last line number displayed.
//
static uint16_t Last_ = UINT16_MAX;

//------------------------------------------------------------------------------

CxxTrace::CxxTrace(Action action) :
   TraceRecord(ParserTracer),
   line_(UINT16_MAX)
{
   rid_ = action;

   if((rid_ >= PUSH_OP) && (rid_ < EXECUTE))
   {
      auto parser = Context::GetParser();
      if(parser != nullptr) line_ = parser->GetLineNum(Context::GetPos());
   }
}

//------------------------------------------------------------------------------

bool CxxTrace::Display(ostream& stream, const string& opts)
{
   auto buff = Singleton< TraceBuffer >::Instance();

   if(buff->ToolIsOn(FunctionTracer))
   {
      TraceRecord::Display(stream, opts);
      stream << spaces(TraceDump::EvtToObj);
   }

   if((line_ != Last_) && ((rid_ >= PUSH_OP) && (rid_ < EXECUTE)))
   {
      stream << setw(5) << line_ + 1;
      Last_ = line_;
   }
   else
   {
      stream << spaces(5);
      if(rid_ <= END_TEMPLATE) Last_ = UINT16_MAX;
   }

   auto& s = ActionStrings[rid_];
   stream << spaces(10 - strlen(s)) << s << TraceDump::Tab();
   return true;
}

//==============================================================================

ErrTrace::ErrTrace(Action action, word err, const string& expl) :
   CxxTrace(action),
   err_(err),
   expl_(nullptr)
{
   if(!expl.empty()) expl_.reset(new string(expl));
}

//------------------------------------------------------------------------------

ErrTrace::~ErrTrace()
{
   expl_.reset();
}

//------------------------------------------------------------------------------

bool ErrTrace::Display(ostream& stream, const string& opts)
{
   CxxTrace::Display(stream, opts);

   if(rid_ == ERROR)
   {
      if(expl_ != nullptr) stream << "expl=" << *expl_;
      stream << " err=" << strHex(debug64_t(err_)) << CRLF;
   }
   else
   {
      if(err_ != 0) stream << '(' << err_ << ") ";
      if(expl_ != nullptr) stream << *expl_;
   }

   return true;
}

//==============================================================================

FileTrace::FileTrace(Action action, const CodeFile& file) :
   CxxTrace(action),
   file_(&file)
{
}

//------------------------------------------------------------------------------

bool FileTrace::Display(ostream& stream, const string& opts)
{
   CxxTrace::Display(stream, opts);
   stream << file_->Name();
   return true;
}

//==============================================================================

ParseFrame::ParseFrame(const Parser* parser) :
   parser_(parser),
   pos_(string::npos)
{
   Debug::ft("ParseFrame.ctor");
}

//------------------------------------------------------------------------------

fn_name ParseFrame_Clear = "ParseFrame.Clear";

void ParseFrame::Clear(word from)
{
   Debug::ft(ParseFrame_Clear);

   Context::Trace(CxxTrace::CLEAR, from);
   while(!args_.empty())
   {
      auto arg = args_.back();
      args_.pop_back();
      arg.WasRead();
   }
   if(ops_.empty()) return;
   Debug::SwLog(ParseFrame_Clear,
      "operator stack not empty", ops_.size(), false);
   ops_.clear();
}

//------------------------------------------------------------------------------

void ParseFrame::EraseLocal(const CxxScoped* local)
{
   Debug::ft("ParseFrame.EraseLocal");

   EraseSymbol(local, locals_);
}

//------------------------------------------------------------------------------

void ParseFrame::Execute()
{
   Debug::ft("ParseFrame.Execute");

   //  There is nothing to do if the stacks are empty.
   //
   Context::Trace(CxxTrace::EXECUTE);
   if(ops_.empty() && args_.empty()) return;

   //  Pop and execute operators until the start of the current expression is
   //  reached.  If this didn't empty the stack, the results of more than one
   //  expression are being assembled (e.g. as arguments for a function call).
   //
   while(!ops_.empty())
   {
      auto top = PopOp();
      if(top->Op() == Cxx::START_OF_EXPRESSION) return;
      top->Execute();
   }
}

//------------------------------------------------------------------------------

fn_name ParseFrame_FindLocal = "ParseFrame.FindLocal";

CxxScoped* ParseFrame::FindLocal(const string& name, SymbolView& view) const
{
   Debug::ft(ParseFrame_FindLocal);

   SymbolVector list;

   //  Start by looking for a terminal.
   //
   Singleton< CxxSymbols >::Instance()->FindTerminal(name, list);

   if(!list.empty())
   {
      view = DeclaredGlobally;
      return list.front();
   }

   //  Look for a local that matches NAME.
   //
   ListSymbols(name, locals_, list);

   if(!list.empty())
   {
      view = DeclaredLocally;

      if(list.size() > 1)
      {
         auto idx = FindNearestItem(list);
         if(idx != SIZE_MAX) return list[idx];
         auto expl = name + " has more than one definition";
         Context::SwLog(ParseFrame_FindLocal, expl, list.size());
      }

      return list.front();
   }

   return nullptr;
}

//------------------------------------------------------------------------------

void ParseFrame::InsertLocal(CxxScoped* local)
{
   Debug::ft("ParseFrame.InsertLocal");

   typedef std::pair< string, CxxScoped* > LocalPair;

   //  Delete any item with the same name that is defined in the same block.
   //
   auto& name = local->Name();
   auto scope = local->GetScope();
   SymbolVector list;

   ListSymbols(name, locals_, list);

   for(auto s = list.cbegin(); s != list.cend(); ++s)
   {
      if((*s)->GetScope() == scope)
      {
         EraseLocal(*s);
      }
   }

   locals_.insert(LocalPair(Normalize(name), local));
}

//------------------------------------------------------------------------------

OptionalCode* ParseFrame::Optional() const
{
   if(opts_.empty()) return nullptr;
   return opts_.back();
}

//------------------------------------------------------------------------------

CxxScope* ParseFrame::OuterScope() const
{
   Debug::ft("ParseFrame.OuterScope");

   auto size = scopes_.size();
   if(size <= 1) return nullptr;
   return scopes_.at(size - 2).scope;
}

//------------------------------------------------------------------------------

fn_name ParseFrame_PopArg1 = "ParseFrame.PopArg";

StackArg ParseFrame::PopArg(bool read)
{
   Debug::ft(ParseFrame_PopArg1);

   if(args_.empty())
   {
      if(Tracing_)
         Context::Trace(CxxTrace::POP_ARG, -1);
      else
         Context::SwLog(ParseFrame_PopArg1, "Empty argument stack", 0);
      return NilStackArg;
   }

   auto arg = args_.back();
   args_.pop_back();
   Context::Trace(CxxTrace::POP_ARG, arg);
   if(read) arg.WasRead();
   return arg;
}

//------------------------------------------------------------------------------

fn_name ParseFrame_PopArg2 = "ParseFrame.PopArg(arg)";

bool ParseFrame::PopArg(StackArg& arg)
{
   Debug::ft(ParseFrame_PopArg2);

   if(args_.empty())
   {
      if(Tracing_)
         Context::Trace(CxxTrace::POP_ARG, -1);
      else
         Context::SwLog(ParseFrame_PopArg2, "Empty argument stack", 0);
      return false;
   }

   arg = args_.back();
   args_.pop_back();
   Context::Trace(CxxTrace::POP_ARG, arg);
   return true;
}

//------------------------------------------------------------------------------

fn_name ParseFrame_PopOp = "ParseFrame.PopOp";

const Operation* ParseFrame::PopOp()
{
   Debug::ft(ParseFrame_PopOp);

   if(ops_.empty())
   {
      if(Tracing_)
         Context::Trace(CxxTrace::POP_OP, -1);
      else
         Context::SwLog(ParseFrame_PopOp, "Empty operator stack", 0);
      return nullptr;
   }

   auto op = ops_.back();
   ops_.pop_back();
   Context::Trace(CxxTrace::POP_OP, op);
   return op;
}

//------------------------------------------------------------------------------

bool ParseFrame::PopOptional()
{
   Debug::ft("ParseFrame.PopOptional");

   if(opts_.empty()) return false;
   opts_.pop_back();
   return true;
}

//------------------------------------------------------------------------------

fn_name ParseFrame_PopScope = "ParseFrame.PopScope";

void ParseFrame::PopScope()
{
   Debug::ftnt(ParseFrame_PopScope);

   if(!scopes_.empty())
      scopes_.pop_back();
   else
      Context::SwLog(ParseFrame_PopScope, "Empty scope stack", 0);
}

//------------------------------------------------------------------------------

fn_name ParseFrame_PushArg = "ParseFrame.PushArg";

void ParseFrame::PushArg(const StackArg& arg)
{
   Debug::ft(ParseFrame_PushArg);

   if(arg.item_ == nullptr)
   {
      if(Tracing_)
         Context::Trace(CxxTrace::PUSH_ARG, -1);
      else
         Context::SwLog(ParseFrame_PushArg, "Push null argument", 0);
      return;
   }

   if(arg.item_->Type() != Cxx::Elision)
   {
      args_.push_back(arg);
      Context::Trace(CxxTrace::PUSH_ARG, arg);
   }
}

//------------------------------------------------------------------------------

fn_name ParseFrame_PushOp = "ParseFrame.PushOp";

void ParseFrame::PushOp(const Operation* op)
{
   Debug::ft(ParseFrame_PushOp);

   if(op == nullptr)
   {
      if(Tracing_)
         Context::Trace(CxxTrace::PUSH_OP, -1);
      else
         Context::SwLog(ParseFrame_PushOp, "Push null operator", 0);
      return;
   }

   ops_.push_back(op);
   Context::Trace(CxxTrace::PUSH_OP, op);
}

//------------------------------------------------------------------------------

void ParseFrame::PushOptional(OptionalCode* code)
{
   Debug::ft("ParseFrame.PushOptional");

   opts_.push_back(code);
}

//------------------------------------------------------------------------------

void ParseFrame::PushScope(const ActiveScope& scope)
{
   Debug::ft("ParseFrame.PushScope");

   scopes_.push_back(scope);
}

//------------------------------------------------------------------------------

void ParseFrame::Reset()
{
   Debug::ft("ParseFrame.Reset");

   opts_.clear();
   locals_.clear();
   scopes_.clear();
   ops_.clear();
   args_.clear();
   pos_ = string::npos;
}

//------------------------------------------------------------------------------

CxxScope* ParseFrame::Scope() const
{
   return scopes_.back().scope;
}

//------------------------------------------------------------------------------

Cxx::Access ParseFrame::ScopeAccess() const
{
   return scopes_.back().access;
}

//------------------------------------------------------------------------------

Cxx::Access ParseFrame::SetAccess(Cxx::Access access)
{
   auto curr = scopes_.back().access;
   scopes_.back().access = access;
   return curr;
}

//------------------------------------------------------------------------------

StackArg* ParseFrame::TopArg()
{
   Debug::ft("ParseFrame.TopArg");

   if(args_.empty()) return nullptr;
   return &args_.back();
}

//------------------------------------------------------------------------------

const Operation* ParseFrame::TopOp() const
{
   Debug::ft("ParseFrame.TopOp");

   if(ops_.empty()) return nullptr;
   return ops_.back();
}

//==============================================================================

const StackArg NilStackArg = StackArg(nullptr, 0, false);

//  The result of an expression that can be used to resolve the type "auto".
//
static StackArg AutoType_ = NilStackArg;

//------------------------------------------------------------------------------

StackArg::StackArg(CxxToken* t, TagCount p, bool ctor) :
   item_(t),
   name_(nullptr),
   via_(nullptr),
   ptrs_(p),
   refs_(0),
   control_(Cxx::Access_N),
   member_(false),
   const_(t != nullptr ? t->IsConst() : false),
   constptr_(t != nullptr ? t->IsConstPtr() : false),
   mutable_(false),
   invoke_(false),
   this_(false),
   implicit_(false),
   ctor_(ctor),
   read_(false)
{
   Debug::ft("StackArg.ctor(ptrs)");
}

//------------------------------------------------------------------------------

StackArg::StackArg(Function* f, TypeName* name) :
   item_(f),
   name_(name),
   via_(nullptr),
   ptrs_(0),
   refs_(0),
   control_(name != nullptr ? name->MinControl() : Cxx::Access_N),
   member_(false),
   const_(f != nullptr ? f->IsConst() : false),
   constptr_(false),
   mutable_(false),
   invoke_(true),
   this_(false),
   implicit_(false),
   ctor_(false),
   read_(false)
{
   Debug::ft("StackArg.ctor(func)");
}

//------------------------------------------------------------------------------

StackArg::StackArg(Function* f, TypeName* name, const StackArg& via) :
   item_(f),
   name_(name),
   via_(via.item_),
   ptrs_(0),
   refs_(0),
   control_(name->MinControl()),
   member_(false),
   const_(f != nullptr ? f->IsConst() : false),
   constptr_(false),
   mutable_(false),
   invoke_(true),
   this_(false),
   implicit_(false),
   ctor_(false),
   read_(false)
{
   Debug::ft("StackArg.ctor(func, via)");
}

//------------------------------------------------------------------------------

StackArg::StackArg(CxxToken* t, TypeName* name,
   const StackArg& via, Cxx::Operator op) :
   item_(t),
   name_(name),
   via_(via.item_),
   ptrs_(0),
   refs_(0),
   control_(name->MinControl()),
   member_(false),
   const_(t != nullptr ? t->IsConst() : false),
   constptr_(t != nullptr ? t->IsConstPtr() : false),
   mutable_(via.mutable_),
   invoke_(false),
   this_(false),
   implicit_(false),
   ctor_(false),
   read_(false)
{
   Debug::ft("StackArg.ctor(via)");

   //c Support a via_ chain (that is, also record via_.via_).  This would
   //  fix the bug where b is flagged as "could be const" in a statement
   //  like b.c.d = n.

   //  Tag the item as const if VIA was const.
   //
   if(via.const_) SetAsReadOnly();

   //  Tag the item as a member if VIA was a member or a "this" pointer.
   //
   if(op == Cxx::REFERENCE_SELECT)
   {
      if(via.member_) member_ = true;
   }
   else
   {
      if(via.item_->Name() == THIS_STR) member_ = true;
   }
}

//------------------------------------------------------------------------------

StackArg::StackArg(CxxToken* t, TypeName* name) :
   item_(t),
   name_(name),
   via_(nullptr),
   ptrs_(0),
   refs_(0),
   control_(name->MinControl()),
   member_(false),
   const_(t != nullptr ? t->IsConst() : false),
   constptr_(t != nullptr ? t->IsConstPtr() : false),
   mutable_(false),
   invoke_(false),
   this_(false),
   implicit_(false),
   ctor_(false),
   read_(false)
{
   Debug::ft("StackArg.ctor(name)");
}

//------------------------------------------------------------------------------

size_t StackArg::Arrays() const
{
   Debug::ft("StackArg.Arrays");

   if(item_ == nullptr) return 0;
   auto spec = item_->GetTypeSpec();
   word count = (spec == nullptr ? 0 : spec->Arrays());
   return count;
}

//------------------------------------------------------------------------------

fn_name StackArg_AssignedTo = "StackArg.AssignedTo";

void StackArg::AssignedTo(const StackArg& that, AssignmentType type) const
{
   Debug::ft(StackArg_AssignedTo);

   //  The scenarios are
   //  o Copied     that = this   already invoked THAT.WasWritten
   //  o Passed     this(that)    THAT is an Argument type
   //  o Returned   return this   THAT is a Function return type
   //
   if(that.const_) return;
   if(this->item_ == nullptr) return;
   if(that.item_ == nullptr) return;

   auto thisPtrs = this->Ptrs(true);
   auto thatPtrs = that.Ptrs(true);
   auto thatRefs = that.Refs();

   if((type == Returned) && member_)
   {
      if((thatRefs > 0) || (thatPtrs > thisPtrs))
      {
         auto func = Context::Scope()->GetFunction();
         if((func != nullptr) && (func->GetAccess() != Cxx::Private))
         {
            func->Log(ReturnsNonConstMember);
         }
      }
   }

   auto restricted = false;

   if(thatPtrs > 0)
      restricted = (thisPtrs > 0);  // allows const int to pointer
   else if(thatRefs > 0)
      restricted = ((type != Copied) || that.item_->IsInitializing());

   if(!restricted) return;

   if(this->const_ && !this->mutable_)
   {
      auto expl = this->TypeString(true);
      expl += " (const) assigned to " + that.TypeString(true);
      Context::SwLog(StackArg_AssignedTo, expl, 0);
      return;
   }

   //  This item is being assigned to a non-const item, so it cannot be const.
   //
   this->SetNonConst(0);

   //  If this item was accessed through another, that item (via_) cannot be
   //  const if it is exporting a non-pointer item that is a member of either
   //  the context class or the via_ itself.
   //
   auto notMutable = this->member_ && !this->mutable_;
   auto notPointer = !this->item_->IsPointer(false);

   if((this->via_ != nullptr) && notPointer)
   {
      if(notMutable || !this->via_->IsPointer(false))
      {
         this->SetNonConst(1);
      }
   }

   //  A T** cannot be assigned to a const T**.
   //  A T* or T*& cannot be assigned to a const T*&.
   //
   if(thisPtrs > 1)
      that.SetNonConst(0);
   else if((thisPtrs == 1) && (thatRefs == 1))
      that.SetNonConst(0);

   //  The context function cannot be const if it exports a non-mutable,
   //  non-pointer member (whether assigned, passed, or returned).
   //
   if(notMutable && notPointer)
   {
      if((this->via_ == nullptr) || (this->via_->Name() == THIS_STR))
      {
         ContextFunctionIsNonConst();
      }
   }

   if(type == Passed)
   {
      //  If the item was passed as an argument, treat it as a write if
      //  the receiver is a reference or pointer.
      //
      if((thatRefs > 0) || (thatPtrs > 0))
      {
         if(this->item_->WasWritten(this, thatRefs > 0, thatPtrs > 0))
         {
            Context::Trace(CxxTrace::INCR_WRITES, *this);
         }
      }

      //  The context function cannot be const if a non-mutable member is
      //  (a) passed to an argument that has more pointers
      //  (b) passed by reference to an argument that has as many pointers
      //  Given the member declaration T* t, examples of the above include
      //  o passing &t to a T** means that t cannot be const
      //  o passing t to a T* still allows t to be const
      //  o passing *t to a T& still allows t to be const
      //  o passing t to a T*& means that t cannot be const
      //
      if(notMutable)
      {
         auto netPtrs = thisPtrs - ptrs_;

         if(thatPtrs > netPtrs)
            ContextFunctionIsNonConst();
         else if((thatRefs > 0) && (thatPtrs == netPtrs))
            ContextFunctionIsNonConst();
      }
   }
}

//------------------------------------------------------------------------------

TypeMatch StackArg::CalcMatchWith(const StackArg& that,
   const string& thisType, const string& thatType) const
{
   Debug::ft("StackArg.CalcMatchWith");

   auto best = MatchWith(that, thisType, thatType);
   if(best >= Derivable) return best;
   if(that.item_ == nullptr) return Incompatible;
   if(this->item_ == nullptr) return Incompatible;

   //  See if there is a match between any of the types to which the
   //  items can be converted.
   //
   StackArgVector these, those;
   these.push_back(*this);
   those.push_back(that);
   this->item_->Root()->GetConvertibleTypes(these, true);
   that.item_->Root()->GetConvertibleTypes(those, false);
   if((these.size() == 1) && (those.size() == 1)) return best;

   auto first = true;
   string ts1, ts2;

   for(size_t i = 0; i < these.size(); ++i)
   {
      auto& arg1 = these[i];

      if(i == 0)
      {
         ts1 = thisType;
      }
      else
      {
         arg1.ptrs_ = this->ptrs_;
         ts1 = arg1.TypeString(true);
      }

      for(size_t j = 0; j < those.size(); ++j)
      {
         if(first)
         {
            first = false;
            continue;
         }

         auto& arg2 = those[j];

         if(j == 0)
         {
            ts2 = thatType;
         }
         else
         {
            arg2.ptrs_ = that.ptrs_;
            ts2 = arg2.TypeString(true);
         }

         auto match = arg1.MatchWith(arg2, ts1, ts2);
         if(match == Compatible) return Compatible;
         if(match > best) best = match;
      }
   }

   return best;
}

//------------------------------------------------------------------------------

bool StackArg::CanBeOverloaded() const
{
   Debug::ft("StackArg.CanBeOverloaded");

   if(item_ == nullptr) return false;
   if(Ptrs(true) != 0) return false;
   auto type = item_->Root()->Type();
   return ((type == Cxx::Class) || (type == Cxx::Enum));
}

//------------------------------------------------------------------------------

void StackArg::CheckIfBool() const
{
   Debug::ft("StackArg.CheckIfBool");

   switch(DataSpec::Bool->MustMatchWith(*this))
   {
   case Compatible:
   case Incompatible:
      return;
   }

   Context::Log(NonBooleanConditional);
   item_->Log(NonBooleanConditional, item_, -1);
}

//------------------------------------------------------------------------------

bool StackArg::IsBool() const
{
   return (item_->Name() == BOOL_STR);
}

//------------------------------------------------------------------------------

bool StackArg::IsDefaultCtor(const StackArgVector& args) const
{
   Debug::ft("StackArg.IsDefaultCtor");

   //  For this item to be a constructor, it must either *be* a class or its
   //  name must be that *of* its class.  The first can occur because, when
   //  searching for a constructor, name resolution returns the class, given
   //  that it and the constructor have the same name.
   //
   auto cls = item_->GetClass();
   if(cls == nullptr) return false;
   if(item_->Name() != cls->Name()) return false;

   //  A default constructor has one argument ("this").  A default copy
   //  constructor has a second argument, namely a reference to the class
   //  (or a subclass, if the subclass copy constructor invokes a default
   //  copy constructor in its base class).
   //
   switch(args.size())
   {
   case 1:
      return true;
   case 2:
   {
      auto root = args[1].item_->Root();
      if(root == cls) return true;
      if(root->Type() == Cxx::Class)
      {
         auto derived = static_cast< const Class* >(root);
         return (derived->BaseClass() == cls);
      }
   }
   }

   return false;
}

//------------------------------------------------------------------------------

bool StackArg::IsReadOnly() const
{
   Debug::ft("StackArg.IsReadOnly");

   return (Ptrs(true) == 0 ? const_ : constptr_);
}

//------------------------------------------------------------------------------

TypeMatch StackArg::MatchConst(const StackArg& that, TypeMatch match) const
{
   Debug::ft("StackArg.MatchConst");

   //  o A const argument can be passed to a non-const parameter by value.
   //  o A non-const object can be passed to a const function, but only if
   //    there isn't another overload of the function that is non-const.
   //  Note that this function can be invoked merely to check if two operands
   //  are compatible.  Therefore, if it is invoked during argument matching,
   //  it does not reject passing a const argument to a non-const pointer or
   //  reference.  Instead, it returns Adaptable, which satisifies operand
   //  compatibility checks.  Later on, StackArg.AssignedTo verifies whether
   //  constness was properly interpreted.
   //
   if(this->IsIndirect())
   {
      if(that.IsConst())
      {
         if(!this->IsConst()) return Adaptable;
      }
      else
      {
         if(that.IsThis() && this->IsConst()) return Adaptable;
      }
   }

   return match;
}

//------------------------------------------------------------------------------

TypeMatch StackArg::MatchWith(const StackArg& that,
   const string& thisType, const string& thatType) const
{
   Debug::ft("StackArg.MatchWith");

   if(this->item_ == nullptr) return Incompatible;
   if(that.item_ == nullptr) return Incompatible;
   if(thisType == thatType) return Compatible;
   if(this->item_->IsAuto()) return Compatible;
   if(that.item_->IsAuto()) return Compatible;

   //  See if the types are compatible except for constness.
   //
   auto thisNonCVType = RemoveConsts(thisType);
   auto thatNonCVType = RemoveConsts(thatType);
   if(thisNonCVType == thatNonCVType) return MatchConst(that, Compatible);

   //  The items have different types.  But it's a match if
   //  o a pointer is being assigned to a void* or const void*, or
   //  o nullptr is being assigned to a pointer or nullptr_t.
   //
   if(thisNonCVType == "void*")
   {
      if(that.Ptrs(true) > 0) return Compatible;
   }

   if(thatNonCVType == NULLPTR_T_STR)
   {
      if(this->Ptrs(true) > 0) return Compatible;
      if(thisNonCVType == NULLPTR_T_STR) return Compatible;
   }

   //  Some kind of conversion will be required.  Start by seeing if
   //  the items are compatible as integers.
   //
   auto thisNum = this->NumericType();
   auto thatNum = that.NumericType();
   auto match = thisNum.CalcMatchWith(&thatNum);

   //  If this is a class, instantiate it.  It's then a match if THAT
   //  is a subclass of THIS and their levels of indirection are the
   //  same, or if THIS can be constructed from THAT.
   //
   auto thisRoot = this->item_->Root();

   if(thisRoot->Type() == Cxx::Class)
   {
      auto thisClass = static_cast< Class* >(thisRoot);
      thisClass->Instantiate();

      if(this->Ptrs(true) == that.Ptrs(true))
      {
         auto thatRoot = that.item_->Root();
         if(thatRoot->Type() == Cxx::Class)
         {
            auto thatClass = static_cast< Class* >(thatRoot);
            if(thatClass->DerivesFrom(thisClass))
            {
               thatClass->RecordUsage();
               return MatchConst(that, Derivable);
            }
         }
      }

      if(thisClass->CanConstructFrom(that, thatType)) return Constructible;
   }

   return match;
}

//------------------------------------------------------------------------------

Numeric StackArg::NumericType() const
{
   Debug::ft("StackArg.NumericType");

   if(item_ == nullptr) return Numeric::Nil;
   if(Ptrs(true) > 0) return Numeric::Pointer;

   //  Find the item's numeric type.  If it claims to be a pointer, find
   //  its underlying type.
   //
   auto numeric = item_->GetNumeric();
   if(numeric.Type() != Numeric::PTR) return numeric;

   auto root = item_->Root();
   if(root == nullptr) return Numeric::Nil;
   return root->GetNumeric();
}

//------------------------------------------------------------------------------

bool StackArg::operator==(const StackArg& that) const
{
   return ((item_ == that.item_) && (Ptrs(true) == that.Ptrs(true)));
}

//------------------------------------------------------------------------------

bool StackArg::operator!=(const StackArg& that) const
{
   return !(*this == that);
}

//------------------------------------------------------------------------------

fn_name StackArg_Ptrs = "StackArg.Ptrs";

size_t StackArg::Ptrs(bool arrays) const
{
   Debug::ft(StackArg_Ptrs);

   if(item_ == nullptr) return 0;
   auto spec = item_->GetTypeSpec();
   auto count = (spec == nullptr ? 0 : spec->Ptrs(arrays));
   count += ptrs_;
   if(count >= 0) return count;

   auto expl = "Negative pointer count for " + item_->Trace();
   Context::SwLog(StackArg_Ptrs, expl, count);
   return 0;
}

//------------------------------------------------------------------------------

size_t StackArg::Refs() const
{
   Debug::ft("StackArg.Refs");

   if(item_ == nullptr) return 0;
   auto spec = item_->GetTypeSpec();
   size_t count = (spec == nullptr ? 0 : spec->Refs());
   return count + refs_;
}

//------------------------------------------------------------------------------

fn_name StackArg_SetAsAutoType = "StackArg.SetAsAutoType";

void StackArg::SetAsAutoType() const
{
   Debug::ft(StackArg_SetAsAutoType);

   if(item_ != nullptr)
   {
      AutoType_.item_ = this->item_->AutoType();
      AutoType_.ptrs_ = this->ptrs_;
      AutoType_.const_ = this->const_;
      if(AutoType_.item_ != nullptr) return;
   }

   auto expl = "Auto type not set: item is null";
   Context::SwLog(StackArg_SetAsAutoType, expl, 0);
}

//------------------------------------------------------------------------------

void StackArg::SetAsDirect() const
{
   Debug::ft("StackArg.SetAsDirect");

   if(name_ != nullptr) name_->SetAsDirect();
}

//------------------------------------------------------------------------------

void StackArg::SetAsReadOnly()
{
   Debug::ft("StackArg.SetAsReadOnly");

   if(!item_->IsPointer(false))
      const_ = true;
   else
      constptr_ = true;
}

//------------------------------------------------------------------------------

void StackArg::SetAsTemporary()
{
   Debug::ft("StackArg.SetAsTemporary");

   SetAsWriteable();
   member_ = false;
}

//------------------------------------------------------------------------------

void StackArg::SetAsWriteable()
{
   Debug::ft("StackArg.SetAsWriteable");

   if(!item_->IsPointer(false))
      const_ = false;
   else
      constptr_ = false;
}

//------------------------------------------------------------------------------

fn_name StackArg_SetAutoType = "StackArg.SetAutoType";

void StackArg::SetAutoType()
{
   Debug::ft(StackArg_SetAutoType);

   if(!item_->IsAuto()) return;

   if(AutoType_.SetAutoTypeOn(*static_cast< FuncData* >(item_)))
   {
      //  Now that our underlying type is known, update our constness.
      //
      const_ = item_->IsConst();
      constptr_ = item_->IsConstPtr();
      return;
   }

   auto expl = "Failed to set auto type for " + item_->Name();
   Context::SwLog(StackArg_SetAutoType, expl, 0);
}

//------------------------------------------------------------------------------

bool StackArg::SetAutoTypeFor(const FuncData& data)
{
   Debug::ft("StackArg.SetAutoTypeFor");

   return AutoType_.SetAutoTypeOn(data);
}

//------------------------------------------------------------------------------

bool StackArg::SetAutoTypeOn(const FuncData& data) const
{
   Debug::ft("StackArg.SetAutoTypeOn");

   //  An auto type acquires the type that resulted from the right-hand side
   //  of the expression that was just compiled.  However, it is adjusted to
   //  account for pointers and constness.
   //
   if(item_ == nullptr) return false;

   //  SPEC's referent is currently "auto".  Update it to the auto type.  But
   //  first, see if "const/volatile auto" or "auto* const/volatile" was used.
   //
   auto spec = data.GetTypeSpec();
   auto cauto = spec->IsConst();
   auto cautoptr = spec->IsConstPtr();
   auto vauto = spec->IsVolatile();
   auto vautoptr = spec->IsVolatilePtr();

   //  this->ptrs_ tracked any indirection, address of, or array subscript
   //  operators that were applied to the right-hand side, so it needs to be
   //  carried over to the auto variable.  The TypeSpec that underlies
   //  the right-hand side will be reused by the auto variable, but it must be
   //  adjusted by ptrs_.
   //
   if(item_->Type() == Cxx::TypeSpec)
   {
      //  Make the TypeSpec's referent the auto variable's referent and also
      //  apply its pointers the auto variable.
      //
      auto type = static_cast< TypeSpec* >(item_);
      auto ptrs = ptrs_ + type->Tags()->PtrCount(true);
      spec->SetReferent(type->Referent(), nullptr);
      spec->SetPtrs(ptrs);
   }
   else
   {
      //  ITEM is derived from CxxScoped, so just make it the auto variable's
      //  referent and apply this->ptrs_ to the auto variable.
      //
      spec->SetReferent(static_cast< CxxScoped* >(item_), nullptr);
      spec->SetPtrs(ptrs_);
   }

   //  Now that the auto variable's type has been set, look at its pointers
   //  and references.
   //
   auto ptrs = Ptrs(true);
   auto refs = spec->Tags()->RefCount();

   //  If "const/volatile auto" was used, it applies to the pointer, not the
   //  type, if the variable is a pointer.  Same for const/volatile* auto.
   //
   if(cauto)
   {
      if(ptrs == 0)
         spec->Tags()->SetConst(true);
      else
         spec->Tags()->SetConstPtr();
   }

   if(vauto)
   {
      if(ptrs == 0)
         spec->Tags()->SetVolatile(true);
      else
         spec->Tags()->SetVolatilePtr();
   }

   if(cautoptr) spec->Tags()->SetConstPtr();
   if(vautoptr) spec->Tags()->SetVolatilePtr();

   //  If the right-hand side was const, it carries over to the auto variable
   //  if it is a pointer or reference type.
   //
   if(const_)
   {
      if((spec->Ptrs(true) > 0) || (refs > 0)) spec->Tags()->SetConst(true);
   }

   Context::Trace(CxxTrace::SET_AUTO, *this);
   return true;
}

//------------------------------------------------------------------------------

void StackArg::SetNewPtrs()
{
   Debug::ft("StackArg.SetNewPtrs");

   //  When operator new is invoked, it returns a pointer to memory allocated
   //  for the *top level* type.  If that type has any pointer or array tags,
   //  only the first one matters; any deeper pointers or arrays will require
   //  separate allocations.  We therefore adjust ptrs_ so that the underlying
   //  type can contribute at most *one* pointer (or array).
   //
   auto ptrs = Ptrs(true);
   auto adjust = (ptrs <= 1 ? 1 : 2 - ptrs);
   ptrs_ += adjust;
}

//------------------------------------------------------------------------------

fn_name StackArg_SetNonConst = "StackArg.SetNonConst";

void StackArg::SetNonConst(size_t index) const
{
   Debug::ft(StackArg_SetNonConst);

   auto token = (index == 0 ? item_ : via_);
   if(token == nullptr) return;

   if(mutable_)
   {
      token->WasMutated(this);
      return;
   }

   if(token->SetNonConst()) return;

   auto expl = "const " + token->Name() + " cannot be const";
   Context::SwLog(StackArg_SetNonConst, expl, 0);
}

//------------------------------------------------------------------------------

string StackArg::Trace() const
{
   auto s = item_->Trace();
   AdjustPtrs(s, ptrs_);
   if(!s.empty()) s += SPACE;
   s += '[' + strClass(item_, false) + ']';
   return s;
}

//------------------------------------------------------------------------------

string StackArg::TypeString(bool arg) const
{
   if(item_ == nullptr) return ERROR_STR;
   auto ts = item_->TypeString(arg);
   AdjustPtrs(ts, ptrs_);

   //  Now include constness in the result:
   //  o Prefix "const" if the item isn't const but this argument is.
   //  o Suffix "const" if the item isn't a const pointer but this argument is.
   //
   if(const_ && (ts.find(CONST_STR) != 0))
   {
      ts.insert(0, "const ");
   }

   if(constptr_)
   {
      auto pos = ts.rfind('*');
      if((pos != string::npos) && (ts.find(CONST_STR, pos) == string::npos))
         ts.insert(pos + 1, " const");
   }

   return ts;
}

//------------------------------------------------------------------------------

void StackArg::WasIndexed()
{
   Debug::ft("StackArg.WasIndexed");

   //  If the number of pointers (excluding arrays) attached to this type
   //  accounts for all the pointers that remain (which includes arrays),
   //  then all arrays have been indexed.  In that case, we are indexing
   //  via a pointer, and its target is no longer a member for constness
   //  purposes.
   //
   auto ptrs = Ptrs(true);

   if(item_->GetTypeSpec()->Tags()->PtrCount(false) >= TagCount(ptrs))
   {
      member_ = false;
      constptr_ = false;
   }

   //  We are now at one less level of indirection, so if the pointer count
   //  before the decrement is 1 (or less, which would be an error), then
   //  the underlying type is being referenced directly.
   //
   if(ptrs <= 1) SetAsDirect();
   DecrPtrs();
}

//------------------------------------------------------------------------------

void StackArg::WasRead() const
{
   Debug::ft("StackArg.WasRead");

   if(read_) return;
   if(item_ == nullptr) return;
   read_ = true;
   if(!item_->WasRead()) return;
   Context::Trace(CxxTrace::INCR_READS, *this);
}

//------------------------------------------------------------------------------

fn_name StackArg_WasWritten = "StackArg.WasWritten";

void StackArg::WasWritten() const
{
   Debug::ft(StackArg_WasWritten);

   auto ptrs = Ptrs(true);

   if(item_ == nullptr) return;
   if(ptrs == 0) SetAsDirect();
   if(!item_->WasWritten(this, true, false)) return;
   Context::Trace(CxxTrace::INCR_WRITES, *this);

   //  See if a class was just block-copied.
   //
   if((ptrs == 0) && (Refs() == 0))
   {
      auto root = item_->Root();

      if((root != nullptr) && (root->Type() == Cxx::Class))
      {
         static_cast< Class* >(root)->BlockCopied(this);
      }
   }

   if(!mutable_ && (ptrs > 0 ? constptr_ : const_))
   {
      auto expl = "Write to const " + item_->Name();
      Context::SwLog(StackArg_WasWritten, expl, 0);
   }
   else if(via_ != nullptr)
   {
      SetNonConst(1);
   }

   //  The context function must be non-const if it
   //  writes to a non-mutable member.
   //
   if(!member_ || mutable_) return;
   ContextFunctionIsNonConst();
}

//==============================================================================

TokenTrace::TokenTrace(Action action, const CxxToken* token) :
   CxxTrace(action),
   token_(token)
{
}

//------------------------------------------------------------------------------

bool TokenTrace::Display(ostream& stream, const string& opts)
{
   CxxTrace::Display(stream, opts);
   stream << token_->Trace();
   return true;
}

//==============================================================================

Tracepoint::Tracepoint(const CodeFile* file, size_t line, Action act) :
   file_(file),
   line_(line),
   action_(act),
   parsed_(false),
   compiled_(false)
{
}

//------------------------------------------------------------------------------

void Tracepoint::Display(ostream& stream, const string& prefix) const
{
   stream << prefix << action_ << " at ";
   stream << file_->Name() << ", line " << line_ + 1 << ':' << CRLF;
   auto source = file_->GetLexer().GetNthLine(line_);
   stream << spaces(2) << source << CRLF;
}

//------------------------------------------------------------------------------

void Tracepoint::OnLine(const CodeFile* file, size_t line, bool compiling) const
{
   if(file_ != file) return;
   if(line_ != line) return;
   if(parsed_ && !compiling) return;
   if(compiled_ && compiling) return;

   Debug::ft("Tracepoint.OnLine");

   switch(action_)
   {
   case Break:
      //
      //  Set a breakpoint here to break when the parser reaches
      //  a specified file and line in the source code.
      //
      Debug::noop(0xC0DE);  //@
      break;

   case Start:
   {
      auto buff = Singleton< TraceBuffer >::Instance();
      ThisThread::IncludeInTrace();
      ThisThread::StartTracing(EMPTY_STR);
      Tracing_ = buff->ToolIsOn(ParserTracer);
      break;
   }

   case Stop:
      ThisThread::StopTracing();
      Tracing_ = false;
      break;
   }

   if(compiling)
      compiled_ = true;
   else
      parsed_ = true;
}

//------------------------------------------------------------------------------

bool Tracepoint::operator<(const Tracepoint& that) const
{
   auto name1 = this->file_->Name();
   auto name2 = that.file_->Name();
   if(name1 < name2) return true;
   if(name1 > name2) return false;
   if(this->line_ < that.line_) return true;
   if(this->line_ > that.line_) return false;
   return(this->action_ < that.action_);
}

//------------------------------------------------------------------------------

fixed_string TraceActionStrings[Tracepoint::Action_N] =
{
   ERROR_STR,
   "break",
   "start",
   "stop"
};

ostream& operator<<(ostream& stream, Tracepoint::Action action)
{
   if((action > 0) && (action < Tracepoint::Action_N))
      stream << TraceActionStrings[action];
   else
      stream << TraceActionStrings[0];
   return stream;
}

//==============================================================================

XrefFrame::XrefFrame(XrefUpdater updater) : updater_(updater) { }

//------------------------------------------------------------------------------

TypeName* XrefFrame::FindItem(const string& name) const
{
   for(auto i = items_.cbegin(); i != items_.cend(); ++i)
   {
      if((*i)->Name() == name) return *i;
   }

   return nullptr;
}

//------------------------------------------------------------------------------

void XrefFrame::PushItem(TypeName* item)
{
   if(updater_ == TemplateFunction)
   {
      items_.push_back(item);
   }
}
}
