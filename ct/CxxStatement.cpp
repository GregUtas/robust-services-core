//==============================================================================
//
//  CxxStatement.cpp
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
#include "CxxStatement.h"
#include <sstream>
#include "CodeTypes.h"
#include "CxxExecute.h"
#include "CxxNamed.h"
#include "Debug.h"
#include "SysTypes.h"

using namespace NodeBase;
using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace CodeTools
{
fn_name Break_ctor = "Break.ctor";

Break::Break(size_t pos) : CxxStatement(pos)
{
   Debug::ft(Break_ctor);

   CxxStats::Incr(CxxStats::BREAK);
}

//------------------------------------------------------------------------------

void Break::Print(ostream& stream, const Flags& options) const
{
   stream << BREAK_STR << ';';
}

//==============================================================================

fn_name Case_ctor = "Case.ctor";

Case::Case(ExprPtr& expression, size_t pos) : CxxStatement(pos),
   expr_(expression.release())
{
   Debug::ft(Case_ctor);

   CxxStats::Incr(CxxStats::CASE);
}

//------------------------------------------------------------------------------

void Case::AddToXref() const
{
   expr_->AddToXref();
}

//------------------------------------------------------------------------------

void Case::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   auto lead = prefix.substr(0, prefix.size() - INDENT_SIZE);
   stream << lead << CASE_STR << SPACE;
   expr_->Print(stream, options);
   stream << ':' << CRLF;
}

//------------------------------------------------------------------------------

fn_name Case_EnterBlock = "Case.EnterBlock";

void Case::EnterBlock()
{
   Debug::ft(Case_EnterBlock);

   CxxStatement::EnterBlock();

   expr_->EnterBlock();
   auto result = Context::PopArg(true);
   DataSpec::Int->MustMatchWith(result);
}

//------------------------------------------------------------------------------

void Case::GetUsages(const CodeFile& file, CxxUsageSets& symbols) const
{
   expr_->GetUsages(file, symbols);
}

//==============================================================================

fn_name Catch_ctor = "Catch.ctor";

Catch::Catch(size_t pos) : CxxStatement(pos)
{
   Debug::ft(Catch_ctor);

   CxxStats::Incr(CxxStats::CATCH);
}

//------------------------------------------------------------------------------

void Catch::AddToXref() const
{
   if(arg_ != nullptr) arg_->AddToXref();
   handler_->AddToXref();
}

//------------------------------------------------------------------------------

void Catch::Check() const
{
   if(arg_ != nullptr) arg_->Check();
   handler_->Check();
}

//------------------------------------------------------------------------------

void Catch::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   stream << prefix << CATCH_STR << '(';

   if(arg_ != nullptr)
      arg_->Print(stream, options);
   else
      stream << ELLIPSES_STR;

   stream << ')';

   auto opts = options;
   opts.set(DispLF);
   handler_->Display(stream, prefix, opts);
}

//------------------------------------------------------------------------------

fn_name Catch_EnterBlock = "Catch.EnterBlock";

void Catch::EnterBlock()
{
   Debug::ft(Catch_EnterBlock);

   CxxStatement::EnterBlock();

   if(arg_ != nullptr)
   {
      arg_->EnterScope();
      arg_->EnterBlock();
   }

   handler_->EnterBlock();
}

//------------------------------------------------------------------------------

fn_name Catch_ExitBlock = "Catch.ExitBlock";

void Catch::ExitBlock() const
{
   Debug::ft(Catch_ExitBlock);

   if(arg_ != nullptr) arg_->ExitBlock();
}

//------------------------------------------------------------------------------

fn_name Catch_FindNthItem = "Catch.FindNthItem";

CxxScoped* Catch::FindNthItem(const std::string& name, size_t& n) const
{
   Debug::ft(Catch_FindNthItem);

   return handler_->FindNthItem(name, n);
}

//------------------------------------------------------------------------------

void Catch::GetUsages(const CodeFile& file, CxxUsageSets& symbols) const
{
   if(arg_ != nullptr) arg_->GetUsages(file, symbols);
   handler_->GetUsages(file, symbols);
}

//------------------------------------------------------------------------------

fn_name Catch_LocateItem = "Catch.LocateItem";

bool Catch::LocateItem(const CxxNamed* item, size_t& n) const
{
   Debug::ft(Catch_LocateItem);

   return handler_->LocateItem(item, n);
}

//------------------------------------------------------------------------------

void Catch::Shrink()
{
   if(arg_ != nullptr) arg_->Shrink();
   handler_->Shrink();
}

//==============================================================================

fn_name Condition_ctor = "Condition.ctor";

Condition::Condition(size_t pos) : CxxStatement(pos)
{
   Debug::ft(Condition_ctor);
}

//------------------------------------------------------------------------------

void Condition::AddToXref() const
{
   if(condition_ != nullptr) condition_->AddToXref();
}

//------------------------------------------------------------------------------

fn_name Condition_EnterBlock = "Condition.EnterBlock";

void Condition::EnterBlock()
{
   Debug::ft(Condition_EnterBlock);

   CxxStatement::EnterBlock();

   if(condition_ != nullptr)
   {
      //  The result of the conditional expression should be a boolean.
      //
      condition_->EnterBlock();
      auto result = Context::PopArg(true);
      result.CheckIfBool();
   }
}

//------------------------------------------------------------------------------

void Condition::GetUsages(const CodeFile& file, CxxUsageSets& symbols) const
{
   if(condition_ != nullptr) condition_->GetUsages(file, symbols);
}

//------------------------------------------------------------------------------

void Condition::Print(ostream& stream, const Flags& options) const
{
   Show(stream);
}

//------------------------------------------------------------------------------

bool Condition::Show(ostream& stream) const
{
   if(condition_ == nullptr) return false;
   condition_->Print(stream, NoFlags);
   return true;
}

//==============================================================================

fn_name Continue_ctor = "Continue.ctor";

Continue::Continue(size_t pos) : CxxStatement(pos)
{
   Debug::ft(Continue_ctor);

   CxxStats::Incr(CxxStats::CONTINUE);
}

//------------------------------------------------------------------------------

void Continue::Print(ostream& stream, const Flags& options) const
{
   stream << CONTINUE_STR << ';';
}

//==============================================================================

fn_name CxxStatement_ctor = "CxxStatement.ctor";

CxxStatement::CxxStatement(size_t pos) : pos_(pos)
{
   Debug::ft(CxxStatement_ctor);
}

//------------------------------------------------------------------------------

void CxxStatement::EnterBlock()
{
   Context::SetPos(pos_);
}

//==============================================================================

fn_name Do_ctor = "Do.ctor";

Do::Do(size_t pos) : Condition(pos)
{
   Debug::ft(Do_ctor);

   CxxStats::Incr(CxxStats::DO);
}

//------------------------------------------------------------------------------

void Do::AddToXref() const
{
   loop_->AddToXref();
   Condition::AddToXref();
}

//------------------------------------------------------------------------------

void Do::Check() const
{
   loop_->Check();
}

//------------------------------------------------------------------------------

void Do::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   stream << prefix << DO_STR;

   auto lf = loop_->CrlfOver(Block::Unbraced);

   if(!lf)
   {
      loop_->Print(stream, options);
      stream << SPACE;
   }
   else
   {
      auto opts = options;
      opts.set(DispLF);
      loop_->Display(stream, prefix, opts);
      stream << prefix;
   }

   stream << WHILE_STR << '(';
   Condition::Print(stream, options);
   stream << ");" << CRLF;
}

//------------------------------------------------------------------------------

fn_name Do_EnterBlock = "Do.EnterBlock";

void Do::EnterBlock()
{
   Debug::ft(Do_EnterBlock);

   loop_->EnterBlock();
   Condition::EnterBlock();
}

//------------------------------------------------------------------------------

fn_name Do_FindNthItem = "Do.FindNthItem";

CxxScoped* Do::FindNthItem(const std::string& name, size_t& n) const
{
   Debug::ft(Do_FindNthItem);

   return loop_->FindNthItem(name, n);
}

//------------------------------------------------------------------------------

void Do::GetUsages(const CodeFile& file, CxxUsageSets& symbols) const
{
   loop_->GetUsages(file, symbols);
   Condition::GetUsages(file, symbols);
}

//------------------------------------------------------------------------------

bool Do::InLine() const
{
   return !loop_->CrlfOver(Block::Unbraced);
}

//------------------------------------------------------------------------------

fn_name Do_LocateItem = "Do.LocateItem";

bool Do::LocateItem(const CxxNamed* item, size_t& n) const
{
   Debug::ft(Do_LocateItem);

   return loop_->LocateItem(item, n);
}

//------------------------------------------------------------------------------

void Do::Print(ostream& stream, const Flags& options) const
{
   stream << SPACE << DO_STR;
   loop_->Print(stream, options);
   stream << SPACE << WHILE_STR << '(';
   Condition::Print(stream, options);
   stream << ");";
}

//------------------------------------------------------------------------------

void Do::Shrink()
{
   Condition::Shrink();
   loop_->Shrink();
}

//==============================================================================

fn_name Expr_ctor = "Expr.ctor";

Expr::Expr(ExprPtr& expression, size_t pos) : CxxStatement(pos),
   expr_(expression.release())
{
   Debug::ft(Expr_ctor);

   CxxStats::Incr(CxxStats::EXPR);
}

//------------------------------------------------------------------------------

void Expr::AddToXref() const
{
   expr_->AddToXref();
}

//------------------------------------------------------------------------------

fn_name Expr_EnterBlock = "Expr.EnterBlock";

void Expr::EnterBlock()
{
   Debug::ft(Expr_EnterBlock);

   CxxStatement::EnterBlock();

   expr_->EnterBlock();
}

//------------------------------------------------------------------------------

void Expr::GetUsages(const CodeFile& file, CxxUsageSets& symbols) const
{
   expr_->GetUsages(file, symbols);
}

//------------------------------------------------------------------------------

void Expr::Print(ostream& stream, const Flags& options) const
{
   expr_->Print(stream, options);
   stream << ';';
}

//==============================================================================

fn_name For_ctor = "For.ctor";

For::For(size_t pos) : Condition(pos)
{
   Debug::ft(For_ctor);

   CxxStats::Incr(CxxStats::FOR);
}

//------------------------------------------------------------------------------

void For::AddToXref() const
{
   if(initial_ != nullptr) initial_->AddToXref();
   Condition::AddToXref();
   if(subsequent_ != nullptr) subsequent_->AddToXref();
   loop_->AddToXref();
}

//------------------------------------------------------------------------------

void For::Check() const
{
   if(initial_ != nullptr) initial_->Check();
   loop_->Check();
}

//------------------------------------------------------------------------------

void For::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   auto data = false;
   string info;
   auto stats = options.test(DispStats);

   stream << prefix << FOR_STR << '(';

   //  If initial_ declares a loop variable, as is usually the case, it will
   //  end in a // comment showing the variable's init/read/write counts.  To
   //  make the output parsable, save the comment in INFO and append it later.
   //
   if(initial_ != nullptr)
   {
      std::ostringstream buffer;
      initial_->Print(buffer, options);
      auto init = buffer.str();
      auto pos = init.find(COMMENT_STR);
      if(pos != string::npos)
      {
         info = init.substr(pos - 1);
         init.erase(pos - 1);
      }
      stream << init;
      data = (initial_->Type() == Cxx::Data);
   }
   else
   {
      stream << SPACE;
   }

   if(!data) stream << ';';
   stream << SPACE;

   if(!Condition::Show(stream)) stream << SPACE;
   stream << "; ";
   if(subsequent_ != nullptr) subsequent_->Print(stream, options);
   stream << ')';

   if(options.test(DispNoLF))
   {
      loop_->Print(stream, options);
      if(stats) stream << info;
   }
   else
   {
      auto lf = loop_->CrlfOver(Block::Unbraced);

      if(lf)
      {
         auto opts = options;
         opts.set(DispLF);
         if(stats) stream << info;
         loop_->Display(stream, prefix, opts);
      }
      else
      {
         loop_->Print(stream, options);
         if(stats) stream << info;
         stream << CRLF;
      }
   }
}

//------------------------------------------------------------------------------

fn_name For_EnterBlock = "For.EnterBlock";

void For::EnterBlock()
{
   Debug::ft(For_EnterBlock);

   if(initial_ != nullptr)
   {
      initial_->EnterBlock();
      Context::Clear(4);
   }

   Condition::EnterBlock();

   if(subsequent_ != nullptr)
   {
      subsequent_->EnterBlock();
      Context::PopArg(true);
   }

   loop_->EnterBlock();
}

//------------------------------------------------------------------------------

fn_name For_ExitBlock = "For.ExitBlock";

void For::ExitBlock() const
{
   Debug::ft(For_ExitBlock);

   if(initial_ != nullptr) initial_->ExitBlock();
}

//------------------------------------------------------------------------------

fn_name For_FindNthItem = "For.FindNthItem";

CxxScoped* For::FindNthItem(const std::string& name, size_t& n) const
{
   Debug::ft(For_FindNthItem);

   auto item = initial_->FindNthItem(name, n);
   if(item != nullptr) return item;
   item = subsequent_->FindNthItem(name, n);
   if(item != nullptr) return item;
   return loop_->FindNthItem(name, n);
}

//------------------------------------------------------------------------------

void For::GetUsages(const CodeFile& file, CxxUsageSets& symbols) const
{
   if(initial_ != nullptr) initial_->GetUsages(file, symbols);
   Condition::GetUsages(file, symbols);
   if(subsequent_ != nullptr) subsequent_->GetUsages(file, symbols);
   loop_->GetUsages(file, symbols);
}

//------------------------------------------------------------------------------

bool For::InLine() const
{
   return !loop_->CrlfOver(Block::Unbraced);
}

//------------------------------------------------------------------------------

fn_name For_LocateItem = "For.LocateItem";

bool For::LocateItem(const CxxNamed* item, size_t& n) const
{
   Debug::ft(For_LocateItem);

   if(initial_->LocateItem(item, n)) return true;
   if(subsequent_->LocateItem(item, n)) return true;
   return loop_->LocateItem(item, n);
}

//------------------------------------------------------------------------------

void For::Print(ostream& stream, const Flags& options) const
{
   Display(stream, EMPTY_STR, Flags(LF_Mask));
}

//------------------------------------------------------------------------------

void For::Shrink()
{
   if(initial_ != nullptr) initial_->Shrink();
   Condition::Shrink();
   if(subsequent_ != nullptr) subsequent_->Shrink();
   loop_->Shrink();
}

//==============================================================================

fn_name Goto_ctor = "Goto.ctor";

Goto::Goto(string& label, size_t pos) : CxxStatement(pos)
{
   Debug::ft(Goto_ctor);

   std::swap(label_, label);
   CxxStats::Incr(CxxStats::GOTO);
}

//------------------------------------------------------------------------------

fn_name Goto_EnterBlock = "Goto.EnterBlock";

void Goto::EnterBlock()
{
   Debug::ft(Goto_EnterBlock);

   CxxStatement::EnterBlock();

   //  A full compiler would verify the label here, but
   //  we don't bother to do anything with labels.
}

//------------------------------------------------------------------------------

void Goto::Print(std::ostream& stream, const NodeBase::Flags& options) const
{
   stream << GOTO_STR << SPACE << label_ << ';';
}

//==============================================================================

fn_name If_ctor = "If.ctor";

If::If(size_t pos) : Condition(pos),
   elseif_(false)
{
   Debug::ft(If_ctor);

   CxxStats::Incr(CxxStats::IF);
}

//------------------------------------------------------------------------------

void If::AddToXref() const
{
   Condition::AddToXref();
   then_->AddToXref();
   if(else_ != nullptr) else_->AddToXref();
}

//------------------------------------------------------------------------------

void If::Check() const
{
   then_->Check();
   if(else_ != nullptr) else_->Check();
}

//------------------------------------------------------------------------------

void If::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   if(elseif_)
      stream << SPACE;
   else
      stream << prefix;

   stream << IF_STR << '(';
   Condition::Print(stream, options);
   stream << ')';

   auto lf =
      (else_ != nullptr) || (elseif_) || (then_->CrlfOver(Block::Unbraced));

   if(!lf)
   {
      then_->Print(stream, options);
      stream << CRLF;
      return;
   }

   auto opts = options;
   opts.set(DispLF);
   then_->Display(stream, prefix, opts);
   if(else_ == nullptr) return;
   stream << prefix << ELSE_STR;
   else_->Display(stream, prefix, opts);
}

//------------------------------------------------------------------------------

fn_name If_EnterBlock = "If.EnterBlock";

void If::EnterBlock()
{
   Debug::ft(If_EnterBlock);

   Condition::EnterBlock();
   then_->EnterBlock();
   if(else_ != nullptr) else_->EnterBlock();
}

//------------------------------------------------------------------------------

fn_name If_FindNthItem = "If.FindNthItem";

CxxScoped* If::FindNthItem(const std::string& name, size_t& n) const
{
   Debug::ft(If_FindNthItem);

   auto item = then_->FindNthItem(name, n);
   if(item != nullptr) return item;
   if(else_ == nullptr) return nullptr;
   return else_->FindNthItem(name, n);
}

//------------------------------------------------------------------------------

void If::GetUsages(const CodeFile& file, CxxUsageSets& symbols) const
{
   Condition::GetUsages(file, symbols);
   then_->GetUsages(file, symbols);
   if(else_ != nullptr) else_->GetUsages(file, symbols);
}

//------------------------------------------------------------------------------

bool If::InLine() const
{
   if(else_ != nullptr) return false;
   if(elseif_) return false;
   if(then_->CrlfOver(Block::Unbraced)) return false;
   return true;
}

//------------------------------------------------------------------------------

fn_name If_LocateItem = "If.LocateItem";

bool If::LocateItem(const CxxNamed* item, size_t& n) const
{
   Debug::ft(If_LocateItem);

   if(then_->LocateItem(item, n)) return true;
   if(else_ == nullptr) return false;
   return else_->LocateItem(item, n);
}

//------------------------------------------------------------------------------

void If::Print(ostream& stream, const Flags& options) const
{
   stream << IF_STR << '(';
   Condition::Print(stream, options);
   stream << ')';
   then_->Print(stream, options);
   if(else_ == nullptr) return;

   //  We want multiple lines when an "else" clause exists.  Somehow this
   //  didn't happen, but output the else clause anyway.
   //
   stream << " <@ " << ELSE_STR;
   else_->Print(stream, options);
}

//------------------------------------------------------------------------------

void If::Shrink()
{
   Condition::Shrink();
   then_->Shrink();
   if(else_ != nullptr) else_->Shrink();
}

//==============================================================================

fn_name Label_ctor = "Label.ctor";

Label::Label(string& name, size_t pos) : CxxStatement(pos)
{
   Debug::ft(Label_ctor);

   std::swap(name_, name);
   CxxStats::Incr(CxxStats::LABEL);
}

//------------------------------------------------------------------------------

void Label::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   auto lead = prefix.substr(0, prefix.size() - INDENT_SIZE);
   stream << lead << name_ << ':' << CRLF;
}

//------------------------------------------------------------------------------

fn_name Label_EnterBlock = "Label.EnterBlock";

void Label::EnterBlock()
{
   Debug::ft(Label_EnterBlock);

   CxxStatement::EnterBlock();

   //  A full compiler would add the label to a symbol table
   //  here, but we don't bother to do anything with labels.
}

//------------------------------------------------------------------------------

fn_name Label_ExitBlock = "Label.ExitBlock";

void Label::ExitBlock() const
{
   Debug::ft(Label_ExitBlock);

   //  A full compiler would remove the label from a symbol table
   //  here, but we don't bother to do anything with labels.
}

//==============================================================================

fn_name NoOp_ctor = "NoOp.ctor";

NoOp::NoOp(size_t pos) : CxxStatement(pos)
{
   Debug::ft(NoOp_ctor);

   CxxStats::Incr(CxxStats::NOOP);
}

//------------------------------------------------------------------------------

void NoOp::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   stream << ';' << CRLF;
}

//------------------------------------------------------------------------------

void NoOp::Print(ostream& stream, const Flags& options) const
{
   stream << ';';
}

//==============================================================================

fn_name Return_ctor = "Return.ctor";

Return::Return(size_t pos) : CxxStatement(pos)
{
   Debug::ft(Return_ctor);

   CxxStats::Incr(CxxStats::RETURN);
}

//------------------------------------------------------------------------------

void Return::AddToXref() const
{
   if(expr_ != nullptr) expr_->AddToXref();
}

//------------------------------------------------------------------------------

fn_name Return_EnterBlock = "Return.EnterBlock";

void Return::EnterBlock()
{
   Debug::ft(Return_EnterBlock);

   CxxStatement::EnterBlock();

   if(expr_ != nullptr)
   {
      //  Verify that the result is compatible with what the function is
      //  supposed to return.
      //
      expr_->EnterBlock();
      auto result = Context::PopArg(true);
      auto spec = Context::Scope()->GetFunction()->GetTypeSpec();
      Context::Scope()->GetFunction()->GetTypeSpec()->MustMatchWith(result);
      result.AssignedTo(StackArg(spec, 0, false), Returned);
   }
}

//------------------------------------------------------------------------------

void Return::GetUsages(const CodeFile& file, CxxUsageSets& symbols) const
{
   if(expr_ != nullptr) expr_->GetUsages(file, symbols);
}

//------------------------------------------------------------------------------

void Return::Print(ostream& stream, const Flags& options) const
{
   stream << RETURN_STR;

   if(expr_ != nullptr)
   {
      stream << SPACE;
      expr_->Print(stream, options);
   }

   stream << ';';
}

//==============================================================================

fn_name Switch_ctor = "Switch.ctor[>ct]";

Switch::Switch(size_t pos) : CxxStatement(pos)
{
   Debug::ft(Switch_ctor);

   CxxStats::Incr(CxxStats::SWITCH);
}

//------------------------------------------------------------------------------

void Switch::AddToXref() const
{
   expr_->AddToXref();
   cases_->AddToXref();
}

//------------------------------------------------------------------------------

void Switch::Check() const
{
   cases_->Check();
}

//------------------------------------------------------------------------------

void Switch::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   stream << prefix << SWITCH_STR << '(';
   expr_->Print(stream, options);
   stream << ')';

   auto opts = options;
   opts.set(DispLF);
   cases_->Display(stream, prefix, opts);
}

//------------------------------------------------------------------------------

fn_name Switch_EnterBlock = "Switch.EnterBlock";

void Switch::EnterBlock()
{
   Debug::ft(Switch_EnterBlock);

   CxxStatement::EnterBlock();

   expr_->EnterBlock();
   auto result = Context::PopArg(true);
   DataSpec::Int->MustMatchWith(result);
   cases_->EnterBlock();
}

//------------------------------------------------------------------------------

fn_name Switch_FindNthItem = "Switch.FindNthItem";

CxxScoped* Switch::FindNthItem(const std::string& name, size_t& n) const
{
   Debug::ft(Switch_FindNthItem);

   return cases_->FindNthItem(name, n);
}

//------------------------------------------------------------------------------

void Switch::GetUsages(const CodeFile& file, CxxUsageSets& symbols) const
{
   expr_->GetUsages(file, symbols);
   cases_->GetUsages(file, symbols);
}

//------------------------------------------------------------------------------

fn_name Switch_LocateItem = "Switch.LocateItem";

bool Switch::LocateItem(const CxxNamed* item, size_t& n) const
{
   Debug::ft(Switch_LocateItem);

   return cases_->LocateItem(item, n);
}

//------------------------------------------------------------------------------

void Switch::Shrink()
{
   expr_->Shrink();
   cases_->Shrink();
}

//==============================================================================

fn_name Try_ctor = "Try.ctor";

Try::Try(size_t pos) : CxxStatement(pos)
{
   Debug::ft(Try_ctor);

   CxxStats::Incr(CxxStats::TRY);
}

//------------------------------------------------------------------------------

void Try::AddToXref() const
{
   try_->AddToXref();

   for(auto c = catches_.cbegin(); c != catches_.cend(); ++c)
   {
      (*c)->AddToXref();
   }
}

//------------------------------------------------------------------------------

void Try::Check() const
{
   try_->Check();

   for(auto c = catches_.cbegin(); c != catches_.cend(); ++c)
   {
      (*c)->Check();
   }
}

//------------------------------------------------------------------------------

void Try::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   auto opts = options;
   opts.set(DispLF);
   stream << prefix << TRY_STR;
   try_->Display(stream, prefix, opts);

   for(auto c = catches_.cbegin(); c != catches_.cend(); ++c)
   {
      (*c)->Display(stream, prefix, opts);
   }
}

//------------------------------------------------------------------------------

fn_name Try_EnterBlock = "Try.EnterBlock";

void Try::EnterBlock()
{
   Debug::ft(Try_EnterBlock);

   CxxStatement::EnterBlock();

   try_->EnterBlock();

   for(auto c = catches_.cbegin(); c != catches_.cend(); ++c)
   {
      (*c)->EnterBlock();
   }
}

//------------------------------------------------------------------------------

fn_name Try_ExitBlock = "Try.ExitBlock";

void Try::ExitBlock() const
{
   Debug::ft(Try_ExitBlock);

   for(auto c = catches_.crbegin(); c != catches_.crend(); ++c)
   {
      (*c)->ExitBlock();
   }
}

//------------------------------------------------------------------------------

fn_name Try_FindNthItem = "Try.FindNthItem";

CxxScoped* Try::FindNthItem(const std::string& name, size_t& n) const
{
   Debug::ft(Try_FindNthItem);

   auto item = try_->FindNthItem(name, n);
   if(item != nullptr) return item;

   for(auto c = catches_.cbegin(); c != catches_.cend(); ++c)
   {
      item = (*c)->FindNthItem(name, n);
      if(item != nullptr) return item;
   }

   return nullptr;
}

//------------------------------------------------------------------------------

void Try::GetUsages(const CodeFile& file, CxxUsageSets& symbols) const
{
   try_->GetUsages(file, symbols);

   for(auto c = catches_.cbegin(); c != catches_.cend(); ++c)
   {
      (*c)->GetUsages(file, symbols);
   }
}

//------------------------------------------------------------------------------

fn_name Try_LocateItem = "Try.LocateItem";

bool Try::LocateItem(const CxxNamed* item, size_t& n) const
{
   Debug::ft(Try_LocateItem);

   if(try_->LocateItem(item, n)) return true;

   for(auto c = catches_.cbegin(); c != catches_.cend(); ++c)
   {
      if((*c)->LocateItem(item, n)) return true;
   }

   return false;
}

//------------------------------------------------------------------------------

void Try::Shrink()
{
   try_->Shrink();

   for(auto c = catches_.cbegin(); c != catches_.cend(); ++c)
   {
      (*c)->Shrink();
   }
}

//==============================================================================

fn_name While_ctor = "While.ctor";

While::While(size_t pos) : Condition(pos)
{
   Debug::ft(While_ctor);

   CxxStats::Incr(CxxStats::WHILE);
}

//------------------------------------------------------------------------------

void While::AddToXref() const
{
   Condition::AddToXref();
   loop_->AddToXref();
}

//------------------------------------------------------------------------------

void While::Check() const
{
   loop_->Check();
}

//------------------------------------------------------------------------------

void While::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   stream << prefix << WHILE_STR << '(';
   Condition::Print(stream, options);
   stream << ')';

   auto opts = options;
   opts.set(DispLF, loop_->CrlfOver(Block::Unbraced));
   loop_->Display(stream, prefix, opts);
}

//------------------------------------------------------------------------------

fn_name While_EnterBlock = "While.EnterBlock";

void While::EnterBlock()
{
   Debug::ft(While_EnterBlock);

   Condition::EnterBlock();
   loop_->EnterBlock();
}

//------------------------------------------------------------------------------

fn_name While_FindNthItem = "While.FindNthItem";

CxxScoped* While::FindNthItem(const std::string& name, size_t& n) const
{
   Debug::ft(While_FindNthItem);

   return loop_->FindNthItem(name, n);
}

//------------------------------------------------------------------------------

void While::GetUsages(const CodeFile& file, CxxUsageSets& symbols) const
{
   Condition::GetUsages(file, symbols);
   loop_->GetUsages(file, symbols);
}

//------------------------------------------------------------------------------

bool While::InLine() const
{
   return !loop_->CrlfOver(Block::Unbraced);
}

//------------------------------------------------------------------------------

fn_name While_LocateItem = "While.LocateItem";

bool While::LocateItem(const CxxNamed* item, size_t& n) const
{
   Debug::ft(While_LocateItem);

   return loop_->LocateItem(item, n);
}

//------------------------------------------------------------------------------

void While::Print(ostream& stream, const Flags& options) const
{
   stream << SPACE << WHILE_STR << '(';
   Condition::Print(stream, options);
   stream << ')';
   loop_->Print(stream, options);
}

//------------------------------------------------------------------------------

void While::Shrink()
{
   Condition::Shrink();
   loop_->Shrink();
}
}
