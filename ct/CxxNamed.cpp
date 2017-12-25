//==============================================================================
//
//  CxxNamed.cpp
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
#include "CxxNamed.h"
#include <set>
#include <sstream>
#include <utility>
#include "CodeFile.h"
#include "CodeSet.h"
#include "CxxArea.h"
#include "CxxExecute.h"
#include "CxxRoot.h"
#include "CxxScope.h"
#include "CxxScoped.h"
#include "CxxSymbols.h"
#include "Debug.h"
#include "Formatters.h"
#include "Library.h"
#include "Parser.h"
#include "Registry.h"
#include "Singleton.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace CodeTools
{
fn_name CxxNamed_ctor1 = "CxxNamed.ctor";

CxxNamed::CxxNamed()
{
   Debug::ft(CxxNamed_ctor1);
}

//------------------------------------------------------------------------------

fn_name CxxNamed_ctor2 = "CxxNamed.ctor(copy)";

CxxNamed::CxxNamed(const CxxNamed& that)
{
   Debug::ft(CxxNamed_ctor2);

   this->loc_ = that.loc_;
}

//------------------------------------------------------------------------------

fn_name CxxNamed_dtor = "CxxNamed.dtor";

CxxNamed::~CxxNamed()
{
   Debug::ft(CxxNamed_dtor);
}

//------------------------------------------------------------------------------

fn_name CxxNamed_Accessed = "CxxNamed.Accessed";

void CxxNamed::Accessed() const
{
   Debug::ft(CxxNamed_Accessed);

   auto func = Context::Scope()->GetFunction();
   if(func == nullptr) return;
   func->ItemAccessed(this);
}

//------------------------------------------------------------------------------

fn_name CxxNamed_AddUsage = "CxxNamed.AddUsage";

void CxxNamed::AddUsage() const
{
   Debug::ft(CxxNamed_AddUsage);

   if(Context::ParsingTemplateInstance()) return;
   if(IsInTemplateInstance()) return;
   auto file = Context::File();
   if(file == nullptr) return;
   file->AddUsage(this);
}

//------------------------------------------------------------------------------

fn_name CxxNamed_AtFileScope = "CxxNamed.AtFileScope";

bool CxxNamed::AtFileScope() const
{
   Debug::ft(CxxNamed_AtFileScope);

   auto scope = GetScope();
   if(scope == nullptr) return false;
   return (scope->Type() == Cxx::Namespace);
}

//------------------------------------------------------------------------------

void CxxNamed::DisplayReferent(ostream& stream, bool fq) const
{
   auto ref = Referent();

   if(ref == nullptr)
   {
      stream << "null referent";
   }
   else
   {
      stream << ref->GetFile()->Name();
      if(!fq) stream << ": " << *ref->Name();
   }
}

//------------------------------------------------------------------------------

fn_name CxxNamed_FindReferent = "CxxNamed.FindReferent";

bool CxxNamed::FindReferent()
{
   Debug::ft(CxxNamed_FindReferent);

   auto expl = "FindReferent() not implemented by " + strClass(this, false);
   Context::SwErr(CxxNamed_FindReferent, expl, 0);
   return false;
}

//------------------------------------------------------------------------------

CxxArea* CxxNamed::GetArea() const
{
   auto item = GetScope();
   if(item == nullptr) return nullptr;
   return item->GetArea();
}

//------------------------------------------------------------------------------

Class* CxxNamed::GetClass() const
{
   auto item = GetScope();
   if(item == nullptr) return nullptr;
   return item->GetClass();
}

//------------------------------------------------------------------------------

id_t CxxNamed::GetDeclFid() const
{
   auto file = GetDeclFile();
   if(file == nullptr) return NIL_ID;
   return file->Fid();
}

//------------------------------------------------------------------------------

bool CxxNamed::GetScopedName(string& name, size_t n) const
{
   if(n != 0) return false;
   name = SCOPE_STR + ScopedName(false);
   return true;
}

//------------------------------------------------------------------------------

Namespace* CxxNamed::GetSpace() const
{
   auto item = GetScope();
   if(item == nullptr) return nullptr;
   return item->GetSpace();
}

//------------------------------------------------------------------------------

const TemplateParms* CxxNamed::GetTemplateParms() const
{
   auto qname = GetQualName();
   if(qname == nullptr) return nullptr;
   return qname->GetTemplateParms();
}

//------------------------------------------------------------------------------

bool CxxNamed::IsInTemplateInstance() const
{
   return GetScope()->IsInTemplateInstance();
}

//------------------------------------------------------------------------------

fn_name CxxNamed_IsPreviousDeclOf = "CxxNamed.IsPreviousDeclOf";

bool CxxNamed::IsPreviousDeclOf(const CxxNamed* item) const
{
   Debug::ft(CxxNamed_IsPreviousDeclOf);

   //  ITEM and "this" are already known to have the same name.  If "this"
   //  existed before ITEM, ITEM must be another object.  And for them to
   //  refer to the same entity, the file that declares "this" must be in
   //  the transitive #include of ITEM.
   //
   if((item == this) || (item == nullptr)) return false;

   auto file1 = this->GetFile();
   auto file2 = item->GetFile();
   auto& files = Singleton< Library >::Instance()->Files();
   auto& affecters = files.At(file2->Fid())->Affecters();
   SetOfIds::const_iterator it = affecters.find(file1->Fid());
   return (it != affecters.cend());
}

//------------------------------------------------------------------------------

fn_name CxxNamed_Log = "CxxNamed.Log";

void CxxNamed::Log(Warning warning, size_t offset) const
{
   Debug::ft(CxxNamed_Log);

   //  Do not log unused items in class templates or their instances.  The
   //  exception is a friend: although it may appear in such a class, it is
   //  declared in another.
   //
   if(IsUnusedItemWarning(warning) && (Type() != Cxx::Friend))
   {
      auto cls = GetClass();

      if(cls != nullptr)
      {
         if(cls->IsTemplate()) return;
         if(cls->IsInTemplateInstance()) return;
      }
   }

   GetFile()->LogPos(GetPos(), warning, offset);
}

//------------------------------------------------------------------------------

fn_name CxxNamed_MemberToArg = "CxxNamed.MemberToArg";

StackArg CxxNamed::MemberToArg(StackArg& via, Cxx::Operator op)
{
   Debug::ft(CxxNamed_MemberToArg);

   //  This should only be invoked on ClassData.
   //
   auto expl = "Unexpected member selection by " + *via.item->Name();
   Context::SwErr(CxxNamed_MemberToArg, expl, op);

   return NameToArg(op);
}

//------------------------------------------------------------------------------

fn_name CxxNamed_NameToArg = "CxxNamed.NameToArg";

StackArg CxxNamed::NameToArg(Cxx::Operator op)
{
   Debug::ft(CxxNamed_NameToArg);

   Accessed();
   return StackArg(this, 0);
}

//------------------------------------------------------------------------------

fn_name CxxNamed_ResolveLocal = "CxxNamed.ResolveLocal";

CxxNamed* CxxNamed::ResolveLocal(SymbolView* view) const
{
   Debug::ft(CxxNamed_ResolveLocal);

   auto syms = Singleton< CxxSymbols >::Instance();
   auto qname = GetQualName();

   if((qname->Names_size() == 1) && !qname->IsGlobal())
   {
      auto item = syms->FindLocal(*Name(), view);
      if(item != nullptr) return item;
   }

   return ResolveName(Context::File(), Context::Scope(), CODE_REFS, view);
}

//------------------------------------------------------------------------------

fn_name CxxNamed_ResolveName = "CxxNamed.ResolveName";

CxxNamed* CxxNamed::ResolveName(const CodeFile* file,
   const CxxScope* scope, const Flags& mask, SymbolView* view) const
{
   Debug::ft(CxxNamed_ResolveName);

   CxxScoped* item;
   string name;
   Namespace* space;
   Class* cls;
   auto func = GetFunction();
   auto qname = GetQualName();
   auto size = qname->Names_size();
   auto syms = Singleton< CxxSymbols >::Instance();
   auto selector = (size == 1 ? mask : SCOPE_REFS);

   size_t idx = (qname->IsGlobal() ? 0 : 1);

   if(idx == 0)
   {
      //  The name is prefixed by "::", so begin the search in the
      //  global namespace, starting with the first name.
      //
      *view = DeclaredGlobally;
      item = Singleton< CxxRoot >::Instance()->GlobalNamespace();
   }
   else
   {
      //  Start with the first name in the qualified name.  Return
      //  if it cannot be resolved or if the item refers to itself,
      //  which can occur for a friend declaration.
      //
      name = *qname->Names_at(0)->Name();
      item = syms->FindSymbol(file, scope, name, selector, view);
      if((item == nullptr) || (item == this)) return item;
   }

   //  Continue with the name at IDX.
   //
   while(item != nullptr)
   {
      auto type = item->Type();

      switch(type)
      {
      case Cxx::Terminal:
      case Cxx::Function:
      case Cxx::Data:
      case Cxx::Enumerator:
      case Cxx::Macro:
         return item;

      case Cxx::Namespace:
         //
         //  If there is another name, resolve it within this namespace,
         //  else return the namespace itself.
         //
         if(idx >= size) return item;
         space = static_cast< Namespace* >(item);
         if(!name.empty()) name += SCOPE_STR;
         name += *qname->Names_at(idx)->Name();
         item = nullptr;
         if(++idx >= size)
         {
            selector = mask;
            if(func != nullptr) item = space->MatchFunc(func, false);
         }
         if(item == nullptr)
            item = syms->FindSymbol(file, scope, name, selector, view, space);
         if(item == nullptr) return nullptr;
         break;

      case Cxx::Class:
         cls = static_cast< Class* >(item);

         do
         {
            //  Before looking up the next name, see if this class has template
            //  arguments.  If so, create the template instance, and instantiate
            //  it if another name (one of its members) follows.  Don't apply
            //  template arguments, however, when parsing a template or template
            //  instance.
            //
            if(cls->IsInTemplateInstance()) break;
            auto curr = qname->Names_at(idx - 1);
            auto args = curr->GetTemplateArgs();
            if(args == nullptr) break;
            if(args->HasTemplateParmFor(scope)) break;
            if(!ResolveTemplate(cls, args, (idx >= size))) break;
            cls = cls->EnsureInstance(args);
            if(cls == nullptr) return nullptr;
            if(idx < size) cls->Instantiate();
            item = cls;
         }
         while(false);

         //  Resolve the next name within CLS.  This is similar to the above,
         //  when TYPE is a namespace.
         //
         if(idx >= size) return item;
         name = *qname->Names_at(idx)->Name();
         item = nullptr;
         if(++idx >= size)
         {
            if(func != nullptr) item = cls->MatchFunc(func, true);
         }
         if(item == nullptr) item = cls->FindMember(name, true, scope, view);
         if(item == nullptr) return nullptr;
         if(item->GetClass() != cls) SubclassAccess(cls);
         break;

      case Cxx::Enum:
         //
         //  If there is another name, resolve it within this namespace,
         //  else return the enum itself.
         //
         if(idx >= size) return item;
         name = *qname->Names_at(idx)->Name();
         return static_cast< Enum* >(item)->FindEnumerator(name);

      case Cxx::Typedef:
         {
            //  See if the item wants to resolve the typedef.  In case the
            //  typedef is that of a template, instantiate it if a template
            //  member is being named.
            //
            auto tdef = static_cast< Typedef* >(item);
            tdef->SetAsReferent(this);
            if(!ResolveTypedef(tdef)) return tdef;
            auto root = tdef->Root();
            if(root == nullptr) return tdef;
            item = static_cast< CxxScoped* >(root);
            if(idx < size) item->Instantiate();
         }
         break;

      case Cxx::Forward:
      case Cxx::Friend:
         {
            if(!ResolveForward(item)) return item;
            auto ref = item->Referent();
            if(ref == nullptr) return item;
            item = static_cast< CxxScoped* >(ref);
         }
         break;

      default:
         auto expl = "Invalid type found while resolving " + name;
         Context::SwErr(CxxNamed_ResolveName, expl, type);
         return nullptr;
      }
   }

   return item;
}

//------------------------------------------------------------------------------

fn_name CxxNamed_ScopedName = "CxxNamed.ScopedName";

string CxxNamed::ScopedName(bool templates) const
{
   Debug::ft(CxxNamed_ScopedName);

   //  If the item's scope is not yet known, return its qualified name.
   //  If its scope is known, prefix the enclosing scopes to the name
   //  unless the item is unnamed, as in an anonymous enum or union.
   //
   auto scope = GetScope();
   if(scope == nullptr) return QualifiedName(true, templates);
   auto qname = QualifiedName(false, templates);
   if(qname.empty()) return scope->ScopedName(templates);
   return Prefix(scope->ScopedName(templates)) + qname;
}

//------------------------------------------------------------------------------

fn_name CxxNamed_SetTemplateParms = "CxxNamed.SetTemplateParms";

void CxxNamed::SetTemplateParms(TemplateParmsPtr& parms)
{
   Debug::ft(CxxNamed_SetTemplateParms);

   auto qname = GetQualName();

   if(qname != nullptr)
   {
      qname->SetTemplateParms(parms);
      return;
   }

   auto expl = "Failed to find qualified name for " + Trace();
   Context::SwErr(CxxNamed_SetTemplateParms, expl, 0);
}

//------------------------------------------------------------------------------

string CxxNamed::strLocation() const
{
   std::ostringstream stream;
   auto file = GetFile();
   stream << file->Name() << ", line " << file->GetLineNum(GetPos());
   return stream.str();
}

//------------------------------------------------------------------------------

void CxxNamed::strName(ostream& stream, bool fq, const QualName* name) const
{
   if(fq)
      stream << ScopedName(true);
   else
      name->Print(stream);
}

//==============================================================================

fn_name DataSpec_ctor1 = "DataSpec.ctor";

DataSpec::DataSpec(QualNamePtr& name) :
   name_(name.release()),
   arrays_(nullptr),
   ptrs_(0),
   refs_(0),
   arrayPos_(INT8_MAX),
   const_(false),
   constptr_(false),
   using_(false),
   ptrDet_(false),
   refDet_(false)
{
   Debug::ft(DataSpec_ctor1);

   CxxStats::Incr(CxxStats::DATA_SPEC);
}

//------------------------------------------------------------------------------

fn_name DataSpec_ctor2 = "DataSpec.ctor(string)";

DataSpec::DataSpec(const char* name) :
   arrays_(nullptr),
   ptrs_(0),
   refs_(0),
   arrayPos_(INT8_MAX),
   const_(false),
   constptr_(false),
   using_(false),
   ptrDet_(false),
   refDet_(false)
{
   Debug::ft(DataSpec_ctor2);

   name_ = QualNamePtr(new QualName(name));
   CxxStats::Incr(CxxStats::DATA_SPEC);
}

//------------------------------------------------------------------------------

fn_name DataSpec_ctor3 = "DataSpec.ctor(copy)";

DataSpec::DataSpec(const DataSpec& that) : TypeSpec(that),
   arrays_(nullptr),
   ptrs_(that.ptrs_),
   refs_(that.refs_),
   arrayPos_(that.arrayPos_),
   const_(that.const_),
   constptr_(that.constptr_),
   using_(that.using_),
   ptrDet_(that.ptrDet_),
   refDet_(that.refDet_)
{
   Debug::ft(DataSpec_ctor3);

   name_.reset(new QualName(*that.name_.get()));
   CxxStats::Incr(CxxStats::DATA_SPEC);
}

//------------------------------------------------------------------------------

fn_name DataSpec_dtor = "DataSpec.dtor";

DataSpec::~DataSpec()
{
   Debug::ft(DataSpec_dtor);

   CxxStats::Decr(CxxStats::DATA_SPEC);
}

//------------------------------------------------------------------------------

fn_name DataSpec_AddArray = "DataSpec.AddArray";

void DataSpec::AddArray(ArraySpecPtr& array)
{
   Debug::ft(DataSpec_AddArray);

   if(arrays_ == nullptr) arrays_.reset(new ArraySpecPtrVector);
   arrays_->push_back(std::move(array));
}

//------------------------------------------------------------------------------

fn_name DataSpec_AdjustPtrs = "DataSpec.AdjustPtrs";

void DataSpec::AdjustPtrs(TagCount count)
{
   Debug::ft(DataSpec_AdjustPtrs);

   //  This should only be invoked on an auto type.  After adjusting the
   //  count, invoke Ptrs to cause a log if the overall count is invalid.
   //
   if(!IsAutoDecl())
   {
      auto expl = "Adjusting pointers on non-auto type " + this->Trace();
      Context::SwErr(DataSpec_AdjustPtrs, expl, 0);
      return;
   }

   ptrs_ = count;
   Ptrs(true);
}

//------------------------------------------------------------------------------

fn_name DataSpec_AlignTemplateArg = "DataSpec.AlignTemplateArg";

string DataSpec::AlignTemplateArg(const TypeSpec* thatArg) const
{
   Debug::ft(DataSpec_AlignTemplateArg);

   //  If this is a template argument, remove any tags specified
   //  by this type from thatArg's type.
   //
   if(GetTemplateRole() != TemplateArgument) return ERROR_STR;

   auto thisTags = this->GetTags();

   if((thisTags.ptrs_ == 0) && (thisTags.arrays_ == 0))
   {
      return thatArg->TypeString(true);
   }

   auto thatTags = thatArg->GetTags();
   if(thatTags.ptrs_ < thisTags.ptrs_) return ERROR_STR;
   if(thatTags.arrays_ < thisTags.arrays_) return ERROR_STR;
   thatTags.ptrs_ -= thisTags.ptrs_;
   thatTags.arrays_ -= thisTags.arrays_;
   return thatArg->TypeTagsString(thatTags);
}

//------------------------------------------------------------------------------

fn_name DataSpec_ArrayCount = "DataSpec.ArrayCount";

TagCount DataSpec::ArrayCount() const
{
   Debug::ft(DataSpec_ArrayCount);

   TagCount count = (arrayPos_ != INT8_MAX ? 1 : 0);
   if(arrays_ != nullptr) count += TagCount(arrays_->size());
   return count;
}

//------------------------------------------------------------------------------

fn_name DataSpec_Arrays = "DataSpec.Arrays";

TagCount DataSpec::Arrays() const
{
   Debug::ft(DataSpec_Arrays);

   TagCount count = 0;
   auto spec = static_cast< const TypeSpec* >(this);

   while(spec != nullptr)
   {
      count += spec->ArrayCount();
      auto ref = spec->Referent();
      if(ref == nullptr) break;
      spec = ref->GetTypeSpec();
   }

   return count;
}

//------------------------------------------------------------------------------

fn_name DataSpec_Check = "DataSpec.Check";

void DataSpec::Check() const
{
   Debug::ft(DataSpec_Check);

   if(ptrDet_) Log(PtrTagDetached);
   if(refDet_) Log(RefTagDetached);
}

//------------------------------------------------------------------------------

fn_name DataSpec_Clone = "DataSpec.Clone";

TypeSpec* DataSpec::Clone() const
{
   Debug::ft(DataSpec_Clone);

   return new DataSpec(*this);
}

//------------------------------------------------------------------------------

fn_name DataSpec_DirectClass = "DataSpec.DirectClass";

Class* DataSpec::DirectClass() const
{
   Debug::ft(DataSpec_DirectClass);

   auto root = Root();
   if(root->Type() != Cxx::Class) return nullptr;
   if(IsIndirect()) return nullptr;
   return static_cast< Class* >(root);
}

//------------------------------------------------------------------------------

fn_name DataSpec_DirectType = "DataSpec.DirectType";

CxxNamed* DataSpec::DirectType() const
{
   Debug::ft(DataSpec_DirectType);

   return name_->DirectType();
}

//------------------------------------------------------------------------------

void DataSpec::DisplayArrays(ostream& stream) const
{
   if(arrays_ != nullptr)
   {
      for(auto a = arrays_->cbegin(); a != arrays_->cend(); ++a)
      {
         (*a)->Print(stream);
      }
   }
}

//------------------------------------------------------------------------------

void DataSpec::DisplayTags(ostream& stream) const
{
   if(arrayPos_ != INT8_MAX)
   {
      if(arrayPos_ > 0) stream << string(arrayPos_, '*');
      stream << ARRAY_STR;
      auto after = ptrs_ - arrayPos_;
      if(after > 0) stream << string(after, '*');
   }
   else
   {
      if(ptrs_ > 0) stream << string(ptrs_, '*');
   }

   if(constptr_) stream << " const";
   if(refs_ > 0) stream << string(refs_, '&');
}

//------------------------------------------------------------------------------

fn_name DataSpec_EnterArrays = "DataSpec.EnterArrays";

void DataSpec::EnterArrays() const
{
   Debug::ft(DataSpec_EnterArrays);

   if(arrays_ != nullptr)
   {
      for(auto a = arrays_->cbegin(); a != arrays_->cend(); ++a)
      {
         (*a)->EnterBlock();
      }
   }
}

//------------------------------------------------------------------------------

fn_name DataSpec_EnterBlock = "DataSpec.EnterBlock";

void DataSpec::EnterBlock()
{
   Debug::ft(DataSpec_EnterBlock);

   Context::PushArg(ResultType());
}

//------------------------------------------------------------------------------

fn_name DataSpec_EnteringScope = "DataSpec.EnteringScope";

void DataSpec::EnteringScope(const CxxScope* scope)
{
   Debug::ft(DataSpec_EnteringScope);

   if(scope->NameIsTemplateParm(*Name()))
   {
      SetTemplateRole(TemplateParameter);
   }

   EnterArrays();
   Check();  //* delay until >check
   if(name_->GetReferent() == nullptr) FindReferent();
}

//------------------------------------------------------------------------------

fn_name DataSpec_FindReferent = "DataSpec.FindReferent";

bool DataSpec::FindReferent()
{
   Debug::ft(DataSpec_FindReferent);

   //  Find referents for any template arguments used in the type's name.
   //  Bypass name_ itself; a QualName only finds its referent when used
   //  in executable code.
   //
   auto size = name_->Names_size();

   for(size_t i = 0; i < size; ++i)
   {
      name_->Names_at(i)->FindReferent();
   }

   //  This should find a referent during parsing, when there is a context
   //  file.  If it isn't found then, it's pointless to look later.
   //
   auto file = Context::File();
   if(file == nullptr) return false;
   auto scope = Context::Scope();
   if(scope == nullptr) return false;

   if(ResolveTemplateArgument()) return true;

   SymbolView view;
   auto item = ResolveName(file, scope, TYPESPEC_REFS, &view);

   if(item != nullptr)
   {
      SetReferent(item, view.using_);
      return true;
   }

   //  The referent wasn't found.  If this is a template parameter (the "T"
   //  in "template< typename T >", for example) it never will be.
   //c If templates were executed, some or all of the following might have
   //  to move to ResolveName in order to identify locals that are template
   //  parameters.
   //
   auto qname = QualifiedName(true, false);

   if(scope->NameIsTemplateParm(qname))
   {
      SetTemplateRole(TemplateParameter);
      return true;
   }

   auto syms = Singleton< CxxSymbols >::Instance();

   switch(GetTemplateRole())
   {
   case TemplateArgument:
      //
      //  Here, NAME could be a constant instead of a type.  If not, it could
      //  be a template parameter used in a partial specialization.  In either
      //  case, report that the referent was found.
      //
      item = syms->FindSymbol(file, scope, qname, VALUE_REFS, &view);
      if(item != nullptr) SetReferent(item, view.using_);
   case TemplateParameter:
   case TemplateClass:
      //
      //  When Operation.ExecuteOverload checks if a function overload applies,
      //  Function.MatchTemplate may create the DataSpec for a class template
      //  that defines an operator at file scope, like operator<< for a string.
      //  In this case, the class template may not even be visible in the scope
      //  where the possibility of the overload is being checked.
      //
      return true;
   }

   //  When parsing a template instance, the arguments may not be visible,
   //  because the scope is the template instance itself.  For example, the
   //  type A is often not visible in the scope std::unique_ptr<A>.
   //
   if(scope->IsInTemplateInstance()) return true;

   //  The referent couldn't be found.
   //
   auto expl = "Failed to find referent for " + qname;
   Context::SwErr(DataSpec_FindReferent, expl, 0);
   return false;
}

//------------------------------------------------------------------------------

Numeric DataSpec::GetNumeric() const
{
   auto ptrs = Ptrs(true);

   if(ptrs > 0)
   {
      auto arrays = ArrayCount();
      if(ptrs - arrays > 0) return Numeric::Pointer;
      return Numeric::Nil;
   }

   if(Refs() > 0) return Numeric::Nil;

   auto root = Root();
   if(root == nullptr) return Numeric::Nil;
   return root->GetNumeric();
}

//------------------------------------------------------------------------------

CxxScope* DataSpec::GetScope() const
{
   auto type = name_->DirectType();
   if(type != nullptr) return type->GetScope();
   return nullptr;
}

//------------------------------------------------------------------------------

TypeTags DataSpec::GetTags() const
{
   return TypeTags(*this);
}

//------------------------------------------------------------------------------

TypeSpec* DataSpec::GetTypeSpec() const
{
   return const_cast< DataSpec* >(this);
}

//------------------------------------------------------------------------------

fn_name DataSpec_GetUsages = "DataSpec.GetUsages";

void DataSpec::GetUsages(const CodeFile& file, CxxUsageSets& symbols) const
{
   Debug::ft(DataSpec_GetUsages);

   //  Don't obtain usages for an auto type.  If the type ends up being
   //  used to invoke a function, for example, this usage will be noted.
   //  However, items in the expression that obtain the auto type will
   //  be noted as usages, and it must transitively #include that type.
   //
   if(IsAutoDecl()) return;

   //  Find usages for any template arguments used in the type's name.
   //  Bypass name_ itself, because it doesn't know if it is indirect.
   //
   auto size = name_->Names_size();

   for(size_t i = 0; i < size; ++i)
   {
      name_->Names_at(i)->GetUsages(file, symbols);
   }

   //  Get the usages for any array specifications.
   //
   if(arrays_ != nullptr)
   {
      for(auto a = arrays_->cbegin(); a != arrays_->cend(); ++a)
      {
         (*a)->GetUsages(file, symbols);
      }
   }

   auto ref = DirectType();

   if(ref == nullptr)
   {
      //  The referent for this type was never found.  Log it.
      //
      auto role = GetTemplateRole();
      if((role == TemplateParameter) || (role == TemplateArgument)) return;
      auto qname = QualifiedName(true, false);
      auto log = "Unknown type for " + qname + " at " + strLocation();
      Debug::SwErr(DataSpec_GetUsages, log, 0, InfoLog);
      return;
   }

   //  If this item was accessed via a subclass, add the subclass as a
   //  direct usage.
   //
   name_->GetClassUsage(file, symbols);

   //  Record how the item was used.
   //
   auto type = ref->Type();

   switch(type)
   {
   case Cxx::Terminal:
      break;

   case Cxx::Forward:
   case Cxx::Friend:
      symbols.AddForward(ref);
      break;

   case Cxx::Class:
      {
         auto cls = static_cast< Class* >(ref);
         auto tmplt = cls->GetTemplate();
         if(tmplt != nullptr) ref = tmplt;
      }
   default:
      //  Although a .cpp can use a type indirectly, it is unusual.  In most
      //  cases, a pointer or reference type will be initialized, in which case
      //  it cannot be declared forward unless, for example, it is initialized
      //  to nullptr, passed as an argument, and not looked at again.  To make
      //  an accurate direct/indirect determination for a .cpp seems to involve
      //  more effort than is worthwhile.
      //
      if(file.IsHeader() && IsUsedInNameOnly())
         symbols.AddIndirect(ref);
      else
         symbols.AddDirect(ref);
   }

   auto forw = name_->GetForward();
   if(forw != nullptr) symbols.AddForward(forw);

   //  Indicate whether our referent was made visible by a using statement.
   //
   if(using_) symbols.AddUser(this);
}

//------------------------------------------------------------------------------

fn_name DataSpec_Instantiating = "DataSpec.Instantiating";

void DataSpec::Instantiating() const
{
   Debug::ft(DataSpec_Instantiating);

   //  When instantiating a template, each of its arguments should have a
   //  referent.  Invoke Instantiate on each argument that is a class: if
   //  it's also a template, it must be instantiated so that our template
   //  instance can use it.
   //
   auto ref = Referent();

   if(ref != nullptr)
   {
      ref->Instantiate();
      return;
   }

   auto expl = "Failed to find referent for " + TypeString(false);
   Context::SwErr(DataSpec_Instantiating, expl, 0);
}

//------------------------------------------------------------------------------

fn_name DataSpec_IsAuto = "DataSpec.IsAuto";

bool DataSpec::IsAuto() const
{
   Debug::ft(DataSpec_IsAuto);

   //  A data item (FuncData) of type auto initially has the keyword "auto"
   //  as its referent.  This referent is overwritten when the data's actual
   //  type is determined.
   //
   return (Referent() == Singleton< CxxRoot >::Instance()->AutoTerm());
}

//------------------------------------------------------------------------------

bool DataSpec::IsAutoDecl() const
{
   return (*Name() == AUTO_STR);
}

//------------------------------------------------------------------------------

fn_name DataSpec_IsConst = "DataSpec.IsConst";

bool DataSpec::IsConst() const
{
   Debug::ft(DataSpec_IsConst);

   if(IsAutoDecl()) return const_;

   if(const_) return true;
   auto ref = Referent();
   if(ref == nullptr) return false;
   auto spec = ref->GetTypeSpec();
   if(spec == nullptr) return false;
   return spec->IsConst();
}

//------------------------------------------------------------------------------

fn_name DataSpec_IsConstPtr = "DataSpec.IsConstPtr";

bool DataSpec::IsConstPtr() const
{
   Debug::ft(DataSpec_IsConstPtr);

   if(IsAutoDecl()) return constptr_;

   if(constptr_) return true;
   auto ref = Referent();
   if(ref == nullptr) return false;
   auto spec = ref->GetTypeSpec();
   if(spec == nullptr) return false;
   return spec->IsConstPtr();
}

//------------------------------------------------------------------------------

fn_name DataSpec_IsIndirect = "DataSpec.IsIndirect";

bool DataSpec::IsIndirect() const
{
   Debug::ft(DataSpec_IsIndirect);

   return ((Refs() > 0) || (Ptrs(true) > 0));
}

//------------------------------------------------------------------------------

fn_name DataSpec_IsUsedInNameOnly = "DataSpec.IsUsedInNameOnly";

bool DataSpec::IsUsedInNameOnly() const
{
   Debug::ft(DataSpec_IsUsedInNameOnly);

   //  The type did not have to be defined before it appeared in this
   //  specification if one of the following is true:
   //  o It has pointer or reference tags, ignoring arrays.
   //  o It is the type for a function's argument or return value.
   //  o It is a template argument--unless it is a template instance
   //    or appears in code.
   //  o It appears in a typedef--unless it is a template.
   //
   auto count = Ptrs(false);
   if(count > 0) return true;
   if(Refs() > 0) return true;

   auto loc = GetLocale();
   if(loc == Cxx::Function) return true;

   CxxNamed* ref = nullptr;

   if(GetTemplateRole() != TemplateNone)
   {
      ref = name_->GetReferent();
      if((ref != nullptr) && ref->IsInTemplateInstance()) return false;
      return (loc != Cxx::Operation);
   }

   if(loc != Cxx::Typedef) return false;

   if(ref == nullptr) ref = name_->GetReferent();
   if((ref != nullptr) && (ref->Type() == Cxx::Class))
   {
      auto cls = static_cast< Class* >(ref);
      if(cls->IsInTemplateInstance()) return false;
      if(cls->GetTemplate() != nullptr) return false;
   }

   return true;
}

//------------------------------------------------------------------------------

fn_name DataSpec_MatchesExactly = "DataSpec.MatchesExactly";

bool DataSpec::MatchesExactly(const TypeSpec* that) const
{
   Debug::ft(DataSpec_MatchesExactly);

   auto type1 = this->TypeString(false);
   auto type2 = that->TypeString(false);
   return (type1 == type2);
}

//------------------------------------------------------------------------------

fn_name DataSpec_MatchTemplate = "DataSpec.MatchTemplate";

TypeMatch DataSpec::MatchTemplate(TypeSpec* that, stringVector& tmpltParms,
   stringVector& tmpltArgs, bool& argFound) const
{
   Debug::ft(DataSpec_MatchTemplate);

   //  Do a depth-first traversal of this type and THAT.  For each node
   //  in this type, THAT must have a corresponding node.
   //
   if(that->Referent() == nullptr) return Incompatible;

   //  If this type is a template parameter, the node in THAT becomes its
   //  template argument.  If a template argument has already been found
   //  for the parameter, THAT must match it.
   //
   auto parm = ScopedName(false);
   auto idx = FindIndex(tmpltParms, parm);
   auto match = Compatible;

   if(idx >= 0)
   {
      //  If the template parameter specifies pointers, remove that number
      //  of pointers from the template argument to find the actual type.
      //
      TagCount thisPtrs = this->Ptrs(true);
      TagCount thatPtrs = that->Ptrs(true);
      if(thisPtrs > thatPtrs) return Incompatible;
      if(thisPtrs < thatPtrs) match = Convertible;

      argFound = true;
      auto thatType = that->TypeString(true);
      if(thisPtrs > 0) CodeTools::AdjustPtrs(thatType, -thisPtrs);

      //  If the type of the template parameter has already been set, assume
      //  that this type matches it.  Strictly comparing thatArg == thatType
      //  erroneously rejects, for example, std::min(int, unsigned int).
      //
      auto& thatArg = tmpltArgs.at(idx);
      if(thatArg.empty()) thatArg = thatType;
      return match;
   }

   //  This type was not a template parameter.  THAT must match it.
   //
   if(this->Referent() == nullptr) return Incompatible;

   auto thisType = RemoveTemplates(this->TypeString(true));
   auto thatType = RemoveTemplates(that->TypeString(true));
   if((thisType != thatType) && (RemoveConsts(thisType) != thatType))
   {
      return Incompatible;
   }

   auto thisName = this->GetQualName();
   auto thatName = that->GetQualName();
   return thisName->MatchTemplate(thatName, tmpltParms, tmpltArgs, argFound);
}

//------------------------------------------------------------------------------

fn_name DataSpec_MatchTemplateArg = "DataSpec.MatchTemplateArg";

TypeMatch DataSpec::MatchTemplateArg(const TypeSpec* that) const
{
   Debug::ft(DataSpec_MatchTemplateArg);

   //  If this is a template argument, match on the basis of tags,
   //  leaving room to prefer an exact match.
   //
   if(GetTemplateRole() == TemplateArgument)
   {
      auto thisTags = this->GetTags();
      auto thatTags = that->GetTags();

      if(thisTags.ptrs_ > thatTags.ptrs_) return Incompatible;
      if(thisTags.arrays_ > thatTags.arrays_) return Incompatible;
      if(thisTags.ptrs_ < thatTags.ptrs_) return Convertible;
      if(thisTags.arrays_ < thatTags.arrays_) return Convertible;
      return Compatible;
   }

   //  This is not a template argument, so match on types.
   //
   if(MatchesExactly(that)) return Compatible;
   return Incompatible;
}

//------------------------------------------------------------------------------

void DataSpec::Print(ostream& stream) const
{
   if(const_) stream << CONST_STR << SPACE;
   name_->Print(stream);
   DisplayTags(stream);

   if(IsAutoDecl())
   {
      stream << SPACE << COMMENT_START_STR << SPACE;
      stream << TypeString(true) << SPACE << COMMENT_END_STR;
   }
}

//------------------------------------------------------------------------------

fn_name DataSpec_PtrCount = "DataSpec.PtrCount";

TagCount DataSpec::PtrCount(bool arrays) const
{
   Debug::ft(DataSpec_PtrCount);

   auto count = ptrs_;
   if(!arrays) return count;
   if(arrays_ != nullptr) count += TagCount(arrays_->size());
   if(arrayPos_ != INT8_MAX) ++count;
   return count;
}

//------------------------------------------------------------------------------

fn_name DataSpec_Ptrs = "DataSpec.Ptrs";

TagCount DataSpec::Ptrs(bool arrays) const
{
   Debug::ft(DataSpec_Ptrs);

   TagCount count = 0;
   auto spec = static_cast< const TypeSpec* >(this);

   while(spec != nullptr)
   {
      count += spec->PtrCount(arrays);
      auto ref = spec->Referent();
      if(ref == nullptr) break;
      spec = ref->GetTypeSpec();
   }

   if(count >= 0) return count;

   auto expl = "Negative pointer count for " + Trace();
   Context::SwErr(DataSpec_Ptrs, expl, count);
   return 0;
}

//------------------------------------------------------------------------------

fn_name DataSpec_Referent = "DataSpec.Referent";

CxxNamed* DataSpec::Referent() const
{
   Debug::ft(DataSpec_Referent);

   auto ref = name_->GetReferent();
   if(ref != nullptr) return ref;

   const_cast< DataSpec* >(this)->FindReferent();
   return name_->GetReferent();
}

//------------------------------------------------------------------------------

fn_name DataSpec_Refs = "DataSpec.Refs";

TagCount DataSpec::Refs() const
{
   Debug::ft(DataSpec_Refs);

   TagCount count = 0;
   auto spec = static_cast< const TypeSpec* >(this);

   while(spec != nullptr)
   {
      count += spec->RefCount();
      auto ref = spec->Referent();
      if(ref == nullptr) break;
      spec = ref->GetTypeSpec();
   }

   if(count >= 0) return count;

   auto expl = "Negative reference count for " + Trace();
   Context::SwErr(DataSpec_Refs, expl, count);
   return 0;
}

//------------------------------------------------------------------------------

fn_name DataSpec_RemoveRefs = "DataSpec.RemoveRefs";

void DataSpec::RemoveRefs()
{
   Debug::ft(DataSpec_RemoveRefs);

   if(!IsAutoDecl())
   {
      auto expl = "Removing references on non-auto type " + this->Trace();
      Context::SwErr(DataSpec_RemoveRefs, expl, 0);
      return;
   }

   if(Refs() == 0) return;

   if(refs_ != 0)
   {
      auto expl = "Removing references from auto& type " + this->Trace();
      Context::SwErr(DataSpec_RemoveRefs, expl, 1);
      return;
   }

   //  Negate any reference(s) on the referent's type.
   //
   refs_ = -Refs();
}

//------------------------------------------------------------------------------

fn_name DataSpec_ResolveForward = "DataSpec.ResolveForward";

bool DataSpec::ResolveForward(CxxScoped* decl) const
{
   Debug::ft(DataSpec_ResolveForward);

   //  Stop at the forward declaration unless it's a template.  If it is,
   //  continue so that template arguments can be applied to its referent.
   //
   if((decl->IsTemplate()) && (decl->Referent() != nullptr))
   {
      name_->SetForward(decl);
      decl->SetAsReferent(this);
      return true;
   }

   return false;
}

//------------------------------------------------------------------------------

fn_name DataSpec_ResolveTemplate = "DataSpec.ResolveTemplate";

bool DataSpec::ResolveTemplate(Class* cls, const TypeName* args, bool end) const
{
   Debug::ft(DataSpec_ResolveTemplate);

   //  Don't create a template instance if this item was only created
   //  interally, during template matching.
   //
   return (GetTemplateRole() != TemplateClass);
}

//------------------------------------------------------------------------------

fn_name DataSpec_ResolveTemplateArgument = "DataSpec.ResolveTemplateArgument";

bool DataSpec::ResolveTemplateArgument()
{
   Debug::ft(DataSpec_ResolveTemplateArgument);

   if(GetTemplateRole() != TemplateArgument) return false;

   auto parser = Context::GetParser();
   auto item = parser->ResolveInstanceArgument(name_.get());
   if(item == nullptr) return false;

   SetReferent(item, false);
   return true;
}

//------------------------------------------------------------------------------

fn_name DataSpec_ResolveTypedef = "DataSpec.ResolveTypedef";

bool DataSpec::ResolveTypedef(Typedef* type) const
{
   Debug::ft(DataSpec_ResolveTypedef);

   //  Stop at the typedef unless it has template arguments.  If it does,
   //  record that it was a referent but resolve to the template instance.
   //
   if(type->GetTemplateArgs() == nullptr) return false;
   name_->SetTypedef(type);
   return true;
}

//------------------------------------------------------------------------------

fn_name DataSpec_ResultType = "DataSpec.ResultType";

StackArg DataSpec::ResultType() const
{
   Debug::ft(DataSpec_ResultType);

   auto result = Referent();

   if(result != nullptr)
   {
      auto type = result->Type();

      if((type == Cxx::Forward) || (type == Cxx::Friend))
      {
         auto target = result->Referent();
         if(target != nullptr) result = target;
      }

      auto arg = StackArg(result, PtrCount(true));
      if(const_) arg.SetAsConst();
      if(constptr_) arg.SetAsConstPtr();
      return arg;
   }

   switch(GetTemplateRole())
   {
   case TemplateParameter:
   case TemplateClass:
      break;
   default:
      auto expl = "Failed to find referent for " + QualifiedName(true, true);
      Context::SwErr(DataSpec_ResultType, expl, 0);
   }

   return NilStackArg;
}

//------------------------------------------------------------------------------

fn_name DataSpec_SetLocale = "DataSpec.SetLocale";

void DataSpec::SetLocale(Cxx::ItemType locale)
{
   Debug::ft(DataSpec_SetLocale);

   TypeSpec::SetLocale(locale);
   name_->SetLocale(locale);
}

//------------------------------------------------------------------------------

fn_name DataSpec_SetReferent = "DataSpec.SetReferent";

void DataSpec::SetReferent(CxxNamed* ref, bool use)
{
   Debug::ft(DataSpec_SetReferent);

   //  If REF is an unresolved forward declaration for a template, our referent
   //  needs to be a template instance instantiated from that template.  This is
   //  not yet possible, so leave our referent empty so that we will revisit it.
   //
   if(!ref->IsTemplate() || (ref->Referent() != nullptr))
   {
      name_->SetReferent(ref);
      using_ = use;

      if(ref->Type() != Cxx::Typedef)
      {
         //  SetAsReferent has already been invoked if our referent is a
         //  typedef, so don't invoke it again.
         //
         ref->SetAsReferent(this);
      }
      else
      {
         //  If our referent is a pointer typedef, "const" applies to the
         //  pointer, not its target.
         //
         if(const_ && (ref->GetTypeSpec()->Ptrs(false) > 0))
         {
            const_ = false;
            constptr_ = true;
         }
      }
   }
}

//------------------------------------------------------------------------------

fn_name DataSpec_SetTemplateRole = "DataSpec.SetTemplateRole";

void DataSpec::SetTemplateRole(TemplateRole role) const
{
   Debug::ft(DataSpec_SetTemplateRole);

   TypeSpec::SetTemplateRole(role);

   if(role == TemplateClass)
   {
      auto size = name_->Names_size();

      for(size_t i = 0; i < size; ++i)
      {
         name_->Names_at(i)->SetTemplateRole(TemplateParameter);
      }
   }
}

//------------------------------------------------------------------------------

void DataSpec::Shrink()
{
   name_->Shrink();

   if(arrays_ != nullptr)
   {
      for(auto a = arrays_->cbegin(); a != arrays_->cend(); ++a)
      {
         (*a)->Shrink();
      }

      auto size = arrays_->capacity() * sizeof(ArraySpecPtr);
      CxxStats::Vectors(CxxStats::DATA_SPEC, size);
   }
}

//------------------------------------------------------------------------------

fn_name DataSpec_SubclassAccess = "DataSpec.SubclassAccess";

void DataSpec::SubclassAccess(Class* cls) const
{
   Debug::ft(DataSpec_SubclassAccess);

   name_->SubclassAccess(cls);
}

//------------------------------------------------------------------------------

string DataSpec::Trace() const
{
   auto result = TypeString(false);
   if(result != ERROR_STR) return result;
   return CxxNamed::Trace();
}

//------------------------------------------------------------------------------

string DataSpec::TypeString(bool arg) const
{
   string ts;
   auto role = GetTemplateRole();

   //  Use the referent if it is known.  However, a template parameter has
   //  no referent, and a template argument could be an unresolved forward
   //  declaration.  In such cases, just use the full name.
   //
   auto ref = (role != TemplateParameter ? Referent() : nullptr);
   auto tags = GetTags();

   if(ref != nullptr)
   {
      ts = name_->TypeString(arg);
   }
   else
   {
      if(role == TemplateNone) return ERROR_STR;
      ts = QualifiedName(true, true);
   }

   //  Remove any tags from TS and replace them with our own.
   //
   RemoveTags(ts);

   if(tags.const_) ts = "const " + ts;

   if(arg)
   {
      ts += string(tags.ptrs_ + tags.arrays_, '*');
   }
   else
   {
      ts += string(tags.ptrs_, '*');

      if(tags.arrays_ > 0)
      {
         for(auto i = 0 ; i < tags.arrays_; ++i) ts += ARRAY_STR;
      }
   }

   if(tags.constptr_) ts += " const";
   if(!arg) ts += string(tags.refs_, '&');
   return ts;
}

//------------------------------------------------------------------------------

string DataSpec::TypeTagsString(const TypeTags& tags) const
{
   string ts;

   if(tags.const_)
      ts += "const ";
   ts += name_->TypeString(true);
   if(tags.ptrs_ > 0) ts += string(tags.ptrs_, '*');
   for(auto i = 0; i < tags.arrays_; ++i) ts += ARRAY_STR;
   if(tags.constptr_) ts += " const";
   if(tags.refs_ > 0) ts += string(tags.refs_, '&');
   return ts;
}

//==============================================================================

fn_name MemberInit_ctor = "MemberInit.ctor";

MemberInit::MemberInit(string& name, TokenPtr& init) : init_(init.release())
{
   Debug::ft(MemberInit_ctor);

   std::swap(name_, name);
   CxxStats::Incr(CxxStats::MEMBER_INIT);
}

//------------------------------------------------------------------------------

fn_name MemberInit_GetUsages = "MemberInit.GetUsages";

void MemberInit::GetUsages(const CodeFile& file, CxxUsageSets& symbols) const
{
   Debug::ft(MemberInit_GetUsages);

   init_->GetUsages(file, symbols);
}

//------------------------------------------------------------------------------

void MemberInit::Print(ostream& stream) const
{
   stream << name_;
   init_->Print(stream);
}

//------------------------------------------------------------------------------

void MemberInit::Shrink()
{
   name_.shrink_to_fit();
   CxxStats::Strings(CxxStats::MEMBER_INIT, name_.capacity());
   init_->Shrink();
}

//==============================================================================

fn_name QualName_ctor1 = "QualName.ctor(type)";

QualName::QualName(TypeNamePtr& type) :
   name_(type.release()),
   ref_(nullptr),
   class_(nullptr),
   type_(nullptr),
   forw_(nullptr),
   using_(false),
   oper_(Cxx::NIL_OPERATOR),
   global_(false),
   qualified_(false)
{
   Debug::ft(QualName_ctor1);

   CxxStats::Incr(CxxStats::QUAL_NAME);
}

//------------------------------------------------------------------------------

fn_name QualName_ctor2 = "QualName.ctor(string)";

QualName::QualName(const string& name) :
   name_(nullptr),
   ref_(nullptr),
   class_(nullptr),
   type_(nullptr),
   forw_(nullptr),
   using_(false),
   oper_(Cxx::NIL_OPERATOR),
   global_(false),
   qualified_(false)
{
   Debug::ft(QualName_ctor2);

   auto copy = name;
   name_ = new TypeName(copy);
   CxxStats::Incr(CxxStats::QUAL_NAME);
}

//------------------------------------------------------------------------------

fn_name QualName_ctor3 = "QualName.ctor(copy)";

QualName::QualName(const QualName& that) : CxxNamed(that),
   name_(nullptr),
   ref_(that.ref_),
   class_(that.class_),
   type_(that.type_),
   forw_(that.forw_),
   using_(that.using_),
   oper_(that.oper_),
   global_(that.global_),
   qualified_(that.qualified_)
{
   Debug::ft(QualName_ctor3);

   if(qualified_) names_ = new TypeNamePtrVector;
   auto size = that.Names_size();

   for(size_t i = 0; i < size; ++i)
   {
      auto thatName = that.Names_at(i);
      auto thisName = new TypeName(*thatName);

      if(size == 1)
         name_ = thisName;
      else
         names_->push_back(TypeNamePtr(thisName));
   }

   if(that.parms_ != nullptr)
   {
      parms_.reset(new TemplateParms(*that.parms_));
   }

   CxxStats::Incr(CxxStats::QUAL_NAME);
}

//------------------------------------------------------------------------------

fn_name QualName_dtor = "QualName.dtor";

QualName::~QualName()
{
   Debug::ft(QualName_dtor);

   if(qualified_)
   {
      delete names_;
      names_ = nullptr;
   }
   else
   {
      delete name_;
      name_ = nullptr;
   }

   CxxStats::Decr(CxxStats::QUAL_NAME);
}

//------------------------------------------------------------------------------

fn_name QualName_AddTypeName = "QualName.AddTypeName";

void QualName::AddTypeName(TypeNamePtr& type)
{
   Debug::ft(QualName_AddTypeName);

   if(!qualified_)
   {
      auto name = name_;
      name_ = nullptr;
      names_ = new TypeNamePtrVector;
      qualified_ = true;
      names_->push_back(TypeNamePtr(name));
   }

   names_->push_back(std::move(type));
}

//------------------------------------------------------------------------------

fn_name QualName_Append = "QualName.Append";

void QualName::Append(const string& name, bool space)
{
   Debug::ft(QualName_Append);

   auto size = Names_size();

   if(size > 0)
   {
      if(space) Names_back()->Append(SPACE_STR);
      Names_back()->Append(name);
   }
}

//------------------------------------------------------------------------------

fn_name QualName_CheckCtorDefn = "QualName.CheckCtorDefn";

bool QualName::CheckCtorDefn() const
{
   Debug::ft(QualName_CheckCtorDefn);

   auto size = Names_size();
   if(size <= 1) return false;
   return (*Names_at(size - 1)->Name() == *Names_at(size - 2)->Name());
}

//------------------------------------------------------------------------------

CxxNamed* QualName::DirectType() const
{
   return (type_ != nullptr ? type_ : ref_);
}

//------------------------------------------------------------------------------

fn_name QualName_EnterBlock = "QualName.EnterBlock";

void QualName::EnterBlock()
{
   Debug::ft(QualName_EnterBlock);

   if(*Name() == "NULL") Log(UseOfNull);

   //  If a "." or "->" operator is waiting for its second argument,
   //  push this name and return so that the operator can be executed.
   //
   auto op = Cxx::NIL_OPERATOR;
   auto top = Context::TopOp();

   if(top != nullptr)
   {
      op = top->Op();

      if((op == Cxx::REFERENCE_SELECT) || (op == Cxx::POINTER_SELECT))
      {
         Context::PushArg(StackArg(this, 0));
         return;
      }
   }

   auto ref = Referent();
   if(ref != nullptr) Context::PushArg(ref->NameToArg(op));
}

//------------------------------------------------------------------------------

fn_name QualName_GetClassUsage = "QualName.GetClassUsage";

void QualName::GetClassUsage(const CodeFile& file, CxxUsageSets& symbols) const
{
   Debug::ft(QualName_GetClassUsage);

   auto cls = class_;

   if(cls != nullptr)
   {
      if(cls->IsInTemplateInstance()) cls = cls->GetTemplate();
      if(cls->GetFile() != &file) symbols.AddDirect(cls);
   }
}

//------------------------------------------------------------------------------

CxxScoped* QualName::GetForward() const
{
   return forw_;
}

//------------------------------------------------------------------------------

fn_name QualName_GetTemplateArgs = "QualName.GetTemplateArgs";

TypeName* QualName::GetTemplateArgs() const
{
   Debug::ft(QualName_GetTemplateArgs);

   auto size = Names_size();

   for(size_t i = 0; i < size; ++i)
   {
      auto spec = Names_at(i)->GetTemplateArgs();
      if(spec != nullptr) return spec;
   }

   return nullptr;
}

//------------------------------------------------------------------------------

fn_name QualName_GetUsages = "QualName.GetUsages";

void QualName::GetUsages(const CodeFile& file, CxxUsageSets& symbols) const
{
   Debug::ft(QualName_GetUsages);

   //  Get the usages for any template arguments.
   //
   auto size = Names_size();

   for(size_t i = 0; i < size; ++i)
   {
      Names_at(i)->GetUsages(file, symbols);
   }

   //  Add, as a direct usage, the class (if any) through which our
   //  referent was accessed.
   //
   GetClassUsage(file, symbols);

   //  Add, as a direct usage, our referent or the typedef through which
   //  it was accessed.  Omit terminals and function arguments.
   //
   auto ref = DirectType();
   if(ref == nullptr) return;

   auto type = ref->Type();
   if((type == Cxx::Terminal) || (type == Cxx::Argument)) return;

   //  If the used item is in a template instance, find the corresponding
   //  item in the class template.
   //
   auto cls = ref->GetClass();

   if(cls != nullptr)
   {
      if(cls->IsInTemplateInstance())
      {
         ref = static_cast< ClassInst* >(cls)->FindTemplateAnalog(ref);
         if(ref == nullptr) return;
      }
   }

   //  If the item is a function, the referent could be an override, but
   //  only its original declaration needs to be accessible.
   //
   if(type == Cxx::Function)
   {
      ref = static_cast< Function* >(ref)->FindRootFunc();
   }

   symbols.AddDirect(ref);
   if(using_) symbols.AddUser(this);
}

//------------------------------------------------------------------------------

fn_name QualName_MatchTemplate = "QualName.MatchTemplate";

TypeMatch QualName::MatchTemplate(const QualName* that,
   stringVector& tmpltParms, stringVector& tmpltArgs, bool& argFound) const
{
   Debug::ft(QualName_MatchTemplate);

   auto match = Compatible;
   auto size = Names_size();

   for(size_t i = 0; i < size; ++i)
   {
      auto n1 = this->Names_at(i);
      auto n2 = that->Names_at(i);
      auto result = n1->MatchTemplate(n2, tmpltParms, tmpltArgs, argFound);
      if(result == Incompatible) return Incompatible;
      if(result < match) match = result;
   }

   return match;
}

//------------------------------------------------------------------------------

fn_name QualName_MemberAccessed = "QualName.MemberAccessed";

void QualName::MemberAccessed(Class* cls, CxxNamed* mem) const
{
   Debug::ft(QualName_MemberAccessed);

   ref_ = mem;
   class_ = cls;
}

//------------------------------------------------------------------------------

fn_name QualName_MoveTemplateParms = "QualName.MoveTemplateParms";

void QualName::MoveTemplateParms(CxxScoped* item)
{
   Debug::ft(QualName_MoveTemplateParms);

   item->SetTemplateParms(parms_);
}

//------------------------------------------------------------------------------

const string* QualName::Name() const
{
   return (qualified_ ? names_->back()->Name() : name_->Name());
}

//------------------------------------------------------------------------------

TypeName* QualName::Names_at(size_t index) const
{
   if(qualified_) return names_->at(index).get();
   if(index == 0) return name_;
   return nullptr;
}

//------------------------------------------------------------------------------

TypeName* QualName::Names_back() const
{
   return (qualified_ ? names_->back().get() : name_);
}

//------------------------------------------------------------------------------

size_t QualName::Names_size() const
{
   return (qualified_ ? names_->size() : 1);
}

//------------------------------------------------------------------------------

void QualName::Print(ostream& stream) const
{
   if(global_) stream << SCOPE_STR;

   auto size = Names_size();

   for(size_t i = 0; i < size; ++i)
   {
      Names_at(i)->Print(stream);
      if(i < (size - 1)) stream << SCOPE_STR;
   }
}

//------------------------------------------------------------------------------

string QualName::QualifiedName(bool scopes, bool templates) const
{
   if(scopes)
   {
      //  Build the qualified name.  Each name (except the first) is preceded
      //  by a scope resolution operator.
      //
      string qname = (global_ ? SCOPE_STR : EMPTY_STR);

      auto size = Names_size();

      for(size_t i = 0; i < size; ++i)
      {
         qname += Names_at(i)->QualifiedName(scopes, templates);
         if(i < (size - 1)) qname += SCOPE_STR;
      }

      return qname;
   }

   //  Only the leaf name is wanted.
   //
   return Names_back()->QualifiedName(scopes, templates);
}

//------------------------------------------------------------------------------

fn_name QualName_Referent = "QualName.Referent";

CxxNamed* QualName::Referent() const
{
   Debug::ft(QualName_Referent);

   //  This is invoked to find a referent in executable code.
   //
   if(ref_ != nullptr) return ref_;

   SymbolView view;
   auto item = ResolveLocal(&view);
   if(item == nullptr) return ReferentError(QualifiedName(true, true), 0);

   //  Verify that the item has a referent in case it's a typedef or a
   //  forward declaration.
   //
   ref_ = item->Referent();
   if(ref_ == nullptr) return ReferentError(item->Trace(), item->Type());
   using_ = view.using_;
   return ref_;
}

//------------------------------------------------------------------------------

fn_name QualName_ReferentError = "QualName.ReferentError";

CxxNamed* QualName::ReferentError(const string& item, debug32_t offset) const
{
   Debug::ft(QualName_ReferentError);

   auto expl = "Failed to find referent for " + item;
   Context::SwErr(QualName_ReferentError, expl, offset);
   ref_ = nullptr;
   return nullptr;
}

//------------------------------------------------------------------------------

fn_name QualName_ResolveTemplate = "QualName.ResolveTemplate";

bool QualName::ResolveTemplate(Class* cls, const TypeName* args, bool end) const
{
   Debug::ft(QualName_ResolveTemplate);

   //  If something within the template instance is being named,
   //  force its instantiation.
   //
   if(end) return true;
   auto inst = cls->EnsureInstance(args);
   if(inst == nullptr) return false;
   inst->Instantiate();
   return true;
}

//------------------------------------------------------------------------------

fn_name QualName_ResolveTypedef = "QualName.ResolveTypedef";

bool QualName::ResolveTypedef(Typedef* type) const
{
   Debug::ft(QualName_ResolveTypedef);

   SetTypedef(type);
   return true;
}

//------------------------------------------------------------------------------

fn_name QualName_SetForward = "QualName.SetForward";

void QualName::SetForward(CxxScoped* decl) const
{
   Debug::ft(QualName_SetForward);

   forw_ = decl;
}

//------------------------------------------------------------------------------

fn_name QualName_SetLocale = "QualName.SetLocale";

void QualName::SetLocale(Cxx::ItemType locale) const
{
   Debug::ft(QualName_SetLocale);

   auto size = Names_size();

   for(size_t i = 0; i < size; ++i)
   {
      Names_at(i)->SetLocale(locale);
   }
}

//------------------------------------------------------------------------------

fn_name QualName_SetOperator = "QualName.SetOperator";

void QualName::SetOperator(Cxx::Operator oper)
{
   Debug::ft(QualName_SetOperator);

   oper_ = oper;
   Names_back()->SetOperator(oper_);
}

//------------------------------------------------------------------------------

fn_name QualName_SetReferent = "QualName.SetReferent";

bool QualName::SetReferent(CxxNamed* ref) const
{
   Debug::ft(QualName_SetReferent);

   ref_ = ref;
   return true;
}

//------------------------------------------------------------------------------

fn_name QualName_SetTemplateParms = "QualName.SetTemplateParms";

void QualName::SetTemplateParms(TemplateParmsPtr& parms)
{
   Debug::ft(QualName_SetTemplateParms);

   parms_ = std::move(parms);
}

//------------------------------------------------------------------------------

fn_name QualName_SetTypedef = "QualName.SetTypedef";

void QualName::SetTypedef(CxxNamed* type) const
{
   Debug::ft(QualName_SetTypedef);

   type_ = type;
}

//------------------------------------------------------------------------------

void QualName::Shrink()
{
   for(size_t i = 0; i < Names_size(); ++i)
   {
      Names_at(i)->Shrink();
   }

   if(parms_ != nullptr) parms_->Shrink();

   if(qualified_)
   {
      auto size = names_->capacity() * sizeof(TypeNamePtr);
      CxxStats::Vectors(CxxStats::QUAL_NAME, size);
   }
}

//------------------------------------------------------------------------------

fn_name QualName_SubclassAccess = "QualName.SubclassAccess";

void QualName::SubclassAccess(Class* cls) const
{
   Debug::ft(QualName_SubclassAccess);

   class_ = cls;
}

//------------------------------------------------------------------------------

fn_name QualName_TypeString = "QualName.TypeString";

string QualName::TypeString(bool arg) const
{
   auto ref = Referent();

   if(ref != nullptr)
   {
      auto ts = ref->TypeString(arg);

      if(ref->IsTemplate())
      {
         ts += Names_back()->TypeString(arg);
      }

      return ts;
   }

   auto expl = "Failed to find referent for " + QualifiedName(true, true);
   Context::SwErr(QualName_TypeString, expl, 0);
   return ERROR_STR;
}

//==============================================================================

fn_name TemplateParm_ctor1 = "TemplateParm.ctor";

TemplateParm::TemplateParm
   (string& name, Cxx::ClassTag tag, size_t ptrs, TypeNamePtr& default) :
   tag_(tag),
   ptrs_(ptrs)
{
   Debug::ft(TemplateParm_ctor1);

   std::swap(name_, name);
   default_ = std::move(default);
   CxxStats::Incr(CxxStats::TEMPLATE_PARM);
}

//------------------------------------------------------------------------------

fn_name TemplateParm_ctor2 = "TemplateParm.ctor(copy)";

TemplateParm::TemplateParm(const TemplateParm& that) :
   name_(that.name_),
   tag_(that.tag_),
   ptrs_(that.ptrs_)
{
   Debug::ft(TemplateParm_ctor2);

   if(that.default_ != nullptr)
   {
      auto default = TypeNamePtr(new TypeName(*that.default_));
      default_ = std::move(default);
   }

   CxxStats::Incr(CxxStats::TEMPLATE_PARM);
}

//------------------------------------------------------------------------------

void TemplateParm::Print(ostream& stream) const
{
   stream << tag_ << SPACE;
   stream << *Name();
   if(ptrs_ > 0) stream << string(ptrs_, '*');
   if(default_ != nullptr)
   {
      stream << " = ";
      default_->Print(stream);
   }
}

//------------------------------------------------------------------------------

void TemplateParm::Shrink()
{
   name_.shrink_to_fit();
   CxxStats::Strings(CxxStats::TEMPLATE_PARM, name_.capacity());
}

//------------------------------------------------------------------------------

string TemplateParm::TypeString(bool arg) const
{
   auto ts = *Name();
   if(ptrs_ > 0) ts += string(ptrs_, '*');
   return ts;
}

//==============================================================================

fn_name TemplateParms_ctor1 = "TemplateParms.ctor";

TemplateParms::TemplateParms(TemplateParmPtr& parm)
{
   Debug::ft(TemplateParms_ctor1);

   parms_.push_back(std::move(parm));
   CxxStats::Incr(CxxStats::TEMPLATE_PARMS);
}

//------------------------------------------------------------------------------

fn_name TemplateParms_ctor2 = "TemplateParms.ctor(copy)";

TemplateParms::TemplateParms(const TemplateParms& that)
{
   Debug::ft(TemplateParms_ctor2);

   for(auto p = that.parms_.cbegin(); p != that.parms_.cend(); ++p)
   {
      auto parm = TemplateParmPtr(new TemplateParm(**p));
      parms_.push_back(std::move(parm));
   }

   CxxStats::Incr(CxxStats::TEMPLATE_PARMS);
}

//------------------------------------------------------------------------------

fn_name TemplateParms_AddParm = "TemplateParms.AddParm";

void TemplateParms::AddParm(TemplateParmPtr& parm)
{
   Debug::ft(TemplateParms_AddParm);

   parms_.push_back(std::move(parm));
}

//------------------------------------------------------------------------------

void TemplateParms::Print(ostream& stream) const
{
   stream << TEMPLATE_STR << '<';

   for(auto p = parms_.cbegin(); p != parms_.cend(); ++p)
   {
      (*p)->Print(stream);
      if(*p != parms_.back()) stream << ',';
   }

   stream << "> ";
}

//------------------------------------------------------------------------------

void TemplateParms::Shrink()
{
   for(auto p = parms_.cbegin(); p != parms_.cend(); ++p)
   {
      (*p)->Shrink();
   }

   auto size = parms_.capacity() * sizeof(TemplateParmPtr);
   CxxStats::Vectors(CxxStats::TEMPLATE_PARMS, size);
}

//------------------------------------------------------------------------------

string TemplateParms::TypeString(bool arg) const
{
   string ts = "<";

   for(auto p = parms_.cbegin(); p != parms_.cend(); ++p)
   {
      ts += (*p)->TypeString(false);
      if(*p != parms_.back()) ts += ',';
   }

   return ts + '>';
}

//==============================================================================

fn_name TypeName_ctor1 = "TypeName.ctor";

TypeName::TypeName(string& name) : args_(nullptr)
{
   Debug::ft(TypeName_ctor1);

   std::swap(name_, name);
   CxxStats::Incr(CxxStats::TYPE_NAME);
}

//------------------------------------------------------------------------------

fn_name TypeName_ctor2 = "TypeName.ctor(copy)";

TypeName::TypeName(const TypeName& that) : CxxNamed(that),
   name_(that.name_)
{
   Debug::ft(TypeName_ctor2);

   if(that.args_ != nullptr)
   {
      args_.reset(new TypeSpecPtrVector);

      for(auto a = that.args_->cbegin(); a != that.args_->cend(); ++a)
      {
         auto arg = TypeSpecPtr((*a)->Clone());
         args_->push_back(std::move(arg));
      }
   }

   CxxStats::Incr(CxxStats::TYPE_NAME);
}

//------------------------------------------------------------------------------

fn_name TypeName_dtor = "TypeName.dtor";

TypeName::~TypeName()
{
   Debug::ft(TypeName_dtor);

   CxxStats::Decr(CxxStats::TYPE_NAME);
}

//------------------------------------------------------------------------------

fn_name TypeName_AddTemplateArg = "TypeName.AddTemplateArg";

void TypeName::AddTemplateArg(TypeSpecPtr& arg)
{
   Debug::ft(TypeName_AddTemplateArg);

   if(args_ == nullptr) args_.reset(new TypeSpecPtrVector);
   arg->SetTemplateRole(TemplateArgument);
   args_->push_back(std::move(arg));
}

//------------------------------------------------------------------------------

fn_name TypeName_Append = "TypeName.Append";

void TypeName::Append(const string& name)
{
   Debug::ft(TypeName_Append);

   name_ += name;
}

//------------------------------------------------------------------------------

fn_name TypeName_FindReferent = "TypeName.FindReferent";

bool TypeName::FindReferent()
{
   Debug::ft(TypeName_FindReferent);

   if(args_ != nullptr)
   {
      for(auto a = args_->cbegin(); a != args_->cend(); ++a)
      {
         (*a)->FindReferent();
      }
   }

   return true;
}

//------------------------------------------------------------------------------

fn_name TypeName_GetTemplateArgs = "TypeName.GetTemplateArgs";

TypeName* TypeName::GetTemplateArgs() const
{
   Debug::ft(TypeName_GetTemplateArgs);

   if(args_ == nullptr) return nullptr;
   return const_cast< TypeName* >(this);
}

//------------------------------------------------------------------------------

fn_name TypeName_GetUsages = "TypeName.GetUsages";

void TypeName::GetUsages(const CodeFile& file, CxxUsageSets& symbols) const
{
   Debug::ft(TypeName_GetUsages);

   if(args_ != nullptr)
   {
      for(auto a = args_->cbegin(); a != args_->cend(); ++a)
      {
         (*a)->GetUsages(file, symbols);
      }
   }
}

//------------------------------------------------------------------------------

fn_name TypeName_HasTemplateParmFor = "TypeName.HasTemplateParmFor";

bool TypeName::HasTemplateParmFor(const CxxScope* scope) const
{
   Debug::ft(TypeName_HasTemplateParmFor);

   if(args_ != nullptr)
   {
      for(auto a = args_->cbegin(); a != args_->cend(); ++a)
      {
         if(scope->NameIsTemplateParm(*(*a)->Name())) return true;
      }
   }

   return false;
}

//------------------------------------------------------------------------------

fn_name TypeName_Instantiating = "TypeName.Instantiating";

void TypeName::Instantiating() const
{
   Debug::ft(TypeName_Instantiating);

   if(args_ != nullptr)
   {
      for(auto a = args_->cbegin(); a != args_->cend(); ++a)
      {
         (*a)->Instantiating();
      }
   }
}

//------------------------------------------------------------------------------

fn_name TypeName_MatchTemplate = "TypeName.MatchTemplate";

TypeMatch TypeName::MatchTemplate(const TypeName* that,
   stringVector& tmpltParms, stringVector& tmpltArgs, bool& argFound) const
{
   Debug::ft(TypeName_MatchTemplate);

   if(this->args_ == nullptr) return Compatible;
   auto thisSize = this->args_->size();
   if(thisSize == 0) return Compatible;
   if(thisSize != that->args_->size()) return Incompatible;

   auto match = Compatible;

   for(size_t i = 0 ; i < thisSize; ++i)
   {
      auto result = this->args_->at(i)->MatchTemplate
         (that->args_->at(i).get(), tmpltParms, tmpltArgs, argFound);
      if(result == Incompatible) return Incompatible;
      if(result < match) match = result;
   }

   return match;
}

//------------------------------------------------------------------------------

void TypeName::Print(ostream& stream) const
{
   stream << *Name();

   if(args_ != nullptr)
   {
      stream << '<';

      for(auto a = args_->cbegin(); a != args_->cend(); ++a)
      {
         (*a)->Print(stream);
         if(*a != args_->back()) stream << ',';
      }

      stream << '>';
   }
}

//------------------------------------------------------------------------------

string TypeName::QualifiedName(bool scopes, bool templates) const
{
   if(!templates || (args_ == nullptr)) return name_;

   auto qname = name_ + '<';

   for(auto a = args_->cbegin(); a != args_->cend(); ++a)
   {
      qname += (*a)->QualifiedName(scopes, templates);
      if(*a != args_->back()) qname += ',';
   }

   return qname + '>';
}

//------------------------------------------------------------------------------

fn_name TypeName_SetLocale = "TypeName.SetLocale";

void TypeName::SetLocale(Cxx::ItemType locale) const
{
   Debug::ft(TypeName_SetLocale);

   if(args_ != nullptr)
   {
      for(auto a = args_->cbegin(); a != args_->cend(); ++a)
      {
         (*a)->SetLocale(locale);
      }
   }
}

//------------------------------------------------------------------------------

fn_name TypeName_SetOperator = "TypeName.SetOperator";

void TypeName::SetOperator(Cxx::Operator oper)
{
   Debug::ft(TypeName_SetOperator);

   switch(oper)
   {
   case Cxx::NIL_OPERATOR:
   case Cxx::CAST:
      //
      //  This either isn't an operator, or it's a conversion operator.  The
      //  name doesn't change in either case.  For the latter, the name is
      //  simply left as "operator", which will display as operator type()
      //  rather than operator() type().
      //
      break;

   default:
      if(oper != Cxx::NIL_OPERATOR)
      {
         //  For a function template instance, the name already includes the
         //  operator.  The template arguments have also been appended to the
         //  name, so leave it alone.
         //
         auto name = CxxOp::OperatorToName(oper);

         if(name_.compare(0, name.size(), name) != 0)
         {
            name_ = CxxOp::OperatorToName(oper);
         }
      }
   }
}

//------------------------------------------------------------------------------

fn_name TypeName_SetTemplateRole = "TypeName.SetTemplateRole";

void TypeName::SetTemplateRole(TemplateRole role) const
{
   Debug::ft(TypeName_SetTemplateRole);

   if(args_ != nullptr)
   {
      for(auto a = args_->cbegin(); a != args_->cend(); ++a)
      {
         (*a)->SetTemplateRole(role);
      }
   }
}

//------------------------------------------------------------------------------

void TypeName::Shrink()
{
   name_.shrink_to_fit();

   CxxStats::Strings(CxxStats::TYPE_NAME, name_.capacity());

   if(args_ != nullptr)
   {
      for(auto a = args_->cbegin(); a != args_->cend(); ++a)
      {
         (*a)->Shrink();
      }

      auto size = args_->capacity() * sizeof(TypeSpecPtr);
      CxxStats::Vectors(CxxStats::TYPE_NAME, size);
   }
}

//------------------------------------------------------------------------------

string TypeName::TypeString(bool arg) const
{
   if(args_ == nullptr) return EMPTY_STR;

   string ts = "<";

   for(auto a = args_->cbegin(); a != args_->cend(); ++a)
   {
      ts += (*a)->TypeString(false);
      if(*a != args_->back()) ts += ',';
   }

   return ts + '>';
}

//==============================================================================

fn_name TypeSpec_ctor1 = "TypeSpec.ctor";

TypeSpec::TypeSpec() :
   locale_(Cxx::Operation),
   role_(TemplateNone)
{
   Debug::ft(TypeSpec_ctor1);
}

//------------------------------------------------------------------------------

fn_name TypeSpec_ctor2 = "TypeSpec.ctor(copy)";

TypeSpec::TypeSpec(const TypeSpec& that) : CxxNamed(that),
   locale_(that.locale_),
   role_(that.role_)
{
   Debug::ft(TypeSpec_ctor2);
}

//------------------------------------------------------------------------------

fn_name TypeSpec_PureVirtualFunction = "TypeSpec.PureVirtualFunction";

//------------------------------------------------------------------------------

void TypeSpec::AddArray(ArraySpecPtr& array)
{
   Debug::SwErr(TypeSpec_PureVirtualFunction, "AddArray", 0);
}

//------------------------------------------------------------------------------

void TypeSpec::AdjustPtrs(TagCount count)
{
   Debug::SwErr(TypeSpec_PureVirtualFunction, "AdjustPtrs", 0);
}

//------------------------------------------------------------------------------

string TypeSpec::AlignTemplateArg(const TypeSpec* thatArg) const
{
   Debug::SwErr(TypeSpec_PureVirtualFunction, "AlignTemplateArg", 0);
   return ERROR_STR;
}

//------------------------------------------------------------------------------

TagCount TypeSpec::ArrayCount() const
{
   Debug::SwErr(TypeSpec_PureVirtualFunction, "ArrayCount", 0);
   return 0;
}

//------------------------------------------------------------------------------

TagCount TypeSpec::Arrays() const
{
   Debug::SwErr(TypeSpec_PureVirtualFunction, "Arrays", 0);
   return 0;
}

//------------------------------------------------------------------------------

TypeSpec* TypeSpec::Clone() const
{
   Debug::SwErr(TypeSpec_PureVirtualFunction, "Clone", 0);
   return nullptr;
}

//------------------------------------------------------------------------------

void TypeSpec::DisplayArrays(ostream& stream) const
{
   Debug::SwErr(TypeSpec_PureVirtualFunction, "DisplayArrays", 0);
}

//------------------------------------------------------------------------------

void TypeSpec::DisplayTags(ostream& stream) const
{
   Debug::SwErr(TypeSpec_PureVirtualFunction, "DisplayTags", 0);
}

//------------------------------------------------------------------------------

void TypeSpec::EnterArrays() const
{
   Debug::SwErr(TypeSpec_PureVirtualFunction, "EnterArrays", 0);
}

//------------------------------------------------------------------------------

void TypeSpec::EnteringScope(const CxxScope* scope)
{
   Debug::SwErr(TypeSpec_PureVirtualFunction, "EnteringScope", 0);
}

//------------------------------------------------------------------------------

TypeTags TypeSpec::GetTags() const
{
   TypeTags tags;

   Debug::SwErr(TypeSpec_PureVirtualFunction, "GetTags", 0);
   return tags;
}

//------------------------------------------------------------------------------

bool TypeSpec::HasArrayDefn() const
{
   Debug::SwErr(TypeSpec_PureVirtualFunction, "HasArrayDefn", 0);
   return false;
}

//------------------------------------------------------------------------------

void TypeSpec::Instantiating() const
{
   Debug::SwErr(TypeSpec_PureVirtualFunction, "Instantiating", 0);
}

//------------------------------------------------------------------------------

bool TypeSpec::MatchesExactly(const TypeSpec* that) const
{
   Debug::SwErr(TypeSpec_PureVirtualFunction, "MatchesExactly", 0);
   return false;
}

//------------------------------------------------------------------------------

TypeMatch TypeSpec::MatchTemplate(TypeSpec* that, stringVector& tmpltParms,
   stringVector& tmpltArgs, bool& argFound) const
{
   Debug::SwErr(TypeSpec_PureVirtualFunction, "MatchTemplate", 0);
   return Incompatible;
}

//------------------------------------------------------------------------------

TypeMatch TypeSpec::MatchTemplateArg(const TypeSpec* that) const
{
   Debug::SwErr(TypeSpec_PureVirtualFunction, "MatchTemplateArg", 0);
   return Incompatible;
}

//------------------------------------------------------------------------------

fn_name TypeSpec_MustMatchWith = "TypeSpec.MustMatchWith";

TypeMatch TypeSpec::MustMatchWith(const StackArg& that, bool implicit) const
{
   Debug::ft(TypeSpec_MustMatchWith);

   if(GetTemplateRole() == TemplateParameter) return Compatible;

   auto thisType = this->TypeString(true);
   auto thatType = that.TypeString(true);
   auto match = ResultType().CalcMatchWith(that, thisType, thatType, implicit);

   if(match == Incompatible)
   {
      auto expl = thisType + " is incompatible with " + thatType;
      Context::SwErr(TypeSpec_MustMatchWith, expl, 0);
   }

   return match;
}

//------------------------------------------------------------------------------

TagCount TypeSpec::PtrCount(bool arrays) const
{
   Debug::SwErr(TypeSpec_PureVirtualFunction, "PtrCount", 0);
   return 0;
}

//------------------------------------------------------------------------------

TagCount TypeSpec::Ptrs(bool arrays) const
{
   Debug::SwErr(TypeSpec_PureVirtualFunction, "Ptrs", 0);
   return 0;
}

//------------------------------------------------------------------------------

TagCount TypeSpec::RefCount() const
{
   Debug::SwErr(TypeSpec_PureVirtualFunction, "RefCount", 0);
   return 0;
}

//------------------------------------------------------------------------------

TagCount TypeSpec::Refs() const
{
   Debug::SwErr(TypeSpec_PureVirtualFunction, "Refs", 0);
   return 0;
}

//------------------------------------------------------------------------------

void TypeSpec::RemoveRefs()
{
   Debug::SwErr(TypeSpec_PureVirtualFunction, "RemoveRefs", 0);
}

//------------------------------------------------------------------------------

StackArg TypeSpec::ResultType() const
{
   Debug::SwErr(TypeSpec_PureVirtualFunction, "ResultType", 0);
   return NilStackArg;
}

//------------------------------------------------------------------------------

void TypeSpec::SetArrayPos(int8_t pos)
{
   Debug::SwErr(TypeSpec_PureVirtualFunction, "SetArrayPos", 0);
}

//------------------------------------------------------------------------------

void TypeSpec::SetConst(bool readonly)
{
   Debug::SwErr(TypeSpec_PureVirtualFunction, "SetConst", 0);
}

//------------------------------------------------------------------------------

void TypeSpec::SetConstPtr(bool constptr)
{
   Debug::SwErr(TypeSpec_PureVirtualFunction, "SetConstPtr", 0);
}

//------------------------------------------------------------------------------

fn_name TypeSpec_SetLocale = "TypeSpec.SetLocale";

void TypeSpec::SetLocale(Cxx::ItemType locale)
{
   Debug::ft(TypeSpec_SetLocale);

   locale_ = locale;
}

//------------------------------------------------------------------------------

void TypeSpec::SetPtrDetached(bool on)
{
   Debug::SwErr(TypeSpec_PureVirtualFunction, "SetPtrDetached", 0);
}

//------------------------------------------------------------------------------

void TypeSpec::SetPtrs(TagCount ptrs)
{
   Debug::SwErr(TypeSpec_PureVirtualFunction, "SetPtrs", 0);
}

//------------------------------------------------------------------------------

void TypeSpec::SetRefDetached(bool on)
{
   Debug::SwErr(TypeSpec_PureVirtualFunction, "SetRefDetached", 0);
}

//------------------------------------------------------------------------------

void TypeSpec::SetReferent(CxxNamed* ref, bool use)
{
   Debug::SwErr(TypeSpec_PureVirtualFunction, "SetReferent", 0);
}

//------------------------------------------------------------------------------

void TypeSpec::SetRefs(TagCount refs)
{
   Debug::SwErr(TypeSpec_PureVirtualFunction, "SetRefs", 0);
}

//------------------------------------------------------------------------------

string TypeSpec::TypeTagsString(const TypeTags& tags) const
{
   Debug::SwErr(TypeSpec_PureVirtualFunction, "TypeTagsString", 0);
   return ERROR_STR;
}

//==============================================================================

TypeTags::TypeTags() :
   const_(false),
   constptr_(false),
   arrays_(0),
   ptrs_(0),
   refs_(0)
{
}

//------------------------------------------------------------------------------

TypeTags::TypeTags(const TypeSpec& spec) :
   const_(spec.IsConst()),
   constptr_(spec.IsConstPtr()),
   arrays_(spec.Arrays()),
   ptrs_(spec.Ptrs(true) - arrays_),
   refs_(spec.Refs())
{
}
}
