//==============================================================================
//
//  CxxDirective.cpp
//
//  Copyright (C) 2013-2025  Greg Utas
//
//  This file is part of the Robust Services Core (RSC).
//
//  RSC is free software: you can redistribute it and/or modify it under the
//  terms of the Lesser GNU General Public License as published by the Free
//  Software Foundation, either version 3 of the License, or (at your option)
//  any later version.
//
//  RSC is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
//  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
//  details.
//
//  You should have received a copy of the Lesser GNU General Public License
//  along with RSC.  If not, see <http://www.gnu.org/licenses/>.
//
#include "CxxDirective.h"
#include <bitset>
#include <iosfwd>
#include <sstream>
#include "CodeFile.h"
#include "CxxArea.h"
#include "CxxExecute.h"
#include "CxxRoot.h"
#include "CxxSymbols.h"
#include "Debug.h"
#include "Formatters.h"
#include "Lexer.h"
#include "Library.h"
#include "Singleton.h"
#include "SysTypes.h"

using namespace NodeBase;
using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace CodeTools
{
static void AlignLeft(ostream& stream, const string& prefix)
{
   //  If PREFIX is more than one indentation, indent one level less.
   //
   if(prefix.size() < IndentSize())
      stream << prefix;
   else
      stream << prefix.substr(IndentSize());
}

//------------------------------------------------------------------------------

bool IncludesAreSorted(const IncludePtr& incl1, const IncludePtr& incl2)
{
   //  Check that the #includes appear in the same file.
   //
   if(incl1->GetFile() != incl2->GetFile()) return false;

   //  #includes are sorted by group, then alphabetically within each group.
   //  If the #includes are identical, sort them by pointer.
   //
   auto group1 = incl1->Group();
   auto group2 = incl2->Group();
   if(group1 < group2) return true;
   if(group1 > group2) return false;

   auto comp = strCompare(incl1->Name(), incl2->Name());
   if(comp < 0) return true;
   if(comp > 0) return false;

   return (incl1 < incl2);
}

//==============================================================================

Conditional::Conditional()
{
   Debug::ft("Conditional.ctor");
}

//------------------------------------------------------------------------------

void Conditional::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   condition_->Print(stream, options);
   stream << CRLF;
   OptionalCode::Display(stream, prefix, options);
}

//------------------------------------------------------------------------------

bool Conditional::EnterScope()
{
   Debug::ft("Conditional.EnterScope");

   //c The expression that follows an #if or #elif is not currently evaluated.
   //  This function returns false, so the code that follows the directive will
   //  be ignored.  To support #if and #elif, the expression would have to be
   //  evaluated so that this function could return true or false as required.

   condition_->EnterBlock();
   auto result = Context::PopArg(true);
   result.CheckIfBool();
   return false;
}

//------------------------------------------------------------------------------

void Conditional::GetUsages(const CodeFile& file, CxxUsageSets& symbols)
{
   condition_->GetUsages(file, symbols);
}

//------------------------------------------------------------------------------

CxxToken* Conditional::PosToItem(size_t pos) const
{
   auto item = OptionalCode::PosToItem(pos);
   if(item != nullptr) return item;

   return condition_->PosToItem(pos);
}

//------------------------------------------------------------------------------

void Conditional::UpdatePos
   (EditorAction action, size_t begin, size_t count, size_t from) const
{
   OptionalCode::UpdatePos(action, begin, count, from);
   condition_->UpdatePos(action, begin, count, from);
}

//------------------------------------------------------------------------------

void Conditional::UpdateXref(bool insert)
{
   condition_->UpdateXref(insert);
}

//==============================================================================

CxxDirective::CxxDirective()
{
   Debug::ft("CxxDirective.ctor");
}

//------------------------------------------------------------------------------

bool CxxDirective::GetSpan(size_t& begin, size_t& left, size_t& end) const
{
   Debug::ft("CxxDirective.GetSpan");

   begin = GetPos();
   end = GetFile()->GetLexer().CurrEnd(begin);
   return true;
}

//==============================================================================

Define::Define(const string& name) : Macro(name),
   rhs_(nullptr),
   value_(nullptr),
   defined_(false)
{
   Debug::ft("Define.ctor");
}

//------------------------------------------------------------------------------

Define::Define(const string& name, ExprPtr& rhs) : Macro(name),
   rhs_(std::move(rhs)),
   value_(nullptr),
   defined_(true)
{
   Debug::ft("Define.ctor(rhs)");
}

//------------------------------------------------------------------------------

Define::~Define()
{
   Debug::ftnt("Define.dtor");
}

//------------------------------------------------------------------------------

CxxToken* Define::AutoType() const
{
   return value_;
}

//------------------------------------------------------------------------------

void Define::Check() const
{
   Debug::ftnt("Define.Check");

   if(rhs_ != nullptr) Log(PreprocessorDirective);
}

//------------------------------------------------------------------------------

void Define::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   AlignLeft(stream, prefix);
   stream << HASH_DEFINE_STR << SPACE << Name();

   if(rhs_ != nullptr)
   {
      stream << SPACE;
      rhs_->Print(stream, options);
   }

   if(!options.test(DispCode))
   {
      std::ostringstream buff;
      buff << " // ";
      if(options.test(DispStats)) buff << "r=" << refs_ << SPACE;
      if(!defined_) buff << "[not defined]" << SPACE;
      if(!options.test(DispFQ)) DisplayFiles(buff);
      auto str = buff.str();
      if(str.size() > 4) stream << str;
   }

   stream << CRLF;
}

//------------------------------------------------------------------------------

bool Define::EnterScope()
{
   Debug::ft("Define.EnterScope");

   //  If the macro is not yet defined, wait for its definition.
   //
   if(!defined_) return true;

   Context::SetPos(GetLoc());
   Context::File()->InsertMacro(this);
   if(!IsAtFileScope()) Log(DefineNotAtFileScope);

   if(rhs_ != nullptr)
   {
      rhs_->EnterBlock();
      auto result = Context::PopArg(true);
      value_ = result.item_;
   }

   return true;
}

//------------------------------------------------------------------------------

bool Define::GetSpan(size_t& begin, size_t& left, size_t& end) const
{
   Debug::ft("Define.GetSpan");

   begin = GetPos();
   end = GetFile()->GetLexer().CurrEnd(begin);
   return true;
}

//------------------------------------------------------------------------------

CxxToken* Define::PosToItem(size_t pos) const
{
   auto item = Macro::PosToItem(pos);
   if(item != nullptr) return item;

   return (rhs_ != nullptr ? rhs_->PosToItem(pos) : nullptr);
}

//------------------------------------------------------------------------------

void Define::SetExpr(ExprPtr& rhs)
{
   Debug::ft("Define.SetExpr");

   //  Now that the macro has been defined, EnterScope can be invoked.
   //
   rhs_ = std::move(rhs);
   defined_ = true;
   EnterScope();
}

//------------------------------------------------------------------------------

void Define::UpdatePos
   (EditorAction action, size_t begin, size_t count, size_t from) const
{
   Macro::UpdatePos(action, begin, count, from);
   if(rhs_ != nullptr) rhs_->UpdatePos(action, begin, count, from);
}

//==============================================================================

Elif::Elif()
{
   Debug::ft("Elif.ctor");
}

//------------------------------------------------------------------------------

void Elif::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   AlignLeft(stream, prefix);
   stream << HASH_ELIF_STR << SPACE;
   Conditional::Display(stream, prefix, options);
}

//------------------------------------------------------------------------------

bool Elif::EnterScope()
{
   Debug::ft("Elif.EnterScope");

   //  Compile the code that follows the #elif if its #if has not yet compiled
   //  any code and the condition following the #elif evaluates to true.
   //
   Context::SetPos(GetLoc());
   auto iff = Context::Optional();
   if(iff->HasCompiledCode()) return false;
   if(!Conditional::EnterScope()) return false;
   SetCompile();
   return true;
}

//==============================================================================

Else::Else()
{
   Debug::ft("Else.ctor");
}

//------------------------------------------------------------------------------

void Else::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   AlignLeft(stream, prefix);
   stream << HASH_ELSE_STR << CRLF;
   OptionalCode::Display(stream, prefix, options);
}

//------------------------------------------------------------------------------

bool Else::EnterScope()
{
   Debug::ft("Else.EnterScope");

   //  Compile the code that follows the #else if its #if/#ifdef/#ifndef has
   //  not yet compiled any code.
   //
   Context::SetPos(GetLoc());
   auto ifx = Context::Optional();
   return !ifx->HasCompiledCode();
}

//==============================================================================

Endif::Endif()
{
   Debug::ft("Endif.ctor");
}

//------------------------------------------------------------------------------

void Endif::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   AlignLeft(stream, prefix);
   stream << HASH_ENDIF_STR << CRLF;
}

//==============================================================================

Error::Error(string& text) : StringDirective(text)
{
   Debug::ft("Error.ctor");
}

//------------------------------------------------------------------------------

void Error::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   AlignLeft(stream, prefix);
   stream << HASH_ERROR_STR << SPACE << GetText() << CRLF;
}

//------------------------------------------------------------------------------

fn_name Error_EnterScope = "Error.EnterScope";

bool Error::EnterScope()
{
   Debug::ft(Error_EnterScope);

   Context::SetPos(GetLoc());
   Context::SwLog(Error_EnterScope, GetText(), 0);
   return true;
}

//==============================================================================

Existential::Existential(MacroNamePtr& macro) :
   else_(nullptr),
   endif_(nullptr)
{
   Debug::ft("Existential.ctor");

   name_ = std::move(macro);
}

//------------------------------------------------------------------------------

bool Existential::AddElse(const Else* e)
{
   Debug::ft("Existential.AddElse");

   if(else_ != nullptr) return false;
   else_ = e;
   return true;
}

//------------------------------------------------------------------------------

bool Existential::AddEndif(const Endif* e)
{
   Debug::ft("Existential.AddEndif");

   if(endif_ != nullptr) return false;
   endif_ = e;
   return true;
}

//------------------------------------------------------------------------------

void Existential::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   stream << Name() << CRLF;
   OptionalCode::Display(stream, prefix, options);
}

//------------------------------------------------------------------------------

void Existential::GetUsages(const CodeFile& file, CxxUsageSets& symbols)
{
   auto ref = name_->Referent();
   if(ref != nullptr) symbols.AddDirect(ref);
}

//------------------------------------------------------------------------------

CxxToken* Existential::PosToItem(size_t pos) const
{
   auto item = OptionalCode::PosToItem(pos);
   if(item != nullptr) return item;

   item = name_->PosToItem(pos);
   if(item != nullptr) return item;

   return (else_ != nullptr ? else_->PosToItem(pos) : nullptr);
}

//------------------------------------------------------------------------------

void Existential::UpdatePos
   (EditorAction action, size_t begin, size_t count, size_t from) const
{
   OptionalCode::UpdatePos(action, begin, count, from);
   name_->UpdatePos(action, begin, count, from);
   if(else_ != nullptr) else_->UpdatePos(action, begin, count, from);
}

//------------------------------------------------------------------------------

void Existential::UpdateXref(bool insert)
{
   name_->UpdateXref(insert);
}

//==============================================================================

Ifdef::Ifdef(MacroNamePtr& macro) : Existential(macro)
{
   Debug::ft("Ifdef.ctor");
}

//------------------------------------------------------------------------------

void Ifdef::Check() const
{
   Debug::ft("Ifdef.Check");

   //  A platform target in a .cpp should include or exclude the entire file.
   //
   auto file = GetFile();

   if(file->IsCpp() && !file->IsLastItem(GetEndif()))
   {
      Log(PreprocessorDirective);
   }
}

//------------------------------------------------------------------------------

void Ifdef::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   AlignLeft(stream, prefix);
   stream << HASH_IFDEF_STR << SPACE;
   Existential::Display(stream, prefix, options);
}

//------------------------------------------------------------------------------

bool Ifdef::EnterScope()
{
   Debug::ft("Ifdef.EnterScope");

   //  Compile the code that follows the #ifdef if the symbol that follows
   //  it has been defined.
   //
   Context::SetPos(GetLoc());
   Context::PushOptional(this);
   if(!Existential::SymbolPredefined()) return false;
   SetCompile();
   return true;
}

//==============================================================================

Iff::Iff() :
   else_(nullptr),
   endif_(nullptr)
{
   Debug::ft("Iff.ctor");
}

//------------------------------------------------------------------------------

bool Iff::AddElif(Elif* e)
{
   Debug::ft("Iff.AddElif");

   if(else_ != nullptr) return false;
   elifs_.push_back(e);
   return true;
}

//------------------------------------------------------------------------------

bool Iff::AddElse(const Else* e)
{
   Debug::ft("Iff.AddElse");

   if(else_ != nullptr) return false;
   else_ = e;
   return true;
}

//------------------------------------------------------------------------------

bool Iff::AddEndif(const Endif* e)
{
   Debug::ft("Iff.AddEndif");

   if(endif_ != nullptr) return false;
   endif_ = e;
   return true;
}

//------------------------------------------------------------------------------

void Iff::Check() const
{
   Debug::ft("Iff.Check");

   Log(PreprocessorDirective);
}

//------------------------------------------------------------------------------

void Iff::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   AlignLeft(stream, prefix);
   stream << HASH_IF_STR << SPACE;
   Conditional::Display(stream, prefix, options);
}

//------------------------------------------------------------------------------

bool Iff::EnterScope()
{
   Debug::ft("Iff.EnterScope");

   //  Compile the code that follows the #if if the condition that follows
   //  evaluates to true.
   //
   Context::SetPos(GetLoc());
   Context::PushOptional(this);
   if(!Conditional::EnterScope()) return false;
   SetCompile();
   return true;
}

//------------------------------------------------------------------------------

bool Iff::HasCompiledCode() const
{
   Debug::ft("Iff.HasCompiledCode");

   if(Conditional::HasCompiledCode()) return true;

   for(auto e = elifs_.cbegin(); e != elifs_.cend(); ++e)
   {
      if((*e)->HasCompiledCode()) return true;
   }

   return false;
}

//------------------------------------------------------------------------------

CxxToken* Iff::PosToItem(size_t pos) const
{
   auto item = Conditional::PosToItem(pos);
   if(item != nullptr) return item;

   for(auto e = elifs_.cbegin(); e != elifs_.cend(); ++e)
   {
      item = (*e)->PosToItem(pos);
      if(item != nullptr) return item;
   }

   return (else_ != nullptr ? else_->PosToItem(pos) : nullptr);
}

//------------------------------------------------------------------------------

void Iff::UpdatePos
   (EditorAction action, size_t begin, size_t count, size_t from) const
{
   Conditional::UpdatePos(action, begin, count, from);

   for(auto e = elifs_.cbegin(); e != elifs_.cend(); ++e)
   {
      (*e)->UpdatePos(action, begin, count, from);
   }

   if(else_ != nullptr) else_->UpdatePos(action, begin, count, from);
}

//==============================================================================

Ifndef::Ifndef(MacroNamePtr& macro) : Existential(macro)
{
   Debug::ft("Ifndef.ctor");
}

//------------------------------------------------------------------------------

void Ifndef::ChangeName(const string& name) const
{
   Debug::ft("Ifndef.ChangeName");

   auto ref = GetSymbol()->Referent();
   if(ref != nullptr) ref->Rename(name);
}

//------------------------------------------------------------------------------

void Ifndef::Check() const
{
   Debug::ft("Ifndef.Check");

   if(IsIncludeGuard())
   {
      if(Name() != GetFile()->MakeGuardName())
      {
         Log(IncludeGuardMisnamed);
      }
   }
   else
   {
      Log(PreprocessorDirective);
   }
}

//------------------------------------------------------------------------------

void Ifndef::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   AlignLeft(stream, prefix);
   stream << HASH_IFNDEF_STR << SPACE;
   Existential::Display(stream, prefix, options);
}

//------------------------------------------------------------------------------

bool Ifndef::EnterScope()
{
   Debug::ft("Ifndef.EnterScope");

   //  Compile the code that follows the #ifndef if the symbol that follows
   //  it has not been defined.
   //
   Context::SetPos(GetLoc());
   Context::PushOptional(this);
   if(Existential::SymbolPredefined()) return false;
   SetCompile();
   return true;
}

//------------------------------------------------------------------------------

bool Ifndef::IsIncludeGuard() const
{
   Debug::ft("Ifndef.IsIncludeGuard");

   //  For this to be an #include guard, it must appear in a header,
   //  the #endif must be the last item in the file, and the guard
   //  symbol after the #ifndef must have appeared in a #define.
   //
   auto file = GetFile();
   if(!file->IsHeader()) return false;
   if(!file->IsLastItem(GetEndif())) return false;
   return (Referent() != nullptr);
}

//==============================================================================

Include::Include(string& name, bool angle) : SymbolDirective(name),
   angle_(angle),
   group_(Ungrouped)
{
   Debug::ft("Include.ctor");
}

//------------------------------------------------------------------------------

Include::~Include()
{
   Debug::ft("Include.dtor");
}

//------------------------------------------------------------------------------

void Include::CalcGroup()
{
   Debug::ft("Include.CalcGroup");

   group_ = GetFile()->CalcGroup(*this);
}

//------------------------------------------------------------------------------

void Include::Delete()
{
   Debug::ft("Include.Delete");

   GetFile()->DeleteInclude(this);
}

//------------------------------------------------------------------------------

void Include::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   AlignLeft(stream, prefix);
   stream << HASH_INCLUDE_STR << SPACE;
   stream << (angle_ ? '<' : QUOTE);
   stream << Name();
   stream << (angle_ ? '>' : QUOTE);
   stream << CRLF;
}

//------------------------------------------------------------------------------

CodeFile* Include::FindFile() const
{
   Debug::ft("Include.FindFile");

   auto lib = Singleton<Library>::Instance();
   if(Name().empty()) return nullptr;
   return lib->FindFile(Name());
}

//==============================================================================

Line::Line(string& text) : StringDirective(text)
{
   Debug::ft("Line.ctor");
}

//------------------------------------------------------------------------------

void Line::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   AlignLeft(stream, prefix);
   stream << HASH_LINE_STR << SPACE << GetText() << CRLF;
}

//==============================================================================

Macro::Macro(const string& name) :
   refs_(0),
   name_(name)
{
   Debug::ft("Macro.ctor");

   SetScope(Singleton<CxxRoot>::Instance()->GlobalNamespace());
   Singleton<CxxSymbols>::Instance()->InsertMacro(this);
}

//------------------------------------------------------------------------------

Macro::~Macro()
{
   Debug::ftnt("Macro.dtor");

   Singleton<CxxSymbols>::Extant()->EraseMacro(this);
}

//------------------------------------------------------------------------------

void Macro::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   AlignLeft(stream, prefix);
   stream << HASH_DEFINE_STR << SPACE;
   stream << Name();

   if(!options.test(DispCode))
   {
      stream << " // ";
      if(options.test(DispStats)) stream << "r=" << refs_ << SPACE;
      stream << "[built-in]";
   }

   stream << CRLF;
}

//------------------------------------------------------------------------------

bool Macro::Empty() const
{
   Debug::ft("Macro.Empty");

   return (GetValue() == nullptr);
}

//------------------------------------------------------------------------------

Numeric Macro::GetNumeric() const
{
   Debug::ft("Macro.GetNumeric");

   auto ref = Referent();
   if(ref != nullptr) return ref->GetNumeric();
   return Numeric::Nil;
}

//------------------------------------------------------------------------------

fn_name Macro_GetValue = "Macro.GetValue";

CxxToken* Macro::GetValue() const
{
   Debug::ft(Macro_GetValue);

   Debug::SwLog(Macro_GetValue, strOver(this), 0);
   return nullptr;
}

//------------------------------------------------------------------------------

bool Macro::NameRefersToItem(const string& name,
   const CxxScope* scope, CodeFile* file, SymbolView& view) const
{
   Debug::ft("Macro.NameRefersToItem");

   //  If this item was not declared in a file, it must be a macro name
   //  that was defined for the compile (e.g. OS_WIN).
   //
   if(GetFile() == nullptr)
   {
      view = DeclaredGlobally;
      return true;
   }

   return CxxScoped::NameRefersToItem(name, scope, file, view);
}

//------------------------------------------------------------------------------

void Macro::Rename(const string& name)
{
   Debug::ft("Macro.Rename");

   CxxScoped::RenameNonQual(name_, name);
}

//------------------------------------------------------------------------------

fn_name Macro_SetExpr = "Macro.SetExpr";

void Macro::SetExpr(ExprPtr& rhs)
{
   Debug::ft(Macro_SetExpr);

   //  This shouldn't be invoked on a built-in macro.
   //
   Debug::SwLog(Macro_SetExpr, name_, 0);
}

//------------------------------------------------------------------------------

string Macro::TypeString(bool arg) const
{
   Debug::ft("Macro.TypeString");

   auto value = GetValue();
   if(value != nullptr) return value->TypeString(arg);
   return EMPTY_STR;
}

//------------------------------------------------------------------------------

bool Macro::WasRead()
{
   ++refs_;
   return true;
}

//==============================================================================

MacroName::MacroName(string& name) :
   ref_(nullptr),
   predefined_(false)
{
   Debug::ft("MacroName.ctor");

   std::swap(name_, name);
}

//------------------------------------------------------------------------------

MacroName::~MacroName()
{
   Debug::ftnt("MacroName.dtor");

   UpdateXref(false);
}

//------------------------------------------------------------------------------

void MacroName::EnterBlock()
{
   Debug::ft("MacroName.EnterBlock");

   Context::SetPos(GetLoc());
   Context::PushArg(StackArg(Referent(), 0, false, false));
}

//------------------------------------------------------------------------------

CxxScope* MacroName::GetScope() const
{
   return Singleton<CxxRoot>::Instance()->GlobalNamespace();
}

//------------------------------------------------------------------------------

void MacroName::GetUsages(const CodeFile& file, CxxUsageSets& symbols)
{
   //  Add our referent as a direct usage.
   //
   if(ref_ != nullptr) symbols.AddDirect(ref_);
}

//------------------------------------------------------------------------------

void MacroName::ItemDeleted(const CxxScoped* item) const
{
   if(ref_ == item)
   {
      ref_ = nullptr;
   }
}

//------------------------------------------------------------------------------

void MacroName::Print(ostream& stream, const Flags& options) const
{
   stream << name_;
}

//------------------------------------------------------------------------------

CxxScoped* MacroName::Referent() const
{
   Debug::ft("MacroName.Referent");

   //  This is invoked to find a referent in a preprocessor directive.
   //
   if(ref_ != nullptr) return ref_;

   //  Look for the macro name.  If it is visible, it has not necessarily been
   //  defined: it could have been used in a file that is visible to this one,
   //  but only in a conditional compilation directive that caused it to be
   //  added to the symbol table, which is done at the bottom of this function.
   //
   auto syms = Singleton<CxxSymbols>::Instance();
   auto file = Context::File();
   auto scope = Singleton<CxxRoot>::Instance()->GlobalNamespace();
   SymbolView view;
   ref_ = syms->FindSymbol(file, scope, name_, MACRO_MASK, view);

   if(ref_ != nullptr)
   {
      auto macro = static_cast<Macro*>(ref_);
      predefined_ = macro->IsDefined();
      macro->WasRead();
      return ref_;
   }

   //  Look for the macro name again, even if it has been defined in a file
   //  that is not visible to this one.
   //
   ref_ = syms->FindMacro(name_);

   if(ref_ != nullptr)
   {
      auto macro = static_cast<Macro*>(ref_);
      macro->WasRead();
      return ref_;
   }

   //  This is the first appearance of the macro name, so create a placeholder
   //  for it.
   //
   MacroPtr macro(new Define(name_));
   ref_ = macro.get();
   Singleton<CxxRoot>::Instance()->AddMacro(macro);
   ref_->WasRead();
   return ref_;
}

//------------------------------------------------------------------------------

void MacroName::Rename(const string& name)
{
   Debug::ft("MacroName.Rename");

   CxxNamed::RenameNonQual(name_, name);
}

//------------------------------------------------------------------------------

fn_name MacroName_TypeString = "MacroName.TypeString";

string MacroName::TypeString(bool arg) const
{
   auto ref = Referent();
   if(ref != nullptr) return ref->TypeString(arg);

   auto expl = "Failed to find referent for " + name_;
   Context::SwLog(MacroName_TypeString, expl, 0);
   return ERROR_STR;
}

//------------------------------------------------------------------------------

void MacroName::UpdateXref(bool insert)
{
   if(ref_ != nullptr) ref_->UpdateReference(this, insert);
}

//------------------------------------------------------------------------------

bool MacroName::WasPredefined() const
{
   Debug::ft("MacroName.WasPredefined");

   //  Make sure that the referent has been searched for.
   //
   Referent();
   return predefined_;
}

//==============================================================================

Optional::Optional()
{
   Debug::ft("Optional.ctor");
}

//==============================================================================

OptionalCode::OptionalCode() :
   begin_(string::npos),
   end_(0),
   erased_(false),
   compile_(false)
{
   Debug::ft("OptionalCode.ctor");
}

//------------------------------------------------------------------------------

void OptionalCode::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   if(compile_) return;

   if(begin_ == string::npos) return;

   auto file = GetDeclFile();

   if(file == nullptr)
   {
      stream << "ERROR: FILE NOT FOUND" << CRLF;
      return;
   }

   const auto& code = file->GetCode();

   if(code.size() < end_)
   {
      stream << "ERROR: CODE NOT FOUND" << CRLF;
      return;
   }

   stream << prefix;

   for(auto i = begin_; i < end_; ++i)
   {
      stream << code.at(i);
      if(code.at(i) == CRLF) stream << prefix;
   }

   stream << CRLF;
}

//------------------------------------------------------------------------------

fn_name OptionalCode_UpdatePos = "OptionalCode.UpdatePos";

void OptionalCode::UpdatePos
   (EditorAction action, size_t begin, size_t count, size_t from) const
{
   Optional::UpdatePos(action, begin, count, from);

   if(begin_ == string::npos) return;
   if(IsInternal()) return;

   //  Uncompiled code can only be cut and pasted in its entirety.
   //
   switch(action)
   {
   case Erased:
      if(((begin + count) > begin_) && (begin + count < end_))
      {
         Debug::SwLog(OptionalCode_UpdatePos, "Partially erased", 0);
         begin_ = string::npos;
         end_ = 0;
      }
      else if(!erased_ && (begin_ >= begin))
      {
         if(begin_ < (begin + count))
         {
            erased_ = true;
         }
         else
         {
            begin_ -= count;
            end_ -= count;
         }
      }
      break;

   case Inserted:
      if(!erased_ && (begin_ >= begin))
      {
         begin_ += count;
         end_ += count;
      }
      break;

   case Pasted:
      if(erased_)
      {
         if(begin_ >= from)
         {
            if(end_ < (from + count))
            {
               begin_ = begin_ + begin - from;
               end_ = end_ + begin - from;
               erased_ = false;
            }
            else
            {
               Debug::SwLog(OptionalCode_UpdatePos, "Partially pasted", 0);
               begin_ = string::npos;
               end_ = 0;
            }
         }
      }
      else if(begin_ >= begin)
      {
         begin_ += count;
         end_ += count;
      }
      break;
   }
}

//==============================================================================

Pragma::Pragma(string& text) : StringDirective(text)
{
   Debug::ft("Pragma.ctor");
}

//------------------------------------------------------------------------------

void Pragma::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   AlignLeft(stream, prefix);
   stream << HASH_PRAGMA_STR << SPACE << GetText() << CRLF;
}

//------------------------------------------------------------------------------

bool Pragma::IsIncludeGuard() const
{
   Debug::ft("Pragma.IsIncludeGuard");

   return (GetText() == "once");
}

//==============================================================================

StringDirective::StringDirective(string& text)
{
   Debug::ft("StringDirective.ctor");

   std::swap(text_, text);
}

//==============================================================================

SymbolDirective::SymbolDirective(string& name)
{
   Debug::ft("SymbolDirective.ctor");

   std::swap(name_, name);
}

//==============================================================================

Undef::Undef(string& name) : SymbolDirective(name)
{
   Debug::ft("Undef.ctor");
}

//------------------------------------------------------------------------------

void Undef::Check() const
{
   Debug::ft("Undef.Check");

   Log(PreprocessorDirective);
}

//------------------------------------------------------------------------------

void Undef::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   AlignLeft(stream, prefix);
   stream << HASH_UNDEF_STR << SPACE;
   stream << Name();
   stream << CRLF;
}
}
