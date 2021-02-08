//==============================================================================
//
//  CxxDirective.cpp
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
#include "CxxDirective.h"
#include <bitset>
#include <iosfwd>
#include <sstream>
#include "CodeFile.h"
#include "CodeTypes.h"
#include "CxxArea.h"
#include "CxxExecute.h"
#include "CxxRoot.h"
#include "CxxSymbols.h"
#include "Debug.h"
#include "Library.h"
#include "Registry.h"
#include "Singleton.h"

using namespace NodeBase;
using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace CodeTools
{
void AlignLeft(ostream& stream, const string& prefix)
{
   //  If PREFIX is more than one indentation, indent one level less.
   //
   if(prefix.size() < INDENT_SIZE)
      stream << prefix;
   else
      stream << prefix.substr(INDENT_SIZE);
}

//------------------------------------------------------------------------------

Conditional::Conditional()
{
   Debug::ft("Conditional.ctor");
}

//------------------------------------------------------------------------------

void Conditional::AddToXref() const
{
   condition_->AddToXref();
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

void Conditional::GetUsages(const CodeFile& file, CxxUsageSets& symbols) const
{
   condition_->GetUsages(file, symbols);
}

//------------------------------------------------------------------------------

void Conditional::Shrink()
{
   OptionalCode::Shrink();
   condition_->Shrink();
}

//------------------------------------------------------------------------------

void Conditional::UpdatePos
   (EditorAction action, size_t begin, size_t count, size_t from) const
{
   OptionalCode::UpdatePos(action, begin, count, from);
   condition_->UpdatePos(action, begin, count, from);
}

//==============================================================================

CxxDirective::CxxDirective()
{
   Debug::ft("CxxDirective.ctor");
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

void Define::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   AlignLeft(stream, prefix);
   stream << HASH_DEFINE_STR << SPACE << *Name();

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
   if(!AtFileScope()) Log(DefineNotAtFileScope);

   if(rhs_ != nullptr)
   {
      rhs_->EnterBlock();
      auto result = Context::PopArg(true);
      value_ = result.item;
   }

   return true;
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

void Define::Shrink()
{
   Macro::Shrink();
   if(rhs_ != nullptr) rhs_->Shrink();
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

   CxxStats::Incr(CxxStats::ELIF_DIRECTIVE);
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

   CxxStats::Incr(CxxStats::ELSE_DIRECTIVE);
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

   CxxStats::Incr(CxxStats::ENDIF_DIRECTIVE);
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

   CxxStats::Incr(CxxStats::ERROR_DIRECTIVE);
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
   else_(nullptr)
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

void Existential::AddToXref() const
{
   name_->AddToXref();
}

//------------------------------------------------------------------------------

void Existential::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   stream << *Name() << CRLF;
   OptionalCode::Display(stream, prefix, options);
}

//------------------------------------------------------------------------------

void Existential::GetUsages(const CodeFile& file, CxxUsageSets& symbols) const
{
   auto ref = name_->Referent();
   if(ref != nullptr) symbols.AddDirect(ref);
}

//------------------------------------------------------------------------------

void Existential::Shrink()
{
   OptionalCode::Shrink();
   name_->Shrink();
}

//------------------------------------------------------------------------------

void Existential::UpdatePos
   (EditorAction action, size_t begin, size_t count, size_t from) const
{
   OptionalCode::UpdatePos(action, begin, count, from);
   name_->UpdatePos(action, begin, count, from);
   if(else_ != nullptr) else_->UpdatePos(action, begin, count, from);
}

//==============================================================================

Ifdef::Ifdef(MacroNamePtr& macro) : Existential(macro)
{
   Debug::ft("Ifdef.ctor");

   CxxStats::Incr(CxxStats::IFDEF_DIRECTIVE);
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
   if(!Existential::SymbolDefined()) return false;
   SetCompile();
   return true;
}

//==============================================================================

Iff::Iff() :
   else_(nullptr)
{
   Debug::ft("Iff.ctor");

   CxxStats::Incr(CxxStats::IF_DIRECTIVE);
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

   if(HasCompiledCode()) return true;

   for(auto e = elifs_.cbegin(); e != elifs_.cend(); ++e)
   {
      if((*e)->HasCompiledCode()) return true;
   }

   return false;
}

//------------------------------------------------------------------------------

void Iff::Shrink()
{
   Conditional::Shrink();
   elifs_.shrink_to_fit();
   CxxStats::Vectors(CxxStats::IF_DIRECTIVE, elifs_.capacity());
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

   CxxStats::Incr(CxxStats::IFNDEF_DIRECTIVE);
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
   if(Existential::SymbolDefined()) return false;
   SetCompile();
   return true;
}

//==============================================================================

Include::Include(string& name, bool angle) : SymbolDirective(name),
   angle_(angle)
{
   Debug::ft("Include.ctor");

   CxxStats::Incr(CxxStats::INCLUDE_DIRECTIVE);
}

//------------------------------------------------------------------------------

void Include::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   AlignLeft(stream, prefix);
   stream << HASH_INCLUDE_STR << SPACE;
   stream << (angle_ ? '<' : QUOTE);
   stream << *Name();
   stream << (angle_ ? '>' : QUOTE);
   stream << CRLF;
}

//------------------------------------------------------------------------------

CodeFile* Include::FindFile() const
{
   Debug::ft("Include.FindFile");

   auto lib = Singleton< Library >::Instance();
   auto name = Name();
   if(name == nullptr) return nullptr;
   return lib->FindFile(*name);
}

//------------------------------------------------------------------------------

void Include::Shrink()
{
   SymbolDirective::Shrink();
   CxxStats::Strings(CxxStats::INCLUDE_DIRECTIVE, Name()->capacity());
}

//==============================================================================

Line::Line(string& text) : StringDirective(text)
{
   Debug::ft("Line.ctor");

   CxxStats::Incr(CxxStats::LINE_DIRECTIVE);
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

   SetScope(Singleton< CxxRoot >::Instance()->GlobalNamespace());
   Singleton< CxxSymbols >::Instance()->InsertMacro(this);
   CxxStats::Incr(CxxStats::DEFINE_DIRECTIVE);
}

//------------------------------------------------------------------------------

Macro::~Macro()
{
   Debug::ftnt("Macro.dtor");

   Singleton< CxxSymbols >::Extant()->EraseMacro(this);
   CxxStats::Decr(CxxStats::DEFINE_DIRECTIVE);
}

//------------------------------------------------------------------------------

void Macro::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   AlignLeft(stream, prefix);
   stream << HASH_DEFINE_STR << SPACE;
   stream << *Name();

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
   const CxxScope* scope, const CodeFile* file, SymbolView* view) const
{
   Debug::ft("Macro.NameRefersToItem");

   //  If this item was not declared in a file, it must be a macro name
   //  that was defined for the compile (e.g. OS_WIN).
   //
   if(GetFile() == nullptr)
   {
      *view = DeclaredGlobally;
      return true;
   }

   return CxxScoped::NameRefersToItem(name, scope, file, view);
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

void Macro::Shrink()
{
   CxxScoped::Shrink();
   name_.shrink_to_fit();
   CxxStats::Strings(CxxStats::DEFINE_DIRECTIVE, name_.capacity());
   CxxStats::Vectors(CxxStats::DEFINE_DIRECTIVE, XrefSize());
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
   defined_(false)
{
   Debug::ft("MacroName.ctor");

   std::swap(name_, name);
   CxxStats::Incr(CxxStats::MACRO_NAME);
}

//------------------------------------------------------------------------------

MacroName::~MacroName()
{
   Debug::ftnt("MacroName.dtor");

   CxxStats::Decr(CxxStats::QUAL_NAME);
}

//------------------------------------------------------------------------------

void MacroName::AddToXref() const
{
   if(ref_ != nullptr) ref_->AddReference(this);
}

//------------------------------------------------------------------------------

void MacroName::EnterBlock()
{
   Debug::ft("MacroName.EnterBlock");

   Context::SetPos(GetLoc());
   Context::PushArg(StackArg(Referent(), 0, false));
}

//------------------------------------------------------------------------------

CxxScope* MacroName::GetScope() const
{
   return Singleton< CxxRoot >::Instance()->GlobalNamespace();
}

//------------------------------------------------------------------------------

void MacroName::GetUsages(const CodeFile& file, CxxUsageSets& symbols) const
{
   //  Add our referent as a direct usage.
   //
   if(ref_ != nullptr) symbols.AddDirect(ref_);
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
   auto syms = Singleton< CxxSymbols >::Instance();
   auto file = Context::File();
   auto scope = Singleton< CxxRoot >::Instance()->GlobalNamespace();
   SymbolView view;
   ref_ = syms->FindSymbol(file, scope, name_, MACRO_MASK, &view);

   if(ref_ != nullptr)
   {
      auto macro = static_cast< Macro* >(ref_);
      defined_ = macro->IsDefined();
      macro->WasRead();
      return ref_;
   }

   //  Look for the macro name again, even if it has been defined in a file
   //  that is not visible to this one.
   //
   ref_ = syms->FindMacro(name_);

   if(ref_ != nullptr)
   {
      auto macro = static_cast< Macro* >(ref_);
      macro->WasRead();
      return ref_;
   }

   //  This is the first appearance of the macro name, so create a placeholder
   //  for it.
   //
   auto name = name_;
   MacroPtr macro(new Define(name));
   ref_ = macro.get();
   Singleton< CxxRoot >::Instance()->AddMacro(macro);
   ref_->WasRead();
   return ref_;
}

//------------------------------------------------------------------------------

void MacroName::Shrink()
{
   CxxNamed::Shrink();
   name_.shrink_to_fit();
   CxxStats::Strings(CxxStats::MACRO_NAME, name_.capacity());
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

bool MacroName::WasDefined() const
{
   Debug::ft("MacroName.WasDefined");

   //  Make sure that the referent has been searched for.
   //
   Referent();
   return defined_;
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

   auto file = Singleton< Library >::Instance()->Files().At(GetDeclFid());

   if(file == nullptr)
   {
      stream << "ERROR: FILE NOT FOUND" << CRLF;
      return;
   }

   auto code = file->GetCode();

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

void OptionalCode::UpdatePos
   (EditorAction action, size_t begin, size_t count, size_t from) const
{
   Optional::UpdatePos(action, begin, count, from);

   //  Although begin_ and end_ should probably be updated, they are currently
   //  used only to display code, and this isn't done after editing it.
}

//==============================================================================

Pragma::Pragma(string& text) : StringDirective(text)
{
   Debug::ft("Pragma.ctor");

   CxxStats::Incr(CxxStats::PRAGMA_DIRECTIVE);
}

//------------------------------------------------------------------------------

void Pragma::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   AlignLeft(stream, prefix);
   stream << HASH_PRAGMA_STR << SPACE << GetText() << CRLF;
}

//==============================================================================

StringDirective::StringDirective(string& text)
{
   Debug::ft("StringDirective.ctor");

   std::swap(text_, text);
}

//------------------------------------------------------------------------------

void StringDirective::Shrink()
{
   CxxDirective::Shrink();
   text_.shrink_to_fit();
}

//==============================================================================

SymbolDirective::SymbolDirective(string& name)
{
   Debug::ft("SymbolDirective.ctor");

   std::swap(name_, name);
}

//------------------------------------------------------------------------------

void SymbolDirective::Shrink()
{
   CxxDirective::Shrink();
   name_.shrink_to_fit();
}

//==============================================================================

Undef::Undef(string& name) : SymbolDirective(name)
{
   Debug::ft("Undef.ctor");

   CxxStats::Incr(CxxStats::UNDEF_DIRECTIVE);
}

//------------------------------------------------------------------------------

void Undef::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   AlignLeft(stream, prefix);
   stream << HASH_UNDEF_STR << SPACE;
   stream << *Name();
   stream << CRLF;
}

//------------------------------------------------------------------------------

void Undef::Shrink()
{
   SymbolDirective::Shrink();
   CxxStats::Strings(CxxStats::UNDEF_DIRECTIVE, Name()->capacity());
}
}
