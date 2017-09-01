//==============================================================================
//
//  CxxExecute.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "CxxExecute.h"
#include <cstring>
#include <iosfwd>
#include <sstream>
#include <utility>
#include "CodeFile.h"
#include "CxxArea.h"
#include "CxxNamed.h"
#include "CxxScope.h"
#include "CxxScoped.h"
#include "CxxString.h"
#include "CxxSymbols.h"
#include "CxxToken.h"
#include "Debug.h"
#include "Formatters.h"
#include "NbTracer.h"
#include "Parser.h"
#include "Singleton.h"
#include "ThisThread.h"
#include "ToolTypes.h"
#include "TraceBuffer.h"
#include "TraceDump.h"

using std::string;
using namespace NodeBase;

//------------------------------------------------------------------------------

namespace CodeTools
{
//  Concrete classes for tracing execution.
//
class ActTrace : public CxxTrace
{
public:
   //  Creates a trace record for ACTION, which has no additional data.
   //
   explicit ActTrace(Action action);

   //  Overridden to display the trace record.
   //
   virtual bool Display(std::ostream& stream) override;
};

class ArgTrace : public CxxTrace
{
public:
   //  Creates a trace record for ACTION, which is associated with ARG.
   //
   ArgTrace(Action action, const StackArg& arg);

   //  Overridden to display the trace record.
   //
   virtual bool Display(std::ostream& stream) override;
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
   virtual bool Display(std::ostream& stream) override;
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
   virtual bool Display(std::ostream& stream) override;
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
   ErrTrace(Action action, word err = 0, const std::string& expl = EMPTY_STR);

   //  Not subclassed.
   //
   ~ErrTrace();

   //  Overridden to display the trace record.
   //
   virtual bool Display(std::ostream& stream) override;
private:
   //  Overridden to prohibit copying.
   //
   ErrTrace(const ErrTrace& that);
   void operator=(const ErrTrace& that);

   //  If non-zero, the error associated with the action.
   //
   const word err_;

   //  Any explanation associated with the error.
   //
   stringPtr expl_;
};

//==============================================================================

ActTrace::ActTrace(Action action) : CxxTrace(sizeof(ActTrace), action) { }

//------------------------------------------------------------------------------

bool ActTrace::Display(std::ostream& stream)
{
   CxxTrace::Display(stream);
   return true;
}

//==============================================================================

ArgTrace::ArgTrace(Action action, const StackArg& arg) :
   CxxTrace(sizeof(ArgTrace), action),
   arg_(arg)
{
}

//------------------------------------------------------------------------------

bool ArgTrace::Display(std::ostream& stream)
{
   CxxTrace::Display(stream);
   stream << arg_.Trace();
   return true;
}

//==============================================================================

string Context::Options_ = EMPTY_STR;
CodeFile* Context::File_ = nullptr;
std::vector< ParseFramePtr > Context::Frames_ = std::vector< ParseFramePtr >();
ParseFrame* Context::Frame_ = nullptr;
bool Context::Tracing_ = false;

//------------------------------------------------------------------------------

fn_name Context_AddUsing = "Context.AddUsing";

bool Context::AddUsing(UsingPtr& use)
{
   Debug::ft(Context_AddUsing);

   if(use->EnterScope()) File_->InsertUsing(use);
   return true;
}

//------------------------------------------------------------------------------

fn_name Context_Enter = "Context.Enter";

void Context::Enter(const CxxScoped* owner)
{
   Debug::ft(Context_Enter);

   Trace(CxxTrace::START_SCOPE, 0, owner->ScopedName(true));
   SetPos(owner);
}

//------------------------------------------------------------------------------

string Context::Location()
{
   if(File_ == nullptr) return "[in unknown file]";

   std::ostringstream stream;
   stream << " [in " << File_->Name();

   auto scope = Scope();

   if(scope != nullptr)
   {
      auto name = scope->ScopedName(true);
      string locals(SCOPE_STR);
      locals += LOCALS_STR;
      auto pos = name.find(locals);
      if(pos != string::npos) name.erase(pos);

      if(ParsingTemplateInstance())
      {
         stream << ", while instantiating " << name;
      }
      else
      {
         stream << ", line " << File_->GetLineNum(GetPos()) + 1;
         if(!name.empty()) stream << ", scope " << name;
      }
   }

   stream << ']';
   return stream.str();
}

//------------------------------------------------------------------------------

fn_name Context_Log = "Context.Log";

void Context::Log(Warning warning)
{
   Debug::ft(Context_Log);

   if(File_ == nullptr) return;
   File_->LogPos(GetPos(), warning);
}

//------------------------------------------------------------------------------

bool Context::OptionIsOn(char opt)
{
   return (GetOptions().find(opt) != string::npos);
}

//------------------------------------------------------------------------------

bool Context::ParsingTemplateInstance()
{
   if(Frame_ == nullptr) return false;
   return Frame_->GetParser()->ParsingTemplateInstance();
}

//------------------------------------------------------------------------------

fn_name Context_PopParser= "Context.PopParser";

void Context::PopParser(const Parser* parser)
{
   Debug::ft(Context_PopParser);

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

fn_name Context_PushParser= "Context.PushParser";

void Context::PushParser(const Parser* parser)
{
   Debug::ft(Context_PushParser);

   auto frame = ParseFramePtr(new ParseFrame(parser));
   Frame_ = frame.get();
   Frames_.push_back(std::move(frame));
}

//------------------------------------------------------------------------------

fn_name Context_Reset = "Context.Reset";

void Context::Reset()
{
   Debug::ft(Context_Reset);

   Frame_->Reset();
   File_ = nullptr;
   Block::ResetUsings();
   Singleton< CxxSymbols >::Instance()->EraseLocals();
}

//------------------------------------------------------------------------------

fn_name Context_SetFile = "Context.SetFile";

void Context::SetFile(CodeFile* file)
{
   Debug::ft(Context_SetFile);

   //  This is the start of a new parse, so reinitialize the context.
   //
   Reset();
   File_ = file;
}

//------------------------------------------------------------------------------

fn_name Context_SetPos = "Context.SetPos";

void Context::SetPos(const CxxScoped* scope)
{
   Debug::ft(Context_SetPos);

   if(scope->GetDefnFile() == File_)
      SetPos(scope->GetDefnPos());
   else if(scope->GetDeclFile() == File_)
      SetPos(scope->GetDeclPos());
   else
      Context::SwErr(Context_SetPos, scope->Trace(), 0);
}

//------------------------------------------------------------------------------

fn_name Context_Shutdown = "Context.Shutdown";

void Context::Shutdown(RestartLevel level)
{
   Debug::ft(Context_Shutdown);

   Options_ = EMPTY_STR;
   File_ = nullptr;
   Frames_.clear();
   Frame_ = nullptr;
   Tracing_ = false;
}

//------------------------------------------------------------------------------

fn_name Context_StartTracing = "Context.StartTracing";

bool Context::StartTracing()
{
   Debug::ft(Context_StartTracing);

   auto x = OptionIsOn(TraceExecution);
   auto f = OptionIsOn(TraceFunctions);

   if(!x && !f) return false;

   Tracing_ = x;

   auto buff = Singleton< TraceBuffer >::Instance();
   buff->SetTool(ParserTracer, x);
   buff->SetTool(FunctionTracer, f);

   if(f) NbTracer::SelectThread(ThisThread::RunningThreadId(), TraceIncluded);

   auto immed = Context::OptionIsOn(TraceImmediate);
   ThisThread::StartTracing(immed, false);
   return true;
}

//------------------------------------------------------------------------------

fn_name Context_SwErr = "Context.SwErr";

void Context::SwErr
   (fn_name_arg func, const string& expl, word errval, LogLevel level)
{
   Debug::ft(Context_SwErr);

   auto info = expl + Location();
   Trace(CxxTrace::ERROR, errval, info);
   if(Tracing_ && (level == InfoLog)) return;
   Debug::SwErr(func, info, errval, level);
}

//------------------------------------------------------------------------------

void Context::Trace(CxxTrace::Action act)
{
   if(!Tracing_) return;
   new ActTrace(act);
}

//------------------------------------------------------------------------------

void Context::Trace(CxxTrace::Action act, const StackArg& arg)
{
   if(!Tracing_) return;
   new ArgTrace(act, arg);
}

//------------------------------------------------------------------------------

void Context::Trace(CxxTrace::Action act, word err, const string& expl)
{
   if(!Tracing_) return;
   new ErrTrace(act, err, expl);
}

//------------------------------------------------------------------------------

void Context::Trace(CxxTrace::Action act, const CodeFile& file)
{
   if(!Tracing_) return;
   new FileTrace(act, file);
}

//------------------------------------------------------------------------------

void Context::Trace(CxxTrace::Action act, const CxxToken* token)
{
   if(!Tracing_) return;
   new TokenTrace(act, token);
}

//------------------------------------------------------------------------------

void Context::WasCalled(Function* func)
{
   if(func == nullptr) return;
   func->WasCalled();
   auto arg = StackArg(func, 0);
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
   ERROR_STR  // actually used: appears in execution traces
};

//------------------------------------------------------------------------------

const size_t CxxTrace::ActionWidth = 8;

CxxTrace::CxxTrace(size_t size, Action action) : TraceRecord(size, ParserTracer)
{
   rid_ = action;
}

//------------------------------------------------------------------------------

bool CxxTrace::Display(std::ostream& stream)
{
   auto buff = Singleton< TraceBuffer >::Instance();

   if(buff->ToolIsOn(FunctionTracer))
   {
      TraceRecord::Display(stream);
      stream << spaces(TraceDump::EvtToObj);
   }

   auto& s = ActionStrings[rid_];
   stream << string(ActionWidth - strlen(s), SPACE) << s << TraceDump::Tab();
   return true;
}

//==============================================================================

ErrTrace::ErrTrace(Action action, word err, const std::string& expl) :
   CxxTrace(sizeof(ErrTrace), action),
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

bool ErrTrace::Display(std::ostream& stream)
{
   CxxTrace::Display(stream);

   if(rid_ == ERROR)
   {
      if(expl_ != nullptr) stream << "expl=" << *expl_;
      stream << " err=" << strHex(debug32_t(err_)) << CRLF;
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
   CxxTrace(sizeof(FileTrace), action),
   file_(&file)
{
}

//------------------------------------------------------------------------------

bool FileTrace::Display(std::ostream& stream)
{
   CxxTrace::Display(stream);
   stream << file_->Name();
   return true;
}

//==============================================================================

fn_name ParseFrame_ctor = "ParseFrame.ctor";

ParseFrame::ParseFrame(const Parser* parser) :
   parser_(parser),
   pos_(string::npos)
{
   Debug::ft(ParseFrame_ctor);
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
   Context::SwErr(ParseFrame_Clear, "Operator stack not empty", ops_.size());
   ops_.clear();
}

//------------------------------------------------------------------------------

fn_name ParseFrame_Execute = "ParseFrame.Execute";

void ParseFrame::Execute()
{
   Debug::ft(ParseFrame_Execute);

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

OptionalCode* ParseFrame::Optional() const
{
   if(opts_.empty()) return nullptr;
   return opts_.back();
}

//------------------------------------------------------------------------------

fn_name ParseFrame_PopArg1 = "ParseFrame.PopArg";

StackArg ParseFrame::PopArg(bool read)
{
   Debug::ft(ParseFrame_PopArg1);

   if(args_.empty())
   {
      if(Context::Tracing_)
         new ErrTrace(CxxTrace::POP_ARG, -1);
      else
         Context::SwErr(ParseFrame_PopArg1, "Empty argument stack", 0);
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
      if(Context::Tracing_)
         new ErrTrace(CxxTrace::POP_ARG, -1);
      else
         Context::SwErr(ParseFrame_PopArg2, "Empty argument stack", 0);
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
      if(Context::Tracing_)
         new ErrTrace(CxxTrace::POP_OP, -1);
      else
         Context::SwErr(ParseFrame_PopOp, "Empty operator stack", 0);
      return nullptr;
   }

   auto op = ops_.back();
   ops_.pop_back();
   Context::Trace(CxxTrace::POP_OP, op);
   return op;
}

//------------------------------------------------------------------------------

fn_name ParseFrame_PopOptional = "ParseFrame.PopOptional";

bool ParseFrame::PopOptional()
{
   Debug::ft(ParseFrame_PopOptional);

   if(opts_.empty()) return false;
   opts_.pop_back();
   return true;
}

//------------------------------------------------------------------------------

fn_name ParseFrame_PopScope = "ParseFrame.PopScope";

void ParseFrame::PopScope()
{
   Debug::ft(ParseFrame_PopScope);

   if(!scopes_.empty())
      scopes_.pop_back();
   else
      Context::SwErr(ParseFrame_PopScope, "Empty scope stack", 0);
}

//------------------------------------------------------------------------------

fn_name ParseFrame_PushArg = "ParseFrame.PushArg";

void ParseFrame::PushArg(const StackArg& arg)
{
   Debug::ft(ParseFrame_PushArg);

   if(arg.item == nullptr)
   {
      if(Context::Tracing_)
         new ErrTrace(CxxTrace::PUSH_ARG, -1);
      else
         Context::SwErr(ParseFrame_PushArg, "Push null argument", 0);
      return;
   }

   if(arg.item->Type() != Cxx::Elision)
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
      if(Context::Tracing_)
         new ErrTrace(CxxTrace::PUSH_OP, -1);
      else
         Context::SwErr(ParseFrame_PushOp, "Push null operator", 0);
      return;
   }

   ops_.push_back(op);
   Context::Trace(CxxTrace::PUSH_OP, op);
}

//------------------------------------------------------------------------------

fn_name ParseFrame_PushOptional = "ParseFrame.PushOptional";

void ParseFrame::PushOptional(OptionalCode* opt)
{
   Debug::ft(ParseFrame_PushOptional);

   opts_.push_back(opt);
}

//------------------------------------------------------------------------------

fn_name ParseFrame_PushScope = "ParseFrame.PushScope";

void ParseFrame::PushScope(CxxScope* scope)
{
   Debug::ft(ParseFrame_PushScope);

   scopes_.push_back(scope);
}

//------------------------------------------------------------------------------

fn_name ParseFrame_Reset = "ParseFrame.Reset";

void ParseFrame::Reset()
{
   Debug::ft(ParseFrame_Reset);

   opts_.clear();
   scopes_.clear();
   ops_.clear();
   args_.clear();
   pos_ = string::npos;
}

//------------------------------------------------------------------------------

CxxScope* ParseFrame::Scope() const
{
   return scopes_.back();
}

//------------------------------------------------------------------------------

fn_name ParseFrame_TopArg = "ParseFrame.TopArg";

StackArg* ParseFrame::TopArg()
{
   Debug::ft(ParseFrame_TopArg);

   if(args_.empty()) return nullptr;
   return &args_.back();
}

//------------------------------------------------------------------------------

fn_name ParseFrame_TopOp = "ParseFrame.TopOp";

const Operation* ParseFrame::TopOp() const
{
   Debug::ft(ParseFrame_TopOp);

   if(ops_.empty()) return nullptr;
   return ops_.back();
}

//==============================================================================

const StackArg NilStackArg = StackArg(nullptr, 0);

StackArg StackArg::AutoType_ = NilStackArg;

//------------------------------------------------------------------------------

fn_name StackArg_ctor1 = "StackArg.ctor";

StackArg::StackArg(CxxToken* t, TagCount p) :
   item(t),
   via_(nullptr),
   ptrs_(p),
   member_(false),
   const_(t != nullptr ? t->IsConst() : false),
   constptr_(t != nullptr ? t->IsConstPtr() : false),
   mutable_(false),
   invoke_(false),
   this_(false),
   implicit_(false),
   read_(false)
{
   Debug::ft(StackArg_ctor1);
}

//------------------------------------------------------------------------------

fn_name StackArg_ctor2 = "StackArg.ctor(func)";

StackArg::StackArg(Function* f) :
   item(f),
   via_(nullptr),
   ptrs_(0),
   member_(false),
   const_(f != nullptr ? f->IsConst() : false),
   constptr_(false),
   mutable_(false),
   invoke_(true),
   this_(false),
   implicit_(false),
   read_(false)
{
   Debug::ft(StackArg_ctor2);
}

//------------------------------------------------------------------------------

fn_name StackArg_ctor3 = "StackArg.ctor(via)";

StackArg::StackArg(CxxToken* t, const StackArg& via, Cxx::Operator op) :
   item(t),
   via_(via.item),
   ptrs_(0),
   member_(false),
   const_(t != nullptr ? t->IsConst() : false),
   constptr_(t != nullptr ? t->IsConstPtr() : false),
   mutable_(via.mutable_),
   invoke_(false),
   this_(false),
   implicit_(false),
   read_(false)
{
   Debug::ft(StackArg_ctor3);

   //c Support a via_ chain (that is, also record via_.via_).  This
   //  would fix the bug where b is flagged as "could be const" despite
   //  a statement like b.c.d = n.

   //  If VIA did not access the item through a pointer, tag the item
   //  as read-only if PREV was const, and tag it as a member if VIA
   //  was a member.
   //
   if(op == Cxx::REFERENCE_SELECT)
   {
      if(via.const_) SetAsReadOnly();
      if(via.member_) member_ = true;
   }
   else
   {
      if(*via.item->Name() == THIS_STR)
      {
         //  Tag this as a member of the context class, and tag it as
         //  read-only if "this" was read-only.
         //
         if(via.const_) SetAsReadOnly();
         member_ = true;
      }
   }
}

//------------------------------------------------------------------------------

fn_name StackArg_Arrays = "StackArg.Arrays";

size_t StackArg::Arrays() const
{
   Debug::ft(StackArg_Arrays);

   if(item == nullptr) return 0;
   auto spec = item->GetTypeSpec();
   word count = (spec == nullptr ? 0 : spec->Arrays());
   return count;
}

//------------------------------------------------------------------------------

fn_name StackArg_AssignedTo = "StackArg.AssignedTo";

void StackArg::AssignedTo(const StackArg& that, AssignmentType type) const
{
   Debug::ft(StackArg_AssignedTo);

   if(that.const_) return;
   if(this->item == nullptr) return;
   if(this->item->Type() == Cxx::Terminal) return;
   if(that.item == nullptr) return;

   auto thatPtrs = that.Ptrs(true);
   auto thatRefs = that.Refs();
   auto restricted = false;

   if(thatPtrs > 0)
      restricted = true;
   else if(thatRefs > 0)
      restricted = ((type != Copied) || that.item->IsInitializing());

   if(!restricted) return;

   if(this->const_ && !this->mutable_)
   {
      auto expl = this->TypeString(true);
      expl += " (const) assigned to " + that.TypeString(true);
      Context::SwErr(StackArg_AssignedTo, expl, 0);
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
   auto notPointer = !this->item->IsPointer();

   if((this->via_ != nullptr) && notPointer)
   {
      if(notMutable || !this->via_->IsPointer())
      {
         this->SetNonConst(1);
      }
   }

   //  A T** cannot be assigned to a const T**.
   //  A T* or T*& cannot be assigned to a const T*&.
   //
   auto thisPtrs = this->Ptrs(true);

   if(thisPtrs > 1)
      that.SetNonConst(0);
   else if((thisPtrs == 1) && (thatRefs == 1))
      that.SetNonConst(0);

   //  The context function cannot be const if it exports a non-mutable,
   //  non-pointer member (whether assigned, passed, or returned).
   //
   if(notMutable && notPointer)
   {
      if((this->via_ == nullptr) || (*this->via_->Name() == THIS_STR))
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
         if(this->item->WasWritten(this, true))
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

fn_name StackArg_CalcMatchWith = "StackArg.CalcMatchWith";

TypeMatch StackArg::CalcMatchWith(const StackArg& that,
   const string& thisType, const string& thatType, bool implicit) const
{
   Debug::ft(StackArg_CalcMatchWith);

   auto best = MatchWith(that, thisType, thatType, implicit);
   if(best == Compatible) return best;
   if(that.item == nullptr) return Incompatible;

   //  See if there is a match between any of the types to which the
   //  items can be converted.
   //
   StackArgVector these, those;
   these.push_back(*this);
   those.push_back(that);
   this->item->Root()->GetConvertibleTypes(these);
   that.item->Root()->GetConvertibleTypes(those);
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

         auto match = arg1.MatchWith(arg2, ts1, ts2, implicit);
         if(match == Compatible) return Compatible;
         if(match > best) best = match;
      }
   }

   return best;
}

//------------------------------------------------------------------------------

fn_name StackArg_CanBeOverloaded = "StackArg.CanBeOverloaded";

bool StackArg::CanBeOverloaded() const
{
   Debug::ft(StackArg_CanBeOverloaded);

   if(item == nullptr) return false;
   if(Ptrs(true) != 0) return false;
   auto type = item->Root()->Type();
   return ((type == Cxx::Class) || (type == Cxx::Enum));
}

//------------------------------------------------------------------------------

fn_name StackArg_CheckIfBool = "StackArg.CheckIfBool";

void StackArg::CheckIfBool() const
{
   Debug::ft(StackArg_CheckIfBool);

   switch(DataSpec(BOOL_STR).MustMatchWith(*this))
   {
   case Compatible:
   case Incompatible:
      return;
   }

   Context::Log(NonBooleanConditional);
}

//------------------------------------------------------------------------------

fn_name StackArg_ContextFunctionIsNonConst =
   "StackArg.ContextFunctionIsNonConst";

void StackArg::ContextFunctionIsNonConst()
{
   Debug::ft(StackArg_ContextFunctionIsNonConst);

   auto func = Context::Scope()->GetFunction();
   if(func != nullptr) func->IncrThisWrites();
}

//------------------------------------------------------------------------------

fn_name StackArg_IsDefaultCtor = "StackArg.IsDefaultCtor";

bool StackArg::IsDefaultCtor(const StackArgVector& args) const
{
   Debug::ft(StackArg_IsDefaultCtor);

   //  For this item to be a constructor, it must either *be* a class or its
   //  name must be that *of* its class.  The first can occur because, when
   //  searching for a constructor, name resolution can return the class,
   //  given that it and the constructor have the same name.
   //
   auto cls = item->GetClass();
   if(cls == nullptr) return false;
   if(*item->Name() != *cls->Name()) return false;

   //  A default constructor has one argument ("this").  A default copy
   //  constructor has a second argument, namely a reference to the class.
   //
   switch(args.size())
   {
   case 1:
      return true;
   case 2:
      auto argType = RemoveConsts(args[1].TypeString(true));
      return (argType == cls->TypeString(true));
   }

   return false;
}

//------------------------------------------------------------------------------

fn_name StackArg_IsReadOnly = "StackArg.IsReadOnly";

bool StackArg::IsReadOnly(bool passed) const
{
   Debug::ft(StackArg_IsReadOnly);

   if(passed) return const_;
   return (Ptrs(true) == 0 ? const_ : constptr_);
}

//------------------------------------------------------------------------------

fn_name StackArg_MatchConst = "StackArg.MatchConst";

TypeMatch StackArg::MatchConst(const StackArg& that, TypeMatch match) const
{
   Debug::ft(StackArg_MatchConst);

   //  o A const argument can be passed to a non-const parameter by value.
   //  o A non-const object can be passed to a const function, but only if
   //    there isn't another overload of the function that is non-const.
   //  Note that this function can be invoked merely to check if two operands
   //  were compatible.  Consequently, if it is being invoked during argument
   //  matching, it does not reject passing a const argument to a non-const
   //  pointer or reference.  Instead, it returns Adaptable, which satisifies
   //  operand compatibility checks.  Later on, StackArg.AssignedTo verifies
   //  whether constness was properly interpreted.
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

fn_name StackArg_MatchWith = "StackArg.MatchWith";

TypeMatch StackArg::MatchWith(const StackArg& that,
   const string& thisType, const string& thatType, bool implicit) const
{
   Debug::ft(StackArg_MatchWith);

   if(this->item == nullptr) return Incompatible;
   if(that.item == nullptr) return Incompatible;
   if(thisType == thatType) return Compatible;
   if(this->item->IsAuto()) return Compatible;
   if(that.item->IsAuto()) return Compatible;

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
   auto thisRoot = this->item->Root();

   if(thisRoot->Type() == Cxx::Class)
   {
      auto thisClass = static_cast< Class* >(thisRoot);
      thisClass->Instantiate();

      if(this->Ptrs(true) == that.Ptrs(true))
      {
         auto thatRoot = that.item->Root();
         if(thatRoot->Type() == Cxx::Class)
         {
            auto thatClass = static_cast< Class* >(thatRoot);
            if(thatClass->DerivesFrom(thisClass))
            {
               thatClass->RecordUsage();
               return MatchConst(that, Compatible);
            }
         }
      }

      if(thisClass->CanConstructFrom(that, thatType, implicit))
         return Constructible;
   }

   return match;
}

//------------------------------------------------------------------------------

fn_name StackArg_NumericType = "StackArg.NumericType";

Numeric StackArg::NumericType() const
{
   Debug::ft(StackArg_NumericType);

   if(item == nullptr) return Numeric::Nil;
   if(Ptrs(true) > 0) return Numeric::Pointer;
   auto root = item->Root();
   if(root == nullptr) return Numeric::Nil;
   return root->GetNumeric();
}

//------------------------------------------------------------------------------

bool StackArg::operator==(const StackArg& that) const
{
   return ((this->item == that.item) && (this->Ptrs(true) == that.Ptrs(true)));
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

   if(item == nullptr) return 0;
   auto spec = item->GetTypeSpec();
   word count = (spec == nullptr ? 0 : spec->Ptrs(arrays));
   count += ptrs_;
   if(count >= 0) return size_t(count);

   auto expl = "Negative pointer count for " + item->Trace();
   Context::SwErr(StackArg_Ptrs, expl, count);
   return 0;
}

//------------------------------------------------------------------------------

fn_name StackArg_Refs = "StackArg.Refs";

size_t StackArg::Refs() const
{
   Debug::ft(StackArg_Refs);

   if(item == nullptr) return 0;
   auto spec = item->GetTypeSpec();
   return (spec == nullptr ? 0 : spec->Refs());
}

//------------------------------------------------------------------------------

fn_name StackArg_SetAsAutoType = "StackArg.SetAsAutoType";

void StackArg::SetAsAutoType() const
{
   Debug::ft(StackArg_SetAsAutoType);

   if(item != nullptr)
   {
      AutoType_.item = this->item->AutoType();
      AutoType_.ptrs_ = this->ptrs_;
      AutoType_.const_ = this->const_;
      if(AutoType_.item != nullptr) return;
   }

   auto expl = "Auto type not set for " + Trace();
   Context::SwErr(StackArg_SetAsAutoType, expl, 0);
}

//------------------------------------------------------------------------------

fn_name StackArg_SetAsReadOnly = "StackArg.SetAsReadOnly";

void StackArg::SetAsReadOnly()
{
   Debug::ft(StackArg_SetAsReadOnly);

   if(!item->IsPointer())
      const_ = true;
   else
      constptr_ = true;
}

//------------------------------------------------------------------------------

fn_name StackArg_SetAsWriteable = "StackArg.SetAsWriteable";

void StackArg::SetAsWriteable()
{
   Debug::ft(StackArg_SetAsWriteable);

   if(!item->IsPointer())
      const_ = false;
   else
      constptr_ = false;
}

//------------------------------------------------------------------------------

fn_name StackArg_SetAutoType = "StackArg.SetAutoType";

void StackArg::SetAutoType()
{
   Debug::ft(StackArg_SetAutoType);

   if(!item->IsAuto()) return;

   if(AutoType_.SetAutoTypeOn(*static_cast< FuncData* >(item)))
   {
      //  Now that our underlying type is known, update our constness.
      //
      const_ = item->IsConst();
      constptr_ = item->IsConstPtr();
      return;
   }

   auto expl = "Failed to set auto type for " + *item->Name();
   Context::SwErr(StackArg_SetAutoType, expl, 0);
}

//------------------------------------------------------------------------------

fn_name StackArg_SetAutoTypeFor = "StackArg.SetAutoTypeFor";

bool StackArg::SetAutoTypeFor(FuncData& data)
{
   Debug::ft(StackArg_SetAutoTypeFor);

   return AutoType_.SetAutoTypeOn(data);
}

//------------------------------------------------------------------------------

fn_name StackArg_SetAutoTypeOn = "StackArg.SetAutoTypeOn";

bool StackArg::SetAutoTypeOn(FuncData& data) const
{
   Debug::ft(StackArg_SetAutoTypeOn);

   //  An auto type acquires the type that resulted from the right-hand side
   //  of the expression that was just executed.  However, it is adjusted to
   //  account for pointers and constness.
   //
   if(item == nullptr) return false;

   //  SPEC's referent is currently "auto".  Update it to the auto type.
   //  But first, see if "const auto" or "auto* const" was used.
   //
   auto spec = data.GetTypeSpec();
   auto constauto = spec->IsConst();
   auto constautoptr = spec->IsConstPtr();
   auto ref = static_cast< CxxNamed* >(item);
   spec->SetReferent(ref, NoUsing);

   //  RefCount() is the number of references that were attached to "auto"
   //  (usually 0, but 1 when "auto&" is used).  Unless "auto&" is used, the
   //  auto variable is a copy of, not a reference to, the right-hand side.
   //
   auto refs = spec->RefCount();
   if(refs == 0) spec->RemoveRefs();

   //  ptrs_ tracked any indirection, address of, or array subscript operators
   //  that were applied to the right-hand side.  The TypeSpec that underlies
   //  the right-hand side will be reused by the auto variable, but it must be
   //  adjusted by ptrs_.
   //
   spec->AdjustPtrs(ptrs_);
   auto ptrs = Ptrs(true);

   //  If "const auto" was used, it applies to the pointer, rather than the
   //  type, if the auto variable is a pointer.  Handle const* auto as well.
   //
   if(constauto)
   {
      if(ptrs == 0)
         spec->SetConst(true);
      else
         spec->SetConstPtr(true);
   }

   if(constautoptr) spec->SetConstPtr(true);

   //  If the right-hand side was const, it carries over to the auto variable
   //  if it is a pointer or reference type.
   //
   if(const_)
   {
      if((spec->Ptrs(true) > 0) || (refs > 0)) spec->SetConst(true);
   }

   Context::Trace(CxxTrace::SET_AUTO, *this);
   return true;
}

//------------------------------------------------------------------------------

fn_name StackArg_SetNonConst = "StackArg.SetNonConst";

void StackArg::SetNonConst(size_t index) const
{
   Debug::ft(StackArg_SetNonConst);

   auto token = (index == 0 ? item : via_);
   if(token == nullptr) return;

   if(mutable_)
   {
      token->WasMutated(this);
      return;
   }

   if(token->SetNonConst()) return;

   auto expl = "const " + *token->Name() + " cannot be const";
   Context::SwErr(StackArg_SetNonConst, expl, 0);
}

//------------------------------------------------------------------------------

string StackArg::Trace() const
{
   auto s = item->Trace();
   AdjustPtrs(s, ptrs_);
   if(!s.empty()) s += SPACE;
   s += '[' + strClass(item, false) + ']';
   return s;
}

//------------------------------------------------------------------------------

string StackArg::TypeString(bool arg) const
{
   if(item == nullptr) return ERROR_STR;
   auto ts = item->TypeString(arg);
   AdjustPtrs(ts, ptrs_);
   return ts;
}

//------------------------------------------------------------------------------

fn_name StackArg_WasRead = "StackArg.WasRead";

void StackArg::WasRead() const
{
   Debug::ft(StackArg_WasRead);

   if(read_) return;
   if(item == nullptr) return;
   read_ = true;
   if(!item->WasRead()) return;
   Context::Trace(CxxTrace::INCR_READS, *this);
}

//------------------------------------------------------------------------------

fn_name StackArg_WasWritten = "StackArg.WasWritten";

void StackArg::WasWritten() const
{
   Debug::ft(StackArg_WasWritten);

   if(item == nullptr) return;
   if(!item->WasWritten(this, false)) return;
   Context::Trace(CxxTrace::INCR_WRITES, *this);

   auto ptrs = Ptrs(true);

   //  See if a class was just block-copied.
   //
   if((ptrs == 0) && (Refs() == 0))
   {
      auto root = item->Root();

      if((root != nullptr) && (root->Type() == Cxx::Class))
      {
         static_cast< Class* >(root)->BlockCopied(this);
      }
   }

   if(!mutable_ && (ptrs > 0 ? constptr_ : const_))
   {
      auto expl = "Write to const " + *item->Name();
      Context::SwErr(StackArg_WasWritten, expl, 0);
   }
   else
   {
      if(via_ != nullptr) SetNonConst(1);
   }

   //  The context function must be non-const if it
   //  writes to a non-mutable member.
   //
   if(!member_ || mutable_) return;
   ContextFunctionIsNonConst();
}

//==============================================================================

TokenTrace::TokenTrace(Action action, const CxxToken* token) :
   CxxTrace(sizeof(TokenTrace), action),
   token_(token)
{
}

//------------------------------------------------------------------------------

bool TokenTrace::Display(std::ostream& stream)
{
   CxxTrace::Display(stream);
   stream << token_->Trace();
   return true;
}
}
