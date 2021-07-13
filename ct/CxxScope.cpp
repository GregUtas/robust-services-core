//==============================================================================
//
//  CxxScope.cpp
//
//  Copyright (C) 2013-2021  Greg Utas
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
#include "CxxScope.h"
#include <iterator>
#include <set>
#include <sstream>
#include <utility>
#include "CodeFile.h"
#include "CxxArea.h"
#include "CxxExecute.h"
#include "CxxRoot.h"
#include "CxxString.h"
#include "CxxSymbols.h"
#include "CxxVector.h"
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
bool FuncDefnsAreSorted(const Function* func1, const Function* func2)
{
   if(func1->GetScope() != func2->GetScope()) return true;

   //  Sort special member functions in the cardinal order defined by
   //  FunctionRole.
   //
   auto role1 = func1->FuncRole();
   auto role2 = func2->FuncRole();
   if(role1 < role2) return true;
   if(role1 > role2) return false;

   //  Operators can appear in any order but are sorted alphabetically
   //  with respect to other functions.
   //
   auto type1 = func1->FuncType();
   auto type2 = func1->FuncType();
   if((type1 == FuncOperator) && (type2 == FuncOperator)) return true;

   //  Sort the functions alphabetically.
   //
   auto result = strCompare(func1->Name(), func2->Name());
   if(result < 0) return true;
   if(result > 0) return false;

   //  The functions have the same name, so leave them as they are.
   //
   return (func1->GetPos() < func2->GetPos());
}

//------------------------------------------------------------------------------

FunctionVector FuncsInArea(const FunctionVector& defns, const CxxArea* area)
{
   FunctionVector funcs;

   for(auto f = defns.cbegin(); f != defns.cend(); ++f)
   {
      if((*f)->GetArea() == area)
      {
         funcs.push_back(*f);
      }
   }

   return funcs;
}

//==============================================================================

UsingVector Block::Usings_ = UsingVector();

//------------------------------------------------------------------------------

Block::Block(bool braced) :
   name_(LOCALS_STR),
   braced_(braced),
   nested_(false)
{
   Debug::ft("Block.ctor");

   CxxStats::Incr(CxxStats::BLOCK_DECL);
}

//------------------------------------------------------------------------------

Block::~Block()
{
   Debug::ftnt("Block.dtor");

   CxxStats::Decr(CxxStats::BLOCK_DECL);
}

//------------------------------------------------------------------------------

bool Block::AddStatement(CxxToken* s)
{
   Debug::ft("Block.AddStatement");

   statements_.push_back(TokenPtr(s));
   s->SetScope(this);
   return true;
}

//------------------------------------------------------------------------------

void Block::AddUsing(Using* use)
{
   Debug::ft("Block.AddUsing");

   Usings_.push_back(use);
}

//------------------------------------------------------------------------------

void Block::Check() const
{
   for(auto s = statements_.cbegin(); s != statements_.cend(); ++s)
   {
      (*s)->Check();
   }
}

//------------------------------------------------------------------------------

bool Block::CrlfOver(Form form) const
{
   //  Whether to insert an endline depends on the number of statements:
   //  o two or more: always inserted
   //  o one: inserted if FORM or the statement requests it
   //  o none: inserted if braced (an empty function) and FORM requests it
   //
   switch(statements_.size())
   {
   case 0:
      if(form == Empty) return braced_;
      return false;

   case 1:
      switch(form)
      {
      case Empty:
         return true;
      case Unbraced:
         if(braced_) return true;
         //  [[fallthrough]]
      default:
         return !statements_.front()->InLine();
      }
   }

   return true;
}

//------------------------------------------------------------------------------

void Block::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   auto opts = options;
   auto lf = options.test(DispLF);

   switch(statements_.size())
   {
   case 0:
      if(!lf)
      {
         Print(stream, options);
      }
      else
      {
         stream << CRLF << prefix << '{';
         stream << CRLF << prefix << '}';
      }
      break;

   case 1:
      if(!nested_)
      {
         if(!lf)
         {
            Print(stream, options);
            break;
         }

         if(!braced_)
         {
            if(!statements_.front()->InLine())
            {
               opts.set(DispLF);
               statements_.front()->Display(stream, prefix, opts);
               return;
            }

            stream << CRLF << prefix << spaces(IndentSize());
            statements_.front()->Print(stream, options);
            break;
         }
      }
      //  [[fallthrough]]
   default:
      if(!nested_) stream << CRLF;
      stream << prefix << '{' << CRLF;
      auto lead = prefix + spaces(IndentSize());

      for(auto s = statements_.cbegin(); s != statements_.cend(); ++s)
      {
         (*s)->Display(stream, lead, opts);
      }

      stream << prefix << '}';
   }

   stream << CRLF;
}

//------------------------------------------------------------------------------

void Block::EnterBlock()
{
   Debug::ft("Block.EnterBlock");

   Context::SetPos(GetLoc());
   Context::PushScope(this, true);

   for(auto s = statements_.cbegin(); s != statements_.cend(); ++s)
   {
      (*s)->EnterBlock();
      Context::Execute();
      Context::Clear(1);
   }

   for(auto s = statements_.crbegin(); s != statements_.crend(); ++s)
   {
      (*s)->ExitBlock();
   }

   Context::PopScope();
}

//------------------------------------------------------------------------------

void Block::EraseItem(const CxxToken* item)
{
   Debug::ft("Block.EraseItem");

   EraseItemPtr(statements_, item);
}

//------------------------------------------------------------------------------

CxxScoped* Block::FindNthItem(const std::string& name, size_t& n) const
{
   Debug::ft("Block.FindNthItem");

   for(auto s = statements_.cbegin(); s != statements_.cend(); ++s)
   {
      auto item = (*s)->FindNthItem(name, n);
      if(item != nullptr) return item;
   }

   return nullptr;
}

//------------------------------------------------------------------------------

CxxToken* Block::FirstStatement() const
{
   if(statements_.empty()) return nullptr;
   return statements_.front().get();
}

//------------------------------------------------------------------------------

Function* Block::GetFunction() const
{
   auto s = GetScope();
   if(s != nullptr) return s->GetFunction();
   return nullptr;
}

//------------------------------------------------------------------------------

void Block::GetUsages(const CodeFile& file, CxxUsageSets& symbols)
{
   for(auto s = statements_.cbegin(); s != statements_.cend(); ++s)
   {
      (*s)->GetUsages(file, symbols);
   }
}

//------------------------------------------------------------------------------

Using* Block::GetUsingFor(const string& fqName,
   size_t prefix, const CxxNamed* item, const CxxScope* scope) const
{
   Debug::ft("Block.GetUsingFor");

   for(auto u = Usings_.cbegin(); u != Usings_.cend(); ++u)
   {
      if((*u)->IsUsingFor(fqName, prefix, scope)) return *u;
   }

   return nullptr;
}

//------------------------------------------------------------------------------

bool Block::InLine() const
{
   auto size = statements_.size();
   if(size >= 2) return false;
   if(size == 0) return true;
   if(nested_) return false;
   return statements_.front()->InLine();
}

//------------------------------------------------------------------------------

bool Block::LocateItem(const CxxToken* item, size_t& n) const
{
   Debug::ft("Block.LocateItem");

   for(auto s = statements_.cbegin(); s != statements_.cend(); ++s)
   {
      if((*s)->LocateItem(item, n)) return true;
   }

   return false;
}

//------------------------------------------------------------------------------

CxxToken* Block::PosToItem(size_t pos) const
{
   if(braced_)
   {
      auto item = CxxScope::PosToItem(pos);
      if(item != nullptr) return item;
   }

   for(auto s = statements_.cbegin(); s != statements_.cend(); ++s)
   {
      auto item = (*s)->PosToItem(pos);
      if(item != nullptr) return item;
   }

   return nullptr;
}

//------------------------------------------------------------------------------

void Block::Print(ostream& stream, const Flags& options) const
{
   switch(statements_.size())
   {
   case 0:
      stream << " { }";
      return;

   case 1:
      if(statements_.front()->Type() != Cxx::NoOp) stream << SPACE;
      if(braced_) stream << "{ ";
      statements_.front()->Print(stream, options);
      if(braced_) stream << " }";
      return;

   default:
      stream << " { /*" << ERROR_STR << "(block=2+) */ }";
   }
}

//------------------------------------------------------------------------------

void Block::RemoveUsing(const Using* use)
{
   Debug::ft("Block.RemoveUsing");

   for(auto u = Usings_.cbegin(); u != Usings_.cend(); ++u)
   {
      if(*u == use)
      {
         Usings_.erase(u);
         return;
      }
   }
}

//------------------------------------------------------------------------------

void Block::ReplaceItem(const CxxToken* curr, CxxToken* next)
{
   Debug::ft("Block.ReplaceItem");

   for(auto s = statements_.begin(); s != statements_.end(); ++s)
   {
      if(s->get() == curr)
      {
         s->release();
         *s = TokenPtr(next);
         return;
      }
   }
}

//------------------------------------------------------------------------------

void Block::ResetUsings()
{
   Debug::ft("Block.ResetUsings");

   Usings_.clear();
}

//------------------------------------------------------------------------------

fn_name Block_ScopedName = "Block.ScopedName";

string Block::ScopedName(bool templates) const
{
   for(auto s = GetScope(); s != nullptr; s = s->GetScope())
   {
      if(s->Type() == Cxx::Function)
      {
         return s->ScopedName(templates) + SCOPE_STR + LOCALS_STR;
      }
   }

   Debug::SwLog(Block_ScopedName, "function not found", 0);
   return ERROR_STR;
}

//------------------------------------------------------------------------------

void Block::Shrink()
{
   CxxScope::Shrink();
   name_.shrink_to_fit();
   CxxStats::Strings(CxxStats::BLOCK_DECL, name_.capacity());

   ShrinkTokens(statements_);
   auto size = statements_.capacity() * sizeof(TokenPtr);
   size += XrefSize();
   CxxStats::Vectors(CxxStats::BLOCK_DECL, size);
}

//------------------------------------------------------------------------------

void Block::UpdatePos
   (EditorAction action, size_t begin, size_t count, size_t from) const
{
   CxxScope::UpdatePos(action, begin, count, from);

   for(auto s = statements_.cbegin(); s != statements_.cend(); ++s)
   {
      (*s)->UpdatePos(action, begin, count, from);
   }
}

//------------------------------------------------------------------------------

void Block::UpdateXref(bool insert)
{
   for(auto s = statements_.cbegin(); s != statements_.cend(); ++s)
   {
      (*s)->UpdateXref(insert);
   }
}

//==============================================================================

ClassData::ClassData(string& name, TypeSpecPtr& type) : Data(type),
   memInit_(nullptr),
   mutable_(false),
   mutated_(false),
   first_(false),
   last_(false),
   depth_(0)
{
   Debug::ft("ClassData.ctor");

   std::swap(name_, name);
   Singleton< CxxSymbols >::Instance()->InsertData(this);
   OpenScope(name_);
   CxxStats::Incr(CxxStats::CLASS_DATA);
}

//------------------------------------------------------------------------------

ClassData::~ClassData()
{
   Debug::ftnt("ClassData.dtor");

   GetFile()->EraseData(this);
   Singleton< CxxSymbols >::Extant()->EraseData(this);
   CxxStats::Decr(CxxStats::CLASS_DATA);
}

//------------------------------------------------------------------------------

void ClassData::Check() const
{
   Debug::ft("ClassData.Check");

   Data::Check();

   if(width_ != nullptr) width_->Check();

   if(IsDecl())
   {
      CheckUsage();

      //  If a class has a copy or move operator, it cannot have a const
      //  member.
      //
      auto cls = GetClass();
      auto copy = cls->FindFuncByRole(CopyOper, true);
      auto move = cls->FindFuncByRole(MoveOper, true);
      auto could = (((copy == nullptr) || copy->IsDeleted()) &&
         ((move == nullptr) || move->IsDeleted()));
      CheckConstness(could);

      CheckIfInitialized();
      CheckIfRelocatable();
      CheckIfHiding();
      CheckAccessControl();
      CheckIfMutated();
   }
}

//------------------------------------------------------------------------------

void ClassData::CheckAccessControl() const
{
   Debug::ft("ClassData.CheckAccessControl");

   CxxScope::CheckAccessControl();

   //  This also logs data that isn't private, unless
   //  o it is static and const
   //  o it is declared in a .cpp
   //  o it belongs to a struct or union
   //
   if(IsStatic() && IsConst()) return;
   if(GetAccess() == Cxx::Private) return;
   if(GetFile()->IsCpp()) return;
   if(GetClass()->GetClassTag() != Cxx::ClassType) return;
   if(depth_ > 0) return;
   Log(DataNotPrivate);
}

//------------------------------------------------------------------------------

void ClassData::CheckIfInitialized() const
{
   Debug::ft("ClassData.CheckIfInitialized");

   //  Static data should be initialized.
   //
   if(!WasInited() && IsStatic()) Log(DataUninitialized);
}

//------------------------------------------------------------------------------

void ClassData::CheckIfMutated() const
{
   Debug::ft("ClassData.CheckIfMutated");

   if(mutable_ && !mutated_) Log(DataNeedNotBeMutable);
}

//------------------------------------------------------------------------------

void ClassData::CheckIfRelocatable() const
{
   Debug::ft("ClassData.CheckIfRelocatable");

   //  Static data that is only referenced within the .cpp that initializes
   //  it can be moved out of the class and into the .cpp.
   //
   if(IsStatic())
   {
      if(GetDeclFile()->IsCpp()) return;
      auto file = GetDefnFile();
      if(file == nullptr) return;
      if(GetClass()->IsTemplate()) return;

      auto xref = Xref();

      for(auto r = xref->cbegin(); r != xref->cend(); ++r)
      {
         if((*r)->GetFile() != file) return;
      }

      Log(DataCouldBeFree);
   }
}

//------------------------------------------------------------------------------

void ClassData::Delete()
{
   Debug::ftnt("ClassData.Delete");

   ClearMate();
   GetArea()->EraseData(this);
   delete this;
}

//------------------------------------------------------------------------------

void ClassData::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   auto fq = options.test(DispFQ);
   auto access = GetAccess();

   if(depth_ > 0)
   {
      //  This member was promoted from an anonymous union to an outer class.
      //  The first member in the union recreates the union declaration; its
      //  access control is actually that of the union.  Indent each member
      //  based on depth_, which supports the nesting of anonymous unions.
      //  Each member is assumed to be public to its enclosing classes.
      //
      if(first_)
      {
         auto lead = spaces(IndentSize() * (depth_ - 1));
         stream << prefix << lead << access << ": " << UNION_STR << CRLF;
         stream << prefix << lead << '{' << CRLF;
      }

      stream << spaces(IndentSize() * depth_);
      access = Cxx::Public;
   }

   stream << prefix << access << ": ";
   DisplayAlignment(stream, options);
   if(IsStatic()) stream << STATIC_STR << SPACE;
   if(IsThreadLocal()) stream << THREAD_LOCAL_STR << SPACE;
   if(IsConstexpr()) stream << CONSTEXPR_STR << SPACE;
   if(mutable_) stream << MUTABLE_STR << SPACE;
   GetTypeSpec()->Print(stream, options);
   stream << SPACE << (fq ? ScopedName(true) : name_);
   GetTypeSpec()->DisplayArrays(stream);

   if(width_ != nullptr)
   {
      stream << " : ";
      width_->Print(stream, options);
   }

   DisplayAssignment(stream, options);
   stream << ';';

   if(!options.test(DispCode))
   {
      std::ostringstream buff;
      buff << " // ";
      if(!WasInited() && IsStatic())
      {
         buff << "<@";
         if(!options.test(DispStats)) buff << "uninit ";
      }
      DisplayStats(buff, options);
      if(!fq) DisplayFiles(buff);
      auto str = buff.str();
      if(str.size() > 4) stream << str;
   }

   stream << CRLF;

   if(last_)
   {
      stream << prefix << spaces(IndentSize() * (depth_ - 1)) << "};" << CRLF;
   }
}

//------------------------------------------------------------------------------

fn_name ClassData_EnterBlock = "ClassData.EnterBlock";

void ClassData::EnterBlock()
{
   Debug::ft(ClassData_EnterBlock);

   //  The initialization of a static member is handled by
   //  o ClassData.EnterScope, if initialized where declared, or
   //  o SpaceData.EnterScope, if initialized separately.
   //
   if(IsStatic())
   {
      string expl("Improper initialization of static member ");
      expl += ScopedName(true);
      Context::SwLog(ClassData_EnterBlock, expl, 0);
      return;
   }

   //  If there is a member initialization statement, compile it and then
   //  clear it: there could be more than one constructor, each with its
   //  own version of the member initialization.  If there is no member
   //  initialization statement, see if a class member is using a default
   //  constructor.
   //
   if(memInit_ != nullptr)
   {
      Context::SetPos(memInit_->GetLoc());
      InitByExpr(memInit_->GetInit());
      memInit_ = nullptr;
      return;
   }

   InitByDefault();
}

//------------------------------------------------------------------------------

bool ClassData::EnterScope()
{
   Debug::ft("ClassData.EnterScope");

   //  When class data is declared, its type and field with are known.
   //  A static const POD member (unless it's a pointer) could also be
   //  initialized at this point.
   //
   Context::SetPos(GetLoc());
   ExecuteAlignment();
   GetTypeSpec()->EnteringScope(this);

   if(width_ != nullptr)
   {
      width_->EnterBlock();
      auto result = Context::PopArg(true);
      auto numeric = result.NumericType();

      if(numeric.Type() != Numeric::INT)
      {
         auto expl = "Non-numeric value for field width";
         Context::SwLog(ClassData_EnterBlock, expl, numeric.Type());
      }
   }

   //  Presumably we're dealing with well-formed code.  We could therefore
   //  remove these checks and just invoke ExecuteInit directly.  However,
   //  they are included because they help to verify that *this* software
   //  is correct.  The same is true for most of the checks on type
   //  restrictions or type compatibility, such as the one above for field
   //  width.
   //
   if(IsStatic() && IsConst() && IsPOD() && !IsPointer(true))
   {
      ExecuteInit(false);
   }

   CloseScope();
   return true;
}

//------------------------------------------------------------------------------

void ClassData::GetDecls(CxxNamedSet& items)
{
   if(IsDecl()) items.insert(this);
}

//------------------------------------------------------------------------------

void ClassData::GetUsages(const CodeFile& file, CxxUsageSets& symbols)
{
   Data::GetUsages(file, symbols);

   if(width_ != nullptr) width_->GetUsages(file, symbols);
}

//------------------------------------------------------------------------------

bool ClassData::IsUnionMember() const
{
   Debug::ft("ClassData.IsUnionMember");

   //  Look for an anonymous union as well as a named union.
   //
   if(depth_ > 0) return true;

   auto scope = GetScope();

   if(scope->Type() == Cxx::Class)
   {
      auto cls = static_cast<const Class*>(scope);
      return (cls->GetClassTag() == Cxx::UnionType);
   }

   return false;
}

//------------------------------------------------------------------------------

StackArg ClassData::MemberToArg(StackArg& via, TypeName* name, Cxx::Operator op)
{
   Debug::ft("ClassData.MemberToArg");

   //  Create an argument for this member, which was accessed through VIA.
   //
   Accessed(&via);
   StackArg arg(this, name, via, op);
   if(mutable_) arg.SetAsMutable();
   return arg;
}

//------------------------------------------------------------------------------

StackArg ClassData::NameToArg(Cxx::Operator op, TypeName* name)
{
   Debug::ft("ClassData.NameToArg");

   //  Create an argument, marking it as a member of the context class and
   //  noting if it is mutable.  Log a read on the implicit "this", and if
   //  the context function is const, also mark the item as const.
   //
   auto arg = Data::NameToArg(op, name);
   if(IsStatic()) return arg;
   arg.SetAsMember();
   if(mutable_) arg.SetAsMutable();
   auto func = Context::Scope()->GetFunction();
   if(func == nullptr) return arg;
   func->IncrThisReads();
   if(!IsInitializing() && func->IsConst()) arg.SetAsReadOnly();
   return arg;
}

//------------------------------------------------------------------------------

CxxToken* ClassData::PosToItem(size_t pos) const
{
   auto item = Data::PosToItem(pos);
   if(item != nullptr) return item;

   return (width_ != nullptr ? width_->PosToItem(pos) : nullptr);
}

//------------------------------------------------------------------------------

void ClassData::Promote(Class* cls, Cxx::Access access, bool first, bool last)
{
   Debug::ft("ClassData.Promote");

   //  Update our scope and access control.  To support nested anonymous unions,
   //  don't overwrite first_ and last_.  Members of a nested anonymous union
   //  first move into their outer class (another anonymous union), then to the
   //  next outer class, and so on.
   //
   SetScope(cls);
   SetAccess(access);
   if(first) first_ = true;
   if(last) last_ = true;
   ++depth_;
}

//------------------------------------------------------------------------------

void ClassData::SetMemInit(const MemberInit* init)
{
   memInit_ = init;
   IncrWrites();
}

//------------------------------------------------------------------------------

void ClassData::Shrink()
{
   Data::Shrink();
   name_.shrink_to_fit();
   CxxStats::Strings(CxxStats::CLASS_DATA, name_.capacity());
   CxxStats::Vectors(CxxStats::CLASS_DATA, XrefSize());
   if(width_ != nullptr) width_->Shrink();
}

//------------------------------------------------------------------------------

void ClassData::UpdatePos
   (EditorAction action, size_t begin, size_t count, size_t from) const
{
   Data::UpdatePos(action, begin, count, from);
   if(width_ != nullptr) width_->UpdatePos(action, begin, count, from);
}

//------------------------------------------------------------------------------

void ClassData::UpdateXref(bool insert)
{
   Data::UpdateXref(insert);

   if(width_ != nullptr) width_->UpdateXref(insert);
}

//------------------------------------------------------------------------------

void ClassData::WasMutated(const StackArg* arg)
{
   Debug::ft("ClassData.WasMutated");

   Data::SetNonConst();

   //  A StackArg inherits its mutable_ attribute from arg.via_, so this
   //  function can be invoked on data that is not tagged mutable itself.
   //
   if(!mutable_) return;

   //  This item is using its mutability if it is currently const.
   //
   if(arg->item_ == this)
   {
      if(arg->IsConst()) mutated_ = true;
      return;
   }

   //  This item is actually arg.via_, in which case this function is only
   //  invoked when the item is using its mutability.
   //
   mutated_ = true;
}

//------------------------------------------------------------------------------

bool ClassData::WasWritten(const StackArg* arg, bool direct, bool indirect)
{
   Debug::ft("ClassData.WasWritten");

   auto result = Data::WasWritten(arg, direct, indirect);

   //  Check if mutable data just made use of its mutability.
   //
   if(mutable_ && direct && arg->IsReadOnly())
   {
      mutated_ = true;
   }

   return result;
}

//==============================================================================

CxxScope::CxxScope() : pushes_(0)
{
   Debug::ft("CxxScope.ctor");
}

//------------------------------------------------------------------------------

CxxScope::~CxxScope()
{
   Debug::ftnt("CxxScope.dtor");

   CloseScope();
}

//------------------------------------------------------------------------------

void CxxScope::AccessibilityOf
   (const CxxScope* scope, const CxxScoped* item, SymbolView& view) const
{
   Debug::ft("CxxScope.AccessibilityOf");

   view.distance_ = scope->ScopeDistance(this);
   view.accessibility_ =
      (view.distance_ == NOT_A_SUBSCOPE ? Inaccessible : Unrestricted);
}

//------------------------------------------------------------------------------

void CxxScope::CloseScope()
{
   Debug::ftnt("CxxScope.CloseScope");

   for(NO_OP; pushes_ > 0; --pushes_) Context::PopScope();
}

//------------------------------------------------------------------------------

CodeFile* CxxScope::GetDistinctDeclFile() const
{
   auto defn = GetDefnFile();

   if(defn != nullptr)
   {
      auto decl = GetDeclFile();
      if(decl != defn) return decl;
   }

   return nullptr;
}

//------------------------------------------------------------------------------

TemplateParm* CxxScope::NameToTemplateParm(const string& name) const
{
   Debug::ft("CxxScope.NameToTemplateParm");

   auto scope = this;

   while(scope != nullptr)
   {
      auto tmplt = scope->GetTemplateParms();

      if(tmplt != nullptr)
      {
         auto parms = tmplt->Parms();

         for(auto p = parms->cbegin(); p != parms->cend(); ++p)
         {
            if((*p)->Name() == name) return p->get();
         }
      }

      scope = scope->GetScope();
   }

   return nullptr;
}

//------------------------------------------------------------------------------

fn_name CxxScope_OpenScope = "CxxScope.OpenScope";

void CxxScope::OpenScope(string& name)
{
   Debug::ft(CxxScope_OpenScope);

   //  This is invoked when parsing functions and data, whether declarations
   //  or definitions.  If NAME is qualified, this is a definition, and the
   //  qualifier (a namespace or class) should be pushed as a scope first.
   //  After that, the item itself is pushed as a scope.  Do not apply this to
   //  template instances, which may contain qualified names but whose members
   //  do not have separate declarations and definitions.
   //
   auto scope = Context::Scope();

   if(!scope->IsInTemplateInstance())
   {
      auto pos = name.rfind(SCOPE_STR, name.find('<'));

      if(pos != string::npos)
      {
         //  POS is the last scope resolution operator before any template.
         //  Whatever precedes it qualifies NAME and should be a known scope
         //  within SCOPE.
         //
         name.erase(pos);
         scope = Singleton< CxxSymbols >::Instance()->FindScope(scope, name);

         if(scope != nullptr)
         {
            Context::PushScope(scope, false);
            pushes_++;
         }
         else
         {
            auto expl = "Could not find scope " + name;
            Context::SwLog(CxxScope_OpenScope, expl, 0);
            scope = Context::Scope();
         }
      }
   }

   SetScope(scope);
   Context::PushScope(this, false);
   pushes_++;
}

//------------------------------------------------------------------------------

void CxxScope::ReplaceTemplateParms
   (string& code, const TypeSpecPtrVector* args, size_t begin) const
{
   Debug::ft("CxxScope.ReplaceTemplateParms");

   //  Replace the template parameters with the instance arguments.
   //
   auto tmpltParms = GetTemplateParms()->Parms();
   auto tmpltSpec = GetQualName()->GetTemplateArgs();
   auto tmpltArgs = (tmpltSpec != nullptr ? tmpltSpec->Args() : nullptr);
   string argName;

   for(size_t i = 0; i < tmpltParms->size(); ++i)
   {
      auto& parmName = tmpltParms->at(i)->Name();

      //  If the instance arguments ran out, use template parameter defaults.
      //
      if(i < args->size())
      {
         if(tmpltArgs != nullptr)
            argName = tmpltArgs->at(i)->AlignTemplateArg(args->at(i).get());
         else
            argName = args->at(i)->TypeString(true);
      }
      else
      {
         argName = tmpltParms->at(i)->Default()->TypeString(true);
      }

      RemoveRefs(argName);

      //  If an instance argument is a pointer type, modify its template
      //  parameter so that constness is applied to the pointer instead of
      //  the type, and so that the pointer is not treated as a reference:
      //  o const T* becomes T* const
      //  o const T& becomes T const
      //  o const T  becomes T const
      //
      if(argName.back() == '*')
      {
         auto constParm = "const " + parmName + '*';
         auto parmConst = parmName + '*' + " const";
         Replace(code, constParm, parmConst, begin, string::npos);
         constParm.back() = '&';
         parmConst = parmName + " const";
         Replace(code, constParm, parmConst, begin, string::npos);
         constParm.pop_back();
         Replace(code, constParm, parmConst, begin, string::npos);
      }

      //  Replace this template parameter with the template argument.
      //
      Replace(code, parmName, argName, begin, string::npos);

      //  Replace occurrences of "const const" with "const", which can occur
      //  when both a template parameter and template argument are const.
      //
      Replace(code, "const const", CONST_STR, 0, string::npos);
   }
}

//------------------------------------------------------------------------------

Distance CxxScope::ScopeDistance(const CxxScope* scope) const
{
   Debug::ft("CxxScope.ScopeDistance");

   Distance dist = 0;

   for(auto curr = this; curr != nullptr; curr = curr->GetScope())
   {
      if(curr == scope) return dist;
      ++dist;
   }

   return NOT_A_SUBSCOPE;
}

//==============================================================================

Data::Data(TypeSpecPtr& spec) :
   extern_(false),
   static_(false),
   thread_local_(false),
   constexpr_(false),
   inited_(false),
   initing_(false),
   nonconst_(false),
   nonconstptr_(false),
   defn_(false),
   mate_(nullptr),
   spec_(spec.release()),
   reads_(0),
   writes_(0)
{
   Debug::ft("Data.ctor");
}

//------------------------------------------------------------------------------

Data::~Data()
{
   Debug::ftnt("Data.dtor");
}

//------------------------------------------------------------------------------

void Data::Check() const
{
   Debug::ft("Data.Check");

   if(alignas_ != nullptr) alignas_->Check();
   spec_->Check();
   if(expr_ != nullptr) expr_->Check();
   if(init_ != nullptr) init_->Check();

   if(!defn_ && (mate_ != nullptr)) mate_->Check();
}

//------------------------------------------------------------------------------

void Data::CheckConstness(bool could) const
{
   Debug::ft("Data.CheckConstness");

   if(reads_ > 0)
   {
      if(!IsConst())
      {
         if(!nonconst_ && could) Log(DataCouldBeConst);
      }
      else
      {
         if(nonconst_) Log(DataCannotBeConst);
      }

      if(!IsConstPtr())
      {
         if(!nonconstptr_ && could)
         {
            //  Only log this for pointers, not arrays.
            //
            if(spec_->Ptrs(false) > 0) Log(DataCouldBeConstPtr);
         }
      }
      else
      {
         if(nonconstptr_) Log(DataCannotBeConstPtr);
      }
   }
}

//------------------------------------------------------------------------------

void Data::CheckUsage() const
{
   Debug::ft("Data.CheckUsage");

   if(reads_ == 0)
   {
      if(writes_ > 0)
         Log(DataWriteOnly);
      else if(WasInited() && !IsConst())
         Log(DataInitOnly);
      else Log(DataUnused);
   }
}

//------------------------------------------------------------------------------

void Data::ClearMate() const
{
   Debug::ft("Data.ClearMate");

   if(mate_ != nullptr) mate_->mate_ = nullptr;
}

//------------------------------------------------------------------------------

void Data::DisplayAlignment(ostream& stream, const Flags& options) const
{
   if(alignas_ != nullptr)
   {
      alignas_->Print(stream, options);
      stream << SPACE;
   }
}

//------------------------------------------------------------------------------

void Data::DisplayAssignment(ostream& stream, const Flags& options) const
{
   //  Always display an assignment in namespace view.  In file view,
   //  only display it where it occurs.
   //
   auto ns = options.test(DispNS);
   if(!ns && (init_ == nullptr)) return;

   auto init = GetDefn()->init_.get();
   if(init == nullptr) return;

   //  The source code only contains the assignment operator and the
   //  initialization expression.
   //
   std::ostringstream buffer;

   stream << " = ";
   init->Back()->Print(buffer, options);

   auto expr = buffer.str();

   if(expr.size() <= LineLengthMax())
      stream << expr;
   else
      stream << "{ /*" << expr.size() << " characters */ }";
}

//------------------------------------------------------------------------------

void Data::DisplayExpression(ostream& stream, const Flags& options) const
{
   //  Always display an expression in namespace view.  In file view,
   //  only display it where it occurs.
   //
   auto ns = options.test(DispNS);
   if(!ns && (expr_ == nullptr)) return;

   auto expr = GetDefn()->expr_.get();
   if(expr == nullptr) return;

   expr->Print(stream, options);
}

//------------------------------------------------------------------------------

void Data::DisplayStats(ostream& stream, const Flags& options) const
{
   if(!options.test(DispStats)) return;

   auto decl = GetDecl();
   stream << "i=" << decl->inited_ << SPACE;
   stream << "r=" << decl->reads_ << SPACE;
   stream << "w=" << decl->writes_ << SPACE;
}

//------------------------------------------------------------------------------

void Data::ExecuteAlignment() const
{
   Debug::ft("Data.ExecuteAlignment");

   if(alignas_ != nullptr) alignas_->EnterBlock();
}

//------------------------------------------------------------------------------

bool Data::ExecuteInit(bool push)
{
   Debug::ft("Data.ExecuteInit");

   if(push)
   {
      Context::Enter(this);
      Context::PushScope(this, true);
   }

   //  If some form of initialization exists, one of the following will
   //  set inited_ and return true; thus the empty statement.
   //
   auto decl = GetDecl();
   decl->initing_ = true;
   if(InitByExpr(expr_.get()) || InitByAssign() || InitByDefault()) { }
   decl->initing_ = false;
   if(push) Context::PopScope();
   return decl->inited_;
}

//------------------------------------------------------------------------------

CodeFile* Data::GetDeclFile() const
{
   return (defn_ ? mate_->GetFile() : GetFile());
}

//------------------------------------------------------------------------------

const Data* Data::GetDefn() const
{
   if(defn_) return this;
   if(mate_ != nullptr) return mate_;
   return this;
}

//------------------------------------------------------------------------------

CodeFile* Data::GetDefnFile() const
{
   if(defn_) return GetFile();
   if(mate_ != nullptr) return mate_->GetFile();
   return nullptr;
}

//------------------------------------------------------------------------------

void Data::GetInitName(QualNamePtr& qualName) const
{
   Debug::ft("Data.GetInitName");

   qualName.reset(new QualName(Name()));
}

//------------------------------------------------------------------------------

bool Data::GetSpan(size_t& begin, size_t& left, size_t& end) const
{
   Debug::ft("Data.GetSpan");

   return GetTypeSpan(begin, end);
}

//------------------------------------------------------------------------------

bool Data::GetStrValue(string& str) const
{
   Debug::ft("Data.GetStrValue");

   //  In order to return a string, the data must have an initialization
   //  statement.  Display the statement and look for the quotation marks
   //  that denote a string literal.  Strip off everything outside the
   //  quotes to generate STR.
   //
   if(init_ == nullptr) return false;

   std::ostringstream stream;
   init_->Print(stream, NoFlags);
   str = stream.str();
   auto quote = str.find(QUOTE);
   if(quote == string::npos) return false;
   str.erase(0, quote + 1);
   quote = str.find(QUOTE);
   if(quote == string::npos) return false;
   str.erase(quote);
   return true;
}

//------------------------------------------------------------------------------

TypeName* Data::GetTemplateArgs() const
{
   return spec_->GetTemplateArgs();
}

//------------------------------------------------------------------------------

void Data::GetUsages(const CodeFile& file, CxxUsageSets& symbols)
{
   if(alignas_ != nullptr) alignas_->GetUsages(file, symbols);
   spec_->GetUsages(file, symbols);
   if(expr_ != nullptr) expr_->GetUsages(file, symbols);
   if(init_ != nullptr) init_->GetUsages(file, symbols);
}

//------------------------------------------------------------------------------

bool Data::InitByAssign()
{
   Debug::ft("Data.InitByAssign");

   if(init_ == nullptr) return false;

   auto cls = DirectClass();
   if(cls != nullptr) cls->Creating();

   init_->EnterBlock();
   auto result = Context::PopArg(true);
   spec_->MustMatchWith(result);
   SetInited();

   if(result.WasConstructed() && (result.Ptrs(true) == 0) &&
      (GetScope()->GetFunction() != nullptr))
   {
      Log(InitCouldUseConstructor);
   }

   return true;
}

//------------------------------------------------------------------------------

bool Data::InitByDefault()
{
   Debug::ft("Data.InitByDefault");

   auto cls = DirectClass();
   if(cls == nullptr) return false;
   cls->Creating();
   auto ctor = cls->FindCtor(nullptr);

   if(ctor != nullptr)
   {
      SymbolView view;
      cls->AccessibilityOf(Context::Scope(), ctor, view);
      ctor->RecordAccess(view.control_);
      ctor->WasCalled();
      SetInited();
   }
   else
   {
      cls->WasCalled(PureCtor, this);
      if(!cls->HasPODMember()) SetInited();
   }

   return GetDecl()->inited_;
}

//------------------------------------------------------------------------------

fn_name Data_InitByExpr = "Data.InitByExpr";

bool Data::InitByExpr(CxxToken* expr)
{
   Debug::ft(Data_InitByExpr);

   //  The following handles a definition of the form
   //    <TypeSpec> <name>(<Expr>);
   //  which initializes dataName using an expression.
   //  It also handles an item in a constructor's member initialization list:
   //    <name>(<Expr>),
   //  whether it is initializing a class or POD member.
   //
   if(expr == nullptr) return false;

   auto cls = DirectClass();

   if(cls != nullptr)
   {
      cls->Creating();

      //  Push CLS as the constructor name that will handle expr_, which is
      //  a FUNCTION_CALL Operation that contains an argument list but which
      //  is missing the antecedent, the class's name.  The constructor also
      //  requires a "this" argument.
      //
      Context::PushArg(StackArg(cls, 0, false));
      Context::TopArg()->SetInvoke();
      Context::PushArg(StackArg(cls, 1, false));
      Context::TopArg()->SetAsThis(true);
      Expression::Start();
      expr->EnterBlock();
      Context::Execute();
      Context::Clear(2);
   }
   else
   {
      //  ROOT is not a class, so EXPR should contain a single expression.
      //  Compile it as if it was a single-member brace initialization list.
      //
      if(expr->Type() == Cxx::Operation)
      {
         auto op = static_cast< Operation* >(expr);

         if(op->ArgsSize() == 1)
         {
            op->FrontArg()->EnterBlock();
            auto result = Context::PopArg(true);
            spec_->MustMatchWith(result);
            result.AssignedTo(StackArg(this, 0, false), Copied);
         }
         else
         {
            auto expl = "Invalid arguments for " + spec_->Name();
            Context::SwLog(Data_InitByExpr, expl, op->ArgsSize());
         }
      }
      else
      {
         auto expl = "Invalid expression for " + spec_->Name();
         Context::SwLog(Data_InitByExpr, expl, expr->Type());
      }
   }

   SetInited();
   return true;
}

//------------------------------------------------------------------------------

bool Data::IsConst() const
{
   if(constexpr_) return true;
   return spec_->IsConst();
}

//------------------------------------------------------------------------------

bool Data::IsDefaultConstructible() const
{
   Debug::ft("Data.IsDefaultConstructible");

   if(static_) return true;

   auto type = spec_->TypeString(false);
   if(type.find(ARRAY_STR) != string::npos) return true;

   auto cls = DirectClass();
   if(cls == nullptr) return false;
   return cls->IsDefaultConstructible();
}

//------------------------------------------------------------------------------

StackArg Data::NameToArg(Cxx::Operator op, TypeName* name)
{
   Debug::ft("Data.NameToArg");

   //  Make data writeable during its initialization.
   //
   auto arg = CxxNamed::NameToArg(op, name);
   if(initing_) arg.SetAsWriteable();
   return arg;
}

//------------------------------------------------------------------------------

CxxToken* Data::PosToItem(size_t pos) const
{
   auto item = CxxScope::PosToItem(pos);
   if(item != nullptr) return item;

   if(alignas_ != nullptr) item = alignas_->PosToItem(pos);
   if(item != nullptr) return item;

   item = spec_->PosToItem(pos);
   if(item != nullptr) return item;

   if(expr_ != nullptr) item = expr_->PosToItem(pos);
   if(item != nullptr) return item;

   return (init_ != nullptr ? init_->PosToItem(pos) : nullptr);
}

//------------------------------------------------------------------------------

void Data::SetAlignment(AlignAsPtr& align)
{
   Debug::ft("Data.SetAlignment");

   alignas_ = std::move(align);
}

//------------------------------------------------------------------------------

void Data::SetAssignment(ExprPtr& expr, size_t eqpos)
{
   Debug::ft("Data.SetAssignment");

   //  Create an assignment expression in which the name of this data item
   //  is the first argument and EXPR is the second argument.
   //
   if(expr == nullptr) return;
   init_.reset(new Expression(expr->EndPos(), true));

   QualNamePtr name;
   GetInitName(name);
   name->CopyContext(this, false);
   TokenPtr arg1(name.release());
   init_->AddItem(arg1);
   TokenPtr op(new Operation(Cxx::ASSIGN));
   op->SetContext(eqpos);
   init_->AddItem(op);
   TokenPtr arg2(expr.release());
   init_->AddItem(arg2);
}

//------------------------------------------------------------------------------

void Data::SetDefn(Data* data)
{
   Debug::ft("Data.SetDefn");

   data->mate_ = this;
   data->defn_ = true;
   this->mate_ = data;
}

//------------------------------------------------------------------------------

void Data::SetInited()
{
   Debug::ft("Data.SetInited");

   GetDecl()->inited_ = true;

   auto item = static_cast< Data* >(FindTemplateAnalog(this));
   if(item != nullptr) item->SetInited();
}

//------------------------------------------------------------------------------

bool Data::SetNonConst()
{
   Debug::ft("Data.SetNonConst");

   if(initing_) return true;
   if(nonconst_) return true;

   nonconst_ = true;
   auto item = static_cast< Data* >(FindTemplateAnalog(this));
   if(item != nullptr) item->nonconst_ = true;

   return !IsConst();
}

//------------------------------------------------------------------------------

void Data::Shrink()
{
   CxxScope::Shrink();
   if(alignas_ != nullptr) alignas_->Shrink();
   spec_->Shrink();
   if(expr_ != nullptr) expr_->Shrink();
   if(init_ != nullptr) init_->Shrink();
}

//------------------------------------------------------------------------------

string Data::TypeString(bool arg) const
{
   return spec_->TypeString(arg);
}

//------------------------------------------------------------------------------

void Data::UpdatePos
   (EditorAction action, size_t begin, size_t count, size_t from) const
{
   CxxScope::UpdatePos(action, begin, count, from);
   if(alignas_ != nullptr) alignas_->UpdatePos(action, begin, count, from);
   spec_->UpdatePos(action, begin, count, from);
   if(expr_ != nullptr) expr_->UpdatePos(action, begin, count, from);
   if(init_ != nullptr) init_->UpdatePos(action, begin, count, from);
}

//------------------------------------------------------------------------------

void Data::UpdateXref(bool insert)
{
   if(alignas_ != nullptr) alignas_->UpdateXref(insert);
   spec_->UpdateXref(insert);
   if(expr_ != nullptr) expr_->UpdateXref(insert);
   if(init_ != nullptr) init_->UpdateXref(insert);
}

//------------------------------------------------------------------------------

bool Data::WasRead()
{
   if(initing_) return false;
   ++reads_;
   auto item = static_cast< Data* >(FindTemplateAnalog(this));
   if(item != nullptr) ++item->reads_;
   return true;
}

//------------------------------------------------------------------------------

fn_name Data_WasWritten = "Data.WasWritten";

bool Data::WasWritten(const StackArg* arg, bool direct, bool indirect)
{
   Debug::ft(Data_WasWritten);

   if(initing_) return false;
   ++writes_;
   auto item = static_cast< Data* >(FindTemplateAnalog(this));
   if(item != nullptr) ++item->writes_;

   auto ptrs = (arg->item_ == this ? arg->Ptrs(true) : spec_->Ptrs(true));

   if(ptrs == 0)
   {
      if(direct)
      {
         nonconst_ = true;
         if(item != nullptr) item->nonconst_ = true;
      }

      if(indirect)
      {
         string expl("Indirection through ");
         expl += arg->item_->Name();

         Context::SwLog(Data_WasWritten, expl, 0);
      }
   }
   else
   {
      if(direct)
      {
         nonconstptr_ = true;
         if(item != nullptr) item->nonconstptr_ = true;
      }

      if(indirect)
      {
         nonconst_ = true;
         if(item != nullptr) item->nonconst_ = true;
      }
   }

   return true;
}

//==============================================================================

FuncData::FuncData(string& name, TypeSpecPtr& type) : Data(type),
   first_(this)
{
   Debug::ft("FuncData.ctor");

   std::swap(name_, name);
   CxxStats::Incr(CxxStats::FUNC_DATA);
}

//------------------------------------------------------------------------------

FuncData::~FuncData()
{
   Debug::ftnt("FuncData.dtor");

   CxxStats::Decr(CxxStats::FUNC_DATA);
}

//------------------------------------------------------------------------------

void FuncData::Check() const
{
   Debug::ft("FuncData.Check");

   //  Don't check a function's internal variables for potential constness.
   //
   Data::Check();

   if(next_ != nullptr) next_->Check();

   CheckUsage();
   CheckConstness(false);
}

//------------------------------------------------------------------------------

void FuncData::Delete()
{
   Debug::ftnt("FuncData.Delete");

   if(first_ == this)
   {
      if(next_ == nullptr)
      {
         //  Delete this item, which appears alone.
         //
         static_cast< Block* >(GetScope())->EraseItem(this);
      }
      else
      {
         //  The item being deleted is the first in a series declaration.  The
         //  next item becomes the first in the series.  Its TypeSpec, which was
         //  cloned from this item, becomes the one for the series declaration,
         //  and it becomes the first item in the data declaration statement.
         //
         for(auto d = first_->next_.get(); d != nullptr; d = d->next_.get())
         {
            d->first_ = next_.get();
         }

         auto spec = next_->GetTypeSpec();
         spec->SetLoc(GetFile(), GetTypeSpec()->GetPos(), false);
         static_cast< Block* >(GetScope())->ReplaceItem(this, next_.release());
      }
   }
   else
   {
      //  Extract the item from the middle of a series declaration.
      //
      for(auto d = first_; d != nullptr; d = d->next_.get())
      {
         if(d->next_.get() == this)
         {
            d->next_.release();
            d->next_ = std::move(next_);
            break;
         }
      }
   }

   delete this;
}

//------------------------------------------------------------------------------

void FuncData::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   stream << prefix;
   DisplayItem(stream, options);
   stream << CRLF;
}

//------------------------------------------------------------------------------

void FuncData::DisplayItem(ostream& stream, const Flags& options) const
{
   if(first_ == this)
   {
      DisplayAlignment(stream, options);
      if(IsExtern()) stream << EXTERN_STR << SPACE;
      if(IsStatic()) stream << STATIC_STR << SPACE;
      if(IsThreadLocal()) stream << THREAD_LOCAL_STR << SPACE;
      if(IsConstexpr()) stream << CONSTEXPR_STR << SPACE;
      GetTypeSpec()->Print(stream, options);
      stream << SPACE;
   }
   else
   {
      GetTypeSpec()->DisplayTags(stream);
   }

   stream << name_;
   GetTypeSpec()->DisplayArrays(stream);
   DisplayExpression(stream, options);
   DisplayAssignment(stream, options);

   if(next_ == nullptr)
   {
      stream << ';';

      std::ostringstream buff;
      buff << " // ";
      DisplayStats(buff, options);
      auto str = buff.str();
      if(str.size() > 4) stream << str;
   }
   else
   {
      stream << ", ";
      next_->Print(stream, options);
   }
}

//------------------------------------------------------------------------------

void FuncData::EnterBlock()
{
   Debug::ft("FuncData.EnterBlock");

   //  This also doubles as the equivalent of EnterScope for function data.
   //  Set the data's scope, add it to the local symbol table, and compile
   //  its definition.
   //
   auto spec = GetTypeSpec();
   auto anon = spec->IsAuto();

   Context::SetPos(GetLoc());
   Context::InsertLocal(this);
   ExecuteAlignment();
   spec->EnteringScope(this);
   ExecuteInit(false);

   //  If this statement contains multiple declarations, continue with the
   //  next one.
   //
   if(next_ != nullptr)
   {
      if(anon) StackArg::SetAutoTypeFor(*next_);
      next_->EnterBlock();
   }
}

//------------------------------------------------------------------------------

void FuncData::ExitBlock() const
{
   Debug::ft("FuncData.ExitBlock");

   Context::EraseLocal(this);

   auto cls = DirectClass();
   if(cls != nullptr) cls->WasCalled(PureDtor, this);
   if(next_ != nullptr) next_->ExitBlock();
}

//------------------------------------------------------------------------------

bool FuncData::GetSpan(size_t& begin, size_t& left, size_t& end) const
{
   Debug::ft("FuncData.GetSpan");

   if((first_ == this) && (next_ == nullptr))
   {
      //  Cut the entire data item.
      //
      return GetTypeSpan(begin, end);
   }

   auto& lexer = GetFile()->GetLexer();
   auto pos = GetPos();

   if(first_ == this)
   {
      //  For a data item, GetPos() is the position of its name, so it
      //  excludes the type.  Cut from the name to the following comma.
      //
      begin = pos;
      end = lexer.FindFirstOf(",", pos);
   }
   else
   {
      //  Cut from the preceding comma to the position before the next
      //  comma or semicolon.
      //
      begin = lexer.RfindFirstOf(pos, ",");
      end = lexer.FindFirstOf(",;", pos) - 1;
   }

   return true;
}

//------------------------------------------------------------------------------

void FuncData::GetUsages(const CodeFile& file, CxxUsageSets& symbols)
{
   Data::GetUsages(file, symbols);

   if(next_ != nullptr) next_->GetUsages(file, symbols);
}

//------------------------------------------------------------------------------

CxxToken* FuncData::PosToItem(size_t pos) const
{
   auto item = Data::PosToItem(pos);
   if(item != nullptr) return item;

   return (next_ != nullptr ? next_->PosToItem(pos) : nullptr);
}

//------------------------------------------------------------------------------

void FuncData::Print(ostream& stream, const Flags& options) const
{
   DisplayItem(stream, options);
}

//------------------------------------------------------------------------------

void FuncData::SetNext(FuncDataPtr& next)
{
   Debug::ft("FuncData.SetNext");

   next_.reset(next.release());
   next_->SetFirst(first_);
}

//------------------------------------------------------------------------------

void FuncData::Shrink()
{
   Data::Shrink();
   name_.shrink_to_fit();
   CxxStats::Strings(CxxStats::FUNC_DATA, name_.capacity());
   CxxStats::Vectors(CxxStats::FUNC_DATA, XrefSize());
   if(next_ != nullptr) next_->Shrink();
}

//------------------------------------------------------------------------------

void FuncData::UpdatePos
   (EditorAction action, size_t begin, size_t count, size_t from) const
{
   Data::UpdatePos(action, begin, count, from);
   if(next_ != nullptr) next_->UpdatePos(action, begin, count, from);
}

//------------------------------------------------------------------------------

void FuncData::UpdateXref(bool insert)
{
   Data::UpdateXref(insert);

   if(next_ != nullptr) next_->UpdateXref(insert);
}

//==============================================================================

Function::Function(QualNamePtr& name) :
   name_(name.release()),
   tspec_(nullptr),
   extern_(false),
   inline_(false),
   constexpr_(false),
   static_(false),
   virtual_(false),
   explicit_(false),
   const_(false),
   volatile_(false),
   noexcept_(false),
   override_(false),
   final_(false),
   pure_(false),
   type_(false),
   friend_(false),
   found_(false),
   this_(false),
   tparm_(false),
   nonpublic_(false),
   nonstatic_(false),
   implicit_(false),
   defn_(false),
   deleted_(false),
   defaulted_(false),
   calls_(0),
   mate_(nullptr),
   pos_(string::npos),
   base_(nullptr),
   tmplt_(nullptr)
{
   Debug::ft("Function.ctor");

   Singleton< CxxSymbols >::Instance()->InsertFunc(this);

   auto qname = name_->QualifiedName(true, false);
   OpenScope(qname);
   CxxStats::Incr(CxxStats::FUNCTION);
}

//------------------------------------------------------------------------------

Function::Function(QualNamePtr& name, TypeSpecPtr& spec, bool type) :
   name_(name.release()),
   tspec_(nullptr),
   extern_(false),
   inline_(false),
   constexpr_(false),
   static_(false),
   virtual_(false),
   explicit_(false),
   const_(false),
   volatile_(false),
   noexcept_(false),
   override_(false),
   final_(false),
   pure_(false),
   type_(type),
   friend_(false),
   found_(false),
   this_(false),
   tparm_(false),
   nonpublic_(false),
   nonstatic_(false),
   implicit_(false),
   defn_(false),
   deleted_(false),
   defaulted_(false),
   calls_(0),
   mate_(nullptr),
   spec_(spec.release()),
   pos_(string::npos),
   base_(nullptr),
   tmplt_(nullptr)
{
   Debug::ft("Function.ctor(spec)");

   spec_->SetUserType(TS_Function);
   if(type_) return;

   auto qname = name_->QualifiedName(true, false);
   OpenScope(qname);
   CxxStats::Incr(CxxStats::FUNCTION);
}

//------------------------------------------------------------------------------

Function::~Function()
{
   Debug::ftnt("Function.dtor");

   CxxStats::Decr(CxxStats::FUNCTION);
   if(type_) return;

   if(base_ != nullptr) base_->EraseOverride(this);
   GetFile()->EraseFunc(this);
   Singleton< CxxSymbols >::Extant()->EraseFunc(this);
}

//------------------------------------------------------------------------------

void Function::AddArg(ArgumentPtr& arg)
{
   Debug::ft("Function.AddArg");

   arg->SetScope(this);
   args_.push_back(std::move(arg));
}

//------------------------------------------------------------------------------

void Function::AddMemberInit(MemberInitPtr& init)
{
   Debug::ft("Function.AddMemberInit");

   mems_.push_back(std::move(init));
}

//------------------------------------------------------------------------------

void Function::AddOverride(Function* over) const
{
   Debug::ft("Function.AddOverride");

   overs_.push_back(over);
}

//------------------------------------------------------------------------------

void Function::AddThisArg()
{
   Debug::ft("Function.AddThisArg");

   //  Don't add a "this" argument if the function
   //  o is static
   //  o already has one
   //  o is an inline friend
   //  o does not belong to a class
   //
   if(static_ || this_ || friend_) return;
   auto cls = GetClass();
   if(cls == nullptr) return;

   //  The above does not reject the *definition* of a static member function,
   //  which lacks a declaration's "static" keyword.  To prevent the addition
   //  of a "this" argument, see if a function with the same arguments, but no
   //  "this" argument, already exists.
   //
   if(impl_ != nullptr)
   {
      auto prev = GetArea()->MatchFunc(this, false);

      if((prev != nullptr) && prev->IsPreviousDeclOf(this))
      {
         if(!prev->this_) return;
      }
   }

   //  Add an argument with the name "this", which is a pointer to the class
   //  that defined the function.  The argument is const if the function was
   //  defined as const.  Include the template parameters in the name of a
   //  function template's "this" argument.
   //
   TypeSpecPtr typeSpec(new DataSpec(cls->Name().c_str()));
   typeSpec->CopyContext(this, true);
   typeSpec->Tags()->SetConst(const_);
   typeSpec->Tags()->SetPointer(0, true, false);
   typeSpec->SetReferent(cls, nullptr);
   typeSpec->SetUserType(TS_Function);
   auto parms = cls->GetTemplateParms();
   if(parms != nullptr) typeSpec->GetQualName()->SetTemplateArgs(parms);
   string argName(THIS_STR);
   ArgumentPtr arg(new Argument(argName, typeSpec));
   arg->CopyContext(this, true);
   args_.insert(args_.begin(), std::move(arg));
   this_ = true;
}

//------------------------------------------------------------------------------

void Function::AdjustRecvConstness
   (const Function* invoker, StackArg& recvArg) const
{
   Debug::ft("Function.AdjustRecvConstness");

   //  Make ARG const if it's another instance of the same virtual function or
   //  it's a "this" argument and this function also has a const version.  This
   //  is done so that the argument about to be passed to ARG will not lose the
   //  possibility of being logged as "could be const".  If it is not declared
   //  const, it will select the non-const version of a function (making its
   //  non-constness self-fulfilling).  The same thing occurs if it is passed
   //  to the same argument in another instance of the same virtual function.
   //
   if(invoker == nullptr) return;
   if(recvArg.IsConst()) return;

   if(IsVirtual() && (FindRootFunc() == invoker->FindRootFunc()))
   {
      recvArg.SetAsConst();
      return;
   }

   if(recvArg.item_->Name() != THIS_STR) return;

   auto target = RemoveConsts(TypeString(true));
   auto list = GetArea()->FuncVector(Name());

   for(size_t i = 0; i < list->size(); ++i)
   {
      auto func = list->at(i).get();
      if(!func->IsConst()) continue;

      auto& temp = func->Name();

      if(temp == Name())
      {
         auto actual = RemoveConsts(func->TypeString(true));

         if(actual == target)
         {
            //  The following is commented out because it causes spurious
            //  "could be const" logs for data, arguments, and functions.
            //  The code can be enabled when the *chain* (entire expression)
            //  that precedes an assignment is preserved, which is a known
            //  deficiency.  For example, with vector V
            //    v.at(i)->ConstFunction();
            //    v.at(i)->NonConstFunction();
            //  the commented out code will discover that at() has a version
            //  that returns a const value.  It will therefore mark recvArg
            //  as const, even though it is subsequently used in a non-const
            //  manner.  Whether V can be const isn't known until the final
            //  function is invoked, but we don't preserve the chain that led
            //  up to that function call and therefore cannot retroactively
            //  mark V as ultimately being used in a non-const way.
            //
//c         recvArg.SetAsConst();
            return;
         }
      }
   }
}

//------------------------------------------------------------------------------

bool Function::ArgCouldBeConst(size_t n) const
{
   Debug::ft("Function.ArgCouldBeConst");

   //  If the function has overrides, check the argument in each.  If the
   //  function is a template or a member of a class template, check the
   //  argument in the first template instance.
   //
   auto arg = GetDefn()->args_[n].get();

   if(!arg->CouldBeConst()) return false;

   for(auto f = overs_.cbegin(); f != overs_.cend(); ++f)
   {
      if(!(*f)->ArgCouldBeConst(n)) return false;
   }

   auto fti = FirstInstance();
   if((fti != nullptr) && !fti->ArgCouldBeConst(n)) return false;

   auto fci = FirstInstanceInClass();
   if((fci != nullptr) && !fci->ArgCouldBeConst(n)) return false;

   return true;
}

//------------------------------------------------------------------------------

bool Function::ArgIsUnused(size_t n) const
{
   Debug::ft("Function.ArgIsUnused");

   //  If the function has overrides, check the argument in each.  If the
   //  function is a template or a member of a class template, check the
   //  argument in the first template instance.
   //
   auto arg = GetDefn()->args_[n].get();

   if(!arg->IsUnused()) return false;

   for(auto f = overs_.cbegin(); f != overs_.cend(); ++f)
   {
      if(!(*f)->ArgIsUnused(n)) return false;
   }

   auto fti = FirstInstance();
   if((fti != nullptr) && !fti->ArgIsUnused(n)) return false;

   auto fci = FirstInstanceInClass();
   if((fci != nullptr) && !fci->ArgIsUnused(n)) return false;

   return true;
}

//------------------------------------------------------------------------------

bool Function::ArgumentsMatch(const Function* that) const
{
   Debug::ft("Function.ArgumentsMatch");

   //  Check each argument for an exact match.
   //
   if(this->args_.size() != that->args_.size()) return false;

   auto s1 = this->spec_.get();
   auto s2 = that->spec_.get();

   if(s1 == nullptr)
   {
      if(s2 != nullptr) return false;
   }
   else
   {
      if(s2 == nullptr) return false;
      if(!s1->MatchesExactly(s2)) return false;
   }

   return true;
}

//------------------------------------------------------------------------------

TypeMatch Function::CalcConstructibilty
   (const StackArg& that, const string& thatType) const
{
   Debug::ft("Function.CalcConstructibilty");

   //  If this function must be invoked explicitly or is not even a
   //  constructor, there is no compatibility.
   //
   if(IsExplicit() || (FuncRole() != PureCtor)) return Incompatible;

   //  If this constructor can be invoked with a single argument, find
   //  out how well THAT matches with the constructor's argument.
   //
   if((MinArgs() <= 2) && (MaxArgs() == 2))
   {
      auto thisArg = args_[1].get();
      auto thisType = thisArg->TypeString(true);
      return StackArg(thisArg, 0, false).CalcMatchWith
         (that, thisType, thatType);
   }

   return Incompatible;
}

//------------------------------------------------------------------------------

bool Function::CanBeNoexcept() const
{
   Debug::ft("Function.CanBeNoexcept");

   //  A deleted function need not be noexcept.
   //
   if(deleted_) return false;

   //  The only functions that should be noexcept are virtual functions whose
   //  base class defined the function as noexcept.  This should be enforced
   //  by the compiler but must be checked to avoid generating a warning.
   //
   auto bf = FindBaseFunc();

   if(IsVirtual() && (FuncType() != FuncDtor))
   {
      for(NO_OP; bf != nullptr; bf = bf->FindBaseFunc())
      {
         if(bf->noexcept_ && bf->GetFile()->IsSubsFile()) return true;
      }
   }

   //  No other function should be noexcept:
   //  o A virtual function should not be noexcept because this forces all
   //    overrides to be noexcept.
   //  o A non-virtual function should not be noexcept because it will cause
   //    an exception if it uses a bad "this" pointer.
   //  o The compiler may treat constructors, destructors, and operators as
   //    noexcept if they rely on nothing that is potentially throwing, so
   //    there is little advantage to marking them noexcept.  Even a simple
   //    constructor or destructor shouldn't be noexcept if a constructor or
   //    destructor in a constructed base class is potentially throwing.
   //
   return false;
}

//------------------------------------------------------------------------------

Function* Function::CanInvokeWith
   (StackArgVector& args, stringVector& argTypes, TypeMatch& match) const
{
   Debug::ft("Function.CanInvokeWith");

   //  ARGS must not contain more arguments than this function accepts.  If
   //  the function has a "this" argument, ignore it if this function takes
   //  no "this" argument: this occurs when an implicit "this" is provided.
   //  This is safe because, ignoring the "this" argument, a static function
   //  and a member function of the same name cannot take the same arguments.
   //
   auto recvSize = args_.size();
   auto sendSize = args.size();
   auto sendIncr = 0;

   if(!this_ && (sendSize > 0) && args.front().IsThis())
   {
      sendSize -= 1;
      sendIncr = 1;
   }

   if(recvSize < sendSize) return FoundFunc(nullptr, args, match);

   //  If this is a function template, create a vector that will map template
   //  parameters to template arguments.
   //
   stringVector tmpltParms;
   stringVector tmpltArgs;
   auto tmplt = GetTemplateParms();

   if(tmplt != nullptr)
   {
      auto parms = tmplt->Parms();

      for(size_t i = 0; i < parms->size(); ++i)
      {
         tmpltParms.push_back(parms->at(i)->Name());
         tmpltArgs.push_back(EMPTY_STR);
      }
   }

   //  Each argument in ARGS must match, or be transformable to, the type that
   //  this function expects.  Assume compatibility and downgrade from there.
   //  Note that a "this" argument is skipped if thisIncr is 1 instead of 0.
   //
   match = Compatible;

   for(size_t i = 0; i < sendSize; ++i)
   {
      auto recvArg = args_.at(i).get();
      auto recvType = recvArg->TypeString(true);
      auto& sendType = argTypes.at(i + sendIncr);

      if(tmplt != nullptr)
      {
         //  We're invoking a function template.  It's a match if the function's
         //  argument contains a template parameter and the supplied argument is
         //  a valid specialization of that parameter.  It's a failure if the
         //  parameter has already been bound and this argument should have the
         //  same type, but doesn't.
         //
         auto argFound = false;

         auto curr = MatchTemplate
            (recvType, sendType, tmpltParms, tmpltArgs, argFound);

         if(curr != Incompatible)
         {
            if(curr < match) match = curr;
            continue;
         }

         if(argFound) return FoundFunc(nullptr, args, match);
      }

      auto recvResult = recvArg->GetTypeSpec()->ResultType();
      if(recvResult.item_ == nullptr) return FoundFunc(nullptr, args, match);
      auto sendArg = args.at(i + sendIncr);
      auto curr = recvResult.CalcMatchWith(sendArg, recvType, sendType);
      if(curr < match) match = curr;
      if(match == Incompatible) return FoundFunc(nullptr, args, match);
   }

   //  If ARGS had fewer arguments than this function, this function
   //  must have default values for the missing arguments.
   //
   if(sendSize < recvSize)
   {
      for(size_t i = sendSize; i < recvSize; ++i)
      {
         if(!args_.at(i)->HasDefault())
         {
            return FoundFunc(nullptr, args, match);
         }
      }
   }

   if(tmplt != nullptr)
   {
      //  This is a function template, so it needs to be instantiated.
      //
      auto inst = InstantiateFunction(tmpltArgs);
      if(inst == nullptr) match = Incompatible;
      return FoundFunc(inst, args, match);
   }

   return FoundFunc(const_cast< Function* >(this), args, match);
}

//------------------------------------------------------------------------------

void Function::Check() const
{
   Debug::ft("Function.Check");

   //  Only check the first instance of a function template.  Any warnings
   //  logged against it will be moved to the function template itself.
   //
   if((tmplt_ != nullptr) && (tmplt_->FirstInstance() != this)) return;

   if(parms_ != nullptr) parms_->Check();
   if(spec_ != nullptr) spec_->Check();

   for(auto a = args_.cbegin(); a != args_.cend(); ++a)
   {
      (*a)->Check();
   }

   if(call_ != nullptr)
   {
      call_->Check();
   }

   for(auto m = mems_.cbegin(); m != mems_.cend(); ++m)
   {
      (*m)->Check();
   }

   if(impl_ != nullptr) impl_->Check();

   if(!defn_)
   {
      auto w = CheckIfDefined();
      if(w != FunctionNotDefined) CheckIfUnused(FunctionUnused);
      if(w != FunctionNotDefined) CheckNoexcept();
      CheckIfHiding();
      CheckArgs();
      CheckAccessControl();
      CheckCtor();
      CheckDtor();
      CheckIfOverridden();
      CheckIfPublicVirtual();
      CheckForVirtualDefault();
      if(w != FunctionNotDefined) CheckMemberUsage();
      if(mate_ != nullptr) mate_->Check();
   }
}

//------------------------------------------------------------------------------

fn_name Function_CheckAccessControl = "Function.CheckAccessControl";

void Function::CheckAccessControl() const
{
   Debug::ft(Function_CheckAccessControl);

   if(defn_) return Debug::SwLog(Function_CheckAccessControl, "defn", 0);

   //  Checking the access control of a deleted function causes a "could be
   //  private" recommendation.
   //
   if(deleted_) return;

   //  Don't check the access control of destructors or operators (except
   //  for operator=).
   //
   switch(FuncType())
   {
   case FuncDtor:
      return;
   case FuncOperator:
      if(Name() != "operator=") return;
   }

   //  If this is an override, don't suggest a more restricted access control
   //  unless the function has a broader access control than the root function.
   //
   if(override_)
   {
      if(GetAccess() <= FindRootFunc()->GetAccess()) return;
   }

   CxxScoped::CheckAccessControl();
}

//------------------------------------------------------------------------------

fn_name Function_CheckArgs = "Function.CheckArgs";

void Function::CheckArgs() const
{
   Debug::ft(Function_CheckArgs);

   if(defn_) return Debug::SwLog(Function_CheckArgs, "defn", 0);

   //  See if the function has any arguments to check.  Don't check the
   //  arguments to a function that is undefined, unused, or an operator.
   //
   auto n = args_.size();
   if(n == 0 || IsUndefined() || IsUnused()) return;
   auto type = FuncType();
   if(type == FuncOperator) return;

   //  If the function is an override, look for arguments that were renamed
   //  from the root base class.
   //
   if(override_)
   {
      auto root = FindRootFunc();

      for(size_t i = 0; i < n; ++i)
      {
         if(args_[i]->Name() != root->args_[i]->Name())
         {
            LogToArg(OverrideRenamesArgument, i);
         }
      }

      if(mate_ != nullptr)
      {
         for(size_t i = 0; i < n; ++i)
         {
            if(mate_->args_[i]->Name() != root->args_[i]->Name())
            {
               mate_->LogToArg(OverrideRenamesArgument, i);
            }
         }
      }

      //  Other checks do not apply to an overridden function.
      //
      return;
   }

   //  If the function is defined separately from its declaration, look
   //  for renamed arguments.
   //
   if(mate_ != nullptr)
   {
      for(size_t i = 0; i < n; ++i)
      {
         auto& mateName = mate_->args_[i]->Name();

         if(!mateName.empty() && (mateName != args_[i]->Name()))
         {
            mate_->LogToArg(DefinitionRenamesArgument, i);
         }
      }
   }

   //  Look for unused arguments and arguments that could be const.
   //
   for(size_t i = 0; i < n; ++i)
   {
      auto arg = args_[i].get();

      if(ArgIsUnused(i))
      {
         if((i != 0) || !this_)
         {
            LogToArg(ArgumentUnused, i);
         }
      }
      else
      {
         //  If the argument is declared as const, see if a non-const
         //  usage was erroneously detected.
         //
         if(arg->IsConst())
         {
            if(!ArgCouldBeConst(i))
            {
               if((i == 0) && this_)
               {
                  if(type == FuncStandard) Log(FunctionCannotBeConst);
               }
               else
               {
                  LogToArg(ArgumentCannotBeConst, i);
               }
            }

            continue;
         }

         //  The argument is not const.  If it could be const, then
         //  o if the argument is "this", the function could be const;
         //  o if the argument is an object passed by value, it could
         //    be passed as a const reference;
         //  o otherwise, the argument could be declared const unless it
         //    is a pointer type used as a template argument (in which
         //    case making it const would apply to the pointer, not the
         //    underlying type)
         //
         if(ArgCouldBeConst(i))
         {
            if((i == 0) && this_)
            {
               if(type == FuncStandard) CheckIfCouldBeConst();
            }
            else
            {
               auto spec = arg->GetTypeSpec();

               if((spec->Ptrs(true) == 0) && (spec->Refs() == 0))
               {
                  if(arg->Root()->Type() == Cxx::Class)
                     LogToArg(ArgumentCouldBeConstRef, i);
               }
               else
               {
                  if(!IsTemplateArg(arg) || (spec->Ptrs(true) == 0))
                  {
                     LogToArg(ArgumentCouldBeConst, i);
                  }
               }
            }
         }
      }
   }

   //  If there are more than two arguments, look for adjacent arguments
   //  that have the same type.
   //
   if(n <= 2) return;
   if((n == 3) && this_) return;

   auto a1 = args_.cbegin();
   if(this_) a1 = std::next(a1);

   for(NO_OP; a1 != args_.cend(); ++a1)
   {
      auto a2 = std::next(a1);
      if(a2 == args_.cend()) return;

      auto t1 = (*a1)->TypeString(true);
      auto t2 = (*a2)->TypeString(true);
      if(t1 == t2) Log(AdjacentArgumentTypes);
   }
}

//------------------------------------------------------------------------------

fn_name Function_CheckCtor = "Function.CheckCtor";

void Function::CheckCtor() const
{
   Debug::ft(Function_CheckCtor);

   if(defn_) return Debug::SwLog(Function_CheckCtor, "defn", 0);

   //  Check that this is a constructor and that it isn't deleted.
   //
   if(FuncType() != FuncCtor) return;
   auto defn = GetDefn();
   auto impl = defn->impl_.get();
   if(!IsImplemented()) return;

   auto role = FuncRole();

   if(role == PureCtor)
   {
      //  This is a not a copy or move constructor.  It should probably be
      //  tagged explicit if it is not invoked implicitly and can take one
      //  argument (besides the "this" argument that we give it). On the
      //  other hand, a constructor that cannot take one argument does not
      //  need to be tagged explicit.
      //
      auto min = MinArgs() - 1;
      auto max = MaxArgs() - 1;

      if((min <= 1) && (max == 1) && !explicit_ && !implicit_)
      {
         Log(NonExplicitConstructor);
      }
      else if(explicit_ && ((max == 0) || (min >= 2)))
      {
         Log(ExplicitConstructor);
      }

      if(base_ == nullptr)
      {
         auto base = GetClass()->BaseClass();
         if(base != nullptr)
         {
            base->WasCalled(role, this);
         }
      }
   }

   //  An empty constructor that neither explicitly invokes a base class
   //  constructor nor explicitly initializes a member can be defaulted.
   //
   auto& mems = defn->mems_;

   if((impl != nullptr) && (impl->FirstStatement() == nullptr) &&
      (defn->call_ == nullptr) && mems.empty())
   {
      Log(FunctionCouldBeDefaulted);
   }

   //  The compiler default is for a copy or move constructor to invoke the
   //  base class *constructor*, not its copy or move constructor.  This is
   //  alright if this class has a default copy or move constructor that can
   //  simply make a bitwise copy.  Otherwise, it may not be the desired
   //  behavior unless the base copy or move constructor is deleted.
   //
   if((role == CopyCtor) || (role == MoveCtor))
   {
      if((defn->call_ == nullptr) && !IsDefaulted())
      {
         auto base = GetClass()->BaseClass();
         if(base != nullptr)
         {
            auto func = base->FindFuncByRole(role, true);
            if((func == nullptr) || !func->IsDeleted())
            {
               defn->Log(CopyCtorConstructsBase);
            }
         }
      }
   }

   //  Get ITEMS, a list of the class's data members.  This list contains the
   //  members in order of declaration and indicates how each member should be
   //  initialized. Go through the member initialization list, if any, find
   //  each initialized member in ITEMS, and record when it was initialized.
   //
   auto cls = GetClass();
   DataInitVector items;
   cls->GetMemberInitAttrs(items);

   for(size_t i = 0; i < mems.size(); ++i)
   {
      auto mem = mems.at(i).get();
      auto data = cls->FindData(mem->Name());

      for(size_t j = 0; j < items.size(); ++j)
      {
         if(items[j].member == data)
         {
            items[j].initOrder = i + 1;
            break;
         }
      }
   }

   //  All members that require initialization should be initialized in order
   //  of declaration.  If a member should be initialized but was not, log it
   //  unless this is a default copy constructor, which effectively does a
   //  bitwise copy.  If a member was initialized out of order, log it against
   //  the initialization statement.
   //
   size_t last = 0;

   for(auto item = items.cbegin(); item != items.cend(); ++item)
   {
      if(item->initOrder == 0)
      {
         if((item->initNeeded) && (!IsDefaulted() || FuncRole() == PureCtor))
         {
            //  Log both the missing member and the suspicious constructor.
            //  This helps to pinpoint where the concern lies.
            //
            Log(MemberInitMissing);
            item->member->Log(MemberInitMissing);
         }
      }
      else
      {
         if(item->initOrder < last)
         {
            auto token = mems.at(item->initOrder - 1).get();
            token->Log(MemberInitNotSorted);
         }

         last = item->initOrder;
      }
   }
}

//------------------------------------------------------------------------------

const string LeftPunctuation("([<{");

bool Function::CheckDebugName(const string& str) const
{
   Debug::ft("Function.CheckDebugName");

   //  Check that STR is of the form
   //     "<scope>.<name>"
   //  where <scope> is the name of the function's scope and <name> is its
   //  name.  However
   //  o If the function is defined in the global namespace, its name will
   //    have no <scope> prefix.
   //  o If function is overloaded, "left punctuation" can follow <name> in
   //    order to give a unique name to each of the function's overloads.
   //
   auto name = DebugName();
   auto scope = GetScope()->Name();

   if(scope.empty())
   {
      if(str.find(name) != 0) return false;
      auto size = name.size();
      if(str.size() == size) return true;
      return (LeftPunctuation.find(str[size]) != string::npos);
   }

   auto dot = str.find('.');
   if(dot == string::npos) return false;
   if(str.find(scope) != 0) return false;
   if(str.find(name, dot) != dot + 1) return false;
   auto size = scope.size() + 1 + name.size();
   if(str.size() == size) return true;
   return (LeftPunctuation.find(str[size]) != string::npos);
}

//------------------------------------------------------------------------------

fn_name Function_CheckDtor = "Function.CheckDtor";

void Function::CheckDtor() const
{
   Debug::ft(Function_CheckDtor);

   if(defn_) return Debug::SwLog(Function_CheckDtor, "defn", 0);
   if(FuncType() != FuncDtor) return;

   auto impl = GetDefn()->impl_.get();
   if((impl != nullptr) && (impl->FirstStatement() == nullptr))
   {
      Log(FunctionCouldBeDefaulted);
   }
}

//------------------------------------------------------------------------------

fn_name Function_CheckForVirtualDefault = "Function.CheckForVirtualDefault";

void Function::CheckForVirtualDefault() const
{
   Debug::ft(Function_CheckForVirtualDefault);

   if(defn_) return Debug::SwLog(Function_CheckForVirtualDefault, "defn", 0);
   if(!virtual_) return;

   for(auto a = args_.cbegin(); a != args_.cend(); ++a)
   {
      if((*a)->HasDefault())
      {
         Log(VirtualDefaultArgument);
         return;
      }
   }
}

//------------------------------------------------------------------------------

void Function::CheckFree() const
{
   Debug::ft("Function.CheckFree");

   //  This function can be free.  But if it has a possible "this" argument
   //  for another class, it should probably be a member of that class.
   //
   for(size_t i = (this_ ? 1 : 0); i < args_.size(); ++i)
   {
      auto cls = args_[i]->IsThisCandidate();

      if((cls != nullptr) && (cls != GetClass()))
      {
         LogToArg(FunctionCouldBeMember, i);
         return;
      }
   }

   Log(FunctionCouldBeFree);
}

//------------------------------------------------------------------------------

fn_name Function_CheckIfCouldBeConst = "Function.CheckIfCouldBeConst";

void Function::CheckIfCouldBeConst() const
{
   Debug::ft(Function_CheckIfCouldBeConst);

   if(defn_) return Debug::SwLog(Function_CheckIfCouldBeConst, "defn", 0);

   //  Before claiming that a function could be const, check for const
   //  overloading (another function in this class that has the same name
   //  and that takes the same arguments).  If so, it can only differ in
   //  constness, which prevents this function from being const.
   //
   auto list = GetClass()->FuncVector(Name());

   for(auto f = list->cbegin(); f != list->cend(); ++f)
   {
      auto func = f->get();

      if((func != this) && (func->Name() == Name())) return;
   }

   Log(FunctionCouldBeConst);
}

//------------------------------------------------------------------------------

fn_name Function_CheckIfDefined = "Function.CheckIfDefined";

Warning Function::CheckIfDefined() const
{
   Debug::ft(Function_CheckIfDefined);

   if(defn_)
   {
      Debug::SwLog(Function_CheckIfDefined, "defn", 0);
      return Warning_N;
   }

   //  A function without an implementation is logged as undefined unless
   //  o it's actually part of a function signature typedef;
   //  o it is deleting the default that the compiler would otherwise provide;
   //  o it uses the compiler-generated default.
   //  Pure virtual functions are logged separately, because not providing an
   //  implementation may be intentional.
   //
   if(GetDefn()->impl_ != nullptr) return Warning_N;
   if(IsDefaulted()) return Warning_N;
   if(type_) return Warning_N;
   if(IsDeleted()) return Warning_N;

   auto w = (pure_ ? PureVirtualNotDefined : FunctionNotDefined);
   Log(w);
   return w;
}

//------------------------------------------------------------------------------

fn_name Function_CheckIfHiding = "Function.CheckIfHiding";

void Function::CheckIfHiding() const
{
   Debug::ft(Function_CheckIfHiding);

   if(defn_) return Debug::SwLog(Function_CheckIfHiding, "defn", 0);
   if(FuncType() != FuncStandard) return;

   auto item = FindInheritedName();
   if(item == nullptr) return;

   if(item->Type() != Cxx::Function)
   {
      if(item->GetAccess() != Cxx::Private) Log(HidesInheritedName);
   }
   else
   {
      if(!static_cast< Function* >(item)->virtual_)
      {
         if(item->GetAccess() != Cxx::Private) Log(HidesInheritedName);
      }
      else
      {
         if(!override_) Log(HidesInheritedName);
      }
   }
}

//------------------------------------------------------------------------------

fn_name Function_CheckIfOverridden = "Function.CheckIfOverridden";

void Function::CheckIfOverridden() const
{
   Debug::ft(Function_CheckIfOverridden);

   if(defn_) return Debug::SwLog(Function_CheckIfOverridden, "defn", 0);

   //  To be logged for having no overrides, this function must be virtual,
   //  not an override, and a standard function (not a destructor).
   //
   if(!virtual_ || override_ || (FuncType() != FuncStandard)) return;
   if(overs_.empty()) Log(FunctionNotOverridden);
}

//------------------------------------------------------------------------------

fn_name Function_CheckIfPublicVirtual = "Function.CheckIfPublicVirtual";

void Function::CheckIfPublicVirtual() const
{
   Debug::ft(Function_CheckIfPublicVirtual);

   if(defn_) return Debug::SwLog(Function_CheckIfPublicVirtual, "defn", 0);

   //  To be logged for being public and virtual, this must be a standard
   //  function that is not overriding one that was already public.
   //
   if(!virtual_ || (GetAccess() != Cxx::Public)) return;
   if(FuncType() != FuncStandard) return;

   for(auto b = base_; b != nullptr; b = b->base_)
   {
      if(base_->GetAccess() == Cxx::Public) return;
   }

   Log(VirtualAndPublic);
}

//------------------------------------------------------------------------------

fn_name Function_CheckIfUnused = "Function.CheckIfUnused";

bool Function::CheckIfUnused(Warning warning) const
{
   Debug::ft(Function_CheckIfUnused);

   if(defn_)
   {
      Debug::SwLog(Function_CheckIfUnused, "defn", 0);
      return false;
   }

   if(type_) return false;
   if(override_) return false;
   if(!IsUnused()) return false;

   switch(FuncRole())
   {
   case CopyCtor:
   case CopyOper:
      return IsUnusedCopyFunction();
   }

   if(FuncType() == FuncOperator) return false;
   Log(warning);
   return true;
}

//------------------------------------------------------------------------------

fn_name Function_CheckMemberUsage = "Function.CheckMemberUsage";

void Function::CheckMemberUsage() const
{
   Debug::ft(Function_CheckMemberUsage);

   if(defn_) return Debug::SwLog(Function_CheckMemberUsage, "defn", 0);

   //  Check if this function could be static or free.  For either to be
   //  possible, the function cannot be virtual, must not have accessed a
   //  non-static member, and must be a standard member function that is not
   //  part of a template.  [A function which accesses a non-static member
   //  could still be free or static, provided that the member was public.
   //  However, the function would have to add the underlying object as an
   //  argument--essentially a "this" argument.  Some will argue that this
   //  improves encapsulation, but we will demur.]
   //
   if(virtual_) return;
   if(type_) return;
   if(GetDefn()->nonstatic_) return;
   if(FuncType() != FuncStandard) return;

   auto cls = GetClass();
   if(cls == nullptr) return;

   //  The function can be free if
   //  (a) it only accessed public members, and
   //  (b) it's not inline (which is probably to obey ODR), and
   //  (c) it doesn't use a template parameter.
   //  Otherwise it can be static.
   //
   if(!GetDefn()->nonpublic_ && !inline_ && !tparm_)
      CheckFree();
   else
      CheckStatic();
}

//------------------------------------------------------------------------------

fn_name Function_CheckNoexcept = "Function.CheckNoexcept";

void Function::CheckNoexcept() const
{
   Debug::ft(Function_CheckNoexcept);

   if(defn_) return Debug::SwLog(Function_CheckNoexcept, "defn", 0);

   auto can = CanBeNoexcept();

   if(noexcept_)
   {
      if(!can) Log(ShouldNotBeNoexcept);
   }
   else
   {
      if(can) Log(CouldBeNoexcept);
   }
}

//------------------------------------------------------------------------------

fn_name Function_CheckOverride = "Function.CheckOverride";

void Function::CheckOverride()
{
   Debug::ft(Function_CheckOverride);

   if(defn_) return Debug::SwLog(Function_CheckOverride, "defn", 0);

   //  If this function is an override, register it against the function that
   //  it immediately overrides.  A destructor is neither registered nor logged.
   //  It is also redundant (and can cause unintended consequences) to use more
   //  than one of virtual, override, or final, so specify which of these should
   //  be removed (or added, in the case of an unmarked override).
   //
   base_ = FindBaseFunc();
   if(base_ == nullptr) return;
   if(FuncType() == FuncDtor) return;

   base_->AddOverride(this);
   if(virtual_ && (override_ || final_)) Log(RemoveVirtualTag);
   if(override_ && final_) Log(RemoveOverrideTag);
   if(!override_ && !final_) Log(OverrideTagMissing);
   virtual_ = true;
   override_ = true;
}

//------------------------------------------------------------------------------

void Function::CheckStatic() const
{
   Debug::ft("Function.CheckStatic");

   //  If this function isn't static, it could be.
   //
   if(!static_)
   {
      Log(FunctionCouldBeStatic);
      return;
   }

   //  The function is already static.  But if it has a possible "this"
   //  argument for its class, it should probably be non-static.
   //
   for(size_t i = 0; i < args_.size(); ++i)
   {
      auto cls = args_[i]->IsThisCandidate();

      if(cls == GetClass())
      {
         LogToArg(FunctionCouldBeMember, i);
         return;
      }
   }
}

//------------------------------------------------------------------------------

bool Function::ContainsTemplateParameter() const
{
   Debug::ft("Function.ContainsTemplateParameter");

   return tspec_->ContainsTemplateParameter();
}

//------------------------------------------------------------------------------

string Function::DebugName() const
{
   Debug::ft("Function.DebugName");

   switch(FuncType())
   {
   case FuncCtor:
      return "ctor";
   case FuncDtor:
      return "dtor";
   }

   return Name();
}

//------------------------------------------------------------------------------

void Function::Delete()
{
   Debug::ftnt("Function.Delete");

   if(mate_ != nullptr) mate_->mate_ = nullptr;
   GetArea()->EraseFunc(this);
   delete this;
}

//------------------------------------------------------------------------------

void Function::DeleteVoidArg()
{
   Debug::ft("Function.DeleteVoidArg");

   if(this_)
      args_[1].reset();
   else
      args_[0].reset();
}

//------------------------------------------------------------------------------

void Function::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   if(tmplt_ != nullptr) return;

   stream << prefix;
   if(!options.test(DispNoAC) && !defn_ && (GetClass() != nullptr))
   {
      stream << GetAccess() << ": ";
   }
   DisplayDecl(stream, options);
   DisplayDefn(stream, prefix, options);

   if(!options.test(DispCode) && !tmplts_.empty())
   {
      stream << prefix << "instantiations (" << tmplts_.size() << "):" << CRLF;
      auto lead = prefix + spaces(IndentSize());

      for(auto t = tmplts_.cbegin(); t != tmplts_.cend(); ++t)
      {
         stream << lead;
         (*t)->DisplayDecl(stream, options);
         stream << ';';
         (*t)->DisplayInfo(stream, options);
         stream << CRLF;
      }
   }
}

//------------------------------------------------------------------------------

void Function::DisplayDecl(ostream& stream, const Flags& options) const
{
   //  Note that, except for "const", tags (extern, inline, constexpr, static,
   //  virtual, explicit, noexcept, override, and "= 0" for pure virtual) are
   //  only set in the declaration, not in a separate definition.  Because of
   //  this, they will not appear when displaying a separate definition.
   //
   if(extern_) stream << EXTERN_STR << SPACE;

   if(!options.test(DispNoTP))
   {
      if(parms_ != nullptr) parms_->Print(stream, options);
   }

   if(inline_) stream << INLINE_STR << SPACE;
   if(constexpr_) stream << CONSTEXPR_STR << SPACE;
   if(static_) stream << STATIC_STR << SPACE;
   if(virtual_ && !override_ && !final_) stream << VIRTUAL_STR << SPACE;
   if(explicit_) stream << EXPLICIT_STR << SPACE;

   if(Operator() == Cxx::CAST)
   {
      strName(stream, options.test(DispFQ), name_.get());
      stream << SPACE;
      spec_->Print(stream, options);
   }
   else
   {
      if(spec_ != nullptr)
      {
         spec_->Print(stream, options);
         stream << SPACE;
      }

      strName(stream, options.test(DispFQ), name_.get());
   }

   stream << '(';

   auto args = &args_;

   if(options.test(DispNS))
   {
      //  In namespace view, the definition will follow, so display
      //  the arguments as they appear in the definition.
      //
      args = &GetDefn()->args_;
   }

   for(size_t i = (this_ ? 1 : 0); i < args->size(); ++i)
   {
      (*args)[i]->Print(stream, options);
      if(i != args->size() - 1) stream << ", ";
   }

   stream << ')';
   if(const_) stream << SPACE << CONST_STR;
   if(volatile_) stream << SPACE << VOLATILE_STR;
   if(noexcept_) stream << SPACE << NOEXCEPT_STR;
   if(override_ && !final_) stream << SPACE << OVERRIDE_STR;
   if(final_) stream << SPACE << FINAL_STR;
   if(pure_) stream << " = 0";
}

//------------------------------------------------------------------------------

void Function::DisplayDefn(ostream& stream,
   const string& prefix, const Flags& options) const
{
   auto ns = options.test(DispNS);
   auto defn = GetDefn();
   auto impl = defn->impl_.get();

   //  Do not display the function's implementation if
   //  (a) there isn't one
   //  (b) this is only the function's declaration in file view
   //  (c) this is internally generated code (a template instance)
   //
   if((impl == nullptr) || (!ns && (impl_ == nullptr)) || IsInternal())
   {
      if(deleted_)
         stream << " = " << DELETE_STR;
      else if(IsDefaulted())
         stream << " = " << DEFAULT_STR;
      stream << ';';
      DisplayInfo(stream, options);
      stream << CRLF;
      return;
   }

   auto call = defn->call_.get();
   auto& mems = defn->mems_;
   auto inits = mems.size() + (call != nullptr ? 1 : 0);

   switch(inits)
   {
   case 0:
      break;

   case 1:
      stream << " : ";

      if(call != nullptr)
         call->Print(stream, options);
      else
         mems.front()->Print(stream, options);
      break;

   default:
      stream << " :";
      DisplayInfo(stream, options);
      stream << CRLF;
      auto lead = prefix + spaces(IndentSize());

      if(call != nullptr)
      {
         stream << lead;
         call->Print(stream, options);
         if(!mems.empty()) stream << ',' << CRLF;
      }

      for(auto m = mems.cbegin(); m != mems.cend(); ++m)
      {
         stream << lead;
         (*m)->Print(stream, options);
         if(*m != mems.back()) stream << ',' << CRLF;
      }
   }

   auto form = Block::Braced;  // inlined unless multiple statements

   if(inits > 1)
      form = Block::Empty;  // never inlined, even if empty
   else if(inits > 0)
      form = Block::Unbraced;  // inlined only if empty

   if(!impl->CrlfOver(form))
   {
      impl->Print(stream, options);
      if(inits <= 1) DisplayInfo(stream, options);
      stream << CRLF;
   }
   else
   {
      auto opts = options;
      opts.set(DispLF);
      if(inits <= 1) DisplayInfo(stream, options);
      impl->Display(stream, prefix, opts);
   }
}

//------------------------------------------------------------------------------

void Function::DisplayInfo(ostream& stream, const Flags& options) const
{
   if(options.test(DispCode)) return;

   auto cls = GetClass();
   auto decl = GetDecl();
   auto defn = GetDefn();
   auto impl = (defn->impl_ != nullptr);
   auto inst = ((cls != nullptr) && cls->IsInTemplateInstance());
   auto subs = GetFile()->IsSubsFile();
   auto def = IsDefaulted();

   std::ostringstream buff;
   buff << " // ";

   if(!impl && !inst && !subs && !deleted_ && !def)
      buff << "<@unimpl" << SPACE;

   if(options.test(DispStats))
   {
      auto calls = !override_ && (impl || inst || subs || def);
      if(!decl->overs_.empty()) buff << "o=" << decl->overs_.size() << SPACE;
      if((decl->calls_ > 0) || calls) buff << "c=" << decl->calls_ << SPACE;
   }

   if(!options.test(DispFQ) && impl) DisplayFiles(buff);
   auto str = buff.str();
   if(str.size() > 4) stream << str;
}

//------------------------------------------------------------------------------

void Function::EnterBlock()
{
   Debug::ft("Function.EnterBlock");

   //  If the function has no implementation, do nothing.  An empty function
   //  (just the braces) has an empty code block, so it will get past this.
   //  A defaulted function is treated as if it had an empty code block.
   //
   if(!IsImplemented()) return;

   if(GetTemplateType() != NonTemplate)
   {
      //  This is a function template or a function in a class template.
      //  Don't bother compiling a function template in a class template
      //  *instance*.  However, a *regular* function in a class template
      //  instance *is* compiled, and so is a function template instance
      //  (GetTemplateType returns NonTemplate in those cases).
      //
      if(Context::ParsingTemplateInstance()) return;
   }

   //  Set up the compilation context and add any template parameters and
   //  arguments to the local symbol table.  Compile the function's code,
   //  including any base constructor call and member initializations.  The
   //  latter are first assigned to their respective members, after which
   //  all non-static members are initialized so that class members can
   //  invoke default constructors.
   //
   const Class* cls = GetClass();
   if(cls != nullptr) cls->EnterParms();

   if(parms_ != nullptr) parms_->EnterBlock();

   Context::Enter(this);
   Context::PushScope(this, true);

   for(auto a = args_.cbegin(); a != args_.cend(); ++a)
   {
      (*a)->EnterBlock();
   }

   if(FuncType() == FuncCtor)
   {
      if(call_ != nullptr)
      {
         call_->EnterBlock();
         Context::Clear(3);
      }
      else
      {
         InvokeDefaultBaseCtor();
      }

      for(auto m = mems_.cbegin(); m != mems_.cend(); ++m)
      {
         (*m)->EnterBlock();
      }

      auto data = cls->Datas();

      for(auto d = data->cbegin(); d != data->cend(); ++d)
      {
         auto item = d->get();
         if(!item->IsStatic()) item->EnterBlock();
      }
   }

   if(impl_ != nullptr) impl_->EnterBlock();

   for(auto a = args_.cbegin(); a != args_.cend(); ++a)
   {
      (*a)->ExitBlock();
   }

   if(parms_ != nullptr) parms_->ExitBlock();
   if(cls != nullptr) cls->ExitParms();

   Context::PopScope();
}

//------------------------------------------------------------------------------

bool Function::EnterScope()
{
   Debug::ft("Function.EnterScope");

   //  If this function requires a "this" argument, add it now.
   //
   AddThisArg();
   Context::Enter(this);
   if(parms_ != nullptr) parms_->EnterScope();

   //  Enter our return type and arguments.
   //
   EnterSignature();
   CloseScope();

   //  See whether this is a new function or the definition of a
   //  previously declared function.
   //
   auto defn = false;

   if(IsImplemented())
   {
      auto decl = GetArea()->MatchFunc(this, false);

      if((decl != nullptr) && decl->IsPreviousDeclOf(this))
      {
         defn = true;
         Singleton< CxxSymbols >::Instance()->EraseFunc(this);
         decl->SetDefn(this);
      }
   }

   //  Add the function to its file's functions.  If it's a declaration,
   //  check if it's an override.  Add it to the area where it was found
   //  and compile it.
   //
   found_ = true;
   if(defn || AtFileScope()) GetFile()->InsertFunc(this);
   if(!defn) CheckOverride();
   GetArea()->InsertFunc(this);
   EnterBlock();
   return !defn;
}

//------------------------------------------------------------------------------

void Function::EnterSignature()
{
   Debug::ft("Function.EnterSignature");

   if(spec_ != nullptr)
   {
      if(GetArea()->FindItem(Name()) != nullptr)
         spec_->SetUserType(TS_Definition);
      spec_->EnteringScope(this);
   }

   for(auto a = args_.cbegin(); a != args_.cend(); ++a)
   {
      (*a)->EnterScope();
   }

   //  When a function's argument list is "(void)", an argument is created
   //  and later deleted by DeleteVoidArg.  This leaves an empty argument at
   //  the end of our vector, so clean it up now.  It can't be cleaned up by
   //  DeleteVoidArg, because this would cause the above iterator to fail.
   //
   if(!args_.empty() && (args_.back() == nullptr)) args_.pop_back();
}

//------------------------------------------------------------------------------

void Function::EraseArg(const Argument* arg)
{
   Debug::ft("Function.EraseArg");

   EraseItemPtr(args_, arg);
}

//------------------------------------------------------------------------------

void Function::EraseMemberInit(const MemberInit* init)
{
   Debug::ft("Function.EraseMemberInit");

   EraseItemPtr(mems_, init);
}

//------------------------------------------------------------------------------

void Function::EraseOverride(const Function* over) const
{
   Debug::ft("Function.EraseOverride");

   EraseItem(overs_, over);
}

//------------------------------------------------------------------------------

size_t Function::FindArg(const Argument* arg, bool disp) const
{
   Debug::ft("Function.FindArg");

   for(size_t i = 0; i < args_.size(); ++i)
   {
      if(args_[i].get() == arg)
      {
         return (this_ || !disp ? i : i + 1);
      }
   }

   return SIZE_MAX;
}

//------------------------------------------------------------------------------

Function* Function::FindBaseFunc() const
{
   Debug::ft("Function.FindBaseFunc");

   if(defn_) return GetDecl()->FindBaseFunc();

   //  If the base class function has already been found, return it.
   //
   if(base_ != nullptr) return base_;

   //  To have a base class version, a function cannot be static, cannot
   //  be a type, and must be declared in a class.
   //
   if(static_ || type_) return nullptr;

   auto cls = GetClass();
   if(cls == nullptr) return nullptr;

   //  For a constructor, base_ is set to the constructor invoked in the
   //  member initialization list or the base constructor that is invoked
   //  implicitly (see WasCalled).
   //
   switch(FuncType())
   {
   case FuncDtor:
      for(auto s = cls->BaseClass(); s != nullptr; s = s->BaseClass())
      {
         auto dtor = s->FindDtor();
         if(dtor != nullptr) return dtor;
      }
      return nullptr;

   case FuncOperator:
      for(auto s = cls->BaseClass(); s != nullptr; s = s->BaseClass())
      {
         auto opers = s->Opers();

         for(auto o = opers->cbegin(); o != opers->cend(); ++o)
         {
            auto oper = o->get();

            if((oper->Name() == this->Name()) && SignatureMatches(oper, true))
            {
               return oper;
            }
         }
      }
      return nullptr;

   case FuncStandard:
      for(auto s = cls->BaseClass(); s != nullptr; s = s->BaseClass())
      {
         auto funcs = s->Funcs();

         for(auto f = funcs->cbegin(); f != funcs->cend(); ++f)
         {
            auto func = f->get();

            if((func->Name() == this->Name()) && SignatureMatches(func, true))
            {
               return (func->virtual_ ? func : nullptr);
            }
         }
      }
   }

   return nullptr;
}

//------------------------------------------------------------------------------

CxxScoped* Function::FindNthItem(const std::string& name, size_t& n) const
{
   Debug::ft("Function.FindNthItem");

   for(auto a = args_.cbegin(); a != args_.cend(); ++a)
   {
      auto item = (*a)->FindNthItem(name, n);
      if(item != nullptr) return item;
   }

   if(impl_ == nullptr) return nullptr;
   return impl_->FindNthItem(name, n);
}

//------------------------------------------------------------------------------

Function* Function::FindRootFunc() const
{
   Debug::ft("Function.FindRootFunc");

   if(defn_) return GetDecl()->FindRootFunc();

   //  Follow the chain of overrides to the original virtual function.
   //
   auto prev = const_cast< Function* >(this);

   for(auto curr = base_; curr != nullptr; curr = curr->base_)
   {
      prev = curr;
   }

   return prev;
}

//------------------------------------------------------------------------------

fn_name Function_FindTemplateAnalog = "Function.FindTemplateAnalog";

CxxScoped* Function::FindTemplateAnalog(const CxxToken* item) const
{
   Debug::ft(Function_FindTemplateAnalog);

   //  Start by assuming that this is a function template instance.
   //
   auto func = tmplt_;

   if(func == nullptr)
   {
      //  This can be invoked on a regular function in a class template
      //  instance.  In that case it needs to find ITEM's analog in the
      //  class template's version of that function, so start by finding
      //  that function.
      //
      auto inst = GetTemplateInstance();
      if(inst == nullptr) return nullptr;
      func = static_cast< Function* >(inst->FindTemplateAnalog(this));
      if(func == nullptr) return nullptr;
   }

   auto type = item->Type();

   switch(type)
   {
   case Cxx::Function:
      return func;

   case Cxx::Argument:
   {
      auto i = FindArg(static_cast< const Argument* >(item), false);
      if(i == SIZE_MAX) return nullptr;
      return func->GetArgs().at(i).get();
   }

   case Cxx::Data:
   case Cxx::Enum:
   case Cxx::Enumerator:
   case Cxx::Typedef:
   {
      //  This item is defined inside this function.  Find its offset and then
      //  find the item in the template at the same offset.
      //
      size_t n = 0;
      if(!LocateItem(item, n)) return nullptr;
      return func->FindNthItem(item->Name(), n);
   }

   default:
      Context::SwLog(Function_FindTemplateAnalog, "Unexpected item", type);
   }

   return nullptr;
}

//------------------------------------------------------------------------------

Function* Function::FirstInstance() const
{
   Debug::ft("Function.FirstInstance");

   auto fti = tmplts_.cbegin();
   if(fti == tmplts_.cend()) return nullptr;
   return *fti;
}

//------------------------------------------------------------------------------

Function* Function::FirstInstanceInClass() const
{
   Debug::ft("Function.FirstInstanceInClass");

   auto cls = GetClass();
   if(cls == nullptr) return nullptr;
   auto instances = cls->Instances();
   auto cti = instances->cbegin();
   if(cti == instances->cend()) return nullptr;
   return static_cast< Function* >((*cti)->FindInstanceAnalog(this));
}

//------------------------------------------------------------------------------

Function* Function::FoundFunc
   (Function* func, const StackArgVector& args, TypeMatch& match)
{
   Debug::ft("Function.FoundFunc");

   //  If a function template has been instantiated, record that each of its
   //  arguments was used.  This ensures that >trim will ask for each type to
   //  be #included in the file that is using the function template.  Although
   //  this is strictly necessary only for those arguments that were used to
   //  determine the template specialization, it is a reasonable approximation.
   //
   if(func != nullptr)
   {
      if(func->IsTemplateInstance())
      {
         for(auto a = args.cbegin(); a != args.cend(); ++a)
         {
            a->item_->Root()->RecordUsage();
         }
      }
   }
   else
   {
      match = Incompatible;
   }

   return func;
}

//------------------------------------------------------------------------------

FunctionRole Function::FuncRole() const
{
   Argument* arg;
   size_t refs;

   switch(FuncType())
   {
   case FuncCtor:
      if(args_.size() == 1) return PureCtor;
      if(MinArgs() > 2) return PureCtor;
      arg = args_[1].get();
      if(arg->Root() != GetClass()) return PureCtor;
      refs = arg->GetTypeSpec()->Refs();
      if(refs == 1) return CopyCtor;
      if(refs == 2) return MoveCtor;
      return PureCtor;

   case FuncDtor:
      return PureDtor;

   case FuncOperator:
      if(Operator() == Cxx::ASSIGN)
      {
         arg = args_[1].get();
         if(arg->Root() != GetClass()) return FuncOther;
         if(parms_ != nullptr) return FuncOther;
         refs = arg->GetTypeSpec()->Refs();
         if(refs == 2) return MoveOper;
         return CopyOper;
      }
   }

   return FuncOther;
}

//------------------------------------------------------------------------------

FunctionType Function::FuncType() const
{
   if(Operator() != Cxx::NIL_OPERATOR) return FuncOperator;
   if(spec_ != nullptr) return FuncStandard;
   if(Name().find('~') != string::npos) return FuncDtor;
   if(parms_ != nullptr) return FuncStandard;
   return FuncCtor;
}

//------------------------------------------------------------------------------

Cxx::Access Function::GetAccess() const
{
   if(defn_) return GetDecl()->GetAccess();
   return CxxScope::GetAccess();
}

//------------------------------------------------------------------------------

CodeFile* Function::GetDeclFile() const
{
   return (defn_ ? mate_->GetFile() : GetFile());
}

//------------------------------------------------------------------------------

void Function::GetDecls(CxxNamedSet& items)
{
   if(IsDecl()) items.insert(this);
}

//------------------------------------------------------------------------------

const Function* Function::GetDefn() const
{
   if(defn_) return this;
   if(mate_ != nullptr) return mate_;
   return this;
}

//------------------------------------------------------------------------------

Function* Function::GetDefn()
{
   if(defn_) return this;
   if(mate_ != nullptr) return mate_;
   return this;
}

//------------------------------------------------------------------------------

CodeFile* Function::GetDefnFile() const
{
   if(impl_ != nullptr) return GetFile();
   if(mate_ != nullptr) return mate_->GetFile();
   return nullptr;
}

//------------------------------------------------------------------------------

CxxScope* Function::GetScope() const
{
   //  An inline friend function is considered to be defined in the same
   //  scope that defined the class.
   //
   auto scope = CxxScoped::GetScope();
   if(!friend_) return scope;
   return scope->GetScope();
}

//------------------------------------------------------------------------------

bool Function::GetSpan(size_t& begin, size_t& left, size_t& end) const
{
   Debug::ft("Function.GetSpan");

   GetTypeSpan(begin, end);
   if(impl_ == nullptr) return (end != string::npos);

   auto& lexer = GetFile()->GetLexer();
   left = impl_->GetPos();
   if(left == string::npos) return false;
   end = lexer.FindClosing('{', '}', left + 1);
   return (end != string::npos);
}

//------------------------------------------------------------------------------

CxxScope* Function::GetTemplate() const
{
   if(tmplt_ != nullptr) return tmplt_;
   if(IsTemplate()) return const_cast< Function* >(this);
   auto cls = GetClass();
   if(cls != nullptr) return cls->GetTemplate();
   return nullptr;
}

//------------------------------------------------------------------------------

CxxScope* Function::GetTemplateInstance() const
{
   if(tmplt_ != nullptr) return (CxxScope*) this;
   return CxxScope::GetTemplateInstance();
}

//------------------------------------------------------------------------------

TemplateType Function::GetTemplateType() const
{
   if(IsTemplate()) return FuncTemplate;

   //  An inline function in a class template is treated as a regular function
   //  because it is not copied into template instances.
   //
   if(inline_) return NonTemplate;

   auto cls = GetClass();

   if(cls != nullptr)
   {
      if(cls->IsTemplate()) return ClassTemplate;
   }

   return NonTemplate;
}

//------------------------------------------------------------------------------

void Function::GetUsages(const CodeFile& file, CxxUsageSets& symbols)
{
   if(deleted_) return;

   //  See if this function appears in a function or class template.
   //
   switch(GetTemplateType())
   {
   case NonTemplate:
      //
      //  This could be a regular function or a function in a template
      //  instance.
      //
      break;

   case FuncTemplate:
      //
      //  This is a function template, so obtain usage information from its
      //  first instance in case some symbols in the template could not be
      //  resolved.
      //
      if(!tmplts_.empty())
      {
         CxxUsageSets sets;
         auto first = tmplts_.front();
         first->GetUsages(file, sets);
         sets.EraseTemplateArgs(first->GetTemplateArgs());
         sets.EraseLocals();
         symbols.Union(sets);
         return;
      }
      break;

   case ClassTemplate:
      //
      //  This function appears in a class template, which pulls its usages
      //  from its first instance, in the same way as above.
      //
      return;
   }

   //  Place the symbols used in the function's signature in a local variable.
   //  The reason for this is discussed below.
   //
   CxxUsageSets usages;

   if(parms_ != nullptr) parms_->GetUsages(file, usages);
   if(spec_ != nullptr) spec_->GetUsages(file, usages);

   for(size_t i = (this_ ? 1 : 0); i < args_.size(); ++i)
   {
      args_[i]->GetUsages(file, usages);
   }

   //  The symbols in a function signature are always visible
   //  o in the definition (if separate from the declaration)
   //  o in an overridden function (which must #include the base class)
   //  Consequently, symbols used in the signature only need to be reported
   //  (for the purpose of determining which files to #include) when they
   //  appear in the declaration of a new function.  Symbols accessed via a
   //  using statement, however, must be reported because a using statement
   //  is still needed.  To support the creation of a global cross-reference,
   //  symbols that were previously unreported for an override or definition
   //  are now reported as "inherited".
   //
   auto first = !IsOverride() && !defn_;

   for(auto d = usages.directs.cbegin(); d != usages.directs.cend(); ++d)
   {
      if(first)
         symbols.AddDirect(*d);
      else
         symbols.AddInherit(*d);
   }

   for(auto i = usages.indirects.cbegin(); i != usages.indirects.cend(); ++i)
   {
      if(first)
         symbols.AddIndirect(*i);
      else
         symbols.AddInherit(*i);
   }

   for(auto f = usages.forwards.cbegin(); f != usages.forwards.cend(); ++f)
   {
      if(first)
         symbols.AddForward(*f);
      else
         symbols.AddInherit(*f);
   }

   for(auto f = usages.friends.cbegin(); f != usages.friends.cend(); ++f)
   {
      if(first)
         symbols.AddForward(*f);
      else
         symbols.AddInherit(*f);
   }

   for(auto u = usages.users.cbegin(); u != usages.users.cend(); ++u)
   {
      symbols.AddUser(*u);
   }

   //  If this is an override, report the original function declaration for
   //  cross-reference purposes.
   //
   if(IsOverride())
   {
      symbols.AddInherit(FindRootFunc());
   }

   //  If this is a function definition, include the declaration as a usage.
   //
   if(defn_) symbols.AddDirect(mate_);

   //  If this is a constructor, include usages from the base class constructor
   //  call, the member initializations, and the default member initializations.
   //  Only the constructor's definition has the first two, but make sure the
   //  last one is only done for the definition, and only for non-POD members.
   //
   if(FuncType() == FuncCtor)
   {
      const Class* cls = GetClass();

      if(call_ != nullptr) call_->GetUsages(file, symbols);

      for(auto m = mems_.cbegin(); m != mems_.cend(); ++m)
      {
         (*m)->GetUsages(file, symbols);
      }

      if(GetDefn() == this)
      {
         auto data = cls->Datas();

         for(auto d = data->cbegin(); d != data->cend(); ++d)
         {
            auto item = d->get();

            if(!item->IsStatic() && !item->IsPOD())
            {
               item->GetUsages(file, symbols);
               item->GetDirectTemplateArgs(symbols);
            }
         }
      }
   }

   if(impl_ != nullptr) impl_->GetUsages(file, symbols);
}

//------------------------------------------------------------------------------

bool Function::HasInvokers() const
{
   Debug::ft("Function.HasInvokers");

   if(defn_) return GetDecl()->HasInvokers();

   //  A non-virtual function must be invoked directly.
   //
   if(calls_ > 0) return true;
   if(!virtual_) return false;

   //  Assume that a virtual function is invoked if any of its overrides
   //  are invoked.  The overrides won't invoke a pure virtual function,
   //  and can't invoke a private virtual function, but pretending that
   //  such a function is invoked prevents it from being logged as unused,
   //  which would be misleading when its declaration is required.
   //
   for(auto f = overs_.cbegin(); f != overs_.cend(); ++f)
   {
      if((*f)->HasInvokers()) return true;
   }

   return false;
}

//------------------------------------------------------------------------------

void Function::IncrThisReads() const
{
   Debug::ft("Function.IncrThisReads");

   if(this_) args_[0]->WasRead();
}

//------------------------------------------------------------------------------

fn_name Function_IncrThisWrites = "Function.IncrThisWrites";

void Function::IncrThisWrites() const
{
   Debug::ft(Function_IncrThisWrites);

   if(!this_) return;

   auto arg = args_[0].get();
   arg->WasWritten(nullptr, false, true);
   arg->SetNonConst();

   if(arg->IsConst())
   {
      Context::SwLog(Function_IncrThisWrites, "Function cannot be const", 0);
   }
}

//------------------------------------------------------------------------------

fn_name Function_InstantiateError = "Function.InstantiateError";

Function* Function::InstantiateError(const string& instName, debug64_t offset)
{
   Debug::ft(Function_InstantiateError);

   auto expl = "Failed to instantiate " + instName;
   Context::SwLog(Function_InstantiateError, expl, offset);
   return nullptr;
}

//------------------------------------------------------------------------------

Function* Function::InstantiateFunction(const TypeName* type) const
{
   Debug::ft("Function.InstantiateFunction(type)");

   //  Create the name for the function template instance and look for it.
   //  If it has already been instantiated, return it.
   //
   auto ts = type->TypeString(true);
   auto instName = Name() + ts;
   RemoveRefs(instName);
   auto area = GetArea();
   auto func =
      area->FindFunc(instName, nullptr, nullptr, false, nullptr, nullptr);
   if(func != nullptr) return func;

   //  Notify TYPE, which contains the template name and arguments, that its
   //  template is being instantiated.  This causes the instantiation of any
   //  templates on which this one depends.
   //
   CxxScopedVector locals;
   type->Instantiating(locals);

   //  Get the code for the function template, assembling it if this is the
   //  first instantiation.
   //
   if(code_ == nullptr)
   {
      std::ostringstream stream;
      Flags options(FQ_Mask | Code_Mask | NoAC_Mask | NoTP_Mask);
      Display(stream, EMPTY_STR, options);
      code_.reset(new string(stream.str()));
   }

   stringPtr code(new string(*code_));
   if(code->empty()) return InstantiateError(instName, 0);

   //  A function template in a substitute file (e.g. std::move) does not
   //  have an implementation, which will cause Parser.GetProcDefn to fail.
   //  Replacing the final semicolon with braces overcomes this.
   //
   if(code->back() == CRLF) code->pop_back();

   if(code->back() == ';')
   {
      code->pop_back();
      code->append("{ }");
   }

   //  Replace occurrences of the function template name with the function
   //  instance name.
   //
   Replace(*code, QualifiedName(true, false), instName, 0, string::npos);

   //  If the code was obtained from the function's definition, there could
   //  be scopes before its name.  Remove them.
   //
   auto end = code->find(instName);
   if(end == string::npos) return InstantiateError(instName, 1);
   auto begin = code->rfind(SPACE, end);
   if(begin == string::npos) return InstantiateError(instName, 2);
   code->erase(begin + 1, end - begin - 1);

   //  Replace template parameters with the corresponding template arguments.
   //
   ReplaceTemplateParms(*code, type->Args(), 0);

   //  Create a parser and tell it to parse the function template instance.
   //  Once it is parsed, set its access control to that of the template
   //  function and register it as one of that function's instances.
   //
   auto fullName = ScopedName(true) + ts;
   RemoveRefs(fullName);
   ParserPtr parser(new Parser(EMPTY_STR));

   if(!locals.empty())
   {
      for(auto item = locals.cbegin(); item != locals.cend(); ++item)
      {
         Context::InsertLocal(*item);
      }
   }

   parser->ParseFuncInst(fullName, this, area, type, code);
   parser.reset();
   code.reset();

   func = area->FindFunc(instName, nullptr, nullptr, false, nullptr, nullptr);
   if(func == nullptr) return InstantiateError(instName, 3);
   tmplts_.push_back(func);
   return func;
}

//------------------------------------------------------------------------------

fn_name Function_InstantiateFunction2 = "Function.InstantiateFunction(args)";

Function* Function::InstantiateFunction(stringVector& tmpltArgs) const
{
   Debug::ft(Function_InstantiateFunction2);

   //  The number of type strings in tmpltArgs should be the same as the number
   //  of template parameters, and each parameter should have an argument.
   //
   auto parms = GetTemplateParms()->Parms();

   if(tmpltArgs.size() != parms->size())
   {
      auto expl = "Invalid number of template arguments for " + Name();
      Context::SwLog(Function_InstantiateFunction2, expl, tmpltArgs.size());
      return nullptr;
   }

   for(size_t i = 0; i < parms->size(); ++i)
   {
      if(tmpltArgs[i].empty()) return nullptr;
   }

   //  Build the TypeName for the function instance and instantiate it.
   //
   auto name = Name();
   TypeNamePtr type(new TypeName(name));
   auto scope = Context::Scope();
   ParserPtr parser(new Parser(scope));

   for(size_t i = 0; i < parms->size(); ++i)
   {
      TypeSpecPtr arg;
      parser->ParseTypeSpec(tmpltArgs[i], arg);
      if(arg == nullptr) return nullptr;
      type->AddTemplateArg(arg);
   }

   parser.reset();
   return InstantiateFunction(type.get());
}

//------------------------------------------------------------------------------

fn_name Function_Invoke = "Function.Invoke";

Warning Function::Invoke(StackArgVector* args)
{
   Debug::ft(Function_Invoke);

   auto size1 = (args != nullptr ? args->size() : 0);
   auto size2 = args_.size();

   if(size1 > size2)
   {
      auto expl = "Too many arguments for " + Name();
      Context::SwLog(Function_Invoke, expl, size1 - size2);
      size1 = size2;
   }

   auto func = Context::Scope()->GetFunction();

   //  Register a read on each sent argument and check its assignment to
   //  the received argument.
   //
   for(size_t i = 0; i < size1; ++i)
   {
      auto& sendArg = args->at(i);
      sendArg.WasRead();
      StackArg recvArg(args_.at(i).get(), 0, false);
      AdjustRecvConstness(func, recvArg);
      sendArg.AssignedTo(recvArg, Passed);
   }

   //  Push the function's result onto the stack and increment the number
   //  of calls to it.
   //
   Context::PushArg(ResultType());
   Context::WasCalled(this);

   //  Generate a warning if a constructor or destructor invoked a
   //  standard virtual function that is overridden by one of its
   //  subclasses but not by its own class.
   //
   if(IsVirtual() && (FuncType() == FuncStandard))
   {
      if(func != nullptr)
      {
         auto type = func->FuncType();

         if((type == FuncCtor) || (type == FuncDtor))
         {
            auto cls = func->GetClass();

            if(cls->ClassDistance(GetClass()) != NOT_A_SUBCLASS)
            {
               if(IsOverriddenAtOrBelow(cls))
               {
                  return VirtualFunctionInvoked;
               }
            }
         }
      }
   }

   return Warning_N;
}

//------------------------------------------------------------------------------

void Function::InvokeDefaultBaseCtor() const
{
   Debug::ft("Function.InvokeDefaultBaseCtor");

   auto cls = GetClass();
   if(cls == nullptr) return;
   auto base = cls->BaseClass();
   if(base == nullptr) return;
   auto ctor = base->FindCtor(nullptr);
   if(ctor == nullptr) return;
   ctor->WasCalled();
   ctor->RecordAccess(Cxx::Protected);
}

//------------------------------------------------------------------------------

bool Function::IsDeleted() const
{
   Debug::ft("Function.IsDeleted");

   if(deleted_) return true;

   //  A private constructor, operator=, or operator new, usually serves to
   //  prohibit stack allocation, copying, or heap allocation, respectively.
   //
   if(GetAccess() == Cxx::Private)
   {
      switch(FuncType())
      {
      case FuncCtor:
         return true;
      case FuncOperator:
         switch(Operator())
         {
         case Cxx::ASSIGN:
         case Cxx::OBJECT_CREATE:
         case Cxx::OBJECT_CREATE_ARRAY:
            return true;
         }
         break;
      }
   }

   return false;
}

//------------------------------------------------------------------------------

bool Function::IsImplemented() const
{
   return ((GetDefn()->impl_ != nullptr) || IsDefaulted());
}

//------------------------------------------------------------------------------

bool Function::IsInvokedInBase() const
{
   Debug::ft("Function.IsInvokedInBase");

   if(defn_) return GetDecl()->IsInvokedInBase();

   for(auto b = base_; b != nullptr; b = b->base_)
   {
      if(b->calls_ > 0) return true;
   }

   return false;
}

//------------------------------------------------------------------------------

bool Function::IsOverriddenAtOrBelow(const Class* cls) const
{
   Debug::ft("Function.IsOverriddenAtOrBelow");

   for(auto f = overs_.cbegin(); f != overs_.cend(); ++f)
   {
      auto over = (*f)->GetClass();
      if(over->ScopeDistance(cls) != NOT_A_SUBCLASS) return true;
      if((*f)->IsOverriddenAtOrBelow(cls)) return true;
   }

   return false;
}

//------------------------------------------------------------------------------

bool Function::IsTemplateArg(const Argument* arg) const
{
   Debug::ft("Function.IsTemplateArg");

   //  For ARG to be a template argument, it must be a template parameter
   //  in its template.
   //
   auto inst = GetTemplateInstance();
   if(inst == nullptr) return false;
   auto that = static_cast< const Argument* >(FindTemplateAnalog(arg));
   if(that == nullptr) return false;
   return (that->GetTypeSpec()->GetTemplateRole() == TemplateParameter);
}

//------------------------------------------------------------------------------

bool Function::IsTrivial() const
{
   Debug::ft("Function.IsTrivial");

   if(IsDefaulted()) return true;
   if(tmplt_ != nullptr) return false;

   auto defn = GetDefn();
   if(defn->impl_ == nullptr) return false;
   size_t begin, end;
   if(!defn->GetSpan2(begin, end)) return false;

   auto& lexer = defn->GetFile()->GetLexer();
   auto last = lexer.GetLineNum(end);
   auto body = false;

   for(auto n = lexer.GetLineNum(begin); n <= last; ++n)
   {
      auto type = lexer.LineToType(n);

      if(!LineTypeAttr::Attrs[type].isCode) continue;

      switch(type)
      {
      case OpenBrace:
      case DebugFt:
         body = true;
         break;

      case CloseBrace:
         return true;

      case CodeLine:
         if(body) return false;
      }
   }

   return true;
}

//------------------------------------------------------------------------------

bool Function::IsUndefined() const
{
   Debug::ft("Function.IsUndefined");

   if(GetDefn()->impl_ == nullptr) return true;
   if(IsDeleted()) return true;
   return false;
}

//------------------------------------------------------------------------------

bool Function::IsUnused() const
{
   Debug::ft("Function.IsUnused");

   //  If a function template has no specializations, it is unused.
   //
   if(IsTemplate()) return tmplts_.empty();

   //  Assume that destructors are used, and do not flag deleted
   //  functions as unused.
   //
   auto type = FuncType();
   if(type == FuncDtor) return false;
   if(IsDeleted()) return false;

   //  Look for invocations of the function.  A virtual function
   //  can be invoked through a base class.
   //
   if(HasInvokers()) return false;
   if(type == FuncCtor) return true;
   if(IsInvokedInBase()) return false;
   return true;
}

//------------------------------------------------------------------------------

bool Function::IsUnusedCopyFunction() const
{
   Debug::ft("Function.IsUnusedCopyFunction");

   auto cls = GetClass();
   if(cls == nullptr) return false;
   auto func = cls->FindFuncByRole(PureDtor, false);
   if(func != nullptr) return false;
   func = cls->FindFuncByRole(CopyCtor, false);
   if((func != nullptr) && func->HasInvokers()) return false;
   func = cls->FindFuncByRole(CopyOper, false);
   if((func != nullptr) && func->HasInvokers()) return false;
   return true;
}

//------------------------------------------------------------------------------

void Function::ItemAccessed(const CxxNamed* item, const StackArg* via)
{
   Debug::ft("Function.ItemAccessed");

   //  This currently determines if this function
   //  o accessed a non-public member in its own class or a base class;
   //  o accessed non-static data in its own class or a base class.
   //  The purpose of this is to see if the function could be static or free.
   //
   auto thisClass = GetClass();
   if(thisClass == nullptr) return;
   auto thatClass = item->GetClass();
   if(thatClass == nullptr) return;

   if(thisClass != thatClass)
   {
      //  Return if the item is accessing an item outside its class hierarchy.
      //
      if(!thisClass->DerivesFrom(thatClass)) return;
   }

   //  If the function takes an argument or declares local data whose type is
   //  not public, this prevents it from being free, although it could still
   //  be static.
   //
   auto spec = item->GetTypeSpec();
   if(spec != nullptr)
   {
      auto ref = spec->Referent();
      if(ref != nullptr)
      {
         if(ref->GetAccess() != Cxx::Public) SetNonPublic();
      }
   }

   //  Check for "this" explicitly.  Its referent is a class, which is usually
   //  public, and it is (implicitly) declared in the function, so it is about
   //  to escape detection.
   //
   if(item->Name() == THIS_STR)
   {
      SetNonPublic();
      SetNonStatic();
      return;
   }

   if(item->IsDeclaredInFunction()) return;
   if(item->GetAccess() != Cxx::Public) SetNonPublic();

   if((via == nullptr) || via->IsThis())
   {
      if(!item->IsStatic()) SetNonStatic();
   }
}

//------------------------------------------------------------------------------

bool Function::LocateItem(const CxxToken* item, size_t& n) const
{
   Debug::ft("Function.LocateItem");

   for(auto a = args_.cbegin(); a != args_.cend(); ++a)
   {
      if((*a)->LocateItem(item, n)) return true;
   }

   if(impl_ == nullptr) return false;
   return impl_->LocateItem(item, n);
}

//------------------------------------------------------------------------------

size_t Function::LogOffsetToArgIndex(word offset) const
{
   return (this_ ? offset : offset - 1);
}

//------------------------------------------------------------------------------

void Function::LogToArg(Warning warning, size_t index) const
{
   Debug::ft("Function.LogToArg");

   auto arg = GetArgs().at(index).get();
   arg->Log(warning, this, index + (this_ ? 0 : 1));
}

//------------------------------------------------------------------------------

TypeMatch Function::MatchTemplate(const string& thisType,
   const string& thatType, stringVector& tmpltParms,
   stringVector& tmpltArgs, bool& argFound)
{
   Debug::ft("Function.MatchTemplate");

   //  Create TypeSpecs for thisType and thatType by invoking a new parser.
   //  Parsing requires a scope, so use the current one.  Note that const
   //  qualification is stripped when deducing a template argument.
   //
   auto thatNonCVType = RemoveConsts(thatType);
   TypeSpecPtr thisSpec;
   TypeSpecPtr thatSpec;

   auto scope = Context::Scope();
   ParserPtr parser(new Parser(scope));
   parser->ParseTypeSpec(thisType, thisSpec);
   parser->ParseTypeSpec(thatNonCVType, thatSpec);
   parser.reset();

   if(thisSpec == nullptr) return Incompatible;
   if(thatSpec == nullptr) return Incompatible;
   thisSpec->SetTemplateRole(TemplateClass);
   return thisSpec->MatchTemplate
      (thatSpec.get(), tmpltParms, tmpltArgs, argFound);
}

//------------------------------------------------------------------------------

StackArg Function::MemberToArg(StackArg& via, TypeName* name, Cxx::Operator op)
{
   Debug::ft("Function.MemberToArg");

   //  Push this function and return VIA as its "this" argument.  When a class
   //  has both a static and a member function with the same name, name lookup
   //  may initially select the wrong function.  This is handled by
   //  o always pushing an implicit "this" argument if the context function
   //    is a member function (see PushThisArg);
   //  o having static functions ignore any "this" argument during argument
   //    matching (see CanInvokeWith);
   //  o discarding an unnecessary "this" argument when a static function is
   //    selected as the result of argument matching (see UpdateThisArg).
   //
   Accessed(&via);
   Context::PushArg(StackArg(this, name, via));
   if(op == Cxx::REFERENCE_SELECT) via.IncrPtrs();
   via.SetAsThis(true);
   return via;
}

//------------------------------------------------------------------------------

size_t Function::MinArgs() const
{
   Debug::ft("Function.MinArgs");

   size_t min = 0;

   for(auto a = args_.cbegin(); a != args_.cend(); ++a)
   {
      if((*a)->HasDefault()) break;
      ++min;
   }

   return min;
}

//------------------------------------------------------------------------------

bool Function::NameRefersToItem(const string& name,
   const CxxScope* scope, CodeFile* file, SymbolView& view) const
{
   Debug::ft("Function.NameRefersToItem");

   //  If this isn't a function template instance, invoke the base class
   //  version.
   //
   if(tspec_ == nullptr)
   {
      return CxxScoped::NameRefersToItem(name, scope, file, view);
   }

   //  Split NAME into its component (template name and arguments).  If it
   //  refers to this function instance's template, see if also refers to
   //  its template arguments.
   //
   //  NOTE: This has not been tested.  Nothing in the code base caused its
   //  ====  execution, but it is identical to ClassInst.NameRefersToItem.
   //
   auto names = GetNameAndArgs(name);
   auto syms = Singleton< CxxSymbols >::Instance();
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

CxxToken* Function::PosToItem(size_t pos) const
{
   auto item = CxxScope::PosToItem(pos);
   if(item != nullptr) return item;

   item = name_->PosToItem(pos);
   if(item != nullptr) return item;

   if(parms_ != nullptr) item = parms_->PosToItem(pos);
   if(item != nullptr) return item;

   if(spec_ != nullptr) item = spec_->PosToItem(pos);
   if(item != nullptr) return item;

   for(auto a = args_.cbegin(); a != args_.cend(); ++a)
   {
      item = (*a)->PosToItem(pos);
      if(item != nullptr) return item;
   }

   if(call_ != nullptr) item = call_->PosToItem(pos);
   if(item != nullptr) return item;

   for(auto m = mems_.cbegin(); m != mems_.cend(); ++m)
   {
      item = (*m)->PosToItem(pos);
      if(item != nullptr) return item;
   }

   return (impl_ != nullptr ? impl_->PosToItem(pos) : nullptr);
}

//------------------------------------------------------------------------------

void Function::PushThisArg(StackArgVector& args) const
{
   Debug::ft("Function.PushThisArg");

   //  Return if this function doesn't take a "this" argument.
   //
   if(!this_) return;

   if(args.empty() || !args.front().IsThis())
   {
      //  A "this" argument hasn't been pushed.  If the context function
      //  has one, push it as an implicit "this" argument.  If this is a
      //  constructor, however, there may not be a context function (e.g.
      //  during static member initialization), or the context function
      //  may belong to another class, so push our own "this" argument.
      //
      if(FuncType() != FuncCtor)
      {
         auto func = Context::Scope()->GetFunction();
         if((func == nullptr) || !func->this_) return;
         StackArg arg(func->args_[0].get(), 0, false);
         args.insert(args.cbegin(), arg);
      }
      else
      {
         args.insert(args.cbegin(), StackArg(GetClass(), 1, false));
      }

      args.front().SetAsImplicitThis();
   }
}

//------------------------------------------------------------------------------

void Function::RecordUsage()
{
   Debug::ft("Function.RecordUsage");

   if(tmplt_ == nullptr)
      AddUsage();
   else
      tmplt_->RecordUsage();
}

//------------------------------------------------------------------------------

StackArg Function::ResultType() const
{
   Debug::ft("Function.ResultType");

   //  Constructors and destructors have no return type.
   //
   if(spec_ != nullptr) return spec_->ResultType();
   if(FuncType() == FuncCtor) return StackArg(GetClass(), 0, true);
   return StackArg(Singleton< CxxRoot >::Instance()->VoidTerm(), 0, false);
}

//------------------------------------------------------------------------------

void Function::SetBaseInit(ExprPtr& init)
{
   Debug::ft("Function.SetBaseInit");

   call_.reset(init.release());
}

//------------------------------------------------------------------------------

void Function::SetDefn(Function* func)
{
   Debug::ft("Function.SetDefn");

   func->mate_ = this;
   func->defn_ = true;
   this->mate_ = func;

   //  Set the referent on each name in FUNC's (the definition's) qualified
   //  name.  They can be set from the declaration's scopes.
   //
   auto qname = func->GetQualName();
   CxxScope* scope = this;

   for(auto i = func->GetQualName()->Size() - 1; i != SIZE_MAX; --i)
   {
      qname->SetReferentN(i, scope, nullptr);
      scope = scope->GetScope();
   }
}

//------------------------------------------------------------------------------

void Function::SetImpl(BlockPtr& block)
{
   Debug::ft("Function.SetImpl");

   impl_.reset(block.release());

   //  This is invoked when
   //  o The definition of a previously declared function is encountered.
   //    EnterScope will be invoked on this new instance momentarily.
   //  o A function is simultaneously declared and defined in a class (an
   //    inline).  In this case, parsing of the implementation is delayed
   //    until the class has been parsed.  EnterScope was already invoked
   //    on this instance and will not be invoked again, so EnterBlock must
   //    be invoked now.
   //  o A function is simultaneously declared and defined at file scope (in
   //    a namespace).  When EnterScope is invoked, the function will notice
   //    that its code is also present.
   //
   if(found_) EnterBlock();
}

//------------------------------------------------------------------------------

void Function::SetNonPublic()
{
   Debug::ft("Function.SetNonPublic");

   if(nonpublic_) return;
   nonpublic_ = true;
   auto func = static_cast< Function* >(FindTemplateAnalog(this));
   if(func != nullptr) func->nonpublic_ = true;
}

//------------------------------------------------------------------------------

void Function::SetNonStatic()
{
   Debug::ft("Function.SetNonStatic");

   if(nonstatic_) return;
   nonstatic_ = true;
   auto func = static_cast< Function* >(FindTemplateAnalog(this));
   if(func != nullptr) func->nonstatic_ = true;
}

//------------------------------------------------------------------------------

void Function::SetOperator(Cxx::Operator oper)
{
   Debug::ft("Function.SetOperator");

   //  Verify that the number of arguments is correct for OPER.  Since we
   //  assume that the code is well-formed, this doesn't check correctness.
   //  What it does do is update ambiguous operators, based on the number
   //  of arguments.  For example, operator& is initially interpreted as
   //  Cxx::ADDRESS_OF, but it might actually be Cxx::BITWISE_AND.
   //
   if(oper != Cxx::NIL_OPERATOR)
   {
      auto count = args_.size();
      if(oper == Cxx::CAST) ++count;
      CxxOp::UpdateOperator(oper, count);
      name_->SetOperator(oper);
   }

   //  Adding the function to the symbol table was deferred until now in
   //  case an operator symbol had not yet been appended to its name.
   //
   Singleton< CxxSymbols >::Instance()->InsertFunc(this);
}

//------------------------------------------------------------------------------

void Function::SetStatic(bool stat, Cxx::Operator oper)
{
   Debug::ft("Function.SetStatic");

   static_ = stat;
   if(static_) return;

   switch(oper)
   {
   case Cxx::OBJECT_CREATE:
   case Cxx::OBJECT_CREATE_ARRAY:
   case Cxx::OBJECT_DELETE:
   case Cxx::OBJECT_DELETE_ARRAY:
      static_ = (Context::Scope()->GetClass() != nullptr);
   }
}

//------------------------------------------------------------------------------

void Function::SetTemplateArgs(const TypeName* spec)
{
   Debug::ft("Function.SetTemplateArgs");

   tspec_.reset(new TypeName(*spec));
   tspec_->CopyContext(spec, true);
}

//------------------------------------------------------------------------------

void Function::SetTemplateParms(TemplateParmsPtr& parms)
{
   Debug::ft("Function.SetTemplateParms");

   parms_ = std::move(parms);
}

//------------------------------------------------------------------------------

void Function::Shrink()
{
   CxxScope::Shrink();
   name_->Shrink();
   if(parms_ != nullptr) parms_->Shrink();
   if(spec_ != nullptr) spec_->Shrink();

   for(auto a = args_.cbegin(); a != args_.cend(); ++a)
   {
      (*a)->Shrink();
   }

   if(call_ != nullptr) call_->Shrink();

   for(auto m = mems_.cbegin(); m != mems_.cend(); ++m)
   {
      (*m)->Shrink();
   }

   if(impl_ != nullptr) impl_->Shrink();
   tmplts_.shrink_to_fit();
   overs_.shrink_to_fit();

   auto size = args_.capacity() * sizeof(ArgumentPtr);
   size += (mems_.capacity() * sizeof(TokenPtr));
   size += (tmplts_.capacity() * sizeof(Function*));
   size += (overs_.capacity() * sizeof(Function*));
   size += XrefSize();
   CxxStats::Vectors(CxxStats::FUNCTION, size);
}

//------------------------------------------------------------------------------

bool Function::SignatureMatches(const Function* that, bool base) const
{
   Debug::ft("Function.SignatureMatches");

   //  The functions match if they have the same number of arguments and
   //  their return types and arguments also match.
   //
   if(!ArgumentsMatch(that)) return false;

   size_t i = 0;

   if(base && this->this_ && that->this_)
   {
      //  THIS can be a subclass of THAT.  They both have "this" arguments,
      //  so check them accordingly.  However, the functions must also have
      //  the same constness.
      //
      if(this->const_ != that->const_) return false;
      auto thisCls = this->GetClass();
      auto thatCls = that->GetClass();
      if(thisCls->ClassDistance(thatCls) == NOT_A_SUBCLASS) return false;
      i = 1;
   }

   auto thisSize = this->args_.size();

   for(NO_OP; i < thisSize; ++i)
   {
      auto s1 = this->args_.at(i)->GetTypeSpec();
      auto s2 = that->args_.at(i)->GetTypeSpec();
      if(!s1->MatchesExactly(s2)) return false;
   }

   return true;
}

//------------------------------------------------------------------------------

fn_name Function_TypeString = "Function.TypeString";

string Function::TypeString(bool arg) const
{
   //  The full type begins with the function's return type, but constructors
   //  and destructors don't have one.  For a constructor, return a pointer to
   //  the class.  For a destructor, return void.
   //
   string ts;

   if(spec_ != nullptr)
   {
      ts = spec_->TypeString(arg);
   }
   else
   {
      auto ft = FuncType();

      switch(ft)
      {
      case FuncCtor:
         ts = GetClass()->Name() + '*';
         break;
      case FuncDtor:
         ts = VOID_STR;
         break;
      default:
         auto expl = "Return type not found for " + Name();
         Context::SwLog(Function_TypeString, expl, ft);
         return ERROR_STR;
      }
   }

   //  If the function is not an argument, include its fully qualified
   //  name after its return type.  (When the function is an argument,
   //  only its signature, and not its name, is included.)
   //
   if(!arg)
   {
      ts += SPACE + Prefix(GetScope()->TypeString(false)) + Name();
   }

   //  Append the function's argument types.
   //
   ts.push_back('(');

   for(auto a = args_.cbegin(); a != args_.cend(); ++a)
   {
      ts += (*a)->TypeString(arg);
      if(*a != args_.back()) ts.push_back(',');
   }

   ts.push_back(')');
   return ts;
}

//------------------------------------------------------------------------------

void Function::UpdatePos
   (EditorAction action, size_t begin, size_t count, size_t from) const
{
   CxxScope::UpdatePos(action, begin, count, from);
   name_->UpdatePos(action, begin, count, from);
   if(parms_ != nullptr) parms_->UpdatePos(action, begin, count, from);
   if(spec_ != nullptr) spec_->UpdatePos(action, begin, count, from);

   for(auto a = args_.cbegin(); a != args_.cend(); ++a)
   {
      (*a)->UpdatePos(action, begin, count, from);
   }

   if(call_ != nullptr) call_->UpdatePos(action, begin, count, from);

   for(auto m = mems_.cbegin(); m != mems_.cend(); ++m)
   {
      (*m)->UpdatePos(action, begin, count, from);
   }

   if(impl_ != nullptr) impl_->UpdatePos(action, begin, count, from);
}

//------------------------------------------------------------------------------

void Function::UpdateThisArg(StackArgVector& args) const
{
   Debug::ft("Function.UpdateThisArg");

   if(!this_)
   {
      if(!args.empty() && args.front().IsThis())
      {
         //  See if an unnecessary "this" argument exists.  This occurs when
         //  name resolution initially selects a member function instead of
         //  a static function with the same name.  When argument matching
         //  corrects this to the static function, it discards the "this"
         //  argument here.
         //
         if(!args.front().IsImplicitThis())
         {
            auto file = Context::File();

            if(file != nullptr)
            {
               auto pos = Context::GetPos();
               auto item = static_cast< CxxNamed* >(args.front().item_);
               file->LogPos
                  (pos, StaticFunctionViaMember, item, 0, GetClass()->Name());
            }
         }

         args.erase(args.cbegin());
      }
   }
   else
   {
      //  Set a constructor's "this" argument to its actual "this" argument.
      //  A pointer to the class acts as the implicit "this" argument until
      //  the constructor is found.
      //
      if(FuncType() == FuncCtor)
      {
         args.front() = StackArg(args_[0].get(), 0, false);
      }
   }
}

//------------------------------------------------------------------------------

void Function::UpdateXref(bool insert)
{
   if(deleted_) return;

   auto type = GetTemplateType();

   switch(type)
   {
   case NonTemplate:
      //
      //  This includes a function in a template instance.  It normally doesn't
      //  update the cross-reference, since it has no code file or line numbers.
      //  However, a function in a template cannot resolve an item accessed by a
      //  template parameter, so it uses a template instance to add that item to
      //  the cross-reference.
      //
      if(IsInTemplateInstance())
      {
         if(Context::GetXrefUpdater() != TemplateFunction) return;
         Context::PushXrefFrame(InstanceFunction);
      }
      else
      {
         Context::PushXrefFrame(StandardFunction);
      }
      break;

   default:
      //
      //  This is either a function template or a function in a class template.
      //
      Context::PushXrefFrame(TemplateFunction);
   };

   if(defn_) name_->UpdateXref(insert);

   if(parms_ != nullptr) parms_->UpdateXref(insert);
   if(spec_ != nullptr) spec_->UpdateXref(insert);

   for(size_t i = (this_ ? 1 : 0); i < args_.size(); ++i)
   {
      args_[i]->UpdateXref(insert);
   }

   if(base_ != nullptr)
   {
      //  Record an override as a reference to the original declaration of
      //  the virtual function.  If the override appears in a template, the
      //  function's template analog should be considered the override.
      //
      if(FuncType() == FuncStandard)
      {
         base_->UpdateReference(this, insert);
      }
   }

   if(call_ != nullptr)
   {
      call_->UpdateXref(insert);
   }

   for(auto m = mems_.cbegin(); m != mems_.cend(); ++m)
   {
      (*m)->UpdateXref(insert);
   }

   if(impl_ != nullptr) impl_->UpdateXref(insert);

   switch(type)
   {
   case NonTemplate:
      break;

   case FuncTemplate:
      //
      //  Add any unresolved symbols to the cross-reference by consulting
      //  our first template instance.
      //
      if(!tmplts_.empty())
      {
         tmplts_.front()->UpdateXref(insert);
      }
      break;

   case ClassTemplate:
   default:
      //
      //  Add any unresolved symbols to the cross-reference by consulting
      //  our analog in first template instance.
      //
      auto instances = GetClass()->Instances();
      if(instances->empty()) break;
      auto func = instances->front()->FindInstanceAnalog(this);
      if(func != nullptr) func->UpdateXref(insert);
   }

   Context::PopXrefFrame();
}

//------------------------------------------------------------------------------

void Function::WasCalled()
{
   Debug::ft("Function.WasCalled");

   //  Don't record a recursive invocation: a function should be logged
   //  as unused if its only invoker is itself.
   //
   auto scope = Context::Scope();
   if(scope == nullptr) return;
   if(scope->GetFunction() == this) return;

   ++GetDecl()->calls_;

   auto type = FuncType();

   switch(type)
   {
   case FuncDtor:
   {
      //  Destruct members and invoke destructors up the class hierarchy.
      //
      GetClass()->DestructMembers();

      auto dtor = GetBase();
      if(dtor != nullptr) dtor->WasCalled();
      break;
   }

   case FuncCtor:
   {
      //  If this is an invocation by a derived class's constructor, set
      //  this constructor as its base.
      //
      auto func = scope->GetFunction();

      if((func != nullptr) && (func->FuncType() == FuncCtor))
      {
         if(func->GetBase() == nullptr) func->GetDecl()->base_ = GetDecl();
      }

      //  Record invocations up the class hierarchy.  Although destructors
      //  are invoked on class members (above), this doesn't invoke their
      //  constructors.  Doing so would add little value because it is done
      //  indirectly, in EnterBlock, and it would also have to consider this
      //  constructor's member initialization statements.
      //
      for(auto ctor = GetBase(); ctor != nullptr; ctor = ctor->base_)
      {
         ++ctor->calls_;
      }
      break;
   }
   }

   //  For a function template instance, record an invocation on the
   //  function template.
   //
   if(tmplt_ != nullptr) ++tmplt_->calls_;

   //  For a function in a class template instance, record an invocation
   //  on the class template's function.
   //
   auto cls = GetClass();

   if(cls != nullptr)
   {
      auto func = static_cast< Function* >(cls->FindTemplateAnalog(this));
      if(func != nullptr) ++func->calls_;
   }
}

//------------------------------------------------------------------------------

bool Function::WasRead()
{
   ++calls_;
   return true;
}

//------------------------------------------------------------------------------

string Function::XrefName(bool templates) const
{
   auto name = CxxScoped::XrefName(templates);

   if(!Singleton< CxxSymbols >::Instance()->IsUniqueName(GetScope(), Name()))
   {
      std::ostringstream stream;
      Flags options(FQ_Mask);

      name.push_back('(');

      for(size_t i = (this_ ? 1 : 0); i < args_.size(); ++i)
      {
         args_[i]->GetTypeSpec()->Print(stream, options);
         if(i < args_.size() - 1) stream << ',';
      }

      name += stream.str();
      name.push_back(')');

      if(const_) name += " const";
   }

   return name;
}

//==============================================================================

SpaceDefn::SpaceDefn(Namespace* ns) :
   space_(ns)
{
   Debug::ft("SpaceDefn.ctor");

   CxxStats::Incr(CxxStats::SPACE_DEFN);
}

//------------------------------------------------------------------------------

SpaceDefn::~SpaceDefn()
{
   Debug::ft("SpaceDefn.dtor");

   GetFile()->EraseSpace(this);
   CxxStats::Decr(CxxStats::SPACE_DEFN);
}

//------------------------------------------------------------------------------

void SpaceDefn::Delete()
{
   Debug::ft("SpaceDefn.Delete");

   space_->UpdateReference(this, false);
   space_->EraseDefn(this);
   delete this;
}

//------------------------------------------------------------------------------

void SpaceDefn::GetDecls(CxxNamedSet& items)
{
   items.insert(this);
}

//------------------------------------------------------------------------------

bool SpaceDefn::GetSpan(size_t& begin, size_t& left, size_t& end) const
{
   Debug::ft("SpaceDefn.GetSpan");

   return GetBracedSpan(begin, left, end);
}

//------------------------------------------------------------------------------

const std::string& SpaceDefn::Name() const
{
   return space_->Name();
}

//------------------------------------------------------------------------------

string SpaceDefn::ScopedName(bool templates) const
{
   return space_->ScopedName(templates);
}

//------------------------------------------------------------------------------

void SpaceDefn::UpdateXref(bool insert)
{
   space_->UpdateReference(this, insert);
}

//==============================================================================

FuncSpec::FuncSpec(FunctionPtr& func) : func_(func.release())
{
   Debug::ft("FuncSpec.ctor");

   CxxStats::Incr(CxxStats::FUNC_SPEC);
}

//------------------------------------------------------------------------------

FuncSpec::~FuncSpec()
{
   Debug::ft("FuncSpec.dtor");

   CxxStats::Decr(CxxStats::FUNC_SPEC);
}

//------------------------------------------------------------------------------

fn_name FuncSpec_Warning = "FuncSpec.Warning";

//------------------------------------------------------------------------------

void FuncSpec::AddArray(ArraySpecPtr& array)
{
   func_->GetTypeSpec()->AddArray(array);
}

//------------------------------------------------------------------------------

string FuncSpec::AlignTemplateArg(const TypeSpec* thatArg) const
{
   return func_->GetTypeSpec()->AlignTemplateArg(thatArg);
}

//------------------------------------------------------------------------------

TagCount FuncSpec::Arrays() const
{
   return func_->GetTypeSpec()->Arrays();
}

//------------------------------------------------------------------------------

void FuncSpec::Check() const
{
   func_->Check();
}

//------------------------------------------------------------------------------

TypeSpec* FuncSpec::Clone() const
{
   Debug::SwLog(FuncSpec_Warning, "Clone", 0);
   return nullptr;
}

//------------------------------------------------------------------------------

bool FuncSpec::ContainsTemplateParameter() const
{
   if(TypeSpec::ContainsTemplateParameter()) return true;
   return func_->ContainsTemplateParameter();
}

//------------------------------------------------------------------------------

void FuncSpec::DisplayArrays(ostream& stream) const
{
   func_->GetTypeSpec()->DisplayArrays(stream);
}

//------------------------------------------------------------------------------

void FuncSpec::DisplayTags(ostream& stream) const
{
   func_->GetTypeSpec()->DisplayTags(stream);
}

//------------------------------------------------------------------------------

void FuncSpec::EnterArrays() const
{
   Debug::SwLog(FuncSpec_Warning, "EnterArrays", 0);
   func_->GetTypeSpec()->EnterArrays();
}

//------------------------------------------------------------------------------

void FuncSpec::EnteringScope(const CxxScope* scope)
{
   Debug::ft("FuncSpec.EnteringScope");

   func_->EnterSignature();
}

//------------------------------------------------------------------------------

void FuncSpec::FindReferent()
{
   Debug::SwLog(FuncSpec_Warning, "FindReferent", 0);
   func_->GetTypeSpec()->FindReferent();
}

//------------------------------------------------------------------------------

TypeTags FuncSpec::GetAllTags() const
{
   return func_->GetTypeSpec()->GetAllTags();
}

//------------------------------------------------------------------------------

void FuncSpec::GetNames(stringVector& names) const
{
   Debug::SwLog(FuncSpec_Warning, "GetNames", 0);
   func_->GetTypeSpec()->GetNames(names);
}

//------------------------------------------------------------------------------

TypeName* FuncSpec::GetTemplateArgs() const
{
   return func_->GetTypeSpec()->GetTemplateArgs();
}

//------------------------------------------------------------------------------

TypeSpec* FuncSpec::GetTypeSpec() const
{
   return func_->GetTypeSpec();
}

//------------------------------------------------------------------------------

bool FuncSpec::HasArrayDefn() const
{
   return func_->GetTypeSpec()->HasArrayDefn();
}

//------------------------------------------------------------------------------

void FuncSpec::Instantiating(CxxScopedVector& locals) const
{
   Debug::SwLog(FuncSpec_Warning, "Instantiating", 0);
   func_->GetTypeSpec()->Instantiating(locals);
}

//------------------------------------------------------------------------------

bool FuncSpec::ItemIsTemplateArg(const CxxNamed* item) const
{
   Debug::SwLog(FuncSpec_Warning, "ItemIsTemplateArg", 0);
   return func_->GetTypeSpec()->ItemIsTemplateArg(item);
}

//------------------------------------------------------------------------------

bool FuncSpec::MatchesExactly(const TypeSpec* that) const
{
   Debug::SwLog(FuncSpec_Warning, "MatchesExactly", 0);
   return func_->GetTypeSpec()->MatchesExactly(that);
}

//------------------------------------------------------------------------------

TypeMatch FuncSpec::MatchTemplate(const TypeSpec* that,
   stringVector& tmpltParms, stringVector& tmpltArgs, bool& argFound) const
{
   Debug::SwLog(FuncSpec_Warning, "MatchTemplate", 0);
   return func_->GetTypeSpec()->MatchTemplate
      (that, tmpltParms, tmpltArgs, argFound);
}

//------------------------------------------------------------------------------

TypeMatch FuncSpec::MatchTemplateArg(const TypeSpec* that) const
{
   Debug::SwLog(FuncSpec_Warning, "MatchTemplateArg", 0);
   return func_->GetTypeSpec()->MatchTemplateArg(that);
}

//------------------------------------------------------------------------------

bool FuncSpec::NamesReferToArgs(const NameVector& names,
   const CxxScope* scope, CodeFile* file, size_t& index) const
{
   Debug::SwLog(FuncSpec_Warning, "NamesReferToArgs", 0);
   return func_->GetTypeSpec()->NamesReferToArgs(names, scope, file, index);
}

//------------------------------------------------------------------------------

CxxToken* FuncSpec::PosToItem(size_t pos) const
{
   auto item = TypeSpec::PosToItem(pos);
   if(item != nullptr) return item;

   return func_->PosToItem(pos);
}

//------------------------------------------------------------------------------

void FuncSpec::Print(ostream& stream, const Flags& options) const
{
   func_->DisplayDecl(stream, NoFlags);
}

//------------------------------------------------------------------------------

TagCount FuncSpec::Ptrs(bool arrays) const
{
   return func_->GetTypeSpec()->Ptrs(arrays);
}

//------------------------------------------------------------------------------

TagCount FuncSpec::Refs() const
{
   return func_->GetTypeSpec()->Refs();
}

//------------------------------------------------------------------------------

StackArg FuncSpec::ResultType() const
{
   return func_->ResultType();
}

//------------------------------------------------------------------------------

void FuncSpec::SetPtrs(TagCount count)
{
   func_->GetTypeSpec()->SetPtrs(count);
}

//------------------------------------------------------------------------------

void FuncSpec::SetReferent(CxxScoped* item, const SymbolView* view) const
{
   Debug::SwLog(FuncSpec_Warning, "SetReferent", 0);
   func_->GetTypeSpec()->SetReferent(item, view);
}

//------------------------------------------------------------------------------

void FuncSpec::Shrink()
{
   TypeSpec::Shrink();
   func_->Shrink();
}

//------------------------------------------------------------------------------

const TypeTags* FuncSpec::Tags() const
{
   return func_->GetTypeSpec()->Tags();
}

//------------------------------------------------------------------------------

TypeTags* FuncSpec::Tags()
{
   return func_->GetTypeSpec()->Tags();
}

//------------------------------------------------------------------------------

string FuncSpec::Trace() const
{
   return func_->TypeString(false);
}

//------------------------------------------------------------------------------

string FuncSpec::TypeString(bool arg) const
{
   return func_->TypeString(arg);
}

//------------------------------------------------------------------------------

string FuncSpec::TypeTagsString(const TypeTags& tags) const
{
   return func_->GetTypeSpec()->TypeTagsString(tags);
}

//------------------------------------------------------------------------------

void FuncSpec::UpdatePos
   (EditorAction action, size_t begin, size_t count, size_t from) const
{
   TypeSpec::UpdatePos(action, begin, count, from);
   func_->UpdatePos(action, begin, count, from);
}

//------------------------------------------------------------------------------

void FuncSpec::UpdateXref(bool insert)
{
   func_->UpdateXref(insert);
}

//==============================================================================

SpaceData::SpaceData(QualNamePtr& name, TypeSpecPtr& type) : Data(type),
   name_(name.release())
{
   Debug::ft("SpaceData.ctor");

   auto qname = name_->QualifiedName(true, false);
   OpenScope(qname);
   CxxStats::Incr(CxxStats::FILE_DATA);
}

//------------------------------------------------------------------------------

SpaceData::~SpaceData()
{
   Debug::ftnt("SpaceData.dtor");

   GetFile()->EraseData(this);
   Singleton< CxxSymbols >::Extant()->EraseData(this);
   CxxStats::Decr(CxxStats::FILE_DATA);
}

//------------------------------------------------------------------------------

void SpaceData::Check() const
{
   Debug::ft("SpaceData.Check");

   Data::Check();

   if(parms_ != nullptr) parms_->Check();

   if(IsDecl())
   {
      CheckUsage();
      CheckConstness(true);
      CheckIfStatic();
      CheckIfInitialized();
   }
}

//------------------------------------------------------------------------------

void SpaceData::CheckIfInitialized() const
{
   Debug::ft("SpaceData.CheckIfInitialized");

   //  Data declared at file scope should be initialized.
   //
   if(!WasInited()) Log(DataUninitialized);
}

//------------------------------------------------------------------------------

void SpaceData::CheckIfStatic() const
{
   Debug::ft("SpaceData.CheckIfStatic");

   //  Data declared at file scope in a header has static linkage (that is,
   //  will have a separate instance for each user of the header) unless it
   //  is defined using constexpr or extern.  This is rarely desirable.
   //
   //  Data declared at file scope in a .cpp has external linkage (that is,
   //  can be made visible by an extern declaration in a header) unless it
   //  is defined as static.  Therefore, if it is not made visible this way,
   //  it is probably intended to be static (that is, private to the .cpp).
   //  This warning is not generated for const data, which cannot be changed.
   //
   if(IsConstexpr()) return;

   auto file = GetFile();

   if(file->IsHeader())
   {
      if(!IsExtern())
      {
         Log(GlobalStaticData);
      }
   }
   else
   {
      if((GetMate() == nullptr) && !IsConst() && !IsStatic())
      {
         Log(DataShouldBeStatic);
      }
   }
}

//------------------------------------------------------------------------------

void SpaceData::Delete()
{
   Debug::ftnt("SpaceData.Delete");

   ClearMate();
   GetArea()->EraseData(this);
   delete this;
}

//------------------------------------------------------------------------------

void SpaceData::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   auto fq = options.test(DispFQ);

   stream << prefix;
   DisplayAlignment(stream, options);
   if(IsExtern()) stream << EXTERN_STR << SPACE;
   if(IsStatic()) stream << STATIC_STR << SPACE;
   if(IsThreadLocal()) stream << THREAD_LOCAL_STR << SPACE;
   if(IsConstexpr()) stream << CONSTEXPR_STR << SPACE;
   GetTypeSpec()->Print(stream, options);
   stream << SPACE;
   strName(stream, fq, name_.get());
   GetTypeSpec()->DisplayArrays(stream);
   DisplayExpression(stream, options);
   DisplayAssignment(stream, options);
   stream << ';';

   if(!options.test(DispCode))
   {
      std::ostringstream buff;
      buff << " // ";
      if(!WasInited())
      {
         buff << "<@";
         if(!options.test(DispStats)) buff << "uninit ";
      }
      DisplayStats(buff, options);
      if(!fq) DisplayFiles(buff);
      auto str = buff.str();
      if(str.size() > 4) stream << str;
   }

   stream << CRLF;
}

//------------------------------------------------------------------------------

bool SpaceData::EnterScope()
{
   Debug::ft("SpaceData.EnterScope");

   //  Note that a separate definition for class static data is
   //  parsed at namespace scope, so it comes through here.
   //
   Context::SetPos(GetLoc());
   if(parms_ != nullptr) parms_->EnterScope();
   ExecuteAlignment();

   auto spec = GetTypeSpec();
   if(GetArea()->FindItem(Name()) != nullptr)
      spec->SetUserType(TS_Definition);
   spec->EnteringScope(this);
   CloseScope();

   //  See whether this is a new declaration or the definition
   //  of previously declared data (i.e. class static data or
   //  data that was declared extern).
   //
   auto decl = GetArea()->FindData(Name());
   auto defn = ((decl != nullptr) && decl->IsPreviousDeclOf(this));

   if(defn)
      decl->SetDefn(this);
   else
      Singleton< CxxSymbols >::Instance()->InsertData(this);

   if(defn || AtFileScope()) GetFile()->InsertData(this);
   ExecuteInit(true);
   return !defn;
}

//------------------------------------------------------------------------------

void SpaceData::GetDecls(CxxNamedSet& items)
{
   if(IsDecl()) items.insert(this);
}

//------------------------------------------------------------------------------

void SpaceData::GetInitName(QualNamePtr& qualName) const
{
   Debug::ft("SpaceData.GetInitName");

   qualName.reset(new QualName(*name_));
   qualName->SetDataInit();
}

//------------------------------------------------------------------------------

CxxToken* SpaceData::PosToItem(size_t pos) const
{
   auto item = Data::PosToItem(pos);
   if(item != nullptr) return item;

   item = name_->PosToItem(pos);
   if(item != nullptr) return item;

   return (parms_ != nullptr ? parms_->PosToItem(pos) : nullptr);
}

//------------------------------------------------------------------------------

void SpaceData::SetTemplateParms(TemplateParmsPtr& parms)
{
   Debug::ft("SpaceData.SetTemplateParms");

   parms_ = std::move(parms);
}

//------------------------------------------------------------------------------

void SpaceData::Shrink()
{
   Data::Shrink();
   CxxStats::Vectors(CxxStats::FILE_DATA, XrefSize());
   name_->Shrink();
   if(parms_ != nullptr) parms_->Shrink();
}

//------------------------------------------------------------------------------

void SpaceData::UpdatePos
   (EditorAction action, size_t begin, size_t count, size_t from) const
{
   Data::UpdatePos(action, begin, count, from);
   name_->UpdatePos(action, begin, count, from);
   if(parms_ != nullptr) parms_->UpdatePos(action, begin, count, from);
}
}
