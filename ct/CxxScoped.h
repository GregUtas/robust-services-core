//==============================================================================
//
//  CxxScoped.h
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
#ifndef CXXSCOPED_H_INCLUDED
#define CXXSCOPED_H_INCLUDED

#include "CxxNamed.h"
#include <cstddef>
#include <iosfwd>
#include <string>
#include "CodeTypes.h"
#include "Cxx.h"
#include "CxxFwd.h"
#include "LibraryTypes.h"
#include "SysTypes.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace CodeTools
{
//  The base class for C++ entities that track the scope in which they appear
//  and are typically subject to access control.
//
class CxxScoped : public CxxNamed
{
public:
   //  Virtual to allow subclassing.
   //
   virtual ~CxxScoped();

   //  Returns the file where the item is implemented.
   //
   CodeFile* GetImplFile() const;

   //  Returns true if NAME, used in SCOPE and FILE, could refer to this ITEM.
   //  NAME must either match the item's fully qualified name or a trailing
   //  portion of it that follows a scope qualifier.  Updates VIEW to specify
   //  how ITEM was accessed when true is returned.
   //
   bool NameRefersToItem(const std::string& name,
      const CxxScope* scope, const CodeFile* file, SymbolView* view) const;

   //  Returns true if the item is a member of AREA.  The search stops after
   //  it reaches the first namespace.
   //
   bool IsDefinedIn(const CxxArea* area) const;

   //  Updates VIEW to indicate this item's accessibility to SCOPE.
   //
   virtual void AccessibilityTo(const CxxScope* scope, SymbolView* view) const;

   //  Determines this item's visibility at file scope if it is public or
   //  its user is known to be a friend.
   //
   Accessibility FileScopeAccessiblity() const;

   //  Records that the item required *at least* ACCESS to be accessible.
   //
   virtual void RecordAccess(Cxx::Access access) const;

   //  Returns the least restrictive access control that was required
   //  to access the item.
   //
   Cxx::Access BroadestAccessUsed() const;

   //  Returns an item with the same name that is defined in a base class.
   //
   CxxScoped* FindInheritedName() const;

   //  Displays the filename where the item is declared.  If the item is
   //  defined in another file, that file is also displayed.
   //
   void DisplayFiles(std::ostream& stream) const;

   //  Updates imSet with the files that declare and define the item.
   //
   virtual void AddFiles(SetOfIds& imSet) const;

   //  Returns true if the item is unused.
   //
   virtual bool IsUnused() const { return false; }

   //  Overridden to return the access control level for the item.
   //
   virtual Cxx::Access GetAccess() const override { return access_; }

   //  Overridden to return the scope where the declaration appeared.
   //
   virtual CxxScope* GetScope() const override { return scope_; }

   //  Overridden to indicate that inline display is not supported.
   //
   virtual bool InLine() const override { return false; }

   //  Overridden to invoke IsConst on GetTypeSpec unless the latter
   //  returns nullptr, in which case it returns false.
   //
   virtual bool IsConst() const override;

   //  Overridden to invoke IsConstPtr on GetTypeSpec unless the latter
   //  returns nullptr, in which case it returns false.
   //
   virtual bool IsConstPtr() const override;

   //  Overridden to return true if the item is declared in a code block
   //  or argument list.
   //
   virtual bool IsDeclaredInFunction() const override;

   //  Overridden to invoke IsAuto on GetTypeSpec unless the latter
   //  returns nullptr, in which case it returns false.
   //
   virtual bool IsAuto() const override;

   //  Overridden to invoke IsIndirect on GetTypeSpec unless the latter
   //  returns nullptr, in which case it returns false.
   //
   virtual bool IsIndirect() const override;

   //  Overridden to return the item itself.
   //
   virtual CxxNamed* Referent() const override { return (CxxNamed*) this; }

   //  Overridden to set the access control level for the item.
   //
   virtual void SetAccess(Cxx::Access access) override { access_ = access; }

   //  Overridden to set the scope where the declaration appeared.
   //
   virtual void SetScope(CxxScope* scope) const override { scope_ = scope; }
protected:
   //  Protected because this class is virtual.
   //
   CxxScoped();

   //  Logs an unused item.  The default version generates a log that
   //  contains WARNING if IsUnused returns true.
   //
   virtual void CheckIfUsed(Warning warning) const;

   //  Logs an item whose name hides a name defined in a base class.
   //
   virtual void CheckIfHiding() const;

   //  Logs an item whose access control could be more restrictive.
   //
   virtual void CheckAccessControl() const;
private:
   //  Overridden to prohibit copying.
   //
   CxxScoped(const CxxScoped& that);

   //  The scope where the item appeared.
   //
   mutable CxxScope* scope_;

   //  The access control level for the item.
   //
   Cxx::Access access_ : 8;

   //  Set if the function required public access.
   //
   mutable bool public_;

   //  Set if the function required protected access.
   //
   mutable bool protected_;
};

//------------------------------------------------------------------------------
//
//  An argument to a function.
//
class Argument : public CxxScoped
{
public:
   //  Creates an argument defined by NAME (which might be nullptr) and SPEC.
   //
   Argument(std::string& name, TypeSpecPtr& spec);

   //  Not subclassed.
   //
   ~Argument() { CxxStats::Decr(CxxStats::ARG_DECL); }

   //  Sets the argument's default value.
   //
   void SetDefault(ExprPtr& default) { default_.reset(default.release()); }

   //  Returns true if the argument has a default value.
   //
   bool HasDefault() const { return (default_ != nullptr); }

   //  Returns true if the argument is passed by value.
   //
   bool IsPassedByValue() const;

   //  Returns true if the argument could be const.
   //
   bool CouldBeConst() const { return !nonconst_; }

   //  Overridden to set the type for an "auto" variable.
   //
   virtual CxxToken* AutoType() const override { return spec_.get(); }

   //  Overridden to log warnings associated with the argument.
   //
   virtual void Check() const override;

   //  Overridden to make the argument visible within its function.
   //
   virtual void EnterBlock() override;

   //  Overridden to remove the argument as a local.
   //
   virtual void ExitBlock() override;

   //  Overridden to invoke EnterBlock on the argument's type (spec_) and
   //  any default value (default_).
   //
   virtual bool EnterScope() override;

   //  Overridden to return the argument's underlying numeric type.
   //
   virtual Numeric GetNumeric() const override { return spec_->GetNumeric(); }

   //  Overridden to return the argument's type.
   //
   virtual TypeSpec* GetTypeSpec() const override { return spec_.get(); }

   //  Overridden to update SYMBOLS with the argument's type usage.
   //
   virtual void GetUsages
      (const CodeFile& file, CxxUsageSets& symbols) const override;

   //  Overridden to indicate that inline display is supported.
   //
   virtual bool InLine() const override { return true; }

   //  Overridden to return false for a "this" argument.
   //
   virtual bool IsStatic() const override { return (name_ != THIS_STR); }

   //  Overridden to determine if the argument is unused.
   //
   virtual bool IsUnused() const override { return ((reads_ + writes_) == 0); }

   //  Overridden to return the argument's name, if any.
   //
   virtual const std::string* Name() const override { return &name_; }

   //  Overridden to display the argument.
   //
   virtual void Print(std::ostream& stream) const override;

   //  Overridden to record that the argument cannot be const.
   //
   virtual bool SetNonConst() override;

   //  Overridden to shrink containers.
   //
   virtual void Shrink() override;

   //  Overridden to reveal that this is an argument.
   //
   virtual Cxx::ItemType Type() const override { return Cxx::Argument; }

   //  Overridden to return the argument's full root type.
   //
   virtual std::string TypeString(bool arg) const override;

   //  Overridden to increment the number of reads.
   //
   virtual bool WasRead() override { ++reads_; return true; }

   //  Overridden to increment the number of writes.
   //
   virtual bool WasWritten(const StackArg* arg, bool passed) override;
private:
   //  Overridden to return the argument's type.
   //
   virtual CxxToken* RootType() const override { return spec_.get(); }

   //  Checks for a "(void)" argument.
   //
   void CheckVoid() const;

   //  The argument's name, if any.
   //
   std::string name_;

   //  The argument's type.
   //
   const TypeSpecPtr spec_;

   //  The argument's default value, if any.
   //
   ExprPtr default_;

   //  How many times the argument was read.
   //
   size_t reads_ : 16;

   //  How many times the argument was written.
   //
   size_t writes_ : 15;

   //  Set if the argument cannot be const.
   //
   bool nonconst_ : 1;
};

//------------------------------------------------------------------------------
//
//  A base class declaration.
//
class BaseDecl : public CxxScoped
{
public:
   //  Invoked when a new class subclasses from NAME.  ACCESS is the access
   //  control for the base class; it is currently assumed to be "public".
   //
   BaseDecl(QualNamePtr& name, Cxx::Access access);

   //  Not subclassed.
   //
   ~BaseDecl() { CxxStats::Decr(CxxStats::BASE_DECL); }

   //  Displays the base class declaration.
   //
   void DisplayDecl(std::ostream& stream, bool fq) const;

   //  Overridden to record the current scope as a subclass of the base class.
   //
   virtual bool EnterScope() override;

   //  Overridden to return the class that the declaration refers to.
   //
   virtual Class* GetClass() const override;

   //  Overridden to return the base class's qualified name.
   //
   virtual QualName* GetQualName() const override { return name_.get(); }

   //  Overridden to update SYMBOLS with the declaration's type usage.
   //
   virtual void GetUsages
      (const CodeFile& file, CxxUsageSets& symbols) const override;

   //  Overridden to indicate that inline display is supported.
   //
   virtual bool InLine() const override { return true; }

   //  Overridden to return the base class's name.
   //
   virtual const std::string* Name() const override { return name_->Name(); }

   //  Overridden to return the base class's qualified name.
   //
   virtual std::string QualifiedName(bool scopes, bool templates) const
      override { return name_->QualifiedName(scopes, templates); }

   //  Overridden to return the base class.
   //
   virtual CxxNamed* Referent() const override;

   //  Overridden to return the base class's scoped name.
   //
   virtual std::string ScopedName(bool templates) const override;

   //  Overridden to preserve the access control set by the constructor.
   //
   virtual void SetAccess(Cxx::Access access) override;

   //  Overridden to shrink containers.
   //
   virtual void Shrink() override{ name_->Shrink(); }

   //  Overridden to return the declaration's full root type.
   //
   virtual std::string TypeString(bool arg) const override;
private:
   //  Overridden to find the base class's class.
   //
   virtual bool FindReferent() override;

   //  The (possibly) qualified name of the base class.
   //
   const QualNamePtr name_;

   //  Set if a using statement made the base class visible.
   //
   bool using_;
};

//------------------------------------------------------------------------------
//
//  An enumeration.
//
class Enum : public CxxScoped
{
public:
   //  Creates an enumeration for NAME.
   //
   explicit Enum(std::string& name);

   //  Not subclassed.
   //
   ~Enum();

   //  Adds an enumerator, with NAME and INIT, to the enumeration.  POS is
   //  its location within the source code file.
   //
   void AddEnumerator(std::string& name, ExprPtr& init, size_t pos);

   //  Returns the enumerator defined by NAME.
   //
   Enumerator* FindEnumerator(const std::string& name) const;

   //  Overridden to set the type for an "auto" variable.
   //
   virtual CxxToken* AutoType() const override { return (CxxToken*) this; }

   //  Overridden to log warnings associated with the enumeration.
   //
   virtual void Check() const override;

   //  Overridden to check each enumerator before suggesting a more
   //  restrictive access control.
   //
   virtual void CheckAccessControl() const override;

   //  Overridden to display the enumeration.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden to make the enumeration visible as a local.
   //
   virtual void EnterBlock() override;

   //  Overridden to add the enumeration to the current scope.
   //
   virtual bool EnterScope() override;

   //  Overridden to remove the enumeration as a local.
   //
   virtual void ExitBlock() override;

   //  Overridden to indicate that an enum can be converted to an integer.
   //
   virtual Numeric GetNumeric() const override { return Numeric::Enum; }

   //  Overridden to determine if the enum and all its enumerators are unused.
   //
   virtual bool IsUnused() const override;

   //  Overridden to return the enumeration's name.
   //
   virtual const std::string* Name() const override { return &name_; }

   //  Overridden to record usage of the enumeration.
   //
   virtual void RecordUsage() const override { AddUsage(); }

   //  Overridden to count references.
   //
   virtual void SetAsReferent(const CxxNamed* user) override { ++refs_; }

   //  Overridden to shrink containers.
   //
   virtual void Shrink() override;

   //  Overridden to reveal that this is an enumeration.
   //
   virtual Cxx::ItemType Type() const override { return Cxx::Enum; }

   //  Overridden to return an enumeration's fully qualified name.
   //
   virtual std::string TypeString(bool arg) const override;
private:
   //  The enumeration's name.
   //
   std::string name_;

   //  The enumerators.
   //
   EnumeratorPtrVector etors_;

   //  How many times the enumeration was used as a type.
   //
   size_t refs_ : 16;
};

//------------------------------------------------------------------------------
//
//  An enumerator.
//
class Enumerator : public CxxScoped
{
public:
   //  Creates an enumerator with NAME and INIT, belonging to DECL.
   //
   Enumerator(std::string& name, ExprPtr& init, const Enum* decl);

   //  Not subclassed.
   //
   ~Enumerator();

   //  Overridden to set the type for an "auto" variable.
   //
   virtual CxxToken* AutoType() const override { return (CxxNamed*) enum_; }

   //  Overridden to log warnings associated with the enumerator.
   //
   virtual void Check() const override;

   //  Overridden to display the enumeration.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden to make the enumerator visible as a local and to execute
   //  its initialization statement.
   //
   virtual void EnterBlock() override;

   //  Overridden to execute the enumerator's initialization statement.
   //
   virtual bool EnterScope() override;

   //  Overridden to remove the enumerator as a local.
   //
   virtual void ExitBlock() override;

   //  Overridden to count references to the enumerator.
   //
   virtual bool WasRead() override { ++refs_; return true; }

   //  Overridden to indicate that an enum can be converted to an integer.
   //
   virtual Numeric GetNumeric() const override { return Numeric::Enum; }

   //  Overridden to enable promotion of the enumerator to its enum's scope.
   //
   virtual bool GetScopedName(std::string& name, size_t n) const override;

   //  Overridden to determine if the enumerator is unused.
   //
   virtual bool IsUnused() const override { return (refs_ == 0); }

   //  Overridden to return the enumerator's name.
   //
   virtual const std::string* Name() const override { return &name_; }

   //  Overridden to prefix the enum as a scope.
   //
   virtual std::string ScopedName(bool templates) const override;

   //  Overridden to note that the enumeration required ACCESS.
   //
   virtual void RecordAccess(Cxx::Access access) const override;

   //  Overridden to record usage of the enumerator.
   //
   virtual void RecordUsage() const override { AddUsage(); }

   //  Overridden to count references.
   //
   virtual void SetAsReferent(const CxxNamed* user) override { ++refs_; }

   //  Overridden to shrink containers.
   //
   virtual void Shrink() override;

   //  Overridden to reveal that this is an enumerator.
   //
   virtual Cxx::ItemType Type() const override { return Cxx::Enumerator; }

   //  Overridden to return the enumeration's type.
   //
   virtual std::string TypeString(bool arg) const override;
private:
   //  Overridden to prohibit copying.
   //
   Enumerator(const Enumerator& that);

   //  The enumerator's name.
   //
   std::string name_;

   //  The enumerator's initialization statement, if any.
   //
   const ExprPtr init_;

   //  The enumeration to which the enumerator belongs.
   //
   const Enum* const enum_;

   //  How many times the enumerator was referenced.
   //
   size_t refs_ : 16;
};

//------------------------------------------------------------------------------
//
//  A forward declaration.
//
class Forward : public CxxScoped
{
public:
   //  Creates a forward declaration for a class of TYPE and NAME.
   //
   Forward(QualNamePtr& name, Cxx::ClassTag tag);

   //  Not subclassed.
   //
   ~Forward();

   //  Overridden to return the referent if known, else the forward declaration.
   //
   virtual CxxToken* AutoType() const override;

   //  Overridden to log warnings associated with the declaration.
   //
   virtual void Check() const override;

   //  Overridden to display the declaration.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden to push the declaration's referent onto the argument stack.
   //
   virtual void EnterBlock() override;

   //  Overridden to add the declaration to the current scope.
   //
   virtual bool EnterScope() override;

   //  Overridden to return the class's qualified name.
   //
   virtual QualName* GetQualName() const override { return name_.get(); }

   //  Overridden to determine if the declaration is unused.
   //
   virtual bool IsUnused() const override { return (users_ == 0); }

   //  Overridden to returns the class's name.
   //
   virtual const std::string* Name() const override { return name_->Name(); }

   //  Overridden to return the class.
   //
   virtual CxxNamed* Referent() const override;

   //  Overridden to return the class's scoped name.
   //
   virtual std::string ScopedName(bool templates) const override;

   //  Overridden to count usages.
   //
   virtual void SetAsReferent(const CxxNamed* user) override { ++users_; }

   //  Overridden to shrink containers.
   //
   virtual void Shrink() override;

   //  Overridden to reveal that this is a forward declaration.
   //
   virtual Cxx::ItemType Type() const override { return Cxx::Forward; }

   //  Overridden to return the class's full type.
   //
   virtual std::string TypeString(bool arg) const override;
private:
   //  Overridden to return the class.
   //
   virtual CxxToken* RootType() const override { return Referent(); }

   //  The class's type.
   //
   const Cxx::ClassTag tag_ : 8;

   //  The class's name.
   //
   const QualNamePtr name_;

   //  How many times the declaration resolved a symbol.
   //
   size_t users_ : 16;
};

//------------------------------------------------------------------------------
//
//  A friend declaration.
//
class Friend : public CxxScoped
{
public:
   //  Creates a friend declaration upon parsing "friend".
   //
   Friend();

   //  Not subclassed.
   //
   ~Friend();

   //  Sets the type of friend.
   //
   void SetTag(Cxx::ClassTag tag) { tag_ = tag; }

   //  Sets the friend's name.
   //
   void SetName(QualNamePtr& name);

   //  Sets FUNC when the friend is a function.
   //
   void SetFunc(FunctionPtr& func);

   //  Returns the function for a friend definition (inline).
   //
   Function* Inline() const { return inline_; }

   //  Tracks how many times the declaration was used to access something
   //  that would otherwise have been restricted.
   //
   void IncrUsers() { ++users_; }

   //  Adds each users_ in a set of class template INSTANCES to the users_
   //  of the original friend declaration in the class template.
   //
   void AddUsers(const ClassInstPtrVector& instances);

   //  Overridden to return the referent if known, else the friend declaration.
   //
   virtual CxxToken* AutoType() const override;

   //  Overridden to log warnings associated with the declaration.
   //
   virtual void Check() const override;

   //  Overridden to display the declaration.  If FQ is set, the friend's
   //  fully qualified name is displayed.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden to push the declaration's referent onto the argument stack.
   //
   virtual void EnterBlock() override;

   //  Overridden to add the declaration to the current scope.
   //
   virtual bool EnterScope() override;

   //  Overridden to return the friend if it is a function.
   //
   virtual Function* GetFunction() const override;

   //  Overridden to return the declaration's qualified name.
   //
   virtual QualName* GetQualName() const override;

   //  Overridden to update SYMBOLS with the declaration's type usage.
   //
   virtual void GetUsages
      (const CodeFile& file, CxxUsageSets& symbols) const override;

   //  Overridden to indicate that inline display is not supported.
   //
   virtual bool InLine() const override { return false; }

   //  Overridden to determine if the declaration is unused.
   //
   virtual bool IsUnused() const override { return (users_ == 0); }

   //  Overridden to return the friend's name.
   //
   virtual const std::string* Name() const override;

   //  Overridden to return the friend's qualified name.
   //
   virtual std::string QualifiedName
      (bool scopes, bool templates) const override;

   //  Overridden to return the friend.
   //
   virtual CxxNamed* Referent() const override;

   //  Overridden to apply the arguments after updating the scope to that
   //  of the class template.
   //
   virtual bool ResolveTemplate
      (Class* cls, const TypeName* args, bool end) const override;

   //  Overridden to return the friend's scoped name.
   //
   virtual std::string ScopedName(bool templates) const override;

   //  Overridden to log a warning.  A forward declaration, not a friend
   //  declaration, should be used to resolve an indirect type.
   //
   virtual void SetAsReferent(const CxxNamed* user) override;

   //  Overridden to shrink containers.
   //
   virtual void Shrink() override;

   //  Overridden to reveal that this is a friend declaration.
   //
   virtual Cxx::ItemType Type() const override { return Cxx::Friend; }

   //  Overridden to return the friend's full type.
   //
   virtual std::string TypeString(bool arg) const override;
private:
   //  Overridden to find the item that the declaration refers to.
   //
   virtual bool FindReferent() override;

   //  Overridden to record DECL and update the friend's scope.
   //
   virtual bool ResolveForward(CxxScoped* decl, size_t n) const override;

   //  Overridden to return the friend.
   //
   virtual CxxToken* RootType() const override { return Referent(); }

   //  Finds the item that the declaration refers to when it was not
   //  visible from the scope where the declaration appeared.
   //
   CxxNamed* FindForward() const;

   //  If REF is valid, sets it as the referent and returns true, else
   //  returns false.
   //
   bool SetReferent(CxxNamed* ref) const;

   //  Returns the referent.
   //
   CxxNamed* GetReferent() const;

   //  Overridden to prohibit copying.
   //
   Friend(const Friend& that);

   //  The friend's qualified name.
   //
   QualNamePtr name_;

   //  If the friend is a non-inlined function, its specification.
   //
   FunctionPtr func_;

   //  If the friend is an inlined function, a pointer to it.
   //
   Function* inline_;

   //  If the friend is a class, its type.
   //
   Cxx::ClassTag tag_ : 8;

   //  Set if a using statement made the friend visible.
   //
   bool using_ : 1;

   //  Set when searching for the friend's referent, to prevent recursive
   //  invocations of FindReferent.
   //
   mutable bool searching_ : 1;

   //  Set as the result of the first referent search.
   //
   bool searched_ : 1;

   //  How many times the declaration was used.
   //
   size_t users_ : 16;

   //  Set to prevent FindReferent from nesting too deeply.
   //
   static size_t Depth_;
};

//------------------------------------------------------------------------------
//
//  This is created for a reserved word that can be the referent of a type.
//
class Terminal : public CxxScoped
{
public:
   //  Creates a terminal known by NAME, with a TypeString() of TYPE.  If
   //  TYPE is not supplied, TypeString() is NAME.
   //
   explicit Terminal
      (const std::string& name, const std::string& type = EMPTY_STR);

   //  Not subclassed.
   //
   ~Terminal();

   //  Sets the terminal's integer attributes.
   //
   void SetNumeric(const Numeric& attrs) { attrs_ = attrs; }

   //  Overridden to return true if the terminal is "auto".
   //
   virtual bool IsAuto() const override;

   //  Overridden to set the type for an "auto" variable.
   //
   virtual CxxToken* AutoType() const override { return (CxxToken*) this; }

   //  Overridden to display the terminal.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden to push the terminal onto the stack.
   //
   virtual void EnterBlock() override;

   //  Overridden to indicate that a terminal is always in scope.
   //
   virtual bool EnterScope() override { return true; }

   //  Overridden to indicate that a terminal does not appear in a file.
   //
   virtual id_t GetDeclFid() const override { return NIL_ID; }

   //  Overridden to return the terminal's attributes as an integer.
   //
   virtual Numeric GetNumeric() const override { return attrs_; }

   //  Overridden to return the terminal's name.
   //
   virtual const std::string* Name() const override { return &name_; }

   //  Overridden to shrink containers.
   //
   virtual void Shrink() override;

   //  Overridden to reveal that this is a terminal.
   //
   virtual Cxx::ItemType Type() const override { return Cxx::Terminal; }

   //  Overridden to return the terminal's root type.
   //
   virtual std::string TypeString(bool arg) const override { return type_; }

   //  Overridden to support, for example, writing to a char in a std::string
   //  or passing an int as an argument.
   //
   virtual bool WasWritten(const StackArg* arg, bool passed)
      override { return false; }
private:
   //  The terminal's name.
   //
   std::string name_;

   //  The terminal's type.
   //
   std::string type_;

   //  The terminal's attributes as an integer.
   //
   Numeric attrs_;
};

//------------------------------------------------------------------------------
//
//  A typedef.
//
class Typedef : public CxxScoped
{
public:
   //  Creates a typedef that introduces NAME as an alias for the type
   //  that SPEC describes.
   //
   Typedef(std::string& name, TypeSpecPtr& spec);

   //  Not subclassed.
   //
   ~Typedef();

   //  Overridden to set the type for an "auto" variable.
   //
   virtual CxxToken* AutoType() const override { return (CxxToken*) this; }

   //  Overridden to log warnings associated with the typedef.
   //
   virtual void Check() const override;

   //  Overridden to display the typedef in a function.
   //
   virtual void Print(std::ostream& stream) const override;

   //  Overridden to display the typedef.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden to make the typedef visible as a local.
   //
   virtual void EnterBlock() override;

   //  Overridden to add the typedef to the current scope.
   //
   virtual bool EnterScope() override;

   //  Overridden to remove the typedef as a local.
   //
   virtual void ExitBlock() override;

   //  Overridden to return the underlying type.
   //
   virtual TypeSpec* GetTypeSpec() const override { return spec_.get(); }

   //  Overridden to search the underlying type for template arguments.
   //
   virtual TypeName* GetTemplateArgs() const override;

   //  Overridden to update SYMBOLS with the typedef's type usage.
   //
   virtual void GetUsages
      (const CodeFile& file, CxxUsageSets& symbols) const override;

   //  Overridden to determine if the typedef is unused.
   //
   virtual bool IsUnused() const override { return (refs_ == 0); }

   //  Overridden to return the alias introduced by the typedef.
   //
   virtual const std::string* Name() const override { return &name_; }

   //  Overridden to return the referent of GetTypeSpec().
   //
   virtual CxxNamed* Referent() const override;

   //  Overridden to count references.
   //
   virtual void SetAsReferent(const CxxNamed* user) override { ++refs_; }

   //  Overridden to shrink containers.
   //
   virtual void Shrink() override;

   //  Overridden to reveal that this is a typedef.
   //
   virtual Cxx::ItemType Type() const override { return Cxx::Typedef; }

   //  Overridden to return the typedef's full root type.
   //
   virtual std::string TypeString(bool arg) const override;

   //  Overridden to support a temporary variable represented by a typedef
   //  that was, for example, returned by one function and passed to another.
   //
   virtual bool WasWritten(const StackArg* arg, bool passed)
      override { return false; }
private:
   //  Overridden to return the underlying type.
   //
   virtual CxxToken* RootType() const override { return GetTypeSpec(); }

   //  Checks if the typedef is for a pointer type.
   //
   void CheckPointerType() const;

   //  The name introduced by the typedef.
   //
   std::string name_;

   //  The typedef's underlying type.
   //
   const TypeSpecPtr spec_;

   //  How many times the typedef was used as a type.
   //
   size_t refs_ : 16;
};

//------------------------------------------------------------------------------
//
//  A using declaration or directive.
//
class Using : public CxxScoped
{
public:
   //  NAME is what is being used.  SPACE is set if it is a namespace.
   //  ADDED is set if the statement was added by the >trim command.
   //
   Using(QualNamePtr& name, bool space, bool added = false);

   //  Not subclassed.
   //
   ~Using() { CxxStats::Decr(CxxStats::USING_DECL); }

   //  Returns true if the declaration/directive makes NAME visible to
   //  at least the position specified by PREFIX.
   //
   bool IsUsingFor(const std::string& name, size_t prefix) const;

   //  Used by >trim when the statement should be removed.
   //
   void MarkForRemoval() { remove_ = true; }

   //  Used by >trim when the statement should be retained.
   //
   void MarkForRetention() { remove_ = false; }

   //  Returns true if the >trim command added the statement.
   //
   bool WasAdded() const { return added_; }

   //  Returns true if the >trim command marked the statement for removal.
   //
   bool IsToBeRemoved() const { return remove_; }

   //  Overridden to log warnings associated with the declaration.
   //
   virtual void Check() const override;

   //  Overridden to display the declaration.  If FQ is set, the fully
   //  qualified name is displayed.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden to make the declaration available to the current block.
   //
   virtual void EnterBlock() override;

   //  Overridden to add the declaration to the current scope.
   //
   virtual bool EnterScope() override;

   //  Overridden to make the declaration unavailable.
   //
   virtual void ExitBlock() override;

   //  Overridden to return the declaration's qualified name.
   //
   virtual QualName* GetQualName() const override { return name_.get(); }

   //  Overridden to indicate that inline display is not supported.
   //
   virtual bool InLine() const override { return false; }

   //  Overridden to determine if the declaration is unused.
   //
   virtual bool IsUnused() const override { return (users_ == 0); }

   //  Overridden to return the name of what is being used.
   //
   virtual const std::string* Name() const override { return name_->Name(); }

   //  Overridden to return the qualified name of what is being used.
   //
   virtual std::string QualifiedName(bool scopes, bool templates) const
      override { return name_->QualifiedName(scopes, templates); }

   //  Overridden to return what the declaration refers to.
   //
   virtual CxxNamed* Referent() const override;

   //  Overridden to stop at a typedef.
   //
   virtual bool ResolveTypedef(Typedef* type, size_t n) const
      override { return false; }

   //  Overridden to return the scoped name of what is being used.
   //
   virtual std::string ScopedName(bool templates) const override;

   //  Overridden to set the scope where the declaration appeared.
   //
   virtual void SetScope(CxxScope* scope) const override;

   //  Overridden to shrink the item's name.
   //
   virtual void Shrink() override { name_->Shrink(); }
private:
   //  Overridden to find the item that the declaration refers to.
   //
   virtual bool FindReferent() override;

   //  The declaration's (possibly) qualified name.
   //
   const QualNamePtr name_;

   //  How many times the declaration resolved a symbol.
   //
   mutable size_t users_ : 13;

   //  Set if the declaration was added by >trim.
   //
   bool added_ : 1;

   //  Set if the declaration is to be removed.
   //
   bool remove_ : 1;

   //  Set if name_ is a namespace.
   //
   const bool space_ : 1;
};
}
#endif
