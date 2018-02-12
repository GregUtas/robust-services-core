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
#include "Lexer.h"
#include "Library.h"
#include "Parser.h"
#include "Registry.h"
#include "Singleton.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace CodeTools
{
fn_name CodeTools_ReferentError = "CodeTools.ReferentError";

CxxNamed* ReferentError(const string& item, debug32_t offset)
{
   Debug::ft(CodeTools_ReferentError);

   auto expl = "Failed to find referent for " + item;
   Context::SwErr(CodeTools_ReferentError, expl, offset);
   return nullptr;
}

//==============================================================================

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

void CxxNamed::FindReferent()
{
   Debug::ft(CxxNamed_FindReferent);

   auto expl = "FindReferent() not implemented by " + strClass(this, false);
   Context::SwErr(CxxNamed_FindReferent, expl, 0);
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

   if((qname->Size() == 1) && !qname->IsGlobal())
   {
      auto item = syms->FindLocal(*Name(), view);

      if(item != nullptr)
      {
         qname->SetReferentN(0, item, view);
         return item;
      }
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
   auto size = qname->Size();
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
      //  Start with the first name in the qualified name.  Return if
      //  it refers to itself, which can occur for a friend declaration.
      //
      name = *qname->At(0)->Name();
      item = syms->FindSymbol(file, scope, name, selector, view);
      qname->SetReferentN(0, item, view);
      if(item == this) return item;
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
         name += *qname->At(idx)->Name();
         item = nullptr;
         if(++idx >= size)
         {
            selector = mask;
            if(func != nullptr)
            {
               *view = DeclaredLocally;
               item = space->MatchFunc(func, false);
            }
         }
         if(item == nullptr)
         {
            *view = NotAccessible;
            item = syms->FindSymbol(file, scope, name, selector, view, space);
            if(name.find(SCOPE_STR) != string::npos) view->using_ = false;
         }
         qname->SetReferentN(idx - 1, item, view);
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
            auto args = qname->At(idx - 1)->GetTemplateArgs();
            if(args == nullptr) break;
            if(args->HasTemplateParmFor(scope)) break;
            if(!ResolveTemplate(cls, args, (idx >= size))) break;
            cls = cls->EnsureInstance(args);
            item = cls;
            qname->SetReferentN(idx - 1, item, view);  // updated value
            if(item == nullptr) return nullptr;
            if(idx < size) cls->Instantiate();
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
            if(func != nullptr)
            {
               *view = DeclaredLocally;
               item = cls->MatchFunc(func, true);
            }
         }
         if(item == nullptr)
         {
            *view = NotAccessible;
            item = cls->FindMember(name, true, scope, view);
         }
         qname->SetReferentN(idx - 1, item, view);
         if(item == nullptr) return nullptr;
         if(item->GetClass() != cls) qname->At(idx - 1)->SubclassAccess(cls);
         break;

      case Cxx::Enum:
         //
         //  If there is another name, resolve it within this namespace,
         //  else return the enum itself.
         //
         if(idx >= size) return item;
         name = *qname->At(idx)->Name();
         item = static_cast< Enum* >(item)->FindEnumerator(name);
         *view = DeclaredLocally;
         qname->SetReferentN(idx, item, view);
         return item;

      case Cxx::Typedef:
         {
            //  See if the item wants to resolve the typedef.  In case the
            //  typedef is that of a template, instantiate it if a template
            //  member is being named.
            //
            auto tdef = static_cast< Typedef* >(item);
            tdef->SetAsReferent(this);
            if(!ResolveTypedef(tdef, idx - 1)) return tdef;
            auto root = tdef->Root();
            if(root == nullptr) return tdef;
            item = static_cast< CxxScoped* >(root);
            qname->SetReferentN(idx - 1, item, view);  // updated value
            if(idx < size) item->Instantiate();
         }
         break;

      case Cxx::Forward:
      case Cxx::Friend:
         {
            if(!ResolveForward(item, idx - 1)) return item;
            auto ref = item->Referent();
            if(ref == nullptr) return item;
            item = static_cast< CxxScoped* >(ref);
            qname->SetReferentN(idx - 1, item, view);  // updated value
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

fn_name CxxNamed_SetReferent = "CxxNamed.SetReferent";

void CxxNamed::SetReferent(CxxNamed* item, const SymbolView* view) const
{
   Debug::ft(CxxNamed_SetReferent);

   auto expl = "SetReferent() not implemented by " + strClass(this, false);
   Context::SwErr(CxxNamed_FindReferent, expl, 0);
}

//------------------------------------------------------------------------------

fn_name CxxNamed_SetTemplateParms = "CxxNamed.SetTemplateParms";

void CxxNamed::SetTemplateParms(TemplateParmsPtr& parms)
{
   Debug::ft(CxxNamed_SetTemplateParms);

   auto expl = "Template parameters not supported by " + Trace();
   Context::SwErr(CxxNamed_SetTemplateParms, expl, 0);
}

//------------------------------------------------------------------------------

string CxxNamed::strLocation() const
{
   auto file = GetFile();
   if(file == nullptr) return "unknown location";

   std::ostringstream stream;
   stream << file->Name() << ", line ";
   stream << file->GetLexer().GetLineNum(GetPos()) + 1;
   return stream.str();
}

//------------------------------------------------------------------------------

void CxxNamed::strName(ostream& stream, bool fq, const QualName* name) const
{
   if(fq)
      stream << ScopedName(true);
   else
      name->Print(stream, Flags());
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
         (*a)->Print(stream, Flags());
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

   Context::SetPos(GetPos());

   if(scope->NameIsTemplateParm(*Name()))
   {
      SetTemplateRole(TemplateParameter);
   }

   EnterArrays();
   if(name_->GetReferent() == nullptr) FindReferent();
}

//------------------------------------------------------------------------------

fn_name DataSpec_FindReferent = "DataSpec.FindReferent";

void DataSpec::FindReferent()
{
   Debug::ft(DataSpec_FindReferent);

   //  Find referents for any template arguments used in the type's name.
   //  Bypass name_ itself; a QualName only finds its referent when used
   //  in executable code.
   //
   for(auto n = name_->First(); n != nullptr; n = n->Next())
   {
      n->FindReferent();
   }

   //  This should find a referent during parsing, when there is a context
   //  file.  If it isn't found then, it's pointless to look later.
   //
   auto file = Context::File();
   if(file == nullptr) return;
   auto scope = Context::Scope();
   if(scope == nullptr) return;

   if(ResolveTemplateArgument()) return;

   SymbolView view;
   auto item = ResolveName(file, scope, TYPESPEC_REFS, &view);

   if(item != nullptr)
   {
      SetReferent(item, &view);
      return;
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
      return;
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
      view = NotAccessible;
      item = syms->FindSymbol(file, scope, qname, VALUE_REFS, &view);
      if(item != nullptr) SetReferent(item, &view);
      //  [[fallthrough]]
   case TemplateParameter:
   case TemplateClass:
      //
      //  When Operation.ExecuteOverload checks if a function overload applies,
      //  Function.MatchTemplate may create the DataSpec for a class template
      //  that defines an operator at file scope, like operator<< for a string.
      //  In this case, the class template may not even be visible in the scope
      //  where the possibility of the overload is being checked.
      //
      return;
   }

   //  When parsing a template instance, the arguments may not be visible,
   //  because the scope is the template instance itself.  For example, the
   //  type A is often not visible in the scope std::unique_ptr<A>.
   //
   if(scope->IsInTemplateInstance()) return;

   //  The referent couldn't be found.
   //
   auto expl = "Failed to find referent for " + qname;
   Context::SwErr(DataSpec_FindReferent, expl, 0);
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
   for(auto n = name_->First(); n != nullptr; n = n->Next())
   {
      n->GetUsages(file, symbols);
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
      auto log = "Unknown type for " + qname + " [" + strLocation() + ']';
      Debug::SwErr(DataSpec_GetUsages, log, 0, InfoLog);
      return;
   }

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
      //  [[fallthrough]]
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

   if(idx != string::npos)
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

void DataSpec::Print(ostream& stream, const Flags& options) const
{
   if(const_) stream << CONST_STR << SPACE;
   name_->Print(stream, options);
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

bool DataSpec::ResolveForward(CxxScoped* decl, size_t n) const
{
   Debug::ft(DataSpec_ResolveForward);

   //  Stop at the forward declaration unless it's a template.  If it is,
   //  continue so that template arguments can be applied to its referent,
   //  provided that it has already been found.
   //
   if(decl->IsTemplate())
   {
      name_->At(n)->SetForward(decl);
      decl->SetAsReferent(this);
      return (decl->Referent() != nullptr);
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

   SetReferent(item, nullptr);
   return true;
}

//------------------------------------------------------------------------------

fn_name DataSpec_ResolveTypedef = "DataSpec.ResolveTypedef";

bool DataSpec::ResolveTypedef(Typedef* type, size_t n) const
{
   Debug::ft(DataSpec_ResolveTypedef);

   //  Stop at the typedef unless it has template arguments.  If it does,
   //  delegate to name_, which will record it as a referent and resolve
   //  it to the template instance.
   //
   if(type->GetTemplateArgs() == nullptr) return false;
   return name_->ResolveTypedef(type, n);
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

void DataSpec::SetReferent(CxxNamed* item, const SymbolView* view) const
{
   Debug::ft(DataSpec_SetReferent);

   //  If ITEM is an unresolved forward declaration for a template, our referent
   //  needs to be a template instance instantiated from that template.  This is
   //  not yet possible, so make sure that our referent is empty so that we will
   //  revisit it.
   //
   if((item->IsTemplate() && item->Referent() == nullptr))
   {
      name_->SetReferent(nullptr, nullptr);
   }
   else
   {
      name_->SetReferent(item, view);

      if(item->Type() != Cxx::Typedef)
      {
         //  SetAsReferent has already been invoked if our referent is a
         //  typedef, so don't invoke it again.
         //
         item->SetAsReferent(this);
      }
      else
      {
         //  If our referent is a pointer typedef, "const" applies to the
         //  pointer, not its target.
         //
         if(const_ && (item->GetTypeSpec()->Ptrs(false) > 0))
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
      for(auto n = name_->First(); n != nullptr; n = n->Next())
      {
         n->SetTemplateRole(TemplateParameter);
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

void MemberInit::Print(ostream& stream, const Flags& options) const
{
   stream << name_;
   init_->Print(stream, options);
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

QualName::QualName(TypeNamePtr& type)
{
   Debug::ft(QualName_ctor1);

   first_ = std::move(type);
   CxxStats::Incr(CxxStats::QUAL_NAME);
}

//------------------------------------------------------------------------------

fn_name QualName_ctor2 = "QualName.ctor(string)";

QualName::QualName(const string& name)
{
   Debug::ft(QualName_ctor2);

   auto copy = name;
   first_ = (TypeNamePtr(new TypeName(copy)));
   CxxStats::Incr(CxxStats::QUAL_NAME);
}

//------------------------------------------------------------------------------

fn_name QualName_ctor3 = "QualName.ctor(copy)";

QualName::QualName(const QualName& that) : CxxNamed(that)
{
   Debug::ft(QualName_ctor3);

   for(auto n = that.First(); n != nullptr; n = n->Next())
   {
      auto thisName = new TypeName(*n);
      PushBack(TypeNamePtr(thisName));
   }

   CxxStats::Incr(CxxStats::QUAL_NAME);
}

//------------------------------------------------------------------------------

fn_name QualName_dtor = "QualName.dtor";

QualName::~QualName()
{
   Debug::ft(QualName_dtor);

   CxxStats::Decr(CxxStats::QUAL_NAME);
}

//------------------------------------------------------------------------------

fn_name QualName_Append = "QualName.Append";

void QualName::Append(const string& name, bool space) const
{
   Debug::ft(QualName_Append);

   Last()->Append(name, space);
}

//------------------------------------------------------------------------------

TypeName* QualName::At(size_t n) const
{
   for(auto i = First(); i != nullptr; i = i->Next())
   {
      if(n == 0)
         return i;
      else
         --n;
   }

   return nullptr;
}

//------------------------------------------------------------------------------

fn_name QualName_Check = "QualName.Check";

void QualName::Check() const
{
   Debug::ft(QualName_Check);

   for(auto n = First(); n != nullptr; n = n->Next())
   {
      n->Check();
   }
}

//------------------------------------------------------------------------------

fn_name QualName_CheckCtorDefn = "QualName.CheckCtorDefn";

bool QualName::CheckCtorDefn() const
{
   Debug::ft(QualName_CheckCtorDefn);

   auto size = Size();
   if(size <= 1) return false;
   return (*At(size - 1)->Name() == *At(size - 2)->Name());
}

//------------------------------------------------------------------------------

fn_name QualName_EnterBlock = "QualName.EnterBlock";

void QualName::EnterBlock()
{
   Debug::ft(QualName_EnterBlock);

   Context::SetPos(GetPos());
   auto name = *Name();
   if(name == "NULL") Log(UseOfNull);

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

CxxScoped* QualName::GetForward() const
{
   CxxScoped* forw = nullptr;

   for(auto n = First(); n != nullptr; n = n->Next())
   {
      auto f = n->GetForward();
      if(f != nullptr) forw = f;
   }

   return forw;
}

//------------------------------------------------------------------------------

fn_name QualName_GetTemplateArgs = "QualName.GetTemplateArgs";

TypeName* QualName::GetTemplateArgs() const
{
   Debug::ft(QualName_GetTemplateArgs);

   for(auto n = First(); n != nullptr; n = n->Next())
   {
      auto spec = n->GetTemplateArgs();
      if(spec != nullptr) return spec;
   }

   return nullptr;
}

//------------------------------------------------------------------------------

fn_name QualName_GetUsages = "QualName.GetUsages";

void QualName::GetUsages(const CodeFile& file, CxxUsageSets& symbols) const
{
   Debug::ft(QualName_GetUsages);

   //  Get the usages for each individual name.
   //
   for(auto n = First(); n != nullptr; n = n->Next())
   {
      n->GetUsages(file, symbols);
   }

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
}

//------------------------------------------------------------------------------

TypeName* QualName::Last() const
{
   TypeName* prev = nullptr;

   for(auto curr = First(); curr != nullptr; curr = curr->Next())
   {
      prev = curr;
   }

   return prev;
}

//------------------------------------------------------------------------------

fn_name QualName_MatchTemplate = "QualName.MatchTemplate";

TypeMatch QualName::MatchTemplate(const QualName* that,
   stringVector& tmpltParms, stringVector& tmpltArgs, bool& argFound) const
{
   Debug::ft(QualName_MatchTemplate);

   auto match = Compatible;
   auto size = Size();
   auto n1 = this->First();
   auto n2 = that->First();

   for(size_t i = 0; i < size; ++i)
   {
      auto result = n1->MatchTemplate(n2, tmpltParms, tmpltArgs, argFound);
      if(result == Incompatible) return Incompatible;
      if(result < match) match = result;
      n1 = n1->Next();
      n2 = n2->Next();
   }

   return match;
}

//------------------------------------------------------------------------------

fn_name QualName_MemberAccessed = "QualName.MemberAccessed";

void QualName::MemberAccessed(Class* cls, CxxNamed* mem) const
{
   Debug::ft(QualName_MemberAccessed);

   for(auto n = First(); n != nullptr; n = n->Next())
   {
      if(n->Referent() == nullptr)
      {
         if(*n->Name() == *mem->Name())
         {
            n->MemberAccessed(cls, mem);
            return;
         }
      }
   }

   auto expl = "Could not find member name for " + *mem->Name();
   Context::SwErr(QualName_MemberAccessed, expl, 0);
}

//------------------------------------------------------------------------------

void QualName::Print(ostream& stream, const Flags& options) const
{
   for(auto n = First(); n != nullptr; n = n->Next())
   {
      n->Print(stream, options);
   }
}

//------------------------------------------------------------------------------

fn_name QualName_PushBack = "QualName.PushBack";

void QualName::PushBack(TypeNamePtr& type)
{
   Debug::ft(QualName_PushBack);

   if(first_ == nullptr)
      first_ = std::move(type);
   else
      Last()->PushBack(type);
}

//------------------------------------------------------------------------------

string QualName::QualifiedName(bool scopes, bool templates) const
{
   if(scopes)
   {
      //  Build the qualified name.
      //
      string qname;

      for(auto n = First(); n != nullptr; n = n->Next())
      {
         qname += n->QualifiedName(scopes, templates);
      }

      return qname;
   }

   //  Only the last name is wanted.
   //
   return Last()->QualifiedName(scopes, templates);
}

//------------------------------------------------------------------------------

fn_name QualName_Referent = "QualName.Referent";

CxxNamed* QualName::Referent() const
{
   Debug::ft(QualName_Referent);

   //  This is invoked to find a referent in executable code.
   //
   auto ref = Last()->Referent();
   if(ref != nullptr) return ref;

   SymbolView view;
   auto item = ResolveLocal(&view);
   if(item == nullptr) return ReferentError(QualifiedName(true, true), 0);

   //  Verify that the item has a referent in case it's a typedef or a
   //  forward declaration.
   //
   ref = item->Referent();
   if(ref == nullptr) return ReferentError(item->Trace(), item->Type());
   return ref;
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

bool QualName::ResolveTypedef(Typedef* type, size_t n) const
{
   Debug::ft(QualName_ResolveTypedef);

   return At(n)->ResolveTypedef(type, n);
}

//------------------------------------------------------------------------------

fn_name QualName_SetLocale = "QualName.SetLocale";

void QualName::SetLocale(Cxx::ItemType locale) const
{
   Debug::ft(QualName_SetLocale);

   for(auto n = First(); n != nullptr; n = n->Next())
   {
      n->SetLocale(locale);
   }
}

//------------------------------------------------------------------------------

fn_name QualName_SetOperator = "QualName.SetOperator";

void QualName::SetOperator(Cxx::Operator oper) const
{
   Debug::ft(QualName_SetOperator);

   Last()->SetOperator(oper);
}

//------------------------------------------------------------------------------

fn_name QualName_SetReferent = "QualName.SetReferent";

void QualName::SetReferent(CxxNamed* item, const SymbolView* view) const
{
   Debug::ft(QualName_SetReferent);

   Last()->SetReferent(item, view);
}

//------------------------------------------------------------------------------

fn_name QualName_SetReferentN = "QualName.SetReferentN";

void QualName::SetReferentN
   (size_t n, CxxNamed* item, const SymbolView* view) const
{
   Debug::ft(QualName_SetReferentN);

   At(n)->SetReferent(item, view);
}

//------------------------------------------------------------------------------

void QualName::Shrink()
{
   for(auto n = First(); n != nullptr; n = n->Next())
   {
      n->Shrink();
   }
}

//------------------------------------------------------------------------------

size_t QualName::Size() const
{
   size_t s = 0;
   for(auto n = First(); n != nullptr; n = n->Next()) ++s;
   return s;
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
         ts += Last()->TypeString(arg);
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
   (string& name, Cxx::ClassTag tag, size_t ptrs, TypeNamePtr& preset) :
   tag_(tag),
   ptrs_(ptrs)
{
   Debug::ft(TemplateParm_ctor1);

   std::swap(name_, name);
   default_ = std::move(preset);
   CxxStats::Incr(CxxStats::TEMPLATE_PARM);
}

//------------------------------------------------------------------------------

fn_name TemplateParm_Check = "TemplateParm.Check";

void TemplateParm::Check() const
{
   Debug::ft(TemplateParm_Check);

   if(default_ != nullptr) default_->Check();
}

//------------------------------------------------------------------------------

void TemplateParm::Print(ostream& stream, const Flags& options) const
{
   stream << tag_ << SPACE;
   stream << *Name();
   if(ptrs_ > 0) stream << string(ptrs_, '*');
   if(default_ != nullptr)
   {
      stream << " = ";
      default_->Print(stream, options);
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

fn_name TemplateParms_AddParm = "TemplateParms.AddParm";

void TemplateParms::AddParm(TemplateParmPtr& parm)
{
   Debug::ft(TemplateParms_AddParm);

   parms_.push_back(std::move(parm));
}

//------------------------------------------------------------------------------

fn_name TemplateParms_Check = "TemplateParms.Check";

void TemplateParms::Check() const
{
   Debug::ft(TemplateParms_Check);

   for(auto p = parms_.cbegin(); p != parms_.cend(); ++p)
   {
      (*p)->Check();
   }
}

//------------------------------------------------------------------------------

void TemplateParms::Print(ostream& stream, const Flags& options) const
{
   stream << TEMPLATE_STR << '<';

   for(auto p = parms_.cbegin(); p != parms_.cend(); ++p)
   {
      (*p)->Print(stream, options);
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

TypeName::TypeName(string& name) :
   args_(nullptr),
   scope_(nullptr),
   ref_(nullptr),
   class_(nullptr),
   type_(nullptr),
   forw_(nullptr),
   oper_(Cxx::NIL_OPERATOR),
   scoped_(false),
   using_(false)
{
   Debug::ft(TypeName_ctor1);

   std::swap(name_, name);
   CxxStats::Incr(CxxStats::TYPE_NAME);
}

//------------------------------------------------------------------------------

fn_name TypeName_ctor2 = "TypeName.ctor(copy)";

TypeName::TypeName(const TypeName& that) : CxxNamed(that),
   name_(that.name_),
   scope_(that.scope_),
   ref_(that.ref_),
   class_(that.class_),
   type_(that.type_),
   forw_(that.forw_),
   oper_(that.oper_),
   scoped_(that.scoped_),
   using_(that.using_)
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

void TypeName::Append(const string& name, bool space)
{
   Debug::ft(TypeName_Append);

   if(space) name_ += SPACE;
   name_ += name;
}

//------------------------------------------------------------------------------

fn_name TypeName_Check = "TypeName.Check";

void TypeName::Check() const
{
   Debug::ft(TypeName_Check);

   if(args_ != nullptr)
   {
      for(auto a = args_->cbegin(); a != args_->cend(); ++a)
      {
         (*a)->Check();
      }
   }
}

//------------------------------------------------------------------------------

CxxNamed* TypeName::DirectType() const
{
   return (type_ != nullptr ? type_ : ref_);
}

//------------------------------------------------------------------------------

fn_name TypeName_FindReferent = "TypeName.FindReferent";

void TypeName::FindReferent()
{
   Debug::ft(TypeName_FindReferent);

   if(args_ != nullptr)
   {
      for(auto a = args_->cbegin(); a != args_->cend(); ++a)
      {
         (*a)->FindReferent();
      }
   }
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

   //  Currently, this does not report usages based on ref_ or type_.
   //  If it did,  DataSpec.GetUsages would need a way to suppress or
   //  bypass it, because a name doesn't know whether its ref_ or type_
   //  was used directly or indirectly.
   //
   if(args_ != nullptr)
   {
      for(auto a = args_->cbegin(); a != args_->cend(); ++a)
      {
         (*a)->GetUsages(file, symbols);
      }
   }

   auto cls = class_;

   if(cls != nullptr)
   {
      if(cls->IsInTemplateInstance()) cls = cls->GetClassTemplate();
      if(cls->GetFile() != &file) symbols.AddDirect(cls);
   }

   if(forw_ != nullptr) symbols.AddForward(forw_);
   if(using_) symbols.AddUser(this);
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

fn_name TypeName_MemberAccessed = "TypeName.MemberAccessed";

void TypeName::MemberAccessed(Class* cls, CxxNamed* mem) const
{
   Debug::ft(TypeName_MemberAccessed);

   ref_ = mem;
   class_ = cls;
}

//------------------------------------------------------------------------------

void TypeName::Print(ostream& stream, const Flags& options) const
{
   if(scoped_) stream << SCOPE_STR;
   stream << *Name();

   if(args_ != nullptr)
   {
      stream << '<';

      for(auto a = args_->cbegin(); a != args_->cend(); ++a)
      {
         (*a)->Print(stream, options);
         if(*a != args_->back()) stream << ',';
      }

      stream << '>';
   }
}

//------------------------------------------------------------------------------

void TypeName::PushBack(TypeNamePtr& type)
{
   next_ = std::move(type);
}

//------------------------------------------------------------------------------

string TypeName::QualifiedName(bool scopes, bool templates) const
{
   string qname = (scoped_ && scopes ? SCOPE_STR : EMPTY_STR);
   qname += name_;

   if(!templates || (args_ == nullptr)) return qname;

   qname += '<';

   for(auto a = args_->cbegin(); a != args_->cend(); ++a)
   {
      qname += (*a)->QualifiedName(scopes, templates);
      if(*a != args_->back()) qname += ',';
   }

   return qname + '>';
}

//------------------------------------------------------------------------------

fn_name TypeName_ResolveTypedef = "TypeName.ResolveTypedef";

bool TypeName::ResolveTypedef(Typedef* type, size_t n) const
{
   Debug::ft(TypeName_ResolveTypedef);

   type_ = type;
   return true;
}

//------------------------------------------------------------------------------

fn_name TypeName_SetForward = "TypeName.SetForward";

void TypeName::SetForward(CxxScoped* decl) const
{
   Debug::ft(TypeName_SetForward);

   forw_ = decl;
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

   oper_ = oper;

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

fn_name TypeName_SetReferent = "TypeName.SetReferent";

void TypeName::SetReferent(CxxNamed* item, const SymbolView* view) const
{
   Debug::ft(TypeName_SetReferent);

   //  This can be invoked more than once when a class template name clears
   //  its referent, instead of leaving it as a forward declaration, so that
   //  the referent can later be set to a class template instance.  When this
   //  occurs, this function is also reinvoked on template arguments.  If an
   //  argument's name was already resolved, however, its using_ flag should
   //  not be set by a subsequent invocation.
   //
   if((view != nullptr) && view->using_ && (ref_ == nullptr)) using_ = true;
   ref_ = item;
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

fn_name TypeName_SubclassAccess = "TypeName.SubclassAccess";

void TypeName::SubclassAccess(Class* cls) const
{
   Debug::ft(TypeName_SubclassAccess);

   class_ = cls;
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
