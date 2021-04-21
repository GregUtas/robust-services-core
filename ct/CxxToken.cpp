//==============================================================================
//
//  CxxToken.cpp
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
#include "CxxToken.h"
#include <sstream>
#include "CodeFile.h"
#include "CxxArea.h"
#include "CxxExecute.h"
#include "CxxNamed.h"
#include "CxxRoot.h"
#include "CxxScope.h"
#include "CxxScoped.h"
#include "CxxSymbols.h"
#include "Debug.h"
#include "Formatters.h"
#include "Lexer.h"
#include "Singleton.h"

using namespace NodeBase;
using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace CodeTools
{
bool IsSortedByFilePos(const CxxToken* item1, const CxxToken* item2)
{
   auto file1 = item1->GetFile();
   auto file2 = item2->GetFile();
   if(file1 == nullptr)
   {
      if(file2 != nullptr) return true;
   }
   else if(file2 == nullptr)
   {
      return false;
   }
   else
   {
      auto fn1 = file1->Path(false);
      auto fn2 = file2->Path(false);
      auto result = fn1.compare(fn2);
      if(result < 0) return true;
      if(result > 0) return false;
   }

   auto pos1 = item1->GetPos();
   auto pos2 = item2->GetPos();
   if(pos1 < pos2) return true;
   if(pos1 > pos2) return false;
   return (item1 < item2);
}

//------------------------------------------------------------------------------

bool IsSortedByPos(const CxxToken* item1, const CxxToken* item2)
{
   if(item1->GetPos() < item2->GetPos()) return true;
   if(item1->GetPos() > item2->GetPos()) return false;
   return (item1 < item2);
}

//==============================================================================

AlignAs::AlignAs(TokenPtr& token) : token_(std::move(token))
{
   Debug::ft("AlignAs.ctor");

   CxxStats::Incr(CxxStats::ALIGNAS);
}

//------------------------------------------------------------------------------

void AlignAs::AddToXref()
{
   token_->AddToXref();
}

//------------------------------------------------------------------------------

void AlignAs::Check() const
{
   token_->Check();
}

//------------------------------------------------------------------------------

void AlignAs::EnterBlock()
{
   Debug::ft("AlignAs.EnterBlock");

   token_->EnterBlock();
}

//------------------------------------------------------------------------------

void AlignAs::GetUsages(const CodeFile& file, CxxUsageSets& symbols)
{
   token_->GetUsages(file, symbols);
}

//------------------------------------------------------------------------------

void AlignAs::Print(ostream& stream, const Flags& options) const
{
   stream << ALIGNAS_STR << '(';
   token_->Print(stream, options);
   stream << ")";
}

//------------------------------------------------------------------------------

void AlignAs::Shrink()
{
   CxxToken::Shrink();
   token_->Shrink();
}

//------------------------------------------------------------------------------

void AlignAs::UpdatePos
   (EditorAction action, size_t begin, size_t count, size_t from) const
{
   CxxToken::UpdatePos(action, begin, count, from);
   token_->UpdatePos(action, begin, count, from);
}

//==============================================================================

ArraySpec::ArraySpec(ExprPtr& expr) : expr_(expr.release())
{
   Debug::ft("ArraySpec.ctor");

   CxxStats::Incr(CxxStats::ARRAY_SPEC);
}

//------------------------------------------------------------------------------

void ArraySpec::AddToXref()
{
   if(expr_ != nullptr) expr_->AddToXref();
}

//------------------------------------------------------------------------------

void ArraySpec::Check() const
{
   if(expr_ != nullptr) expr_->Check();
}

//------------------------------------------------------------------------------

void ArraySpec::EnterBlock()
{
   Debug::ft("ArraySpec.EnterBlock");

   if(expr_ != nullptr) expr_->EnterBlock();
}

//------------------------------------------------------------------------------

void ArraySpec::GetUsages(const CodeFile& file, CxxUsageSets& symbols)
{
   if(expr_ != nullptr) expr_->GetUsages(file, symbols);
}

//------------------------------------------------------------------------------

void ArraySpec::Print(ostream& stream, const Flags& options) const
{
   stream << '[';
   if(expr_ != nullptr) expr_->Print(stream, options);
   stream << ']';
}

//------------------------------------------------------------------------------

void ArraySpec::Shrink()
{
   CxxToken::Shrink();
   if(expr_ != nullptr) expr_->Shrink();
}

//------------------------------------------------------------------------------

string ArraySpec::TypeString(bool arg) const
{
   return (arg ? "*" : ARRAY_STR);
}

//------------------------------------------------------------------------------

void ArraySpec::UpdatePos
   (EditorAction action, size_t begin, size_t count, size_t from) const
{
   CxxToken::UpdatePos(action, begin, count, from);
   if(expr_ != nullptr) expr_->UpdatePos(action, begin, count, from);
}

//==============================================================================

CxxScoped* BoolLiteral::Referent() const
{
   Debug::ft("BoolLiteral.Referent");

   return Singleton< CxxRoot >::Instance()->BoolTerm();
}

//==============================================================================

BraceInit::BraceInit()
{
   Debug::ft("BraceInit.ctor");

   CxxStats::Incr(CxxStats::BRACE_INIT);
}

//------------------------------------------------------------------------------

void BraceInit::AddToXref()
{
   for(auto i = items_.cbegin(); i != items_.cend(); ++i)
   {
      (*i)->AddToXref();
   }
}

//------------------------------------------------------------------------------

void BraceInit::Check() const
{
   for(auto i = items_.cbegin(); i != items_.cend(); ++i)
   {
      (*i)->Check();
   }
}

//------------------------------------------------------------------------------

void BraceInit::EnterBlock()
{
   Debug::ft("BraceInit.EnterBlock");

   for(auto i = items_.cbegin(); i != items_.cend(); ++i)
   {
      (*i)->EnterBlock();
      Context::PopArg(true);
   }

   //c The above has left the argument stack empty, but something needs to
   //  be available for the pending assignment operation.  It should be the
   //  type of structure being initialized, but we'll just return "auto",
   //  which acts as a wildcard when checking LHS and RHS compatibility.
   //
   StackArg arg(Singleton< CxxRoot >::Instance()->AutoTerm(), 0, false);
   Context::PushArg(arg);
}

//------------------------------------------------------------------------------

void BraceInit::GetUsages(const CodeFile& file, CxxUsageSets& symbols)
{
   for(auto i = items_.cbegin(); i != items_.cend(); ++i)
   {
      (*i)->GetUsages(file, symbols);
   }
}

//------------------------------------------------------------------------------

void BraceInit::Print(ostream& stream, const Flags& options) const
{
   stream << "{ ";

   for(auto i = items_.cbegin(); i != items_.cend(); ++i)
   {
      (*i)->Print(stream, options);
      if(*i != items_.back()) stream << ',';
      stream << SPACE;
   }

   stream << '}';
}

//------------------------------------------------------------------------------

void BraceInit::Shrink()
{
   CxxToken::Shrink();
   ShrinkTokens(items_);
   auto size = items_.capacity() * sizeof(TokenPtr);
   CxxStats::Vectors(CxxStats::BRACE_INIT, size);
}

//------------------------------------------------------------------------------

void BraceInit::UpdatePos
   (EditorAction action, size_t begin, size_t count, size_t from) const
{
   CxxToken::UpdatePos(action, begin, count, from);

   for(auto i = items_.cbegin(); i != items_.cend(); ++i)
   {
      (*i)->UpdatePos(action, begin, count, from);
   }
}

//==============================================================================

CxxToken::CxxToken()
{
   Debug::ft("CxxToken.ctor");
}

//------------------------------------------------------------------------------

CxxToken::CxxToken(const CxxToken& that) : LibraryItem(that),
   loc_(that.loc_)
{
   Debug::ft("CxxToken.ctor(copy)");
}

//------------------------------------------------------------------------------

CxxToken::~CxxToken()
{
   Debug::ftnt("CxxToken.dtor");
}

//------------------------------------------------------------------------------

void CxxToken::CopyContext(const CxxToken* that)
{
   Debug::ft("CxxToken.CopyContext");

   loc_.SetLoc(that->GetFile(), that->GetPos());
   loc_.SetInternal(true);
}

//------------------------------------------------------------------------------

Class* CxxToken::DirectClass() const
{
   Debug::ft("CxxToken.DirectClass");

   auto spec = GetTypeSpec();
   if(spec == nullptr) return nullptr;
   return spec->DirectClass();
}

//------------------------------------------------------------------------------

void CxxToken::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   stream << prefix;
   Print(stream, options);
   stream << CRLF;
}

//------------------------------------------------------------------------------

fn_name CxxToken_EnterBlock = "CxxToken.EnterBlock";

void CxxToken::EnterBlock()
{
   Debug::ft(CxxToken_EnterBlock);

   Context::SwLog(CxxToken_EnterBlock, strOver(this), 0);
}

//------------------------------------------------------------------------------

CxxScoped* CxxToken::FindTemplateAnalog(const CxxToken* item) const
{
   Debug::ft("CxxToken.FindTemplateAnalog");

   auto inst = GetTemplateInstance();
   if(inst == nullptr) return nullptr;
   return inst->FindTemplateAnalog(item);
}

//------------------------------------------------------------------------------

bool CxxToken::GetRange(size_t& begin, size_t& left, size_t& end) const
{
   begin = string::npos;
   left = string::npos;
   end = string::npos;
   return false;
}

//------------------------------------------------------------------------------

TypeName* CxxToken::GetTemplateArgs() const
{
   Debug::ft("CxxToken.GetTemplateArgs");

   auto name = GetQualName();
   return (name != nullptr ? name->GetTemplateArgs() : nullptr);
}

//------------------------------------------------------------------------------

CxxScope* CxxToken::GetTemplateInstance() const
{
   auto scope = GetScope();
   if(scope == nullptr) return nullptr;
   return scope->GetTemplateInstance();
}

//------------------------------------------------------------------------------

bool CxxToken::IsInTemplateInstance() const
{
   return (GetTemplateInstance() != nullptr);
}

//------------------------------------------------------------------------------

bool CxxToken::IsPointer(bool arrays) const
{
   auto spec = GetTypeSpec();
   if(spec == nullptr) return (GetNumeric().Type() == Numeric::PTR);
   auto ptrs = spec->Ptrs(arrays);
   return (ptrs > 0);
}

//------------------------------------------------------------------------------

fn_name CxxToken_Log = "CxxToken.Log";

void CxxToken::Log(Warning warning,
   const CxxToken* item, word offset, const string& info) const
{
   Debug::ft(CxxToken_Log);

   //  If this warning is associated with a template instance, log it
   //  against the template.
   //
   auto inst = GetTemplateInstance();

   if(inst != nullptr)
   {
      auto that = inst->FindTemplateAnalog(this);
      if(that == nullptr) return;
      if(item != nullptr) item = inst->FindTemplateAnalog(item);
      that->Log(warning, item, offset, info);
      return;
   }

   auto err = 0;
   auto file = GetFile();
   auto pos = GetPos();

   if(file == nullptr)
   {
      file = Context::File();
      err += 2;
   }

   if(pos == string::npos)
   {
      pos = Context::GetPos();
      err += 1;
   }

   if(err != 0)
   {
      auto expl = string("Location not set for ") + strClass(this, false);
      Context::SwLog(CxxToken_Log, expl, err);
   }

   if(item == nullptr) item = this;
   file->LogPos(pos, warning, item, offset, info);
}

//------------------------------------------------------------------------------

CxxToken& CxxToken::operator=(const CxxToken& that)
{
   Debug::ft("CxxToken.operator=");

   this->loc_ = that.loc_;
   return *this;
}

//------------------------------------------------------------------------------

void CxxToken::Print(ostream& stream, const Flags& options) const
{
   stream << "// " << ERROR_STR << '(' << strClass(this, false) << ')';
}

//------------------------------------------------------------------------------

fn_name CxxToken_Referent = "CxxToken.Referent";

CxxScoped* CxxToken::Referent() const
{
   Debug::ft(CxxToken_Referent);

   Debug::SwLog(CxxToken_Referent, strOver(this), 0);
   return nullptr;
}

//------------------------------------------------------------------------------

CxxScoped* CxxToken::ReferentDefn() const
{
   Debug::ft("CxxToken.ReferentDefn");

   auto ref1 = Referent();

   if((ref1 != nullptr) && ref1->IsForward())
   {
      auto ref2 = ref1->Referent();
      if(ref2 != nullptr) return ref2;
   }

   return ref1;
}

//------------------------------------------------------------------------------

CxxToken* CxxToken::Root() const
{
   Debug::ft("CxxToken.Root");

   CxxToken* prev = const_cast< CxxToken* >(this);
   CxxToken* curr = prev->RootType();

   while(curr != prev)
   {
      if(curr == nullptr) return prev;
      prev = curr;
      curr = curr->RootType();
   }

   return prev;
}

//------------------------------------------------------------------------------

void CxxToken::SetContext(size_t pos)
{
   Debug::ft("CxxToken.SetContext");

   loc_.SetLoc(Context::File(), pos);
}

//------------------------------------------------------------------------------

void CxxToken::SetLoc(CodeFile* file, size_t pos) const
{
   Debug::ft("CxxToken.SetLoc");

   loc_.SetLoc(file, pos);
}

//------------------------------------------------------------------------------

void CxxToken::SetLoc(CodeFile* file, size_t pos, bool internal) const
{
   Debug::ft("CxxToken.SetLoc(internal)");

   SetLoc(file, pos);
   loc_.SetInternal(internal);
}

//------------------------------------------------------------------------------

void CxxToken::ShrinkTokens(const TokenPtrVector& tokens)
{
   for(auto t = tokens.cbegin(); t != tokens.cend(); ++t)
   {
      (*t)->Shrink();
   }
}

//------------------------------------------------------------------------------

void CxxToken::UpdatePos
   (EditorAction action, size_t begin, size_t count, size_t from) const
{
   loc_.UpdatePos(action, begin, count, from);
}

//------------------------------------------------------------------------------

fn_name CxxToken_WasWritten = "CxxToken.WasWritten";

bool CxxToken::WasWritten(const StackArg* arg, bool direct, bool indirect)
{
   Debug::ft(CxxToken_WasWritten);

   auto expl = "Write not supported to " + Trace();
   Context::SwLog(CxxToken_WasWritten, expl, Type());
   return false;
}

//==============================================================================

const TokenPtr Expression::StartOfExpr =
   TokenPtr(new Operation(Cxx::START_OF_EXPRESSION));

//------------------------------------------------------------------------------

Expression::Expression(size_t end, bool force) :
   end_(end),
   force_(force)
{
   Debug::ft("Expression.ctor");

   CxxStats::Incr(CxxStats::EXPRESSION);
}

//------------------------------------------------------------------------------

bool Expression::AddBinaryOp(TokenPtr& item)
{
   Debug::ft("Expression.AddBinaryOp");

   auto oper = static_cast< Operation* >(item.get());

   if(!items_.empty())
   {
      //  ITEM is a binary operator and something preceded it.
      //  o If a constant or variable, make that the first argument.
      //  o If an operator, elide.  If the previous operator needs another
      //    argument, it can elide forward.  This occurs, for example, in
      //      a = (t) b;
      //    where operator= can take the result of the cast as its second
      //    argument.  Normally, however, the new binary operator elides
      //    backwards, because the expression
      //      a = <binop>
      //    is an error when there is nothing before the binary operator.
      //
      auto& prev = items_.back();
      TokenPtr arg;

      if(prev->Type() != Cxx::Operation)
      {
         arg.reset(prev.release());
         items_.pop_back();
         oper->AddArg(arg, true);
      }
      else
      {
         auto ante = static_cast< Operation* >(prev.get());

         if(!ante->ElideForward())
         {
            arg.reset(new Elision);
            oper->AddArg(arg, true);
         }
      }
   }
   else
   {
      switch(oper->Op())
      {
      case Cxx::CAST:
      case Cxx::STATIC_CAST:
      case Cxx::CONST_CAST:
      case Cxx::DYNAMIC_CAST:
      case Cxx::REINTERPRET_CAST:
         //
         //  These are handled by dedicated functions, so simply add them
         //  when requested to do so.
         //
         break;

      default:
         //
         //  Nothing preceded this binary operator.  Back up and try another
         //  parse.  This occurs with "(<name>) <binOp>", which looks like
         //  a cast until <binOp> appears.  If Parser.GetTypeSpec (used by
         //  Parser.GetCast) checked that its name was actually a type, this
         //  might be avoided, but it does not do so.
         //
         return false;
      }
   }

   items_.push_back(std::move(item));
   return true;
}

//------------------------------------------------------------------------------

fn_name Expression_AddItem = "Expression.AddItem";

bool Expression::AddItem(TokenPtr& item)
{
   Debug::ft(Expression_AddItem);

   //  The first item sets the position where the expression begins.
   //
   if(items_.empty())
   {
      SetContext(item->GetPos());
   }

   if(item->Type() == Cxx::Operation)
   {
      //  We're adding an operator.  See how many arguments it takes.
      //
      auto oper = static_cast< Operation* >(item.get());

      switch(CxxOp::Attrs[oper->Op()].arguments)
      {
      case 2:
         return AddBinaryOp(item);
      case 1:
         return AddUnaryOp(item);
      }

      return AddVariableOp(item);
   }

   //  This is a variable or constant.  What preceded it?
   //  o If nothing, add it as the first item in the expression.
   //  o If a constant or variable, log an error.
   //  o If an operator, add it as an argument unless the operator is full.
   //
   if(items_.empty())
   {
      items_.push_back(std::move(item));
      return true;
   }

   auto& prev = items_.back();
   auto type = prev->Type();

   if(type != Cxx::Operation)
   {
      Debug::SwLog(Expression_AddItem, "unexpected item type", type);
      return false;
   }

   auto oper = static_cast< Operation* >(prev.get());
   auto op = oper->Op();
   auto& attrs = CxxOp::Attrs[op];

   if(oper->ArgsSize() < attrs.arguments)
   {
      oper->AddArg(item, false);
      return true;
   }

   //  We get here if an operator takes a variable number of arguments, which
   //  is coded as attrs.arguments = 0.  However, the parser handles the only
   //  operators of this type (function calls, new, and new[]) by assembling
   //  all of the arguments itself, in GetArgList and GetNew.
   //
   Debug::SwLog(Expression_AddItem, "unexpected operator", oper->Op());
   return false;
}

//------------------------------------------------------------------------------

void Expression::AddToXref()
{
   for(auto i = items_.cbegin(); i != items_.cend(); ++i)
   {
      (*i)->AddToXref();
   }
}

//------------------------------------------------------------------------------

fn_name Expression_AddUnaryOp = "Expression.AddUnaryOp";

bool Expression::AddUnaryOp(TokenPtr& item)
{
   Debug::ft(Expression_AddUnaryOp);

   auto oper = static_cast< Operation* >(item.get());

   if(items_.empty())
   {
      //  ++ and -- are initially parsed as postfix operators.  But
      //  this operator begins an expression, so it must be prefix.
      //
      switch(oper->Op())
      {
      case Cxx::POSTFIX_INCREMENT:
         oper->SetOp(Cxx::PREFIX_INCREMENT);
         break;
      case Cxx::POSTFIX_DECREMENT:
         oper->SetOp(Cxx::PREFIX_DECREMENT);
         break;
      }
   }
   else
   {
      //  It's an error if something precedes operator delete.
      //
      switch(oper->Op())
      {
      case Cxx::OBJECT_DELETE:
      case Cxx::OBJECT_DELETE_ARRAY:
         Debug::SwLog(Expression_AddUnaryOp, "unexpected args", oper->Op());
         return false;
      }

      auto prev = items_.back()->Back();

      if(!prev->AppendUnary())
      {
         //  This operator has both binary and unary interpretations.
         //  The previous token thinks that the binary interpretation
         //  is correct.  Note: This also acquires the argument for a
         //  postfix increment/decrement operator.
         //
         if(!oper->MakeBinary()) return false;
         return AddBinaryOp(item);
      }
   }

   //  Add this unary operator to the expression.  It either began it
   //  or was preceded by another operator.
   //
   items_.push_back(std::move(item));
   return true;
}

//------------------------------------------------------------------------------

fn_name Expression_AddVariableOp = "Expression.AddVariableOp";

bool Expression::AddVariableOp(TokenPtr& item)
{
   Debug::ft(Expression_AddVariableOp);

   auto oper = static_cast< Operation* >(item.get());
   auto op = oper->Op();

   //  o If nothing preceded the operator, add it as the first item.
   //  o Add a function call immediately, as it can be preceded by either an
   //    operator (probably "." or "->", which acquired the function name as
   //    an argument) or the function name itself (in a bare function call).
   //  o Add a conditional operator immediately, as it elides backwards to
   //    the expression before the "?".
   //
   if(items_.empty() || (op == Cxx::FUNCTION_CALL) || (op == Cxx::CONDITIONAL))
   {
      items_.push_back(std::move(item));
      return true;
   }

   //  This is operator new.  It can be used alone (handled above) or after
   //  an operator (typically operator=) that will elide forward.
   //
   auto& prev = items_.back();

   if(prev->Type() == Cxx::Operation)
   {
      auto ante = static_cast< Operation* >(prev.get());

      if(ante->ElideForward())
      {
         items_.push_back(std::move(item));
         return true;
      }

      Debug::SwLog(Expression_AddVariableOp, "failed to elide", ante->Op());
      return false;
   }

   Debug::SwLog(Expression_AddVariableOp, "unexpected item", prev->Type());
   return false;
}

//------------------------------------------------------------------------------

CxxToken* Expression::Back()
{
   Debug::ft("Expression.Back");

   if(items_.empty()) return nullptr;
   return items_.back()->Back();
}

//------------------------------------------------------------------------------

void Expression::Check() const
{
   for(auto i = items_.cbegin(); i != items_.cend(); ++i)
   {
      (*i)->Check();
   }
}

//------------------------------------------------------------------------------

void Expression::EnterBlock()
{
   Debug::ft("Expression.EnterBlock");

   //  If evaluation of this expression is to be forced at its end_, mark
   //  the beginning of the expression by pushing a token onto the operator
   //  stack.  Compile each of the items in the expression, and force the
   //  compilation of anything still above our start token.
   //
   if(force_) Start();

   for(auto i = items_.cbegin(); i != items_.cend(); ++i)
   {
      (*i)->EnterBlock();
   }

   if(force_) Context::Execute();
}

//------------------------------------------------------------------------------

void Expression::GetUsages(const CodeFile& file, CxxUsageSets& symbols)
{
   for(auto i = items_.cbegin(); i != items_.cend(); ++i)
   {
      (*i)->GetUsages(file, symbols);
   }
}

//------------------------------------------------------------------------------

void Expression::Print(ostream& stream, const Flags& options) const
{
   for(auto i = items_.cbegin(); i != items_.cend(); ++i)
   {
      (*i)->Print(stream, options);
   }
}

//------------------------------------------------------------------------------

void Expression::Shrink()
{
   CxxToken::Shrink();
   ShrinkTokens(items_);
   auto size = items_.capacity() * sizeof(TokenPtr);
   CxxStats::Vectors(CxxStats::EXPRESSION, size);
}

//------------------------------------------------------------------------------

void Expression::Start()
{
   Debug::ft("Expression.Start");

   //  Push StartOfExpr onto the stack.  Its priority is lower than all other
   //  operators.  This allows an expression to push its operators onto the
   //  stack during compilation.
   //
   Context::PushOp(static_cast< Operation* >(StartOfExpr.get()));
}

//------------------------------------------------------------------------------

string Expression::Trace() const
{
   std::ostringstream stream;
   Print(stream, NoFlags);
   return stream.str();
}

//------------------------------------------------------------------------------

void Expression::UpdatePos
   (EditorAction action, size_t begin, size_t count, size_t from) const
{
   CxxToken::UpdatePos(action, begin, count, from);

   for(auto i = items_.cbegin(); i != items_.cend(); ++i)
   {
      (*i)->UpdatePos(action, begin, count, from);
   }
}

//==============================================================================

Numeric FloatLiteral::GetNumeric() const
{
   switch(tags_.size_)
   {
   case SIZE_D: return Numeric::Double;
   case SIZE_F: return Numeric::Float;
   case SIZE_L: return Numeric::LongDouble;
   }

   return Numeric::Nil;
}

//------------------------------------------------------------------------------

void FloatLiteral::Print(ostream& stream, const Flags& options) const
{
   if(tags_.exp_)
      stream << std::scientific;
   else
      stream << std::fixed;
   stream << num_ << std::dec;

   switch(tags_.size_)
   {
   case SIZE_F:
      stream << 'F';
      break;
   case SIZE_L:
      stream << 'L';
   }
}

//------------------------------------------------------------------------------

CxxScoped* FloatLiteral::Referent() const
{
   Debug::ft("FloatLiteral.Referent");

   auto base = Singleton< CxxRoot >::Instance();

   switch(tags_.size_)
   {
   case SIZE_D: return base->DoubleTerm();
   case SIZE_F: return base->FloatTerm();
   case SIZE_L: return base->LongDoubleTerm();
   }

   return nullptr;
}

//------------------------------------------------------------------------------

string FloatLiteral::TypeString(bool arg) const
{
   return Referent()->TypeString(arg);
}

//==============================================================================

Numeric IntLiteral::BaseNumeric() const
{
   if(tags_.unsigned_)
   {
      switch(tags_.size_)
      {
      case SIZE_I: return Numeric::uInt;
      case SIZE_L: return Numeric::uLong;
      case SIZE_LL: return Numeric::uLongLong;
      }
   }
   else
   {
      switch(tags_.size_)
      {
      case SIZE_I: return Numeric::Int;
      case SIZE_L: return Numeric::Long;
      case SIZE_LL: return Numeric::LongLong;
      }
   }

   return Numeric::Nil;
}

//------------------------------------------------------------------------------

Numeric IntLiteral::GetNumeric() const
{
   Debug::ft("IntLiteral.GetNumeric");

   //  Get the default numeric type for this constant and adjust its width
   //  to what is actually needed to represent the constant.  In this way,
   //  Numeric::CalcMatchWith can determine that a "0", for example, is
   //  Convertible (rather than Abridgeable) to a function argument with a
   //  type of uint8_t, even though the default type for "0" is a full int.
   //
   auto numeric = BaseNumeric();

   if(tags_.unsigned_)
   {
      if(num_ <= UINT8_MAX)
         numeric.SetWidth(sizeof(uint8_t) << 3);
      else if(num_ <= UINT16_MAX)
         numeric.SetWidth(sizeof(uint16_t) << 3);
      else if(num_ <= UINT32_MAX)
         numeric.SetWidth(sizeof(uint32_t) << 3);
   }
   else
   {
      if((num_ >= INT8_MIN) && (num_ <= INT8_MAX))
         numeric.SetWidth(sizeof(int8_t) << 3);
      else if((num_ >= INT16_MIN) && (num_ <= INT16_MAX))
         numeric.SetWidth(sizeof(int16_t) << 3);
      else if((num_ >= INT32_MIN) && (num_ <= INT32_MAX))
         numeric.SetWidth(sizeof(int32_t) << 3);
   }

   return numeric;
}

//------------------------------------------------------------------------------

void IntLiteral::Print(ostream& stream, const Flags& options) const
{
   switch(tags_.radix_)
   {
   case HEX:
      stream << "0x" << std::hex << num_ << std::dec;
      break;
   case OCT:
      stream << '0' << std::oct << num_ << std::dec;
      break;
   default:
      if(tags_.unsigned_)
         stream << uint64_t(num_);
      else
         stream << num_;
   }

   if(tags_.unsigned_) stream << 'U';

   switch(tags_.size_)
   {
   case SIZE_L:
      stream << "L";
      break;
   case SIZE_LL:
      stream << "LL";
   }
}

//------------------------------------------------------------------------------

CxxScoped* IntLiteral::Referent() const
{
   Debug::ft("IntLiteral.Referent");

   auto base = Singleton< CxxRoot >::Instance();

   if(tags_.unsigned_)
   {
      switch(tags_.size_)
      {
      case SIZE_I: return base->uIntTerm();
      case SIZE_L: return base->uLongTerm();
      case SIZE_LL: return base->uLongLongTerm();
      }
   }
   else
   {
      switch(tags_.size_)
      {
      case SIZE_I: return base->IntTerm();
      case SIZE_L: return base->LongTerm();
      case SIZE_LL: return base->LongLongTerm();
      }
   }

   return nullptr;
}

//------------------------------------------------------------------------------

string IntLiteral::TypeString(bool arg) const
{
   return Referent()->TypeString(arg);
}

//==============================================================================

Literal::Literal()
{
   Debug::ft("Literal.ctor");
}

//------------------------------------------------------------------------------

CxxToken* Literal::AutoType() const
{
   return Referent()->AutoType();
}

//------------------------------------------------------------------------------

void Literal::EnterBlock()
{
   Debug::ft("Literal.EnterBlock");

   Context::PushArg(StackArg(this, 0, false));
}

//------------------------------------------------------------------------------

const string& Literal::Name() const
{
   return Referent()->Name();
}

//------------------------------------------------------------------------------

CxxToken* Literal::RootType() const
{
   return Referent()->Root();
}

//------------------------------------------------------------------------------

string Literal::Trace() const
{
   std::ostringstream stream;
   Print(stream, NoFlags);
   return stream.str();
}

//------------------------------------------------------------------------------

Cxx::ItemType Literal::Type() const
{
   return Referent()->Type();
}

//==============================================================================

CxxScoped* NullPtr::Referent() const
{
   Debug::ft("NullPtr.Referent");

   return Singleton< CxxRoot >::Instance()->NullptrTerm();
}

//==============================================================================

Operation::Operation(Cxx::Operator op) :
   op_(op),
   fcnew_(false),
   overload_(nullptr)
{
   Debug::ft("Operation.ctor");

   CxxStats::Incr(CxxStats::OPERATION);
}

//------------------------------------------------------------------------------

fn_name Operation_AddArg = "Operation.AddArg";

void Operation::AddArg(TokenPtr& arg, bool prefixed)
{
   Debug::ft(Operation_AddArg);

   auto& attrs = CxxOp::Attrs[op_];

   if(arg == nullptr)
   {
      Debug::SwLog(Operation_AddArg, "null argument", op_);
      return;
   }

   if((attrs.arguments != 0) && (args_.size() >= attrs.arguments))
   {
      Debug::SwLog(Operation_AddArg, "too many arguments", op_);
   }

   args_.push_back(std::move(arg));

   if(!prefixed)
   {
      //  The argument appeared after the operator.  The postfix increment
      //  and decrement operators have higher precedence than their prefix
      //  counterparts, so they get matched first.  But since the argument
      //  followed the operator, make it prefix.
      //
      switch(op_)
      {
      case Cxx::POSTFIX_INCREMENT:
         op_ = Cxx::PREFIX_INCREMENT;
         break;
      case Cxx::POSTFIX_DECREMENT:
         op_ = Cxx::PREFIX_DECREMENT;
         break;
      }
   }
}

//------------------------------------------------------------------------------

void Operation::AddToXref()
{
   for(auto a = args_.cbegin(); a != args_.cend(); ++a)
   {
      (*a)->AddToXref();
   }
}

//------------------------------------------------------------------------------

fn_name Operation_AppendUnary = "Operation.AppendUnary";

bool Operation::AppendUnary()
{
   Debug::ft(Operation_AppendUnary);

   //  When a unary operator appears after a function call,
   //  it must be binary (e.g. f() + 1).
   //
   if(op_ == Cxx::FUNCTION_CALL) return false;

   //  Other operators elide forward to the unary operator
   //  (e.g. i + *j, i + -3).
   //
   if(ElideForward()) return true;

   Debug::SwLog(Operation_AppendUnary, "failed to elide", op_);
   return false;
}

//------------------------------------------------------------------------------

size_t Operation::ArgCapacity() const
{
   Debug::ft("Operation.ArgCapacity");

   auto& attrs = CxxOp::Attrs[op_];
   if(attrs.arguments == 0) return SIZE_MAX;
   auto curr = args_.size();
   if(curr >= attrs.arguments) return 0;
   return attrs.arguments - curr;
}

//------------------------------------------------------------------------------

CxxToken* Operation::Back()
{
   Debug::ft("Operation.Back");

   auto size = args_.size();
   if(size == 0) return this;

   auto& attrs = CxxOp::Attrs[op_];
   if(attrs.arguments == 0) return args_.back().get();
   if(size >= attrs.arguments) return args_.back().get();
   return this;
}

//------------------------------------------------------------------------------

void Operation::Check() const
{
   if(!IsInternal())
   {
      auto& attrs = CxxOp::Attrs[op_];
      auto& lexer = GetFile()->GetLexer();
      auto pos = GetPos();
      auto lchar = lexer.At(pos - 1);
      auto rchar = lexer.At(pos + attrs.symbol.size());

      switch(attrs.spacing[0])
      {
      case '@':
         if((WhitespaceChars.find(lchar) != string::npos) &&
            (lexer.LineFindFirst(pos) != pos))
         {
            if((op_ != Cxx::FUNCTION_CALL) || !fcnew_)
            {
               Log(OperatorSpacing);
            }
         }
         break;

      case '_':
         if((WhitespaceChars.find(lchar) == string::npos) && (lchar != '('))
         {
            Log(OperatorSpacing);
         }
         break;
      }

      switch(attrs.spacing[1])
      {
      case '@':
         if((WhitespaceChars.find(rchar) != string::npos) && (rchar != CRLF))
         {
            Log(OperatorSpacing);
         }
         break;
      case '_':
         if((WhitespaceChars.find(rchar) == string::npos) && (rchar != ')'))
         {
            Log(OperatorSpacing);
         }
         break;
      }
   }

   for(auto a = args_.cbegin(); a != args_.cend(); ++a)
   {
      (*a)->Check();
   }
}

//------------------------------------------------------------------------------

void Operation::CheckBitwiseOp(const StackArg& arg1, const StackArg& arg2) const
{
   Debug::ft("Operation.CheckBitwiseOp");

   switch(op_)
   {
      case Cxx::BITWISE_AND:
      case Cxx::BITWISE_OR:
      case Cxx::BITWISE_AND_ASSIGN:
      case Cxx::BITWISE_OR_ASSIGN:
         if(arg1.IsBool() || arg2.IsBool())
         {
            Log(BitwiseOperatorOnBoolean);
         }
   }
}

//------------------------------------------------------------------------------

void Operation::CheckCast(const StackArg& inArg, const StackArg& outArg) const
{
   Debug::ft("Operation.CheckCast");

   //  Some casts are always logged.
   //
   auto constCast = false;

   switch(op_)
   {
   case Cxx::REINTERPRET_CAST:
      Log(ReinterpretCast);
      break;

   case Cxx::CAST:
      Log(UseOfCast);
      break;
   }

   //  Log the removal of const qualification.
   //
   if(!outArg.IsConst())
   {
      switch(op_)
      {
      case Cxx::CONST_CAST:
      case Cxx::CAST:
         if((inArg.IsConst()) && (inArg.IsIndirect() == outArg.IsIndirect()))
         {
            Log(CastingAwayConstness);
            constCast = true;
         }
         break;

      default:
         //
         //  Other casts cannot remove constness, which means that inArg
         //  cannot be const.  If it actually *is* const, don't log it:
         //  not all compilers enforce constness in exactly the same way.
         //
         if(!inArg.IsConst()) inArg.SetNonConst(0);
      }
   }

   //  Log downcasting.
   //
   Class* inClass = nullptr;
   auto inRoot = inArg.item->Root();
   if((inRoot != nullptr) && (inRoot->Type() == Cxx::Class))
      inClass = static_cast< Class* >(inRoot);

   Class* outClass = nullptr;
   auto outRoot = outArg.item->Root();
   if((outRoot != nullptr) && (outRoot->Type() == Cxx::Class))
      outClass = static_cast< Class* >(outRoot);

   if((inClass != nullptr) && (outClass != nullptr))
   {
      if(outClass->DerivesFrom(inClass))
      {
         Log(Downcasting);
         outClass->RecordUsage();

         if((op_ != Cxx::STATIC_CAST) && (op_ != Cxx::DYNAMIC_CAST))
         {
            if(!constCast && !outClass->IsInTemplateInstance())
            {
               Log(ExcessiveCast);
            }
         }
      }
      else if((inClass == outClass) || (inClass->DerivesFrom(outClass)))
      {
         if(!constCast && Context::ParsingSourceCode())
         {
            Log(UnnecessaryCast);
         }
      }
   }
}

//------------------------------------------------------------------------------

void Operation::DisplayArg(ostream& stream, size_t index) const
{
   if(index < args_.size())
      args_.at(index)->Print(stream, NoFlags);
   else
      stream << ERROR_STR << "(arg=" << index << ')';
}

//------------------------------------------------------------------------------

void Operation::DisplayNew(ostream& stream) const
{
   auto call = static_cast< Operation* >(args_.front().get());

   stream << NEW_STR << SPACE;

   if(call->ArgsSize() > 0)
   {
      DisplayArg(stream, 0);
      stream << SPACE;
   }

   for(size_t i = 1; i < args_.size(); ++i)
   {
      DisplayArg(stream, i);
   }
}

//------------------------------------------------------------------------------

bool Operation::ElideForward()
{
   Debug::ft("Operation.ElideForward");

   //  An operator can elide forward if it needs one more argument.
   //
   if(ArgCapacity() != 1) return false;

   TokenPtr arg(new Elision);
   args_.push_back(std::move(arg));
   return true;
}

//------------------------------------------------------------------------------

void Operation::EnterBlock()
{
   Debug::ft("Operation.EnterBlock");

   auto& attrs = CxxOp::Attrs[op_];

   switch(attrs.arguments)
   {
   case 1:
      if((op_ == Cxx::POSTFIX_INCREMENT) ||
         (op_ == Cxx::POSTFIX_DECREMENT))
      {
         args_.front()->EnterBlock();
         Push();
      }
      else
      {
         Push();
         args_.front()->EnterBlock();
      }
      break;

   case 2:
      args_.front()->EnterBlock();
      Push();
      args_.back()->EnterBlock();
      break;

   default:
      switch(op_)
      {
      case Cxx::OBJECT_CREATE:
      case Cxx::OBJECT_CREATE_ARRAY:
         Context::PushOp(this);
         ExecuteNew();
         Context::PopOp();
         break;

      default:
         Push();
         PushArgs();
      }
   }
}

//------------------------------------------------------------------------------

fn_name Operation_Execute = "Operation.Execute";

void Operation::Execute() const
{
   Debug::ft(Operation_Execute);

   StackArg arg1 = NilStackArg;
   StackArg arg2 = NilStackArg;

   //  Pop the argument(s) if this is a unary or binary operator.
   //  Other types of operators will pop their arguments later.
   //
   switch(CxxOp::Attrs[op_].arguments)
   {
   case 2:
      if(!Context::PopArg(arg2)) return;
      //  [[fallthrough]]
   case 1:
      if(!Context::PopArg(arg1)) return;
   }

   switch(op_)
   {
   case Cxx::REFERENCE_SELECT:
   case Cxx::POINTER_SELECT:
      //
      //  ARG2 is accessing one of ARG1's members.
      //
      if(IsOverloaded(arg1))
         arg1 = Context::PopArg(false);
      else
         Record(op_, arg1, &arg2);
      PushMember(arg1, arg2);
      return;

   case Cxx::ARRAY_SUBSCRIPT:
      //
      //  Push ARG1 again.
      //
      if(IsOverloaded(arg1, arg2)) return;
      Record(op_, arg1, &arg2);
      Context::PushArg(arg1.EraseName());
      return;

   case Cxx::FUNCTION_CALL:
      ExecuteCall();
      return;

   case Cxx::POSTFIX_INCREMENT:
   case Cxx::POSTFIX_DECREMENT:
   case Cxx::PREFIX_INCREMENT:
   case Cxx::PREFIX_DECREMENT:
      //
      //  Push ARG1 again.
      //
      if(IsOverloaded(arg1)) return;
      Record(op_, arg1, &arg2);
      Context::PushArg(arg1.EraseName());
      return;

   case Cxx::TYPE_NAME:
      //
      //  Push a typeid result.
      //
      Record(op_, arg1, &arg2);
      PushType("type_info");
      return;

   case Cxx::CONST_CAST:
   case Cxx::DYNAMIC_CAST:
   case Cxx::REINTERPRET_CAST:
   case Cxx::STATIC_CAST:
   case Cxx::CAST:
      //
      //  Push ARG1 (a TypeSpec).
      //
      CheckCast(arg2, arg1);
      Record(op_, arg1, &arg2);
      Context::PushArg(arg1.EraseName());
      return;

   case Cxx::SIZEOF_TYPE:
   case Cxx::ALIGNOF_TYPE:
      //
      //  Push a size_t result.
      //
      Record(op_, arg1, &arg2);
      PushType("size_t");
      return;

   case Cxx::NOEXCEPT:
      //
      //  Push a bool result.
      //
      Record(op_, arg1, &arg2);
      PushType("bool");
      return;

   case Cxx::ONES_COMPLEMENT:
   case Cxx::UNARY_PLUS:
   case Cxx::UNARY_MINUS:
      //
      //  Push ARG1 again.
      //
      if(IsOverloaded(arg1)) return;
      Record(op_, arg1, &arg2);
      Context::PushArg(arg1.EraseName());
      return;

   case Cxx::LOGICAL_NOT:
      //
      //  Push ARG1 again after checking that it's a boolean.
      //
      if(IsOverloaded(arg1)) return;
      Record(op_, arg1, &arg2);
      arg1.CheckIfBool();
      Context::PushArg(arg1.EraseName());
      return;

   case Cxx::ADDRESS_OF:
      //
      //  Push ARG1 after incrementing its indirection level.
      //
      if(IsOverloaded(arg1)) return;
      Record(op_, arg1, &arg2);
      arg1.IncrPtrs();
      Context::PushArg(arg1.EraseName());
      return;

   case Cxx::INDIRECTION:
      //
      //  Push ARG1 after decrementing its indirection level.
      //
      if(IsOverloaded(arg1)) return;
      Record(op_, arg1, &arg2);
      arg1.DecrPtrs();
      Context::PushArg(arg1.EraseName());
      return;

   case Cxx::OBJECT_CREATE:
   case Cxx::OBJECT_CREATE_ARRAY:
      ExecuteNew();
      return;

   case Cxx::OBJECT_DELETE:
   case Cxx::OBJECT_DELETE_ARRAY:
      ExecuteDelete(arg1);
      return;

   case Cxx::REFERENCE_SELECT_MEMBER:
   case Cxx::POINTER_SELECT_MEMBER:
      //
      //c Support the .* and ->* operators.
      //
      Debug::SwLog(Operation_Execute, "unsupported operator", op_);
      return;

   case Cxx::MULTIPLY:
   case Cxx::DIVIDE:
   case Cxx::MODULO:
   case Cxx::ADD:
   case Cxx::SUBTRACT:
   case Cxx::LEFT_SHIFT:
   case Cxx::RIGHT_SHIFT:
   case Cxx::BITWISE_AND:
   case Cxx::BITWISE_XOR:
   case Cxx::BITWISE_OR:
      CheckBitwiseOp(arg1, arg2);
      if(IsOverloaded(arg1, arg2)) return;
      Record(op_, arg1, &arg2);
      PushResult(arg1, arg2);
      return;

   case Cxx::LESS:
   case Cxx::LESS_OR_EQUAL:
   case Cxx::GREATER:
   case Cxx::GREATER_OR_EQUAL:
   case Cxx::EQUALITY:
   case Cxx::INEQUALITY:
      if(IsOverloaded(arg1, arg2)) return;
      Record(op_, arg1, &arg2);
      PushResult(arg1, arg2);
      return;

   case Cxx::LOGICAL_AND:
   case Cxx::LOGICAL_OR:
      if(IsOverloaded(arg1, arg2))
      {
         Log(OperatorOverloaded);
         return;
      }
      Record(op_, arg1, &arg2);
      arg1.CheckIfBool();
      arg2.CheckIfBool();
      PushResult(arg1, arg2);
      return;

   case Cxx::CONDITIONAL:
      //
      //  A read on each of the three arguments.  Push ARG2.
      {
         StackArg arg3 = NilStackArg;

         if(!Context::PopArg(arg3)) return;
         if(!Context::PopArg(arg2)) return;
         if(!Context::PopArg(arg1)) return;
         Record(op_, arg1, &arg2);
         arg3.WasRead();
         arg1.CheckIfBool();
         if(arg2.item->TypeString(true) == NULLPTR_T_STR)
            Context::PushArg(arg3.EraseName());
         else
            Context::PushArg(arg2.EraseName());
      }
      return;

   case Cxx::ASSIGN:
      if(IsOverloaded(arg1, arg2)) return;
      arg2.SetAsAutoType();
      arg1.SetAutoType();
      Record(op_, arg1, &arg2);
      arg2.AssignedTo(arg1, Copied);
      PushResult(arg1, arg2);
      return;

   case Cxx::MULTIPLY_ASSIGN:
   case Cxx::DIVIDE_ASSIGN:
   case Cxx::MODULO_ASSIGN:
   case Cxx::ADD_ASSIGN:
   case Cxx::SUBTRACT_ASSIGN:
   case Cxx::LEFT_SHIFT_ASSIGN:
   case Cxx::RIGHT_SHIFT_ASSIGN:
   case Cxx::BITWISE_AND_ASSIGN:
   case Cxx::BITWISE_XOR_ASSIGN:
   case Cxx::BITWISE_OR_ASSIGN:
      CheckBitwiseOp(arg1, arg2);
      if(IsOverloaded(arg1, arg2)) return;
      Record(op_, arg1, &arg2);
      PushResult(arg1, arg2);
      return;

   case Cxx::THROW:
      //
      //  There can be an expression, but it is optional.
      //
      if(!args_.empty())
      {
         Context::PopArg(arg1);
         arg1.item->Instantiate(true);
      }
      return;

   case Cxx::STATEMENT_SEPARATOR:
      //
      //  Push the result of the second statement.
      //
      Record(op_, arg1, &arg2);
      Context::PushArg(arg2.EraseName());
      return;

   default:
      Debug::SwLog(Operation_Execute, "unexpected operator", op_);
   }
}

//------------------------------------------------------------------------------

fn_name Operation_ExecuteCall = "Operation.ExecuteCall";

void Operation::ExecuteCall() const
{
   Debug::ft(Operation_ExecuteCall);

   //  Pop the arguments.  The last one is on top of the stack, so restore
   //  the original order by inserting from the front of the list.
   //
   StackArgVector args;

   for(auto arg = Context::TopArg(); arg != nullptr; arg = Context::TopArg())
   {
      if(arg->InvokeSet()) break;
      Context::PopArg(false);
      if(arg->item == nullptr) return;
      args.insert(args.begin(), *arg);
   }

   //  Pop the function.
   //
   auto proc = Context::PopArg(false);
   if(proc.item == nullptr) return;

   //  Use ARGS to find the right function, because the initial lookup only
   //  returned the first match.  However, there are a couple of exceptions:
   //  o If the function is a constructor, name resolution actually returned
   //    the class, because it and the constructor have the same names.
   //    QualName.Referent does not instantiate a template that is only named
   //    as a class, so instantiate it here in case it doesn't yet exist.
   //  o If the function is an operator (except new and new[]), the correct
   //    function has already been identified.
   //  o If the function is a terminal, typedef, or enum this is an explicit
   //    type conversion, such as double(<arg>).
   //
   Function* func = nullptr;
   Class* cls = nullptr;
   auto scope = Context::Scope();

   switch(proc.item->Type())
   {
   case Cxx::Function:
      //
      //  Before matching arguments, insert an implicit "this" argument if
      //  it may be required.  After the matching function has been found,
      //  UpdateThisArg (below) erases the argument if it is not needed.
      //
      func = static_cast< Function* >(proc.item);
      func->PushThisArg(args);

      switch(func->Operator())
      {
      case Cxx::NIL_OPERATOR:
      case Cxx::OBJECT_CREATE:
      case Cxx::OBJECT_CREATE_ARRAY:
         func = func->GetArea()->FindFunc
            (func->Name(), &args, true, scope, nullptr);
      }
      break;

   case Cxx::Class:
      cls = static_cast< Class* >(proc.item);
      cls->Instantiate(false);
      func = cls->FindCtor(&args, scope);
      if((proc.name != nullptr) && (func != nullptr))
      {
         proc.name->SetReferent(func, nullptr);
      }
      break;

   case Cxx::Terminal:
   case Cxx::Typedef:
   case Cxx::Enum:
      //
      //  To perform an explicit conversion, the type must be convertible
      //  to a numeric and there must be one argument.  If so, register a
      //  read to that argument and push the target type.  If an implicit
      //  conversion would have been safe, don't log it.
      //
      auto dstNum = proc.NumericType();
      if((args.size() != 1) || (dstNum.Type() == Numeric::NIL))
      {
         auto expl = "Invalid type conversion: " + proc.Trace();
         Context::SwLog(Operation_ExecuteCall, expl, proc.item->Type());
         return;
      }

      auto srcNum = args.front().NumericType();

      switch(dstNum.CalcMatchWith(&srcNum))
      {
      case Incompatible:
      case Abridgeable:
         Context::Log(FunctionalCast);
      }

      args.front().WasRead();
      Context::PushArg(StackArg(proc.item->Referent(), 0, false));
      return;
   }

   if(func != nullptr)
   {
      //  Invoke the function, which pushes its return value onto the stack.
      //
      func->UpdateThisArg(args);
      auto warning = func->Invoke(&args);
      if(warning != Warning_N) Log(warning);
      return;
   }

   //  The function wasn't found.  This can occur when a default constructor
   //  is invoked, in which case we need to push its result onto the stack.
   //
   auto size = args.size();
   if(proc.IsDefaultCtor(args))
   {
      auto role = (size == 1 ? PureCtor : CopyCtor);
      cls = proc.item->GetClass();
      cls->WasCalled(role, nullptr);
      if(size > 1) args[1].WasRead();
      Context::PushArg(StackArg(cls, 0, true));
      return;
   }

   auto expl = "Failed to find function " + proc.Trace() + '(';
   for(size_t i = 0; i < size; ++i)
   {
      expl += args[i].Trace();
      if(i < size - 1) expl += ',';
   }
   expl += ')';
   Context::SwLog(Operation_ExecuteCall, expl, proc.item->Type());
}

//------------------------------------------------------------------------------

void Operation::ExecuteDelete(const StackArg& arg) const
{
   Debug::ft("Operation.ExecuteDelete");

   //  Look for operator delete for ARG.  Register a call to it if it is
   //  found.  If ARG was a pointer to a class, also register a call to
   //  its destructor and record it as a direct usage.
   //
   arg.WasRead();
   arg.SetAsDirect();

   auto pod = false;
   auto opDel = FindNewOrDelete(arg, true, pod);

   if(opDel != nullptr)
   {
      StackArgVector args;
      args.push_back(arg);
      opDel->Invoke(&args);
      overload_ = opDel;
   }

   if(pod) return;
   arg.item->RecordUsage();

   auto cls = static_cast< Class* >(arg.item->Root());
   cls->WasCalled(PureDtor, nullptr);
}

//------------------------------------------------------------------------------

void Operation::ExecuteNew() const
{
   Debug::ft("Operation.ExecuteNew");

   //  If this is operator new[], compile its array argument(s), which
   //  start at the third argument.  Pop each result.
   //
   if(op_ == Cxx::OBJECT_CREATE_ARRAY)
   {
      for(size_t i = 2; i < args_.size(); ++i)
      {
         args_[i]->EnterBlock();
         Context::PopArg(true);
      }
   }

   //  The second argument is the type for which to allocate memory.
   //  Look for its operator new.
   //
   StackArg spec(args_[1].get(), 0, false);
   auto pod = false;
   auto opNew = FindNewOrDelete(spec, false, pod);

   if(opNew != nullptr)
   {
      //  Push operator new onto the stack, followed by its arguments.  The
      //  first one's type is size_t.  It does not appear in source code but
      //  is always the first argument to operator new.  In a true compiler,
      //  it is the size of the type to be created and, for operator new[],
      //  would be multiplied by the size of each array (determined above).
      //
      auto newArg = Singleton< CxxRoot >::Instance()->IntTerm();
      auto newCall = static_cast< Operation* >(args_.front().get());
      Context::PushArg(StackArg(opNew, nullptr));
      Context::PushArg(StackArg(newArg, 0, false));
      newCall->PushArgs();

      //  Compile the call to the operator new function and pop the result,
      //  which should be a void*.  Push the TypeSpec that new created, but
      //  add a pointer to it.
      //
      ExecuteCall();
      Context::PopArg(false);
      spec.SetNewPtrs();
      Context::PushArg(spec);
      overload_ = opNew;
   }

   //  If a class is being created, push its constructor onto the stack.
   //  If one isn't found, it must be the default constructor.  If there
   //  is more than one, ExecuteCall (below) will find the correct one.
   //
   if(pod) return;
   auto cls = static_cast< Class* >(spec.item->Root());
   auto ctor = cls->FindCtor(nullptr, Context::Scope());
   if(ctor == nullptr)
   {
      cls->WasCalled(PureCtor, nullptr);
      return;
   }

   //  Before pushing the constructor, discard the result of operator new.
   //  Then push the constructor's arguments, starting with "this".  Only
   //  operator new, not new[], can have additional arguments, which appear
   //  in the optional third argument.  Compile the call to the constructor
   //  and add a pointer to the result.
   //
   Context::PopArg(false);
   Context::PushArg(StackArg(ctor, nullptr));
   Context::PushArg(StackArg(cls, 1, false));
   Context::TopArg()->SetAsThis(true);

   if((op_ == Cxx::OBJECT_CREATE) && (args_.size() >= 3))
   {
      auto ctorCall = static_cast< Operation* >(args_[2].get());
      ctorCall->PushArgs();
   }

   ExecuteCall();
   auto result = Context::TopArg();
   if(result != nullptr) result->IncrPtrs();
}

//------------------------------------------------------------------------------

fn_name Operation_ExecuteOverload = "Operation.ExecuteOverload";

bool Operation::ExecuteOverload
   (const string& name, StackArg& arg1, const StackArg* arg2) const
{
   Debug::ft(Operation_ExecuteOverload);

   //  If ARG1 is a class, make sure that it is instantiated.
   //
   Class* cls = nullptr;
   auto root = arg1.item->Root();
   if(root->Type() == Cxx::Class)
   {
      cls = static_cast< Class* >(root);
      cls->Instantiate(false);
   }

   //  Search for an overload in ARG1 and its base classes.  The arguments
   //  are ARG1, ARG2 (if present), and for postfix increment/decrement, a
   //  dummy int that distinguishes them from their prefix versions.
   //
   StackArgVector args;
   bool autoAssign = false;

   if(arg1.item->IsAuto())
   {
      //  ARG1 is of type "auto".  If this is an assignment operation,
      //  FindFunc will not match on type "auto".  Push ARG2 instead,
      //  because it has the type that will be assigned to auto ARG1.
      //
      if((op_ != Cxx::ASSIGN) || (arg2 == nullptr))
      {
         auto expl = "Invalid auto assignment for " + arg1.Trace();
         Context::SwLog(Operation_ExecuteOverload, expl, (arg2 == nullptr));
         return false;
      }

      args.insert(args.cbegin(), *arg2);
      autoAssign = true;
   }
   else
   {
      args.insert(args.cbegin(), arg1);
   }

   if(arg2 != nullptr) args.push_back(*arg2);

   switch(op_)
   {
   case Cxx::POSTFIX_INCREMENT:
   case Cxx::POSTFIX_DECREMENT:
      {
         auto dummyArg = Singleton< CxxRoot >::Instance()->IntTerm();
         args.push_back(StackArg(dummyArg, 0, false));
      }
      break;
   case Cxx::ASSIGN:
      //
      //c If ARG2 is of type "auto", it is a hack for brace initialization.
      //  This is only legal when ARG1 is an aggregate, and operator= will
      //  not be used.
      //
      if(arg2->item->Name() == AUTO_STR) return false;
      break;
   }

   enum OperatorVenue  // where to search for the overload
   {
      Arg1Class,  // in ARG1's class
      Arg1Scope,  // in ARG1's namespace
      Arg2Scope,  // in ARG2's namespace
      CurrScope,  // in the context namespace
      Exhausted   // finished
   };

   auto scope = Context::Scope();
   Function* oper = nullptr;
   auto match = Incompatible;
   CxxArea* area = nullptr;
   auto mem = false;
   auto hasThis = false;

   for(auto venue = 0; venue != Exhausted; ++venue)
   {
      switch(venue)
      {
      case Arg1Class:
         area = cls;
         args.front().IncrPtrs();
         args.front().SetAsThis(true);
         hasThis = true;
         break;

      case Arg1Scope:
         //
         //  Before searching for the operator at file scope, check
         //  for operators that do not allow non-member versions.
         //
         switch(op_)
         {
         case Cxx::POINTER_SELECT:
         case Cxx::ARRAY_SUBSCRIPT:
         case Cxx::ASSIGN:
            area = nullptr;
            venue = CurrScope;  // will break out of for loop
            break;
         default:
            area = root->GetSpace();
            args.front().DecrPtrs();
            args.front().SetAsThis(false);
            hasThis = false;
         }
         break;

      case Arg2Scope:
         area = (arg2 != nullptr ? arg2->item->Root()->GetSpace() : nullptr);
         break;

      case CurrScope:
         area = scope->GetSpace();
         break;
      }

      if(area != nullptr)
      {
         SymbolView view;
         auto candidate = area->FindFunc(name, &args, true, scope, &view);

         if((candidate != nullptr) && (view.match > match))
         {
            oper = candidate;
            match = view.match;
            mem = (venue == Arg1Class);
         }
      }
   }

   //  If an overload was found, invoke it after fixing its "this" argument
   //  if it a member function.  If assigning an auto type, pop the function's
   //  return type and set it as the auto type for FuncData.EnterBlock.  When
   //  setting an auto type, update ARG1 to ARG2, the argument on which the
   //  function was invoked.
   //
   if(oper == nullptr) return false;

   if(mem && !hasThis)
   {
      args.front().IncrPtrs();
      args.front().SetAsThis(true);
   }

   arg1.SetAsDirect();
   Context::PushArg(StackArg(oper, nullptr));

   for(size_t i = 0; i < args.size(); ++i)
   {
      Context::PushArg(args[i].EraseName());
   }

   ExecuteCall();
   overload_ = oper;

   if(autoAssign)
   {
      Context::PopArg(false).SetAsAutoType();
      arg1.SetAutoType();
      Record(op_, arg1, arg2);
      arg1 = *arg2;
   }
   else
   {
      Record(op_, arg1, arg2);
   }

   //  If OPER was an assignment operator in a base class, the default
   //  assignment operator in CLS would have been invoked.  The stack
   //  currently contains the base class but should actually have CLS.
   //
   if((op_ == Cxx::ASSIGN) && (oper->GetClass() != cls))
   {
      cls->WasCalled(CopyOper, nullptr);
      Context::PopArg(false);
      Context::PushArg(StackArg(cls, 0, false));
   }

   return true;
}

//------------------------------------------------------------------------------

fn_name Operation_FindNewOrDelete = "Operation.FindNewOrDelete";

Function* Operation::FindNewOrDelete
   (const StackArg& arg, bool del, bool& pod) const
{
   Debug::ft(Operation_FindNewOrDelete);

   //  If ARG is a class, search in its class hierarchy, provided that
   //  o for operators new/new[], ARG is not a pointer;
   //  o for operators delete/delete[], ARG is a direct pointer.
   //  When new or delete will be invoked on a class, make sure that it
   //  has been instantiated in case it is a template instance.  In all
   //  other cases, search in ARG's namespace hierarchy.
   //
   CxxArea* area = nullptr;
   auto targ = arg.item->Root();
   size_t ptrs = (del ? 1 : 0);

   if((targ->Type() == Cxx::Class) && (arg.Ptrs(true) == ptrs))
   {
      area = static_cast< Class* >(targ);
      pod = false;
      static_cast< Class* >(targ)->Instantiate(true);
   }
   else
   {
      area = targ->GetSpace();
      pod = true;
   }

   if(area == nullptr)
   {
      auto expl = "Failed to find area for " + targ->Trace();
      Context::SwLog(Operation_FindNewOrDelete, expl, 0);
      return nullptr;
   }

   Function* oper = nullptr;
   auto scope = Context::Scope();
   auto sName = (del ?
      CxxOp::OperatorToName(Cxx::OBJECT_DELETE) :
      CxxOp::OperatorToName(Cxx::OBJECT_CREATE));
   auto array =
      (op_ == Cxx::OBJECT_CREATE_ARRAY) || (op_ == Cxx::OBJECT_DELETE_ARRAY);

   for(auto i = 0; i <= 1; ++i)
   {
      if(array)
      {
         auto vName = (del ?
            CxxOp::OperatorToName(Cxx::OBJECT_DELETE_ARRAY) :
            CxxOp::OperatorToName(Cxx::OBJECT_CREATE_ARRAY));
         oper = area->FindFunc(vName, nullptr, true, scope, nullptr);
      }

      if(oper == nullptr)
      {
         oper = area->FindFunc(sName, nullptr, true, scope, nullptr);
      }

      //  If the operator was not found in a class hierarchy, look in
      //  the namespace hierarchy.
      //
      if((oper == nullptr) && !pod)
         area = area->GetSpace();
      else
         break;
   }

   if(oper == nullptr)
   {
      auto expl = "Failed to find operator new/delete for " + targ->Trace();
      Context::SwLog(Operation_FindNewOrDelete, expl, op_);
   }

   return oper;
}

//------------------------------------------------------------------------------

void Operation::GetUsages(const CodeFile& file, CxxUsageSets& symbols)
{
   for(auto a = args_.cbegin(); a != args_.cend(); ++a)
   {
      (*a)->GetUsages(file, symbols);
   }

   if(overload_ != nullptr)
   {
      switch(op_)
      {
      case Cxx::OBJECT_CREATE:
      case Cxx::OBJECT_CREATE_ARRAY:
      case Cxx::OBJECT_DELETE:
      case Cxx::OBJECT_DELETE_ARRAY:
         //
         //  These are omitted because the appropriate version will be found
         //  automatically.  The default versions are in the global namespace,
         //  and adding them as usages causes >trim to generate unnecessary
         //  recommendations to #include <new>.
         //
         break;

      default:
         if(!overload_->IsInternal()) symbols.AddDirect(overload_);
      }
   }
}

//------------------------------------------------------------------------------

bool Operation::IsOverloaded(StackArg& arg) const
{
   Debug::ft("Operation.IsOverloaded(unary)");

   if(!CxxOp::Attrs[op_].overloadable) return false;
   if(!arg.CanBeOverloaded()) return false;

   auto name = CxxOp::OperatorToName(op_);
   return ExecuteOverload(name, arg, nullptr);
}

//------------------------------------------------------------------------------

bool Operation::IsOverloaded(StackArg& arg1, StackArg& arg2) const
{
   Debug::ft("Operation.IsOverloaded(binary)");

   //  If this operator can be overloaded, see if an overload exists.
   //  If its arguments can be flipped, also look for that overload.
   //  (Although the operators >, >=, <, and <= are symmetric, they
   //  would have to be inverted if generating true object code.)
   //
   if(!CxxOp::Attrs[op_].overloadable) return false;
   if(!arg1.CanBeOverloaded() && !arg2.CanBeOverloaded()) return false;

   auto name = CxxOp::OperatorToName(op_);
   if(ExecuteOverload(name, arg1, &arg2)) return true;
   if(CxxOp::Attrs[op_].symmetric) return ExecuteOverload(name, arg2, &arg1);
   return false;
}

//------------------------------------------------------------------------------

fn_name Operation_MakeBinary = "Operation.MakeBinary";

bool Operation::MakeBinary()
{
   Debug::ft(Operation_MakeBinary);

   switch(op_)
   {
   case Cxx::POSTFIX_INCREMENT:
   case Cxx::POSTFIX_DECREMENT:
      return true;
   case Cxx::UNARY_MINUS:
      op_ = Cxx::SUBTRACT;
      return true;
   case Cxx::UNARY_PLUS:
      op_ = Cxx::ADD;
      return true;
   case Cxx::ADDRESS_OF:
      op_ = Cxx::BITWISE_AND;
      return true;
   case Cxx::INDIRECTION:
      op_ = Cxx::MULTIPLY;
      return true;
   }

   Debug::SwLog(Operation_MakeBinary, "unexpected operator", op_);
   return false;
}

//------------------------------------------------------------------------------

fn_name Operation_Print = "Operation.Print";

void Operation::Print(ostream& stream, const Flags& options) const
{
   auto& attrs = CxxOp::Attrs[op_];
   bool space;

   switch(op_)
   {
   case Cxx::FUNCTION_CALL:
      stream << '(';
      for(auto a = args_.cbegin(); a != args_.cend(); ++a)
      {
         (*a)->Print(stream, options);
         if(*a != args_.back()) stream << ", ";
      }
      stream << ')';
      break;

   case Cxx::CAST:
      stream << '(';
      DisplayArg(stream, 0);
      stream << ") ";
      DisplayArg(stream, 1);
      break;

   case Cxx::ARRAY_SUBSCRIPT:
      DisplayArg(stream, 0);
      stream << '[';
      DisplayArg(stream, 1);
      stream << ']';
      break;

   case Cxx::OBJECT_CREATE:
   case Cxx::OBJECT_CREATE_ARRAY:
      DisplayNew(stream);
      break;

   case Cxx::OBJECT_DELETE:
   case Cxx::OBJECT_DELETE_ARRAY:
      stream << attrs.symbol << SPACE;
      DisplayArg(stream, 0);
      break;

   case Cxx::POSTFIX_INCREMENT:
   case Cxx::POSTFIX_DECREMENT:
      DisplayArg(stream, 0);
      stream << attrs.symbol;
      break;

   case Cxx::CONST_CAST:
   case Cxx::DYNAMIC_CAST:
   case Cxx::REINTERPRET_CAST:
   case Cxx::STATIC_CAST:
      stream << attrs.symbol << '<';
      DisplayArg(stream, 0);
      stream << ">(";
      DisplayArg(stream, 1);
      stream << ')';
      break;

   case Cxx::TYPE_NAME:
   case Cxx::SIZEOF_TYPE:
   case Cxx::ALIGNOF_TYPE:
   case Cxx::NOEXCEPT:
      stream << attrs.symbol;
      stream << '(';
      DisplayArg(stream, 0);
      stream << ')';
      break;

   case Cxx::THROW:
      stream << attrs.symbol;
      if(args_.empty()) break;
      stream << SPACE;
      DisplayArg(stream, 0);
      break;

   case Cxx::CONDITIONAL:
      DisplayArg(stream, 0);
      stream << " ? ";
      DisplayArg(stream, 1);
      stream << " : ";
      DisplayArg(stream, 2);
      break;

   default:
      switch(attrs.arguments)
      {
      case 1:
         stream << attrs.symbol;
         DisplayArg(stream, 0);
         break;

      case 2:
         space = (attrs.priority <= 14);
         DisplayArg(stream, 0);
         if(space && (op_ != Cxx::STATEMENT_SEPARATOR)) stream << SPACE;
         stream << attrs.symbol;
         if(space) stream << SPACE;
         DisplayArg(stream, 1);
         break;

      default:
         Debug::SwLog(Operation_Print, "unexpected operator", op_);
         stream << ERROR_STR << "(op=" << op_ << ')';
      }
   }
}

//------------------------------------------------------------------------------

fn_name Operation_Push = "Operation.Push";

void Operation::Push() const
{
   Debug::ft(Operation_Push);

   //  Pop operators from the stack and execute them until the stack is empty or
   //  the operator on top of the stack has a lower priority than this one.  At
   //  that point, push this operator.
   //
   for(auto top = Context::TopOp(); top != nullptr; top = Context::TopOp())
   {
      auto& topAttrs = CxxOp::Attrs[top->op_];
      auto& thisAttrs = CxxOp::Attrs[this->op_];
      if(topAttrs.priority < thisAttrs.priority) break;
      if(thisAttrs.rightToLeft && (topAttrs.priority == thisAttrs.priority))
         break;
      top->Execute();
      Context::PopOp();
   }

   Context::PushOp(this);

   //  When a function call operator is pushed, and the argument on top of the
   //  stack is the function to be invoked, mark it for invocation.  Note that
   //  in a functional cast, the type (a terminal, typedef, or enum) is treated
   //  like a function.
   //
   if(op_ == Cxx::FUNCTION_CALL)
   {
      auto top = Context::TopArg();

      if(top == nullptr)
      {
         auto expl = "No function name for function call operator";
         Context::SwLog(Operation_Push, expl, 0);
         return;
      }

      switch(top->item->Type())
      {
      case Cxx::Function:
      case Cxx::Terminal:
      case Cxx::Typedef:
      case Cxx::Enum:
         top->SetInvoke();
      }
   }
}

//------------------------------------------------------------------------------

void Operation::PushArgs() const
{
   Debug::ft("Operation.PushArgs");

   for(auto a = args_.cbegin(); a != args_.cend(); ++a)
   {
      (*a)->EnterBlock();
   }
}

//------------------------------------------------------------------------------

fn_name Operation_PushMember = "Operation.PushMember";

void Operation::PushMember(StackArg& arg1, const StackArg& arg2) const
{
   Debug::ft(Operation_PushMember);

   //  Check that
   //  o ARG1 is a class (and that it is instantiated)
   //  o ARG1 is a reference (for ".") or a direct pointer (for "->")
   //  o ARG2 has a name
   //  o ARG2 is a member of ARG1
   //
   auto root = arg1.item->Root();
   auto type = root->Type();

   if(type != Cxx::Class)
   {
      auto expl = arg1.Trace() + " is not a class";
      Context::SwLog(Operation_PushMember, expl, type);
      return;
   }

   auto cls = static_cast< Class* >(root);
   cls->Instantiate(false);

   auto ptrs = arg1.Ptrs(true);
   auto err = (op_ == Cxx::REFERENCE_SELECT ? (ptrs != 0) : (ptrs != 1));

   if(err)
   {
      auto expl = "Invalid indirection count to member of " + arg1.Trace();
      Context::SwLog(Operation_PushMember, expl, (op_ << 4) + ptrs);
   }

   auto& name = arg2.item->Name();

   if(name.empty())
   {
      auto expl = "Name not found for " + arg2.Trace();
      Context::SwLog(Operation_PushMember, expl, 0);
      return;
   }

   SymbolView view;
   auto scope = Context::Scope();
   auto mem = cls->FindMember(name, true, scope, &view);

   if(mem == nullptr)
   {
      auto expl = "Member " + cls->Name() + SCOPE_STR + name + " not found";
      Context::SwLog(Operation_PushMember, expl, 0);
      return;
   }

   if(arg2.name != nullptr)
   {
      //c If MEM is a function, the following should be deferred until function
      //  matching is concluded.
      //  Record that MEM was accessed through CLS (cls.mem or cls->mem).  If
      //  MEM was Inherited, it must actually be public (rather than protected)
      //  if SCOPE was not a friend of its declarer and neither in CLS nor one
      //  of its subclasses.
      //
      arg2.name->MemberAccessed(cls, mem);

      if((view.accessibility == Inherited) && (!view.friend_) &&
         (cls->ClassDistance(scope->GetClass()) == NOT_A_SUBCLASS))
      {
         mem->RecordAccess(Cxx::Public);
      }
   }
   else
   {
      auto expl = "Unexpected access to " + cls->Name() + SCOPE_STR + name;
      Context::SwLog(Operation_PushMember, expl, arg2.item->Type());
   }

   //  If ARG2 specified template arguments, use them to find (or instantiate)
   //  the correct function template instance.
   //
   auto tmplt = arg2.item->GetTemplateArgs();

   if(tmplt != nullptr)
   {
      if(mem->Type() == Cxx::Function)
      {
         mem = static_cast< Function* >(mem)->InstantiateFunction(tmplt);
      }
      else
      {
         auto expl = "Invalid type for " + cls->Name() + SCOPE_STR + name;
         Context::SwLog(Operation_PushMember, expl, mem->Type());
      }
   }

   //  Push MEM via ARG1 and op_ after recording that ARG1 was used directly.
   //
   arg1.SetAsDirect();
   Context::PushArg(mem->MemberToArg(arg1, arg2.name, op_));
}

//------------------------------------------------------------------------------

fn_name Operation_PushResult = "Operation.PushResult";

void Operation::PushResult(StackArg& lhs, StackArg& rhs) const
{
   Debug::ft(Operation_PushResult);

   auto lhsType = lhs.TypeString(true);
   auto rhsType = rhs.TypeString(true);
   auto match = lhs.CalcMatchWith(rhs, lhsType, rhsType);

   if((match == Incompatible) && CxxOp::Attrs[op_].symmetric)
   {
      match = rhs.CalcMatchWith(lhs, rhsType, lhsType);
   }

   if((match == Promotable) || (match == Abridgeable))
   {
      if(lhs.IsBool() || rhs.IsBool())
      {
         Log(BoolMixedWithNumeric);
      }
   }

   auto diff = false;

   if(match <= Convertible)  // allows detection of pointer arithmetic
   {
      auto err = (match == Incompatible);

      switch(lhs.NumericType().Type())
      {
      case Numeric::ENUM:
         //
         //  <int><op><enum> succeeds because an enum can be assigned to an int.
         //  <enum><op><int> only succeeds for a symmetric operator, so handle
         //  the non-symmetric operators here.  Many of these operations appear
         //  dubious and are therefore commented out.
         //
         switch(rhs.NumericType().Type())
         {
         case Numeric::INT:
            switch(op_)
            {
            case Cxx::SUBTRACT:
//          case Cxx::DIVIDE:
//          case Cxx::MODULO:
            case Cxx::LEFT_SHIFT:
            case Cxx::RIGHT_SHIFT:
//          case Cxx::MULTIPLY_ASSIGN:
//          case Cxx::DIVIDE_ASSIGN:
//          case Cxx::MODULO_ASSIGN:
            case Cxx::ADD_ASSIGN:
            case Cxx::SUBTRACT_ASSIGN:
//          case Cxx::LEFT_SHIFT_ASSIGN:
//          case Cxx::RIGHT_SHIFT_ASSIGN:
//          case Cxx::BITWISE_AND_ASSIGN:
//          case Cxx::BITWISE_XOR_ASSIGN:
//          case Cxx::BITWISE_OR_ASSIGN:
               err = false;
            }
            break;

         case Numeric::ENUM:
            Log(EnumTypesDiffer);
            err = false;
            break;
         }
         break;

      case Numeric::PTR:
         switch(op_)
         {
         case Cxx::SUBTRACT:
            //
            //  Allow ptr1 - ptr2.
            //
            if(rhs.NumericType().Type() == Numeric::PTR)
            {
               Log(PointerArithmetic);
               err = false;
               diff = true;
               break;
            }
            //  [[fallthrough]]
         case Cxx::ADD:
         case Cxx::ADD_ASSIGN:
         case Cxx::SUBTRACT_ASSIGN:
            //
            //  Allow ptr + int, ptr - int, ptr += int, and ptr -= int.
            //
            if(rhs.NumericType().Type() == Numeric::INT)
            {
               Log(PointerArithmetic);
               err = false;
            }
            break;
         }
      }

      if(err)
      {
         auto expl = lhsType + " is incompatible with " + rhsType;
         Context::SwLog(Operation_PushResult, expl, op_);
      }
   }

   switch(op_)
   {
   case Cxx::SUBTRACT:
      if(diff)
      {
         PushType(INT_STR);
         break;
      }
      //  [[fallthrough]]
   case Cxx::MULTIPLY:
   case Cxx::DIVIDE:
   case Cxx::MODULO:
   case Cxx::ADD:
   case Cxx::LEFT_SHIFT:
   case Cxx::RIGHT_SHIFT:
   case Cxx::BITWISE_AND:
   case Cxx::BITWISE_XOR:
   case Cxx::BITWISE_OR:
      //
      //  The result is a temporary.
      //
      if(lhs.item->Type() == Cxx::Terminal)
      {
         rhs.SetAsTemporary();
         Context::PushArg(rhs.EraseName());
      }
      else
      {
         lhs.SetAsTemporary();
         Context::PushArg(lhs.EraseName());
      }
      break;

   case Cxx::LESS:
   case Cxx::LESS_OR_EQUAL:
   case Cxx::GREATER:
   case Cxx::GREATER_OR_EQUAL:
   case Cxx::EQUALITY:
   case Cxx::INEQUALITY:
   case Cxx::LOGICAL_AND:
   case Cxx::LOGICAL_OR:
      PushType(BOOL_STR);
      break;

   case Cxx::ASSIGN:
      Context::PushArg(rhs.EraseName());
      break;

   case Cxx::MULTIPLY_ASSIGN:
   case Cxx::DIVIDE_ASSIGN:
   case Cxx::MODULO_ASSIGN:
   case Cxx::ADD_ASSIGN:
   case Cxx::SUBTRACT_ASSIGN:
   case Cxx::LEFT_SHIFT_ASSIGN:
   case Cxx::RIGHT_SHIFT_ASSIGN:
   case Cxx::BITWISE_AND_ASSIGN:
   case Cxx::BITWISE_XOR_ASSIGN:
   case Cxx::BITWISE_OR_ASSIGN:
      Context::PushArg(lhs.EraseName());
      break;

   default:
      auto expl = "Unknown operator";
      Context::SwLog(Operation_PushResult, expl, op_);
   }
}

//------------------------------------------------------------------------------

fn_name Operation_PushType = "Operation.PushType";

void Operation::PushType(const string& name)
{
   Debug::ft(Operation_PushType);

   //  Look up NAME and push what it refers to.
   //
   auto syms = Singleton< CxxSymbols >::Instance();
   auto file = Context::File();
   auto scope = Context::Scope();
   SymbolView view;
   auto item = syms->FindSymbol(file, scope, name, TYPE_REFS, view);

   if(item != nullptr)
   {
      Context::PushArg(StackArg(item, 0, false));
      return;
   }

   auto expl = "Failed to find type for " + name;
   Context::SwLog(Operation_PushType, expl, 0);
}

//------------------------------------------------------------------------------

fn_name Operation_Record = "Operation.Record";

void Operation::Record(Cxx::Operator op, StackArg& arg1, const StackArg* arg2)
{
   Debug::ft(Operation_Record);

   switch(op)
   {
   case Cxx::REFERENCE_SELECT:
   case Cxx::POINTER_SELECT:
   case Cxx::TYPE_NAME:
   case Cxx::SIZEOF_TYPE:
   case Cxx::ALIGNOF_TYPE:
   case Cxx::NOEXCEPT:
   case Cxx::ONES_COMPLEMENT:
   case Cxx::UNARY_PLUS:
   case Cxx::UNARY_MINUS:
   case Cxx::LOGICAL_NOT:
   case Cxx::ADDRESS_OF:
   case Cxx::INDIRECTION:
      arg1.WasRead();
      return;

   case Cxx::ARRAY_SUBSCRIPT:
      arg1.WasRead();
      arg2->WasRead();
      arg1.WasIndexed();
      return;

   case Cxx::FUNCTION_CALL:
   case Cxx::OBJECT_CREATE:
   case Cxx::OBJECT_CREATE_ARRAY:
   case Cxx::OBJECT_DELETE:
   case Cxx::OBJECT_DELETE_ARRAY:
   case Cxx::THROW:
      return;

   case Cxx::POSTFIX_INCREMENT:
   case Cxx::POSTFIX_DECREMENT:
   case Cxx::PREFIX_INCREMENT:
   case Cxx::PREFIX_DECREMENT:
      arg1.WasRead();
      arg1.WasWritten();
      return;

   case Cxx::CONST_CAST:
   case Cxx::DYNAMIC_CAST:
   case Cxx::REINTERPRET_CAST:
   case Cxx::STATIC_CAST:
   case Cxx::CAST:
   case Cxx::STATEMENT_SEPARATOR:
      arg2->WasRead();
      return;

   case Cxx::REFERENCE_SELECT_MEMBER:
   case Cxx::POINTER_SELECT_MEMBER:
   case Cxx::MULTIPLY:
   case Cxx::DIVIDE:
   case Cxx::MODULO:
   case Cxx::ADD:
   case Cxx::SUBTRACT:
   case Cxx::LEFT_SHIFT:
   case Cxx::RIGHT_SHIFT:
   case Cxx::LESS:
   case Cxx::LESS_OR_EQUAL:
   case Cxx::GREATER:
   case Cxx::GREATER_OR_EQUAL:
   case Cxx::EQUALITY:
   case Cxx::INEQUALITY:
   case Cxx::BITWISE_AND:
   case Cxx::BITWISE_XOR:
   case Cxx::BITWISE_OR:
   case Cxx::LOGICAL_AND:
   case Cxx::LOGICAL_OR:
   case Cxx::CONDITIONAL:
      arg1.WasRead();
      arg2->WasRead();
      return;

   case Cxx::ASSIGN:
      arg1.WasWritten();
      arg2->WasRead();
      return;

   case Cxx::MULTIPLY_ASSIGN:
   case Cxx::DIVIDE_ASSIGN:
   case Cxx::MODULO_ASSIGN:
   case Cxx::ADD_ASSIGN:
   case Cxx::SUBTRACT_ASSIGN:
   case Cxx::LEFT_SHIFT_ASSIGN:
   case Cxx::RIGHT_SHIFT_ASSIGN:
   case Cxx::BITWISE_AND_ASSIGN:
   case Cxx::BITWISE_XOR_ASSIGN:
   case Cxx::BITWISE_OR_ASSIGN:
      arg1.WasRead();
      arg1.WasWritten();
      arg2->WasRead();
      return;

   default:
      Debug::SwLog(Operation_Record, "unexpected operator", op);
   }
}

//------------------------------------------------------------------------------

void Operation::Shrink()
{
   CxxToken::Shrink();
   ShrinkTokens(args_);
   auto size = args_.capacity() * sizeof(TokenPtr);
   CxxStats::Vectors(CxxStats::OPERATION, size);
}

//------------------------------------------------------------------------------

string Operation::Trace() const
{
   switch(op_)
   {
   case Cxx::ARRAY_SUBSCRIPT:
      return "[]";
   case Cxx::FUNCTION_CALL:
      return "() (function call)";
   case Cxx::POSTFIX_INCREMENT:
      return "++ (postfix)";
   case Cxx::POSTFIX_DECREMENT:
      return "-- (postfix)";
   case Cxx::PREFIX_INCREMENT:
      return "++ (prefix)";
   case Cxx::PREFIX_DECREMENT:
      return "-- (prefix)";
   case Cxx::UNARY_PLUS:
      return "+ (unary)";
   case Cxx::UNARY_MINUS:
      return "- (unary)";
   case Cxx::ADDRESS_OF:
      return "& (address of)";
   case Cxx::INDIRECTION:
      return "* (indirection)";
   case Cxx::CAST:
      return "() (cast)";
   case Cxx::MULTIPLY:
      return "* (multiply)";
   case Cxx::ADD:
      return "+ (add)";
   case Cxx::SUBTRACT:
      return "- (subtract)";
   case Cxx::BITWISE_AND:
      return "& (bitwise and)";
   }

   return CxxOp::Attrs[op_].symbol;
}

//------------------------------------------------------------------------------

void Operation::UpdatePos
   (EditorAction action, size_t begin, size_t count, size_t from) const
{
   CxxToken::UpdatePos(action, begin, count, from);

   for(auto a = args_.cbegin(); a != args_.cend(); ++a)
   {
      (*a)->UpdatePos(action, begin, count, from);
   }
}

//==============================================================================

void Precedence::AddToXref()
{
   if(expr_ != nullptr) expr_->AddToXref();
}

//------------------------------------------------------------------------------

void Precedence::Check() const
{
   if(expr_ != nullptr) expr_->Check();
}

//------------------------------------------------------------------------------

void Precedence::EnterBlock()
{
   Debug::ft("Precedence.EnterBlock");

   if(expr_ != nullptr) expr_->EnterBlock();
}

//------------------------------------------------------------------------------

void Precedence::GetUsages(const CodeFile& file, CxxUsageSets& symbols)
{
   if(expr_ != nullptr) expr_->GetUsages(file, symbols);
}

//------------------------------------------------------------------------------

void Precedence::Print(ostream& stream, const Flags& options) const
{
   stream << '(';
   if(expr_ != nullptr) expr_->Print(stream, options);
   stream << ')';
}

//------------------------------------------------------------------------------

void Precedence::Shrink()
{
   CxxToken::Shrink();
   if(expr_ != nullptr) expr_->Shrink();
}

//------------------------------------------------------------------------------

void Precedence::UpdatePos
   (EditorAction action, size_t begin, size_t count, size_t from) const
{
   CxxToken::UpdatePos(action, begin, count, from);
   if(expr_ != nullptr) expr_->UpdatePos(action, begin, count, from);
}

//==============================================================================

fn_name StringLiteral_PushBack = "StringLiteral.PushBack";

void StringLiteral::PushBack(uint32_t c)
{
   Debug::ft(StringLiteral_PushBack);

   Debug::SwLog(StringLiteral_PushBack, strOver(this), 0);
}
}
