//==============================================================================
//
//  CxxScope.cpp
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
#include "CxxScope.h"
#include <iterator>
#include <set>
#include <sstream>
#include <utility>
#include "Algorithms.h"
#include "CodeFile.h"
#include "CxxArea.h"
#include "CxxExecute.h"
#include "CxxRoot.h"
#include "CxxSymbols.h"
#include "CxxToken.h"
#include "Debug.h"
#include "Formatters.h"
#include "Lexer.h"
#include "Parser.h"
#include "Singleton.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace CodeTools
{
UsingVector Block::Usings_ = UsingVector();

//------------------------------------------------------------------------------

fn_name Block_ctor = "Block.ctor";

Block::Block(bool braced) :
   name_(LOCALS_STR),
   braced_(braced),
   nested_(false)
{
   Debug::ft(Block_ctor);

   CxxStats::Incr(CxxStats::BLOCK_DECL);
}

//------------------------------------------------------------------------------

fn_name Block_dtor = "Block.dtor";

Block::~Block()
{
   Debug::ft(Block_dtor);

   CxxStats::Decr(CxxStats::BLOCK_DECL);
}

//------------------------------------------------------------------------------

fn_name Block_AddStatement = "Block.AddStatement";

bool Block::AddStatement(CxxToken* s)
{
   Debug::ft(Block_AddStatement);

   statements_.push_back(TokenPtr(s));
   return true;
}

//------------------------------------------------------------------------------

fn_name Block_AddUsing = "Block.AddUsing";

void Block::AddUsing(Using* use)
{
   Debug::ft(Block_AddUsing);

   Usings_.push_back(use);
}

//------------------------------------------------------------------------------

fn_name Block_Check = "Block.Check";

void Block::Check() const
{
   Debug::ft(Block_Check);

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

            stream << CRLF << prefix << spaces(Indent_Size);
            statements_.front()->Print(stream, options);
            break;
         }
      }
      //  [[fallthrough]]
   default:
      if(!nested_) stream << CRLF;
      stream << prefix << '{' << CRLF;
      auto lead = prefix + spaces(Indent_Size);

      for(auto s = statements_.cbegin(); s != statements_.cend(); ++s)
      {
         (*s)->Display(stream, lead, opts);
      }

      stream << prefix << '}';
   }

   stream << CRLF;
}

//------------------------------------------------------------------------------

fn_name Block_EnterBlock = "Block.EnterBlock";

void Block::EnterBlock()
{
   Debug::ft(Block_EnterBlock);

   Context::SetPos(GetPos());
   SetScope(Context::Scope());
   Context::PushScope(this);

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

fn_name Block_GetUsages = "Block.GetUsages";

void Block::GetUsages(const CodeFile& file, CxxUsageSets& symbols) const
{
   Debug::ft(Block_GetUsages);

   for(auto s = statements_.cbegin(); s != statements_.cend(); ++s)
   {
      (*s)->GetUsages(file, symbols);
   }
}

//------------------------------------------------------------------------------

fn_name Block_GetUsingFor = "Block.GetUsingFor";

Using* Block::GetUsingFor(const std::string& name,
   size_t prefix, const CxxNamed* item, const CxxScope* scope) const
{
   Debug::ft(Block_GetUsingFor);

   for(auto u = Usings_.cbegin(); u != Usings_.cend(); ++u)
   {
      if((*u)->IsUsingFor(name, prefix)) return *u;
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

fn_name Block_RemoveUsing = "Block.RemoveUsing";

void Block::RemoveUsing(const Using* use)
{
   Debug::ft(Block_RemoveUsing);

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

fn_name Block_ResetUsings = "Block.ResetUsings";

void Block::ResetUsings()
{
   Debug::ft(Block_ResetUsings);

   Usings_.clear();
}

//------------------------------------------------------------------------------

fn_name Block_ScopedName = "Block.ScopedName";

string Block::ScopedName(bool templates) const
{
   Debug::ft(Block_ScopedName);

   for(auto s = GetScope(); s != nullptr; s = s->GetScope())
   {
      if(s->Type() == Cxx::Function)
      {
         return s->ScopedName(templates) + SCOPE_STR + LOCALS_STR;
      }
   }

   Debug::SwErr(Block_ScopedName, 0, 0);
   return ERROR_STR;
}

//------------------------------------------------------------------------------

void Block::Shrink()
{
   name_.shrink_to_fit();
   CxxStats::Strings(CxxStats::BLOCK_DECL, name_.capacity());

   ShrinkTokens(statements_);
   auto size = statements_.capacity() * sizeof(TokenPtr);
   CxxStats::Vectors(CxxStats::BLOCK_DECL, size);
}

//==============================================================================

fn_name ClassData_ctor = "ClassData.ctor";

ClassData::ClassData(string& name, TypeSpecPtr& type) : Data(type),
   init_(nullptr),
   mutable_(false),
   mutated_(false),
   first_(false),
   last_(false),
   depth_(0)
{
   Debug::ft(ClassData_ctor);

   std::swap(name_, name);
   Singleton< CxxSymbols >::Instance()->InsertData(this);
   OpenScope(name_);
   CxxStats::Incr(CxxStats::CLASS_DATA);
}

//------------------------------------------------------------------------------

fn_name ClassData_dtor = "ClassData.dtor";

ClassData::~ClassData()
{
   Debug::ft(ClassData_dtor);

   CloseScope();
   Singleton< CxxSymbols >::Instance()->EraseData(this);
   CxxStats::Decr(CxxStats::CLASS_DATA);
}

//------------------------------------------------------------------------------

fn_name ClassData_Check = "ClassData.Check";

void ClassData::Check() const
{
   Debug::ft(ClassData_Check);

   Data::Check();

   if(IsDecl())
   {
      CheckUsage();
      CheckConstness();
      CheckIfInitialized();
      CheckIfHiding();
      CheckAccessControl();
      CheckIfMutated();
   }
}

//------------------------------------------------------------------------------

fn_name ClassData_CheckAccessControl = "ClassData.CheckAccessControl";

void ClassData::CheckAccessControl() const
{
   Debug::ft(ClassData_CheckAccessControl);

   //  This checks for data that
   //  o could have a more restricted access control, or
   //  o that is not private.
   //  Neither of these is particularly useful for static const data.
   //
   if(IsStatic() & IsConst()) return;
   CxxScope::CheckAccessControl();

   //  Do not log data as non-private if
   //  o it is static and const (already checked above)
   //  o it is declared in a .cpp
   //  o it belongs to a struct or union
   //
   if(GetAccess() == Cxx::Private) return;
   if(GetFile()->IsCpp()) return;
   if(GetClass()->GetClassTag() != Cxx::ClassType) return;
   if(depth_ > 0) return;
   Log(DataNotPrivate);
}

//------------------------------------------------------------------------------

fn_name ClassData_CheckIfInitialized = "ClassData.CheckIfInitialized";

void ClassData::CheckIfInitialized() const
{
   Debug::ft(ClassData_CheckIfInitialized);

   //  Static data should be initialized.
   //
   if(!WasInited() && IsStatic()) Log(DataUninitialized);
}

//------------------------------------------------------------------------------

fn_name ClassData_CheckIfMutated = "ClassData.CheckIfMutated";

void ClassData::CheckIfMutated() const
{
   Debug::ft(ClassData_CheckIfMutated);

   if(mutable_ && !mutated_) Log(DataNeedNotBeMutable);
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
         auto lead = spaces(Indent_Size * (depth_ - 1));
         stream << prefix << lead << access << ": " << UNION_STR << CRLF;
         stream << prefix << lead << '{' << CRLF;
      }

      stream << spaces(Indent_Size * depth_);
      access = Cxx::Public;
   }

   stream << prefix << access << ": ";
   if(IsStatic()) stream << STATIC_STR << SPACE;
   if(mutable_) stream << MUTABLE_STR << SPACE;
   if(IsConstexpr()) stream << CONSTEXPR_STR << SPACE;
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
      stream << prefix << spaces(Indent_Size * (depth_ - 1)) << "};" << CRLF;
   }
}

//------------------------------------------------------------------------------

fn_name ClassData_EnterBlock = "ClassData.EnterBlock";

void ClassData::EnterBlock()
{
   Debug::ft(ClassData_EnterBlock);

   Context::SetPos(GetPos());

   //  The initialization of a static member is handled by
   //  o ClassData.EnterScope, if initialized where declared, or
   //  o SpaceData.EnterScope, if initialized separately.
   //
   if(IsStatic())
   {
      auto expl = string("Improper initialization of static member ");
      expl += ScopedName(true);
      Context::SwErr(ClassData_EnterBlock, expl, 0);
      return;
   }

   //  If there is a member initialization statement, execute it and then
   //  clear it: there could be more than one constructor, each with its
   //  own version of the member initialization.  If there is no member
   //  initialization statement, see if a class member is using a default
   //  constructor.
   //
   if(init_ != nullptr)
   {
      Context::SetPos(init_->GetPos());
      InitByExpr(init_->GetInit());
      init_ = nullptr;
      return;
   }

   InitByDefault();
}

//------------------------------------------------------------------------------

fn_name ClassData_EnterScope = "ClassData.EnterScope";

bool ClassData::EnterScope()
{
   Debug::ft(ClassData_EnterScope);

   //  When class data is declared, its type and field with are known.
   //  A static const POD member (unless its a pointer) could also be
   //  initialized at this point.
   //
   Context::SetPos(GetPos());
   GetTypeSpec()->EnteringScope(this);

   if(width_ != nullptr)
   {
      width_->EnterBlock();
      auto result = Context::PopArg(true);
      auto numeric = result.NumericType();

      if(numeric.Type() != Numeric::INT)
      {
         auto expl = "Non-numeric value for field width";
         Context::SwErr(ClassData_EnterBlock, expl, numeric.Type());
      }
   }

   //  Presumably we're dealing with well-formed code.  We could therefore
   //  remove these checks and just invoke ExecuteInit directly.  However,
   //  they are included because they help to verify that *this* software
   //  is correct.  The same is true for most of the checks on type
   //  restrictions or type compatibility, such as the one above for field
   //  width.
   //
   if(IsStatic() && IsConst() && IsPOD() && !IsPointer())
   {
      ExecuteInit(false);
   }

   CloseScope();
   return true;
}

//------------------------------------------------------------------------------

fn_name ClassData_GetUsages = "ClassData.GetUsages";

void ClassData::GetUsages(const CodeFile& file, CxxUsageSets& symbols) const
{
   Debug::ft(ClassData_GetUsages);

   Data::GetUsages(file, symbols);

   if(width_ != nullptr) width_->GetUsages(file, symbols);
}

//------------------------------------------------------------------------------

fn_name ClassData_MemberToArg = "ClassData.MemberToArg";

StackArg ClassData::MemberToArg(StackArg& via, Cxx::Operator op)
{
   Debug::ft(ClassData_MemberToArg);

   //  Create an argument for this member, which was accessed through VIA.
   //
   Accessed();
   auto arg = StackArg(this, via, op);
   if(mutable_) arg.SetAsMutable();
   return arg;
}

//------------------------------------------------------------------------------

fn_name ClassData_NameToArg = "ClassData.NameToArg";

StackArg ClassData::NameToArg(Cxx::Operator op)
{
   Debug::ft(ClassData_NameToArg);

   //  Create an argument, marking it as a member of the context class and
   //  noting if it is mutable.  Log a read on the implicit "this", and if
   //  the context function is const, also mark the item as const.
   //
   auto arg = Data::NameToArg(op);
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

fn_name ClassData_Promote = "ClassData.Promote";

void ClassData::Promote(Class* cls, Cxx::Access access, bool first, bool last)
{
   Debug::ft(ClassData_Promote);

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

void ClassData::Shrink()
{
   Data::Shrink();

   name_.shrink_to_fit();
   CxxStats::Strings(CxxStats::CLASS_DATA, name_.capacity());
   if(width_ != nullptr) width_->Shrink();
}

//------------------------------------------------------------------------------

fn_name ClassData_WasMutated = "ClassData.WasMutated";

void ClassData::WasMutated(const StackArg* arg)
{
   Debug::ft(ClassData_WasMutated);

   Data::SetNonConst();

   //  A StackArg inherits its mutable_ attribute from arg.via_, so this
   //  function can be invoked on data that is not tagged mutable itself.
   //
   if(!mutable_) return;

   //  This item is using its mutability if it is currently const.
   //
   if(arg->item == this)
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

fn_name ClassData_WasWritten = "ClassData.WasWritten";

bool ClassData::WasWritten(const StackArg* arg, bool passed)
{
   Debug::ft(ClassData_WasWritten);

   auto result = Data::WasWritten(arg, passed);

   //  Check if mutable data just made use of its mutability.
   //
   if(mutable_ && arg->IsReadOnly(passed))
   {
      mutated_ = true;
   }

   return result;
}

//==============================================================================

fn_name CxxScope_ctor = "CxxScope.ctor";

CxxScope::CxxScope() : pushes_(0)
{
   Debug::ft(CxxScope_ctor);
}

//------------------------------------------------------------------------------

fn_name CxxScope_dtor = "CxxScope.dtor";

CxxScope::~CxxScope()
{
   Debug::ft(CxxScope_dtor);
}

//------------------------------------------------------------------------------

fn_name CxxScope_AccessibilityOf = "CxxScope.AccessibilityOf";

void CxxScope::AccessibilityOf
   (const CxxScope* scope, const CxxScoped* item, SymbolView* view) const
{
   Debug::ft(CxxScope_AccessibilityOf);

   view->distance = scope->ScopeDistance(this);
   view->accessibility =
      (view->distance == NOT_A_SUBSCOPE ? Inaccessible : Unrestricted);
}

//------------------------------------------------------------------------------

fn_name CxxScope_CloseScope = "CxxScope.CloseScope";

void CxxScope::CloseScope()
{
   Debug::ft(CxxScope_CloseScope);

   for(NO_OP; pushes_ > 0; --pushes_) Context::PopScope();
}

//------------------------------------------------------------------------------

id_t CxxScope::GetDistinctDeclFid() const
{
   auto defn = GetDefnFile();

   if(defn != nullptr)
   {
      auto decl = GetDeclFile();
      if(decl != defn) return decl->Fid();
   }

   return NIL_ID;
}

//------------------------------------------------------------------------------

fn_name CxxScope_IsSubscopeOf = "CxxScope.IsSubscopeOf";

bool CxxScope::IsSubscopeOf(const string& name) const
{
   Debug::ft(CxxScope_IsSubscopeOf);

   //  This scope is a subscope of NAME if its fully qualified name is equal
   //  to, or a subset of, NAME.  Template arguments are ignored because the
   //  subclassing of a template instance is not supported.
   //
   auto fqName = ScopedName(false);
   auto size = name.size();

   if(name.compare(0, size, fqName, 0, size) == 0)
   {
      if(size == fqName.size()) return true;
      if(fqName.compare(size, 2, SCOPE_STR) == 0) return true;
   }

   return false;
}

//------------------------------------------------------------------------------

fn_name CxxScope_NameIsTemplateParm = "CxxScope.NameIsTemplateParm";

bool CxxScope::NameIsTemplateParm(const string& name) const
{
   Debug::ft(CxxScope_NameIsTemplateParm);

   auto scope = this;

   while(scope != nullptr)
   {
      auto tmplt = scope->GetTemplateParms();

      if(tmplt != nullptr)
      {
         auto parms = tmplt->Parms();

         for(auto p = parms->cbegin(); p != parms->cend(); ++p)
         {
            if(*(*p)->Name() == name) return true;
         }
      }

      scope = scope->GetScope();
   }

   return false;
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
            Context::PushScope(scope);
            pushes_++;
         }
         else
         {
            auto expl = "Could not find scope " + name;
            Context::SwErr(CxxScope_OpenScope, expl, 0);
            scope = Context::Scope();
         }
      }
   }

   SetScope(scope);
   Context::PushScope(this);
   pushes_++;
}

//------------------------------------------------------------------------------

fn_name CxxScope_ReplaceTemplateParms = "CxxScope.ReplaceTemplateParms";

void CxxScope::ReplaceTemplateParms
   (std::string& code, const TypeSpecPtrVector* args, size_t begin) const
{
   Debug::ft(CxxScope_ReplaceTemplateParms);

   //  Replace the template parameters with the instance arguments.
   //
   auto tmpltParms = GetTemplateParms()->Parms();
   auto tmpltSpec = GetQualName()->GetTemplateArgs();
   auto tmpltArgs = (tmpltSpec != nullptr ? tmpltSpec->Args() : nullptr);
   string argName;

   for(size_t i = 0; i < tmpltParms->size(); ++i)
   {
      auto& parmName = *tmpltParms->at(i)->Name();

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

fn_name CxxScope_ScopeDistance = "CxxScope.ScopeDistance";

Distance CxxScope::ScopeDistance(const CxxScope* scope) const
{
   Debug::ft(CxxScope_ScopeDistance);

   Distance dist = 0;

   for(auto curr = this; curr != nullptr; curr = curr->GetScope())
   {
      if(curr == scope) return dist;
      ++dist;
   }

   return NOT_A_SUBSCOPE;
}

//==============================================================================

fn_name Data_ctor = "Data.ctor";

Data::Data(TypeSpecPtr& spec) :
   extern_(false),
   static_(false),
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
   Debug::ft(Data_ctor);

   spec_->SetUserType(Cxx::Data);
}

//------------------------------------------------------------------------------

fn_name Data_dtor = "Data.dtor";

Data::~Data()
{
   Debug::ft(Data_dtor);
}

//------------------------------------------------------------------------------

fn_name Data_Check = "Data.Check";

void Data::Check() const
{
   Debug::ft(Data_Check);

   spec_->Check();
   if(!defn_ && (mate_ != nullptr)) mate_->Check();
}

//------------------------------------------------------------------------------

fn_name Data_CheckConstness = "Data.CheckConstness";

void Data::CheckConstness(bool could) const
{
   Debug::ft(Data_CheckConstness);

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
         if(!nonconstptr_ && IsPointer() && could) Log(DataCouldBeConstPtr);
      }
      else
      {
         if(nonconstptr_) Log(DataCannotBeConstPtr);
      }
   }
}

//------------------------------------------------------------------------------

const string LeftPunctuation("([<{");

fn_name Data_CheckFunctionString = "Data.CheckFunctionString";

bool Data::CheckFunctionString(const Function* func) const
{
   Debug::ft(Data_CheckFunctionString);

   //  In order to find a string literal that identifies FUNC, this member
   //  must have an initialization statement, and the line where it appears
   //  should have been classified as one that defines a function name.
   //
   if(rhs_ == nullptr) return false;

   //  Extract the string literal by displaying the member's initialization
   //  statement.  Look for the "." between <scope> and <name>.
   //
   std::ostringstream stream;
   rhs_->Print(stream, Flags());

   auto str = stream.str();
   auto quote = str.find(QUOTE);
   if(quote == string::npos) return false;
   str.erase(0, quote + 1);
   quote = str.find(QUOTE);
   str.erase(quote);

   //  Check that the string literal is of the form
   //     "<scope>.<name>"
   //  where <scope> is the name of FUNC's scope and <name> is its name.
   //  However
   //  o If FUNC is defined in the global namespace, its name will have
   //    no <scope> prefix.
   //  o If FUNC is overloaded, "left punctuation" can follow <name> in
   //    order to give a unique name to each of the function's overloads.
   //
   string name;

   switch(func->FuncType())
   {
   case FuncCtor:
      name = "ctor";
      break;
   case FuncDtor:
      name = "dtor";
      break;
   default:
      name = *func->Name();
   }

   auto scope = func->GetScope()->Name();

   if(scope->empty())
   {
      if(str.find(name) != 0) return false;
      auto size = name.size();
      if(str.size() == size) return true;
      return (LeftPunctuation.find(str[size]) != string::npos);
   }

   auto dot = str.find('.');
   if(dot == string::npos) return false;
   if(str.find(*scope) != 0) return false;
   if(str.find(name, dot) != dot + 1) return false;
   auto size = scope->size() + 1 + name.size();
   if(str.size() == size) return true;
   return (LeftPunctuation.find(str[size]) != string::npos);
}

//------------------------------------------------------------------------------

fn_name Data_CheckUsage = "Data.CheckUsage";

void Data::CheckUsage() const
{
   Debug::ft(Data_CheckUsage);

   if(reads_ == 0)
   {
      if(writes_ > 0)
         Log(DataWriteOnly);
      else if(WasInited() & !IsConst())
         Log(DataInitOnly);
      else Log(DataUnused);
   }
}

//------------------------------------------------------------------------------

void Data::DisplayAssignment(ostream& stream, const Flags& options) const
{
   //  Always display an assignment in namespace view.  In file view,
   //  only display it where it occurs.
   //
   auto ns = options.test(DispNS);
   if(!ns && (rhs_ == nullptr)) return;

   auto rhs = GetDefn()->rhs_.get();
   if(rhs == nullptr) return;

   //  The source code only contains the assignment operator and the
   //  initialization expression.
   //
   std::ostringstream buffer;

   stream << " = ";
   rhs->Back()->Print(buffer, options);

   auto expr = buffer.str();

   if(expr.size() <= 80)
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
   stream << "i=" << int(decl->inited_) << SPACE;
   stream << "r=" << decl->reads_ << SPACE;
   stream << "w=" << decl->writes_ << SPACE;
}

//------------------------------------------------------------------------------

fn_name Data_ExecuteInit = "Data.ExecuteInit";

bool Data::ExecuteInit(bool push)
{
   Debug::ft(Data_ExecuteInit);

   if(push)
   {
      Context::Enter(this);
      Context::PushScope(this);
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

fn_name Data_GetInitName = "Data.GetInitName";

void Data::GetInitName(QualNamePtr& qualName) const
{
   Debug::ft(Data_GetInitName);

   qualName.reset(new QualName(*Name()));
}

//------------------------------------------------------------------------------

TypeName* Data::GetTemplateArgs() const
{
   return spec_->GetTemplateArgs();
}

//------------------------------------------------------------------------------

fn_name Data_GetUsages = "Data.GetUsages";

void Data::GetUsages(const CodeFile& file, CxxUsageSets& symbols) const
{
   Debug::ft(Data_GetUsages);

   spec_->GetUsages(file, symbols);
   if(expr_ != nullptr) expr_->GetUsages(file, symbols);
   if(rhs_ != nullptr) rhs_->GetUsages(file, symbols);
}

//------------------------------------------------------------------------------

fn_name Data_InitByAssign = "Data.InitByAssign";

bool Data::InitByAssign()
{
   Debug::ft(Data_InitByAssign);

   if(rhs_ == nullptr) return false;

   rhs_->EnterBlock();
   auto result = Context::PopArg(true);
   spec_->MustMatchWith(result);
   GetDecl()->inited_ = true;
   return true;
}

//------------------------------------------------------------------------------

fn_name Data_InitByDefault = "Data.InitByDefault";

bool Data::InitByDefault()
{
   Debug::ft(Data_InitByDefault);

   if(extern_) return false;

   auto spec = GetTypeSpec();
   if(spec == nullptr) return false;
   auto root = spec->Root();
   if(root == nullptr) return false;
   if(root->Type() != Cxx::Class) return false;
   if(spec->Ptrs(false) > 0) return false;
   if(spec->Refs() > 0) return false;

   auto decl = GetDecl();
   auto cls = static_cast< Class* >(root);
   cls->Instantiate();
   auto ctor = cls->FindCtor(nullptr);

   if(ctor != nullptr)
   {
      SymbolView view;
      cls->AccessibilityOf(Context::Scope(), ctor, &view);
      ctor->WasCalled();
      decl->inited_ = true;
   }
   else
   {
      decl->inited_ = (!cls->HasPODMember());

      if(decl->inited_)
         Log(DefaultConstructor);
      else
         Log(DefaultPODConstructor);
   }

   return decl->inited_;
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
      cls->Instantiate();

      //  Push CLS as the constructor name that will handle expr_, which is
      //  a FUNCTION_CALL Operation that contains an argument list but which
      //  is missing the antecedent, the class's name.  The constructor also
      //  requires a "this" argument.
      //
      Context::PushArg(StackArg(cls, 0));
      Context::TopArg()->SetInvoke();
      Context::PushArg(StackArg(cls, 1));
      Context::TopArg()->SetAsThis(true);
      Expression::Start();
      expr->EnterBlock();
      Context::Execute();
      Context::Clear(2);
   }
   else
   {
      //  ROOT is not a class, so EXPR should contain a single expression.
      //  Execute it as if it was a single-member brace initialization list.
      //
      if(expr->Type() == Cxx::Operation)
      {
         auto op = static_cast< Operation* >(expr);

         if(op->ArgsSize() == 1)
         {
            op->FrontArg()->EnterBlock();
            auto result = Context::PopArg(true);
            spec_->MustMatchWith(result);
            result.AssignedTo(StackArg(this, 0), Copied);
         }
         else
         {
            auto expl = "Invalid arguments for " + *spec_->Name();
            Context::SwErr(Data_InitByExpr, expl, op->ArgsSize());
         }
      }
      else
      {
         auto expl = "Invalid expression for " + *spec_->Name();
         Context::SwErr(Data_InitByExpr, expl, expr->Type());
      }
   }

   GetDecl()->inited_ = true;
   return true;
}

//------------------------------------------------------------------------------

bool Data::IsConst() const
{
   if(constexpr_) return true;
   return spec_->IsConst();
}

//------------------------------------------------------------------------------

bool Data::IsConstPtr() const
{
   if(constexpr_) return true;
   return spec_->IsConstPtr();
}

//------------------------------------------------------------------------------

fn_name Data_IsDefaultConstructible = "Data.IsDefaultConstructible";

bool Data::IsDefaultConstructible() const
{
   Debug::ft(Data_IsDefaultConstructible);

   if(static_) return true;

   auto type = spec_->TypeString(false);
   if(type.find(ARRAY_STR) != string::npos) return true;

   auto cls = DirectClass();

   if(cls != nullptr)
   {
      return static_cast< Class* >(cls)->IsDefaultConstructible();
   }

   return false;
}

//------------------------------------------------------------------------------

fn_name Data_NameToArg = "Data.NameToArg";

StackArg Data::NameToArg(Cxx::Operator op)
{
   Debug::ft(Data_NameToArg);

   //  Make data writeable during its initialization.
   //
   auto arg = CxxNamed::NameToArg(op);
   if(initing_) arg.SetAsWriteable();
   return arg;
}

//------------------------------------------------------------------------------

fn_name Data_SetAssignment = "Data.SetAssignment";

void Data::SetAssignment(ExprPtr& expr)
{
   Debug::ft(Data_SetAssignment);

   //  Create an assignment operation in which the name of this data item
   //  is the first argument and EXPR is the second argument.
   //
   if(expr == nullptr) return;
   rhs_.reset(new Expression(expr->EndPos(), true));

   QualNamePtr name;
   GetInitName(name);
   auto arg1 = TokenPtr(name.release());
   rhs_->AddItem(arg1);
   auto op = TokenPtr(new Operation(Cxx::ASSIGN));
   rhs_->AddItem(op);
   auto arg2 = TokenPtr(expr.release());
   rhs_->AddItem(arg2);
}

//------------------------------------------------------------------------------

fn_name Data_SetDefn = "Data.SetDefn";

void Data::SetDefn(Data* data)
{
   Debug::ft(Data_SetDefn);

   data->mate_ = this;
   data->defn_ = true;
   this->mate_ = data;
}

//------------------------------------------------------------------------------

fn_name Data_SetNonConst = "Data.SetNonConst";

bool Data::SetNonConst()
{
   Debug::ft(Data_SetNonConst);

   if(initing_) return true;
   nonconst_ = true;
   return !IsConst();
}

//------------------------------------------------------------------------------

void Data::Shrink()
{
   spec_->Shrink();
   if(expr_ != nullptr) expr_->Shrink();
   if(rhs_ != nullptr) rhs_->Shrink();
}

//------------------------------------------------------------------------------

string Data::TypeString(bool arg) const
{
   return spec_->TypeString(arg);
}

//------------------------------------------------------------------------------

bool Data::WasRead()
{
   if(initing_) return false;
   ++reads_;
   return true;
}

//------------------------------------------------------------------------------

fn_name Data_WasWritten = "Data.WasWritten";

bool Data::WasWritten(const StackArg* arg, bool passed)
{
   Debug::ft(Data_WasWritten);

   if(initing_) return false;
   ++writes_;

   auto ptrs = (arg->item == this ? arg->Ptrs(true) : spec_->Ptrs(true));

   if(ptrs == 0)
      nonconst_ = true;
   else
      nonconstptr_ = true;

   return true;
}

//==============================================================================

fn_name FuncData_ctor = "FuncData.ctor";

FuncData::FuncData(string& name, TypeSpecPtr& type) : Data(type),
   first_(this)
{
   Debug::ft(FuncData_ctor);

   std::swap(name_, name);
   CxxStats::Incr(CxxStats::FUNC_DATA);
}

//------------------------------------------------------------------------------

fn_name FuncData_dtor = "FuncData.dtor";

FuncData::~FuncData()
{
   Debug::ft(FuncData_dtor);

   CxxStats::Decr(CxxStats::FUNC_DATA);
}

//------------------------------------------------------------------------------

fn_name FuncData_Check = "FuncData.Check";

void FuncData::Check() const
{
   Debug::ft(FuncData_Check);

   Data::Check();

   //  Don't check a variable in a function template, as it isn't executed.
   //  Don't check a function's internal variables for potential constness.
   //
   auto func = GetScope()->GetFunction();
   if(func->IsTemplate()) return;
   CheckUsage();
   CheckConstness(false);
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
      if(IsExtern()) stream << EXTERN_STR << SPACE;
      if(IsStatic()) stream << STATIC_STR << SPACE;
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

fn_name FuncData_EnterBlock = "FuncData.EnterBlock";

void FuncData::EnterBlock()
{
   Debug::ft(FuncData_EnterBlock);

   //  This also doubles as the equivalent of EnterScope for function data.
   //  Set the data's scope, add it to the local symbol table, and execute
   //  its definition.
   //
   auto spec = GetTypeSpec();
   auto anon = spec->IsAuto();

   Context::SetPos(GetPos());
   SetScope(Context::Scope());
   Singleton< CxxSymbols >::Instance()->InsertLocal(this);
   spec->EnteringScope(this);
   ExecuteInit(false);

   //  If this statement contains multiple declarations, continue with the
   //  next one.
   //
   if(next_ != nullptr)
   {
      if(anon) StackArg::SetAutoTypeFor(static_cast< FuncData& >(*next_));
      next_->EnterBlock();
   }
}

//------------------------------------------------------------------------------

fn_name FuncData_ExitBlock = "FuncData.ExitBlock";

void FuncData::ExitBlock()
{
   Debug::ft(FuncData_ExitBlock);

   Singleton< CxxSymbols >::Instance()->EraseLocal(this);
   if(next_ != nullptr) next_->ExitBlock();
}

//------------------------------------------------------------------------------

fn_name FuncData_GetUsages = "FuncData.GetUsages";

void FuncData::GetUsages(const CodeFile& file, CxxUsageSets& symbols) const
{
   Debug::ft(FuncData_GetUsages);

   Data::GetUsages(file, symbols);

   if(next_ != nullptr) next_->GetUsages(file, symbols);
}

//------------------------------------------------------------------------------

void FuncData::Print(ostream& stream, const Flags& options) const
{
   DisplayItem(stream, options);
}

//------------------------------------------------------------------------------

fn_name FuncData_SetNext = "FuncData.SetNext";

void FuncData::SetNext(DataPtr& next)
{
   Debug::ft(FuncData_SetNext);

   next_.reset(next.release());
   auto data = static_cast< FuncData* >(next_.get());
   data->SetFirst(first_);
   data->GetTypeSpec()->SetPtrDetached(false);
   data->GetTypeSpec()->SetRefDetached(false);
}

//------------------------------------------------------------------------------

void FuncData::Shrink()
{
   Data::Shrink();

   name_.shrink_to_fit();
   CxxStats::Strings(CxxStats::FUNC_DATA, name_.capacity());
   if(next_ != nullptr) next_->Shrink();
}

//==============================================================================

fn_name Function_ctor1 = "Function.ctor";

Function::Function(QualNamePtr& name) :
   name_(name.release()),
   extern_(false),
   inline_(false),
   constexpr_(false),
   static_(false),
   virtual_(false),
   explicit_(false),
   const_(false),
   noexcept_(false),
   override_(false),
   pure_(false),
   type_(false),
   friend_(false),
   found_(false),
   this_(false),
   nonpublic_(false),
   nonstatic_(false),
   calls_(0),
   defn_(false),
   deleted_(false),
   defaulted_(false),
   mate_(nullptr),
   pos_(string::npos),
   begin_(string::npos),
   end_(string::npos),
   base_(nullptr),
   tmplt_(nullptr)
{
   Debug::ft(Function_ctor1);

   Singleton< CxxSymbols >::Instance()->InsertFunc(this);

   auto qname = name_->QualifiedName(true, false);
   OpenScope(qname);
   CxxStats::Incr(CxxStats::FUNC_DECL);
}

//------------------------------------------------------------------------------

fn_name Function_ctor2 = "Function.ctor(spec)";

Function::Function(QualNamePtr& name, TypeSpecPtr& spec, bool type) :
   name_(name.release()),
   extern_(false),
   inline_(false),
   constexpr_(false),
   static_(false),
   virtual_(false),
   explicit_(false),
   const_(false),
   noexcept_(false),
   override_(false),
   pure_(false),
   type_(type),
   friend_(false),
   found_(false),
   this_(false),
   nonpublic_(false),
   nonstatic_(false),
   calls_(0),
   defn_(false),
   deleted_(false),
   defaulted_(false),
   mate_(nullptr),
   spec_(spec.release()),
   pos_(string::npos),
   begin_(string::npos),
   end_(string::npos),
   base_(nullptr),
   tmplt_(nullptr)
{
   Debug::ft(Function_ctor2);

   spec_->SetUserType(Cxx::Function);
   if(type_) return;

   auto qname = name_->QualifiedName(true, false);
   OpenScope(qname);
   CxxStats::Incr(CxxStats::FUNC_DECL);
}

//------------------------------------------------------------------------------

fn_name Function_dtor = "Function.dtor";

Function::~Function()
{
   Debug::ft(Function_dtor);

   if(type_) return;

   CloseScope();
   Singleton< CxxSymbols >::Instance()->EraseFunc(this);
   CxxStats::Decr(CxxStats::FUNC_DECL);
}

//------------------------------------------------------------------------------

fn_name Function_AddArg = "Function.AddArg";

void Function::AddArg(ArgumentPtr& arg)
{
   Debug::ft(Function_AddArg);

   arg->SetScope(this);
   args_.push_back(std::move(arg));
}

//------------------------------------------------------------------------------

fn_name Function_AddMemberInit = "Function.AddMemberInit";

void Function::AddMemberInit(MemberInitPtr& init)
{
   Debug::ft(Function_AddMemberInit);

   mems_.push_back(std::move(init));
}

//------------------------------------------------------------------------------

fn_name Function_AddOverride = "Function.AddOverride";

void Function::AddOverride(Function* over) const
{
   Debug::ft(Function_AddOverride);

   overs_.push_back(over);
}

//------------------------------------------------------------------------------

fn_name Function_AddThisArg = "Function.AddThisArg";

void Function::AddThisArg()
{
   Debug::ft(Function_AddThisArg);

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
   //  defined as const.
   //
   TypeSpecPtr typeSpec(new DataSpec(cls->Name()->c_str()));
   typeSpec->SetPtrs(1);
   typeSpec->SetConst(const_);
   typeSpec->SetReferent(cls, nullptr);
   string argName(THIS_STR);
   ArgumentPtr arg(new Argument(argName, typeSpec));
   arg->SetScope(this);
   arg->SetPos(GetFile(), GetPos());
   args_.insert(args_.begin(), std::move(arg));
   this_ = true;
}

//------------------------------------------------------------------------------

fn_name Function_ArgCouldBeConst = "Function.ArgCouldBeConst";

bool Function::ArgCouldBeConst(size_t n) const
{
   Debug::ft(Function_ArgCouldBeConst);

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

fn_name Function_ArgIsUnused = "Function.ArgIsUnused";

bool Function::ArgIsUnused(size_t n) const
{
   Debug::ft(Function_ArgIsUnused);

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

fn_name Function_ArgumentsMatch = "Function.ArgumentsMatch";

bool Function::ArgumentsMatch(const Function* that) const
{
   Debug::ft(Function_ArgumentsMatch);

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

fn_name Function_CanConstructFrom = "Function.CanConstructFrom";

bool Function::CanConstructFrom
   (const StackArg& that, const string& thatType, bool implicit) const
{
   Debug::ft(Function_CanConstructFrom);

   //  Return true if this is a constructor that is not explicit, that can
   //  take one argument (after "this"), and that can be invoked with THAT.
   //
   if((!explicit_ || implicit) && (FuncRole() == PureCtor))
   {
      auto min = MinArgs();
      auto max = MaxArgs();

      if((min <= 2) && (max == 2))
      {
         auto thisArg = args_[1].get();
         auto thisType = thisArg->TypeString(true);
         return (StackArg(thisArg, 0).CalcMatchWith
            (that, thisType, thatType, implicit) == Compatible);
      }
   }

   return false;
}

//------------------------------------------------------------------------------

fn_name Function_CanInvokeWith = "Function.CanInvokeWith";

Function* Function::CanInvokeWith
   (StackArgVector& args, stringVector& argTypes, TypeMatch& match) const
{
   Debug::ft(Function_CanInvokeWith);

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
         tmpltParms.push_back(*parms->at(i)->Name());
         tmpltArgs.push_back(EMPTY_STR);
      }
   }

   //  Each argument in ARGS must match, or be transformable to, the type that
   //  this function expects.  Assume compatability and downgrade from there.
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
      if(recvResult.item == nullptr) return FoundFunc(nullptr, args, match);
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

fn_name Function_Check = "Function.Check";

void Function::Check() const
{
   Debug::ft(Function_Check);

   if(IsTemplateInstance()) return;

   if(parms_ != nullptr) parms_->Check();
   if(spec_ != nullptr) spec_->Check();

   for(auto a = args_.cbegin(); a != args_.cend(); ++a)
   {
      (*a)->Check();
   }

   if(!defn_)
   {
      CheckIfDefined();
      CheckIfUsed(FunctionUnused);
      CheckIfHiding();
      CheckArgs();
      CheckAccessControl();
      CheckCtor();
      CheckDtor();
      CheckIfOverridden();
      CheckIfPublicVirtual();
      CheckForVirtualDefault();
      CheckMemberUsage();
      if(mate_ != nullptr) mate_->Check();
   }

   if(impl_ != nullptr) impl_->Check();
}

//------------------------------------------------------------------------------

fn_name Function_CheckAccessControl = "Function.CheckAccessControl";

void Function::CheckAccessControl() const
{
   Debug::ft(Function_CheckAccessControl);

   //  Do not check the access control of destructors.  If this is an override,
   //  do not suggest a more restricted access control unless the function has
   //  a broader access control than the root function.
   //
   auto type = FuncType();
   if(type == FuncDtor) return;

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

   //  See if the function has any arguments to check.  Don't check the
   //  arguments to a function that is undefined, unused, or an operator.
   //
   auto n = args_.size();
   if(n == 0 || IsUndefined() || IsUnused()) return;
   auto type = FuncType();
   if(type == FuncOperator) return;

   //  If the function is defined separately from its declaration, look
   //  for renamed arguments.  If the function is an override, look for
   //  arguments that were renamed from the direct base class.
   //
   if(mate_ != nullptr)
   {
      for(size_t i = 0; i < n; ++i)
      {
         if(*args_[i]->Name() != *mate_->args_[i]->Name())
         {
            args_[i]->Log(DefinitionRenamesArgument, i + (this_ ? 0 : 1));
         }
      }
   }

   if(override_)
   {
      auto base = FindBaseFunc();

      for(size_t i = 0; i < n; ++i)
      {
         if(*args_[i]->Name() != *base->args_[i]->Name())
         {
            args_[i]->Log(OverrideRenamesArgument, i + (this_ ? 0 : 1));
         }
      }

      //  The remaining checks are not applied to an override.
      //
      return;
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
            arg->Log(ArgumentUnused, i + (this_ ? 0 : 1));
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
                  arg->Log(ArgumentCannotBeConst, i + (this_ ? 0 : 1));
               }
            }

            continue;
         }

         //  The argument is not const.  If it could be const, then
         //  o if the argument is "this", the function could be const;
         //  o if the argument is an object passed by value, it could
         //    be passed as a const reference;
         //  o otherwise, the argument could be declared const.
         //
         if(ArgCouldBeConst(i))
         {
            if((i == 0) && this_)
            {
               if(type == FuncStandard) CheckIfCouldBeConst();
            }
            else
            {
               if(arg->IsPassedByValue())
               {
                  if(arg->Root()->Type() == Cxx::Class)
                     arg->Log(ArgumentCouldBeConstRef, i + (this_ ? 0 : 1));
               }
               else
               {
                  arg->Log(ArgumentCouldBeConst, i + (this_ ? 0 : 1));
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

   //  Check that this is a constructor and that it isn't deleted.
   //
   if(FuncType() != FuncCtor) return;
   if(GetDefn()->impl_ == nullptr) return;

   //  A base class constructor should not be public.
   //
   if(GetAccess() == Cxx::Public)
   {
      if(!GetClass()->Subclasses()->empty()) Log(PublicConstructor);
   }

   //  A constructor should be tagged explicit if it can take a single
   //  argument (besides "this") and is not a copy or move constructor.
   //
   if(!explicit_)
   {
      auto min = MinArgs();
      auto max = MaxArgs();

      if((min <= 2) && (max == 2))
      {
         if(FuncRole() == PureCtor) Log(NonExplicitConstructor);
      }
   }

   //  Get ITEMS, a list of the class's data members.  This list contains the
   //  members in order of declaration and indicates how each member should be
   //  initialized. Go through the member initialization list, if any, find
   //  each initialized member in ITEMS, and record when it was initialized.
   //
   DataInitVector items;
   auto cls = GetClass();
   cls->GetMemberInitAttrs(items);

   auto& mems = GetDefn()->mems_;

   for(size_t i = 0; i < mems.size(); ++i)
   {
      auto mem = mems.at(i).get();
      auto data = cls->FindData(*mem->Name());

      for(size_t j = 0; j < items.size(); ++j)
      {
         if(items[j].member == data)
         {
            items[j].initOrder = i + 1;
            break;
         }
      }
   }

   //  All members that need initialization should have been initialized in
   //  order of declaration.  If a member should have been initialized but
   //  was not, log this against the member.  If a member was initialized
   //  out of order, log this against the initialization statement.
   //
   size_t last = 0;

   for(auto item = items.cbegin(); item != items.cend(); ++item)
   {
      if(item->initOrder == 0)
      {
         if(item->initNeeded)
         {
            item->member->Log(MemberInitMissing);
         }
      }
      else
      {
         if(item->initOrder < last)
         {
            auto token = mems.at(item->initOrder - 1).get();
            auto init = static_cast< MemberInit* >(token);
            init->Log(MemberInitNotSorted);
         }

         last = item->initOrder;
      }
   }
}

//------------------------------------------------------------------------------

fn_name Function_CheckDtor = "Function.CheckDtor";

void Function::CheckDtor() const
{
   Debug::ft(Function_CheckDtor);

   if(FuncType() != FuncDtor) return;

   auto cls = GetClass();
   if(cls->Subclasses()->empty()) return;

   if(virtual_)
   {
      for(auto s = cls->BaseClass(); s != nullptr; s = s->BaseClass())
      {
         auto dtor = s->FindDtor();
         if((dtor != nullptr) && (dtor->GetAccess() != Cxx::Public)) return;
      }

      if(GetAccess() != Cxx::Public) Log(VirtualDestructor);
   }
   else
   {
      if(GetAccess() == Cxx::Public) Log(NonVirtualDestructor);
   }
}

//------------------------------------------------------------------------------

fn_name Function_CheckForVirtualDefault = "Function.CheckForVirtualDefault";

void Function::CheckForVirtualDefault() const
{
   Debug::ft(Function_CheckForVirtualDefault);

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

fn_name Function_CheckIfCouldBeConst = "Function.CheckIfCouldBeConst";

void Function::CheckIfCouldBeConst() const
{
   Debug::ft(Function_CheckIfCouldBeConst);

   //  Before claiming that a function could be const, check for const
   //  overloading (another function in this class that has the same name
   //  and that takes the same arguments).  If so, it can only differ in
   //  constness, which prevents this function from being const.
   //
   auto list = GetClass()->FuncVector(*Name());

   for(auto f = list->cbegin(); f != list->cend(); ++f)
   {
      auto func = f->get();

      if((func != this) && (*func->Name() == *Name())) return;
   }

   Log(FunctionCouldBeConst);
}

//------------------------------------------------------------------------------

fn_name Function_CheckIfDefined = "Function.CheckIfDefined";

void Function::CheckIfDefined() const
{
   Debug::ft(Function_CheckIfDefined);

   //  A function without an implementation is logged as undefined unless
   //  o it's actually part of a function signature typedef, or
   //  o it is deleting the default that the compiler would otherwise provide.
   //  Pure virtual functions are logged separately, because not providing an
   //  implementation may be intentional.
   //
   if(GetDefn()->impl_ != nullptr) return;
   if(type_) return;
   if(IsDeleted()) return;

   if(pure_)
      Log(PureVirtualNotDefined);
   else
      Log(FunctionNotDefined);
}

//------------------------------------------------------------------------------

fn_name Function_CheckIfHiding = "Function.CheckIfHiding";

void Function::CheckIfHiding() const
{
   Debug::ft(Function_CheckIfHiding);

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

fn_name Function_CheckIfUsed = "Function.CheckIfUsed";

void Function::CheckIfUsed(Warning warning) const
{
   Debug::ft(Function_CheckIfUsed);

   if(type_) return;
   CxxScoped::CheckIfUsed(warning);
}

//------------------------------------------------------------------------------

fn_name Function_CheckMemberUsage = "Function.CheckMemberUsage";

void Function::CheckMemberUsage() const
{
   Debug::ft(Function_CheckMemberUsage);

   //  Check if this function could be static or free.  For either to be
   //  possible, the function cannot be virtual, must not have accessed a
   //  non-static member, and must be a standard member function that is not
   //  part of a template.  [A function which accesses a non-static member
   //  could still be free or static, provided that the member was public.
   //  However, the function would have to add the underlying object as an
   //  argument--essentially a "this" argument.  Some will argue that this
   //  improves encapsulation, but we will demur.]
   //c To support this for templates, nonstatic_ and nonpublic_ would have to
   //  to be looked at over all instances of a function, whether instantiated
   //  in a class or function template, because these flags are never set in
   //  a pure template.
   //
   if(virtual_) return;
   if(type_) return;
   if(GetDefn()->nonstatic_) return;
   if(tmplt_ != nullptr) return;
   if(!tmplts_.empty()) return;
   if(FuncType() != FuncStandard) return;

   auto cls = GetClass();
   if(cls == nullptr) return;
   if(cls->GetTemplate() != nullptr) return;

   //  If the function accessed only public members, it could be free.
   //  Otherwise, it could be static.
   //
   if(!GetDefn()->nonpublic_) Log(FunctionCouldBeFree);
   else if(!static_) Log(FunctionCouldBeStatic);
}

//------------------------------------------------------------------------------

fn_name Function_CheckOverride = "Function.CheckOverride";

void Function::CheckOverride()
{
   Debug::ft(Function_CheckOverride);

   //  If this function is an override, register it against the function that
   //  it immediately overrides, and log it if it is missing the "virtual" or
   //  "override" tag.  A destructor is neither registered nor logged.
   //
   base_ = FindBaseFunc();
   if(base_ == nullptr) return;
   if(FuncType() == FuncDtor) return;

   base_->AddOverride(this);
   if(!virtual_) Log(VirtualTagMissing);
   if(!override_) Log(OverrideTagMissing);
   virtual_ = true;
   override_ = true;
}

//------------------------------------------------------------------------------

fn_name Function_DeleteVoidArg = "Function.DeleteVoidArg";

void Function::DeleteVoidArg()
{
   Debug::ft(Function_DeleteVoidArg);

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
      stream << GetDecl()->GetAccess() << ": ";
   }
   DisplayDecl(stream, options);
   DisplayDefn(stream, prefix, options);

   if(!options.test(DispCode) & !tmplts_.empty())
   {
      stream << prefix << "instantiations (" << tmplts_.size() << "):" << CRLF;
      auto lead = prefix + spaces(Indent_Size);

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
   if(virtual_) stream << VIRTUAL_STR << SPACE;
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

   for(size_t i = 0 ; i < args_.size(); ++i)
   {
      if((i == 0) && this_) continue;
      args_[i]->Print(stream, options);
      if(i != args_.size() - 1) stream << ", ";
   }

   stream << ')';
   if(const_) stream << SPACE << CONST_STR;
   if(noexcept_) stream << SPACE << NOEXCEPT_STR;
   if(override_) stream << SPACE << OVERRIDE_STR;
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
      else if(defaulted_)
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
      auto lead = prefix + spaces(Indent_Size);

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
   auto inst = ((cls != nullptr) && cls->IsInTemplateInstance());
   auto subs = GetFile()->IsSubsFile();
   auto impl = (GetDefn()->impl_ != nullptr);
   auto decl = GetDecl();
   auto calls = false;

   std::ostringstream buff;
   buff << " // ";

   if(!impl && !inst && !subs && !deleted_)
      buff << "<@unimpl" << SPACE;
   else
      calls = true;

   if(options.test(DispStats))
   {
      if(!decl->overs_.empty()) buff << "o=" << decl->overs_.size() << SPACE;
      if((decl->calls_ > 0) || calls) buff << "c=" << decl->calls_ << SPACE;
   }

   if(!options.test(DispFQ) && impl) DisplayFiles(buff);
   auto str = buff.str();
   if(str.size() > 4) stream << str;
}

//------------------------------------------------------------------------------

fn_name Function_EnterBlock = "Function.EnterBlock";

void Function::EnterBlock()
{
   Debug::ft(Function_EnterBlock);

   //  If the function has no implementation, do nothing.  An empty function
   //  (just the braces) has an empty code block, so it will get past this.
   //
   if(impl_ == nullptr) return;

   //  Don't execute a function template or a function in a class template.
   //  A function in a class template *instance*, however, is executed.
   //
   auto type = GetTemplateType();

   if(type != NonTemplate)
   {
      if(Context::ParsingTemplateInstance()) return;
      auto file = GetImplFile();
      if(file != nullptr) file->SetTemplate(type);
      return;
   }

   //  Set up the "execution" context and add the function's arguments to the
   //  local symbol table.  Execute the function's code block, including any
   //  base constructor call and member initializations.  The latter are first
   //  assigned to their respective members, after which all non-static members
   //  are initialized so that class members can invoke default constructors.
   //
   Context::Enter(this);
   Context::PushScope(this);

   for(auto a = args_.cbegin(); a != args_.cend(); ++a)
   {
      (*a)->EnterBlock();
   }

   if(FuncType() == FuncCtor)
   {
      const Class* cls = GetClass();

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
         auto data = cls->FindData(*(*m)->Name());

         if(data != nullptr)
         {
            static_cast< ClassData* >(data)->SetInit(m->get());
         }
         else
         {
            auto expl = string("Failed to find member ");
            expl += *cls->Name() + SCOPE_STR + *(*m)->Name();
            Context::SwErr(Function_EnterBlock, expl, 0);
         }
      }

      auto data = cls->Datas();

      for(auto d = data->cbegin(); d != data->cend(); ++d)
      {
         auto item = d->get();
         if(!item->IsStatic()) item->EnterBlock();
      }
   }

   impl_->EnterBlock();

   for(auto a = args_.cbegin(); a != args_.cend(); ++a)
   {
      (*a)->ExitBlock();
   }

   Context::PopScope();
}

//------------------------------------------------------------------------------

fn_name Function_EnterScope = "Function.EnterScope";

bool Function::EnterScope()
{
   Debug::ft(Function_EnterScope);

   //  If this function requires a "this" argument, add it now.
   //
   AddThisArg();

   //  Enter our return type and arguments.
   //
   Context::Enter(this);
   EnterSignature();
   CloseScope();

   //  See whether this is a new function or the definition of a
   //  previously declared function.
   //
   auto defn = false;

   if(impl_ != nullptr)
   {
      auto decl = GetArea()->MatchFunc(this, false);

      if((decl != nullptr) && decl->IsPreviousDeclOf(this))
      {
         defn = true;
         Singleton< CxxSymbols >::Instance()->EraseFunc(this);
         decl->SetDefn(this);
      }
   }

   //  Add the function to this file's list of functions.  If it's
   //  a declaration, check if it's an override.  Then execute it.
   //
   found_ = true;
   if(defn || AtFileScope()) GetFile()->InsertFunc(this);
   if(!defn) CheckOverride();
   EnterBlock();
   return !defn;
}

//------------------------------------------------------------------------------

fn_name Function_EnterSignature = "Function.EnterSignature";

void Function::EnterSignature()
{
   Debug::ft(Function_EnterSignature);

   if(spec_ != nullptr) spec_->EnteringScope(this);

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

fn_name Function_FindBaseFunc = "Function.FindBaseFunc";

Function* Function::FindBaseFunc() const
{
   Debug::ft(Function_FindBaseFunc);

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
   //  member initialization list (see WasCalled).
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

            if((*oper->Name() == *this->Name()) && SignatureMatches(oper, true))
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

            if((*func->Name() == *this->Name()) && SignatureMatches(func, true))
            {
               return (func->virtual_ ? func : nullptr);
            }
         }
      }
   }

   return nullptr;
}

//------------------------------------------------------------------------------

fn_name Function_FindRootFunc = "Function.FindRootFunc";

Function* Function::FindRootFunc() const
{
   Debug::ft(Function_FindRootFunc);

   auto prev = const_cast< Function* >(this);

   for(auto curr = base_; curr != nullptr; curr = curr->base_)
   {
      prev = curr;
   }

   return prev;
}

//------------------------------------------------------------------------------

fn_name Function_FirstInstance = "Function.FirstInstance";

Function* Function::FirstInstance() const
{
   Debug::ft(Function_FirstInstance);

   auto fti = tmplts_.cbegin();
   if(fti == tmplts_.cend()) return nullptr;
   return *fti;
}

//------------------------------------------------------------------------------

fn_name Function_FirstInstanceInClass = "Function.FirstInstanceInClass";

Function* Function::FirstInstanceInClass() const
{
   Debug::ft(Function_FirstInstanceInClass);

   auto cls = GetClass();
   if(cls == nullptr) return nullptr;
   auto instances = cls->Instances();
   auto cti = instances->cbegin();
   if(cti == instances->cend()) return nullptr;
   return static_cast< Function* >((*cti)->FindInstanceAnalog(this));
}

//------------------------------------------------------------------------------

fn_name Function_FoundFunc = "Function.FoundFunc";

Function* Function::FoundFunc
   (Function* func, const StackArgVector& args, TypeMatch& match)
{
   Debug::ft(Function_FoundFunc);

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
            a->item->Root()->RecordUsage();
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
   if(Name()->find('~') != string::npos) return FuncDtor;
   return FuncCtor;
}

//------------------------------------------------------------------------------

CodeFile* Function::GetDeclFile() const
{
   return (defn_ ? mate_->GetFile() : GetFile());
}

//------------------------------------------------------------------------------

const Function* Function::GetDefn() const
{
   if(defn_) return this;
   if(mate_ != nullptr) return mate_;
   return this;
}

//------------------------------------------------------------------------------

CodeFile* Function::GetDefnFile() const
{
   if(defn_) return GetFile();
   if(mate_ != nullptr) return mate_->GetFile();
   return nullptr;
}

//------------------------------------------------------------------------------

void Function::GetDefnRange(size_t& begin, size_t& end) const
{
   begin = begin_;
   end = end_;
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

CxxScope* Function::GetTemplate() const
{
   if(tmplt_ != nullptr) return tmplt_;
   if(IsTemplate()) return static_cast< CxxScope* >
      (const_cast< Function* >(this));
   auto cls = GetClass();
   if(cls != nullptr) return cls->GetTemplate();
   return nullptr;
}

//------------------------------------------------------------------------------

TemplateType Function::GetTemplateType() const
{
   if(IsTemplate()) return FuncTemplate;

   auto cls = GetClass();

   if(cls != nullptr)
   {
      if(cls->IsTemplate()) return ClassTemplate;
   }

   if(friend_) return ClassTemplate;
   return NonTemplate;
}

//------------------------------------------------------------------------------

fn_name Function_GetUsages = "Function.GetUsages";

void Function::GetUsages(const CodeFile& file, CxxUsageSets& symbols) const
{
   Debug::ft(Function_GetUsages);

   //  Do not report usages in unexecuted or internally generated code, such
   //  as function templates and their instances.
   //
   if((GetTemplateType() != NonTemplate) || IsInternal()) return;

   //  Place the symbols used in the function's signature in a local variable.
   //  The reason for this is discussed below.
   //
   CxxUsageSets usages;

   if(spec_ != nullptr) spec_->GetUsages(file, usages);

   for(size_t i = 0; i < args_.size(); ++i)
   {
      if((i == 0) && this_) continue;
      args_[i]->GetUsages(file, usages);
   }

   //  The symbols in a function signature are always visible
   //  o in the definition (if separate from the declaration)
   //  o in an overridden function (which must #include the base class)
   //  Consequently, symbols used in the signature only need to be reported
   //  when they appear in the declaration of a new function.  Symbols that
   //  were accessed via a using statement are an exception, however, as the
   //  using statement is always required.  Consequently, all usages were
   //  obtained, but only symbols accessed via a using statement are merged
   //  into SYMBOLS in the cases discussed.
   //
   if(!GetDecl()->override_ && !defn_)
   {
      for(auto d = usages.directs.cbegin(); d != usages.directs.cend(); ++d)
      {
         symbols.AddDirect(*d);
      }

      for(auto i = usages.indirects.cbegin(); i != usages.indirects.cend(); ++i)
      {
         symbols.AddIndirect(*i);
      }

      for(auto f = usages.forwards.cbegin(); f != usages.forwards.cend(); ++f)
      {
         symbols.AddForward(*f);
      }

      for(auto f = usages.friends.cbegin(); f != usages.friends.cend(); ++f)
      {
         symbols.AddForward(*f);
      }
   }

   for(auto u = usages.users.cbegin(); u != usages.users.cend(); ++u)
   {
      symbols.AddUser(*u);
   }

   //  If this is a function definition, include the declaration as a usage.
   //
   if(defn_) symbols.AddDirect(mate_);

   if(call_ != nullptr) call_->GetUsages(file, symbols);

   for(auto m = mems_.cbegin(); m != mems_.cend(); ++m)
   {
      (*m)->GetUsages(file, symbols);
   }

   if(impl_ != nullptr) impl_->GetUsages(file, symbols);
}

//------------------------------------------------------------------------------

fn_name Function_HasInvokers = "Function.HasInvokers";

bool Function::HasInvokers() const
{
   Debug::ft(Function_HasInvokers);

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

fn_name Function_IncrThisReads = "Function.IncrThisReads";

void Function::IncrThisReads() const
{
   Debug::ft(Function_IncrThisReads);

   if(this_) args_[0]->WasRead();
}

//------------------------------------------------------------------------------

fn_name Function_IncrThisWrites = "Function.IncrThisWrites";

void Function::IncrThisWrites() const
{
   Debug::ft(Function_IncrThisWrites);

   if(!this_) return;

   auto arg = args_[0].get();
   arg->WasWritten(nullptr, false);
   arg->SetNonConst();

   if(arg->IsConst())
   {
      Context::SwErr(Function_IncrThisWrites, "Function cannot be const", 0);
   }
}

//------------------------------------------------------------------------------

fn_name Function_InstantiateError = "Function.InstantiateError";

Function* Function::InstantiateError(const string& instName, debug32_t offset)
{
   Debug::ft(Function_InstantiateError);

   auto expl = "Failed to instantiate " + instName;
   Context::SwErr(Function_InstantiateError, expl, offset);
   return nullptr;
}

//------------------------------------------------------------------------------

fn_name Function_InstantiateFunction1 = "Function.InstantiateFunction(type)";

Function* Function::InstantiateFunction(const TypeName* type) const
{
   Debug::ft(Function_InstantiateFunction1);

   //  Create the name for the function template instance and look for it.
   //  If it has already been instantiated, return it.
   //
   auto ts = type->TypeString(true);
   auto instName = *Name() + ts;
   RemoveRefs(instName);
   auto area = GetArea();
   auto func = area->FindFunc(instName, nullptr, false, nullptr, nullptr);
   if(func != nullptr) return func;

   //  Notify TYPE, which contains the template name and arguments, that its
   //  template is being instantiated.  This causes the instantiation of any
   //  templates on which this one depends.
   //
   type->Instantiating();

   //  Get the code for the function template, assembling it if this is the
   //  first instantiation.
   //
   if(code_ == nullptr)
   {
      std::ostringstream stream;
      auto options = Flags(FQ_Mask | Code_Mask | NoAC_Mask | NoTP_Mask);
      Display(stream, EMPTY_STR, options);
      code_.reset(new string(stream.str()));
   }

   auto code = stringPtr(new string(*code_));
   if((code == nullptr) || code->empty()) return InstantiateError(instName, 0);

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
   auto parser = std::unique_ptr< Parser >(new Parser(EMPTY_STR));
   parser->ParseFuncInst(fullName, type, area, code);
   parser.reset();
   code.reset();

   func = area->FindFunc(instName, nullptr, false, nullptr, nullptr);
   if(func == nullptr) return InstantiateError(instName, 3);
   func->SetAccess(GetAccess());
   func->SetTemplate(const_cast< Function* >(this));
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
      auto expl = "Invalid number of template arguments for " + *Name();
      Context::SwErr(Function_InstantiateFunction2, expl, tmpltArgs.size());
      return nullptr;
   }

   for(size_t i = 0; i < parms->size(); ++i)
   {
      if(tmpltArgs[i].empty()) return nullptr;
   }

   //  Build the TypeName for the function instance and instantiate it.
   //
   auto name = *Name();
   auto type = TypeNamePtr(new TypeName(name));
   auto scope = Context::Scope();
   auto parser = std::unique_ptr< Parser >(new Parser(scope));

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

void Function::Invoke(StackArgVector* args)
{
   Debug::ft(Function_Invoke);

   auto size1 = (args != nullptr ? args->size() : 0);
   auto size2 = args_.size();

   if(size1 > size2)
   {
      auto expl = "Too many arguments for " + *Name();
      Context::SwErr(Function_Invoke, expl, pack2(size1, size2));
      size1 = size2;
   }

   //  Register a read on each sent argument and check its assignment to
   //  the received argument.
   //
   for(size_t i = 0; i < size1; ++i)
   {
      auto& sendArg = args->at(i);
      sendArg.WasRead();
      auto recvArg = StackArg(args_.at(i).get(), 0);
      sendArg.AssignedTo(recvArg, Passed);
   }

   //  Push the function's result onto the stack and increment the number
   //  of calls to it.
   //
   Context::PushArg(ResultType());
   Context::WasCalled(this);

   //  Generate a warning if a constructor or destructor just invoked a
   //  standard virtual function that is overridden by its own class or
   //  one of its subclasses.
   //
   if(virtual_ && (this->FuncType() == FuncStandard))
   {
      auto func = Context::Scope()->GetFunction();

      if(func != nullptr)
      {
         auto type = func->FuncType();

         if((type == FuncCtor) || (type == FuncDtor))
         {
            auto cls = func->GetClass();

            if(cls->ClassDistance(this->GetClass()) != NOT_A_SUBCLASS)
            {
               if(this->IsOverriddenAtOrBelow(cls))
               {
                  Context::Log(VirtualFunctionInvoked);
               }
            }
         }
      }
   }
}

//------------------------------------------------------------------------------

fn_name Function_InvokeDefaultBaseCtor = "Function.InvokeDefaultBaseCtor";

void Function::InvokeDefaultBaseCtor() const
{
   Debug::ft(Function_InvokeDefaultBaseCtor);

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

fn_name Function_IsDeleted = "Function.IsDeleted";

bool Function::IsDeleted() const
{
   Debug::ft(Function_IsDeleted);

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

fn_name Function_IsExemptFromTracing = "Function.IsExemptFromTracing";

bool Function::IsExemptFromTracing() const
{
   Debug::ft(Function_IsExemptFromTracing);

   if(tmplt_ != nullptr) return true;

   if((impl_ != nullptr) && (impl_->FirstStatement() == nullptr)) return true;

   auto file = GetImplFile();
   if(file != nullptr)
   {
      if(file->Name() == "Algorithms.cpp") return true;
      if(file->Name() == "Formatters.cpp") return true;
      if(file->Name() == "Clock.cpp") return true;
      if(file->Name() == "TraceRecord.cpp") return true;
      if(file->Name() == "CxxString.cpp") return true;
   }

   if(Name()->find("Display") != string::npos) return true;
   if(Name()->find("Patch") != string::npos) return true;
   if(Name()->find("Print") != string::npos) return true;
   if(Name()->find("Show") != string::npos) return true;
   if(Name()->find("Output") != string::npos) return true;
   if(Name()->find("Shrink") != string::npos) return true;
   if(Name()->find("CellDiff") != string::npos) return true;
   if(Name()->find("LinkDiff") != string::npos) return true;
   if(Name()->find("TypeString") != string::npos) return true;
   if(Name()->find("operator==") != string::npos) return true;
   if(Name()->find("operator!=") != string::npos) return true;
   if(Name()->find("operator<<") != string::npos) return true;

   if(spec_ != nullptr)
   {
      auto type = spec_->TypeString(true);
      if(type.find("string") != string::npos) return true;
      if(type.find("char*") != string::npos) return true;
   }

   auto cls = GetClass();
   if(cls != nullptr)
   {
      if(cls->DerivesFrom("TraceRecord")) return true;

      auto ft = FuncType();
      if((ft == FuncCtor) || (ft == FuncDtor))
      {
         if(cls->DerivesFrom("CliIntParm")) return true;
         if(cls->DerivesFrom("CliBoolParm")) return true;
         if(cls->DerivesFrom("CliText")) return true;
         if(cls->DerivesFrom("CliTextParm")) return true;
         if(cls->DerivesFrom("CliCharParm")) return true;
         if(cls->DerivesFrom("CliPtrParm")) return true;
         if(cls->DerivesFrom("EventHandler")) return true;
         if(cls->DerivesFrom("Signal")) return true;
         if(cls->DerivesFrom("Parameter")) return true;
         if(cls->DerivesFrom("BcState")) return true;
      }
   }

   return false;
}

//------------------------------------------------------------------------------

bool Function::IsInTemplateInstance() const
{
   if(tmplt_ != nullptr) return true;
   return CxxScope::IsInTemplateInstance();
}

//------------------------------------------------------------------------------

fn_name Function_IsInvokedInBase = "Function.IsInvokedInBase";

bool Function::IsInvokedInBase() const
{
   Debug::ft(Function_IsInvokedInBase);

   for(auto b = base_; b != nullptr; b = b->base_)
   {
      if(b->calls_ > 0) return true;
   }

   return false;
}

//------------------------------------------------------------------------------

fn_name Function_IsOverriddenAtOrBelow = "Function.IsOverriddenAtOrBelow";

bool Function::IsOverriddenAtOrBelow(const Class* cls) const
{
   Debug::ft(Function_IsOverriddenAtOrBelow);

   for(auto f = overs_.cbegin(); f != overs_.cend(); ++f)
   {
      auto over = (*f)->GetClass();
      if(over->ScopeDistance(cls) != NOT_A_SUBCLASS) return true;
      if((*f)->IsOverriddenAtOrBelow(cls)) return true;
   }

   return false;
}

//------------------------------------------------------------------------------

fn_name Function_IsTrivial = "Function.IsTrivial";

bool Function::IsTrivial() const
{
   Debug::ft(Function_IsTrivial);

   if(tmplt_ != nullptr) return false;
   if(GetDefn()->impl_ == nullptr) return false;

   auto file = GetImplFile();
   if(file == nullptr) return true;

   auto last = file->GetLexer().GetLineNum(end_);
   auto body = false;

   for(auto n = file->GetLexer().GetLineNum(begin_); n < last; ++n)
   {
      auto type = file->GetLineType(n);

      switch(type)
      {
      case OpenBrace:
      case DebugFt:
         body = true;
         //  [[fallthrough]]
      case Blank:
      case EmptyComment:
      case SeparatorComment:
      case TaggedComment:
      case TextComment:
      case SlashAsteriskComment:
         break;

      case CloseBrace:
         return true;

      case Code:
         if(body) return false;
      }
   }

   return true;
}

//------------------------------------------------------------------------------

fn_name Function_IsUndefined = "Function.IsUndefined";

bool Function::IsUndefined() const
{
   Debug::ft(Function_IsUndefined);

   if(GetDefn()->impl_ == nullptr) return true;
   if(IsDeleted()) return true;
   return false;
}

//------------------------------------------------------------------------------

fn_name Function_IsUnused = "Function.IsUnused";

bool Function::IsUnused() const
{
   Debug::ft(Function_IsUnused);

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

fn_name Function_ItemAccessed = "Function.ItemAccessed";

void Function::ItemAccessed(const CxxNamed* item)
{
   Debug::ft(Function_ItemAccessed);

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
         if(ref->GetAccess() != Cxx::Public) nonpublic_ = true;
      }
   }

   //  Check for "this" explicitly.  Its referent is a class, which is usually
   //  public, and it is (implicitly) declared in the function, so it is about
   //  to escape detection.
   //
   if(*item->Name() == THIS_STR)
   {
      nonpublic_ = true;
      nonstatic_ = true;
      return;
   }

   if(item->IsDeclaredInFunction()) return;
   if(item->GetAccess() != Cxx::Public) nonpublic_ = true;
   if(!item->IsStatic()) nonstatic_ = true;
}

//------------------------------------------------------------------------------

fn_name Function_MatchTemplate = "Function.MatchTemplate";

TypeMatch Function::MatchTemplate
   (const string& thisType, const string& thatType,
      stringVector& tmpltParms, stringVector& tmpltArgs, bool& argFound)
{
   Debug::ft(Function_MatchTemplate);

   //  Create TypeSpecs for thisType and thatType by invoking a new parser.
   //  Parsing requires a scope, so use the current one.  Note that const
   //  qualification is stripped when deducing a template argument.
   //
   auto thatNonCVType = RemoveConsts(thatType);
   TypeSpecPtr thisSpec;
   TypeSpecPtr thatSpec;

   auto scope = Context::Scope();
   auto parser = std::unique_ptr< Parser >(new Parser(scope));
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

fn_name Function_MemberToArg = "Function.MemberToArg";

StackArg Function::MemberToArg(StackArg& via, Cxx::Operator op)
{
   Debug::ft(Function_MemberToArg);

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
   Accessed();
   Context::PushArg(StackArg(this));
   if(op == Cxx::REFERENCE_SELECT) via.IncrPtrs();
   via.SetAsThis(true);
   return via;
}

//------------------------------------------------------------------------------

fn_name Function_MinArgs = "Function.MinArgs";

size_t Function::MinArgs() const
{
   Debug::ft(Function_MinArgs);

   size_t min = 0;

   for(auto a = args_.cbegin(); a != args_.cend(); ++a)
   {
      if((*a)->HasDefault()) break;
      ++min;
   }

   return min;
}

//------------------------------------------------------------------------------

fn_name Function_PushThisArg = "Function.PushThisArg";

void Function::PushThisArg(StackArgVector& args) const
{
   Debug::ft(Function_PushThisArg);

   auto exists = (args.empty() ? false : args.front().IsThis());

   if(!exists)
   {
      //  A "this" argument hasn't been pushed.  If the context function
      //  has one, push it as an implicit "this" argument.  If this is a
      //  constructor, however, there may not be a context function (e.g.
      //  during static member initialization), or the context function
      //  may belong to another class, so push our own "this" argument
      //  (which would eventually be the void* returned by operator new).
      //
      if(FuncType() != FuncCtor)
      {
         auto func = Context::Scope()->GetFunction();
         if((func == nullptr) || !func->this_) return;
         auto arg = StackArg(func->args_[0].get(), 0);
         args.insert(args.cbegin(), arg);
      }
      else
      {
         args.insert(args.cbegin(), StackArg(GetClass(), 1));
      }

      args.front().SetAsImplicitThis();
   }
}

//------------------------------------------------------------------------------

fn_name Function_RecordUsage = "Function.RecordUsage";

void Function::RecordUsage() const
{
   Debug::ft(Function_RecordUsage);

   if(tmplt_ == nullptr)
      AddUsage();
   else
      tmplt_->RecordUsage();
}

//------------------------------------------------------------------------------

fn_name Function_ResultType = "Function.ResultType";

StackArg Function::ResultType() const
{
   Debug::ft(Function_ResultType);

   //  Constructors and destructors have no return type.
   //
   if(spec_ != nullptr) return spec_->ResultType();
   if(FuncType() == FuncCtor) return StackArg(GetClass(), 0);
   return StackArg(Singleton< CxxRoot >::Instance()->VoidTerm(), 0);
}

//------------------------------------------------------------------------------

fn_name Function_SetBaseInit = "Function.SetBaseInit";

void Function::SetBaseInit(ExprPtr& init)
{
   Debug::ft(Function_SetBaseInit);

   call_.reset(init.release());
}

//------------------------------------------------------------------------------

fn_name Function_SetDefn = "Function.SetDefn";

void Function::SetDefn(Function* func)
{
   Debug::ft(Function_SetDefn);

   func->mate_ = this;
   func->defn_ = true;
   this->mate_ = func;
}

//------------------------------------------------------------------------------

fn_name Function_SetDefnRange = "Function.SetDefnRange";

void Function::SetDefnRange(size_t begin, size_t end)
{
   Debug::ft(Function_SetDefnRange);

   begin_ = begin;
   end_ = end;
}

//------------------------------------------------------------------------------

fn_name Function_SetImpl = "Function.SetImpl";

void Function::SetImpl(BlockPtr& block)
{
   Debug::ft(Function_SetImpl);

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

fn_name Function_SetOperator = "Function.SetOperator";

void Function::SetOperator(Cxx::Operator oper)
{
   Debug::ft(Function_SetOperator);

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

fn_name Function_SetStatic = "Function.SetStatic";

void Function::SetStatic(bool stat, Cxx::Operator oper)
{
   Debug::ft(Function_SetStatic);

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

fn_name Function_SetTemplateParms = "Function.SetTemplateParms";

void Function::SetTemplateParms(TemplateParmsPtr& parms)
{
   Debug::ft(Function_SetTemplateParms);

   parms_ = std::move(parms);
}

//------------------------------------------------------------------------------

void Function::Shrink()
{
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
   CxxStats::Vectors(CxxStats::FUNC_DECL, size);
}

//------------------------------------------------------------------------------

fn_name Function_SignatureMatches = "Function.SignatureMatches";

bool Function::SignatureMatches(const Function* that, bool base) const
{
   Debug::ft(Function_SignatureMatches);

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
         ts = *GetClass()->Name() + '*';
         break;
      case FuncDtor:
         ts = VOID_STR;
         break;
      default:
         auto expl = "Return type not found for " + *Name();
         Context::SwErr(Function_TypeString, expl, ft);
         return ERROR_STR;
      }
   }

   //  If the function is not an argument, include its fully qualified
   //  name after its return type.  (When the function is an argument,
   //  only its signature, and not its name, is included.)
   //
   if(!arg)
   {
      ts += SPACE + Prefix(GetScope()->TypeString(false)) + *Name();
   }

   //  Add the full type of each argument within parentheses, using
   //  commas to separate them.
   //
   ts += '(';

   for(auto a = args_.cbegin(); a != args_.cend(); ++a)
   {
      ts += (*a)->TypeString(arg);
      if(*a != args_.back()) ts += ',';
   }

   return ts + ')';
}

//------------------------------------------------------------------------------

fn_name Function_UpdateThisArg = "Function.UpdateThisArg";

void Function::UpdateThisArg(StackArgVector& args) const
{
   Debug::ft(Function_UpdateThisArg);

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
         if(!args.front().IsImplicitThis()) Log(StaticFunctionViaMember);
         args.erase(args.cbegin());
      }
   }
   else
   {
      //  Set a constructor's "this" argument to its actual "this" argument.
      //  A pointer to the class acts as the implicit "this" argument until
      //  the constructor is found.
      //
      if(FuncType() == FuncCtor) args.front() = StackArg(args_[0].get(), 0);
   }
}

//------------------------------------------------------------------------------

fn_name Function_WasCalled = "Function.WasCalled";

void Function::WasCalled()
{
   Debug::ft(Function_WasCalled);

   //  Don't record a recursive invocation.  It's a minor thing, but a
   //  function should be logged as unused if its only invoker is itself.
   //
   if(Context::Scope()->GetFunction() == this) return;

   ++calls_;

   auto type = FuncType();

   switch(type)
   {
   case FuncDtor:
      //
      //  Record invocations up the class hierarchy.
      //
      for(auto dtor = GetDecl()->base_; dtor != nullptr; dtor = dtor->base_)
      {
         ++dtor->calls_;
      }
      break;

   case FuncCtor:
      //
      //  If this is an invocation by a derived class's constructor,
      //  set this constructor as its base.  Its constructor could
      //  also cause the invocation of other constructors, but its
      //  superclass constructor will be the first.
      //
      auto func = Context::Scope()->GetFunction();

      if((func != nullptr) && (func->FuncType() == FuncCtor))
      {
         if(func->GetDecl()->base_ == nullptr) func->GetDecl()->base_ = this;
      }

      //  Now record invocations up the class hierarchy.
      //
      for(auto ctor = GetDecl()->base_; ctor != nullptr; ctor = ctor->base_)
      {
         ++ctor->calls_;
      }
      break;
   }

   //  For a function template instance, record an invocation on the
   //  function template.
   //
   if(tmplt_ != nullptr) ++tmplt_->calls_;

   //  For a function in a class template instance, record an invocation
   //  on the class template's function.
   //
   auto cls = GetClass();

   if((cls != nullptr) && cls->IsInTemplateInstance())
   {
      auto func = static_cast< ClassInst* >(cls)->FindTemplateAnalog(this);
      if(func != nullptr) ++static_cast< Function* >(func)->calls_;
   }
}

//==============================================================================

fn_name FuncSpec_ctor = "FuncSpec.ctor";

FuncSpec::FuncSpec(FunctionPtr& func) : func_(func.release())
{
   Debug::ft(FuncSpec_ctor);

   CxxStats::Incr(CxxStats::FUNC_SPEC);
}

//------------------------------------------------------------------------------

fn_name FuncSpec_Warning = "FuncSpec.Warning";

//------------------------------------------------------------------------------

void FuncSpec::AddArray(ArraySpecPtr& array)
{
   func_->GetTypeSpec()->AddArray(array);
}

//------------------------------------------------------------------------------

void FuncSpec::AdjustPtrs(TagCount count)
{
   func_->GetTypeSpec()->AdjustPtrs(count);
}

//------------------------------------------------------------------------------

std::string FuncSpec::AlignTemplateArg(const TypeSpec* thatArg) const
{
   return func_->GetTypeSpec()->AlignTemplateArg(thatArg);
}

//------------------------------------------------------------------------------

TagCount FuncSpec::ArrayCount() const
{
   return func_->GetTypeSpec()->ArrayCount();
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
   Debug::SwErr(FuncSpec_Warning, "Clone", 0);
   return nullptr;
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
   Debug::SwErr(FuncSpec_Warning, "EnterArrays", 0);
   func_->GetTypeSpec()->EnterArrays();
}

//------------------------------------------------------------------------------

fn_name FuncSpec_EnteringScope = "FuncSpec.EnteringScope";

void FuncSpec::EnteringScope(const CxxScope* scope)
{
   Debug::ft(FuncSpec_EnteringScope);

   func_->EnterSignature();
}

//------------------------------------------------------------------------------

void FuncSpec::FindReferent()
{
   Debug::SwErr(FuncSpec_Warning, "FindReferent", 0);
   func_->GetTypeSpec()->FindReferent();
}

//------------------------------------------------------------------------------

TypeTags FuncSpec::GetTags() const
{
   return func_->GetTypeSpec()->GetTags();
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

void FuncSpec::Instantiating() const
{
   Debug::SwErr(FuncSpec_Warning, "Instantiating", 0);
   func_->GetTypeSpec()->Instantiating();
}

//------------------------------------------------------------------------------

bool FuncSpec::IsConst() const
{
   return func_->IsConst();
}

//------------------------------------------------------------------------------

bool FuncSpec::IsConstPtr() const
{
   return func_->GetTypeSpec()->IsConstPtr();
}

//------------------------------------------------------------------------------

bool FuncSpec::MatchesExactly(const TypeSpec* that) const
{
   Debug::SwErr(FuncSpec_Warning, "MatchesExactly", 0);
   return func_->GetTypeSpec()->MatchesExactly(that);
}

//------------------------------------------------------------------------------

TypeMatch FuncSpec::MatchTemplate(TypeSpec* that, stringVector& tmpltParms,
   stringVector& tmpltArgs, bool& argFound) const
{
   Debug::SwErr(FuncSpec_Warning, "MatchTemplate", 0);
   return func_->GetTypeSpec()->MatchTemplate
      (that, tmpltParms, tmpltArgs, argFound);
}

//------------------------------------------------------------------------------

TypeMatch FuncSpec::MatchTemplateArg(const TypeSpec* that) const
{
   Debug::SwErr(FuncSpec_Warning, "MatchTemplateArg", 0);
   return func_->GetTypeSpec()->MatchTemplateArg(that);
}

//------------------------------------------------------------------------------

const string* FuncSpec::Name() const
{
   return func_->Name();
}

//------------------------------------------------------------------------------

void FuncSpec::Print(ostream& stream, const Flags& options) const
{
   func_->DisplayDecl(stream, Flags());
}

//------------------------------------------------------------------------------

TagCount FuncSpec::PtrCount(bool arrays) const
{
   return func_->GetTypeSpec()->PtrCount(arrays);
}

//------------------------------------------------------------------------------

TagCount FuncSpec::Ptrs(bool arrays) const
{
   return func_->GetTypeSpec()->Ptrs(arrays);
}

//------------------------------------------------------------------------------

TagCount FuncSpec::RefCount() const
{
   return func_->GetTypeSpec()->RefCount();
}

//------------------------------------------------------------------------------

TagCount FuncSpec::Refs() const
{
   return func_->GetTypeSpec()->Refs();
}

//------------------------------------------------------------------------------

void FuncSpec::RemoveRefs()
{
   return func_->GetTypeSpec()->RemoveRefs();
}

//------------------------------------------------------------------------------

StackArg FuncSpec::ResultType() const
{
   return func_->ResultType();
}

//------------------------------------------------------------------------------

void FuncSpec::SetArrayPos(int8_t pos)
{
   Debug::SwErr(FuncSpec_Warning, "SetArrayPos", 0);
   func_->GetTypeSpec()->SetArrayPos(pos);
}

//------------------------------------------------------------------------------

void FuncSpec::SetConst(bool readonly)
{
   Debug::SwErr(FuncSpec_Warning, "SetConst", 0);
   func_->GetTypeSpec()->SetConst(readonly);
}

//------------------------------------------------------------------------------

void FuncSpec::SetConstPtr(bool constptr)
{
   Debug::SwErr(FuncSpec_Warning, "SetConstPtr", 0);
   func_->GetTypeSpec()->SetConst(constptr);
}

//------------------------------------------------------------------------------

void FuncSpec::SetPtrDetached(bool on)
{
   Debug::SwErr(FuncSpec_Warning, "SetPtrDetached", 0);
   func_->GetTypeSpec()->SetPtrDetached(on);
}

//------------------------------------------------------------------------------

void FuncSpec::SetPtrs(TagCount ptrs)
{
   Debug::SwErr(FuncSpec_Warning, "SetPtrs", 0);
   func_->GetTypeSpec()->SetPtrs(ptrs);
}

//------------------------------------------------------------------------------

void FuncSpec::SetRefDetached(bool on)
{
   Debug::SwErr(FuncSpec_Warning, "SetRefDetached", 0);
   func_->GetTypeSpec()->SetRefDetached(on);
}

//------------------------------------------------------------------------------

void FuncSpec::SetReferent(CxxNamed* item, const SymbolView* view) const
{
   Debug::SwErr(FuncSpec_Warning, "SetReferent", 0);
   func_->GetTypeSpec()->SetReferent(item, view);
}

//------------------------------------------------------------------------------

void FuncSpec::SetRefs(TagCount refs)
{
   Debug::SwErr(FuncSpec_Warning, "SetRefs", 0);
   func_->GetTypeSpec()->SetRefs(refs);
}

//------------------------------------------------------------------------------

void FuncSpec::Shrink()
{
   func_->Shrink();
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

//==============================================================================

fn_name SpaceData_ctor = "SpaceData.ctor";

SpaceData::SpaceData(QualNamePtr& name, TypeSpecPtr& type) : Data(type),
   name_(name.release())
{
   Debug::ft(SpaceData_ctor);

   auto qname = name_->QualifiedName(true, false);
   OpenScope(qname);
   CxxStats::Incr(CxxStats::FILE_DATA);
}

//------------------------------------------------------------------------------

fn_name SpaceData_dtor = "SpaceData.dtor";

SpaceData::~SpaceData()
{
   Debug::ft(SpaceData_dtor);

   CloseScope();
   Singleton< CxxSymbols >::Instance()->EraseData(this);
   CxxStats::Decr(CxxStats::FILE_DATA);
}

//------------------------------------------------------------------------------

fn_name SpaceData_Check = "SpaceData.Check";

void SpaceData::Check() const
{
   Debug::ft(SpaceData_Check);

   Data::Check();

   if(parms_ != nullptr) parms_->Check();

   if(IsDecl())
   {
      CheckUsage();
      CheckConstness();
      CheckIfStatic();
      CheckIfInitialized();
   }
}

//------------------------------------------------------------------------------

fn_name SpaceData_CheckIfInitialized = "SpaceData.CheckIfInitialized";

void SpaceData::CheckIfInitialized() const
{
   Debug::ft(SpaceData_CheckIfInitialized);

   //  Data declared at file scope should be initialized.
   //
   if(!WasInited()) Log(DataUninitialized);
}

//------------------------------------------------------------------------------

fn_name SpaceData_CheckIfStatic = "SpaceData.CheckIfStatic";

void SpaceData::CheckIfStatic() const
{
   Debug::ft(SpaceData_CheckIfStatic);

   //  Static data in a header is dubious.  Unless it is tagged extern, it
   //  defaults to static.  However, constexpr (even though theoretically
   //  static) is often preferable to the use of extern.
   //
   if(!IsExtern() && !IsConstexpr() && GetFile()->IsHeader())
   {
      Log(GlobalStaticData);
   }
}

//------------------------------------------------------------------------------

void SpaceData::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   auto fq = options.test(DispFQ);

   stream << prefix;
   if(IsExtern()) stream << EXTERN_STR << SPACE;
   if(IsStatic()) stream << STATIC_STR << SPACE;
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

fn_name SpaceData_EnterScope = "SpaceData.EnterScope";

bool SpaceData::EnterScope()
{
   Debug::ft(SpaceData_EnterScope);

   Context::SetPos(GetPos());
   GetTypeSpec()->EnteringScope(this);
   CloseScope();

   //  See whether this is a new declaration or the definition
   //  of previously declared data (i.e. class static data or
   //  data that was declared extern).
   //
   auto decl = GetArea()->FindData(*Name());
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

fn_name SpaceData_GetInitName = "SpaceData.GetInitName";

void SpaceData::GetInitName(QualNamePtr& qualName) const
{
   Debug::ft(SpaceData_GetInitName);

   qualName.reset(new QualName(*name_));
}

//------------------------------------------------------------------------------

fn_name SpaceData_SetTemplateParms = "SpaceData.SetTemplateParms";

void SpaceData::SetTemplateParms(TemplateParmsPtr& parms)
{
   Debug::ft(SpaceData_SetTemplateParms);

   parms_ = std::move(parms);
}

//------------------------------------------------------------------------------

void SpaceData::Shrink()
{
   Data::Shrink();

   name_->Shrink();
   if(parms_ != nullptr) parms_->Shrink();
}
}
