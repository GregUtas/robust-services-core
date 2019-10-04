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
#include "CxxToken.h"
#include "LibraryTypes.h"
#include "SysTypes.h"

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
   virtual bool NameRefersToItem(const std::string& name,
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

   //  Logs an unused item.  The default version generates a log that
   //  contains WARNING if IsUnused returns true.  Returns true if a
   //  log was generated.
   //
   virtual bool CheckIfUnused(Warning warning) const;

   //  Overridden to decrement N if this item's name matches NAME, and to
   //  return this item if N has reached 0.
   //
   CxxScoped* FindNthItem(const std::string& name, size_t& n) const override;

   //  Overridden to return the access control level for the item.
   //
   Cxx::Access GetAccess() const override { return access_; }

   //  Sets BEGIN to GetPos() of the item or, if GetTypeSpec() returns a
   //  valid type specification, then GetPos() of that specification.  Sets
   //  END to the location of the next semicolon and returns string::npos.
   //
   size_t GetRange(size_t& begin, size_t& end) const override;

   //  Overridden to return the scope where the declaration appeared.
   //
   CxxScope* GetScope() const override { return scope_; }

   //  Overridden to indicate that inline display is not supported.
   //
   bool InLine() const override { return false; }

   //  Overridden to invoke IsConst on GetTypeSpec unless the latter
   //  returns nullptr, in which case it returns false.
   //
   bool IsConst() const override;

   //  Overridden to invoke IsConstPtr on GetTypeSpec unless the latter
   //  returns nullptr, in which case it returns false.
   //
   bool IsConstPtr() const override;

   //  Overridden to invoke IsConstPtr on GetTypeSpec unless the latter
   //  returns nullptr, in which case it returns false.
   //
   bool IsConstPtr(size_t n) const override;

   //  Overridden to return true if the item is declared in a code block
   //  or argument list.
   //
   bool IsDeclaredInFunction() const override;

   //  Overridden to invoke IsAuto on GetTypeSpec unless the latter
   //  returns nullptr, in which case it returns false.
   //
   bool IsAuto() const override;

   //  Overridden to invoke IsIndirect on GetTypeSpec unless the latter
   //  returns nullptr, in which case it returns false.
   //
   bool IsIndirect() const override;

   //  Returns true if this scope is either fqSuper or a subscope of it.
   //
   bool IsSubscopeOf(const std::string& fqSuper) const;

   //  Overridden to use this item's fully qualified name to determine
   //  if it is a superscope of fqSub.
   //
   bool IsSuperscopeOf(const std::string& fqSub, bool tmplt) const override;

   //  Overridden to return true if this item matches ITEM and to increment
   //  N if this item's name matches ITEM.
   //
   bool LocateItem(const CxxNamed* item, size_t& n) const override;

   //  Overridden to return the item itself.
   //
   CxxScoped* Referent() const override { return (CxxScoped*) this; }

   //  Overridden to set the access control level for the item.
   //
   void SetAccess(Cxx::Access access) override { access_ = access; }

   //  Overridden to set the scope where the declaration appeared.
   //
   void SetScope(CxxScope* scope) override { scope_ = scope; }
protected:
   //  Protected because this class is virtual.
   //
   CxxScoped();

   //  Logs an item whose name hides a name defined in a base class.
   //
   virtual void CheckIfHiding() const;

   //  Logs an item whose access control could be more restrictive.
   //
   virtual void CheckAccessControl() const;
private:
   //  Records that the item's template analog, if any, required *at least*
   //  ACCESS to be accessible.
   //
   void RecordTemplateAccess(Cxx::Access access) const;

   //  The scope where the item appeared.
   //
   CxxScope* scope_;

   //  The access control level for the item.
   //
   Cxx::Access access_ : 8;

   //  Set if the item required public access.
   //
   mutable bool public_;

   //  Set if the item required protected access.
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
   void SetDefault(ExprPtr& preset) { default_.reset(preset.release()); }

   //  Returns true if the argument has a default value.
   //
   bool HasDefault() const { return (default_ != nullptr); }

   //  Returns true if the argument could be const.
   //
   bool CouldBeConst() const { return !nonconst_; }

   //  If the argument is a non-const reference *or* pointer to a class
   //  that is not external, returns that class, else returns nullptr.
   //
   Class* IsThisCandidate() const;

   //  Overridden to set the type for an "auto" variable.
   //
   CxxToken* AutoType() const override { return spec_.get(); }

   //  Overridden to log warnings associated with the argument.
   //
   void Check() const override;

   //  Overridden to make the argument visible within its function.
   //
   void EnterBlock() override;

   //  Overridden to remove the argument as a local.
   //
   void ExitBlock() override;

   //  Overridden to invoke EnterBlock on the argument's type (spec_) and
   //  any default value (default_).
   //
   bool EnterScope() override;

   //  Overridden to return the argument's underlying numeric type.
   //
   Numeric GetNumeric() const override { return spec_->GetNumeric(); }

   //  Overridden to return the argument's type.
   //
   TypeSpec* GetTypeSpec() const override { return spec_.get(); }

   //  Overridden to update SYMBOLS with the argument's type usage.
   //
   void GetUsages(const CodeFile& file, CxxUsageSets& symbols) const override;

   //  Overridden to indicate that inline display is supported.
   //
   bool InLine() const override { return true; }

   //  Overridden to return false for a "this" argument.
   //
   bool IsStatic() const override { return (name_ != THIS_STR); }

   //  Overridden to determine if the argument is unused.
   //
   bool IsUnused() const override { return ((reads_ + writes_) == 0); }

   //  Overridden to return the argument's name, if any.
   //
   const std::string* Name() const override { return &name_; }

   //  Overridden to display the argument.
   //
   void Print
      (std::ostream& stream, const NodeBase::Flags& options) const override;

   //  Overridden to record that the argument cannot be const.
   //
   bool SetNonConst() override;

   //  Overridden to shrink containers.
   //
   void Shrink() override;

   //  Overridden to reveal that this is an argument.
   //
   Cxx::ItemType Type() const override { return Cxx::Argument; }

   //  Overridden to return the argument's full root type.
   //
   std::string TypeString(bool arg) const override;

   //  Overridden to increment the number of reads.
   //
   bool WasRead() override;

   //  Overridden to increment the number of writes.
   //
   bool WasWritten(const StackArg* arg, bool passed) override;
private:
   //  Overridden to return the argument's type.
   //
   CxxToken* RootType() const override { return spec_.get(); }

   //  Checks for a "(void)" argument.
   //
   void CheckVoid() const;

   //  Logs WARNING on the argument's function.
   //
   void LogToFunc(Warning warning) const;

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
   size_t reads_ : 15;

   //  How many times the argument was written.
   //
   size_t writes_ : 15;

   //  Set if the argument cannot be const.
   //
   bool nonconst_ : 1;

   //  Set if the argument's value was directly modified.
   //
   bool modified_: 1;
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
   bool EnterScope() override;

   //  Overridden to return the class that the declaration refers to.
   //
   Class* GetClass() const override;

   //  Overridden to return the base class's qualified name.
   //
   QualName* GetQualName() const override { return name_.get(); }

   //  Overridden to update SYMBOLS with the declaration's type usage.
   //
   void GetUsages(const CodeFile& file, CxxUsageSets& symbols) const override;

   //  Overridden to indicate that inline display is supported.
   //
   bool InLine() const override { return true; }

   //  Overridden to return the base class's name.
   //
   const std::string* Name() const override { return name_->Name(); }

   //  Overridden to return the base class's qualified name.
   //
   std::string QualifiedName(bool scopes, bool templates) const
      override { return name_->QualifiedName(scopes, templates); }

   //  Overridden to return the base class.
   //
   CxxScoped* Referent() const override;

   //  Overridden to return the base class's scoped name.
   //
   std::string ScopedName(bool templates) const override;

   //  Overridden to preserve the access control set by the constructor.
   //
   void SetAccess(Cxx::Access access) override;

   //  Overridden to shrink containers.
   //
   void Shrink() override{ name_->Shrink(); }

   //  Overridden to return the declaration's full root type.
   //
   std::string TypeString(bool arg) const override;
private:
   //  Overridden to find the base class's class.
   //
   void FindReferent() override;

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

   //  Adds an underlying type for the enumeration.
   //
   void AddType(TypeSpecPtr& type);

   //  Adds an enumerator, with NAME and INIT, to the enumeration.  POS is
   //  its location within the source code file.
   //
   void AddEnumerator(std::string& name, ExprPtr& init, size_t pos);

   //  Returns the enumerator defined by NAME.
   //
   Enumerator* FindEnumerator(const std::string& name) const;

   //  Returns all of the enumerators.
   //
   const EnumeratorPtrVector& Etors() const { return etors_; }

   //  Overridden to set the type for an "auto" variable.
   //
   CxxToken* AutoType() const override { return (CxxToken*) this; }

   //  Overridden to log warnings associated with the enumeration.
   //
   void Check() const override;

   //  Overridden to check each enumerator before suggesting a more
   //  restrictive access control.
   //
   void CheckAccessControl() const override;

   //  Overridden to display the enumeration.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;

   //  Overridden to make the enumeration visible as a local.
   //
   void EnterBlock() override;

   //  Overridden to add the enumeration to the current scope.
   //
   bool EnterScope() override;

   //  Overridden to remove the enumeration from the global symbol table (its
   //  constructor puts it there, and this removes it as soon as it goes out
   //  of scope).
   //
   void ExitBlock() override;

   //  Overridden to indicate that an enum can be converted to an integer.
   //
   Numeric GetNumeric() const override { return Numeric::Enum; }

   //  Overridden to return the enumeration's type.  Returns DataSpec::Int if
   //  the underlying type was not specified.
   //
   TypeSpec* GetTypeSpec() const override;

   //  Overridden to determine if the enum and all its enumerators are unused.
   //
   bool IsUnused() const override;

   //  Overridden to return the enumeration's name.
   //
   const std::string* Name() const override { return &name_; }

   //  Overridden to record usage of the enumeration.
   //
   void RecordUsage() const override { AddUsage(); }

   //  Overridden to count references.
   //
   void SetAsReferent(const CxxNamed* user) override;

   //  Overridden to shrink containers.
   //
   void Shrink() override;

   //  Overridden to reveal that this is an enumeration.
   //
   Cxx::ItemType Type() const override { return Cxx::Enum; }

   //  Overridden to return an enumeration's fully qualified name.
   //
   std::string TypeString(bool arg) const override;

   //  Overridden to support, for example, writing to an enum in a std::vector
   //  or passing an enum as an argument.
   //
   bool WasWritten(const StackArg* arg, bool passed)
      override { return false; }
private:
   //  The enumeration's name.
   //
   std::string name_;

   //  The enum's underlying type, if specified.
   //
   TypeSpecPtr spec_;

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
   CxxToken* AutoType() const override { return (CxxToken*) enum_; }

   //  Overridden to log warnings associated with the enumerator.
   //
   void Check() const override;

   //  Overridden to display the enumeration.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;

   //  Overridden to execute the enumerator's initialization statement.
   //
   void EnterBlock() override;

   //  Overridden to execute the enumerator's initialization statement.
   //
   bool EnterScope() override;

   //  Overridden to remove the enumerator from the global symbol table (its
   //  constructor puts it there, and this removes it as soon as it goes out
   //  of scope).
   //
   void ExitBlock() override;

   //  Overridden to indicate that an enum can be converted to an integer.
   //
   Numeric GetNumeric() const override { return Numeric::Enum; }

   //  Overridden to enable promotion of the enumerator to its enum's scope
   //  (that is, <scope>::enumerator as well as <scope>::enum::enumerator).
   //
   void GetScopedNames(stringVector& names, bool templates) const override;

   //  Overridden to determine if the enumerator is unused.
   //
   bool IsUnused() const override { return (refs_ == 0); }

   //  Overridden to return the enumerator's name.
   //
   const std::string* Name() const override { return &name_; }

   //  Overridden to prefix the enum as a scope.
   //
   std::string ScopedName(bool templates) const override;

   //  Overridden to note that the enumeration required ACCESS.
   //
   void RecordAccess(Cxx::Access access) const override;

   //  Overridden to record usage of the enumerator.
   //
   void RecordUsage() const override { AddUsage(); }

   //  Overridden to count references.
   //
   void SetAsReferent(const CxxNamed* user) override;

   //  Overridden to shrink containers.
   //
   void Shrink() override;

   //  Overridden to reveal that this is an enumerator.
   //
   Cxx::ItemType Type() const override { return Cxx::Enumerator; }

   //  Overridden to return the enumeration's type.
   //
   std::string TypeString(bool arg) const override;

   //  Overridden to count references to the enumerator.
   //
   bool WasRead() override;
private:
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
   CxxToken* AutoType() const override;

   //  Overridden to log warnings associated with the declaration.
   //
   void Check() const override;

   //  Overridden to display the declaration.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;

   //  Overridden to push the declaration's referent onto the argument stack.
   //
   void EnterBlock() override;

   //  Overridden to add the declaration to the current scope.
   //
   bool EnterScope() override;

   //  Overriden to add the forward's referent to SYMBOLS.
   //
   void GetDirectClasses(CxxUsageSets& symbols) const override;

   //  Overridden to return the class's qualified name.
   //
   QualName* GetQualName() const override { return name_.get(); }

   //  Overriden to support the forward declaration of templates.
   //
   const TemplateParms* GetTemplateParms() const
      override { return parms_.get(); }

   //  Overridden to determine if the declaration is unused.
   //
   bool IsUnused() const override { return (users_ == 0); }

   //  Overridden to returns the class's name.
   //
   const std::string* Name() const override { return name_->Name(); }

   //  Overridden to return the class.
   //
   CxxScoped* Referent() const override;

   //  Overridden to return the class's scoped name.
   //
   std::string ScopedName(bool templates) const override;

   //  Overridden to count usages.
   //
   void SetAsReferent(const CxxNamed* user) override;

   //  Overriden to support the forward declaration of templates.
   //
   void SetTemplateParms(TemplateParmsPtr& parms) override;

   //  Overridden to shrink containers.
   //
   void Shrink() override;

   //  Overridden to reveal that this is a forward declaration.
   //
   Cxx::ItemType Type() const override { return Cxx::Forward; }

   //  Overridden to return the class's full type.
   //
   std::string TypeString(bool arg) const override;
private:
   //  Overridden to return the class.
   //
   CxxToken* RootType() const override { return Referent(); }

   //  The class's type.
   //
   const Cxx::ClassTag tag_ : 8;

   //  The class's name.
   //
   const QualNamePtr name_;

   //  The template parameters if the class declares a template.
   //
   TemplateParmsPtr parms_;

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
   void IncrUsers();

   //  Overridden to return the referent if known, else the friend declaration.
   //
   CxxToken* AutoType() const override;

   //  Overridden to log warnings associated with the declaration.
   //
   void Check() const override;

   //  Overridden to display the declaration.  If FQ is set, the friend's
   //  fully qualified name is displayed.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;

   //  Overridden to push the declaration's referent onto the argument stack.
   //
   void EnterBlock() override;

   //  Overridden to add the declaration to the current scope.
   //
   bool EnterScope() override;

   //  Overriden to add the friend's referent to SYMBOLS.
   //
   void GetDirectClasses(CxxUsageSets& symbols) const override;

   //  Overridden to return the friend if it is a function.
   //
   Function* GetFunction() const override;

   //  Overridden to return the declaration's qualified name.
   //
   QualName* GetQualName() const override;

   //  Overriden to support templates as friends.
   //
   const TemplateParms* GetTemplateParms() const
      override { return parms_.get(); }

   //  Overridden to update SYMBOLS with the declaration's type usage.
   //
   void GetUsages(const CodeFile& file, CxxUsageSets& symbols) const override;

   //  Overridden to indicate that inline display is not supported.
   //
   bool InLine() const override { return false; }

   //  Overridden to determine if the declaration is unused.
   //
   bool IsUnused() const override { return (users_ == 0); }

   //  Overridden to return the friend's name.
   //
   const std::string* Name() const override;

   //  Overridden to return the friend's qualified name.
   //
   std::string QualifiedName(bool scopes, bool templates) const override;

   //  Overridden to return the friend.
   //
   CxxScoped* Referent() const override;

   //  Overridden to apply the arguments after updating the scope to that
   //  of the class template.
   //
   bool ResolveTemplate
      (Class* cls, const TypeName* args, bool end) const override;

   //  Overridden to return the friend's scoped name.
   //
   std::string ScopedName(bool templates) const override;

   //  Overridden to log a warning.  A forward declaration, not a friend
   //  declaration, should be used to resolve an indirect type.
   //
   void SetAsReferent(const CxxNamed* user) override;

   //  Overriden to support templates as friends.
   //
   void SetTemplateParms(TemplateParmsPtr& parms) override;

   //  Overridden to shrink containers.
   //
   void Shrink() override;

   //  Overridden to reveal that this is a friend declaration.
   //
   Cxx::ItemType Type() const override { return Cxx::Friend; }

   //  Overridden to return the friend's full type.
   //
   std::string TypeString(bool arg) const override;
private:
   //  Overridden to find the item that the declaration refers to.
   //
   void FindReferent() override;

   //  Overridden to record DECL and update the friend's scope.
   //
   bool ResolveForward(CxxScoped* decl, size_t n) const override;

   //  Overridden to return the friend.
   //
   CxxToken* RootType() const override { return Referent(); }

   //  Overridden to record what the item refers to.
   //
   void SetReferent(CxxScoped* item, const SymbolView* view) const override;

   //  Finds the item that the declaration refers to when it was not
   //  visible from the scope where the declaration appeared.
   //
   CxxScoped* FindForward() const;

   //  Returns the referent.
   //
   CxxScoped* GetReferent() const;

   //  The friend's qualified name.
   //
   QualNamePtr name_;

   //  The template parameters if the friend is a template.
   //
   TemplateParmsPtr parms_;

   //  If the friend is a non-inlined function, its specification.
   //
   FunctionPtr func_;

   //  If the friend is an inlined function, a pointer to it.
   //
   Function* inline_;

   //  The class that granted friendship.
   //
   const CxxScope* grantor_;

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

   //  Used to prevent FindReferent from nesting too deeply.
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
      (const std::string& name, const std::string& type = NodeBase::EMPTY_STR);

   //  Not subclassed.
   //
   ~Terminal();

   //  Sets the terminal's integer attributes.
   //
   void SetNumeric(const Numeric& attrs) { attrs_ = attrs; }

   //  Overridden to return true if the terminal is "auto".
   //
   bool IsAuto() const override;

   //  Overridden to set the type for an "auto" variable.
   //
   CxxToken* AutoType() const override { return (CxxToken*) this; }

   //  Overridden to display the terminal.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;

   //  Overridden to push the terminal onto the stack.
   //
   void EnterBlock() override;

   //  Overridden to indicate that a terminal is always in scope.
   //
   bool EnterScope() override { return true; }

   //  Overridden to indicate that a terminal does not appear in a file.
   //
   NodeBase::id_t GetDeclFid() const override
      { return NodeBase::NIL_ID; }

   //  Overridden to return the terminal's attributes as an integer.
   //
   Numeric GetNumeric() const override { return attrs_; }

   //  Overridden to return the terminal's name.
   //
   const std::string* Name() const override { return &name_; }

   //  Overridden for when NAME refers to a terminal.
   //
   bool NameRefersToItem(const std::string& name, const CxxScope* scope,
      const CodeFile* file, SymbolView* view) const override;

   //  Overridden to shrink containers.
   //
   void Shrink() override;

   //  Overridden to reveal that this is a terminal.
   //
   Cxx::ItemType Type() const override { return Cxx::Terminal; }

   //  Overridden to return the terminal's root type.
   //
   std::string TypeString(bool arg) const override { return type_; }

   //  Overridden to support, for example, writing to a char in a std::string
   //  or passing an int as an argument.
   //
   bool WasWritten(const StackArg* arg, bool passed)
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

   //  Invoked for a type alias, which uses the keyword "using".
   //
   void SetUsing() { using_ = true; }

   //  Overridden to set the type for an "auto" variable.
   //
   CxxToken* AutoType() const override { return (CxxToken*) this; }

   //  Overridden to log warnings associated with the typedef.
   //
   void Check() const override;

   //  Overridden to display the typedef in a function.
   //
   void Print
      (std::ostream& stream, const NodeBase::Flags& options) const override;

   //  Overridden to display the typedef.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;

   //  Overridden to make the typedef visible as a local.
   //
   void EnterBlock() override;

   //  Overridden to add the typedef to the current scope.
   //
   bool EnterScope() override;

   //  Overridden to remove the typedef as a local.
   //
   void ExitBlock() override;

   //  Overridden to return the definition's underlying numeric type.
   //
   Numeric GetNumeric() const override { return spec_->GetNumeric(); }

   //  Overridden to return the underlying type.
   //
   TypeSpec* GetTypeSpec() const override { return spec_.get(); }

   //  Overridden to search the underlying type for template arguments.
   //
   TypeName* GetTemplateArgs() const override;

   //  Overridden to update SYMBOLS with the typedef's type usage.
   //
   void GetUsages(const CodeFile& file, CxxUsageSets& symbols) const override;

   //  Overridden to determine if the typedef is unused.
   //
   bool IsUnused() const override { return (refs_ == 0); }

   //  Overridden to return the alias introduced by the typedef.
   //
   const std::string* Name() const override { return &name_; }

   //  Overridden to return the referent of GetTypeSpec().
   //
   CxxScoped* Referent() const override;

   //  Overridden to count references.
   //
   void SetAsReferent(const CxxNamed* user) override;

   //  Overridden to shrink containers.
   //
   void Shrink() override;

   //  Overridden to reveal that this is a typedef.
   //
   Cxx::ItemType Type() const override { return Cxx::Typedef; }

   //  Overridden to return the typedef's full root type.
   //
   std::string TypeString(bool arg) const override;

   //  Overridden to support a temporary variable represented by a typedef
   //  that was, for example, returned by one function and passed to another.
   //
   bool WasWritten(const StackArg* arg, bool passed)
      override { return false; }
private:
   //  Overridden to return the underlying type.
   //
   CxxToken* RootType() const override { return spec_.get(); }

   //  Checks if the typedef is for a pointer type.
   //
   void CheckPointerType() const;

   //  The name introduced by the typedef.
   //
   std::string name_;

   //  The typedef's underlying type.
   //
   const TypeSpecPtr spec_;

   //  Set if this is actually a type alias (using <name> = <TypeSpec>).
   //
   bool using_ : 1;

   //  How many times the typedef was used as a type.
   //
   size_t refs_ : 15;
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

   //  Returns true if the declaration/directive makes fqName visible
   //  within SCOPE to at least the position specified by PREFIX.
   //
   bool IsUsingFor
      (const std::string& fqName, size_t prefix, const CxxScope* scope) const;

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
   void Check() const override;

   //  Overridden to display the declaration.  If FQ is set, the fully
   //  qualified name is displayed.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;

   //  Overridden to make the declaration available to the current block.
   //
   void EnterBlock() override;

   //  Overridden to add the declaration to the current scope.
   //
   bool EnterScope() override;

   //  Overridden to make the declaration unavailable.
   //
   void ExitBlock() override;

   //  Overridden to return the declaration's qualified name.
   //
   QualName* GetQualName() const override { return name_.get(); }

   //  Overridden to indicate that inline display is not supported.
   //
   bool InLine() const override { return false; }

   //  Overridden to determine if the declaration is unused.
   //
   bool IsUnused() const override { return (users_ == 0); }

   //  Overridden to return the name of what is being used.
   //
   const std::string* Name() const override { return name_->Name(); }

   //  Overridden to return the qualified name of what is being used.
   //
   std::string QualifiedName(bool scopes, bool templates) const
      override { return name_->QualifiedName(scopes, templates); }

   //  Overridden to return what the declaration refers to.
   //
   CxxScoped* Referent() const override;

   //  Overridden to stop at a typedef.
   //
   bool ResolveTypedef(Typedef* type, size_t n) const
      override { return false; }

   //  Overridden to return the scoped name of what is being used.
   //
   std::string ScopedName(bool templates) const override;

   //  Overridden to adjust the scope for a using statement within a class.
   //
   void SetScope(CxxScope* scope) override;

   //  Overridden to shrink the item's name.
   //
   void Shrink() override { name_->Shrink(); }
private:
   //  Overridden to find the item that the declaration refers to.
   //
   void FindReferent() override;

   //  The declaration's (possibly) qualified name.
   //
   const QualNamePtr name_;

   //  How many times the declaration resolved a symbol.
   //
   mutable size_t users_ : 13;

   //  Set if the declaration was added by >trim.
   //
   const bool added_ : 1;

   //  Set if the declaration is to be removed.
   //
   bool remove_ : 1;

   //  Set if name_ is a namespace.
   //
   const bool space_ : 1;
};
}
#endif
