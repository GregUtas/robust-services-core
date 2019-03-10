//==============================================================================
//
//  CxxArea.cpp
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
#include "CxxArea.h"
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <sstream>
#include <utility>
#include "CodeFile.h"
#include "CxxDirective.h"
#include "CxxExecute.h"
#include "CxxRoot.h"
#include "CxxString.h"
#include "CxxSymbols.h"
#include "CxxToken.h"
#include "Debug.h"
#include "Formatters.h"
#include "Lexer.h"
#include "Parser.h"
#include "Singleton.h"

using namespace NodeBase;
using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace CodeTools
{
fn_name Class_ctor = "Class.ctor[>ct]";

Class::Class(QualNamePtr& name, Cxx::ClassTag tag) :
   name_(name.release()),
   tag_(tag),
   currAccess_(Cxx::Access_N),
   copied_(false)
{
   Debug::ft(Class_ctor);

   Singleton< CxxSymbols >::Instance()->InsertClass(this);
   CxxStats::Incr(CxxStats::CLASS_DECL);
}

//------------------------------------------------------------------------------

fn_name Class_dtor = "Class.dtor[>ct]";

Class::~Class()
{
   Debug::ft(Class_dtor);

   Singleton< CxxSymbols >::Instance()->EraseClass(this);
   CxxStats::Decr(CxxStats::CLASS_DECL);
}

//------------------------------------------------------------------------------

fn_name Class_AccessibilityOf = "Class.AccessibilityOf";

void Class::AccessibilityOf
   (const CxxScope* scope, const CxxScoped* item, SymbolView* view) const
{
   Debug::ft(Class_AccessibilityOf);

   //  Start by assuming the worst.
   //
   view->accessibility = Inaccessible;

   //  Create a list of ITEM's class scopes.  There will be more than one
   //  only if ITEM is an inner class.  Simultaneously, determine ITEM's
   //  access control at each class scope.
   //    If ITEM is a forward declaration of an inner class, the distance
   //  to it is one less than the distance to the definition of the inner
   //  class.  This occurs because the inner class is added to itemClasses,
   //  whereas the forward declaration is not.  Correct for this so that
   //  CxxSymbols.FindNearestItem will resolve the class's name to its
   //  definition instead of to its forward declaration.
   //
   std::vector< Class* > itemClasses;
   std::vector< Cxx::Access > controls;

   auto itemClass = item->GetClass();
   size_t n = 0;
   auto type = item->Type();
   auto f = (type == Cxx::Forward ? 1 : 0);

   for(auto c = itemClass; c != nullptr; c = c->OuterClass(), ++n)
   {
      itemClasses.push_back(c);

      //  On the first pass, make ITEM public to the first level (its outer
      //  class) and apply its access control to the next level.  As we
      //  ascend through outer classes, incorporate their access controls.
      //
      if(n == 0)
      {
         controls.push_back(Cxx::Public);
         controls.push_back(item->GetAccess());
         n = 1;
      }
      else
      {
         controls.push_back(std::min(controls[n - 1], c->GetAccess()));
      }
   }

   if(itemClasses.empty())
   {
      auto expl = "Item is not a class member: " + item->ScopedName(true);
      Context::SwLog(Class_AccessibilityOf, expl, 0);
      return;
   }

   //  Find SCOPE's class.  If it doesn't have one, it must be a friend
   //  of ITEM's class unless ITEM is public at file scope.
   //
   auto usingClass = scope->GetClass();

   if(usingClass != nullptr)
   {
      //  See if SCOPE has a class scope in common with ITEM.  If so, N
      //  will be the index to that class in itemClasses, and M will be
      //  its distance above SCOPE.
      //
      size_t m = 0;
      n = SIZE_MAX;

      for(auto c = usingClass; c != nullptr; c = c->OuterClass(), ++m)
      {
         n = IndexOf(itemClasses, c);
         if(n != SIZE_MAX) break;
      }

      if(n == 0)
      {
         //  SCOPE is either in ITEM's class or one of its inner classes.
         //  It can therefore access ITEM.
         //
         view->distance = m + f;
         view->accessibility = Declared;
         return;
      }
      else if(n != SIZE_MAX)
      {
         //  SCOPE is either in ITEM's outer class or another inner class
         //  that shares an outer class with ITEM.  In either case, ITEM is
         //  only accessible if it is public to the outer class.  Note that
         //  an inner class, or its forward declaration, is always visible
         //  to the outer class.
         //
         if(controls[n - 1] == Cxx::Public)
         {
            view->distance = m + n + f;
            view->accessibility = Declared;
            if((item == itemClass) || (item->Referent() == itemClass)) return;
            item->RecordAccess(Cxx::Public);
            return;
         }
      }
      else
      {
         //  If SCOPE and ITEM don't share a common outer class, see if SCOPE's
         //  class derives from ITEM's class.  If so, SCOPE can see a protected
         //  ITEM, but not a private ITEM unless it is a friend.
         //
         view->distance = usingClass->ClassDistance(this);

         if(view->distance != NOT_A_SUBCLASS)
         {
            auto frnd = itemClass->FindFriend(scope);

            if(frnd != nullptr)
            {
               view->accessibility = Inherited;
               view->friend_ = true;
               if(controls[1] == Cxx::Private) frnd->IncrUsers();
            }
            else if(controls[1] != Cxx::Private)
            {
               view->accessibility = Inherited;
               item->RecordAccess(Cxx::Protected);
            }

            return;
         }
      }

      //  If the using class is a template instance of ITEM's class, it
      //  can use an inline function in the class template.
      //
      if((type == Cxx::Function) && (usingClass->GetTemplate() == this) &&
         static_cast< const Function* >(item)->IsInline())
      {
         view->distance = 1;
         view->accessibility = Declared;
         return;
      }

      //  If the using class is a template instance, it can use a template
      //  argument, even if the argument is private.
      //
      if(usingClass->IsInTemplateInstance())
      {
         auto spec = usingClass->GetTemplateArgs();

         if(spec->ItemIsTemplateArg(item))
         {
            view->accessibility = Unrestricted;
            return;
         }
      }
   }

   view->distance = scope->ScopeDistance(itemClasses.back()->GetSpace());

   //  ITEM must be public at file scope unless SCOPE is a friend.
   //
   auto frnd = itemClass->FindFriend(scope);

   if((frnd == nullptr) && (type == Cxx::Class))
   {
      auto decl = item->Declarer();
      if(decl != nullptr) frnd = decl->FindFriend(scope);
   }

   if(frnd != nullptr)
   {
      view->accessibility = Unrestricted;
      view->friend_ = true;
      if(controls.back() != Cxx::Public) frnd->IncrUsers();
      return;
   }

   if(controls.back() == Cxx::Public)
   {
      view->accessibility = item->FileScopeAccessiblity();
      item->RecordAccess(Cxx::Public);
      return;
   }
}

//------------------------------------------------------------------------------

fn_name Class_AccessibilityTo = "Class.AccessibilityTo";

void Class::AccessibilityTo(const CxxScope* scope, SymbolView* view) const
{
   Debug::ft(Class_AccessibilityTo);

   AccessibilityOf(scope, this, view);
}

//------------------------------------------------------------------------------

fn_name Class_AddAnonymousUnion = "Class.AddAnonymousUnion";

bool Class::AddAnonymousUnion(const ClassPtr& cls)
{
   Debug::ft(Class_AddAnonymousUnion);

   //  There is nothing to do unless CLS is an anonymous union.
   //
   if(cls->GetClassTag() != Cxx::UnionType) return false;
   if(!cls->Name()->empty()) return false;
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
      AddItem(mem.get());
      dstData->push_back(std::move(mem));
   }

   return true;
}

//------------------------------------------------------------------------------

fn_name Class_AddBase = "Class.AddBase";

void Class::AddBase(BaseDeclPtr& base)
{
   Debug::ft(Class_AddBase);

   //  This is always invoked, so verify that a base class actually exists.
   //  If it is found from this scope, make it our base class.
   //
   if(base == nullptr) return;
   if(!base->EnterScope()) return;
   base_ = std::move(base);
}

//------------------------------------------------------------------------------

fn_name Class_AddDirective = "Class.AddDirective";

bool Class::AddDirective(DirectivePtr& dir)
{
   Debug::ft(Class_AddDirective);

   AddItem(dir.get());
   dirs_.push_back(std::move(dir));
   return true;
}

//------------------------------------------------------------------------------

fn_name Class_AddFiles = "Class.AddFiles";

void Class::AddFiles(SetOfIds& imSet) const
{
   Debug::ft(Class_AddFiles);

   auto classes = Classes();

   for(auto c = classes->cbegin(); c != classes->cend(); ++c)
   {
      (*c)->AddFiles(imSet);
   }

   auto funcs = Funcs();

   for(auto f = funcs->cbegin(); f != funcs->cend(); ++f)
   {
      (*f)->AddFiles(imSet);
   }

   auto data = Datas();

   for(auto d = data->cbegin(); d != data->cend(); ++d)
   {
      (*d)->AddFiles(imSet);
   }
}

//------------------------------------------------------------------------------

fn_name Class_AddFriend = "Class.AddFriend";

bool Class::AddFriend(FriendPtr& decl)
{
   Debug::ft(Class_AddFriend);

   if(decl->EnterScope()) friends_.push_back(std::move(decl));
   return true;
}

//------------------------------------------------------------------------------

void Class::AddItem(CxxNamed* item)
{
   items_.push_back(item);
}

//------------------------------------------------------------------------------

fn_name Class_AddSubclass = "Class.AddSubclass";

bool Class::AddSubclass(Class* cls)
{
   Debug::ft(Class_AddSubclass);

   subs_.push_back(cls);
   return true;
}

//------------------------------------------------------------------------------

fn_name Class_BlockCopied = "Class.BlockCopied";

void Class::BlockCopied(const StackArg* arg)
{
   Debug::ft(Class_BlockCopied);

   if(copied_) return;
   copied_ = true;

   if(GetFile()->IsSubsFile()) return;

   auto data = Datas();

   for(auto d = data->cbegin(); d != data->cend(); ++d)
   {
      auto mem = d->get();
      if(!mem->IsStatic()) mem->WasWritten(arg, false);
   }
}

//------------------------------------------------------------------------------

fn_name Class_CanConstructFrom = "Class.CanConstructFrom";

bool Class::CanConstructFrom(const StackArg& that, const string& thatType) const
{
   Debug::ft(Class_CanConstructFrom);

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

fn_name Class_Check = "Class.Check";

void Class::Check() const
{
   Debug::ft(Class_Check);

   CxxArea::Check();

   if(parms_ != nullptr) parms_->Check();

   //  If this is a class template, aggregate the friend usages from its
   //  instantiations.
   //
   for(auto f = friends_.cbegin(); f != friends_.cend(); ++f)
   {
      if(IsTemplate()) (*f)->AddUsers(tmplts_);
      (*f)->Check();
   }

   if(!subs_.empty() && (FindDtor() == nullptr))
   {
      Log(NonVirtualDestructor);
   }

   CheckIfUsed(ClassUnused);
   CheckOverrides();
   CheckRuleOfThree();
   CheckForVirtualOverload();
}

//------------------------------------------------------------------------------

fn_name Class_CheckForVirtualOverload = "Class.CheckForVirtualOverload";

void Class::CheckForVirtualOverload() const
{
   Debug::ft(Class_CheckForVirtualOverload);

   //  Check if this class declares two virtual functions with the same name.
   //  Only base class declarations of a function are logged for this.
   //
   auto funcs = Funcs();

   for(auto f1 = funcs->cbegin(); f1 != funcs->cend(); ++f1)
   {
      auto func1 = f1->get();
      if(!func1->IsVirtual()) continue;

      for(auto f2 = std::next(f1); f2 != funcs->cend(); ++f2)
      {
         auto func2 = f2->get();
         if(!func2->IsVirtual()) continue;

         if(*func1->Name() == *func2->Name())
         {
            if(!func1->IsOverride()) func1->Log(VirtualOverloading);
            if(!func2->IsOverride()) func2->Log(VirtualOverloading);
         }
      }
   }
}

//------------------------------------------------------------------------------

fn_name Class_CheckIfUsed = "Class.CheckIfUsed";

void Class::CheckIfUsed(Warning warning) const
{
   Debug::ft(Class_CheckIfUsed);

   auto attrs = GetUsageAttrs();

   //  If the class has a public inner class or public member functions or
   //  data, suggest making it a struct unless it is derived from a class
   //  or has private items, in which case it should be a class.
   //
   if((attrs.test(HasPublicInnerClass)) ||
      (attrs.test(HasPublicMemberFunction)) ||
      (attrs.test(HasPublicMemberData)))
   {
      auto base = BaseClass();
      if((base != nullptr) && (base->GetClassTag() == Cxx::ClassType))
      {
         if(tag_ == Cxx::StructType) Log(StructCouldBeClass);
         return;
      }

      if((attrs.test(IsBase)) ||
         (attrs.test(HasNonPublicInnerClass)) ||
         (attrs.test(HasNonPublicMemberFunction)) ||
         (attrs.test(HasNonPublicMemberData)) ||
         (attrs.test(HasNonPublicStaticFunction)) ||
         (attrs.test(HasNonPublicStaticData)))
      {
         if(tag_ == Cxx::StructType) Log(StructCouldBeClass);
      }
      else
      {
         if(tag_ == Cxx::ClassType) Log(ClassCouldBeStruct);
      }

      return;
   }

   //  If the class only has public static functions and data, or enums and
   //  typedefs, suggest making it a namespace unless it is derived, used as
   //  a base class, or instantiated.
   //
   if((attrs.test(HasPublicStaticFunction)) ||
      (attrs.test(HasPublicStaticData)) ||
      (attrs.test(HasEnum)) ||
      (attrs.test(HasTypedef)))
   {
      auto base = BaseClass();
      if((base != nullptr) && (base->GetClassTag() == Cxx::ClassType))
      {
         if(tag_ == Cxx::StructType) Log(StructCouldBeClass);
         return;
      }

      if((attrs.test(IsBase)) || (attrs.test(HasInstantiations)) ||
         (attrs.test(HasNonPublicInnerClass)) ||
         (attrs.test(HasNonPublicMemberFunction)) ||
         (attrs.test(HasNonPublicMemberData)) ||
         (attrs.test(HasNonPublicStaticFunction)) ||
         (attrs.test(HasNonPublicStaticData)))
      {
         if(tag_ == Cxx::StructType) Log(StructCouldBeClass);
         return;
      }

      Log(ClassCouldBeNamespace);
      return;
   }

   //  Before logging the class as unused, see if it is constructed.
   //
   if(attrs.test(IsConstructed)) return;

   //  Note that non-public items can only used by the class itself (although
   //  we treat protected members as private, which could cause inaccuracies).
   //  In any case, the class is only considered used if it is constructed or
   //  if it has public items that are used.
   //
   if(!attrs.test(IsConstructed)) Log(warning);
}

//------------------------------------------------------------------------------

fn_name Class_CheckOverrides = "Class.CheckOverrides";

void Class::CheckOverrides() const
{
   Debug::ft(Class_CheckOverrides);

   //  Check for overrides of Patch and Display.  The following are exempt:
   //  o Classes outside of NodeBase and SessionBase (Patch only).
   //  o Templates (Patch only).
   //  o Classes not derived from Base (Display) or Object (Patch).
   //c Allow the above to be customized through a configuration file.
   //
   auto space = GetSpace();
   if(space == nullptr) return;
   if(IsInTemplateInstance()) return;
   if(!DerivesFrom("Base")) return;

   //  Unless the class has no data, or only static const data, it
   //  should override Display.
   //
   auto display = false;

   auto data = Datas();

   for(auto d = data->cbegin(); d != data->cend(); ++d)
   {
      auto mem = d->get();

      if(mem->IsStatic() && mem->IsConst()) continue;
      display = true;
      break;
   }

   //  A class should override Patch unless it is a template, does not
   //  derive from Object, or is not in NodeBase or SessionBase.  Also
   //  exempt are trivial leaf classes.
   //
   auto patch = !IsTemplate();
   if(patch && !DerivesFrom("Object")) patch = false;
   if(patch)
   {
      auto& name = *space->Name();
      patch = (name == "NodeBase");
      patch = patch || (name == "NetworkBase");
      patch = patch || (name == "SessionBase");
   }

   if(subs_.empty())
   {
      if(display && DerivesFrom("Module")) display = false;
      if(display && DerivesFrom("Statistic")) display = false;
      if(display && DerivesFrom("StatisticsGroup")) display = false;
      if(display && DerivesFrom("Tool")) display = false;

      if(patch && DerivesFrom("CliParm")) patch = false;
      if(patch && DerivesFrom("CfgParm")) patch = false;
      if(patch && DerivesFrom("Statistic")) patch = false;
      if(patch && DerivesFrom("StatisticsGroup")) patch = false;
      if(patch && DerivesFrom("Tool")) patch = false;
   }

   if(display && DerivesFrom("CxxToken")) display = false;

   //  Look for overrides of Patch and Display.
   //
   auto simple = true;
   auto funcs = Funcs();

   for(auto f = funcs->cbegin(); f != funcs->cend(); ++f)
   {
      auto func = f->get();

      if(func->IsOverride())
      {
         if(patch && (*func->Name() == "Patch")) patch = false;
         if(display && (*func->Name() == "Display")) display = false;
      }

      if(func->FuncType() == FuncStandard) simple = false;
   }

   if(display) Log(DisplayNotOverridden);
   if(patch && !simple) Log(PatchNotOverridden);
}

//------------------------------------------------------------------------------

fn_name Class_CheckRuleOfThree = "Class.CheckRuleOfThree";

void Class::CheckRuleOfThree() const
{
   Debug::ft(Class_CheckRuleOfThree);

   if(GetFile()->IsSubsFile()) return;

   auto dtor = FindDtor();
   auto copyCtor = FindFuncByRole(CopyCtor, true);
   auto moveCtor = FindFuncByRole(MoveCtor, true);
   auto copyOper = FindFuncByRole(CopyOper, true);
   auto moveOper = FindFuncByRole(MoveOper, true);
   auto dtorLoc = GetFuncDefinition(dtor);
   auto copyCtorLoc = GetFuncDefinition(copyCtor);
   auto moveCtorLoc = GetFuncDefinition(moveCtor);
   auto copyOperLoc = GetFuncDefinition(copyOper);
   auto moveOperLoc = GetFuncDefinition(moveOper);
   word copyCtorTrivial = -1;
   word copyOperTrivial = -1;
   if((copyCtor != nullptr) && copyCtor->IsTrivial()) copyCtorTrivial = 0;
   if((copyOper != nullptr) && copyOper->IsTrivial()) copyOperTrivial = 0;

   //  The copy constructor and copy operator should be declared together.
   //  However, a non-static, const data member causes a compiler warning
   //  if the copy operator is not implemented, so allow deletion of the
   //  copy operator without declaring a copy constructor.
   //
   switch(copyCtorLoc)
   {
   case NotDeclared:
      if((copyOperLoc == LocalDeclared) && copyOper->IsImplemented())
      {
         Log(RuleOf3CopyOperNoCtor, this, copyOperTrivial);
      }
      break;
   case LocalDeclared:
      if(copyOperLoc == LocalDeclared) break;
      if(copyOperLoc == BaseDeleted) break;
      Log(RuleOf3CopyCtorNoOper, this, copyCtorTrivial);
   }

   switch(copyOperLoc)
   {
   case LocalDeclared:
      if(!copyOper->IsImplemented()) break;
      if(copyCtorLoc == LocalDeclared) break;
      if(copyCtorLoc == BaseDeleted) break;
      Log(RuleOf3CopyOperNoCtor, this, copyOperTrivial);
   }

   //  If the destructor is not trivial, then the copy constructor and copy
   //  operator should be defined or deleted unless the move constructor or
   //  move operator is defined, in which case they need not be defined.
   //
   if((dtorLoc == LocalDeclared) && !dtor->IsTrivial())
   {
      if((moveCtorLoc == NotDeclared) &&
         (moveOperLoc == NotDeclared))
      {
         if((copyCtorLoc == NotDeclared) ||
            (copyCtorLoc == BaseDefined))
         {
            Log(RuleOf3DtorNoCopyCtor);
         }

         if((copyOperLoc == NotDeclared) ||
            (copyOperLoc == BaseDefined))
         {
            Log(RuleOf3DtorNoCopyOper);
         }
      }
   }
}

//------------------------------------------------------------------------------

fn_name Class_ClassDistance = "Class.ClassDistance";

Distance Class::ClassDistance(const Class* cls) const
{
   Debug::ft(Class_ClassDistance);

   Distance dist = 0;

   for(auto curr = this; curr != nullptr; curr = curr->BaseClass())
   {
      if(curr == cls) return dist;
      ++dist;
   }

   return NOT_A_SUBCLASS;
}

//------------------------------------------------------------------------------

fn_name Class_CreateCode = "Class.CreateCode";

size_t Class::CreateCode(const ClassInst* inst, stringPtr& code) const
{
   Debug::ft(Class_CreateCode);

   //  If this is a class template, get its source code.
   //
   auto& tmpltName = *Name();
   if(!IsTemplate()) return CreateCodeError(tmpltName, 0);

   if(code_ == nullptr)
   {
      //  This is the first instantiation, so get the class template's code.
      //
      std::ostringstream stream;
      Display(stream, EMPTY_STR, Flags(NS_Mask | Code_Mask | NoTP_Mask));
      code_.reset(new string(stream.str()));
   }

   code.reset(new string(*code_));

   //  If the template is a specialization, delete its arguments.
   //
   auto tmpltSpec = GetQualName()->GetTemplateArgs();

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
   //  any inner templates.  Note that the lexer must be reinitialized
   //  each time through, because it caches the length of CODE, which
   //  changes as the result of symbol substitution.
   //
   Lexer lexer;
   auto& instName = *inst->Name();
   auto begin = code->find(tmpltName);

   while(true)
   {
      auto end = code->find(TEMPLATE_STR, begin);
      end = Replace(*code, tmpltName, instName, begin, end);
      if(end == string::npos) break;
      begin = code->find('{', end);
      if(begin == string::npos) return CreateCodeError(tmpltName, 4);
      lexer.Initialize(code.get());
      lexer.Reposition(begin);
      begin = lexer.FindClosing('{', '}', begin);
      if(begin == string::npos) return CreateCodeError(tmpltName, 5);
   }

   //  Replace the template parameters with the instance arguments.
   //
   begin = code->find(instName) + instName.size();
   ReplaceTemplateParms(*code, inst->GetTemplateArgs()->Args(), begin);
   return begin;
}

//------------------------------------------------------------------------------

fn_name Class_CreateCodeError = "Class.CreateCodeError";

size_t Class::CreateCodeError(const string& name, debug32_t offset)
{
   Debug::ft(Class_CreateCodeError);

   auto expl = "Could not find code for " + name;
   Context::SwLog(Class_CreateCodeError, expl, offset);
   return string::npos;
}

//------------------------------------------------------------------------------

fn_name Class_CreateInstance = "Class.CreateInstance";

ClassInst* Class::CreateInstance(const string& name, const TypeName* type)
{
   Debug::ft(Class_CreateInstance);

   QualNamePtr newName(new QualName(name));
   newName->CopyContext(this);
   ClassInstPtr tmplt(new ClassInst(newName, this, type));
   auto inst = tmplt.get();
   inst->CopyContext(this);
   tmplts_.push_back(std::move(tmplt));
   return inst;
}

//------------------------------------------------------------------------------

fn_name Class_DerivesFrom1 = "Class.DerivesFrom(class)";

bool Class::DerivesFrom(const Class* cls) const
{
   Debug::ft(Class_DerivesFrom1);

   auto dist = ClassDistance(cls);
   return ((dist > 0) && (dist != NOT_A_SUBCLASS));
}

//------------------------------------------------------------------------------

fn_name Class_DerivesFrom2 = "Class.DerivesFrom(name)";

bool Class::DerivesFrom(const string& name) const
{
   Debug::ft(Class_DerivesFrom2);

   for(auto s = BaseClass(); s != nullptr; s = s->BaseClass())
   {
      if(*s->Name() == name) return true;
   }

   return false;
}

//------------------------------------------------------------------------------

void Class::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   auto code = options.test(DispCode);
   auto fq = options.test(DispFQ);
   stream << prefix;
   DisplayBase(stream, options);

   if(!fq && !code && AtFileScope())
   {
      stream << " // ";
      DisplayFiles(stream);
   }

   auto lead = prefix + spaces(INDENT_SIZE);
   auto qual = options;
   auto nonqual = options;
   qual.set(DispFQ);
   nonqual.reset(DispFQ);
   nonqual.reset(DispNoTP);

   stream << CRLF << prefix << '{' << CRLF;
   DisplayObjects(friends_, stream, lead, qual);
   DisplayObjects(*Usings(), stream, lead, qual);
   DisplayObjects(*Forws(), stream, lead, qual);
   DisplayObjects(*Classes(), stream, lead, nonqual);
   DisplayObjects(*Enums(), stream, lead, nonqual);
   DisplayObjects(*Types(), stream, lead, nonqual);
   if(code) DisplayObjects(*Datas(), stream, lead, nonqual);
   DisplayObjects(*Funcs(), stream, lead, nonqual);
   DisplayObjects(*Opers(), stream, lead, nonqual);
   if(!code) DisplayObjects(*Datas(), stream, lead, nonqual);

   if(!code)
   {
      lead += spaces(INDENT_SIZE);

      if(!subs_.empty())
      {
         stream << prefix << spaces(INDENT_SIZE) << "subclasses:" << CRLF;

         for(auto s = subs_.cbegin(); s != subs_.cend(); ++s)
         {
            stream << lead << (*s)->ScopedName(true) << CRLF;
         }
      }

      if(!tmplts_.empty())
      {
         stream << prefix << spaces(INDENT_SIZE)
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

   if(Name()->front() != '$')
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

   auto lead = prefix + spaces(INDENT_SIZE);

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
      auto expl = *Name() + " is not a class template";
      Context::SwLog(Class_EnsureInstance, expl, 0);
      return nullptr;
   }

   //  See if the template instance already exists.
   //
   auto syms = Singleton< CxxSymbols >::Instance();
   auto file = Context::File();
   if(file == nullptr) return nullptr;
   auto scope = GetScope();
   auto name = *Name() + type->TypeString(true);
   auto area = static_cast< CxxArea* >(GetScope());
   SymbolView view;
   auto inst = syms->FindSymbol(file, scope, name, CLASS_MASK, &view, area);
   if(inst != nullptr) return static_cast< ClassInst* >(inst);

   //  The instance doesn't exist, so create it.  If the template
   //  class has specializations, choose the most appropriate one.
   //
   SymbolVector list;
   ViewVector views;
   syms->FindSymbols(file, scope, *Name(), CLASS_MASK, list, views, area);

   Class* base = this;

   if(list.size() > 1)
   {
      TypeMatch match = Incompatible;

      for(auto s = list.cbegin(); s != list.cend(); ++s)
      {
         auto c = static_cast< Class* >(*s);
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

fn_name Class_EnterScope = "Class.EnterScope";

bool Class::EnterScope()
{
   Debug::ft(Class_EnterScope);

   if(AtFileScope()) GetFile()->InsertClass(this);
   return true;
}

//------------------------------------------------------------------------------

fn_name Class_FindCtor = "Class.FindCtor";

Function* Class::FindCtor
   (StackArgVector* args, const CxxScope* scope, SymbolView* view)
{
   Debug::ft(Class_FindCtor);

   //  If no arguments were provided, look for any constructor.
   //
   if(args == nullptr)
   {
      auto funcs = CxxArea::Funcs();
      for(auto f = funcs->cbegin(); f != funcs->cend(); ++f)
      {
         if((*f)->FuncType() == FuncCtor) return f->get();
      }

      return nullptr;
   }

   //  If no "this" argument was provided, insert one.
   //
   if(args->empty() || !args->front().IsThis())
   {
      args->insert(args->begin(), StackArg(this, 1));
   }

   return FindFunc(*Name(), args, false, scope, view);
}

//------------------------------------------------------------------------------

fn_name Class_FindCtors = "Class.FindCtors";

std::vector< Function* > Class::FindCtors() const
{
   Debug::ft(Class_FindCtors);

   std::vector< Function* > ctors;
   const FunctionPtrVector* funcs = Funcs();

   for(auto f = funcs->cbegin(); f != funcs->cend(); ++f)
   {
      if((*f)->FuncRole() == PureCtor) ctors.push_back(f->get());
   }

   if(ctors.empty()) ctors.push_back(nullptr);
   return ctors;
}

//------------------------------------------------------------------------------

fn_name Class_FindDtor = "Class.FindDtor";

Function* Class::FindDtor(const CxxScope* scope, SymbolView* view) const
{
   Debug::ft(Class_FindDtor);

   auto name = *Name();
   name.insert(0, 1, '~');
   return FindFunc(name, nullptr, false, scope, view);
}

//------------------------------------------------------------------------------

fn_name Class_FindFriend = "Class.FindFriend";

Friend* Class::FindFriend(const CxxScope* scope) const
{
   Debug::ft(Class_FindFriend);

   if(friends_.empty()) return nullptr;

   auto fqScope = scope->ScopedName(true);

   for(auto f = friends_.cbegin(); f != friends_.cend(); ++f)
   {
      if((*f)->IsSuperscopeOf(fqScope, true)) return f->get();
   }

   return nullptr;
}

//------------------------------------------------------------------------------

fn_name Class_FindFunc = "Class.FindFunc(scope)";

Function* Class::FindFunc(const string& name, StackArgVector* args,
   bool base, const CxxScope* scope, SymbolView* view) const
{
   Debug::ft(Class_FindFunc);

   auto f = CxxArea::FindFunc(name, args, false, scope, view);
   if(MemberIsAccessibleTo(f, scope, view)) return f;
   if(!base) return nullptr;

   for(auto s = BaseClass(); s != nullptr; s = s->BaseClass())
   {
      f = s->FindFunc(name, args, false, scope, view);
      if(MemberIsAccessibleTo(f, scope, view)) return f;
   }

   return nullptr;
}

//------------------------------------------------------------------------------

fn_name Class_FindFuncByRole = "Class.FindFuncByRole";

Function* Class::FindFuncByRole(FunctionRole role, bool base) const
{
   Debug::ft(Class_FindFuncByRole);

   switch(role)
   {
   case FuncOther:
   case PureCtor:
      Context::SwLog(Class_FindFuncByRole, "Role not supported", role);
      return nullptr;
   }

   const FunctionPtrVector* funcs = nullptr;

   switch(role)
   {
   case FuncOther:
      return nullptr;
   case CopyOper:
   case MoveOper:
      funcs = Opers();
      break;
   default:
      funcs = Funcs();
   }

   for(auto f = funcs->cbegin(); f != funcs->cend(); ++f)
   {
      if((*f)->FuncRole() == role) return f->get();
   }

   if(!base) return nullptr;
   auto super = BaseClass();
   if(super == nullptr) return nullptr;
   return super->FindFuncByRole(role, base);
}

//------------------------------------------------------------------------------

fn_name Class_FindMember = "Class.FindMember";

CxxScoped* Class::FindMember(const string& name,
   bool base, const CxxScope* scope, SymbolView* view) const
{
   Debug::ft(Class_FindMember);

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

fn_name Class_FindName = "Class.FindName";

CxxScoped* Class::FindName(const string& name, const Class* base) const
{
   Debug::ft(Class_FindName);

   auto item = FindMember(name, false);
   if(item != nullptr) return item;

   auto s = BaseClass();
   if((s == nullptr) || (s == base)) return nullptr;
   return s->FindName(name, base);
}

//------------------------------------------------------------------------------

Class* Class::GetClassTemplate() const
{
   if(!IsTemplate()) return nullptr;
   return const_cast< Class* >(this);
}

//------------------------------------------------------------------------------

fn_name Class_GetConvertibleTypes = "Class.GetConvertibleTypes";

void Class::GetConvertibleTypes(StackArgVector& types)
{
   Debug::ft(Class_GetConvertibleTypes);

   Instantiate();

   auto opers = Opers();

   for(auto o = opers->cbegin(); o != opers->cend(); ++o)
   {
      auto oper = o->get();

      if((oper->Operator() == Cxx::CAST) && !oper->IsExplicit())
      {
         auto spec = (*o)->GetTypeSpec();
         types.push_back(spec->ResultType());
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

fn_name Class_GetFuncDefinition = "Class.GetFuncDefinition";

FunctionDefinition Class::GetFuncDefinition(const Function* func) const
{
   Debug::ft(Class_GetFuncDefinition);

   if(func == nullptr) return NotDeclared;
   if(func->GetScope() == this) return LocalDeclared;

   if(!func->IsImplemented() || (func->GetAccess() == Cxx::Private))
   {
      return BaseDeleted;
   }

   return BaseDefined;
}

//------------------------------------------------------------------------------

fn_name Class_GetFuncIndex = "Class.GetFuncIndex";

bool Class::GetFuncIndex(const Function* func, size_t& idx) const
{
   Debug::ft(Class_GetFuncIndex);

   auto list = FuncVector(*func->Name());

   for(idx = 0; idx < list->size(); ++idx)
   {
      if(list->at(idx).get() == func) return true;
   }

   return false;
}

//------------------------------------------------------------------------------

fn_name Class_GetMemberInitAttrs = "Class.GetMemberInitAttrs";

void Class::GetMemberInitAttrs(DataInitVector& members) const
{
   Debug::ft(Class_GetMemberInitAttrs);

   auto data = Datas();

   for(size_t i = 0; i < data->size(); ++i)
   {
      //  The member should be initialized if it is not default constructible.
      //  However, exempt a member that appears in a union.
      //
      auto mem = data->at(i).get();
      auto init = (!mem->IsDefaultConstructible() && !mem->IsUnionMember());
      DataInitAttrs attrs(mem, init, 0);
      members.push_back(attrs);
   }
}

//------------------------------------------------------------------------------

size_t Class::GetRange(size_t& begin, size_t& end) const
{
   //  Set BEGIN to where the class definition begins, and END to the offset of
   //  its closing right brace.  Return the offset of the opening left brace.
   //
   auto lexer = GetFile()->GetLexer();
   begin = GetPos();
   lexer.Reposition(begin);
   auto left = lexer.FindFirstOf("{");
   end = lexer.FindClosing('{', '}', left);
   return left;
}

//------------------------------------------------------------------------------

CxxScope* Class::GetTemplate() const
{
   if(!IsTemplate()) return nullptr;
   return static_cast< CxxScope* >(const_cast< Class* >(this));
}

//------------------------------------------------------------------------------

fn_name Class_GetUsageAttrs = "Class.GetUsageAttrs";

Class::UsageAttributes Class::GetUsageAttrs() const
{
   Debug::ft(Class_GetUsageAttrs);

   UsageAttributes attrs;

   if(!subs_.empty()) attrs.set(IsBase);
   if(!tmplts_.empty()) attrs.set(HasInstantiations);

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
            if((*o)->IsStatic())
               attrs.set(HasPublicStaticFunction);
            else
               attrs.set(HasPublicMemberFunction);
         }
         else
         {
            if((*o)->IsStatic())
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
         if((*f)->FuncType() == FuncCtor)
         {
            attrs.set(IsConstructed);
         }
         else
         {
            if((*f)->GetAccess() == Cxx::Public)
            {
               if((*f)->IsStatic())
                  attrs.set(HasPublicStaticFunction);
               else
                  attrs.set(HasPublicMemberFunction);
            }
            else
            {
               if((*f)->IsStatic())
                  attrs.set(HasNonPublicStaticFunction);
               else
                  attrs.set(HasNonPublicMemberFunction);
            }
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

fn_name Class_GetUsages = "Class.GetUsages";

void Class::GetUsages(const CodeFile& file, CxxUsageSets& symbols) const
{
   Debug::ft(Class_GetUsages);

   auto inst = IsInTemplateInstance();

   if(IsTemplate())
   {
      //  A class template cannot be executed by itself, so it must get its
      //  symbol usage information from its instantiations.  We compile the
      //  full template instead of instantiating each member when it is used,
      //  so it is sufficient to pull symbols from the first instantiation.
      //
      if(!tmplts_.empty())
      {
         auto first = tmplts_.front().get();
         first->GetUsages(file, symbols);
      }
   }

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

   auto funcs = Funcs();

   for(auto f = funcs->cbegin(); f != funcs->cend(); ++f)
   {
      //  Unless this is a class template instance, bypass function
      //  template instantiations, every one of which is registered
      //  against the class that defines the function template.
      //
      if(!inst)
      {
         if((*f)->IsInTemplateInstance()) continue;
      }

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
}

//------------------------------------------------------------------------------

fn_name Class_GetUsingFor = "Class.GetUsingFor";

Using* Class::GetUsingFor(const string& fqName,
   size_t prefix, const CxxNamed* item, const CxxScope* scope) const
{
   Debug::ft(Class_GetUsingFor);

   auto usings = Usings();

   for(auto u = usings->cbegin(); u != usings->cend(); ++u)
   {
      if((*u)->IsUsingFor(fqName, prefix, scope)) return u->get();
   }

   return nullptr;
}

//------------------------------------------------------------------------------

fn_name Class_HasPODMember = "Class.HasPODMember";

bool Class::HasPODMember() const
{
   Debug::ft(Class_HasPODMember);

   auto data = Datas();

   for(auto d = data->cbegin(); d != data->cend(); ++d)
   {
      if((*d)->IsPOD()) return true;
   }

   return false;
}

//------------------------------------------------------------------------------

fn_name Class_IsDefaultConstructible = "Class.IsDefaultConstructible";

bool Class::IsDefaultConstructible()
{
   Debug::ft(Class_IsDefaultConstructible);

   //  A class is default constructible if it
   //  o is not a union;
   //  o it implements a default constructor or all of its data is default
   //    constructible, in which case the compiler provides the constructor;
   //  o its chain of base classes is default constructible.
   //
   if(FindCtor(nullptr) == nullptr)
   {
      if(tag_ == Cxx::UnionType) return false;

      auto data = Datas();

      for(size_t i = 0 ; i < data->size(); ++i)
      {
         if(!data->at(i)->IsDefaultConstructible()) return false;
      }
   }

   auto s = BaseClass();
   if(s == nullptr) return true;
   return s->IsDefaultConstructible();
}

//------------------------------------------------------------------------------

fn_name Class_IsImplemented = "Class.IsImplemented";

bool Class::IsImplemented() const
{
   Debug::ft(Class_IsImplemented);

   auto funcs = Funcs();

   for(auto f = funcs->cbegin(); f != funcs->cend(); ++f)
   {
      if((*f)->IsImplemented()) return true;
   }

   auto opers = Opers();

   for(auto o = opers->cbegin(); o != opers->cend(); ++o)
   {
      if((*o)->IsImplemented()) return true;
   }

   return false;
}

//------------------------------------------------------------------------------

fn_name Class_MatchFunc = "Class.MatchFunc";

Function* Class::MatchFunc(const Function* curr, bool base) const
{
   Debug::ft(Class_MatchFunc);

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
      auto expl = *Name() + " is not a class template";
      Context::SwLog(Class_MatchTemplate, expl, 0);
      return Incompatible;
   }

   auto spec = GetQualName()->GetTemplateArgs();
   if(spec == nullptr) return Abridgeable;

   //  This is a template specialization.  If it and TYPE have the same number
   //  of arguments, find out how well they match.
   //
   auto thisArgs = spec->Args();
   auto thatArgs = type.Args();
   if(thisArgs->size() != thatArgs->size())
   {
      auto expl = "Invalid number of template arguments for " + *Name();
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

fn_name Class_MemberIsAccessibleTo = "Class.MemberIsAccessibleTo";

bool Class::MemberIsAccessibleTo
   (const CxxScoped* member, const CxxScope* scope, SymbolView* view)
{
   Debug::ft(Class_MemberIsAccessibleTo);

   SymbolView local;
   if(member == nullptr) return false;
   if(scope == nullptr) return true;
   if(view == nullptr) view = &local;

   member->AccessibilityTo(scope, view);
   if(view->accessibility != Inaccessible) return true;

   //  We should never get here when compiling well-formed code, so there is
   //  probably a bug in AccessibilityOf.  Log this, but assume that ITEM is
   //  accessible.
   //
   auto expl = member->ScopedName(true) + " is inaccessible";
   Context::SwLog(Class_MemberIsAccessibleTo, expl, 0);
   return true;
}

//------------------------------------------------------------------------------

fn_name Class_NameToArg = "Class.NameToArg";

StackArg Class::NameToArg(Cxx::Operator op)
{
   Debug::ft(Class_NameToArg);

   //  This will be a constructor call unless this class's size is being
   //  looked up.  Set the "invoke" flag on the argument, which will be
   //  used to invoke a constructor.
   //
   StackArg arg(this, 0);
   if(op != Cxx::SIZEOF_TYPE) arg.SetInvoke();
   return arg;
}

//------------------------------------------------------------------------------

Class* Class::OuterClass() const
{
   return GetScope()->GetClass();
}

//------------------------------------------------------------------------------

fn_name Class_SetCurrAccess = "Class.SetCurrAccess";

bool Class::SetCurrAccess(Cxx::Access access)
{
   Debug::ft(Class_SetCurrAccess);

   if(currAccess_ == access)
   {
      auto parser = Context::GetParser();

      if(!parser->ParsingTemplateInstance())
      {
         auto file = Context::File();
         file->LogPos(parser->GetPrev(), RedundantAccessControl);
      }
   }

   currAccess_ = access;
   return true;
}

//------------------------------------------------------------------------------

fn_name Class_SetTemplateParms = "Class.SetTemplateParms";

void Class::SetTemplateParms(TemplateParmsPtr& parms)
{
   Debug::ft(Class_SetTemplateParms);

   parms_ = std::move(parms);
}

//------------------------------------------------------------------------------

void Class::Shrink()
{
   CxxArea::Shrink();

   name_->Shrink();
   if(parms_ != nullptr) parms_->Shrink();
   if(base_ != nullptr) base_->Shrink();

   for(auto f = friends_.cbegin(); f != friends_.cend(); ++f)
   {
      (*f)->Shrink();
   }

   for(auto t = tmplts_.cbegin(); t != tmplts_.cend(); ++t)
   {
      (*t)->Shrink();
   }

   subs_.shrink_to_fit();

   auto size = friends_.capacity() * sizeof(FriendPtr);
   size += (tmplts_.capacity() * sizeof(ClassInstPtr));
   size += (subs_.capacity() * sizeof(Class*));

   if(IsInTemplateInstance())
      CxxStats::Vectors(CxxStats::CLASS_INST, size);
   else
      CxxStats::Vectors(CxxStats::CLASS_DECL, size);
}

//------------------------------------------------------------------------------

string Class::TypeString(bool arg) const
{
   return Prefix(GetScope()->TypeString(arg)) + *Name();
}

//==============================================================================

fn_name ClassInst_ctor = "ClassInst.ctor";

ClassInst::ClassInst(QualNamePtr& name, Class* tmplt, const TypeName* spec) :
   Class(name, tmplt->GetClassTag()),
   tmplt_(tmplt),
   tspec_(nullptr),
   refs_(0),
   created_(false),
   compiled_(false)
{
   Debug::ft(ClassInst_ctor);

   tspec_.reset(new TypeName(*spec));
   tspec_->CopyContext(spec);
   CxxStats::Incr(CxxStats::CLASS_INST);
   CxxStats::Decr(CxxStats::CLASS_DECL);
}

//------------------------------------------------------------------------------

fn_name ClassInst_dtor = "ClassInst.dtor";

ClassInst::~ClassInst()
{
   Debug::ft(ClassInst_dtor);

   //  The following is the kind of thing that can happen when a base class
   //  is not always virtual.
   //
   CxxStats::Decr(CxxStats::CLASS_INST);
   CxxStats::Incr(CxxStats::CLASS_DECL);
}

//------------------------------------------------------------------------------

fn_name ClassInst_DerivesFrom = "ClassInst.DerivesFrom";

bool ClassInst::DerivesFrom(const Class* cls) const
{
   Debug::ft(ClassInst_DerivesFrom);

   //  This is a class template instance, T<args1>.  If CLS is not a class
   //  template instance or is not an instance of the same class template,
   //  just invoke the base class version of the function.
   //
   if(!cls->IsInTemplateInstance()) return Class::DerivesFrom(cls);
   if(cls->GetTemplate() != tmplt_) return Class::DerivesFrom(cls);
   auto thatSpec = cls->GetTemplateArgs();
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
   if(!created_) stream << ";";

   std::ostringstream buff;
   buff << " // ";
   if(options.test(DispStats)) buff << "r=" << refs_ << SPACE;

   if(!created_)
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
      auto lead = prefix + spaces(INDENT_SIZE);
      auto qual = options;
      auto opts = options;
      qual.set(DispFQ);

      DisplayObjects(*Friends(), stream, lead, qual);
      DisplayObjects(*Usings(), stream, lead, qual);
      DisplayObjects(*Forws(), stream, lead, qual);
      DisplayObjects(*Classes(), stream, lead, opts);
      DisplayObjects(*Enums(), stream, lead, opts);
      DisplayObjects(*Types(), stream, lead, opts);
      DisplayObjects(*Funcs(), stream, lead, opts);
      DisplayObjects(*Opers(), stream, lead, opts);
      DisplayObjects(*Datas(), stream, lead, opts);
   }

   stream << prefix << "};" << CRLF;
}

//------------------------------------------------------------------------------

fn_name ClassInst_FindInstanceAnalog = "ClassInst.FindInstanceAnalog";

CxxScoped* ClassInst::FindInstanceAnalog(const CxxNamed* item) const
{
   Debug::ft(ClassInst_FindInstanceAnalog);

   if(!created_) return nullptr;

   auto type = item->Type();

   switch(type)
   {
   case Cxx::Class:
      return const_cast< ClassInst* >(this);

   case Cxx::Function:
      size_t idx;
      auto func = static_cast< const Function* >(item);
      if(!tmplt_->GetFuncIndex(func, idx)) return nullptr;
      auto list = FuncVector(*item->Name());
      return list->at(idx).get();
   }

   return FindMember(*item->Name(), false);
}

//------------------------------------------------------------------------------

fn_name ClassInst_FindTemplateAnalog = "ClassInst.FindTemplateAnalog";

CxxScoped* ClassInst::FindTemplateAnalog(const CxxNamed* item) const
{
   Debug::ft(ClassInst_FindTemplateAnalog);

   auto type = item->Type();

   switch(type)
   {
   case Cxx::Class:
      return tmplt_;

   case Cxx::Function:
      size_t idx;
      auto func = static_cast< const Function* >(item);
      if(!GetFuncIndex(func, idx)) return nullptr;
      auto list = tmplt_->FuncVector(*item->Name());
      return list->at(idx).get();
   }

   return tmplt_->FindMember(*item->Name(), false);
}

//------------------------------------------------------------------------------

fn_name ClassInst_GetUsages = "ClassInst.GetUsages";

void ClassInst::GetUsages(const CodeFile& file, CxxUsageSets& symbols) const
{
   Debug::ft(ClassInst_GetUsages);

   //  This is invoked by a class template in order to obtain symbol usage
   //  information from one of its instances.
   //
   CxxUsageSets sets;
   Class::GetUsages(file, sets);
   sets.EraseTemplateArgs(tspec_.get());
   symbols.Union(sets);
}

//------------------------------------------------------------------------------

fn_name ClassInst_Instantiate = "ClassInst.Instantiate";

bool ClassInst::Instantiate()
{
   Debug::ft(ClassInst_Instantiate);

   //  Return if the template has already been instantiated.  Otherwise,
   //  notify tspec_, which contains the template name and arguments, that
   //  its template is being instantiated.  This causes the instantiation
   //  of any templates that this one requires.
   //
   if(created_) return true;
   created_ = true;
   tspec_->Instantiating();

   //  Get the code for the template instance and parse it.
   //
   code_.reset();
   auto begin = tmplt_->CreateCode(this, code_);
   std::unique_ptr< Parser > parser(new Parser(EMPTY_STR));
   compiled_ = parser->ParseClassInst(this, begin);
   parser.reset();
   if(compiled_) code_.reset();
   return compiled_;
}

//------------------------------------------------------------------------------

void ClassInst::Shrink()
{
   Class::Shrink();

   tspec_->Shrink();
}

//------------------------------------------------------------------------------

string ClassInst::TypeString(bool arg) const
{
   return Prefix(GetScope()->TypeString(arg)) +
      *tspec_->Name() + tspec_->TypeString(arg);
}

//==============================================================================

fn_name CxxArea_ctor = "CxxArea.ctor";

CxxArea::CxxArea()
{
   Debug::ft(CxxArea_ctor);
}

//------------------------------------------------------------------------------

fn_name CxxArea_dtor = "CxxArea.dtor";

CxxArea::~CxxArea()
{
   Debug::ft(CxxArea_dtor);
}

//------------------------------------------------------------------------------

fn_name CxxArea_AddClass = "CxxArea.AddClass";

bool CxxArea::AddClass(ClassPtr& cls)
{
   Debug::ft(CxxArea_AddClass);

   if(AddAnonymousUnion(cls)) return true;

   if(cls->EnterScope())
   {
      AddItem(cls.get());
      classes_.push_back(std::move(cls));
   }

   return true;
}

//------------------------------------------------------------------------------

fn_name CxxArea_AddData = "CxxArea.AddData";

bool CxxArea::AddData(DataPtr& data)
{
   Debug::ft(CxxArea_AddData);

   if(data->EnterScope())
   {
      AddItem(data.get());
      data_.push_back(std::move(data));
   }
   else
   {
      defns_.push_back(std::move(data));
   }

   return true;
}

//------------------------------------------------------------------------------

fn_name CxxArea_AddEnum = "CxxArea.AddEnum";

bool CxxArea::AddEnum(EnumPtr& decl)
{
   Debug::ft(CxxArea_AddEnum);

   if(decl->EnterScope())
   {
      AddItem(decl.get());
      enums_.push_back(std::move(decl));
   }

   return true;
}

//------------------------------------------------------------------------------

fn_name CxxArea_AddForw = "CxxArea.AddForw";

bool CxxArea::AddForw(ForwardPtr& forw)
{
   Debug::ft(CxxArea_AddForw);

   if(forw->EnterScope())
   {
      AddItem(forw.get());
      forws_.push_back(std::move(forw));
   }

   return true;
}

//------------------------------------------------------------------------------

fn_name CxxArea_AddFunc = "CxxArea.AddFunc";

bool CxxArea::AddFunc(FunctionPtr& func)
{
   Debug::ft(CxxArea_AddFunc);

   //  If this is an inline function, do not add it to a template instance.
   //  Simply returning results in FUNC being deleted.
   //
   if(func->IsInline())
   {
      auto cls = GetClass();
      if((cls != nullptr) && cls->IsInTemplateInstance()) return true;
   }

   if(func->EnterScope())
   {
      AddItem(func.get());

      if(func->FuncType() == FuncOperator)
         opers_.push_back(std::move(func));
      else
         funcs_.push_back(std::move(func));
   }
   else
   {
      defns_.push_back(std::move(func));
   }

   return true;
}

//------------------------------------------------------------------------------

fn_name CxxArea_AddType = "CxxArea.AddType";

bool CxxArea::AddType(TypedefPtr& type)
{
   Debug::ft(CxxArea_AddType);

   if(type->EnterScope())
   {
      AddItem(type.get());
      types_.push_back(std::move(type));
   }

   return true;
}

//------------------------------------------------------------------------------

fn_name CxxArea_AddUsing = "CxxArea.AddUsing";

bool CxxArea::AddUsing(UsingPtr& use)
{
   Debug::ft(CxxArea_AddUsing);

   if(use->EnterScope()) usings_.push_back(std::move(use));
   return true;
}

//------------------------------------------------------------------------------

fn_name CxxArea_Check = "CxxArea.Check";

void CxxArea::Check() const
{
   Debug::ft(CxxArea_Check);

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
}

//------------------------------------------------------------------------------

fn_name CxxArea_FindClass = "CxxArea.FindClass";

Class* CxxArea::FindClass(const string& name) const
{
   Debug::ft(CxxArea_FindClass);

   for(auto c = classes_.cbegin(); c != classes_.cend(); ++c)
   {
      if(*(*c)->Name() == name) return c->get();
   }

   return nullptr;
}

//------------------------------------------------------------------------------

fn_name CxxArea_FindData = "CxxArea.FindData";

Data* CxxArea::FindData(const string& name) const
{
   Debug::ft(CxxArea_FindData);

   for(auto d = data_.cbegin(); d != data_.cend(); ++d)
   {
      if(*(*d)->Name() == name) return d->get();
   }

   return nullptr;
}

//------------------------------------------------------------------------------

fn_name CxxArea_FindEnum = "CxxArea.FindEnum";

Enum* CxxArea::FindEnum(const string& name) const
{
   Debug::ft(CxxArea_FindEnum);

   for(auto e = enums_.cbegin(); e != enums_.cend(); ++e)
   {
      if(*(*e)->Name() == name) return e->get();
   }

   return nullptr;
}

//------------------------------------------------------------------------------

fn_name CxxArea_FindEnumerator = "CxxArea.FindEnumerator";

Enumerator* CxxArea::FindEnumerator(const string& name) const
{
   Debug::ft(CxxArea_FindEnumerator);

   for(auto e = enums_.cbegin(); e != enums_.cend(); ++e)
   {
      auto m = (*e)->FindEnumerator(name);
      if(m != nullptr) return m;
   }

   return nullptr;
}

//------------------------------------------------------------------------------

fn_name CxxArea_FindFunc = "CxxArea.FindFunc";

Function* CxxArea::FindFunc(const string& name, StackArgVector* args,
   bool base, const CxxScope* scope, SymbolView* view) const
{
   Debug::ft(CxxArea_FindFunc);

   //  Get the type string for each argument in ARGS.
   //
   FunctionVector funcs;
   std::vector< TypeMatch > matches;

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
      auto& temp = *func->Name();

      if(temp == name)
      {
         if(args == nullptr) return FoundFunc(func, view, Compatible);

         auto match = Incompatible;
         func = func->CanInvokeWith(*args, argTypes, match);
         if(match == Compatible) return FoundFunc(func, view, Compatible);

         if(func != nullptr)
         {
            funcs.push_back(func);
            matches.push_back(match);
         }
      }
   }

   auto count = funcs.size();
   if(count == 1) return FoundFunc(funcs.front(), view, matches.front());
   if(count == 0) return FoundFunc(nullptr, view, Incompatible);

   //  Return the best match.
   //
   Function* func = nullptr;
   auto best = Incompatible;

   for(size_t i = 0; i < count; ++i)
   {
      if(matches[i] > best)
      {
         func = funcs[i];
         best = matches[i];
      }
   }

   return FoundFunc(func, view, best);
}

//------------------------------------------------------------------------------

fn_name CxxArea_FindItem = "CxxArea.FindItem";

CxxScoped* CxxArea::FindItem(const string& name) const
{
   Debug::ft(CxxArea_FindItem);

   auto op = CxxOp::NameToOperator(name);

   if(op != Cxx::NIL_OPERATOR)
   {
      return FindFunc(name, nullptr, false, nullptr, nullptr);
   }

   CxxScoped* item = FindData(name);
   if(item != nullptr) return item;

   item = FindFunc(name, nullptr, false, nullptr, nullptr);
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

fn_name CxxArea_FindType = "CxxArea.FindType";

Typedef* CxxArea::FindType(const string& name) const
{
   Debug::ft(CxxArea_FindType);

   for(auto t = types_.cbegin(); t != types_.cend(); ++t)
   {
      if(*(*t)->Name() == name) return t->get();
   }

   return nullptr;
}

//------------------------------------------------------------------------------

fn_name CxxArea_FoundFunc = "CxxArea.FoundFunc";

Function* CxxArea::FoundFunc(Function* func, SymbolView* view, TypeMatch match)
{
   Debug::ft(CxxArea_FoundFunc);

   if(view != nullptr) view->match = match;
   return func;
}

//------------------------------------------------------------------------------

const FunctionPtrVector* CxxArea::FuncVector(const string& name) const
{
   auto size = strlen(OPERATOR_STR);

   if(name.compare(0, size, OPERATOR_STR) == 0)
   {
      //  For this to be an actual operator, the next character must be illegal
      //  in an identifier (internally, the name of each operation function has
      //  the operator punctuation appended to "operator").
      //
      if(!CxxChar::Attrs[name[size]].validNext) return &opers_;
   }

   return &funcs_;
}

//------------------------------------------------------------------------------

fn_name CxxArea_MatchFunc = "CxxArea.MatchFunc";

Function* CxxArea::MatchFunc(const Function* curr, bool base) const
{
   Debug::ft(CxxArea_MatchFunc);

   auto list = FuncVector(*curr->Name());

   for(auto f = list->cbegin(); f != list->cend(); ++f)
   {
      if(*(*f)->Name() == *curr->Name())
      {
         if((*f)->SignatureMatches(curr, base)) return f->get();
      }
   }

   return nullptr;
}

//------------------------------------------------------------------------------

void CxxArea::Shrink()
{
   for(auto u = usings_.cbegin(); u != usings_.cend(); ++u)
   {
      (*u)->Shrink();
   }

   for(auto c = classes_.cbegin(); c != classes_.cend(); ++c)
   {
      (*c)->Shrink();
   }

   for(auto d = data_.cbegin(); d != data_.cend(); ++d)
   {
      (*d)->Shrink();
   }

   for(auto e = enums_.cbegin(); e != enums_.cend(); ++e)
   {
      (*e)->Shrink();
   }

   for(auto f = forws_.cbegin(); f != forws_.cend(); ++f)
   {
      (*f)->Shrink();
   }

   for(auto f = funcs_.cbegin(); f != funcs_.cend(); ++f)
   {
      (*f)->Shrink();
   }

   for(auto o = opers_.cbegin(); o != opers_.cend(); ++o)
   {
      (*o)->Shrink();
   }

   for(auto t = types_.cbegin(); t != types_.cend(); ++t)
   {
      (*t)->Shrink();
   }

   for(auto d = defns_.cbegin(); d != defns_.cend(); ++d)
   {
      (*d)->Shrink();
   }

   auto size = usings_.capacity() * sizeof(UsingPtr);
   size += (classes_.capacity() * sizeof(ClassPtr));
   size += (data_.capacity() * sizeof(DataPtr));
   size += (enums_.capacity() * sizeof(EnumPtr));
   size += (forws_.capacity() * sizeof(ForwardPtr));
   size += (funcs_.capacity() * sizeof(FunctionPtr));
   size += (opers_.capacity() * sizeof(FunctionPtr));
   size += (types_.capacity() * sizeof(TypedefPtr));
   size += (defns_.capacity() * sizeof(TypedefPtr));

   if(Type() == Cxx::Namespace)
   {
      CxxStats::Vectors(CxxStats::SPACE_DECL, size);
   }
   else
   {
      if(IsInTemplateInstance())
         CxxStats::Vectors(CxxStats::CLASS_INST, size);
      else
         CxxStats::Vectors(CxxStats::CLASS_DECL, size);
   }
}

//==============================================================================

fn_name Namespace_ctor = "Namespace.ctor";

Namespace::Namespace(const string& name, Namespace* space) :
   name_(name),
   checked_(false)
{
   Debug::ft(Namespace_ctor);

   CxxArea::SetScope(space);
   Singleton< CxxSymbols >::Instance()->InsertSpace(this);
   CxxStats::Incr(CxxStats::SPACE_DECL);
}

//------------------------------------------------------------------------------

fn_name Namespace_dtor = "Namespace.dtor";

Namespace::~Namespace()
{
   Debug::ft(Namespace_dtor);

   Singleton< CxxSymbols >::Instance()->EraseSpace(this);
   CxxStats::Decr(CxxStats::SPACE_DECL);
}

//------------------------------------------------------------------------------

fn_name Namespace_AccessibilityOf = "Namespace.AccessibilityOf";

void Namespace::AccessibilityOf
   (const CxxScope* scope, const CxxScoped* item, SymbolView* view) const
{
   Debug::ft(Namespace_AccessibilityOf);

   view->accessibility = (item->GetFile()->IsCpp() ? Restricted : Unrestricted);
   view->distance = scope->ScopeDistance(this);
}

//------------------------------------------------------------------------------

fn_name Namespace_Check = "Namespace.Check";

void Namespace::Check() const
{
   Debug::ft(Namespace_Check);

   if(checked_) return;
   checked_ = true;

   auto name = ScopedName(false);

   if(name.empty())
      name = "namespace ::";
   else
      name = "namespace " + name;

   name.push_back(CRLF);
   Debug::Progress(name, true);

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
   auto name = Name()->c_str();

   if(strlen(name) == 0) name = SCOPE_STR;

   if(!options.test(DispCode))
   {
      stream << prefix << string(132 - prefix.size(), '-') << CRLF;
   }

   stream << prefix << NAMESPACE_STR << SPACE << name << CRLF;
   stream << prefix << '{' << CRLF;

   auto lead = prefix + spaces(INDENT_SIZE);
   auto nonqual = options;
   nonqual.reset(DispFQ);

   DisplayObjects(*Enums(), stream, lead, nonqual);
   DisplayObjects(*Types(), stream, lead, nonqual);
   DisplayObjects(*Funcs(), stream, lead, nonqual);
   DisplayObjects(*Opers(), stream, lead, nonqual);
   DisplayObjects(*Datas(), stream, lead, nonqual);
   DisplayObjects(*Classes(), stream, lead, nonqual);
   DisplayObjects(spaces_, stream, lead, nonqual);
   stream << prefix << '}' << CRLF;
}

//------------------------------------------------------------------------------

fn_name Namespace_EnsureNamespace = "Namespace.EnsureNamespace";

Namespace* Namespace::EnsureNamespace(const string& name)
{
   Debug::ft(Namespace_EnsureNamespace);

   //  If a namespace defined by NAME is not found, create it.
   //
   auto s = FindNamespace(name);
   if(s != nullptr) return s;

   NamespacePtr space(new Namespace(name, this));
   spaces_.push_back(std::move(space));
   return spaces_.back().get();
}

//------------------------------------------------------------------------------

fn_name Namespace_FindFunc = "Namespace.FindFunc";

Function* Namespace::FindFunc(const string& name, StackArgVector* args,
   bool base, const CxxScope* scope, SymbolView* view) const
{
   Debug::ft(Namespace_FindFunc);

   auto f = CxxArea::FindFunc(name, args, false, scope, view);
   if(f != nullptr) return f;
   if(!base) return nullptr;

   for(auto s = OuterSpace(); s != nullptr; s = s->OuterSpace())
   {
      f = s->FindFunc(name, args, false, scope, view);
      if(f != nullptr) return f;
   }

   return nullptr;
}

//------------------------------------------------------------------------------

fn_name Namespace_FindItem = "Namespace.FindItem";

CxxScoped* Namespace::FindItem(const string& name) const
{
   Debug::ft(Namespace_FindItem);

   CxxScoped* item = FindNamespace(name);
   if(item != nullptr) return item;

   return CxxArea::FindItem(name);
}

//------------------------------------------------------------------------------

fn_name Namespace_FindNamespace = "Namespace.FindNamespace";

Namespace* Namespace::FindNamespace(const string& name) const
{
   Debug::ft(Namespace_FindNamespace);

   //  Return the namespace, if any, defined by NAME.
   //
   for(auto s = spaces_.cbegin(); s != spaces_.cend(); ++s)
   {
      if(*(*s)->Name() == name) return s->get();
   }

   return nullptr;
}

//------------------------------------------------------------------------------

fn_name Namespace_ScopedName = "Namespace.ScopedName";

string Namespace::ScopedName(bool templates) const
{
   Debug::ft(Namespace_ScopedName);

   auto scope = GetScope();

   if(scope == nullptr)
   {
      //  This is the global namespace.
      //
      return EMPTY_STR;
   }

   if(scope == Singleton< CxxRoot >::Instance()->GlobalNamespace())
   {
      //  This namespace is directly below the global namespace.
      //
      return *Name();
   }

   return scope->ScopedName(templates) + SCOPE_STR + *Name();
}

//------------------------------------------------------------------------------

fn_name Namespace_SetLoc = "Namespace.SetLoc";

void Namespace::SetLoc(CodeFile* file, size_t pos)
{
   Debug::ft(Namespace_SetLoc);

   if(GetFile() == nullptr) CxxArea::SetLoc(file, pos);
}

//------------------------------------------------------------------------------

void Namespace::Shrink()
{
   CxxArea::Shrink();

   name_.shrink_to_fit();
   CxxStats::Strings(CxxStats::SPACE_DECL, name_.capacity());

   for(auto s = spaces_.cbegin(); s != spaces_.cend(); ++s)
   {
      (*s)->Shrink();
   }

   auto size = spaces_.capacity() * sizeof(NamespacePtr);
   CxxStats::Vectors(CxxStats::SPACE_DECL, size);
}

//------------------------------------------------------------------------------

string Namespace::TypeString(bool arg) const
{
   auto scope = GetScope();
   if(scope == nullptr) return EMPTY_STR;
   return Prefix(scope->TypeString(arg)) + *Name();
}
}
