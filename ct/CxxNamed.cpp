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

using namespace NodeBase;
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
   Context::SwLog(CodeTools_ReferentError, expl, offset);
   return nullptr;
}

//==============================================================================

CxxLocation::CxxLocation() :
   file_(nullptr),
   pos_(NOT_IN_SOURCE),
   internal_(false)
{
}

//------------------------------------------------------------------------------

size_t CxxLocation::GetPos() const
{
   return (pos_ != NOT_IN_SOURCE ? pos_ : std::string::npos);
}

//------------------------------------------------------------------------------

void CxxLocation::SetLoc(CodeFile* file, size_t pos)
{
   file_ = file;
   pos_ = pos;
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

fn_name CxxNamed_CopyContext = "CxxNamed.CopyContext";

void CxxNamed::CopyContext(const CxxNamed* that)
{
   Debug::ft(CxxNamed_CopyContext);

   auto scope = that->GetScope();
   SetScope(scope);
   SetAccess(that->GetAccess());
   loc_.SetLoc(that->GetFile(), that->GetPos());
   SetInternal();
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
   Context::SwLog(CxxNamed_FindReferent, expl, 0);
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

size_t CxxNamed::GetRange(size_t& begin, size_t& end) const
{
   begin = string::npos;
   end = string::npos;
   return string::npos;
}

//------------------------------------------------------------------------------

void CxxNamed::GetScopedNames(stringVector& names) const
{
   names.push_back(SCOPE_STR + ScopedName(false));
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

void CxxNamed::Log(Warning warning, size_t offset, bool hide) const
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

   GetFile()->LogPos(GetPos(), warning, this, offset, EMPTY_STR, hide);
}

//------------------------------------------------------------------------------

fn_name CxxNamed_MemberToArg = "CxxNamed.MemberToArg";

StackArg CxxNamed::MemberToArg(StackArg& via, Cxx::Operator op)
{
   Debug::ft(CxxNamed_MemberToArg);

   //  This should only be invoked on ClassData.
   //
   auto expl = "Unexpected member selection by " + *via.item->Name();
   Context::SwLog(CxxNamed_MemberToArg, expl, op);

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
         Context::SwLog(CxxNamed_ResolveName, expl, type);
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

fn_name CxxNamed_SetContext = "CxxNamed.SetContext";

void CxxNamed::SetContext(size_t pos)
{
   Debug::ft(CxxNamed_SetContext);

   //  If the item has already set its scope, don't overwrite it.
   //
   auto scope = GetScope();

   if(scope == nullptr)
   {
      scope = Context::Scope();
      SetScope(scope);
   }

   SetAccess(scope->GetCurrAccess());
   loc_.SetLoc(Context::File(), pos);
   if(Context::ParsingTemplateInstance()) SetInternal();
}

//------------------------------------------------------------------------------

fn_name CxxNamed_SetLoc = "CxxNamed.SetLoc";

void CxxNamed::SetLoc(CodeFile* file, size_t pos)
{
   Debug::ft(CxxNamed_SetLoc);

   loc_.SetLoc(file, pos);
}

//------------------------------------------------------------------------------

fn_name CxxNamed_SetReferent = "CxxNamed.SetReferent";

void CxxNamed::SetReferent(CxxNamed* item, const SymbolView* view) const
{
   Debug::ft(CxxNamed_SetReferent);

   auto expl = "SetReferent() not implemented by " + strClass(this, false);
   Context::SwLog(CxxNamed_FindReferent, expl, 0);
}

//------------------------------------------------------------------------------

fn_name CxxNamed_SetTemplateParms = "CxxNamed.SetTemplateParms";

void CxxNamed::SetTemplateParms(TemplateParmsPtr& parms)
{
   Debug::ft(CxxNamed_SetTemplateParms);

   auto expl = "Template parameters not supported by " + Trace();
   Context::SwLog(CxxNamed_SetTemplateParms, expl, 0);
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

const TypeSpecPtr DataSpec::Bool = TypeSpecPtr(new DataSpec(BOOL_STR));
const TypeSpecPtr DataSpec::Int = TypeSpecPtr(new DataSpec(INT_STR));

//------------------------------------------------------------------------------

fn_name DataSpec_ctor1 = "DataSpec.ctor";

DataSpec::DataSpec(QualNamePtr& name) :
   name_(name.release()),
   arrays_(nullptr)
{
   Debug::ft(DataSpec_ctor1);

   CxxStats::Incr(CxxStats::DATA_SPEC);
}

//------------------------------------------------------------------------------

fn_name DataSpec_ctor2 = "DataSpec.ctor(string)";

DataSpec::DataSpec(const char* name) : arrays_(nullptr)
{
   Debug::ft(DataSpec_ctor2);

   name_ = QualNamePtr(new QualName(name));
   CxxStats::Incr(CxxStats::DATA_SPEC);
}

//------------------------------------------------------------------------------

fn_name DataSpec_ctor3 = "DataSpec.ctor(copy)";

DataSpec::DataSpec(const DataSpec& that) : TypeSpec(that),
   arrays_(nullptr),
   tags_(that.tags_)
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
   tags_.AddArray();
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

   auto thisTags = this->GetAllTags();

   if(thisTags.PtrCount(true) == 0)
   {
      return thatArg->TypeString(true);
   }

   auto thatTags = thatArg->GetAllTags();
   if(!thisTags.AlignTemplateTag(thatTags)) return ERROR_STR;
   return thatArg->TypeTagsString(thatTags);
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
      count += spec->Tags()->ArrayCount();
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

   if(tags_.ptrDet_) Log(PtrTagDetached);
   if(tags_.refDet_) Log(RefTagDetached);
}

//------------------------------------------------------------------------------

fn_name DataSpec_Clone = "DataSpec.Clone";

TypeSpec* DataSpec::Clone() const
{
   Debug::ft(DataSpec_Clone);

   return new DataSpec(*this);
}

//------------------------------------------------------------------------------

fn_name DataSpec_CopyContext = "DataSpec.CopyContext";

void DataSpec::CopyContext(const CxxNamed* that)
{
   Debug::ft(DataSpec_CopyContext);

   CxxNamed::CopyContext(that);

   name_->CopyContext(that);
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
   tags_.Print(stream);
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

   Context::SetPos(GetLoc());

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
   Context::SwLog(DataSpec_FindReferent, expl, 0);
}

//------------------------------------------------------------------------------

TypeTags DataSpec::GetAllTags() const
{
   return TypeTags(*this);
}

//------------------------------------------------------------------------------

fn_name DataSpec_GetNumeric = "DataSpec.GetNumeric";

Numeric DataSpec::GetNumeric() const
{
   Debug::ft(DataSpec_GetNumeric);

   auto ptrs = Ptrs(true);
   if(ptrs > 0) return Numeric::Pointer;

   auto root = Root();
   if(root == nullptr) return Numeric::Nil;
   return root->GetNumeric();
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
      Debug::SwLog(DataSpec_GetUsages, log, 0, SwInfo);
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
         auto tmplt = ref->GetTemplate();
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
   Context::SwLog(DataSpec_Instantiating, expl, 0);
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

   if(IsAutoDecl()) return tags_.IsConst();

   if(tags_.IsConst()) return true;
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

   auto cp = tags_.IsConstPtr();

   if(cp == 1) return true;
   if(cp == -1) return false;
   if(IsAutoDecl()) return false;

   //  We have no pointers, so see if our referent has any.
   //
   auto ref = Referent();
   if(ref == nullptr) return false;
   auto spec = ref->GetTypeSpec();
   if(spec == nullptr) return false;
   return spec->IsConstPtr();
}

//------------------------------------------------------------------------------

bool DataSpec::IsConstPtr(size_t n) const
{
   Debug::ft(DataSpec_IsConstPtr);

   if(IsAutoDecl()) return tags_.IsConstPtr(n);

   if(tags_.IsConstPtr(n)) return true;
   auto ref = Referent();
   if(ref == nullptr) return false;
   auto spec = ref->GetTypeSpec();
   if(spec == nullptr) return false;
   return spec->IsConstPtr(n);
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

   auto user = GetUserType();
   if(user == Cxx::Function) return true;

   CxxNamed* ref = nullptr;

   if(GetTemplateRole() != TemplateNone)
   {
      ref = name_->GetReferent();
      if((ref != nullptr) && ref->IsInTemplateInstance()) return false;
      return (user != Cxx::Operation);
   }

   if(user != Cxx::Typedef) return false;

   if(ref == nullptr) ref = name_->GetReferent();
   if((ref != nullptr) && (ref->Type() == Cxx::Class))
   {
      if(ref->IsInTemplateInstance()) return false;
      if(ref->GetTemplate() != nullptr) return false;
   }

   return true;
}

//------------------------------------------------------------------------------

fn_name DataSpec_ItemIsTemplateArg = "DataSpec.ItemIsTemplateArg";

bool DataSpec::ItemIsTemplateArg(const CxxScoped* item) const
{
   Debug::ft(DataSpec_ItemIsTemplateArg);

   if(Referent() == item) return true;
   return name_->ItemIsTemplateArg(item);
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
   auto parm = QualifiedName(true, false);
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
      auto thisTags = this->GetAllTags();
      auto thatTags = that->GetAllTags();
      return thisTags.MatchTemplateTags(thatTags);
   }

   //  This is not a template argument, so match on types.
   //
   if(MatchesExactly(that)) return Compatible;
   return Incompatible;
}

//------------------------------------------------------------------------------

void DataSpec::Print(ostream& stream, const Flags& options) const
{
   if(tags_.IsConst()) stream << CONST_STR << SPACE;
   name_->Print(stream, options);
   tags_.Print(stream);

   if(IsAutoDecl())
   {
      stream << SPACE << COMMENT_BEGIN_STR << SPACE;
      stream << TypeString(true) << SPACE << COMMENT_END_STR;
   }
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
      count += spec->Tags()->PtrCount(arrays);
      auto ref = spec->Referent();
      if(ref == nullptr) break;
      spec = ref->GetTypeSpec();
   }

   //  COUNT can be negative if this is invoked on an auto type with ARRAYS
   //  set to false.  Given
   //     auto& entry = table[index];
   //  ENTRY has a referent of TABLE, and ptrs_ = -1.  If arrays are then
   //  excluded from ENTRY's pointer count, the result will be -1 because
   //  TABLE's pointer count of 1 will not be included.
   //
   if(count < 0)
   {
      if(arrays || !IsAutoDecl())
      {
         auto expl = "Negative pointer count for " + Trace();
         Context::SwLog(DataSpec_Ptrs, expl, count);
      }
   }

   return count;
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

   //  An auto type can have a negative reference count that is eliminated
   //  once its type is determined.  Stop as soon as the count is positive;
   //  else an l-value reference (&) could become an r-value reference (&&).
   //
   TagCount count = 0;
   auto spec = static_cast< const TypeSpec* >(this);

   while(spec != nullptr)
   {
      count += spec->Tags()->RefCount();
      if(count > 0) return count;
      auto ref = spec->Referent();
      if(ref == nullptr) break;
      spec = ref->GetTypeSpec();
   }

   if(count >= 0) return count;

   auto expl = "Negative reference count for " + Trace();
   Context::SwLog(DataSpec_Refs, expl, count);
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
      Context::SwLog(DataSpec_RemoveRefs, expl, 0);
      return;
   }

   if(Refs() == 0) return;

   if(tags_.RefCount() != 0)
   {
      auto expl = "Removing references from auto& type " + this->Trace();
      Context::SwLog(DataSpec_RemoveRefs, expl, 1);
      return;
   }

   //  Negate any reference(s) on the referent's type.
   //
   tags_.SetRefs(-Refs());
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

bool DataSpec::ResolveTemplateArgument() const
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

      StackArg arg(result, tags_.PtrCount(true));
      if(tags_.IsConst()) arg.SetAsConst();
      if(tags_.IsConstPtr() == 1) arg.SetAsConstPtr();
      return arg;
   }

   switch(GetTemplateRole())
   {
   case TemplateParameter:
   case TemplateClass:
      break;
   default:
      auto expl = "Failed to find referent for " + QualifiedName(true, true);
      Context::SwLog(DataSpec_ResultType, expl, 0);
   }

   return NilStackArg;
}

//------------------------------------------------------------------------------

fn_name DataSpec_SetPtrs = "DataSpec.SetPtrs";

void DataSpec::SetPtrs(TagCount count)
{
   Debug::ft(DataSpec_SetPtrs);

   //  This should only be invoked on an auto type.  After resetting the
   //  count, invoke Ptrs to cause a log if the overall count is invalid.
   //
   if(!IsAutoDecl())
   {
      auto expl = "Resetting pointers on non-auto type " + this->Trace();
      Context::SwLog(DataSpec_SetPtrs, expl, 0);
      return;
   }

   tags_.SetPtrs(count);
   Ptrs(true);
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
         if(tags_.IsConst() && (item->GetTypeSpec()->Ptrs(false) > 0))
         {
            tags_.SetConst(false);
            tags_.SetConstPtr();
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

fn_name DataSpec_SetUserType = "DataSpec.SetUserType";

void DataSpec::SetUserType(Cxx::ItemType user)
{
   Debug::ft(DataSpec_SetUserType);

   TypeSpec::SetUserType(user);
   name_->SetUserType(user);
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
   auto tags = GetAllTags();

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
   tags.TypeString(ts, arg);
   return ts;
}

//------------------------------------------------------------------------------

string DataSpec::TypeTagsString(const TypeTags& tags) const
{
   auto ts = name_->TypeString(true);
   tags.TypeString(ts, false);
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
      TypeNamePtr thisName(new TypeName(*n));
      PushBack(thisName);
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

fn_name QualName_CopyContext = "QualName.CopyContext";

void QualName::CopyContext(const CxxNamed* that)
{
   Debug::ft(QualName_CopyContext);

   CxxNamed::CopyContext(that);

   for(auto n = First(); n != nullptr; n = n->Next())
   {
      n->CopyContext(that);
   }
}

//------------------------------------------------------------------------------

fn_name QualName_EnterBlock = "QualName.EnterBlock";

void QualName::EnterBlock()
{
   Debug::ft(QualName_EnterBlock);

   Context::SetPos(GetLoc());
   if(*Name() == NULL_STR) Log(UseOfNull);

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

fn_name QualName_GetForward = "QualName.GetForward";

CxxScoped* QualName::GetForward() const
{
   Debug::ft(QualName_GetForward);

   CxxScoped* forw = nullptr;

   for(auto n = First(); n != nullptr; n = n->Next())
   {
      auto f = n->GetForward();
      if(f != nullptr) forw = f;
   }

   return forw;
}

//------------------------------------------------------------------------------

TypeName* QualName::GetTemplateArgs() const
{
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

fn_name QualName_ItemIsTemplateArg = "QualName.ItemIsTemplateArg";

bool QualName::ItemIsTemplateArg(const CxxScoped* item) const
{
   Debug::ft(QualName_ItemIsTemplateArg);

   //  Look for template arguments attached to each name.
   //
   for(auto n = First(); n != nullptr; n = n->Next())
   {
      if(n->ItemIsTemplateArg(item)) return true;
   }

   return false;
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
   Context::SwLog(QualName_MemberAccessed, expl, 0);
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

fn_name QualName_ResolveLocal = "QualName.ResolveLocal";

CxxNamed* QualName::ResolveLocal(SymbolView* view) const
{
   Debug::ft(QualName_ResolveLocal);

   auto syms = Singleton< CxxSymbols >::Instance();

   if((Size() == 1) && !IsGlobal())
   {
      auto item = syms->FindLocal(*Name(), view);

      if(item != nullptr)
      {
         SetReferentN(0, item, view);
         return item;
      }
   }

   return ResolveName(Context::File(), Context::Scope(), CODE_REFS, view);
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

fn_name QualName_SetUserType = "QualName.SetUserType";

void QualName::SetUserType(Cxx::ItemType user) const
{
   Debug::ft(QualName_SetUserType);

   for(auto n = First(); n != nullptr; n = n->Next())
   {
      n->SetUserType(user);
   }
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
   Context::SwLog(QualName_TypeString, expl, 0);
   return ERROR_STR;
}

//==============================================================================

fn_name TemplateParm_ctor1 = "TemplateParm.ctor";

TemplateParm::TemplateParm
   (string& name, Cxx::ClassTag tag, size_t ptrs, QualNamePtr& preset) :
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
         TypeSpecPtr arg((*a)->Clone());
         arg->CopyContext(a->get());
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

TypeName* TypeName::GetTemplateArgs() const
{
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

fn_name TypeName_ItemIsTemplateArg = "TypeName.ItemIsTemplateArg";

bool TypeName::ItemIsTemplateArg(const CxxScoped* item) const
{
   Debug::ft(TypeName_ItemIsTemplateArg);

   if(args_ != nullptr)
   {
      for(auto a = args_->cbegin(); a != args_->cend(); ++a)
      {
         if((*a)->ItemIsTemplateArg(item)) return true;
      }
   }

   auto ref = DirectType();

   if(ref != nullptr)
   {
      auto type = ref->GetTypeSpec();

      if(type != nullptr)
      {
         if(type->ItemIsTemplateArg(item)) return true;
      }
   }

   return false;
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

fn_name TypeName_SetUserType = "TypeName.SetUserType";

void TypeName::SetUserType(Cxx::ItemType user) const
{
   Debug::ft(TypeName_SetUserType);

   if(args_ != nullptr)
   {
      for(auto a = args_->cbegin(); a != args_->cend(); ++a)
      {
         (*a)->SetUserType(user);
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
   user_(Cxx::Operation),
   role_(TemplateNone)
{
   Debug::ft(TypeSpec_ctor1);
}

//------------------------------------------------------------------------------

fn_name TypeSpec_ctor2 = "TypeSpec.ctor(copy)";

TypeSpec::TypeSpec(const TypeSpec& that) : CxxNamed(that),
   user_(that.user_),
   role_(that.role_)
{
   Debug::ft(TypeSpec_ctor2);
}

//------------------------------------------------------------------------------

fn_name TypeSpec_PureVirtualFunction = "TypeSpec.PureVirtualFunction";

//------------------------------------------------------------------------------

void TypeSpec::AddArray(ArraySpecPtr& array)
{
   Debug::SwLog(TypeSpec_PureVirtualFunction, "AddArray", 0);
}

//------------------------------------------------------------------------------

string TypeSpec::AlignTemplateArg(const TypeSpec* thatArg) const
{
   Debug::SwLog(TypeSpec_PureVirtualFunction, "AlignTemplateArg", 0);
   return ERROR_STR;
}

//------------------------------------------------------------------------------

TagCount TypeSpec::Arrays() const
{
   Debug::SwLog(TypeSpec_PureVirtualFunction, "Arrays", 0);
   return 0;
}

//------------------------------------------------------------------------------

TypeSpec* TypeSpec::Clone() const
{
   Debug::SwLog(TypeSpec_PureVirtualFunction, "Clone", 0);
   return nullptr;
}

//------------------------------------------------------------------------------

void TypeSpec::DisplayArrays(ostream& stream) const
{
   Debug::SwLog(TypeSpec_PureVirtualFunction, "DisplayArrays", 0);
}

//------------------------------------------------------------------------------

void TypeSpec::DisplayTags(ostream& stream) const
{
   Debug::SwLog(TypeSpec_PureVirtualFunction, "DisplayTags", 0);
}

//------------------------------------------------------------------------------

void TypeSpec::EnterArrays() const
{
   Debug::SwLog(TypeSpec_PureVirtualFunction, "EnterArrays", 0);
}

//------------------------------------------------------------------------------

void TypeSpec::EnteringScope(const CxxScope* scope)
{
   Debug::SwLog(TypeSpec_PureVirtualFunction, "EnteringScope", 0);
}

//------------------------------------------------------------------------------

TypeTags TypeSpec::GetAllTags() const
{
   TypeTags tags;

   Debug::SwLog(TypeSpec_PureVirtualFunction, "GetAllTags", 0);
   return tags;
}

//------------------------------------------------------------------------------

bool TypeSpec::HasArrayDefn() const
{
   Debug::SwLog(TypeSpec_PureVirtualFunction, "HasArrayDefn", 0);
   return false;
}

//------------------------------------------------------------------------------

void TypeSpec::Instantiating() const
{
   Debug::SwLog(TypeSpec_PureVirtualFunction, "Instantiating", 0);
}

//------------------------------------------------------------------------------

bool TypeSpec::ItemIsTemplateArg(const CxxScoped* item) const
{
   Debug::SwLog(TypeSpec_PureVirtualFunction, "ItemIsTemplateArg", 0);
   return false;
}

//------------------------------------------------------------------------------

bool TypeSpec::MatchesExactly(const TypeSpec* that) const
{
   Debug::SwLog(TypeSpec_PureVirtualFunction, "MatchesExactly", 0);
   return false;
}

//------------------------------------------------------------------------------

TypeMatch TypeSpec::MatchTemplate(TypeSpec* that, stringVector& tmpltParms,
   stringVector& tmpltArgs, bool& argFound) const
{
   Debug::SwLog(TypeSpec_PureVirtualFunction, "MatchTemplate", 0);
   return Incompatible;
}

//------------------------------------------------------------------------------

TypeMatch TypeSpec::MatchTemplateArg(const TypeSpec* that) const
{
   Debug::SwLog(TypeSpec_PureVirtualFunction, "MatchTemplateArg", 0);
   return Incompatible;
}

//------------------------------------------------------------------------------

fn_name TypeSpec_MustMatchWith = "TypeSpec.MustMatchWith";

TypeMatch TypeSpec::MustMatchWith(const StackArg& that) const
{
   Debug::ft(TypeSpec_MustMatchWith);

   if(GetTemplateRole() == TemplateParameter) return Compatible;

   auto thisType = this->TypeString(true);
   auto thatType = that.TypeString(true);
   auto match = ResultType().CalcMatchWith(that, thisType, thatType);

   if(match == Incompatible)
   {
      auto expl = thisType + " is incompatible with " + thatType;
      Context::SwLog(TypeSpec_MustMatchWith, expl, 0);
   }

   return match;
}

//------------------------------------------------------------------------------

TagCount TypeSpec::Ptrs(bool arrays) const
{
   Debug::SwLog(TypeSpec_PureVirtualFunction, "Ptrs", 0);
   return 0;
}

//------------------------------------------------------------------------------

TagCount TypeSpec::Refs() const
{
   Debug::SwLog(TypeSpec_PureVirtualFunction, "Refs", 0);
   return 0;
}

//------------------------------------------------------------------------------

void TypeSpec::RemoveRefs()
{
   Debug::SwLog(TypeSpec_PureVirtualFunction, "RemoveRefs", 0);
}

//------------------------------------------------------------------------------

StackArg TypeSpec::ResultType() const
{
   Debug::SwLog(TypeSpec_PureVirtualFunction, "ResultType", 0);
   return NilStackArg;
}

//------------------------------------------------------------------------------

void TypeSpec::SetPtrs(TagCount count)
{
   Debug::SwLog(TypeSpec_PureVirtualFunction, "SetPtrs", 0);
}

//------------------------------------------------------------------------------

fn_name TypeSpec_SetUserType = "TypeSpec.SetUserType";

void TypeSpec::SetUserType(Cxx::ItemType user)
{
   Debug::ft(TypeSpec_SetUserType);

   user_ = user;
}

//------------------------------------------------------------------------------

const TypeTags* TypeSpec::Tags() const
{
   Debug::SwLog(TypeSpec_PureVirtualFunction, "Tags", 0);
   return nullptr;
}

//------------------------------------------------------------------------------

TypeTags* TypeSpec::Tags()
{
   Debug::SwLog(TypeSpec_PureVirtualFunction, "Tags", 1);
   return nullptr;
}

//------------------------------------------------------------------------------

string TypeSpec::TypeTagsString(const TypeTags& tags) const
{
   Debug::SwLog(TypeSpec_PureVirtualFunction, "TypeTagsString", 0);
   return ERROR_STR;
}

//==============================================================================

fn_name TypeTags_ctor1 = "TypeTags.ctor";

TypeTags::TypeTags() :
   ptrDet_(false),
   refDet_(false),
   const_(false),
   array_(false),
   arrays_(0),
   ptrs_(0),
   refs_(0)
{
   Debug::ft(TypeTags_ctor1);

   for(auto i = 0; i < Cxx::MAX_PTRS; ++i) constptr_[i] = false;
}

//------------------------------------------------------------------------------

fn_name TypeTags_ctor2 = "TypeTags.ctor(TypeSpec)";

TypeTags::TypeTags(const TypeSpec& spec) :
   ptrDet_(false),
   refDet_(false),
   const_(spec.IsConst()),
   array_(spec.Tags()->IsUnboundedArray()),
   arrays_(spec.Arrays()),
   ptrs_(spec.Ptrs(false)),
   refs_(spec.Refs())
{
   Debug::ft(TypeTags_ctor2);

   for(auto i = 0; i < Cxx::MAX_PTRS; ++i)
   {
      if(i < ptrs_)
         constptr_[i] = spec.IsConstPtr(i);
      else
         constptr_[i] = false;
   }
}

//------------------------------------------------------------------------------

fn_name TypeTags_AlignTemplateTag = "TypeTags.AlignTemplateTag";

bool TypeTags::AlignTemplateTag(TypeTags& that) const
{
   Debug::ft(TypeTags_AlignTemplateTag);

   if(that.ptrs_ < this->ptrs_) return false;
   if(that.arrays_ < this->arrays_) return false;
   that.ptrs_ -= this->ptrs_;
   that.arrays_ -= this->arrays_;
   return true;
}

//------------------------------------------------------------------------------

fn_name TypeTags_ArrayCount = "TypeTags.ArrayCount";

TagCount TypeTags::ArrayCount() const
{
   Debug::ft(TypeTags_ArrayCount);

   auto count = arrays_;
   if(array_) ++count;
   return count;
}

//------------------------------------------------------------------------------

int TypeTags::IsConstPtr() const
{
   if(ptrs_ <= 0) return 0;
   return (constptr_[ptrs_ - 1] ? 1 : -1);
}

//------------------------------------------------------------------------------

bool TypeTags::IsConstPtr(size_t n) const
{
   return ((n >= 0) && (n < ptrs_) ? constptr_[n] : false);
}

//------------------------------------------------------------------------------

fn_name TypeTags_MatchTemplateTags = "TypeTags.MatchTemplateTags";

TypeMatch TypeTags::MatchTemplateTags(const TypeTags& that) const
{
   Debug::ft(TypeTags_MatchTemplateTags);

   if(this->ptrs_ > that.ptrs_) return Incompatible;
   if(this->arrays_ > that.arrays_) return Incompatible;
   if(this->ptrs_ < that.ptrs_) return Convertible;
   if(this->arrays_ < that.arrays_) return Convertible;
   return Compatible;
}

//------------------------------------------------------------------------------

void TypeTags::Print(ostream& stream) const
{
   //  This is used to display code, so arrays_ is ignored because
   //  array specifications will follow the name of the data item.
   //
   if(array_) stream << ARRAY_STR;

   for(auto i = 0; i < ptrs_; ++i)
   {
      stream << '*';
      if(constptr_[i]) stream << " const";
   }

   if(refs_ > 0) stream << string(refs_, '&');
}

//------------------------------------------------------------------------------

fn_name TypeTags_PtrCount = "TypeTags.PtrCount";

TagCount TypeTags::PtrCount(bool arrays) const
{
   Debug::ft(TypeTags_PtrCount);

   if(!arrays) return ptrs_;
   auto count = ptrs_ + arrays_;
   if(array_) ++count;
   return count;
}

//------------------------------------------------------------------------------

fn_name TypeTags_SetConstPtr = "TypeTags.SetConstPtr";

void TypeTags::SetConstPtr() const
{
   Debug::ft(TypeTags_SetConstPtr);

   if(ptrs_ > 0)
      constptr_[ptrs_ - 1] = true;
   else
      Context::SwLog(TypeTags_SetConstPtr, "No pointer tags", 0);
}

//------------------------------------------------------------------------------

fn_name TypeTags_SetPointer = "TypeTags.SetPointer";

bool TypeTags::SetPointer(size_t n, bool readonly)
{
   Debug::ft(TypeTags_SetPointer);

   if((n >= 0) && (n < Cxx::MAX_PTRS))
   {
      if(n >= ptrs_) ptrs_ = n + 1;
      constptr_[n] = readonly;
      return true;
   }
   return false;
}

//------------------------------------------------------------------------------

fn_name TypeTags_SetPtrs = "TypeTags.SetPtrs";

void TypeTags::SetPtrs(TagCount count)
{
   Debug::ft(TypeTags_SetPtrs);

   ptrs_ = count;
}

//------------------------------------------------------------------------------

void TypeTags::TypeString(std::string& name, bool arg) const
{
   if(const_) name = "const " + name;

   for(auto i = 0; i < ptrs_; ++i)
   {
      name.push_back('*');
      if(constptr_[i]) name += " const";
   }

   if(arg)
   {
      //  For an auto type, ptrs_ can be negative:
      //     auto& entry = table[index];
      //  ENTRY initially has ptrs_ = 0.  StackArg.WasIndexed, invoked on TABLE,
      //  decrements its ptrs_ from 0 to -1.  The result is ENTRY's referent, so
      //  ENTRY has ptrs_ = -1 and arrays_ = 1 (from TABLE's DataSpec).  These
      //  must cancel each other out so that ENTRY doesn't masquerade as either
      //  an array or a pointer.
      //
      auto count = (ptrs_ < 0 ? ptrs_ + arrays_ : arrays_);
      if(count > 0) name += string(count, '*');
   }
   else
   {
      for(auto i = 0 ; i < arrays_; ++i) name += ARRAY_STR;
   }

   if(!arg && (refs_ > 0)) name += string(refs_, '&');
}
}
