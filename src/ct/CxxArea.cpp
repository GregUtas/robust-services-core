//==============================================================================
//
//  CxxArea.cpp
//
//  Copyright (C) 2013-2022  Greg Utas
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
#include "CxxArea.h"
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <set>
#include <sstream>
#include <utility>
#include "CodeFile.h"
#include "CxxExecute.h"
#include "CxxRoot.h"
#include "CxxString.h"
#include "CxxSymbols.h"
#include "CxxToken.h"
#include "CxxVector.h"
#include "Debug.h"
#include "Editor.h"
#include "Formatters.h"
#include "Parser.h"
#include "Singleton.h"

using namespace NodeBase;
using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace CodeTools
{
fn_name CodeTools_CreateCodeError = "CodeTools.CreateCodeError";

//  Invoked when an error occurs in CreateCode.  NAME is the template
//  whose code could not be found.
//
static size_t CreateCodeError(const string& name, debug64_t offset)
{
   Debug::ft(CodeTools_CreateCodeError);

   auto expl = "Could not find code for " + name;
   Context::SwLog(CodeTools_CreateCodeError, expl, offset);
   return string::npos;
}

//------------------------------------------------------------------------------
//
//  Returns the minimum value in MATCHES.
//
static TypeMatch FindMin(const std::vector<TypeMatch>& matches)
{
   if(matches.empty()) return Incompatible;
   auto min = Compatible;

   for(auto m = matches.cbegin(); m != matches.cend(); ++m)
   {
      if(*m < min) min = *m;
   }

   return min;
}

//------------------------------------------------------------------------------
//
//  Invoked by FindFunc.  Updates VIEW with MATCH and returns FUNC.
//
static Function* FoundFunc(Function* func, SymbolView* view, TypeMatch match)
{
   Debug::ft("CodeTools.FoundFunc(CxxArea)");

   if(view != nullptr) view->match_ = match;
   return func;
}

//------------------------------------------------------------------------------
//
//  Invoked when FindFunc (which defines the other arguments) has resolved
//  a function call to FUNC.
//
static Function* FuncAccessed(Function* func,
   const StackArg* call, const CxxScope* scope, const SymbolView* view)
{
   Debug::ft("CodeTools.FuncAccessed");

   if((func == nullptr) || (view == nullptr)) return func;

   if((call != nullptr) && (call->Via() != nullptr))
   {
      auto cls = static_cast<Class*>(call->Via()->Root());

      call->Name()->MemberAccessed(cls, func);

      if((view->accessibility_ == Inherited) && !view->friend_ &&
         (cls->ClassDistance(scope->GetClass()) == NOT_A_SUBCLASS))
      {
         func->RecordAccess(Cxx::Public);
         return func;
      }
   }

   func->RecordAccess(view->control_);
   return func;
}

//------------------------------------------------------------------------------

fn_name CodeTools_MemberIsAccessibleTo = "CodeTools.MemberIsAccessibleTo";

//  Determines if MEMBER is accessible to SCOPE, updating VIEW with details
//  on its visibility.
//
static bool MemberIsAccessibleTo
   (const CxxScoped* member, const CxxScope* scope, SymbolView* view)
{
   Debug::ft(CodeTools_MemberIsAccessibleTo);

   SymbolView local;
   if(member == nullptr) return false;
   if(scope == nullptr) return true;
   if(view == nullptr) view = &local;

   member->AccessibilityTo(scope, *view);
   if(view->accessibility_ != Inaccessible)
   {
      if(member->Type() != Cxx::Function) member->RecordAccess(view->control_);
      return true;
   }

   //  We should never get here when compiling well-formed code, so there is
   //  probably a bug in AccessibilityOf.  Log this, but assume that ITEM is
   //  accessible.
   //
   auto expl = member->ScopedName(true) + " is inaccessible";
   Context::SwLog(CodeTools_MemberIsAccessibleTo, expl, 0);
   return true;
}

//==============================================================================

Class::Class(QualNamePtr& name, Cxx::ClassTag tag) :
   name_(name.release()),
   tag_(tag),
   currAccess_(tag == Cxx::ClassType ? Cxx::Private : Cxx::Public),
   created_(false),
   implicit_(false),
   copied_(false)
{
   Debug::ft("Class.ctor[>ct]");

   Singleton<CxxSymbols>::Instance()->InsertClass(this);
}

//------------------------------------------------------------------------------

Class::~Class()
{
   Debug::ftnt("Class.dtor[>ct]");

   GetFile()->EraseClass(this);
   Singleton<CxxSymbols>::Extant()->EraseClass(this);
}

//------------------------------------------------------------------------------

fn_name Class_AccessibilityOf = "Class.AccessibilityOf";

void Class::AccessibilityOf
   (const CxxScope* scope, const CxxScoped* item, SymbolView& view) const
{
   Debug::ft(Class_AccessibilityOf);

   //  Start by assuming the worst.
   //
   view.accessibility_ = Inaccessible;

   //  We shouldn't be here if ITEM doesn't belong to a class.
   //
   auto itemClass = item->GetClass();
   if(itemClass == nullptr)
   {
      auto expl = "Item is not a class member: " + item->ScopedName(true);
      Context::SwLog(Class_AccessibilityOf, expl, 0);
      return;
   }

   //  If ITEM is a forward declaration, increase its distance from SCOPE
   //  so that CxxSymbols.FindNearestItem will resolve the class's name to
   //  its definition instead of to the forward declaration.
   //
   auto itemType = item->Type();
   auto f = (itemType == Cxx::Forward ? 1 : 0);

   //  The purpose of this function isn't only to check accessibility, but
   //  also to determine whether an item's access control could be changed
   //  to something more restrictive.  This affects the order of the logic.
   //
   std::vector<Class*> userClasses;
   auto userClass = scope->GetClass();

   if(userClass != nullptr)
   {
      //  SCOPE can see ITEM if userClass is the same as itemClass.
      //
      if(userClass == itemClass)
      {
         view.distance_ = f;
         view.accessibility_ = Declared;
         view.control_ = Context::ScopeVisibility();
         return;
      }

      //  SCOPE can see ITEM if userClass is an inner class of itemClass.
      //
      userClasses.push_back(userClass);
      auto control = Context::ScopeVisibility();

      for(auto c = userClass->OuterClass(); c != nullptr; c = c->OuterClass())
      {
         control = std::min(control, c->GetAccess());

         if(c == itemClass)
         {
            view.distance_ = userClasses.size() + f;
            view.accessibility_ = Declared;
            view.control_ = control;
            return;
         }

         userClasses.push_back(c);
      }

      //  SCOPE can see ITEM if it is a friend of itemClass.
      //
      view.distance_ = userClass->ClassDistance(this);
      auto access =
         (view.distance_ == NOT_A_SUBCLASS ? Unrestricted : Inherited);

      auto frnd = itemClass->FindFriend(scope);

      if(frnd != nullptr)
      {
         view.accessibility_ = access;
         view.friend_ = true;
         if(control == Cxx::Private) frnd->IncrUsers();
         return;
      }

      //  SCOPE can see ITEM if it inherits from this class (the one that
      //  defined ITEM), as long as ITEM is not private.
      //
      if((access == Inherited) && (item->GetAccess() != Cxx::Private))
      {
         view.accessibility_ = Inherited;
         view.control_ = Cxx::Protected;
         return;
      }

      //  If ITEM is an inline function in a class template, SCOPE can see
      //  ITEM if userClass is an instance of the class template.
      //
      if((itemType == Cxx::Function) && (userClass->GetTemplate() == this) &&
         static_cast<const Function*>(item)->IsInline())
      {
         view.distance_ = 1;
         view.accessibility_ = Declared;
         return;
      }

      //  Don't enforce access controls on a class template.  Violations
      //  will be detected on template instances.
      //
      if(userClass->IsTemplate())
      {
         view.accessibility_ = Unrestricted;
         return;
      }

      //  If the using class is a template instance, it can use a template
      //  argument, even if the argument is private.
      //
      if(userClass->IsInTemplateInstance())
      {
         auto spec = userClass->GetTemplatedName();

         if(spec->ItemIsTemplateArg(item))
         {
            view.accessibility_ = Unrestricted;
            return;
         }
      }
   }

   //  Find the distance from SCOPE to ITEM.  Start by seeing whether
   //  they share a common base class.  userClasses already contains
   //  the classes that wrap SCOPE, so do the same for ITEM.  At the
   //  same time, find the access control that applies to ITEM on the
   //  path from its class to its outermost class.
   //
   std::vector<Class*> itemClasses;
   std::vector<Cxx::Access> controls;
   auto control = item->GetAccess();

   for(auto c = itemClass; c != nullptr; c = c->OuterClass())
   {
      itemClasses.push_back(c);
      controls.push_back(std::min(control, c->GetAccess()));
   }

   size_t m = 0;
   auto n = SIZE_MAX;

   for(auto c = userClasses.cbegin(); c != userClasses.cend(); ++c, ++m)
   {
      n = IndexOf(itemClasses, *c);
      if(n != SIZE_MAX) break;
   }

   //  If N isn't SIZE_MAX, ITEM and SCOPE share a base class.  Otherwise
   //  SCOPE must access ITEM through its namespace.
   //
   if(n != SIZE_MAX)
      view.distance_ = m + n + f;
   else
      view.distance_ = scope->ScopeDistance(itemClasses.back()->GetSpace()) + f;

   //  Don't enforce access controls on a function template.  Violations
   //  will be detected on template instances.
   //
   auto userFunc = scope->GetFunction();

   if((userFunc != nullptr) && userFunc->IsTemplate())
   {
      view.accessibility_ = Unrestricted;
      return;
   }

   //  Determine which control applies.  If n=1, userClass is an inner class
   //  of the same class that defines itemClass, so SCOPE can see ITEM if the
   //  latter is a class (rather than one of its members, to which controls
   //  still apply).
   //
   if(n == SIZE_MAX)
   {
      control = controls.back();
   }
   else if((n <= 1) && (itemType == Cxx::Class))
   {
      view.accessibility_ = item->FileScopeAccessiblity();
      return;
   }
   else
   {
      control = controls[n];
   }

   //  See if SCOPE is a friend of ITEM's class.  If it isn't, it might still
   //  be able to access ITEM if ITEM is an inner class and SCOPE is a friend
   //  of the outer class.
   //
   auto frnd = itemClass->FindFriend(scope);

   if((frnd == nullptr) && (itemType == Cxx::Class))
   {
      auto decl = item->Declarer();
      if(decl != nullptr) frnd = decl->FindFriend(scope);
   }

   if(frnd != nullptr)
   {
      view.accessibility_ = Unrestricted;
      view.friend_ = true;
      if(control != Cxx::Public) frnd->IncrUsers();
      return;
   }

   //  If we get here, ITEM must be public for SCOPE to see it.
   //
   if(control == Cxx::Public)
   {
      view.accessibility_ = item->FileScopeAccessiblity();
      view.control_ = Cxx::Public;
   }
}

//------------------------------------------------------------------------------

void Class::AccessibilityTo(const CxxScope* scope, SymbolView& view) const
{
   Debug::ft("Class.AccessibilityTo");

   AccessibilityOf(scope, this, view);
}

//------------------------------------------------------------------------------

bool Class::AddAnonymousUnion(const ClassPtr& cls)
{
   Debug::ft("Class.AddAnonymousUnion");

   //  There is nothing to do unless CLS is an anonymous union.
   //
   if(cls->GetClassTag() != Cxx::UnionType) return false;
   if(!cls->Name().empty()) return false;
   auto access = cls->GetAccess();

   //  Remove the union's members and add them to this class.
   //
   auto srcData = cls->Datas();
   auto dstData = this->Datas();
   auto size = srcData->size();

   for(size_t i = 0; i < size; ++i)
   {
      auto mem = std::move(srcData->at(i));
      mem->Promote(this, access, (i == 0), (i == size - 1));
      dstData->push_back(std::move(mem));
   }

   return true;
}

//------------------------------------------------------------------------------

void Class::AddBase(BaseDeclPtr& base)
{
   Debug::ft("Class.AddBase");

   //  This is always invoked, so verify that a base class actually exists.
   //  If it is found from this scope, make it our base class.
   //
   if(base == nullptr) return;
   if(!base->EnterScope()) return;
   base_ = std::move(base);
}

//------------------------------------------------------------------------------

void Class::AddFiles(LibItemSet& imSet) const
{
   Debug::ft("Class.AddFiles");

   auto classes = Classes();

   for(auto c = classes->cbegin(); c != classes->cend(); ++c)
   {
      if(!(*c)->IsInternal()) (*c)->AddFiles(imSet);
   }

   auto funcs = Funcs();

   for(auto f = funcs->cbegin(); f != funcs->cend(); ++f)
   {
      if(!(*f)->IsInternal()) (*f)->AddFiles(imSet);
   }

   auto data = Datas();

   for(auto d = data->cbegin(); d != data->cend(); ++d)
   {
      if(!(*d)->IsInternal()) (*d)->AddFiles(imSet);
   }
}

//------------------------------------------------------------------------------

bool Class::AddFriend(FriendPtr& decl)
{
   Debug::ft("Class.AddFriend");

   if(decl->EnterScope())
   {
      friends_.push_back(std::move(decl));
   }

   return true;
}

//------------------------------------------------------------------------------

bool Class::AddSubclass(Class* cls)
{
   Debug::ft("Class.AddSubclass");

   subs_.push_back(cls);
   return true;
}

//------------------------------------------------------------------------------

void Class::BlockCopied(const StackArg* arg)
{
   Debug::ft("Class.BlockCopied");

   if(copied_) return;
   copied_ = true;

   if(GetFile()->IsSubsFile()) return;

   auto data = Datas();

   for(auto d = data->cbegin(); d != data->cend(); ++d)
   {
      auto mem = d->get();
      if(!mem->IsStatic()) mem->WasWritten(arg, true, false);
   }
}

//------------------------------------------------------------------------------

bool Class::CanConstructFrom(const StackArg& that, const string& thatType) const
{
   Debug::ft("Class.CanConstructFrom");

   //  Visit our functions to see if one of them is a suitable constructor.
   //
   auto funcs = Funcs();
   TypeMatch result = Incompatible;
   Function* ctor = nullptr;

   for(auto f = funcs->cbegin(); f != funcs->cend(); ++f)
   {
      auto match = (*f)->CalcConstructibilty(that, thatType);

      if(match > result)
      {
         result = match;
         ctor = f->get();
      }
   }

   //  For implicit construction, thatType must at least be convertible
   //  to the constructor's argument.
   //
   if(result >= Convertible)
   {
      ctor->SetImplicit();
      return true;
   }

   return false;
}

//------------------------------------------------------------------------------

void Class::Check() const
{
   Debug::ft("Class.Check");

   CxxArea::Check();

   if(parms_ != nullptr) parms_->Check();
   if(alignas_ != nullptr) alignas_->Check();

   auto base = GetBaseDecl();
   if(base != nullptr) base->Check();

   for(auto f = friends_.cbegin(); f != friends_.cend(); ++f)
   {
      (*f)->Check();
   }

   CheckIfUnused(ClassUnused);
   CheckConstructors();
   CheckDestructor();
   CheckRuleOfThree();
   CheckOverrides();
}

//------------------------------------------------------------------------------

void Class::CheckConstructors() const
{
   Debug::ft("Class.CheckConstructors");

   //  A singleton's constructor should be private.
   //  A base class constructor should not be public.
   //
   auto base = IsBaseClass();
   auto solo = IsSingleton();

   FunctionVector ctors;
   FindCtors(ctors);

   if(ctors.empty())
   {
      if(solo)
         Log(ConstructorNotPrivate);
      else if(base)
         Log(PublicConstructor);
      return;
   }

   for(auto c = ctors.begin(); c != ctors.end(); ++c)
   {
      if((*c)->IsDeleted()) continue;

      auto acc = (*c)->GetAccess();

      if(solo && (acc != Cxx::Private))
         (*c)->Log(ConstructorNotPrivate);
      else if(base && (acc == Cxx::Public))
         (*c)->Log(PublicConstructor);
   }
}

//------------------------------------------------------------------------------

void Class::CheckDestructor() const
{
   Debug::ft("Class.CheckDestructor");

   //  o A singleton's destructor should be private.
   //  o A base class destructor should be virtual.
   //  o A base class destructor should be public unless it is a base class
   //    for singletons or has a base class with a non-public destructor, in
   //    which case a destructor is so declared to prohibit direct deletion.
   //
   auto base = IsBaseClass();
   auto solo = IsSingleton();
   auto ctor = FindCtor(nullptr);
   auto dtor = FindDtor();

   if(dtor == nullptr)
   {
      if(solo)
         Log(DestructorNotPrivate);
      else if(base)
      {
         if((ctor == nullptr) || !ctor->IsDeleted())
            Log(NonVirtualDestructor);
      }
      return;
   }

   auto acc = dtor->GetAccess();

   if(solo && (acc != Cxx::Private))
      dtor->Log(DestructorNotPrivate);
   else if(base && !dtor->IsVirtual())
      dtor->Log(NonVirtualDestructor);
   else if(base && (acc != Cxx::Public) && !IsSingletonBase())
   {
      for(auto c = BaseClass(); c != nullptr; c = c->BaseClass())
      {
         auto d = c->FindDtor();
         if((d != nullptr) && (d->GetAccess() != Cxx::Public)) return;
      }

      dtor->Log(VirtualDestructor);
   }
}

//------------------------------------------------------------------------------

bool Class::CheckIfUnused(Warning warning) const
{
   Debug::ft("Class.CheckIfUnused");

   auto attrs = GetUsageAttrs();

   //  If the class is derived from a class, it can remain a class.
   //  If the class has a public inner class or public member functions
   //  or data, suggest making it a struct unless it has private items,
   //  in which case it should be a class.
   //
   if(attrs.test(HasPublicInnerClass) ||
      attrs.test(HasPublicMemberFunction) ||
      attrs.test(HasPublicMemberData))
   {
      auto base = BaseClass();
      if((base != nullptr) && (base->GetClassTag() == Cxx::ClassType))
      {
         if(tag_ == Cxx::ClassType) return false;
      }

      if(attrs.test(IsBase) ||
         attrs.test(HasNonPublicInnerClass) ||
         attrs.test(HasNonPublicMemberFunction) ||
         attrs.test(HasNonPublicMemberData) ||
         attrs.test(HasNonPublicStaticFunction) ||
         attrs.test(HasNonPublicStaticData))
      {
         if(tag_ == Cxx::StructType) Log(StructCouldBeClass);
      }
      else
      {
         if(tag_ == Cxx::ClassType) Log(ClassCouldBeStruct);
      }

      return false;
   }

   //  If the class only has public static functions and data, or enums and
   //  typedefs, suggest making it a namespace unless it is derived, created,
   //  or instantiated.
   //
   if(attrs.test(HasPublicStaticFunction) ||
      attrs.test(HasPublicStaticData) ||
      attrs.test(HasEnum) ||
      attrs.test(HasTypedef))
   {
      if(BaseClass() != nullptr) return false;

      if(WasCreated(true) ||
         attrs.test(HasInstantiations) ||
         attrs.test(HasNonPublicInnerClass) ||
         attrs.test(HasNonPublicMemberFunction) ||
         attrs.test(HasNonPublicMemberData))
      {
         if(tag_ == Cxx::StructType) Log(StructCouldBeClass);
         return false;
      }

      if(attrs.test(HasNonPublicStaticFunction) ||
         attrs.test(HasNonPublicStaticData))
      {
         auto ctor = FindCtor(nullptr);
         if((ctor == nullptr) || !ctor->IsDeleted())
         {
            Log(CtorCouldBeDeleted);
         }
         return false;
      }

      Log(ClassCouldBeNamespace);
      return false;
   }

   if(IsTemplate())
   {
      //  A class template is unused if it has no instantiations.
      //
      if(!attrs.test(HasInstantiations))
      {
         Log(warning);
         return true;
      }
   }
   else
   {
      //  A class is unused if it is never constructed.  Non-public items can
      //  only used by the class itself (though GetUsageAttrs treats protected
      //  members as private, which could cause inaccuracies).  In any case,
      //  the class is only considered used if it is constructed or it is has
      //  public items that are used (if any such item exists, this function
      //  would already have returned).
      //
      if(!attrs.test(IsConstructed))
      {
         Log(warning);
         return true;
      }
   }

   return false;
}

//------------------------------------------------------------------------------

void Class::CheckOverrides() const
{
   Debug::ft("Class.CheckOverrides");

   //  Check for overrides of Patch and Display.  The following are exempt:
   //  o Template instances (any warnings apply to the template).
   //  o Classes not derived from Base (Display).
   //  o Templates and classes not derived from Object (Patch).
   //
   if(IsInternal()) return;
   if(!DerivesFrom("Base")) return;
   auto patch = !IsTemplate() && DerivesFrom("Object");

   //  Unless the class has no data, or only static const data, it
   //  should override Display.
   //
   auto display = false;
   auto data = Datas();

   for(auto d = data->cbegin(); d != data->cend(); ++d)
   {
      if((*d)->IsStatic() && (*d)->IsConst()) continue;
      display = true;
      break;
   }

   //  Look for overrides of Patch and Display.  Classes without
   //  a standard function are exempt from overriding Patch.
   //
   auto simple = true;
   auto funcs = Funcs();

   for(auto f = funcs->cbegin(); f != funcs->cend(); ++f)
   {
      auto func = f->get();

      if(func->IsOverride())
      {
         if(patch && (func->Name() == "Patch")) patch = false;
         if(display && (func->Name() == "Display")) display = false;
      }

      if(func->FuncType() == FuncStandard) simple = false;
   }

   if(display) Log(DisplayNotOverridden);
   if(patch && !simple) Log(PatchNotOverridden);
}

//------------------------------------------------------------------------------

void Class::CheckRuleOfThree() const
{
   Debug::ft("Class.CheckRuleOfThree");

   if(GetFile()->IsSubsFile()) return;

   //  The warnings logged here all involve defining a copy constructor or
   //  copy operator.  Both should be deleted by a base class specifically
   //  intended for singletons, or by a singleton that is not derived from
   //  such a class. So if this class has such a base class, none of these
   //  warnings apply to it.
   //
   if(HasSingletonBase()) return;

   auto dtor = FindDtor();
   auto copyCtor = GetFuncDefinition(CopyCtor);
   auto copyOper = GetFuncDefinition(CopyOper);

   if(IsSingleton() || IsSingletonBase())
   {
      if((copyCtor != LocalDeleted) && (copyCtor != BaseDeleted))
      {
         Log(CopyCtorNotDeleted);
      }

      if((copyOper != LocalDeleted) && (copyOper != BaseDeleted))
      {
         Log(CopyOperNotDeleted);
      }

      return;
   }

   if((copyCtor == LocalDeclared) || (copyCtor == LocalDeleted))
   {
      if((copyOper == NotDeclared) || (copyOper == BaseDefined))
      {
         Log(RuleOf3CopyCtorNoOper);
      }
   }

   if((copyOper == LocalDeclared) || (copyOper == LocalDeleted))
   {
      if((copyCtor == NotDeclared) || (copyCtor == BaseDefined))
      {
         Log(RuleOf3CopyOperNoCtor);
      }
   }

   //  If the destructor is not trivial, then the copy constructor and copy
   //  operator should be defined or deleted unless the move constructor or
   //  move operator is defined, in which case they need not be defined.
   //
   if((dtor != nullptr) && !dtor->IsTrivial())
   {
      auto moveCtorLoc = GetFuncDefinition(MoveCtor);
      auto moveOperLoc = GetFuncDefinition(MoveOper);

      if((moveCtorLoc == NotDeclared) && (moveOperLoc == NotDeclared))
      {
         if((copyCtor == NotDeclared) || (copyCtor == BaseDefined))
         {
            Log(RuleOf3DtorNoCopyCtor);
         }

         if((copyOper == NotDeclared) || (copyOper == BaseDefined))
         {
            Log(RuleOf3DtorNoCopyOper);
         }
      }
   }
}

//------------------------------------------------------------------------------

Distance Class::ClassDistance(const Class* cls) const
{
   Debug::ft("Class.ClassDistance");

   Distance dist = 0;

   for(auto curr = this; curr != nullptr; curr = curr->BaseClass())
   {
      if(curr == cls) return dist;
      ++dist;
   }

   return NOT_A_SUBCLASS;
}

//------------------------------------------------------------------------------

size_t Class::CreateCode(const ClassInst* inst, stringPtr& code) const
{
   Debug::ft("Class.CreateCode");

   //  If this is a class template, get its source code.
   //
   auto& tmpltName = Name();
   if(!IsTemplate()) return CreateCodeError(tmpltName, 0);

   if(code_ == nullptr)
   {
      //  This is the first instantiation, so get the class template's code.
      //
      std::ostringstream stream;
      Flags flags(NS_Mask | Code_Mask | NoTP_Mask);

      Display(stream, EMPTY_STR, flags);

      //  Include any static member initializations.  Remove their template
      //  parameters: the template name that precedes them will be replaced
      //  with one that includes the template arguments.
      //
      auto datas = Datas();
      for(auto d = datas->cbegin(); d != datas->cend(); ++d)
      {
         if((*d)->IsStatic())
         {
            auto mate = (*d)->GetMate();
            if(mate != nullptr)
            {
               std::ostringstream initStream;
               mate->Display(initStream, EMPTY_STR, flags);
               auto initCode = initStream.str();
               auto left = initCode.find('<');
               auto eqpos = initCode.find('=');
               auto right = initCode.rfind('>', eqpos);
               initCode.erase(left, right - left + 1);
               stream << initCode;
            }
         }
      }

      code_.reset(new string(stream.str()));
   }

   code.reset(new string(*code_));

   //  If the template is a specialization, delete its arguments.
   //
   auto tmpltSpec = GetQualName()->GetTemplatedName();

   if(tmpltSpec != nullptr)
   {
      auto begin = code->find('<');
      if(begin == string::npos) return CreateCodeError(tmpltName, 1);
      auto end = code->find('{');
      if(end == string::npos) return CreateCodeError(tmpltName, 2);
      end = code->rfind('>', end);
      if(end == string::npos) return CreateCodeError(tmpltName, 3);
      code->erase(begin, end - begin + 1);
   }

   //  Replace the template name with the instance name, except within
   //  inner templates or when followed by template parameters.  Lexer
   //  must be reinitialized each time through because it caches the
   //  length of CODE, which changes because of symbol substitution.
   //
   Lexer lexer;
   auto& instName = inst->Name();
   auto begin = code->find(tmpltName);

   while(true)
   {
      auto end = code->find(TEMPLATE_STR, begin);
      end = Replace(*code, tmpltName, instName, begin, end, '<');
      if(end == string::npos) break;
      begin = code->find('{', end);
      if(begin == string::npos) return CreateCodeError(tmpltName, 4);
      lexer.Initialize(*code);
      lexer.Reposition(begin);
      begin = lexer.FindClosing('{', '}', begin + 1);
      if(begin == string::npos) return CreateCodeError(tmpltName, 5);
   }

   //  Replace the template parameters with the instance arguments.
   //
   begin = code->find(instName) + instName.size();
   ReplaceTemplateParms(*code, inst->GetTemplatedName()->Args(), begin);

   return begin;
}

//------------------------------------------------------------------------------

ClassInst* Class::CreateInstance(const string& name, const TypeName* type)
{
   Debug::ft("Class.CreateInstance");

   QualNamePtr newName(new QualName(name));
   newName->CopyContext(this, true);
   ClassInstPtr tmplt(new ClassInst(newName, this, type));
   auto inst = tmplt.get();
   inst->CopyContext(this, true);
   tmplts_.push_back(std::move(tmplt));
   return inst;
}

//------------------------------------------------------------------------------

void Class::Creating()
{
   Debug::ft("Class.Creating");

   created_ = true;
}

//------------------------------------------------------------------------------

Cxx::Access Class::DefaultAccess() const
{
   Debug::ft("Class.DefaultAccess");

   return (tag_ == Cxx::ClassType ? Cxx::Private : Cxx::Public);
}

//------------------------------------------------------------------------------

void Class::Delete()
{
   Debug::ftnt("Class.Delete");

   GetArea()->EraseClass(this);
   delete this;
}

//------------------------------------------------------------------------------

bool Class::DerivesFrom(const Class* cls) const
{
   Debug::ft("Class.DerivesFrom(class)");

   auto dist = ClassDistance(cls);
   return ((dist > 0) && (dist != NOT_A_SUBCLASS));
}

//------------------------------------------------------------------------------

bool Class::DerivesFrom(const string& name) const
{
   Debug::ft("Class.DerivesFrom(name)");

   for(auto s = BaseClass(); s != nullptr; s = s->BaseClass())
   {
      if(s->Name() == name) return true;
   }

   return false;
}

//------------------------------------------------------------------------------

void Class::DestructMembers() const
{
   Debug::ft("Class.DestructMembers");

   auto datas = Datas();

   for(auto d = datas->cbegin(); d != datas->cend(); ++d)
   {
      if(!(*d)->IsStatic())
      {
         auto cls = (*d)->DirectClass();

         if(cls != nullptr)
         {
            auto dtor = cls->FindDtor();
            if(dtor != nullptr) dtor->WasCalled();
         }
      }
   }
}

//------------------------------------------------------------------------------

void Class::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   auto code = options.test(DispCode);
   auto fq = options.test(DispFQ);
   stream << prefix;
   DisplayBase(stream, options);

   if(!fq && !code && IsAtFileScope())
   {
      stream << " // ";
      DisplayFiles(stream);
   }

   auto lead = prefix + spaces(IndentSize());
   auto qual = options;
   auto nonqual = options;
   qual.set(DispFQ);
   nonqual.reset(DispFQ);
   nonqual.reset(DispNoTP);

   stream << CRLF << prefix << '{' << CRLF;
   SortAndDisplayItemPtrs(friends_, stream, lead, qual, SortByPos);
   SortAndDisplayItemPtrs(*Asserts(), stream, lead, qual, SortByPos);
   SortAndDisplayItemPtrs(*Usings(), stream, lead, qual, SortByPos);
   SortAndDisplayItemPtrs(*Forws(), stream, lead, qual, SortByPos);
   SortAndDisplayItemPtrs(*Classes(), stream, lead, nonqual, SortByPos);
   SortAndDisplayItemPtrs(*Enums(), stream, lead, nonqual, SortByPos);
   SortAndDisplayItemPtrs(*Types(), stream, lead, nonqual, SortByPos);
   if(code) SortAndDisplayItemPtrs(*Datas(), stream, lead, nonqual, SortByPos);
   SortAndDisplayItemPtrs(*Funcs(), stream, lead, nonqual, SortByPos);
   SortAndDisplayItemPtrs(*Opers(), stream, lead, nonqual, SortByPos);
   SortAndDisplayItemPtrs(*Assembly(), stream, lead, qual, SortByPos);

   if(!code)
   {
      SortAndDisplayItemPtrs(*Datas(), stream, lead, nonqual, SortByPos);

      lead += spaces(IndentSize());

      if(IsBaseClass())
      {
         stream << prefix << spaces(IndentSize()) << "subclasses:" << CRLF;

         for(auto s = subs_.cbegin(); s != subs_.cend(); ++s)
         {
            stream << lead << (*s)->ScopedName(true) << CRLF;
         }
      }

      if(!tmplts_.empty())
      {
         stream << prefix << spaces(IndentSize())
            << "instantiations (" << tmplts_.size() << "):" << CRLF;

         for(auto t = tmplts_.cbegin(); t != tmplts_.cend(); ++t)
         {
            (*t)->Display(stream, lead, options);
         }
      }
   }

   stream << prefix << "};" << CRLF;
}

//------------------------------------------------------------------------------

void Class::DisplayBase(ostream& stream, const Flags& options) const
{
   if(!options.test(DispNoTP))
   {
      if(parms_ != nullptr) parms_->Print(stream, options);
   }

   if(OuterClass() != nullptr) stream << GetAccess() << ": ";
   stream << tag_;

   if(alignas_ != nullptr)
   {
      stream << SPACE;
      alignas_->Print(stream, options);
   }

   if(Name().front() != '$')
   {
      stream << SPACE;
      strName(stream, options.test(DispFQ), name_.get());
   }

   auto base = GetBaseDecl();
   if(base != nullptr) base->DisplayDecl(stream, true);
}

//------------------------------------------------------------------------------

void Class::DisplayHierarchy(ostream& stream, const string& prefix) const
{
   stream << prefix << ScopedName(true) << CRLF;

   auto lead = prefix + spaces(IndentSize());

   for(auto s = subs_.cbegin(); s != subs_.cend(); ++s)
   {
      (*s)->DisplayHierarchy(stream, lead);
   }
}

//------------------------------------------------------------------------------

fn_name Class_EnsureInstance = "Class.EnsureInstance";

ClassInst* Class::EnsureInstance(const TypeName* type)
{
   Debug::ft(Class_EnsureInstance);

   //  This should only be invoked on a class template.
   //
   if(!IsTemplate())
   {
      auto expl = Name() + " is not a class template";
      Context::SwLog(Class_EnsureInstance, expl, 0);
      return nullptr;
   }

   //  See if the template instance already exists.  Visibility to the
   //  current scope is irrelevant because all users can share the same
   //  instance.
   //
   auto syms = Singleton<CxxSymbols>::Instance();
   auto name = Name() + type->TypeString(true);
   SymbolVector list;
   syms->FindItems(name, CLASS_MASK, list);

   if(!list.empty())
   {
      return static_cast<ClassInst*>(list.front());
   }

   //  The instance doesn't exist, so create it.  If the template class
   //  has specializations, choose the most appropriate one.
   //
   auto file = Context::File();
   if(file == nullptr) return nullptr;
   auto scope = GetScope();
   ViewVector views;
   auto area = static_cast<CxxArea*>(GetScope());
   syms->FindSymbols(file, scope, Name(), CLASS_MASK, list, views, area);

   Class* base = this;

   if(list.size() > 1)
   {
      TypeMatch match = Incompatible;

      for(auto s = list.cbegin(); s != list.cend(); ++s)
      {
         auto c = static_cast<Class*>(*s);
         auto m = c->MatchTemplate(*type);

         if(m >= match)
         {
            base = c;
            match = m;
         }
      }
   }

   return base->CreateInstance(name, type);
}

//------------------------------------------------------------------------------

void Class::EnterParms() const
{
   Debug::ft("Class.EnterParms");

   if(parms_ != nullptr) parms_->EnterBlock();
}

//------------------------------------------------------------------------------

bool Class::EnterScope()
{
   Debug::ft("Class.EnterScope");

   if(IsAtFileScope()) GetFile()->InsertClass(this);
   if(parms_ != nullptr) parms_->EnterScope();
   if(alignas_ != nullptr) alignas_->EnterBlock();

   //  If this class has no constructor, invoke the
   //  default constructor in its base class.
   //
   if(FindFuncByRole(PureCtor, false) == nullptr)
   {
      InvokeDefaultBaseCtor();
   }
   return true;
}

//------------------------------------------------------------------------------

void Class::EraseFriend(const Friend* decl)
{
   Debug::ft("Class.EraseFriend");

   EraseItemPtr(friends_, decl);
}

//------------------------------------------------------------------------------

void Class::EraseSubclass(const Class* cls)
{
   Debug::ft("Class.EraseSubclass");

   EraseItem(subs_, cls);
}

//------------------------------------------------------------------------------

void Class::ExitParms() const
{
   Debug::ft("Class.ExitParms");

   if(parms_ != nullptr) parms_->ExitBlock();
}

//------------------------------------------------------------------------------

Function* Class::FindCtor
   (StackArgVector* args, const CxxScope* scope, SymbolView* view) const
{
   Debug::ft("Class.FindCtor");

   //  If no arguments were provided, look for the default constructor.
   //  If there isn't one, return the first constructor found (if any).
   //
   if(args == nullptr)
   {
      Function* ctor = nullptr;
      auto funcs = CxxArea::Funcs();

      for(auto f = funcs->begin(); f != funcs->end(); ++f)
      {
         if((*f)->FuncType() == FuncCtor)
         {
            ctor = f->get();
            if((*f)->GetArgs().size() == 1) return f->get();
         }
      }

      return ctor;
   }

   //  If no "this" argument was provided, insert one.
   //
   if(args->empty() || !args->front().IsThis())
   {
      auto self = const_cast<Class*>(this);
      args->insert(args->begin(), StackArg(self, 1, true, false));
   }

   return FindFunc(Name(), nullptr, args, false, scope, view);
}

//------------------------------------------------------------------------------

void Class::FindCtors(FunctionVector& ctors) const
{
   Debug::ft("Class.FindCtors");

   auto funcs = Funcs();

   for(auto f = funcs->cbegin(); f != funcs->cend(); ++f)
   {
      if((*f)->FuncRole() == PureCtor)
      {
         ctors.push_back(f->get());
      }
   }
}

//------------------------------------------------------------------------------

Function* Class::FindDtor() const
{
   Debug::ft("Class.FindDtor");

   const FunctionPtrVector* funcs = Funcs();

   for(auto f = funcs->cbegin(); f != funcs->cend(); ++f)
   {
      if((*f)->Name().front() == '~') return f->get();
   }

   return nullptr;
}

//------------------------------------------------------------------------------

Friend* Class::FindFriend(const CxxScope* scope) const
{
   Debug::ft("Class.FindFriend");

   if(friends_.empty()) return nullptr;

   auto fqScope = scope->ScopedName(true);

   for(auto f = friends_.cbegin(); f != friends_.cend(); ++f)
   {
      if((*f)->IsSuperscopeOf(fqScope, true)) return f->get();
   }

   return nullptr;
}

//------------------------------------------------------------------------------

Function* Class::FindFunc(const string& name,
   const StackArg* call, StackArgVector* args, bool base,
   const CxxScope* scope, SymbolView* view) const
{
   Debug::ft("Class.FindFunc");

   auto f = CxxArea::FindFunc(name, call, args, false, scope, view);
   if(MemberIsAccessibleTo(f, scope, view))
      return FuncAccessed(f, call, scope, view);

   if(IsTemplate())
   {
      f = InstantiateFunction(name, args, view);
      if(MemberIsAccessibleTo(f, scope, view))
         return FuncAccessed(f, call, scope, view);
   }

   if(!base) return nullptr;

   for(auto s = BaseClass(); s != nullptr; s = s->BaseClass())
   {
      f = s->FindFunc(name, call, args, false, scope, view);
      if(MemberIsAccessibleTo(f, scope, view))
         return FuncAccessed(f, call, scope, view);
   }

   return nullptr;
}

//------------------------------------------------------------------------------

fn_name Class_FindFuncByRole = "Class.FindFuncByRole";

Function* Class::FindFuncByRole(FunctionRole role, bool base) const
{
   Debug::ft(Class_FindFuncByRole);

   if(role == FuncOther)
   {
      Context::SwLog(Class_FindFuncByRole, "Role not supported", role);
      return nullptr;
   }

   const FunctionPtrVector* funcs = nullptr;

   switch(role)
   {
   case CopyOper:
   case MoveOper:
      funcs = Opers();
      break;
   default:
      funcs = Funcs();
   }

   //  If looking for a constructor, it must have no arguments (which,
   //  internally, is actually one, because it gets an implicit "this").
   //
   for(auto f = funcs->cbegin(); f != funcs->cend(); ++f)
   {
      if((*f)->FuncRole() == role)
      {
         if((role != PureCtor) || ((*f)->GetArgs().size() == 1))
         {
            return f->get();
         }
      }
   }

   if(!base) return nullptr;
   auto super = BaseClass();
   if(super == nullptr) return nullptr;
   return super->FindFuncByRole(role, base);
}

//------------------------------------------------------------------------------

CxxScoped* Class::FindMember(const string& name,
   bool base, const CxxScope* scope, SymbolView* view) const
{
   Debug::ft("Class.FindMember");

   auto item = CxxArea::FindItem(name);

   if(item != nullptr)
   {
      //  The accessibility of a function to SCOPE is rechecked later, when
      //  the function arguments allow the correct override to be selected.
      //
      if(MemberIsAccessibleTo(item, scope, view)) return item;
      if(item->Type() == Cxx::Function) return item;
      return nullptr;
   }

   //  Return if the search is not to include base classes or there is no
   //  base class.  Otherwise, continue the search up the class hierarchy.
   //
   if(!base) return nullptr;
   auto super = BaseClass();
   if(super == nullptr) return nullptr;
   return super->FindMember(name, base, scope, view);
}

//------------------------------------------------------------------------------

CxxScoped* Class::FindName(const string& name, const Class* base) const
{
   Debug::ft("Class.FindName");

   auto item = FindMember(name, false);
   if(item != nullptr) return item;

   auto s = BaseClass();
   if((s == nullptr) || (s == base)) return nullptr;
   return s->FindName(name, base);
}

//------------------------------------------------------------------------------

bool Class::FuncToIndex(const Function* func, size_t& idx) const
{
   Debug::ft("Class.FuncToIndex");

   auto list = FuncVector(func->Name());
   size_t inlines = 0;

   for(idx = 0; idx < list->size(); ++idx)
   {
      auto f = list->at(idx).get();
      if(f->IsInline()) ++inlines;

      if(f == func)
      {
         idx -= inlines;
         return true;
      }
   }

   return false;
}

//------------------------------------------------------------------------------

Class* Class::GetClassTemplate() const
{
   if(!IsTemplate()) return nullptr;
   return const_cast<Class*>(this);
}

//------------------------------------------------------------------------------

void Class::GetConvertibleTypes(StackArgVector& types, bool expl)
{
   Debug::ft("Class.GetConvertibleTypes");

   Instantiate();

   for(auto cls = this; cls != nullptr; cls = cls->BaseClass())
   {
      auto opers = cls->Opers();

      for(auto o = opers->cbegin(); o != opers->cend(); ++o)
      {
         auto oper = o->get();

         if(oper->Operator() == Cxx::CAST)
         {
            if(!expl || !oper->IsExplicit())
            {
               auto spec = oper->GetTypeSpec();
               types.push_back(spec->ResultType());
            }
         }
      }
   }
}

//------------------------------------------------------------------------------

Cxx::Access Class::GetCurrAccess() const
{
   //  When a class is created, currAccess_ is set to the out-of-bounds value
   //  Cxx::Access_N.  This prevents a RedundantAccessControl warning when the
   //  class's default value (e.g. "private:") is specified first.  However, it
   //  also means that the default value must be correctly determined.
   //
   if(currAccess_ == Cxx::Access_N)
   {
      return (tag_ == Cxx::ClassType ? Cxx::Private : Cxx::Public);
   }

   return currAccess_;
}

//------------------------------------------------------------------------------

void Class::GetDecls(CxxNamedSet& items)
{
   CxxArea::GetDecls(items);

   items.insert(this);

   for(auto f = friends_.cbegin(); f != friends_.cend(); ++f)
   {
      (*f)->GetDecls(items);
   }
}

//------------------------------------------------------------------------------

void Class::GetDirectClasses(CxxUsageSets& symbols)
{
   Debug::ft("Class.GetDirectClasses");

   symbols.AddDirect(this);
}

//------------------------------------------------------------------------------

FunctionDefinition Class::GetFuncDefinition(FunctionRole role) const
{
   Debug::ft("Class.GetFuncDefinition");

   auto func = FindFuncByRole(role, true);
   if(func == nullptr) return NotDeclared;

   if(func->GetScope() == this)
   {
      return (func->IsDeleted() ? LocalDeleted : LocalDeclared);
   }

   if(!func->IsImplemented() || (func->GetAccess() == Cxx::Private))
   {
      return BaseDeleted;
   }

   return BaseDefined;
}

//------------------------------------------------------------------------------

FunctionVector Class::GetFunctions() const
{
   Debug::ft("Class.GetFunctions");

   FunctionVector vec;

   auto funcs = Funcs();
   for(auto f = funcs->begin(); f != funcs->end(); ++f)
   {
      vec.push_back(f->get());
   }

   auto opers = Opers();
   for(auto o = opers->begin(); o != opers->end(); ++o)
   {
      vec.push_back(o->get());
   }

   std::sort(vec.begin(), vec.end(), IsSortedByPos);
   return vec;
}

//------------------------------------------------------------------------------

void Class::GetMemberInitAttrs(DataInitVector& members) const
{
   Debug::ft("Class.GetMemberInitAttrs");

   auto data = Datas();

   for(size_t i = 0; i < data->size(); ++i)
   {
      //  The member should be initialized if it is not default constructible.
      //  However, exempt a member that appears in a union or that is a direct
      //  template parameter.
      //
      auto mem = data->at(i).get();
      auto init = (!mem->IsDefaultConstructible() && !mem->IsUnionMember());

      if(init)
      {
         auto spec = mem->GetTypeSpec();

         if((spec->GetTemplateRole() == TemplateParameter) &&
            (spec->Ptrs(false) == 0))
         {
            init = false;
         }
      }

      DataInitAttrs attrs(mem, init, 0);
      members.push_back(attrs);
   }
}

//------------------------------------------------------------------------------

bool Class::GetSpan(size_t& begin, size_t& left, size_t& end) const
{
   Debug::ft("Class.GetSpan");

   return GetBracedSpan(begin, left, end);
}

//------------------------------------------------------------------------------

CxxScope* Class::GetTemplate() const
{
   if(!IsTemplate()) return nullptr;
   return const_cast<Class*>(this);
}

//------------------------------------------------------------------------------

Class::UsageAttributes Class::GetUsageAttrs() const
{
   Debug::ft("Class.GetUsageAttrs");

   UsageAttributes attrs;

   if(IsBaseClass()) attrs.set(IsBase);
   if(!tmplts_.empty()) attrs.set(HasInstantiations);
   if(implicit_) attrs.set(IsConstructed);

   auto classes = Classes();
   for(auto c = classes->cbegin(); c != classes->cend(); ++c)
   {
      if(!(*c)->IsUnused())
      {
         if((*c)->GetAccess() == Cxx::Public)
            attrs.set(HasPublicInnerClass);
         else
            attrs.set(HasNonPublicInnerClass);
      }
   }

   auto opers = Opers();
   for(auto o = opers->cbegin(); o != opers->cend(); ++o)
   {
      if((*o)->HasInvokers())
      {
         if((*o)->GetAccess() == Cxx::Public)
         {
            if((*o)->FuncRole() != FuncOther)
               attrs.set(HasPublicSpecialFunction);
            else if((*o)->IsStatic())
               attrs.set(HasPublicStaticFunction);
            else
               attrs.set(HasPublicMemberFunction);
         }
         else
         {
            if((*o)->FuncRole() != FuncOther)
               attrs.set(HasNonPublicSpecialFunction);
            else if((*o)->IsStatic())
               attrs.set(HasNonPublicStaticFunction);
            else
               attrs.set(HasNonPublicMemberFunction);
         }
      }
   }

   auto funcs = Funcs();
   for(auto f = funcs->cbegin(); f != funcs->cend(); ++f)
   {
      if((*f)->HasInvokers())
      {
         if((*f)->FuncRole() == PureCtor)
         {
            attrs.set(IsConstructed);
         }

         if((*f)->GetAccess() == Cxx::Public)
         {
            if((*f)->FuncRole() != FuncOther)
               attrs.set(HasPublicSpecialFunction);
            else if((*f)->IsStatic())
               attrs.set(HasPublicStaticFunction);
            else
               attrs.set(HasPublicMemberFunction);
         }
         else
         {
            if((*f)->FuncRole() != FuncOther)
               attrs.set(HasNonPublicSpecialFunction);
            else if((*f)->IsStatic())
               attrs.set(HasNonPublicStaticFunction);
            else
               attrs.set(HasNonPublicMemberFunction);
         }
      }
   }

   auto data = Datas();
   for(auto d = data->cbegin(); d != data->cend(); ++d)
   {
      if(!(*d)->IsUnused())
      {
         if((*d)->GetAccess() == Cxx::Public)
         {
            if((*d)->IsStatic())
               attrs.set(HasPublicStaticData);
            else
               attrs.set(HasPublicMemberData);
         }
         else
         {
            if((*d)->IsStatic())
               attrs.set(HasNonPublicStaticData);
            else
               attrs.set(HasNonPublicMemberData);
         }
      }
   }

   auto enums = Enums();
   for(auto e = enums->cbegin(); e != enums->cend(); ++e)
   {
      if(!(*e)->IsUnused()) attrs.set(HasEnum);
   }

   auto types = Types();
   for(auto t = types->cbegin(); t != types->cend(); ++t)
   {
      if(!(*t)->IsUnused()) attrs.set(HasTypedef);
   }

   return attrs;
}

//------------------------------------------------------------------------------

void Class::GetUsages(const CodeFile& file, CxxUsageSets& symbols)
{
   //  If this is a class template, obtain usage information from its first
   //  instance in case some symbols in the template could not be resolved.
   //
   if(!tmplts_.empty())
   {
      auto first = tmplts_.front().get();
      first->GetUsages(file, symbols);
   }

   if(parms_ != nullptr) parms_->GetUsages(file, symbols);
   if(alignas_ != nullptr) alignas_->GetUsages(file, symbols);

   auto base = GetBaseDecl();

   if(base != nullptr) base->GetUsages(file, symbols);

   for(auto f = friends_.cbegin(); f != friends_.cend(); ++f)
   {
      (*f)->GetUsages(file, symbols);
   }

   auto classes = Classes();

   for(auto c = classes->cbegin(); c != classes->cend(); ++c)
   {
      (*c)->GetUsages(file, symbols);
   }

   auto types = Types();

   for(auto t = types->cbegin(); t != types->cend(); ++t)
   {
      (*t)->GetUsages(file, symbols);
   }

   auto inst = IsInTemplateInstance();
   auto funcs = Funcs();

   for(auto f = funcs->cbegin(); f != funcs->cend(); ++f)
   {
      //  If this is not a class template instance, bypass function
      //  template instantiations, every one of which is registered
      //  against the function template.
      //
      if(!inst && (*f)->IsInternal()) continue;

      (*f)->GetUsages(file, symbols);
   }

   auto opers = Opers();

   for(auto o = opers->cbegin(); o != opers->cend(); ++o)
   {
      (*o)->GetUsages(file, symbols);
   }

   auto data = Datas();

   for(auto d = data->cbegin(); d != data->cend(); ++d)
   {
      (*d)->GetUsages(file, symbols);
   }

   auto asserts = Asserts();

   for(auto a = asserts->cbegin(); a != asserts->cend(); ++a)
   {
      (*a)->GetUsages(file, symbols);
   }
}

//------------------------------------------------------------------------------

Using* Class::GetUsingFor(const string& fqName,
   size_t prefix, const CxxNamed* item, const CxxScope* scope) const
{
   Debug::ft("Class.GetUsingFor");

   auto usings = Usings();

   for(auto u = usings->cbegin(); u != usings->cend(); ++u)
   {
      if((*u)->IsUsingFor(fqName, prefix, scope)) return u->get();
   }

   return nullptr;
}

//------------------------------------------------------------------------------

bool Class::HasPODMember() const
{
   Debug::ft("Class.HasPODMember");

   auto data = Datas();

   for(auto d = data->cbegin(); d != data->cend(); ++d)
   {
      if(!(*d)->IsStatic() && (*d)->IsPOD()) return true;
   }

   return false;
}

//------------------------------------------------------------------------------

bool Class::HasSingletonBase() const
{
   Debug::ft("Class.HasSingletonBase");

   for(auto c = BaseClass(); c != nullptr; c = c->BaseClass())
   {
      if(c->IsSingletonBase()) return true;
   }

   return false;
}

//------------------------------------------------------------------------------

Function* Class::IndexToFunc(const std::string& name, size_t idx) const
{
   Debug::ft("Class.IndexToFunc");

   auto list = FuncVector(name);
   size_t count = 0;

   for(size_t i = 0; i < list->size(); ++i)
   {
      auto f = list->at(i).get();
      if(f->IsInline()) continue;
      if(count++ == idx) return f;
   }

   return nullptr;
}

//------------------------------------------------------------------------------

Function* Class::InstantiateFunction
   (const string& name, StackArgVector* args, SymbolView* view) const
{
   Debug::ft("Class.InstantiateFunction");

   //c Support class template argument deduction.

   return nullptr;
}

//------------------------------------------------------------------------------

void Class::InvokeCopyCtor() const
{
   Debug::ft("Class.InvokeCopyCtor");

   auto ctor = FindFuncByRole(CopyCtor, false);

   if(ctor != nullptr)
   {
      auto scope = Context::Scope();
      SymbolView view;

      if(MemberIsAccessibleTo(ctor, scope, &view))
      {
         FuncAccessed(ctor, nullptr, scope, &view);
      }
   }
}

//------------------------------------------------------------------------------

void Class::InvokeDefaultBaseCtor() const
{
   Debug::ft("Class.InvokeDefaultBaseCtor");

   auto base = BaseClass();
   if(base == nullptr) return;
   auto ctor = base->FindCtor(nullptr);
   if(ctor == nullptr) return;
   ctor->WasCalled();
   ctor->RecordAccess(Cxx::Protected);
}

//------------------------------------------------------------------------------

bool Class::IsDefaultConstructible()
{
   Debug::ft("Class.IsDefaultConstructible");

   //  A class is default constructible if
   //  o it is not a union;
   //  o it implements a default constructor or all of its data is default
   //    constructible, in which case the compiler provides the constructor;
   //  o its chain of base classes is default constructible.
   //
   if(FindCtor(nullptr) == nullptr)
   {
      if(tag_ == Cxx::UnionType) return false;

      auto data = Datas();

      for(size_t i = 0; i < data->size(); ++i)
      {
         if(!data->at(i)->IsDefaultConstructible()) return false;
      }
   }

   auto s = BaseClass();
   if(s == nullptr) return true;
   return s->IsDefaultConstructible();
}

//------------------------------------------------------------------------------

bool Class::IsSingleton() const
{
   Debug::ft("Class.IsSingleton");

   for(auto f = friends_.cbegin(); f != friends_.cend(); ++f)
   {
      if((*f)->Name() == "Singleton") return true;
   }

   return false;
}

//------------------------------------------------------------------------------

bool Class::IsSingletonBase() const
{
   Debug::ft("Class.IsSingletonBase");

   if(WasCreated(false)) return false;
   if(IsSingleton()) return false;

   if(subs_.size() + tmplts_.size() == 0)
   {
      auto ctor = FindDtor();
      if(ctor == nullptr) return false;
      if(ctor->GetAccess() != Cxx::Protected) return false;

      auto dtor = FindDtor();
      if(dtor == nullptr) return false;
      if(dtor->GetAccess() != Cxx::Protected) return false;
   }

   for(auto s = subs_.cbegin(); s != subs_.cend(); ++s)
   {
      if(!(*s)->IsSingleton() && !(*s)->IsSingletonBase()) return false;
   }

   for(auto t = tmplts_.cbegin(); t != tmplts_.cend(); ++t)
   {
      if(!(*t)->IsSingleton() && !(*t)->IsSingletonBase()) return false;
   }

   return true;
}

//------------------------------------------------------------------------------

CxxTokenVector Class::Items() const
{
   Debug::ft("Class.Items");

   auto items = CxxArea::Items();

   for(auto f = friends_.cbegin(); f != friends_.cend(); ++f)
   {
      items.push_back(f->get());
   }

   std::sort(items.begin(), items.end(), IsSortedByPos);

   while(!items.empty() && (items.back()->GetPos() == string::npos))
   {
      items.pop_back();
   }

   return items;
}

//------------------------------------------------------------------------------

Function* Class::MatchFunc(const Function* curr, bool base) const
{
   Debug::ft("Class.MatchFunc");

   auto func = CxxArea::MatchFunc(curr, base);
   if(func != nullptr) return func;

   if(!base) return nullptr;
   auto super = BaseClass();
   if(super == nullptr) return nullptr;
   return super->MatchFunc(curr, base);
}

//------------------------------------------------------------------------------

fn_name Class_MatchTemplate = "Class.MatchTemplate";

TypeMatch Class::MatchTemplate(const TypeName& type) const
{
   Debug::ft(Class_MatchTemplate);

   //  This must be a class template.  If it is not a specialization, report the
   //  match as Abridgeable.  This is arbitrary, chosen to give specializations
   //  better gradations for classifying their matches.
   //
   if(!IsTemplate())
   {
      auto expl = Name() + " is not a class template";
      Context::SwLog(Class_MatchTemplate, expl, 0);
      return Incompatible;
   }

   auto spec = GetQualName()->GetTemplatedName();
   if(spec == nullptr) return Abridgeable;

   //  This is a template specialization.  If it and TYPE have the same number
   //  of arguments, find out how well they match.
   //
   auto thisArgs = spec->Args();
   auto thatArgs = type.Args();
   if(thisArgs->size() != thatArgs->size())
   {
      auto expl = "Invalid number of template arguments for " + Name();
      Context::SwLog(Class_MatchTemplate, expl, thatArgs->size());
      return Incompatible;
   }

   auto match = Compatible;
   for(size_t i = 0; i < thisArgs->size(); ++i)
   {
      auto m = thisArgs->at(i)->MatchTemplateArg(thatArgs->at(i).get());
      if(m < match) match = m;
   }

   return match;
}

//------------------------------------------------------------------------------

StackArg Class::NameToArg(Cxx::Operator op, TypeName* name)
{
   Debug::ft("Class.NameToArg");

   //  This will be a constructor call unless this class's size is being
   //  looked up.  Set the "invoke" flag on the argument, which will be
   //  used to invoke a constructor.
   //
   StackArg arg(this, name);
   if(op != Cxx::SIZEOF_TYPE) arg.SetInvoke();
   return arg;
}

//------------------------------------------------------------------------------

Class* Class::OuterClass() const
{
   return GetScope()->GetClass();
}

//------------------------------------------------------------------------------

CxxToken* Class::PosToItem(size_t pos) const
{
   auto item = CxxArea::PosToItem(pos);
   if(item != nullptr) return item;

   item = name_->PosToItem(pos);
   if(item != nullptr) return item;

   if(parms_ != nullptr) item = parms_->PosToItem(pos);
   if(item != nullptr) return item;

   if(base_ != nullptr) item = base_->PosToItem(pos);
   if(item != nullptr) return item;

   for(auto f = friends_.cbegin(); f != friends_.cend(); ++f)
   {
      item = (*f)->PosToItem(pos);
      if(item != nullptr) return item;
   }

   return nullptr;
}

//------------------------------------------------------------------------------

void Class::Rename(const string& name)
{
   Debug::ft("Class.Rename");

   //  Forward declarations must be renamed.  This must be done before renaming
   //  the class, because FindItems compares scoped names to determine which
   //  symbols match the name that is used as a key.
   //
   SymbolVector forwards;
   auto syms = Singleton<CxxSymbols>::Instance();
   syms->FindItems(Name(), CLASS_FORWS, forwards);

   for(auto f = forwards.cbegin(); f != forwards.cend(); ++f)
   {
      if((*f)->Referent() == this)
      {
         (*f)->Rename(name);
      }
   }

   CxxScoped::RenameQual(*name_, name);

   //  Constructors and the destructor must also be renamed, and calls
   //  to Debug::ft must be updated for functions that are not renamed.
   //
   auto dtorName = '~' + name;
   auto funcs = Funcs();

   for(auto f = funcs->cbegin(); f != funcs->cend(); ++f)
   {
      switch((*f)->FuncRole())
      {
      case PureCtor:
      case CopyCtor:
      case MoveCtor:
         (*f)->Rename(name);
         break;
      case PureDtor:
         (*f)->Rename(dtorName);
         break;
      default:
         Editor::UpdateDebugFt(f->get());
      }
   }

   auto opers = Opers();

   for(auto o = opers->cbegin(); o != opers->cend(); ++o)
   {
      Editor::UpdateDebugFt(o->get());
   }
}

//------------------------------------------------------------------------------

void Class::SetAlignment(AlignAsPtr& align)
{
   Debug::ft("Class.SetAlignment");

   alignas_ = std::move(align);
}

//------------------------------------------------------------------------------

bool Class::SetCurrAccess(Cxx::Access access)
{
   Debug::ft("Class.SetCurrAccess");

   if(currAccess_ == access)
   {
      auto parser = Context::GetParser();

      if(parser->ParsingSourceCode())
      {
         auto file = Context::File();
         file->LogPos(parser->GetPrev(), RedundantAccessControl);
      }
   }

   currAccess_ = access;
   return true;
}

//------------------------------------------------------------------------------

void Class::SetTemplateParms(TemplateParmsPtr& parms)
{
   Debug::ft("Class.SetTemplateParms");

   parms_ = std::move(parms);
}

//------------------------------------------------------------------------------

string Class::TypeString(bool arg) const
{
   return Prefix(GetScope()->TypeString(arg)) + Name();
}

//------------------------------------------------------------------------------

void Class::UpdatePos
   (EditorAction action, size_t begin, size_t count, size_t from) const
{
   CxxArea::UpdatePos(action, begin, count, from);
   name_->UpdatePos(action, begin, count, from);
   if(parms_ != nullptr) parms_->UpdatePos(action, begin, count, from);
   if(base_ != nullptr) base_->UpdatePos(action, begin, count, from);

   for(auto f = friends_.cbegin(); f != friends_.cend(); ++f)
   {
      (*f)->UpdatePos(action, begin, count, from);
   }
}

//------------------------------------------------------------------------------

void Class::UpdateXref(bool insert)
{
   CxxArea::UpdateXref(insert);

   if(parms_ != nullptr) parms_->UpdateXref(insert);
   if(alignas_ != nullptr) alignas_->UpdateXref(insert);

   auto base = GetBaseDecl();
   if(base != nullptr) base->UpdateXref(insert);

   for(auto f = friends_.cbegin(); f != friends_.cend(); ++f)
   {
      (*f)->UpdateXref(insert);
   }
}

//------------------------------------------------------------------------------

const Warning RoleToWarning[FuncRole_N] =
{
   ImplicitConstructor,      //  PureCtor
   ImplicitDestructor,       //  PureDtor
   ImplicitCopyConstructor,  //  CopyCtor
   ImplicitCopyConstructor,  //  MoveCtor
   ImplicitCopyOperator,     //  CopyOper
   ImplicitCopyOperator,     //  MoveOper
   Warning_N                 //  FuncOther
};

void Class::WasCalled(FunctionRole role, const CxxNamed* item)
{
   Debug::ft("Class.WasCalled");

   //  The special member function associated with ROLE was invoked.  If
   //  that function is defined, tell it that it was invoked.
   //
   auto func = FindFuncByRole(role, false);

   if(func != nullptr)
   {
      func->WasCalled();
      return;
   }

   //  The special member function is implicitly defined.  Decide whether to
   //  generate a log.  An implicit constructor, copy constructor, or copy
   //  operator always results in a log, and a destructor results in a log if
   //  the class is a base, a singleton, or isn't a struct and has a pointer
   //  member (which might mean that it needs to free free memory).  Propagate
   //  the call up the class hierarchy if a log is not generated.
   //
   implicit_ = true;

   auto base = IsBaseClass();
   auto solo = IsSingleton();
   auto log = true;
   auto data = Datas();

   if(role == PureDtor)
   {
      log = false;

      if(base || solo)
      {
         log = true;
      }
      else if(GetClassTag() == Cxx::ClassType)
      {
         for(auto d = data->cbegin(); d != data->cend(); ++d)
         {
            if(!(*d)->IsStatic() && ((*d)->GetTypeSpec()->Ptrs(false) > 0))
            {
               log = true;
               break;
            }
         }
      }
   }

   if(log)
   {
      auto warning = RoleToWarning[role];

      switch(warning)
      {
      case ImplicitConstructor:
         if(HasPODMember())
            warning = ImplicitPODConstructor;
         else if(solo)
            warning = ConstructorNotPrivate;
         else if(base)
            warning = PublicConstructor;
         break;

      case ImplicitDestructor:
         if(solo)
            warning = DestructorNotPrivate;
         else if(base)
            warning = NonVirtualDestructor;
         break;
      }

      if(warning != Warning_N)
      {
         Log(warning);

         if((item != nullptr) && (item != this))
            item->Log(warning, item, -1);
      }
   }

   if(BaseClass() != nullptr)
   {
      BaseClass()->WasCalled(role, item);
   }
}

//------------------------------------------------------------------------------

bool Class::WasCreated(bool base) const
{
   Debug::ft("Class.WasCreated");

   if(created_) return true;
   if(!base) return false;

   for(auto c = BaseClass(); c != nullptr; c = c->BaseClass())
   {
      if (c->WasCreated(true)) return true;
   }

   return false;
}

//------------------------------------------------------------------------------

string Class::XrefName(bool templates) const
{
   auto spec = GetQualName()->GetTemplatedName();
   auto name = CxxScoped::XrefName(spec == nullptr);

   if(spec != nullptr)
   {
      auto args = spec->Args();
      std::ostringstream stream;
      Flags options(FQ_Mask);

      name.push_back('<');

      for(size_t i = 0; i < args->size(); ++i)
      {
         args->at(i)->Print(stream, options);
         if(i < args->size() - 1) stream << ',';
      }

      name += stream.str();
      name.push_back('>');
   }

   return name;
}

//==============================================================================

ClassInst::ClassInst(QualNamePtr& name, Class* tmplt, const TypeName* spec) :
   Class(name, tmplt->GetClassTag()),
   tmplt_(tmplt),
   tspec_(nullptr),
   refs_(0),
   instantiated_(false),
   compiled_(false)
{
   Debug::ft("ClassInst.ctor");

   tspec_.reset(new TypeName(*spec));
   tspec_->CopyContext(spec, true);
}

//------------------------------------------------------------------------------

ClassInst::~ClassInst()
{
   Debug::ftnt("ClassInst.dtor");
}

//------------------------------------------------------------------------------

void ClassInst::Check() const
{
   Debug::ft("ClassInst.Check");

   //  Only check the first instance of a class template.  Any warnings
   //  logged against it will be moved to the class template itself.
   //
   if(tmplt_->Instances()->front().get() != this) return;
   Class::Check();
}

//------------------------------------------------------------------------------

void ClassInst::Creating()
{
   Debug::ft("ClassInst.Creating");

   Class::Creating();
   Instantiate();
}

//------------------------------------------------------------------------------

bool ClassInst::DerivesFrom(const Class* cls) const
{
   Debug::ft("ClassInst.DerivesFrom");

   //  This is a class template instance, T<args1>.  If CLS is not a class
   //  template instance or is not an instance of the same class template,
   //  just invoke the base class version of the function.
   //
   if(!cls->IsInTemplateInstance()) return Class::DerivesFrom(cls);
   if(cls->GetTemplate() != tmplt_) return Class::DerivesFrom(cls);
   auto thatSpec = cls->GetTemplatedName();
   if(thatSpec == nullptr) return Class::DerivesFrom(cls);

   //  CLS is of the form T<args2>.  See if args1 are compatible with args2.
   //
   auto thisArgs = tspec_->Args();
   auto thatArgs = thatSpec->Args();
   if(thisArgs->size() != thatArgs->size()) return false;

   auto a1 = thisArgs->cbegin();
   auto a2 = thatArgs->cbegin();

   while(a1 != thisArgs->cend())
   {
      auto thisType = (*a1)->TypeString(true);
      auto thatType = (*a2)->TypeString(true);
      auto thisArg = (*a1)->ResultType();
      auto thatArg = (*a2)->ResultType();
      auto match = thatArg.CalcMatchWith(thisArg, thatType, thisType);
      if(match == Incompatible) return false;
      ++a1;
      ++a2;
   }

   return true;
}

//------------------------------------------------------------------------------

void ClassInst::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   auto code = options.test(DispCode);
   stream << prefix;
   Class::DisplayBase(stream, options);
   if(!instantiated_) stream << ";";

   std::ostringstream buff;
   buff << " // ";
   if(options.test(DispStats)) buff << "r=" << refs_ << SPACE;

   if(!instantiated_)
   {
      if(!code) buff << "<@uninst" << CRLF;
      auto str = buff.str();
      if(str.size() > 4) stream << str;
      return;
   }

   if(!compiled_ && !code) buff << "<@failed to parse";
   auto str = buff.str();
   if(str.size() > 4) stream << str;

   stream << CRLF << prefix << '{' << CRLF;

   if(!compiled_)
   {
      stream << prefix;

      for(size_t pos = 0, size = code_->size(); pos < size; ++pos)
      {
         auto c = code_->at(pos);
         stream << c;
         if(c == CRLF) stream << prefix;
      }

      stream << CRLF;
   }
   else
   {
      auto lead = prefix + spaces(IndentSize());
      auto& opts = options;
      auto qual = options;
      qual.set(DispFQ);

      SortAndDisplayItemPtrs(*Friends(), stream, lead, qual, SortByPos);
      SortAndDisplayItemPtrs(*Usings(), stream, lead, qual, SortByPos);
      SortAndDisplayItemPtrs(*Forws(), stream, lead, qual, SortByPos);
      SortAndDisplayItemPtrs(*Classes(), stream, lead, opts, SortByPos);
      SortAndDisplayItemPtrs(*Enums(), stream, lead, opts, SortByPos);
      SortAndDisplayItemPtrs(*Types(), stream, lead, opts, SortByPos);
      SortAndDisplayItemPtrs(*Funcs(), stream, lead, opts, SortByPos);
      SortAndDisplayItemPtrs(*Opers(), stream, lead, opts, SortByPos);
      SortAndDisplayItemPtrs(*Datas(), stream, lead, opts, SortByPos);
   }

   stream << prefix << "};" << CRLF;
}

//------------------------------------------------------------------------------

CxxScoped* ClassInst::FindInstanceAnalog(const CxxNamed* item) const
{
   Debug::ft("ClassInst.FindInstanceAnalog");

   if(!instantiated_) return nullptr;

   auto type = item->Type();

   switch(type)
   {
   case Cxx::Class:
      return const_cast<ClassInst*>(this);

   case Cxx::Function:
      size_t idx;
      auto func = static_cast<const Function*>(item);
      if(!tmplt_->FuncToIndex(func, idx)) return nullptr;
      auto list = FuncVector(item->Name());
      if(idx >= list->size()) return nullptr;
      return list->at(idx).get();
   }

   return FindMember(item->Name(), false);
}

//------------------------------------------------------------------------------

CxxScoped* ClassInst::FindTemplateAnalog(const CxxToken* item) const
{
   Debug::ft("ClassInst.FindTemplateAnalog");

   //  If ITEM is in a function, have that function find its analog.
   //  A friend can be in a function and its scope is not the class
   //  that granted friendship, so don't check this for a friend.
   //
   auto type = item->Type();

   if((item != this) && (type != Cxx::Friend))
   {
      auto scope = item->GetScope();

      if(scope != this)
      {
         auto func = scope->GetFunction();
         if(func == nullptr) return nullptr;
         return func->FindTemplateAnalog(item);
      }
   }

   switch(type)
   {
   case Cxx::Class:
      if(item == this) return tmplt_;
      break;

   case Cxx::Function:
   {
      size_t idx;
      auto func = static_cast<const Function*>(item);
      if(!FuncToIndex(func, idx)) return nullptr;
      return tmplt_->IndexToFunc(item->Name(), idx);
   }

   case Cxx::Friend:
   {
      auto ref = item->Referent();
      if(ref == nullptr) return nullptr;
      return tmplt_->FindFriend(static_cast<const CxxScope*>(ref));
   }
   }

   return tmplt_->FindMember(item->Name(), false);
}

//------------------------------------------------------------------------------

void ClassInst::GetUsages(const CodeFile& file, CxxUsageSets& symbols)
{
   //  This is invoked by a class template in order to obtain symbol usage
   //  information from one of its instances.
   //
   CxxUsageSets sets;
   Class::GetUsages(file, sets);
   sets.EraseTemplateArgs(tspec_.get());
   symbols.Union(sets);
}

//------------------------------------------------------------------------------

void ClassInst::Instantiate()
{
   Debug::ft("ClassInst.Instantiate");

   //  Return if the template has already been instantiated.  Otherwise,
   //  notify tspec_, which contains the template name and arguments, that
   //  its template is being instantiated.  This causes the instantiation
   //  of any templates that this one requires.
   //
   if(instantiated_) return;
   instantiated_ = true;

   CxxScopedVector locals;
   tspec_->Instantiating(locals);

   //  Get the code for the template instance and parse it.
   //
   code_.reset();
   auto begin = tmplt_->CreateCode(this, code_);
   ParserPtr parser(new Parser());

   if(!locals.empty())
   {
      for(auto item = locals.cbegin(); item != locals.cend(); ++item)
      {
         Context::InsertLocal(*item);
      }
   }

   compiled_ = parser->ParseClassInst(this, begin);
   parser.reset();
   if(compiled_) code_.reset();
}

//------------------------------------------------------------------------------

bool ClassInst::NameRefersToItem(const string& name,
   const CxxScope* scope, CodeFile* file, SymbolView& view) const
{
   Debug::ft("ClassInst.NameRefersToItem");

   //  Split NAME into its component (template name and arguments).  If it
   //  refers to this class instance's template, see if also refers to its
   //  template arguments.  Scoped names are compared in case NAME can only
   //  see this class as a forward declaration.
   //
   auto names = GetNameAndArgs(name);
   auto syms = Singleton<CxxSymbols>::Instance();
   auto item = syms->FindSymbol
      (file, scope, names.front().name, FRIEND_CLASSES, view);
   if(item == nullptr) return false;

   auto iname = item->ScopedName(false);
   auto tname = tmplt_->ScopedName(false);

   if(iname == tname)
   {
      size_t index = 1;
      if(Context::ParsingTemplateInstance())
         scope = Context::OuterFrame()->Scope();
      else
         scope = Context::Scope();
      if(!tspec_->NamesReferToArgs(names, scope, file, index)) return false;
      return (index == names.size());
   }

   return false;
}

//------------------------------------------------------------------------------

string ClassInst::TypeString(bool arg) const
{
   return Prefix(GetScope()->TypeString(arg)) + Name();
}

//==============================================================================

CxxArea::CxxArea() : newest_(nullptr)
{
   Debug::ft("CxxArea.ctor");
}

//------------------------------------------------------------------------------

CxxArea::~CxxArea()
{
   Debug::ftnt("CxxArea.dtor");
}

//------------------------------------------------------------------------------

bool CxxArea::AddAsm(AsmPtr& code)
{
   Debug::ft("CxxArea.AddAsm");

   if(!code->IsInternal()) newest_ = code.get();

   if(code->EnterScope())
   {
      assembly_.push_back(std::move(code));
   }

   return true;
}

//------------------------------------------------------------------------------

bool CxxArea::AddClass(ClassPtr& cls)
{
   Debug::ft("CxxArea.AddClass");

   if(!cls->IsInternal()) newest_ = cls.get();

   if(AddAnonymousUnion(cls)) return true;

   if(cls->EnterScope())
   {
      classes_.push_back(std::move(cls));
   }

   return true;
}

//------------------------------------------------------------------------------

bool CxxArea::AddData(DataPtr& data)
{
   Debug::ft("CxxArea.AddData");

   if(!data->IsInternal()) newest_ = data.get();

   if(data->EnterScope())
   {
      data_.push_back(std::move(data));
   }
   else
   {
      defns_.push_back(std::move(data));
   }

   return true;
}

//------------------------------------------------------------------------------

bool CxxArea::AddEnum(EnumPtr& decl)
{
   Debug::ft("CxxArea.AddEnum");

   if(!decl->IsInternal()) newest_ = decl.get();

   if(decl->EnterScope())
   {
      enums_.push_back(std::move(decl));
   }

   return true;
}

//------------------------------------------------------------------------------

bool CxxArea::AddForw(ForwardPtr& forw)
{
   Debug::ft("CxxArea.AddForw");

   if(!forw->IsInternal()) newest_ = forw.get();

   if(forw->EnterScope())
   {
      forws_.push_back(std::move(forw));
   }

   return true;
}

//------------------------------------------------------------------------------

bool CxxArea::AddFunc(FunctionPtr& func)
{
   Debug::ft("CxxArea.AddFunc");

   //  If this is an inline function, do not add it to a template instance.
   //  Simply returning results in FUNC being deleted.
   //
   if(func->IsInline())
   {
      auto cls = GetClass();
      if((cls != nullptr) && cls->IsInTemplateInstance()) return true;
   }

   //  Release the function after adding it to this scope and executing its
   //  code (if supplied).  EnterScope invokes InsertFunc, which assigns us
   //  ownership of the function, so just release our FUNC argument.
   //
   if(!func->IsInternal()) newest_ = func.get();
   func->EnterScope();
   func.release();
   return true;
}

//------------------------------------------------------------------------------

bool CxxArea::AddStaticAssert(StaticAssertPtr& assert)
{
   Debug::ft("CxxArea.AddStaticAssert");

   if(!assert->IsInternal()) newest_ = assert.get();

   if(assert->EnterScope())
   {
      asserts_.push_back(std::move(assert));
   }

   return true;
}

//------------------------------------------------------------------------------

bool CxxArea::AddType(TypedefPtr& type)
{
   Debug::ft("CxxArea.AddType");

   if(!type->IsInternal()) newest_ = type.get();

   if(type->EnterScope())
   {
      types_.push_back(std::move(type));
   }

   return true;
}

//------------------------------------------------------------------------------

bool CxxArea::AddUsing(UsingPtr& use)
{
   Debug::ft("CxxArea.AddUsing");

   if(!use->IsInternal()) newest_ = use.get();

   if(use->EnterScope())
   {
      usings_.push_back(std::move(use));
   }

   return true;
}

//------------------------------------------------------------------------------

void CxxArea::Check() const
{
   Debug::ft("CxxArea.Check");

   for(auto c = classes_.cbegin(); c != classes_.cend(); ++c)
   {
      (*c)->Check();
   }

   for(auto d = data_.cbegin(); d != data_.cend(); ++d)
   {
      (*d)->Check();
   }

   for(auto e = enums_.cbegin(); e != enums_.cend(); ++e)
   {
      (*e)->Check();
   }

   for(auto f = forws_.cbegin(); f != forws_.cend(); ++f)
   {
      (*f)->Check();
   }

   for(auto f = funcs_.cbegin(); f != funcs_.cend(); ++f)
   {
      (*f)->Check();
   }

   for(auto o = opers_.cbegin(); o != opers_.cend(); ++o)
   {
      (*o)->Check();
   }

   for(auto t = types_.cbegin(); t != types_.cend(); ++t)
   {
      (*t)->Check();
   }

   for(auto a = asserts_.cbegin(); a != asserts_.cend(); ++a)
   {
      (*a)->Check();
   }
}

//------------------------------------------------------------------------------

void CxxArea::EraseClass(const Class* cls)
{
   Debug::ft("CxxArea.EraseClass");

   EraseItemPtr(classes_, cls);
}

//------------------------------------------------------------------------------

void CxxArea::EraseData(const Data* data)
{
   Debug::ft("CxxArea.EraseData");

   if(data->IsDecl())
   {
      EraseItemPtr(data_, data);
   }
   else
   {
      EraseItemPtr<CxxScope>(defns_, data);
   }
}

//------------------------------------------------------------------------------

void CxxArea::EraseEnum(const Enum* decl)
{
   Debug::ft("CxxArea.EraseEnum");

   EraseItemPtr(enums_, decl);
}

//------------------------------------------------------------------------------

void CxxArea::EraseForw(const Forward* forw)
{
   Debug::ft("CxxArea.EraseForw");

   EraseItemPtr(forws_, forw);
}

//------------------------------------------------------------------------------

void CxxArea::EraseFunc(const Function* func)
{
   Debug::ft("CxxArea.EraseFunc");

   if(func->IsDecl())
   {
      if(func->FuncType() == FuncOperator)
         EraseItemPtr(opers_, func);
      else
         EraseItemPtr(funcs_, func);
   }
   else
   {
      EraseItemPtr<CxxScope>(defns_, func);
   }
}

//------------------------------------------------------------------------------

void CxxArea::EraseType(const Typedef* type)
{
   Debug::ft("CxxArea.EraseType");

   EraseItemPtr(types_, type);
}

//------------------------------------------------------------------------------

void CxxArea::EraseUsing(const Using* use)
{
   Debug::ft("CxxArea.EraseUsing");

   EraseItemPtr(usings_, use);
}

//------------------------------------------------------------------------------

Class* CxxArea::FindClass(const string& name) const
{
   Debug::ft("CxxArea.FindClass");

   for(auto c = classes_.cbegin(); c != classes_.cend(); ++c)
   {
      if((*c)->Name() == name) return c->get();
   }

   return nullptr;
}

//------------------------------------------------------------------------------

Data* CxxArea::FindData(const string& name) const
{
   Debug::ft("CxxArea.FindData");

   for(auto d = data_.cbegin(); d != data_.cend(); ++d)
   {
      if((*d)->Name() == name) return d->get();
   }

   return nullptr;
}

//------------------------------------------------------------------------------

Enum* CxxArea::FindEnum(const string& name) const
{
   Debug::ft("CxxArea.FindEnum");

   for(auto e = enums_.cbegin(); e != enums_.cend(); ++e)
   {
      if((*e)->Name() == name) return e->get();
   }

   return nullptr;
}

//------------------------------------------------------------------------------

Enumerator* CxxArea::FindEnumerator(const string& name) const
{
   Debug::ft("CxxArea.FindEnumerator");

   for(auto e = enums_.cbegin(); e != enums_.cend(); ++e)
   {
      auto m = (*e)->FindEnumerator(name);
      if(m != nullptr) return m;
   }

   return nullptr;
}

//------------------------------------------------------------------------------

Function* CxxArea::FindFunc(const string& name,
   const StackArg* call, StackArgVector* args, bool base,
   const CxxScope* scope, SymbolView* view) const
{
   Debug::ft("CxxArea.FindFunc");

   //  Get the type string for each argument in ARGS.
   //
   FunctionVector funcs;
   std::vector<std::vector<TypeMatch>> argMatches;

   stringVector argTypes;

   if(args != nullptr)
   {
      for(size_t i = 0; i < args->size(); ++i)
      {
         auto& arg = args->at(i);
         argTypes.push_back(arg.TypeString(true));
      }
   }

   //  Visit our functions, asking each whose name matches NAME if it can
   //  be invoked with ARGS.  Assemble a list of the functions that can be
   //  invoked with ARGS, but return a perfect match immediately.  Because
   //  function templates appear in LIST, LIST can expand when a candidate
   //  function is instantiated.  Hence the index rather than an iterator.
   //
   auto list = FuncVector(name);

   for(size_t i = 0; i < list->size(); ++i)
   {
      auto func = list->at(i).get();
      auto& temp = func->Name();

      if(temp == name)
      {
         if(args == nullptr)
         {
            funcs.push_back(func);
         }
         else
         {
            std::vector<TypeMatch> matches;
            func = func->CanInvokeWith(*args, argTypes, matches);

            if(func != nullptr)
            {
               funcs.push_back(func);
               argMatches.push_back(matches);
            }
         }
      }
   }

   auto count = funcs.size();

   if(count == 0)
   {
      return FoundFunc(nullptr, view, Incompatible);
   }
   else if(args == nullptr)
   {
      //  When we don't yet have arguments and are just matching on the
      //  function name, prefer a non-static function over a static one.
      //  This will cause Operation::ExecuteCall to supply a prospective
      //  "this" argument: it will be ignored when matching with a static
      //  function, but there is no provision for the opposite behavior.
      //
      for(size_t i = 0; i < count; ++i)
      {
         if(!funcs[i]->IsStatic())
         {
            return FoundFunc(funcs[i], view, Compatible);
         }
      }

      return FoundFunc(funcs.front(), view, Compatible);
   }
   else if(count == 1)
   {
      return FoundFunc(funcs.front(), view, FindMin(argMatches.front()));
   }

   //  Return the function that is the best match, as defined by the sum of
   //  how well each argument matches (its distance from being Compatible).
   //
   auto bestMatch = 1000;
   size_t bestIndex = 0;

   for(size_t i = 0; i < count; ++i)
   {
      auto total = 0;
      auto& matches = argMatches[i];

      for(auto m = matches.cbegin(); m != matches.cend(); ++m)
      {
         total += (Compatible - *m);
      }

      if(total < bestMatch)
      {
         bestIndex = i;
         bestMatch = total;
      }
   }

   return FoundFunc(funcs[bestIndex], view, FindMin(argMatches[bestIndex]));
}

//------------------------------------------------------------------------------

CxxScoped* CxxArea::FindItem(const string& name) const
{
   Debug::ft("CxxArea.FindItem");

   auto op = CxxOp::NameToOperator(name);

   if(op != Cxx::NIL_OPERATOR)
   {
      return FindFunc(name, nullptr, nullptr, false, nullptr, nullptr);
   }

   CxxScoped* item = FindData(name);
   if(item != nullptr) return item;

   item = FindFunc(name, nullptr, nullptr, false, nullptr, nullptr);
   if(item != nullptr) return item;

   item = FindClass(name);
   if(item != nullptr) return item;

   item = FindType(name);
   if(item != nullptr) return item;

   item = FindEnum(name);
   if(item != nullptr) return item;

   return FindEnumerator(name);
}

//------------------------------------------------------------------------------

Typedef* CxxArea::FindType(const string& name) const
{
   Debug::ft("CxxArea.FindType");

   for(auto t = types_.cbegin(); t != types_.cend(); ++t)
   {
      if((*t)->Name() == name) return t->get();
   }

   return nullptr;
}

//------------------------------------------------------------------------------

const FunctionPtrVector* CxxArea::FuncVector(const string& name) const
{
   if(name.rfind(OPERATOR_STR, 0) == 0)
   {
      //  For this to be an actual operator, the next character must be illegal
      //  in an identifier (internally, the name of each operation function has
      //  the operator punctuation appended to "operator").
      //
      if(!CxxChar::Attrs[name[strlen(OPERATOR_STR)]].validNext) return &opers_;
   }

   return &funcs_;
}

//------------------------------------------------------------------------------

void CxxArea::GetDecls(CxxNamedSet& items)
{
   for(auto c = classes_.cbegin(); c != classes_.cend(); ++c)
   {
      (*c)->GetDecls(items);
   }

   for(auto d = data_.cbegin(); d != data_.cend(); ++d)
   {
      (*d)->GetDecls(items);
   }

   for(auto e = enums_.cbegin(); e != enums_.cend(); ++e)
   {
      (*e)->GetDecls(items);
   }

   for(auto f = forws_.cbegin(); f != forws_.cend(); ++f)
   {
      (*f)->GetDecls(items);
   }

   for(auto f = funcs_.cbegin(); f != funcs_.cend(); ++f)
   {
      (*f)->GetDecls(items);
   }

   for(auto o = opers_.cbegin(); o != opers_.cend(); ++o)
   {
      (*o)->GetDecls(items);
   }

   for(auto t = types_.cbegin(); t != types_.cend(); ++t)
   {
      (*t)->GetDecls(items);
   }
}

//------------------------------------------------------------------------------

void CxxArea::InsertFunc(Function* func)
{
   if(func->IsDecl())
   {
      if(func->FuncType() == FuncOperator)
         opers_.push_back(FunctionPtr(func));
      else
         funcs_.push_back(FunctionPtr(func));
   }
   else
   {
      defns_.push_back(FunctionPtr(func));
   }
}

//------------------------------------------------------------------------------

CxxTokenVector CxxArea::Items() const
{
   Debug::ft("CxxArea.Items");

   CxxTokenVector items;

   for(auto a = assembly_.cbegin(); a != assembly_.cend(); ++a)
   {
      items.push_back(a->get());
   }

   for(auto a = asserts_.cbegin(); a != asserts_.cend(); ++a)
   {
      items.push_back(a->get());
   }

   for(auto c = classes_.cbegin(); c != classes_.cend(); ++c)
   {
      items.push_back(c->get());
   }

   for(auto d = data_.cbegin(); d != data_.cend(); ++d)
   {
      items.push_back(d->get());
   }

   for(auto e = enums_.cbegin(); e != enums_.cend(); ++e)
   {
      items.push_back(e->get());
   }

   for(auto f = forws_.cbegin(); f != forws_.cend(); ++f)
   {
      items.push_back(f->get());
   }

   for(auto f = funcs_.cbegin(); f != funcs_.cend(); ++f)
   {
      //  Screen out function template instances, which appear in
      //  a class that includes a function template.
      //
      if(!(*f)->IsInternal()) items.push_back(f->get());
   }

   for(auto o = opers_.cbegin(); o != opers_.cend(); ++o)
   {
      items.push_back(o->get());
   }

   for(auto t = types_.cbegin(); t != types_.cend(); ++t)
   {
      items.push_back(t->get());
   }

   for(auto u = usings_.cbegin(); u != usings_.cend(); ++u)
   {
      items.push_back(u->get());
   }

   return items;
}

//------------------------------------------------------------------------------

Function* CxxArea::MatchFunc(const Function* curr, bool base) const
{
   Debug::ft("CxxArea.MatchFunc");

   auto list = FuncVector(curr->Name());

   for(auto f = list->cbegin(); f != list->cend(); ++f)
   {
      if((*f)->Name() == curr->Name())
      {
         if((*f)->SignatureMatches(curr, base)) return f->get();
      }
   }

   return nullptr;
}

//------------------------------------------------------------------------------

CxxToken* CxxArea::NewestItem()
{
   Debug::ft("CxxArea.NewestItem");

   auto item = newest_;
   newest_ = nullptr;
   return item;
}

//------------------------------------------------------------------------------

CxxToken* CxxArea::PosToItem(size_t pos) const
{
   //  This does not forward to decls_, whose items reside at file scope
   //  in a .cpp and are therefore found by CodeFile.PosToItem.
   //
   auto item = CxxScope::PosToItem(pos);
   if(item != nullptr) return item;

   for(auto u = usings_.cbegin(); u != usings_.cend(); ++u)
   {
      item = (*u)->PosToItem(pos);
      if(item != nullptr) return item;
   }

   for(auto c = classes_.cbegin(); c != classes_.cend(); ++c)
   {
      item = (*c)->PosToItem(pos);
      if(item != nullptr) return item;
   }

   for(auto d = data_.cbegin(); d != data_.cend(); ++d)
   {
      item = (*d)->PosToItem(pos);
      if(item != nullptr) return item;
   }

   for(auto e = enums_.cbegin(); e != enums_.cend(); ++e)
   {
      item = (*e)->PosToItem(pos);
      if(item != nullptr) return item;
   }

   for(auto f = forws_.cbegin(); f != forws_.cend(); ++f)
   {
      item = (*f)->PosToItem(pos);
      if(item != nullptr) return item;
   }

   for(auto f = funcs_.cbegin(); f != funcs_.cend(); ++f)
   {
      item = (*f)->PosToItem(pos);
      if(item != nullptr) return item;
   }

   for(auto o = opers_.cbegin(); o != opers_.cend(); ++o)
   {
      item = (*o)->PosToItem(pos);
      if(item != nullptr) return item;
   }

   for(auto t = types_.cbegin(); t != types_.cend(); ++t)
   {
      item = (*t)->PosToItem(pos);
      if(item != nullptr) return item;
   }

   for(auto a = assembly_.cbegin(); a != assembly_.cend(); ++a)
   {
      item = (*a)->PosToItem(pos);
      if(item != nullptr) return item;
   }

   for(auto a = asserts_.cbegin(); a != asserts_.cend(); ++a)
   {
      item = (*a)->PosToItem(pos);
      if(item != nullptr) return item;
   }

   return nullptr;
}

//------------------------------------------------------------------------------

void CxxArea::UpdatePos
   (EditorAction action, size_t begin, size_t count, size_t from) const
{
   //  This does not forward to decls_, whose items reside at file scope
   //  in a .cpp and are therefore updated by CodeFile.UpdatePos.
   //
   CxxScope::UpdatePos(action, begin, count, from);

   for(auto u = usings_.cbegin(); u != usings_.cend(); ++u)
   {
      (*u)->UpdatePos(action, begin, count, from);
   }

   for(auto c = classes_.cbegin(); c != classes_.cend(); ++c)
   {
      (*c)->UpdatePos(action, begin, count, from);
   }

   for(auto d = data_.cbegin(); d != data_.cend(); ++d)
   {
      (*d)->UpdatePos(action, begin, count, from);
   }

   for(auto e = enums_.cbegin(); e != enums_.cend(); ++e)
   {
      (*e)->UpdatePos(action, begin, count, from);
   }

   for(auto f = forws_.cbegin(); f != forws_.cend(); ++f)
   {
      (*f)->UpdatePos(action, begin, count, from);
   }

   for(auto f = funcs_.cbegin(); f != funcs_.cend(); ++f)
   {
      (*f)->UpdatePos(action, begin, count, from);
   }

   for(auto o = opers_.cbegin(); o != opers_.cend(); ++o)
   {
      (*o)->UpdatePos(action, begin, count, from);
   }

   for(auto t = types_.cbegin(); t != types_.cend(); ++t)
   {
      (*t)->UpdatePos(action, begin, count, from);
   }

   for(auto a = assembly_.cbegin(); a != assembly_.cend(); ++a)
   {
      (*a)->UpdatePos(action, begin, count, from);
   }

   for(auto a = asserts_.cbegin(); a != asserts_.cend(); ++a)
   {
      (*a)->UpdatePos(action, begin, count, from);
   }
}

//------------------------------------------------------------------------------

void CxxArea::UpdateXref(bool insert)
{
   for(auto c = classes_.cbegin(); c != classes_.cend(); ++c)
   {
      (*c)->UpdateXref(insert);
   }

   for(auto t = types_.cbegin(); t != types_.cend(); ++t)
   {
      (*t)->UpdateXref(insert);
   }

   auto inst = IsInTemplateInstance();

   for(auto f = funcs_.cbegin(); f != funcs_.cend(); ++f)
   {
      //  If this is not a class template instance, bypass function
      //  template instantiations, every one of which is registered
      //  against the function template.
      //
      if(!inst && (*f)->IsInternal()) continue;

      (*f)->UpdateXref(insert);
   }

   for(auto o = opers_.cbegin(); o != opers_.cend(); ++o)
   {
      (*o)->UpdateXref(insert);
   }

   for(auto d = data_.cbegin(); d != data_.cend(); ++d)
   {
      (*d)->UpdateXref(insert);
   }

   for(auto a = asserts_.cbegin(); a != asserts_.cend(); ++a)
   {
      (*a)->UpdateXref(insert);
   }
}

//==============================================================================

Namespace::Namespace(const string& name, Namespace* space) :
   name_(name)
{
   Debug::ft("Namespace.ctor");

   CxxArea::SetScope(space);
   Singleton<CxxSymbols>::Instance()->InsertSpace(this);
}

//------------------------------------------------------------------------------

Namespace::~Namespace()
{
   Debug::ftnt("Namespace.dtor");

   Singleton<CxxSymbols>::Extant()->EraseSpace(this);
}

//------------------------------------------------------------------------------

void Namespace::AccessibilityOf
   (const CxxScope* scope, const CxxScoped* item, SymbolView& view) const
{
   Debug::ft("Namespace.AccessibilityOf");

   view.accessibility_ = (item->GetFile()->IsCpp() ? Restricted : Unrestricted);
   view.distance_ = scope->ScopeDistance(this);

   //  If ITEM is extern, increase its distance from SCOPE so that a static
   //  item will be preferred over an extern item with the same name.
   //
   if((view.distance_ != NOT_A_SUBSCOPE) && item->IsExtern())
   {
      ++view.distance_;
   }
}

//------------------------------------------------------------------------------

void Namespace::Check() const
{
   Debug::ft("Namespace.Check");

   auto name = ScopedName(false);
   if(name.empty()) name = SCOPE_STR;
   name.insert(0, "namespace ");
   name.push_back(CRLF);
   Debug::Progress(name);

   CxxArea::Check();

   for(auto s = spaces_.cbegin(); s != spaces_.cend(); ++s)
   {
      (*s)->Check();
   }
}

//------------------------------------------------------------------------------

void Namespace::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   auto name = Name();

   if(name.empty()) name = SCOPE_STR;

   if(!options.test(DispCode))
   {
      stream << prefix << string(132 - prefix.size(), '-') << CRLF;
   }

   stream << prefix << NAMESPACE_STR << SPACE << name << CRLF;
   stream << prefix << '{' << CRLF;

   auto lead = prefix + spaces(IndentSize());
   auto nonqual = options;
   nonqual.reset(DispFQ);

   SortAndDisplayItemPtrs(*Asserts(), stream, lead, nonqual, SortByName);
   SortAndDisplayItemPtrs(*Enums(), stream, lead, nonqual, SortByName);
   SortAndDisplayItemPtrs(*Types(), stream, lead, nonqual, SortByName);
   SortAndDisplayItemPtrs(*Funcs(), stream, lead, nonqual, SortByName);
   SortAndDisplayItemPtrs(*Opers(), stream, lead, nonqual, SortByName);
   SortAndDisplayItemPtrs(*Assembly(), stream, lead, nonqual, SortByName);
   SortAndDisplayItemPtrs(*Datas(), stream, lead, nonqual, SortByName);
   SortAndDisplayItemPtrs(*Classes(), stream, lead, nonqual, SortByName);
   SortAndDisplayItemPtrs(spaces_, stream, lead, nonqual, SortByName);
   stream << prefix << '}' << CRLF;
}

//------------------------------------------------------------------------------

Namespace* Namespace::EnsureNamespace(const string& name)
{
   Debug::ft("Namespace.EnsureNamespace");

   //  If a namespace defined by NAME is not found, create it.
   //
   auto s = FindNamespace(name);
   if(s != nullptr) return s;

   NamespacePtr space(new Namespace(name, this));
   spaces_.push_back(std::move(space));
   return spaces_.back().get();
}

//------------------------------------------------------------------------------

void Namespace::EraseDefn(const SpaceDefn* defn)
{
   Debug::ft("Namespace.EraseDefn");

   EraseItemPtr(defns_, defn);
}

//------------------------------------------------------------------------------

Function* Namespace::FindFunc(const string& name,
   const StackArg* call, StackArgVector* args, bool base,
   const CxxScope* scope, SymbolView* view) const
{
   Debug::ft("Namespace.FindFunc");

   auto f = CxxArea::FindFunc(name, call, args, false, scope, view);
   if(f != nullptr) return f;
   if(!base) return nullptr;

   for(auto s = OuterSpace(); s != nullptr; s = s->OuterSpace())
   {
      f = s->FindFunc(name, call, args, false, scope, view);
      if(f != nullptr) return f;
   }

   return nullptr;
}

//------------------------------------------------------------------------------

CxxScoped* Namespace::FindItem(const string& name) const
{
   Debug::ft("Namespace.FindItem");

   CxxScoped* item = FindNamespace(name);
   if(item != nullptr) return item;

   return CxxArea::FindItem(name);
}

//------------------------------------------------------------------------------

Namespace* Namespace::FindNamespace(const string& name) const
{
   Debug::ft("Namespace.FindNamespace");

   //  Return the namespace, if any, defined by NAME.
   //
   for(auto s = spaces_.cbegin(); s != spaces_.cend(); ++s)
   {
      if((*s)->Name() == name) return s->get();
   }

   return nullptr;
}

//------------------------------------------------------------------------------

void Namespace::InsertDefn(CodeFile* file, size_t pos)
{
   Debug::ft("Namespace.InsertDefn");

   SpaceDefnPtr space(new SpaceDefn(this));
   space->SetLoc(file, pos);
   space->SetScope(this);
   file->InsertSpace(space.get());
   defns_.push_back(std::move(space));
}

//------------------------------------------------------------------------------

void Namespace::Rename(const string& name)
{
   Debug::ft("Namespace.Rename");

   CxxScoped::RenameNonQual(name_, name);

   //  Calls to Debug::ft must be updated with the new namespace name.
   //
   auto funcs = Funcs();

   for(auto f = funcs->cbegin(); f != funcs->cend(); ++f)
   {
      Editor::UpdateDebugFt(f->get());
   }

   auto opers = Opers();

   for(auto o = opers->cbegin(); o != opers->cend(); ++o)
   {
      Editor::UpdateDebugFt(o->get());
   }
}

//------------------------------------------------------------------------------

string Namespace::ScopedName(bool templates) const
{
   auto scope = GetScope();

   if(scope == nullptr)
   {
      //  This is the global namespace.
      //
      return EMPTY_STR;
   }

   if(scope == Singleton<CxxRoot>::Instance()->GlobalNamespace())
   {
      //  This namespace is directly below the global namespace.
      //
      return Name();
   }

   return scope->ScopedName(templates) + SCOPE_STR + Name();
}

//------------------------------------------------------------------------------

void Namespace::SetLoc(CodeFile* file, size_t pos) const
{
   Debug::ft("Namespace.SetLoc");

   //  If this is the first definition of the namespace, set its location.
   //
   if(GetFile() == nullptr) CxxArea::SetLoc(file, pos);
}

//------------------------------------------------------------------------------

string Namespace::TypeString(bool arg) const
{
   auto scope = GetScope();
   if(scope == nullptr) return EMPTY_STR;
   return Prefix(scope->TypeString(arg)) + Name();
}
}
