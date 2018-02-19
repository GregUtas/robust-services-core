//==============================================================================
//
//  CxxScoped.cpp
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
#include "CxxScoped.h"
#include <bitset>
#include <set>
#include <sstream>
#include <utility>
#include <vector>
#include "CodeFile.h"
#include "CodeSet.h"
#include "CxxArea.h"
#include "CxxExecute.h"
#include "CxxRoot.h"
#include "CxxScope.h"
#include "CxxString.h"
#include "CxxSymbols.h"
#include "CxxToken.h"
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
fn_name Argument_ctor = "Argument.ctor";

Argument::Argument(string& name, TypeSpecPtr& spec) :
   spec_(spec.release()),
   reads_(0),
   writes_(0),
   nonconst_(false)
{
   Debug::ft(Argument_ctor);

   std::swap(name_, name);
   spec_->SetUserType(Cxx::Function);
   CxxStats::Incr(CxxStats::ARG_DECL);
}

//------------------------------------------------------------------------------

fn_name Argument_Check = "Argument.Check";

void Argument::Check() const
{
   Debug::ft(Argument_Check);

   spec_->Check();
   if(name_.empty()) Log(AnonymousArgument);
}

//------------------------------------------------------------------------------

fn_name Argument_CheckVoid = "Argument.CheckVoid";

void Argument::CheckVoid() const
{
   Debug::ft(Argument_CheckVoid);

   if(name_.empty())
   {
      if(*spec_->Name() == VOID_STR)
      {
         //  Deleting the empty argument "(void)" makes it much easier to
         //  compare function signatures and match arguments to functions.
         //
         Log(VoidAsArgument);
         auto func = static_cast< Function* >(GetScope());
         func->DeleteVoidArg();
      }
   }
}

//------------------------------------------------------------------------------

fn_name Argument_EnterBlock = "Argument.EnterBlock";

void Argument::EnterBlock()
{
   Debug::ft(Argument_EnterBlock);

   if(!name_.empty()) Singleton< CxxSymbols >::Instance()->InsertLocal(this);
}

//------------------------------------------------------------------------------

fn_name Argument_EnterScope = "Argument.EnterScope";

bool Argument::EnterScope()
{
   Debug::ft(Argument_EnterScope);

   Context::SetPos(GetPos());
   spec_->EnteringScope(GetScope());

   if(default_ != nullptr)
   {
      default_->EnterBlock();
      auto result = Context::PopArg(true);
      spec_->MustMatchWith(result);
   }

   CheckVoid();
   return true;
}

//------------------------------------------------------------------------------

fn_name Argument_ExitBlock = "Argument.ExitBlock";

void Argument::ExitBlock()
{
   Debug::ft(Argument_ExitBlock);

   if(name_.empty()) return;
   Singleton< CxxSymbols >::Instance()->EraseLocal(this);
}

//------------------------------------------------------------------------------

fn_name Argument_GetUsages = "Argument.GetUsages";

void Argument::GetUsages(const CodeFile& file, CxxUsageSets& symbols) const
{
   Debug::ft(Argument_GetUsages);

   spec_->GetUsages(file, symbols);
   if(default_ != nullptr) default_->GetUsages(file, symbols);
}

//------------------------------------------------------------------------------

fn_name Argument_IsPassedByValue = "Argument.IsPassedByValue";

bool Argument::IsPassedByValue() const
{
   Debug::ft(Argument_IsPassedByValue);

   return ((spec_->Ptrs(true) == 0) && (spec_->Refs() == 0));
}

//------------------------------------------------------------------------------

void Argument::Print(ostream& stream, const Flags& options) const
{
   spec_->Print(stream, options);

   if(spec_->GetFuncSpec() == nullptr)
   {
      if(!name_.empty()) stream << SPACE << *Name();
   }

   spec_->DisplayArrays(stream);

   if(default_ != nullptr)
   {
      stream << " = ";
      default_->Print(stream, options);
   }
}

//------------------------------------------------------------------------------

fn_name Argument_SetNonConst = "Argument.SetNonConst";

bool Argument::SetNonConst()
{
   Debug::ft(Argument_SetNonConst);

   nonconst_ = true;
   return !IsConst();
}

//------------------------------------------------------------------------------

void Argument::Shrink()
{
   name_.shrink_to_fit();
   CxxStats::Strings(CxxStats::ARG_DECL, name_.capacity());
   spec_->Shrink();
   if(default_ != nullptr) default_->Shrink();
}

//------------------------------------------------------------------------------

string Argument::TypeString(bool arg) const
{
   return spec_->TypeString(arg);
}

//------------------------------------------------------------------------------

fn_name Argument_WasWritten = "Argument.WasWritten";

bool Argument::WasWritten(const StackArg* arg, bool passed)
{
   Debug::ft(Argument_WasWritten);

   ++writes_;
   if((arg == nullptr) || (arg->Ptrs(true) == 0)) nonconst_ = true;
   return true;
}

//==============================================================================

fn_name BaseDecl_ctor = "BaseDecl.ctor";

BaseDecl::BaseDecl(QualNamePtr& name, Cxx::Access access) :
   name_(name.release()),
   using_(false)
{
   Debug::ft(BaseDecl_ctor);

   SetAccess(access);
   CxxStats::Incr(CxxStats::BASE_DECL);
}

//------------------------------------------------------------------------------

void BaseDecl::DisplayDecl(ostream& stream, bool fq) const
{
   stream << " : " << GetAccess() << SPACE;
   strName(stream, fq, name_.get());
}

//------------------------------------------------------------------------------

fn_name BaseDecl_EnterScope = "BaseDecl.EnterScope";

bool BaseDecl::EnterScope()
{
   Debug::ft(BaseDecl_EnterScope);

   //  If the base class cannot be found, return false so that this
   //  object will be deleted.  Otherwise, record our new subclass.
   //
   Context::SetPos(GetPos());
   FindReferent();
   if(Referent() == nullptr) return false;
   GetClass()->AddSubclass(static_cast< Class* >(Context::Scope()));
   return true;
}

//------------------------------------------------------------------------------

fn_name BaseDecl_FindReferent = "BaseDecl.FindReferent";

void BaseDecl::FindReferent()
{
   Debug::ft(BaseDecl_FindReferent);

   //  Find the class to which this base class declaration refers.
   //
   SymbolView view;
   auto item = ResolveName(GetFile(), GetScope(), CLASS_MASK, &view);  //*

   if(item != nullptr)
   {
      using_ = view.using_;
      item->SetAsReferent(this);
      return;
   }

   //  The base class wasn't found.
   //
   auto log = "Unknown base class: " + *Name() + " [" + strLocation() + ']';
   Debug::SwErr(BaseDecl_FindReferent, log, 0, InfoLog);
}

//------------------------------------------------------------------------------

Class* BaseDecl::GetClass() const
{
   return static_cast< Class* >(name_->GetReferent());
}

//------------------------------------------------------------------------------

fn_name BaseDecl_GetUsages = "BaseDecl.GetUsages";

void BaseDecl::GetUsages(const CodeFile& file, CxxUsageSets& symbols) const
{
   Debug::ft(BaseDecl_GetUsages);

   //  Our class was used as a base class.  Its name cannot include template
   //  arguments, because subclassing a template instance is not supported.
   //
   symbols.AddBase(GetClass());
   if(using_) symbols.AddUser(this);
}

//------------------------------------------------------------------------------

fn_name BaseDecl_Referent = "BaseDecl.Referent";

CxxNamed* BaseDecl::Referent() const
{
   Debug::ft(BaseDecl_Referent);

   return name_->GetReferent();
}

//------------------------------------------------------------------------------

fn_name BaseDecl_ScopedName = "BaseDecl.ScopedName";

string BaseDecl::ScopedName(bool templates) const
{
   Debug::ft(BaseDecl_ScopedName);

   return Referent()->ScopedName(templates);
}

//------------------------------------------------------------------------------

fn_name BaseDecl_SetAccess = "BaseDecl.SetAccess";

void BaseDecl::SetAccess(Cxx::Access access)
{
   Debug::ft(BaseDecl_SetAccess);

   //  This is invoked twice: first by our constructor, and then by
   //  Parser.SetContext.  We want to preserve the value set by our
   //  constructor, so if the current value isn't Cxx::Public (the
   //  default), it has already been set and should be preserved.
   //
   if(GetAccess() != Cxx::Public) return;
   CxxScoped::SetAccess(access);
}

//------------------------------------------------------------------------------

string BaseDecl::TypeString(bool arg) const
{
   return GetClass()->TypeString(arg);
}

//==============================================================================

fn_name CxxScoped_ctor = "CxxScoped.ctor";

CxxScoped::CxxScoped() :
   scope_(nullptr),
   access_(Cxx::Public),
   public_(false),
   protected_(false)
{
   Debug::ft(CxxScoped_ctor);
}

//------------------------------------------------------------------------------

fn_name CxxScoped_dtor = "CxxScoped.dtor";

CxxScoped::~CxxScoped()
{
   Debug::ft(CxxScoped_dtor);
}

//------------------------------------------------------------------------------

fn_name CxxScoped_AccessibilityTo = "CxxScoped.AccessibilityTo";

void CxxScoped::AccessibilityTo(const CxxScope* scope, SymbolView* view) const
{
   Debug::ft(CxxScoped_AccessibilityTo);

   return GetScope()->AccessibilityOf(scope, this, view);
}

//------------------------------------------------------------------------------

void CxxScoped::AddFiles(SetOfIds& imSet) const
{
   auto decl = GetDeclFile();
   auto defn = GetDefnFile();
   if(decl != nullptr) imSet.insert(decl->Fid());
   if(defn != nullptr) imSet.insert(defn->Fid());
}

//------------------------------------------------------------------------------

fn_name CxxScoped_BroadestAccessUsed = "CxxScoped.BroadestAccessUsed";

Cxx::Access CxxScoped::BroadestAccessUsed() const
{
   Debug::ft(CxxScoped_BroadestAccessUsed);

   if(GetClass() == nullptr) return Cxx::Public;
   if(public_) return Cxx::Public;
   if(protected_) return Cxx::Protected;
   return Cxx::Private;
}

//------------------------------------------------------------------------------

fn_name CxxScoped_CheckAccessControl = "CxxScoped.CheckAccessControl";

void CxxScoped::CheckAccessControl() const
{
   Debug::ft(CxxScoped_CheckAccessControl);

   //  If an item is used, log it if its access control could be
   //  more restrictive.
   //c Support this for templates, considering all instances/analogs.
   //
   auto cls = GetClass();
   if(cls == nullptr) return;
   if(IsTemplate()) return;
   if(cls->IsTemplate()) return;
   if(IsInTemplateInstance()) return;
   if(IsUnused()) return;

   auto used = BroadestAccessUsed();

   switch(used)
   {
   case Cxx::Private:
      if(GetAccess() > used) Log(ItemCouldBePrivate);
      break;
   case Cxx::Protected:
      if(GetAccess() > used) Log(ItemCouldBeProtected);
   }
}

//------------------------------------------------------------------------------

fn_name CxxScoped_CheckIfHiding = "CxxScoped.CheckIfHiding";

void CxxScoped::CheckIfHiding() const
{
   Debug::ft(CxxScoped_CheckIfHiding);

   auto item = FindInheritedName();
   if((item == nullptr) || (item->GetAccess() == Cxx::Private)) return;
   Log(HidesInheritedName);
}

//------------------------------------------------------------------------------

fn_name CxxScoped_CheckIfUsed = "CxxScoped.CheckIfUsed";

void CxxScoped::CheckIfUsed(Warning warning) const
{
   Debug::ft(CxxScoped_CheckIfUsed);

   if(IsUnused()) Log(warning);
}

//------------------------------------------------------------------------------

void CxxScoped::DisplayFiles(ostream& stream) const
{
   auto decl = GetDeclFile();
   auto defn = GetDefnFile();

   if(AtFileScope())
   {
      if(decl != nullptr)
      {
         stream << decl->Name();

         if((defn != nullptr) && (defn != decl))
         {
            stream << " & " << defn->Name();
         }
      }
   }
   else
   {
      if((defn != nullptr) && (defn != decl))
      {
         stream << defn->Name();
      }
   }
}

//------------------------------------------------------------------------------

fn_name CxxScoped_FileScopeAccessiblity = "CxxScoped.FileScopeAccessiblity";

Accessibility CxxScoped::FileScopeAccessiblity() const
{
   Debug::ft(CxxScoped_FileScopeAccessiblity);

   if(IsInTemplateInstance()) return Unrestricted;
   if(GetFile()->IsCpp()) return Restricted;
   return Unrestricted;
}

//------------------------------------------------------------------------------

fn_name CxxScoped_FindInheritedName = "CxxScoped.FindInheritedName";

CxxScoped* CxxScoped::FindInheritedName() const
{
   Debug::ft(CxxScoped_FindInheritedName);

   auto cls = GetClass();
   if(cls == nullptr) return nullptr;
   auto base = cls->BaseClass();
   if(base == nullptr) return nullptr;
   return base->FindName(*Name(), nullptr);
}

//------------------------------------------------------------------------------

CodeFile* CxxScoped::GetImplFile() const
{
   auto file = GetDefnFile();
   if(file != nullptr) return file;
   return GetDeclFile();
}

//------------------------------------------------------------------------------

size_t CxxScoped::GetRange(size_t& begin, size_t& end) const
{
   auto lexer = GetFile()->GetLexer();
   begin = GetPos();
   lexer.Reposition(begin);
   end = lexer.FindFirstOf(";");
   return string::npos;
}

//------------------------------------------------------------------------------

bool CxxScoped::IsAuto() const
{
   auto spec = GetTypeSpec();
   if(spec == nullptr) return false;
   return spec->IsAuto();
}

//------------------------------------------------------------------------------

bool CxxScoped::IsConst() const
{
   auto spec = GetTypeSpec();
   if(spec == nullptr) return false;
   return spec->IsConst();
}

//------------------------------------------------------------------------------

bool CxxScoped::IsConstPtr() const
{
   auto spec = GetTypeSpec();
   if(spec == nullptr) return false;
   return spec->IsConstPtr();
}

//------------------------------------------------------------------------------

bool CxxScoped::IsDeclaredInFunction() const
{
   auto type = scope_->Type();
   return ((type == Cxx::Block) || (type == Cxx::Function));
}

//------------------------------------------------------------------------------

bool CxxScoped::IsDefinedIn(const CxxArea* area) const
{
   for(auto s = GetScope(); s != nullptr; s = s->GetScope())
   {
      if(s == area) return true;
      if(s->Type() == Cxx::Namespace) return false;
   }

   return false;
}

//------------------------------------------------------------------------------

bool CxxScoped::IsIndirect() const
{
   auto spec = GetTypeSpec();
   if(spec == nullptr) return false;
   return spec->IsIndirect();
}

//------------------------------------------------------------------------------

fn_name CxxScoped_NameRefersToItem = "CxxScoped.NameRefersToItem";

bool CxxScoped::NameRefersToItem(const std::string& name,
   const CxxScope* scope, const CodeFile* file, SymbolView* view) const
{
   Debug::ft(CxxScoped_NameRefersToItem);

   //  If this item was not declared in a file, it's a built-in type and is
   //  therefore visible.  If NAME is a template specification, assume that
   //  it is visible.
   //c Verify the visibility of each component in a template specification.
   //  Not doing so forced Lexer::TypesTable to be renamed from TypeTable so
   //  that it could be distinguished from CxxSymbols::TypeTable, preventing
   //  a false "doubly declared identifier" error.
   //
   auto itemFile = GetFile();

   if((name.find('<') != string::npos) || (itemFile == nullptr))
   {
      view->accessibility = Unrestricted;
      return true;
   }

   //  The file that declares ITEM must affect (that is, be in the transitive
   //  #include of) this file.  The check can fail when looking up a namespace,
   //  which is arbitrarily assigned to the first file where it appears, even
   //  though it can appear in many others.
   //
   SetOfIds::const_iterator it = file->Affecters().find(itemFile->Fid());
   auto affected = (it != file->Affecters().cend());
   if(!affected && (Type() != Cxx::Namespace)) return false;

   //  See how SCOPE can access ITEM: this information is provided in VIEW.
   //  Set checkUsing if a using statement will be needed for ITEM if it is
   //  in another namespace.
   //
   auto checkUsing = true;
   AccessibilityTo(scope, view);

   switch(view->accessibility)
   {
   case Inaccessible:
      return false;

   case Restricted:
      if(file != itemFile) return false;
      break;

   case Inherited:
   case Declared:
      checkUsing = false;
   }

   //  To access ITEM, NAME must partially match its fully qualified name.
   //
   string fqName;
   size_t i = 0;

   while(GetScopedName(fqName, i))
   {
      auto pos = NameCouldReferTo(fqName, name);

      if(pos != string::npos)
      {
         switch(pos)
         {
         case 0:
         case 2:
            //
            //  NAME completely matches ITEM, with the possible exception of
            //  a leading scope resolution operator.
            //
            return true;
         case 1:
         case 3:
            //
            //  These shouldn't occur, because fqName has a "::" prefix.
            //
            Debug::SwErr(CxxScoped_NameRefersToItem, fqName, pos);
            return false;
         }

         //  NAME is a partial match for ITEM.  Report a match if SCOPE
         //  is ITEM's declarer or one of its subclasses.
         //
         if(!checkUsing) return true;

         //  Report a match if SCOPE is already in ITEM's scope.
         //
         fqName.erase(0, 2);
         auto prefix = fqName.substr(0, pos - 4);
         if(scope->IsSubscopeOf(prefix)) return true;

         //  Report a match if SCOPE's class derives from this item's class.
         //
         auto itemClass = Declarer();
         if(itemClass != nullptr)
         {
            auto usingClass = scope->GetClass();
            if((usingClass != nullptr) && usingClass->DerivesFrom(itemClass))
               return true;
         }

         //  Look for a using statement that matches at least the PREFIX
         //  of fqName.  That is, if fqName is "a::b::c::d" and PREFIX is
         //  "a::b", the using statement must be for "a::b", "a::b::c",
         //  or "a::b::c::d".
         //
         view->using_ = file->FindUsingFor(fqName, pos - 4, this, scope);  //*
         if(view->using_) return true;
      }

      ++i;
   }

   return false;
}

//------------------------------------------------------------------------------

fn_name CxxScoped_RecordAccess = "CxxScoped.RecordAccess";

void CxxScoped::RecordAccess(Cxx::Access access) const
{
   Debug::ft(CxxScoped_RecordAccess);

   if(access > access_)
   {
      auto expl = "Member should be inaccessible: " + ScopedName(true);
      Context::SwErr(CxxScoped_RecordAccess, expl, access);
   }

   if(access == Cxx::Public)
      public_ = true;
   else if(access == Cxx::Protected)
      protected_ = true;
}

//==============================================================================

fn_name Enum_ctor = "Enum.ctor";

Enum::Enum(string& name) : refs_(0)
{
   Debug::ft(Enum_ctor);

   std::swap(name_, name);
   if(!name_.empty()) Singleton< CxxSymbols >::Instance()->InsertEnum(this);
   CxxStats::Incr(CxxStats::ENUM_DECL);
}

//------------------------------------------------------------------------------

fn_name Enum_dtor = "Enum.dtor";

Enum::~Enum()
{
   Debug::ft(Enum_dtor);

   if(!name_.empty()) Singleton< CxxSymbols >::Instance()->EraseEnum(this);
   CxxStats::Decr(CxxStats::ENUM_DECL);
}

//------------------------------------------------------------------------------

fn_name Enum_AddEnumerator = "Enum.AddEnumerator";

void Enum::AddEnumerator(string& name, ExprPtr& init, size_t pos)
{
   Debug::ft(Enum_AddEnumerator);

   auto etor = EnumeratorPtr(new Enumerator(name, init, this));
   etor->SetPos(GetFile(), pos);
   etor->SetScope(GetScope());
   etor->SetAccess(GetAccess());
   etors_.push_back(std::move(etor));
}

//------------------------------------------------------------------------------

fn_name Enum_Check = "Enum.Check";

void Enum::Check() const
{
   Debug::ft(Enum_Check);

   if(name_.empty()) Log(AnonymousEnum);
   CheckIfUsed(EnumUnused);
   CheckIfHiding();
   CheckAccessControl();

   for(auto e = etors_.cbegin(); e != etors_.cend(); ++e)
   {
      (*e)->Check();
   }
}

//------------------------------------------------------------------------------

fn_name Enum_CheckAccessControl = "Enum.CheckAccessControl";

void Enum::CheckAccessControl() const
{
   Debug::ft(Enum_CheckAccessControl);

   //  Whether the access control can be further restricted depends on
   //  each of the enumerators as well as the enumeration type itself.
   //
   auto ctrl = GetAccess();
   auto max = BroadestAccessUsed();
   if(max >= ctrl) return;

   for(auto e = etors_.cbegin(); e != etors_.cend(); ++e)
   {
      auto used = (*e)->BroadestAccessUsed();
      if(used >= ctrl) return;
      if(used > max) max = used;
   }

   switch(max)
   {
   case Cxx::Private:
      Log(ItemCouldBePrivate);
      return;
   case Cxx::Protected:
      Log(ItemCouldBeProtected);
      return;
   }
}

//------------------------------------------------------------------------------

void Enum::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   auto anon = name_.empty();
   auto fq = options.test(DispFQ);
   stream << prefix;
   if(GetScope()->Type() == Cxx::Class) stream << GetAccess() << ": ";
   stream << ENUM_STR;
   if(!anon) stream << SPACE << (fq ? ScopedName(true) : name_);

   if(!options.test(DispCode))
   {
      std::ostringstream buff;
      buff << " // ";
      if(!anon && options.test(DispStats)) buff << "r=" << refs_ << SPACE;
      if(!fq) DisplayFiles(buff);
      auto str = buff.str();
      if(str.size() > 4) stream << str;
   }

   auto opts = options;
   stream << CRLF << prefix << '{' << CRLF;

   auto lead = prefix + spaces(Indent_Size);

   for(auto e = etors_.cbegin(); e != etors_.cend(); ++e)
   {
      if(*e == etors_.back()) opts.set(DispLast);
      (*e)->Display(stream, lead, opts);
      stream << CRLF;
   }

   stream << prefix << "};" << CRLF;
}

//------------------------------------------------------------------------------

fn_name Enum_EnterBlock = "Enum.EnterBlock";

void Enum::EnterBlock()
{
   Debug::ft(Enum_EnterBlock);

   Context::SetPos(GetPos());
   SetScope(Context::Scope());

   for(auto e = etors_.cbegin(); e != etors_.cend(); ++e)
   {
      (*e)->EnterBlock();
   }
}

//------------------------------------------------------------------------------

fn_name Enum_EnterScope = "Enum.EnterScope";

bool Enum::EnterScope()
{
   Debug::ft(Enum_EnterScope);

   if(AtFileScope()) GetFile()->InsertEnum(this);

   for(auto e = etors_.cbegin(); e != etors_.cend(); ++e)
   {
      (*e)->EnterScope();
   }

   return true;
}

//------------------------------------------------------------------------------

fn_name Enum_ExitBlock = "Enum.ExitBlock";

void Enum::ExitBlock()
{
   Debug::ft(Enum_ExitBlock);

   for(auto e = etors_.cbegin(); e != etors_.cend(); ++e)
   {
      (*e)->ExitBlock();
   }

   Singleton< CxxSymbols >::Instance()->EraseEnum(this);
}

//------------------------------------------------------------------------------

fn_name Enum_FindEnumerator = "Enum.FindEnumerator";

Enumerator* Enum::FindEnumerator(const string& name) const
{
   Debug::ft(Enum_FindEnumerator);

   for(auto e = etors_.cbegin(); e != etors_.cend(); ++e)
   {
      if(*(*e)->Name() == name) return e->get();
   }

   return nullptr;
}

//------------------------------------------------------------------------------

fn_name Enum_IsUnused = "Enum.IsUnused";

bool Enum::IsUnused() const
{
   Debug::ft(Enum_IsUnused);

   if(refs_ > 0) return false;

   for(auto e = etors_.cbegin(); e != etors_.cend(); ++e)
   {
      if(!(*e)->IsUnused()) return false;
   }

   return true;
}

//------------------------------------------------------------------------------

void Enum::Shrink()
{
   name_.shrink_to_fit();
   CxxStats::Strings(CxxStats::ENUM_DECL, name_.capacity());

   for(auto e = etors_.cbegin(); e != etors_.cend(); ++e)
   {
      (*e)->Shrink();
   }

   auto size = etors_.capacity() * sizeof(EnumeratorPtr);
   CxxStats::Vectors(CxxStats::ENUM_DECL, size);
}

//------------------------------------------------------------------------------

string Enum::TypeString(bool arg) const
{
   return Prefix(GetScope()->TypeString(arg)) + name_;
}

//==============================================================================

fn_name Enumerator_ctor = "Enumerator.ctor";

Enumerator::Enumerator(string& name, ExprPtr& init, const Enum* decl) :
   init_(init.release()),
   enum_(decl),
   refs_(0)
{
   Debug::ft(Enumerator_ctor);

   std::swap(name_, name);
   Singleton< CxxSymbols >::Instance()->InsertEtor(this);
   CxxStats::Incr(CxxStats::ENUM_MEM);
}

//------------------------------------------------------------------------------

fn_name Enumerator_dtor = "Enumerator.dtor";

Enumerator::~Enumerator()
{
   Debug::ft(Enumerator_dtor);

   Singleton< CxxSymbols >::Instance()->EraseEtor(this);
   CxxStats::Decr(CxxStats::ENUM_MEM);
}

//------------------------------------------------------------------------------

fn_name Enumerator_Check = "Enumerator.Check";

void Enumerator::Check() const
{
   Debug::ft(Enumerator_Check);

   CheckIfUsed(EnumeratorUnused);
   CheckIfHiding();
}

//------------------------------------------------------------------------------

void Enumerator::Display
   (std::ostream& stream, const std::string& prefix, const Flags& options) const
{
   stream << prefix << *Name();

   if(init_ != nullptr)
   {
      stream << " = ";
      init_->Print(stream, options);
   }

   if(!options.test(DispLast)) stream << ',';
   if(options.test(DispStats)) stream << " // r=" << refs_;
}

//------------------------------------------------------------------------------

fn_name Enumerator_EnterBlock = "Enumerator.EnterBlock";

void Enumerator::EnterBlock()
{
   Debug::ft(Enumerator_EnterBlock);

   Context::SetPos(GetPos());
   SetScope(Context::Scope());

   if(init_ != nullptr)
   {
      init_->EnterBlock();
      auto result = Context::PopArg(true);
      auto numeric = result.NumericType();

      if((numeric.Type() != Numeric::INT) && (numeric.Type() != Numeric::ENUM))
      {
         auto expl = "Non-numeric value for enumerator";
         Context::SwErr(Enumerator_EnterBlock, expl, numeric.Type());
      }
   }
}

//------------------------------------------------------------------------------

fn_name Enumerator_EnterScope = "Enumerator.EnterScope";

bool Enumerator::EnterScope()
{
   Debug::ft(Enumerator_EnterScope);

   if(init_ != nullptr) init_->EnterBlock();
   return true;
}

//------------------------------------------------------------------------------

fn_name Enumerator_ExitBlock = "Enumerator.ExitBlock";

void Enumerator::ExitBlock()
{
   Debug::ft(Enumerator_ExitBlock);

   Singleton< CxxSymbols >::Instance()->EraseEtor(this);
}

//------------------------------------------------------------------------------

fn_name Enumerator_GetScopedName = "Enumerator.GetScopedName";

bool Enumerator::GetScopedName(string& name, size_t n) const
{
   Debug::ft(Enumerator_GetScopedName);

   //  Start by providing the enumerator's fully qualified name, which includes
   //  that of its enum.  Then, unless the enum is anonymous, delete the enum's
   //  name from the fully qualified name, and provide that as an alternative.
   //
   if(n > 1) return false;
   CxxScoped::GetScopedName(name, 0);
   if(n == 0) return true;
   auto prev = *enum_->Name();
   if(prev.empty()) return false;
   prev += SCOPE_STR;
   auto pos = name.rfind(prev);
   name.erase(pos, prev.size());
   return true;
}

//------------------------------------------------------------------------------

void Enumerator::RecordAccess(Cxx::Access access) const
{
   CxxScoped::RecordAccess(access);
   enum_->RecordAccess(access);
}

//------------------------------------------------------------------------------

fn_name Enumerator_ScopedName = "Enumerator.ScopedName";

string Enumerator::ScopedName(bool templates) const
{
   Debug::ft(Enumerator_ScopedName);

   return Prefix(enum_->ScopedName(templates)) + *Name();
}

//------------------------------------------------------------------------------

void Enumerator::Shrink()
{
   name_.shrink_to_fit();
   CxxStats::Strings(CxxStats::ENUM_MEM, name_.capacity());
   if(init_ != nullptr) init_->Shrink();
}

//------------------------------------------------------------------------------

string Enumerator::TypeString(bool arg) const
{
   auto ts = enum_->TypeString(arg);
   if(!arg) ts += SCOPE_STR + *Name();
   return ts;
}

//==============================================================================

fn_name Forward_ctor = "Forward.ctor";

Forward::Forward(QualNamePtr& name, Cxx::ClassTag tag) :
   tag_(tag),
   name_(name.release()),
   users_(0)
{
   Debug::ft(Forward_ctor);

   Singleton< CxxSymbols >::Instance()->InsertForw(this);
   CxxStats::Incr(CxxStats::FORWARD_DECL);
}

//------------------------------------------------------------------------------

fn_name Forward_dtor = "Forward.dtor";

Forward::~Forward()
{
   Debug::ft(Forward_dtor);

   Singleton< CxxSymbols >::Instance()->EraseForw(this);
   CxxStats::Decr(CxxStats::FORWARD_DECL);
}

//------------------------------------------------------------------------------

CxxToken* Forward::AutoType() const
{
   auto ref = Referent();
   if(ref != nullptr) return ref;
   return (CxxToken*) this;
}

//------------------------------------------------------------------------------

fn_name Forward_Check = "Forward.Check";

void Forward::Check() const
{
   Debug::ft(Forward_Check);

   if(parms_ != nullptr) parms_->Check();

   if(Referent() == nullptr)
   {
      Log(ForwardUnresolved);
      return;
   }

   if(users_ == 0) Log(ForwardUnused);
}

//------------------------------------------------------------------------------

void Forward::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   auto fq = options.test(DispFQ);
   stream << prefix;

   if(!options.test(DispNoTP))
   {
      if(parms_ != nullptr) parms_->Print(stream, options);
   }

   stream << tag_ << SPACE;
   strName(stream, fq, name_.get());
   stream << ';';

   if(!options.test(DispCode))
   {
      stream << " // ";
      if(options.test(DispStats)) stream << "u=" << users_ << SPACE;
      DisplayReferent(stream, fq);
   }
   stream << CRLF;
}

//------------------------------------------------------------------------------

fn_name Forward_EnterBlock = "Forward.EnterBlock";

void Forward::EnterBlock()
{
   Debug::ft(Forward_EnterBlock);

   Context::SetPos(GetPos());
   Context::PushArg(StackArg(Referent(), 0));
}

//------------------------------------------------------------------------------

fn_name Forward_EnterScope = "Forward.EnterScope";

bool Forward::EnterScope()
{
   Debug::ft(Forward_EnterScope);

   if(AtFileScope()) GetFile()->InsertForw(this);
   return true;
}

//------------------------------------------------------------------------------

fn_name Forward_Referent = "Forward.Referent";

CxxNamed* Forward::Referent() const
{
   Debug::ft(Forward_Referent);

   auto ref = name_->GetReferent();
   if(ref != nullptr) return ref;

   auto name = QualifiedName(false, true);
   ref = GetArea()->FindClass(name);
   name_->SetReferent(ref, nullptr);
   return ref;
}

//------------------------------------------------------------------------------

fn_name Forward_ScopedName = "Forward.ScopedName";

string Forward::ScopedName(bool templates) const
{
   Debug::ft(Forward_ScopedName);

   auto ref = Referent();
   if(ref != nullptr) return ref->ScopedName(templates);
   return CxxNamed::ScopedName(templates);
}

//------------------------------------------------------------------------------

fn_name Forward_SetTemplateParms = "Forward.SetTemplateParms";

void Forward::SetTemplateParms(TemplateParmsPtr& parms)
{
   Debug::ft(Forward_SetTemplateParms);

   parms_ = std::move(parms);
}

//------------------------------------------------------------------------------

void Forward::Shrink()
{
   name_->Shrink();
   if(parms_ != nullptr) parms_->Shrink();
}

//------------------------------------------------------------------------------

string Forward::TypeString(bool arg) const
{
   auto ref = Referent();
   if(ref != nullptr) return ref->TypeString(arg);
   return Prefix(GetScope()->TypeString(arg)) + *Name();
}

//==============================================================================

size_t Friend::Depth_ = 0;

//------------------------------------------------------------------------------

fn_name Friend_ctor = "Friend.ctor";

Friend::Friend() :
   inline_(nullptr),
   tag_(Cxx::Typename),
   using_(false),
   searching_(false),
   searched_(false),
   users_(0)
{
   Debug::ft(Friend_ctor);

   CxxStats::Incr(CxxStats::FRIEND_DECL);
}

//------------------------------------------------------------------------------

fn_name Friend_dtor = "Friend.dtor";

Friend::~Friend()
{
   Debug::ft(Friend_dtor);

   if(GetFunction() == nullptr)
   {
      Singleton< CxxSymbols >::Instance()->EraseFriend(this);
   }

   CxxStats::Decr(CxxStats::FRIEND_DECL);
}

//------------------------------------------------------------------------------

fn_name Friend_AddUses = "Friend.AddUsers";

void Friend::AddUsers(const ClassInstPtrVector& instances)
{
   Debug::ft(Friend_AddUses);

   auto ref = Referent();

   if(ref != nullptr)
   {
      users_ = 0;

      for(auto t = instances.cbegin(); t != instances.cend(); ++t)
      {
         auto tf = (*t)->FindFriend(static_cast< const CxxScope* >(ref));
         if(tf != nullptr) users_ += tf->users_;
      }
   }
}

//------------------------------------------------------------------------------

CxxToken* Friend::AutoType() const
{
   auto ref = Referent();
   if(ref != nullptr) return ref;
   return (CxxToken*) this;
}

//------------------------------------------------------------------------------

fn_name Friend_Check = "Friend.Check";

void Friend::Check() const
{
   Debug::ft(Friend_Check);

   if(parms_ != nullptr) parms_->Check();

   //  Log an unknown friend.
   //
   auto ref = GetReferent();

   if(ref == nullptr)
   {
      Log(FriendUnresolved);
      return;
   }

   //  Log an unused friend declaration (that is, one that did not access an
   //  item that would otherwise have been inaccessible) unless the friend
   //  has no implementation (in which case it is probably external).
   //
   if((users_ == 0) && ref->IsImplemented()) Log(FriendUnused);
}

//------------------------------------------------------------------------------

void Friend::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   auto fq = options.test(DispFQ);
   stream << prefix;

   if(!options.test(DispNoTP))
   {
      if(parms_ != nullptr) parms_->Print(stream, options);
   }

   stream << FRIEND_STR << SPACE;

   auto func = GetFunction();

   if(func == nullptr)
   {
      if(tag_ != Cxx::Typename) stream << tag_ << SPACE;
      strName(stream, fq, name_.get());
   }
   else
   {
      func->DisplayDecl(stream, options);
   }

   stream << ';';

   if(!options.test(DispCode))
   {
      stream << " // ";
      if(options.test(DispStats)) stream << "u=" << users_ << SPACE;
      DisplayReferent(stream, fq);

      auto forw = name_->GetForward();
      if(forw != nullptr) stream << " via " << forw->GetFile()->Name();
   }

   stream << CRLF;
}

//------------------------------------------------------------------------------

fn_name Friend_EnterBlock = "Friend.EnterBlock";

void Friend::EnterBlock()
{
   Debug::ft(Friend_EnterBlock);

   Context::SetPos(GetPos());
   Context::PushArg(StackArg(Referent(), 0));
}

//------------------------------------------------------------------------------

fn_name Friend_EnterScope = "Friend.EnterScope";

bool Friend::EnterScope()
{
   Debug::ft(Friend_EnterScope);

   //  A friend declaration can also act as a forward declaration, so add it
   //  to the symbol table.  This was not done in the constructor because the
   //  friend's name was not yet known.  Look for what the friend refers to.
   //
   Context::SetPos(GetPos());
   Singleton< CxxSymbols >::Instance()->InsertFriend(this);
   FindReferent();
   return true;
}

//------------------------------------------------------------------------------

fn_name Friend_FindForward = "Friend.FindForward";

CxxNamed* Friend::FindForward() const
{
   Debug::ft(Friend_FindForward);

   //  This is similar to ResolveName, except that the name's scope must be
   //  known.  On the other hand, its definition need not be visible in the
   //  scope where it appeared.
   //
   CxxScoped* item = GetScope();
   auto func = GetFunction();
   auto qname = GetQualName();
   auto size = qname->Size();
   string name = *qname->First()->Name();
   size_t idx = (*item->Name() == name ? 1 : 0);
   Namespace* space;
   Class* cls;

   while(item != nullptr)
   {
      auto type = item->Type();

      switch(type)
      {
      case Cxx::Function:
         return item;

      case Cxx::Namespace:
         //
         //  If there is another name, resolve it within this namespace, else
         //  return the namespace itself.
         //
         if(idx >= size) return item;
         space = static_cast< Namespace* >(item);
         name = *qname->At(idx)->Name();
         item = nullptr;
         if(++idx >= size)
         {
            if(func != nullptr) item = space->MatchFunc(func, false);
         }
         if(item == nullptr) item = space->FindItem(name);
         qname->SetReferentN(idx - 1, item, nullptr);
         if(item == nullptr) return nullptr;
         break;

      case Cxx::Class:
         cls = static_cast< Class* >(item);

         do
         {
            //  If this class was found through name resolution, see if it has
            //  template arguments before looking up the next name: if it does,
            //  create (but do not instantiate) the template instance.
            //
            if(idx == 0) break;
            if(cls->IsInTemplateInstance()) break;
            auto args = qname->At(idx - 1)->GetTemplateArgs();
            if(args == nullptr) break;
            if(!ResolveTemplate(cls, args, (idx >= size))) break;
            cls = cls->EnsureInstance(args);
            item = cls;
            qname->SetReferentN(idx - 1, item, nullptr);  // updated value
            if(item == nullptr) return nullptr;
         }
         while(false);

         //  Resolve the next name within CLS.  This is similar to the above,
         //  when TYPE is a namespace.
         //
         if(idx >= size) return item;
         name = *qname->At(idx)->Name();
         item = nullptr;
         if(++idx >= size)
         {
            if(func != nullptr) item = cls->MatchFunc(func, true);
         }
         if(item == nullptr) item = cls->FindMember(name, true);
         qname->SetReferentN(idx - 1, item, nullptr);
         if(item == nullptr) return nullptr;
         break;

      case Cxx::Typedef:
         {
            //  See if the item wants to resolve the typedef.
            //
            auto tdef = static_cast< Typedef* >(item);
            tdef->SetAsReferent(this);
            if(!ResolveTypedef(tdef, idx - 1)) return tdef;
            auto root = tdef->Root();
            if(root == nullptr) return tdef;
            item = static_cast< CxxScoped* >(root);
            qname->SetReferentN(idx - 1, item, nullptr);  // updated value
         }
         break;

      default:
         auto expl = name + " is an invalid friend";
         Context::SwErr(Friend_FindForward, expl, type);
         return nullptr;
      }
   }

   return item;
}

//------------------------------------------------------------------------------

fn_name Friend_FindReferent = "Friend.FindReferent";

void Friend::FindReferent()
{
   Debug::ft(Friend_FindReferent);

   //  The following prevents a stack overflow.  The declaration itself can be
   //  found as a candidate when ResolveName is invoked.  To find what it refers
   //  to, it is asked for its scoped name, which looks for its referent, which
   //  causes this function to be invoked recursively.  A depth limit of 2 is
   //  also enforced over all friend declarations.  This allows another friend
   //  declaration to find its referent and provide its resolution to this one.
   //  However, it prevents futile nesting in which declarations ask each other
   //  for a referent that is still only a forward declaration.
   //    Even if the referent is not found, SetReferent(nullptr) must be invoked
   //  to reset searching_ and Depth_, which reenables this function.
   //
   if(searching_ || (Depth_ > 1)) return;
   searching_ = true;
   ++Depth_;

   SymbolView view = DeclaredGlobally;
   auto scope = GetScope();
   auto mask = (GetFunction() != nullptr ? FRIEND_FUNCS : FRIEND_CLASSES);
   CxxNamed* ref = nullptr;

   if(!searched_)
   {
      //  This is the initial search for the friend's referent, so the scope
      //  is the class where the friend declaration appeared.  Search for the
      //  friend from that scope, but update the scope to the namespace where
      //  the class is defined, because that will be the friend's scope if it
      //  has not yet been declared.
      //
      searched_ = true;
      SetScope(scope->GetSpace());
      ref = ResolveName(GetFile(), scope, mask, &view);  //*
      if(ref != nullptr) using_ = view.using_;
   }

   //  Keep searching for the friend if the initial search failed or previous
   //  searches have only returned an unresolved forward declaration.
   //
   auto forw = name_->GetForward();
   if((ref == nullptr) || (ref == this) || (ref == forw)) ref = FindForward();
   SetReferent(ref, nullptr);
}

//------------------------------------------------------------------------------

Function* Friend::GetFunction() const
{
   if(func_ != nullptr) return func_.get();
   if(inline_ != nullptr) return inline_;
   return nullptr;
}

//------------------------------------------------------------------------------

QualName* Friend::GetQualName() const
{
   auto func = GetFunction();
   if(func != nullptr) return func->GetQualName();
   return name_.get();
}

//------------------------------------------------------------------------------

CxxNamed* Friend::GetReferent() const
{
   return GetQualName()->GetReferent();
}

//------------------------------------------------------------------------------

fn_name Friend_GetUsages = "Friend.GetUsages";

void Friend::GetUsages(const CodeFile& file, CxxUsageSets& symbols) const
{
   Debug::ft(Friend_GetUsages);

   auto ref = Referent();
   if(ref == nullptr) return;

   //  If the friend is a class template or class template instance, it must
   //  be visible as a forward declaration, although the friend declaration
   //  itself could have doubled as that forward declaration.
   //
   auto tmplt = ((ref->IsTemplate()) || (ref->GetTemplateArgs() != nullptr));
   auto forw = name_->GetForward();
   auto type = (forw != nullptr ? forw->Type() : ref->Type());

   switch(type)
   {
   case Cxx::Forward:
   case Cxx::Friend:
      if(tmplt) symbols.AddForward(forw);
      break;

   case Cxx::Class:
      {
         // o The outer class for an inner one must be directly visible.
         // o The class template for a class template instance should (must,
         //   if in another namespace) be declared forward.
         //
         auto outer = ref->Declarer();

         if(outer != nullptr)
         {
            if(outer->GetTemplate() != nullptr)
               outer = outer->GetClassTemplate();
            symbols.AddDirect(outer);
         }
         else if(ref->GetTemplateArgs() != nullptr)
         {
            symbols.AddIndirect(ref->GetTemplate());
         }
      }
      break;

   case Cxx::Function:
      //
      //  o An inline friend has no visibility requirements.
      //  o A function's class must be directly visible.
      //  o A function outside a class should be declared forward.
      //
      if(inline_ != nullptr) break;

      if(ref->GetClass() != nullptr)
         symbols.AddDirect(ref->GetClass());
      else
         symbols.AddIndirect(ref);
      break;
   }

   //  Indicate whether our referent was made visible by a using statement.
   //
   if(using_) symbols.AddUser(this);
}

//------------------------------------------------------------------------------

const string* Friend::Name() const
{
   auto func = GetFunction();
   if(func != nullptr) return func->Name();
   return name_->Name();
}

//------------------------------------------------------------------------------

string Friend::QualifiedName(bool scopes, bool templates) const
{
   auto func = GetFunction();
   if(func != nullptr) return func->QualifiedName(scopes, templates);
   return name_->QualifiedName(scopes, templates);
}

//------------------------------------------------------------------------------

fn_name Friend_Referent = "Friend.Referent";

CxxNamed* Friend::Referent() const
{
   Debug::ft(Friend_Referent);

   auto ref = GetReferent();
   if(ref != nullptr) return ref;
   const_cast< Friend* >(this)->FindReferent();
   return GetReferent();
}

//------------------------------------------------------------------------------

fn_name Friend_ResolveForward = "Friend.ResolveForward";

bool Friend::ResolveForward(CxxScoped* decl, size_t n) const
{
   Debug::ft(Friend_ResolveForward);

   //  A forward declaration for the friend was found.  Unless it is
   //  the friend declaration itself, save it, along with its scope,
   //  and continue resolving the name.
   //
   if(decl == this) return false;
   name_->At(n)->SetForward(decl);
   decl->SetAsReferent(this);
   const_cast< Friend* >(this)->SetScope(decl->GetSpace());
   return true;
}

//------------------------------------------------------------------------------

fn_name Friend_ResolveTemplate = "Friend.ResolveTemplate";

bool Friend::ResolveTemplate(Class* cls, const TypeName* args, bool end) const
{
   Debug::ft(Friend_ResolveTemplate);

   const_cast< Friend* >(this)->SetScope(cls->GetScope());
   return true;
}

//------------------------------------------------------------------------------

fn_name Friend_ScopedName = "Friend.ScopedName";

string Friend::ScopedName(bool templates) const
{
   Debug::ft(Friend_ScopedName);

   auto ref = Referent();
   if(ref != nullptr) return ref->ScopedName(templates);
   return CxxNamed::ScopedName(templates);
}

//------------------------------------------------------------------------------

fn_name Friend_SetAsReferent = "Friend.SetAsReferent";

void Friend::SetAsReferent(const CxxNamed* user)
{
   Debug::ft(Friend_SetAsReferent);

   //  Don't log this for another friend or forward declaration.
   //
   switch(user->Type())
   {
   case Cxx::Forward:
   case Cxx::Friend:
      return;
   }

   user->Log(FriendAsForward);
}

//------------------------------------------------------------------------------

fn_name Friend_SetFunc = "Friend.SetFunc";

void Friend::SetFunc(FunctionPtr& func)
{
   Debug::ft(Friend_SetFunc);

   func->CloseScope();

   if(func->GetBracePos() != string::npos)
   {
      //  This is a friend definition (an inline friend function).  Such
      //  a function belongs to the same scope that defined the class in
      //  which the friend appeared.
      //
      auto cls = func->GetClass();
      auto scope = cls->GetScope();
      SetScope(scope);
      inline_ = func.get();
      inline_->SetTemplateParms(parms_);
      inline_->SetFriend();
      static_cast< CxxArea* >(scope)->AddFunc(func);
      GetQualName()->SetReferent(inline_, nullptr);
      inline_->SetAsReferent(this);
   }
   else
   {
      func_ = std::move(func);
   }
}

//------------------------------------------------------------------------------

fn_name Friend_SetName = "Friend.SetName";

void Friend::SetName(QualNamePtr& name)
{
   Debug::ft(Friend_SetName);

   name_ = std::move(name);
}

//------------------------------------------------------------------------------

fn_name Friend_SetReferent = "Friend.SetReferent";

void Friend::SetReferent(CxxNamed* item, const SymbolView* view) const
{
   Debug::ft(Friend_SetReferent);

   searching_ = false;
   --Depth_;

   if(item == nullptr) return;

   auto type = item->Type();

   switch(type)
   {
   case Cxx::Class:
   case Cxx::Function:
      break;
   default:
      auto expl = item->ScopedName(true) + " is an invalid friend";
      Context::SwErr(Friend_SetReferent, expl, type);
      return;
   }

   item->SetAsReferent(this);
   GetQualName()->SetReferent(item, view);
}

//------------------------------------------------------------------------------

fn_name Friend_SetTemplateParms = "Friend.SetTemplateParms";

void Friend::SetTemplateParms(TemplateParmsPtr& parms)
{
   Debug::ft(Friend_SetTemplateParms);

   parms_ = std::move(parms);
}

//------------------------------------------------------------------------------

void Friend::Shrink()
{
   if(name_ != nullptr) name_->Shrink();
   if(parms_ != nullptr) parms_->Shrink();
   if(func_ != nullptr) func_->Shrink();
}

//------------------------------------------------------------------------------

string Friend::TypeString(bool arg) const
{
   auto ref = Referent();
   if(ref != nullptr) return ref->TypeString(arg);

   auto func = GetFunction();
   if(func != nullptr) return func->TypeString(arg);
   return Prefix(GetScope()->TypeString(arg)) + QualifiedName(false, true);
}

//==============================================================================

fn_name Terminal_ctor = "Terminal.ctor";

Terminal::Terminal(const string& name, const string& type) :
   name_(name),
   type_(type.empty() ? name : type),
   attrs_(Numeric::Nil)
{
   Debug::ft(Terminal_ctor);

   SetScope(Singleton< CxxRoot >::Instance()->GlobalNamespace());
   Singleton< CxxSymbols >::Instance()->InsertTerm(this);
   CxxStats::Incr(CxxStats::TERMINAL_DECL);
}

//------------------------------------------------------------------------------

fn_name Terminal_dtor = "Terminal.dtor";

Terminal::~Terminal()
{
   Debug::ft(Terminal_dtor);

   Singleton< CxxSymbols >::Instance()->EraseTerm(this);
   CxxStats::Decr(CxxStats::TERMINAL_DECL);
}

//------------------------------------------------------------------------------

void Terminal::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   stream << prefix << "terminal " << *Name();
   stream << ';';

   if(!options.test(DispFQ))
   {
      stream << " // ";
      DisplayFiles(stream);
   }

   stream << CRLF;
}

//------------------------------------------------------------------------------

fn_name Terminal_EnterBlock = "Terminal.EnterBlock";

void Terminal::EnterBlock()
{
   Debug::ft(Terminal_EnterBlock);

   Context::PushArg(StackArg(this, 0));
}

//------------------------------------------------------------------------------

fn_name Terminal_IsAuto = "Terminal.IsAuto";

bool Terminal::IsAuto() const
{
   Debug::ft(Terminal_IsAuto);

   return (*Name() == AUTO_STR);
}

//------------------------------------------------------------------------------

void Terminal::Shrink()
{
   name_.shrink_to_fit();
   type_.shrink_to_fit();
   CxxStats::Strings(CxxStats::TERMINAL_DECL, name_.capacity());
   CxxStats::Strings(CxxStats::TERMINAL_DECL, type_.capacity());
}

//==============================================================================

fn_name Typedef_ctor = "Typedef.ctor";

Typedef::Typedef(string& name, TypeSpecPtr& spec) :
   spec_(spec.release()),
   refs_(0)
{
   Debug::ft(Typedef_ctor);

   std::swap(name_, name);
   spec_->SetUserType(Cxx::Typedef);
   Singleton< CxxSymbols >::Instance()->InsertType(this);
   CxxStats::Incr(CxxStats::TYPE_DECL);
}

//------------------------------------------------------------------------------

fn_name Typedef_dtor = "Typedef.dtor";

Typedef::~Typedef()
{
   Debug::ft(Typedef_dtor);

   Singleton< CxxSymbols >::Instance()->EraseType(this);
   CxxStats::Decr(CxxStats::TYPE_DECL);
}

//------------------------------------------------------------------------------

fn_name Typedef_Check = "Typedef.Check";

void Typedef::Check() const
{
   Debug::ft(Typedef_Check);

   spec_->Check();
   CheckIfUsed(TypedefUnused);
   CheckIfHiding();
   CheckAccessControl();
   CheckPointerType();
}

//------------------------------------------------------------------------------

fn_name Typedef_CheckPointerType = "Typedef.CheckPointerType";

void Typedef::CheckPointerType() const
{
   Debug::ft(Typedef_CheckPointerType);

   if(spec_->Ptrs(false) > 0) Log(PointerTypedef);
}

//------------------------------------------------------------------------------

void Typedef::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   if(IsDeclaredInFunction())
   {
      stream << prefix;
      Print(stream, options);
      stream << CRLF;
      return;
   }

   auto fq = options.test(DispFQ);
   stream << prefix;
   if(GetScope()->Type() == Cxx::Class) stream << GetAccess() << ": ";
   stream << TYPEDEF_STR << SPACE;
   spec_->Print(stream, options);
   if(spec_->GetFuncSpec() == nullptr)
   {
      stream << SPACE << (fq ? ScopedName(true) : *Name());
   }
   spec_->DisplayArrays(stream);
   stream << ';';

   if(!options.test(DispCode))
   {
      std::ostringstream buff;
      buff << " // ";
      if(options.test(DispStats)) buff << "r=" << refs_ << SPACE;
      if(!fq) DisplayFiles(buff);
      auto str = buff.str();
      if(str.size() > 4) stream << str;
   }

   stream << CRLF;
}

//------------------------------------------------------------------------------

fn_name Typedef_EnterBlock = "Typedef.EnterBlock";

void Typedef::EnterBlock()
{
   Debug::ft(Typedef_EnterBlock);

   Context::SetPos(GetPos());
   SetScope(Context::Scope());
   spec_->EnteringScope(GetScope());
   refs_ = 0;
}

//------------------------------------------------------------------------------

fn_name Typedef_EnterScope = "Typedef.EnterScope";

bool Typedef::EnterScope()
{
   Debug::ft(Typedef_EnterScope);

   if(AtFileScope()) GetFile()->InsertType(this);
   Context::Enter(this);
   spec_->EnteringScope(GetScope());
   refs_ = 0;
   return true;
}

//------------------------------------------------------------------------------

fn_name Typedef_ExitBlock = "Typedef.ExitBlock";

void Typedef::ExitBlock()
{
   Debug::ft(Typedef_ExitBlock);

   Singleton< CxxSymbols >::Instance()->EraseType(this);
}

//------------------------------------------------------------------------------

TypeName* Typedef::GetTemplateArgs() const
{
   return spec_->GetTemplateArgs();
}

//------------------------------------------------------------------------------

fn_name Typedef_GetUsages = "Typedef.GetUsages";

void Typedef::GetUsages(const CodeFile& file, CxxUsageSets& symbols) const
{
   Debug::ft(Typedef_GetUsages);

   return spec_->GetUsages(file, symbols);
}

//------------------------------------------------------------------------------

void Typedef::Print(ostream& stream, const Flags& options) const
{
   stream << TYPEDEF_STR << SPACE;

   spec_->Print(stream, options);
   if(spec_->GetFuncSpec() == nullptr) stream << SPACE << *Name();
   spec_->DisplayArrays(stream);
   stream << ';';
}

//------------------------------------------------------------------------------

fn_name Typedef_Referent = "Typedef.Referent";

CxxNamed* Typedef::Referent() const
{
   Debug::ft(Typedef_Referent);

   return spec_->Referent();
}

//------------------------------------------------------------------------------

void Typedef::Shrink()
{
   name_.shrink_to_fit();
   spec_->Shrink();
   CxxStats::Strings(CxxStats::TYPE_DECL, name_.capacity());
}

//------------------------------------------------------------------------------

string Typedef::TypeString(bool arg) const
{
   return spec_->TypeString(arg);
}

//==============================================================================

fn_name Using_ctor = "Using.ctor";

Using::Using(QualNamePtr& name, bool space, bool added) :
   name_(name.release()),
   users_(0),
   added_(added),
   remove_(false),
   space_(space)
{
   Debug::ft(Using_ctor);

   CxxStats::Incr(CxxStats::USING_DECL);
}

//------------------------------------------------------------------------------

fn_name Using_Check = "Using.Check";

void Using::Check() const
{
   Debug::ft(Using_Check);

   if(added_) return;
   if(users_ == 0) Log(UsingUnused);

   //  A using statement should be avoided in a header except to import
   //  items from a base class.
   //
   if(GetFile()->IsHeader() && (GetScope()->Type() != Cxx::Class))
   {
      Log(UsingInHeader);
   }
}

//------------------------------------------------------------------------------

void Using::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   if(added_) return;

   auto fq = options.test(DispFQ);
   stream << prefix << USING_STR << SPACE;
   if(space_) stream << NAMESPACE_STR << SPACE;
   strName(stream, fq, name_.get());
   stream << ';';

   if(!options.test(DispCode))
   {
      stream << " // ";
      if(options.test(DispStats)) stream << "u=" << users_ << SPACE;
      DisplayReferent(stream, fq);
   }

   stream << CRLF;
}

//------------------------------------------------------------------------------

fn_name Using_EnterBlock = "Using.EnterBlock";

void Using::EnterBlock()
{
   Debug::ft(Using_EnterBlock);

   Context::SetPos(GetPos());
   SetScope(Context::Scope());
   Block::AddUsing(this);
   FindReferent();
}

//------------------------------------------------------------------------------

fn_name Using_EnterScope = "Using.EnterScope";

bool Using::EnterScope()
{
   Debug::ft(Using_EnterScope);

   if(AtFileScope()) GetFile()->InsertUsing(this);
   Context::SetPos(GetPos());
   FindReferent();
   return true;
}

//------------------------------------------------------------------------------

fn_name Using_ExitBlock = "Using.ExitBlock";

void Using::ExitBlock()
{
   Debug::ft(Using_ExitBlock);

   Block::RemoveUsing(this);
}

//------------------------------------------------------------------------------

fn_name Using_FindReferent = "Using.FindReferent";

void Using::FindReferent()
{
   Debug::ft(Using_FindReferent);

   //  If the symbol table doesn't know what this using statement refers to,
   //  log it.  Template arguments are not supported in a using statement.
   //
   if(Referent() != nullptr) return;

   auto qname = QualifiedName(true, false);
   auto log = "Unknown using: " + qname + " [" + strLocation() + ']';
   Debug::SwErr(Using_FindReferent, log, 0, InfoLog);
}

//------------------------------------------------------------------------------

fn_name Using_IsUsingFor = "Using.IsUsingFor";

bool Using::IsUsingFor(const string& name, size_t prefix) const
{
   Debug::ft(Using_IsUsingFor);

   //  Template arguments are not supported in a using statement.
   //
   auto ref = Referent();
   if(ref == nullptr) return false;

   auto refname = ref->ScopedName(false);
   auto pos = NameIsSuperscopeOf(name, refname);

   if((pos != string::npos) && (pos >= prefix))
   {
      //  If there is no context file, a parse is not in progress, in which
      //  case a tool is invoking this after the parse was completed.
      //
      auto file = Context::File();
      if(file != nullptr) ++users_;
      return true;
   }

   return false;
}

//------------------------------------------------------------------------------

fn_name Using_Referent = "Using.Referent";

CxxNamed* Using::Referent() const
{
   Debug::ft(Using_Referent);

   auto ref = name_->GetReferent();
   if(ref != nullptr) return ref;

   SymbolView view;
   return ResolveName(GetFile(), GetScope(), USING_REFS, &view);  //*
}

//------------------------------------------------------------------------------

fn_name Using_ScopedName = "Using.ScopedName";

string Using::ScopedName(bool templates) const
{
   Debug::ft(Using_ScopedName);

   auto ref = Referent();
   if(ref != nullptr) return ref->ScopedName(templates);

   auto expl = "using " + QualifiedName(true, false) + ": symbol not found";
   Context::SwErr(Using_ScopedName, expl, 0);
   return ERROR_STR;
}

//------------------------------------------------------------------------------

fn_name Using_SetScope = "Using.SetScope";

void Using::SetScope(CxxScope* scope)
{
   Debug::ft(Using_SetScope);

   //  If a using statement appears in a class, the class is not part
   //  of what it refers to, so step out to the class's namespace.
   //
   if(scope->Type() == Cxx::Class) scope = scope->GetSpace();
   CxxScoped::SetScope(scope);
}
}
