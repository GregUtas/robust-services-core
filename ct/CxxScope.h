//==============================================================================
//
//  CxxScope.h
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
#ifndef CXXSCOPE_H_INCLUDED
#define CXXSCOPE_H_INCLUDED

#include "CxxNamed.h"
#include "CxxScoped.h"
#include <cstddef>
#include <cstdint>
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
//  A scope (a namespace, class, function, or local block).  The scope in
//  which a name is defined affects its accessibility.
//
class CxxScope : public CxxScoped
{
public:
   //  Virtual to allow subclassing.
   //
   virtual ~CxxScope();

   //  Closes the scope(s) associated with an item.  Invoked when the item
   //  has been fully parsed.
   //
   void CloseScope();

   //  Returns the distance to the superscope SCOPE.  Returns 0 if SCOPE is
   //  the same as this scope.  Returns NOT_A_SUBSCOPE if this scope is not
   //  a subscope of SCOPE.
   //
   Distance ScopeDistance(const CxxScope* scope) const;

   //  Returns the template parameter that corresponds to NAME.  For example,
   //  returns the template parameter for "T" when "T" appears somewhere in
   //    template< typename T > class <name> { ... };
   //
   TemplateParm* NameToTemplateParm(const std::string& name) const;

   //  Returns the file that declares the item if it is *not* the file that
   //  defines the item.  This can only occur for static data or a function.
   //  Returns nullptr if the same file declares and defines the item.
   //
   CodeFile* GetDistinctDeclFile() const;

   //  Returns the current access control level when parsing within the scope.
   //
   virtual Cxx::Access GetCurrAccess() const { return Cxx::Private; }

   //  Returns the using statement, if any, that makes ITEM visible within
   //  SCOPE because it matches fqName to at least PREFIX.
   //
   virtual Using* GetUsingFor(const std::string& fqName, size_t prefix,
      const CxxNamed* item, const CxxScope* scope) const { return nullptr; }

   //  Updates VIEW to indicate the accessibility of ITEM, which was declared
   //  in this scope, to SCOPE.  The default version, for use by functions
   //  and code blocks, returns Unrestricted if SCOPE either matches or is
   //  a subscope of this scope, and Inaccessible otherwise.
   //
   virtual void AccessibilityOf
      (const CxxScope* scope, const CxxScoped* item, SymbolView& view) const;
protected:
   //  Protected because this class is virtual.
   //
   CxxScope();

   //  Updates the scope when the parse recognizes a data or function item.
   //  NAME is the item's name as it was parsed, which may or may not be a
   //  qualified name.
   //
   void OpenScope(std::string& name);

   //  Replaces this class's or function's template parameters, as they
   //  appear in CODE, with the template arguments in ARGS, starting at
   //  BEGIN.
   //
   void ReplaceTemplateParms
      (std::string& code, const TypeSpecPtrVector* args, size_t begin) const;
private:
   //  The number of nested calls to Context::PushScope.
   //
   uint8_t pushes_ : 8;
};

//------------------------------------------------------------------------------
//
//  A code block.
//  o Braces are mandatory if the block is empty or has multiple statements.
//  o Braces are optional if the block has a single statement.  NoOp (a bare
//    semicolon) is a single statement.
//
class Block : public CxxScope
{
public:
   //  Creates a block.  BRACED is set if a left brace was found.
   //
   explicit Block(bool braced);

   //  Not subclassed.
   //
   ~Block();

   //  Adds a statement to the block.
   //
   bool AddStatement(CxxToken* s);

   //  The type of block and how to display it.
   //
   enum Form
   {
      Empty,     // no statements
      Unbraced,  // one statement, no braces
      Braced,    // one statement, braces
      Multiple   // two statements or more
   };

   //  Calculates the setting of CRLF for the Display function.  A null
   //  statement (bare semicolon) is always displayed in-line, and multiple
   //  statements are always displayed on separate lines.  FORM therefore
   //  affects the display of an empty (braced) or non-compound statement:
   //  o Empty: displayed on a new line
   //  o Unbraced: displayed in-line if braced; on a new line if unbraced
   //  o Braced: displayed in-line
   //  Regardless of what FORM is specified, the statement is displayed on
   //  a new line if its Inline function so requests.
   //
   bool CrlfOver(Form form) const;

   //  Returns true if the block uses braces.
   //
   bool IsBraced() const { return braced_; }

   //  Specifies that the block was nested.
   //
   void SetNested() { nested_ = true; }

   //  Returns the first statement in the block.
   //
   CxxToken* FirstStatement() const;

   //  Adds USE to the statements that are local to the block which is
   //  being compiled.
   //
   static void AddUsing(Using* use);

   //  Removes USE from the statements that are local to the block which
   //  is being compiled.
   //
   static void RemoveUsing(const Using* use);

   //  Clears any using statements that are local to code blocks.
   //
   static void ResetUsings();

   //  Removes ITEM from the block's statements when ITEM is being deleted.
   //
   void EraseItem(const CxxToken* item);

   //  Replaces CURR with NEXT in the the block's statements when CURR is
   //  being deleted.
   //
   void ReplaceItem(const CxxToken* curr, CxxToken* next);

   //  Overridden to add the block's components to cross-references.
   //
   void AddToXref(bool insert) override;

   //  Overridden to log warnings within the code.
   //
   void Check() const override;

   //  Overridden to display the block.  If CRLF is set, a block that contains
   //  one statement is displayed on a new line, else it is displayed in-line.
   //  A space or endline is inserted first, as appropriate, and an endline is
   //  always inserted afterwards.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;

   //  Overridden to invoke EnterBlock on each token in statements_, followed
   //  by ExitBlock after all the statements have been compiled.
   //
   void EnterBlock() override;

   //  Overridden to search the block's code for an item that matches NAME.
   //
   CxxScoped* FindNthItem(const std::string& name, size_t& n) const override;

   //  Overridden to return the function in which the block appears.
   //
   Function* GetFunction() const override;

   //  Overridden to update SYMBOLS with each statement's type usage.
   //
   void GetUsages(const CodeFile& file, CxxUsageSets& symbols) override;

   //  Overridden to look at using statements that are local to a function.
   //
   Using* GetUsingFor(const std::string& fqName, size_t prefix,
      const CxxNamed* item, const CxxScope* scope) const override;

   //  Overridden to determine if in-line display is possible.
   //
   bool InLine() const override;

   //  Overridden to search the block's code for ITEM.
   //
   bool LocateItem(const CxxToken* item, size_t& n) const override;

   //  Overridden to return the block's name.
   //
   const std::string& Name() const override { return name_; }

   //  Overridden to find the item located at POS.
   //
   CxxToken* PosToItem(size_t pos) const override;

   //  Overridden to display a block in-line if it has one statement or none.
   //
   void Print
      (std::ostream& stream, const NodeBase::Flags& options) const override;

   //  Overridden to return the enclosing function's scoped name, followed by
   //  a string that signifies executable code rather than only the function.
   //
   std::string ScopedName(bool templates) const override;

   //  Overridden to shrink containers.
   //
   void Shrink() override;

   //  Overridden to reveal that this is a code block.
   //
   Cxx::ItemType Type() const override { return Cxx::Block; }

   //  Overridden to update the location of block's statements.
   //
   void UpdatePos(EditorAction action,
      size_t begin, size_t count, size_t from) const override;
private:
   //  The statements in the block.
   //
   TokenPtrVector statements_;

   //  The block's name.
   //
   std::string name_;

   //  Set if braces were used when there were zero or one statements.
   //
   const bool braced_;

   //  Set if the block suddenly appeared within another one.
   //
   bool nested_;

   //  The using statements visible within the block being compiled.
   //
   static UsingVector Usings_;
};

//------------------------------------------------------------------------------
//
//  Data.
//
class Data : public CxxScope
{
public:
   //  Virtual to allow subclassing.
   //
   virtual ~Data();

   //  Sets the data's alignment.
   //
   void SetAlignment(AlignAsPtr& align);

   //  Specifies whether the data is extern.
   //
   void SetExtern(bool extn) { extern_ = extn; }

   //  Specifies whether the data is static.
   //
   void SetStatic(bool stat) { static_ = stat; }

   //  Specifies whether the data is per-thread.
   //
   void SetThreadLocal(bool local) { thread_local_ = local; }

   //  Specifies whether the data is initialized with a constexpr.
   //
   void SetConstexpr(bool cexpr) { constexpr_ = cexpr; }

   //  Sets the constructor arguments.
   //
   void SetExpression(TokenPtr& expr) { expr_.reset(expr.release()); }

   //  Sets the right-hand side of the assignment statement that
   //  initializes the data.  EQPOS is the location of the assignment
   //  operator.
   //
   void SetAssignment(ExprPtr& expr, size_t eqpos);

   //  Returns true if the data is extern.
   //
   bool IsExtern() const { return extern_; }

   //  Returns true if the data is per-thread.
   //
   bool IsThreadLocal() const { return thread_local_; }

   //  Returns true if the data is initialized with a constexpr.
   //
   bool IsConstexpr() const { return constexpr_; }

   //  Returns true if the data was initialized.
   //
   bool WasInited() const { return GetDecl()->inited_; }

   //  Invoked on a data declaration when its definition (DATA) is found.
   //
   void SetDefn(Data* data);

   //  Returns true if this is a data declaration.
   //
   bool IsDecl() const { return !defn_; }

   //  Returns the data's declaration.
   //
   const Data* GetDecl() const { return (defn_ ? mate_ : this); }

   //  Returns the data's definition (if distinct from its declaration),
   //  else its declaration.
   //
   const Data* GetDefn() const;

   //  Returns true if the data is default constructible.  This function's
   //  purpose is to determine if the data will be initialized if omitted
   //  from a constructor initialization list.  This is only the case for
   //  a class that has a default (zero-argument) constructor.
   //
   bool IsDefaultConstructible() const;

   //  If the data is a string literal, updates STR to its value (minus the
   //  quotes) and returns true.  Returns false if the data is not a string
   //  literal.
   //
   bool GetStrValue(std::string& str) const;

   //  Invoked when promoting a member of an anonymous union to CLS, its
   //  outer scope.  ACCESS is the union's access control.  FIRST and LAST
   //  are set if the member is the first and/or last member of the union.
   //
   virtual void Promote
      (Class* cls, Cxx::Access access, bool first, bool last) { }

   //  Returns true if the member appears in a union.
   //
   virtual bool IsUnionMember() const { return false; }

   //  Overridden to add the data's components to cross-references.
   //
   void AddToXref(bool insert) override;

   //  Overridden to set the type for an "auto" variable.
   //
   CxxToken* AutoType() const override { return (CxxToken*) this; }

   //  Overridden to log warnings associated with the data's type.
   //
   void Check() const override;

   //  Overridden to return the file that declared the data.
   //
   CodeFile* GetDeclFile() const override;

   //  Overridden to return the file (if any) that defined (initialized) the
   //  data.
   //
   CodeFile* GetDefnFile() const override;

   //  Overridden to return the definition if it is distinct from the
   //  declaration, and vice versa.
   //
   CxxNamed* GetMate() const override { return mate_; }

   //  Overridden to return the data's underlying numeric type.
   //
   Numeric GetNumeric() const override { return spec_->GetNumeric(); }

   //  Overridden to return the data's type.
   //
   TypeSpec* GetTypeSpec() const override { return spec_.get(); }

   //  Overridden to search the data's type for template arguments.
   //
   TypeName* GetTemplateArgs() const override;

   //  Overridden to update SYMBOLS with the data's type usage.
   //
   void GetUsages(const CodeFile& file, CxxUsageSets& symbols) override;

   //  Overridden to indicate whether the data is const.
   //
   bool IsConst() const override;

   //  Overridden to use the data's type to determine if it is POD.
   //
   bool IsPOD() const override { return spec_->IsPOD(); }

   //  Returns true if the data's initialization is currently being compiled.
   //
   bool IsInitializing() const override { return initing_; }

   //  Overridden to return true if the data is static.
   //
   bool IsStatic() const override { return static_; }

   //  Overridden to determine if the data is unused.
   //
   bool IsUnused() const override { return ((reads_ == 0) && (writes_ == 0)); }

   size_t Readers() const { return reads_; }

   //  Overridden to indicate whether the data is volatile.
   //
   bool IsVolatile() const override { return spec_->IsVolatile(); }

   //  Overridden to make const data writeable during initialization.
   //
   StackArg NameToArg(Cxx::Operator op, TypeName* name) override;

   //  Overridden to find the item located at POS.
   //
   CxxToken* PosToItem(size_t pos) const override;

   //  Overridden to record that the data cannot be const.
   //
   bool SetNonConst() override;

   //  Overridden to shrink containers.
   //
   void Shrink() override;

   //  Overridden to reveal that this is a data item.
   //
   Cxx::ItemType Type() const override { return Cxx::Data; }

   //  Overridden to return the data's full root type.
   //
   std::string TypeString(bool arg) const override;

   //  Overridden to update the data's location.
   //
   void UpdatePos(EditorAction action,
      size_t begin, size_t count, size_t from) const override;

   //  Overridden to increment the number of times the data was read.
   //
   bool WasRead() override;

   //  Overridden to increment the number of times the data was written.
   //
   bool WasWritten(const StackArg* arg, bool direct, bool indirect) override;
protected:
   //  Creates a data item with SPEC.  Protected because this class is virtual.
   //
   explicit Data(TypeSpecPtr& spec);

   //  Increments the number of writes to the data.
   //
   void IncrWrites() { ++writes_; }

   //  Compiles any alignment directive for the data.
   //
   void ExecuteAlignment() const;

   //  Compiles the data's initialization expression.  Invokes PushScope if
   //  PUSH is set.  Returns false if no form of initialization occurred.
   //
   bool ExecuteInit(bool push);

   //  Compiles the initialization expression EXPR, which appeared in
   //  parentheses.  If the data's type is a class, EXPR is handled as
   //  a constructor call, else it is handled as a single-member brace
   //  initialization.  Returns true unless EXPR is nullptr.
   //
   bool InitByExpr(CxxToken* expr);

   //  Looks for a default constructor (i.e. one without arguments) to
   //  initialize the data.  Returns false if the class has POD members
   //  but no such destructor.
   //
   bool InitByDefault();

   //  Checks if the data is unused, init-only, or write-only.
   //
   void CheckUsage() const;

   //  Checks if the data could (or cannot, which indicates a compiler
   //  logic error) be const.  If COULD is not set, only "cannot" errors
   //  are logged.
   //
   void CheckConstness(bool could) const;

   //  Clears the mate's reference to this data.
   //
   void ClearMate() const;

   //  Displays any alignment directive for the data.
   //
   void DisplayAlignment
      (std::ostream& stream, const NodeBase::Flags& options) const;

   //  Displays any parenthesized expression that initializes the data.
   //
   void DisplayExpression
      (std::ostream& stream, const NodeBase::Flags& options) const;

   //  Displays any assignment statement that initializes the data.
   //
   void DisplayAssignment
      (std::ostream& stream, const NodeBase::Flags& options) const;

   //  Displays read/write statistics.
   //
   void DisplayStats
      (std::ostream& stream, const NodeBase::Flags& options) const;
private:
   //  Returns the data's declaration.
   //
   Data* GetDecl() { return (defn_ ? mate_ : this); }

   //  Returns the name to be used in the initialization statement.
   //
   virtual void GetInitName(QualNamePtr& qualName) const;

   //  Overridden to return the data's type.
   //
   CxxToken* RootType() const override { return spec_.get(); }

   //  Compiles the assignment statement that initializes the data.
   //  Returns true if such a statement existed.
   //
   bool InitByAssign();

   //  Marks the data as initialized.
   //
   void SetInited();

   //  Overridden to return the declaration and/or definition.
   //
   bool GetSpan(size_t& begin, size_t& left, size_t& end) const override;

   //  Set for extern data.
   //
   bool extern_ : 1;

   //  Set for static data.
   //
   bool static_ : 1;

   //  Set for per-thread data.
   //
   bool thread_local_ : 1;

   //  Set for data initialized with a constexpr.
   //
   bool constexpr_ : 1;

   //  Set if the data was initialized.
   //
   bool inited_ : 1;

   //  Set when the data is being initialized.
   //
   bool initing_ : 1;

   //  Set if the data cannot be const.
   //
   bool nonconst_ : 1;

   //  Set if the data cannot be a const pointer.
   //
   bool nonconstptr_ : 1;

   //  Set if this is the definition of previously declared data.
   //
   bool defn_ : 1;

   //  If defn_ is set, the data's declaration.  If defn_ is not set,
   //  the data's definition (if distinct from its declaration).
   //
   Data* mate_;

   //  The data's alignment.
   //
   AlignAsPtr alignas_;

   //  The data's type.
   //
   const TypeSpecPtr spec_;

   //  The expression that initializes the data when
   //  the form <TypeSpec> <name>(<Expr>) is used.
   //
   TokenPtr expr_;

   //  The assignment statement that initializes the data.
   //
   ExprPtr init_;

   //  How many times the data was read.
   //
   size_t reads_ : 16;

   //  How many times the data was written.
   //
   size_t writes_ : 16;
};

//------------------------------------------------------------------------------
//
//  Data declared at file scope.
//
class SpaceData : public Data
{
public:
   //  Creates a data item defined by NAME and TYPE.  NAME is qualified when
   //  initializing static data declared in a class.
   //
   SpaceData(QualNamePtr& name, TypeSpecPtr& type);

   //  Not subclassed.
   //
   ~SpaceData();

   //  Overridden to log warnings associated with the declaration.
   //
   void Check() const override;

   //  Overridden to support the deletion of unused or write-only data.
   //
   void Delete() override;

   //  Overridden to display the data declaration and definition.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;

   //  Overridden to add the item to the current scope.
   //
   bool EnterScope() override;

   //  Adds a data declaration to ITEMS.
   //
   void GetDecls(CxxNamedSet& items) override;

   //  Overridden to return the item's qualified name.
   //
   QualName* GetQualName() const override { return name_.get(); }

   //  Overridden to support static member data in a template.
   //
   const TemplateParms* GetTemplateParms() const
      override { return parms_.get(); }

   //  Overridden to return the item's name.
   //
   const std::string& Name() const override { return name_->Name(); }

   //  Overridden to find the item located at POS.
   //
   CxxToken* PosToItem(size_t pos) const override;

   //  Overridden to return the item's qualified name.
   //
   std::string QualifiedName(bool scopes, bool templates) const
      override { return name_->QualifiedName(scopes, templates); }

   //  Overridden to record usage of the item.
   //
   void RecordUsage() override { AddUsage(); }

   //  Overridden to support static member data in a template.
   //
   void SetTemplateParms(TemplateParmsPtr& parms) override;

   //  Overridden to shrink containers.
   //
   void Shrink() override;

   //  Overridden to update the data's location.
   //
   void UpdatePos(EditorAction action,
      size_t begin, size_t count, size_t from) const override;
private:
   //  Overridden to clone the qualified name.
   //
   void GetInitName(QualNamePtr& qualName) const override;

   //  Checks for static data in a header.
   //
   void CheckIfStatic() const;

   //  Checks that global data is initialized.
   //
   void CheckIfInitialized() const;

   //  The data item's name.  The definition of static class data contains
   //  a qualified name at file scope.  Declarations do not have qualified
   //  names (and so name_->size() == 1).
   //
   const QualNamePtr name_;

   //  The template parameters for static member data belonging to a template.
   //
   TemplateParmsPtr parms_;
};

//------------------------------------------------------------------------------
//
//  Data declared in a class.
//
class ClassData : public Data
{
public:
   //  Creates a data member defined by NAME and TYPE.
   //
   ClassData(std::string& name, TypeSpecPtr& type);

   //  Not subclassed.
   //
   ~ClassData();

   //  Specifies whether the data is mutable.
   //
   void SetMutable(bool mute) { mutable_ = mute; }

   //  Sets the data's field width.
   //
   void SetWidth(ExprPtr& width) { width_.reset(width.release()); }

   //  Sets the member initialization expression provided by the
   //  constructor that is currently being compiled.
   //
   void SetMemInit(const MemberInit* init);

   //  Overridden to add the data's components to cross-references.
   //
   void AddToXref(bool insert) override;

   //  Overridden to log warnings associated with the declaration.
   //
   void Check() const override;

   //  Overridden to support the deletion of unused or write-only data.
   //
   void Delete() override;

   //  Overridden to display the data declaration and definition.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;

   //  Overridden to invoke EnterBlock on any field width expression.
   //
   void EnterBlock() override;

   //  Overridden to add the item to the current scope.
   //
   bool EnterScope() override;

   //  Adds a data declaration to ITEMS.
   //
   void GetDecls(CxxNamedSet& items) override;

   //  Overridden to update SYMBOLS with the data's type usage.
   //
   void GetUsages(const CodeFile& file, CxxUsageSets& symbols) override;

   //  Overridden to return true if the member appears in a union.
   //
   bool IsUnionMember() const override;

   //  Overridden to mark the item as a member of the context class if
   //  it was pushed via "this".
   //
   StackArg MemberToArg
      (StackArg& via, TypeName* name, Cxx::Operator op) override;

   //  Overridden to return the item's name.
   //
   const std::string& Name() const override { return name_; }

   //  Overridden to support an implicit "this".
   //
   StackArg NameToArg(Cxx::Operator op, TypeName* name) override;

   //  Overridden to find the item located at POS.
   //
   CxxToken* PosToItem(size_t pos) const override;

   //  Overridden to promote the member, which currently belongs to
   //  an anonymous union, to CLS, the union's outer scope.
   //
   void Promote(Class* cls, Cxx::Access access, bool first, bool last) override;

   //  Overridden to record usage of the item.
   //
   void RecordUsage() override { AddUsage(); }

   //  Overridden to track usage of the "mutable" attribute.
   //
   void WasMutated(const StackArg* arg) override;

   //  Overridden to shrink containers.
   //
   void Shrink() override;

   //  Overridden to return the item's name.
   //
   std::string Trace() const override { return Name(); }

   //  Overridden to update the item's location.
   //
   void UpdatePos(EditorAction action,
      size_t begin, size_t count, size_t from) const override;

   //  Overridden to track usage of the "mutable" attribute.
   //
   bool WasWritten(const StackArg* arg, bool direct, bool indirect) override;
private:
   //  Overridden to check that data members are private.
   //
   void CheckAccessControl() const override;

   //  Checks that static data has an initialization statement.
   //
   void CheckIfInitialized() const;

   //  Checks if static data could be free (i.e. moved out of the class and
   //  into the .cpp that initializes it).
   //
   void CheckIfRelocatable() const;

   //  Checks if mutable data does not need to be mutable.
   //
   void CheckIfMutated() const;

   //  The data item's name.
   //
   std::string name_;

   //  The field width.
   //
   ExprPtr width_;

   //  The member initialization statement provided by the constructor
   //  that is currently being compiled.
   //
   const MemberInit* memInit_;

   //  Set for mutable data.
   //
   bool mutable_ : 1;

   //  Set if a const function modified the data.
   //
   bool mutated_ : 1;

   //  Set for the first member in an anonymous union.
   //
   bool first_ : 1;

   //  Set for the last member in an anonymous union.
   //
   bool last_ : 1;

   //  If non-zero, the member's depth in nested anonymous unions.
   //
   uint8_t depth_;
};

//------------------------------------------------------------------------------
//
//  Data declared in a function.
//
class FuncData : public Data
{
public:
   //  Creates a data item defined by NAME and TYPE.
   //
   FuncData(std::string& name, TypeSpecPtr& type);

   //  Not subclassed.
   //
   ~FuncData();

   //  Appends the next data declaration in a series (e.g. bool b1, b2).
   //
   void SetNext(FuncDataPtr& next);

   //  Sets the first data declaration in a series.
   //
   void SetFirst(FuncData* first) { first_ = first; }

   //  Overridden to add the data's components to cross-references.
   //
   void AddToXref(bool insert) override;

   //  Overridden to log warnings associated with the data.
   //
   void Check() const override;

   //  Overridden to support the deletion of unused or write-only data.
   //
   void Delete() override;

   //  Overridden to display the data declaration and definition.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;

   //  Overridden to make the item visible as a local.
   //
   void EnterBlock() override;

   //  Overridden to remove the item as a local.
   //
   void ExitBlock() const override;

   //  Overridden to update SYMBOLS with the data's type usage.
   //
   void GetUsages(const CodeFile& file, CxxUsageSets& symbols) override;

   //  Overridden to indicate that inline display is supported.
   //
   bool InLine() const override { return true; }

   //  Overridden to return the item's name.
   //
   const std::string& Name() const override { return name_; }

   //  Overridden to find the item located at POS.
   //
   CxxToken* PosToItem(size_t pos) const override;

   //  Overridden to display the data declaration and definition.
   //
   void Print
      (std::ostream& stream, const NodeBase::Flags& options) const override;

   //  Overridden to shrink containers.
   //
   void Shrink() override;

   //  Overridden to return the item's name.
   //
   std::string Trace() const override { return Name(); }

   //  Overridden to update the data's location.
   //
   void UpdatePos(EditorAction action,
      size_t begin, size_t count, size_t from) const override;
private:
   //  Invoked by Display on each declaration in a possible series.
   //
   void DisplayItem(std::ostream& stream, const NodeBase::Flags& options) const;

   //  Overridden to support multiple declarations on the same line by
   //  including the preceding comma, else the following comma, else nothing
   //  extra.
   //
   bool GetSpan(size_t& begin, size_t& left, size_t& end) const override;

   //  The data item's name.
   //
   std::string name_;

   //  The next declaration in a series.
   //
   FuncDataPtr next_;

   //  The first declaration in a series.
   //
   FuncData* first_;
};

//------------------------------------------------------------------------------
//
//  A function.
//
class Function : public CxxScope
{
public:
   //  Creates a function identified by NAME.
   //
   explicit Function(QualNamePtr& name);

   //  Creates a function identified by NAME, which returns the type described
   //  by SPEC.  TYPE is set when parsing a typedef for a function signature.
   //
   Function(QualNamePtr& name, TypeSpecPtr& spec, bool type = false);

   //  Not subclassed.
   //
   ~Function();

   //  Adds an argument to the function.
   //
   void AddArg(ArgumentPtr& arg);

   //  Removes ARG from the function when ARG is being deleted.
   //
   void EraseArg(const Argument* arg);

   //  Specifies whether the function is tagged as extern.
   //
   void SetExtern(bool extn) { extern_ = extn; }

   //  Specifies whether the function is tagged as inline.
   //
   void SetInline(bool inln) { inline_ = inln; }

   //  Specifies whether the function is tagged as constexpr.
   //
   void SetConstexpr(bool cexpr) { constexpr_ = cexpr; }

   //  Specifies whether the function is static.  If not declared static,
   //  it is forced to static if OPER is not passed a "this" pointer.
   //
   void SetStatic(bool stat, Cxx::Operator oper);

   //  Specifies whether the function is virtual.
   //
   void SetVirtual(bool virt) { virtual_ = virt; }

   //  Specifies whether a constructor is explicit.
   //
   void SetExplicit(bool expl) { explicit_ = expl; }

   //  Specifies whether the function is const.
   //
   void SetConst(bool readonly) { const_ = readonly; }

   //  Specifies whether the function is tagged "volatile".
   //
   void SetVolatile(bool unstable) { volatile_ = unstable; }

   //  Specifies whether the function is tagged "noexcept".
   //
   void SetNoexcept(bool noex) { noexcept_ = noex; }

   //  Specifies whether the function is tagged "override".
   //
   void SetOverride(bool over) { override_ = over; }

   //  Specifies whether the function is tagged "override".
   //
   void SetFinal(bool final) { final_ = final; }

   //  Specifies whether the function is pure virtual.
   //
   void SetPure(bool pure) { pure_ = pure; }

   //  Sets the operator if the function is an operator overload.
   //
   void SetOperator(Cxx::Operator oper);

   //  Invoked if the function is defined as deleted.
   //
   void SetDeleted() { deleted_ = true; }

   //  Invoked if the function is defined as defaulted.
   //
   void SetDefaulted() { defaulted_ = true; }

   //  Marks the function as an inline friend function.
   //
   void SetFriend() { friend_ = true; }

   //  Marks the function as being an implicitly invoked constructor.
   //
   void SetImplicit() { implicit_ = true; }

   //  Sets the base class constructor call.
   //
   void SetBaseInit(ExprPtr& init);

   //  Adds a member initialization to a constructor initialization list.
   //
   void AddMemberInit(MemberInitPtr& init);

   //  Removes INIT from the function when INIT is being deleted.
   //
   void EraseMemberInit(const MemberInit* init);

   //  Sets the starting location (opening brace) of an inline function.
   //
   void SetBracePos(size_t pos) { pos_ = pos; }

   //  Sets the function's implementation, which is immediately compiled.
   //
   void SetImpl(BlockPtr& block);

   //  Sets the function template for a template instance.
   //
   void SetTemplate(Function* func) { tmplt_ = func; }

   //  Sets the name (with template arguments) for a template instance.
   //
   void SetTemplateArgs(const TypeName* spec);

   //  Invoked when a function uses a template parameter.
   //
   void SetTemplateParm() { tparm_ = true; }

   //  Returns true if the function was explicitly inlined.
   //
   bool IsInline() const { return inline_; }

   //  Returns the function's operator (Cxx::NIL_OPERATOR if not an operator).
   //
   Cxx::Operator Operator() const { return name_->Operator(); }

   //  Returns true if the function is deleted or declared private to make
   //  it inaccessible (e.g. a private constructor or assignment operator).
   //
   bool IsDeleted() const;

   //  Returns true if the function is a default special member function.
   //
   bool IsDefaulted() const { return GetDefn()->defaulted_; }

   //  Returns the function's arguments.
   //
   const ArgumentPtrVector& GetArgs() const { return args_; }

   //  Returns the index of ARG in args_.  The index is incremented if DISP
   //  is set and the function does not have an implicit "this" argument.
   //  Returns SIZE_MAX if ARG is not in args_.
   //
   size_t FindArg(const Argument* arg, bool disp) const;

   //  OFFSET is an argument index in a CodeWarning log, which was obtained
   //  with FindArg(arg, true).  This returns its actual index in GetArgs().
   //
   size_t LogOffsetToArgIndex(NodeBase::word offset) const;

   //  Returns true for a function declaration (which might also define the
   //  function).  Returns false for the definition of a previously declared
   //  function.
   //
   bool IsDecl() const { return !defn_; }

   //  Returns the function's declaration.
   //
   const Function* GetDecl() const { return (defn_ ? mate_ : this); }
   Function* GetDecl() { return (defn_ ? mate_ : this); }

   //  Returns the function's definition (if distinct from its declaration),
   //  else its declaration.
   //
   Function* GetDefn();
   const Function* GetDefn() const;

   //  The following are all forwarded to the function declaration because
   //  the fields in question are not set on a function definition.  When
   //  code that implements this class uses one of these fields directly, it
   //  is to deliberately exclude a function definition from consideration.
   //
   //  GetBase: Returns the function that this one overrides.
   //  IsExtern: Returns true if the function was tagged "extern".
   //  IsVirtual: Returns true if the function is virtual.
   //  IsPureVirtual: Returns true if the function is pure virtual.
   //  IsExplicit: Returns true if the function was tagged "explicit".
   //  IsOverride: Returns true if the function is an override.
   //  IsFinal: Returns true if the function is final.
   //  IsStatic: Returns true if the function is static.
   //  GetAccess: Returns the access control for the function.
   //  GetImpl: Returns the function's implementation.
   //
   Function* GetBase() const { return GetDecl()->base_; }
   bool IsExtern() const { return GetDecl()->extern_; }
   bool IsVirtual() const { return GetDecl()->virtual_; }
   bool IsPureVirtual() const { return GetDecl()->pure_; }
   bool IsExplicit() const { return GetDecl()->explicit_; }
   bool IsOverride() const { return GetDecl()->override_; }
   bool IsFinal() const { return GetDecl()->final_; }
   bool IsStatic() const override { return GetDecl()->static_; }
   Cxx::Access GetAccess() const override;
   const Block* GetImpl() const { return GetDefn()->impl_.get(); }

   //  Deletes the single argument "(void)", which simplifies the comparison
   //  of function signatures.
   //
   void DeleteVoidArg();

   //  Returns the first declaration of this function in the class hierarchy
   //  (that is, virtual but not an override).  Returns the function itself
   //  if it is not an override.
   //
   Function* FindRootFunc() const;

   //  Determines how the function is associated with a template.
   //
   TemplateType GetTemplateType() const;

   //  Returns true if this is a function template instance.
   //
   bool IsTemplateInstance() const { return tmplt_ != nullptr; }

   //  Returns true if the function is a compiled function template.
   //
   bool ContainsTemplateParameter() const;

   //  Pushes an implicit "this" argument that may be needed later.
   //
   void PushThisArg(StackArgVector& args) const;

   //  If there is a "this" argument at the front of ARGS
   //  o if this function does not need it, it is erased
   //  o if this function is a constructor, it is replaced with the
   //    constructor's actual this argument
   //
   void UpdateThisArg(StackArgVector& args) const;

   //  Returns true if the function is empty or only invokes Debug::ft.
   //
   bool IsTrivial() const;

   //  Returns the function's type.
   //
   FunctionType FuncType() const;

   //  Returns the function's role.
   //
   FunctionRole FuncRole() const;

   //  Returns the minimum number of arguments that the function accepts.
   //  Includes any "this" argument.
   //
   size_t MinArgs() const;

   //  Returns the maximum number of arguments that the function accepts.
   //  Includes any "this" argument.
   //
   size_t MaxArgs() const { return args_.size(); }

   //  Invoked on the function's return type and arguments by EnterScope.
   //
   void EnterSignature();

   //  Returns the starting location (opening brace) of an inline function.
   //
   size_t GetBracePos() const { return pos_; }

   //  Decides if this function can be invoked with ARGS, whose TypeString
   //  results appear in argTypes.  Updates MATCH to indicate how well the
   //  arguments and the function match.  Returns the function itself--or,
   //  if it is a function template, the correct instance.  Returns nullptr
   //  if the function does not match ARGS.
   //
   Function* CanInvokeWith
      (StackArgVector& args, stringVector& argTypes, TypeMatch& match) const;

   //  THAT is an argument whose type is thatType.  If this is a constructor
   //  that can be invoked implicitly with THAT, determines how compatible
   //  THAT is with the constructor's argument.
   //
   TypeMatch CalcConstructibilty
      (const StackArg& that, const std::string& thatType) const;

   //  Returns true if this function matches THAT on constness, return types,
   //  and arguments.  If BASE is set, this function's "this" argument can be
   //  a subclass of THAT function's "this" argument.
   //
   bool SignatureMatches(const Function* that, bool base) const;

   //  Registers a read on the function's "this" argument.
   //
   void IncrThisReads() const;

   //  Registers a write on the function's "this" argument.
   //
   void IncrThisWrites() const;

   //  Invoked when the function accessed ITEM; if VIA is provided, ITEM was
   //  accessed as VIA.ITEM or VIA->ITEM.
   //
   void ItemAccessed(const CxxNamed* item, const StackArg* via);

   //  Registers an invocation of the function.  ARGS is a list of resolved
   //  arguments, in the same order as declared by the function.  A read is
   //  noted for each argument, and a write if it was passed as a non-const
   //  pointer or reference.  Returns something other than Warning_N if a
   //  warning should be logged on the function call.
   //
   Warning Invoke(StackArgVector* args);

   //  Returns true if the function was invoked.  Considers virtuality.
   //
   bool HasInvokers() const;

   //  Returns true if the function is an override and it is directly invoked
   //  in a base class.
   //
   bool IsInvokedInBase() const;

   //  Returns an argument based on the function's return type.
   //
   StackArg ResultType() const;

   //  Instantiates a function template instance based on TYPE.  If it has
   //  already been instantiated, it is found and returned.
   //
   Function* InstantiateFunction(const TypeName* type) const;

   //  Returns the function's name for Debug::ft purposes.
   //
   std::string DebugName() const;

   //  Returns true if STR is a suitable string for identifying the function
   //  when invoking Debug::ft.
   //
   bool CheckDebugName(const std::string& str) const;

   //  Returns the function's overrides.
   //
   const FunctionVector& GetOverrides() const { return overs_; }

   //  Displays the function's declaration.
   //
   void DisplayDecl(std::ostream& stream, const NodeBase::Flags& options) const;

   //  Overridden to add the function's components to cross-references.
   //
   void AddToXref(bool insert) override;

   //  Overridden to support the deletion of an unused function.
   //
   void Delete() override;

   //  Overridden to log warnings associated with the function.
   //
   void Check() const override;

   //  Overridden to skip destructors.
   //
   void CheckAccessControl() const override;

   //  Overridden to not log an override for hiding an inherited name.
   //
   void CheckIfHiding() const override;

   //  Overridden to generate a log if the function is unused.
   //
   bool CheckIfUnused(Warning warning) const override;

   //  Overridden to display the function.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;

   //  Overridden to compile the function.
   //
   void EnterBlock() override;

   //  Overridden to add the function to the current scope.
   //
   bool EnterScope() override;

   //  Overridden to search the function's arguments and code for an item that
   //  matches NAME.
   //
   CxxScoped* FindNthItem(const std::string& name, size_t& n) const override;

   //  Overridden to return the template item that corresponds to ITEM
   //  if this is a function in a template instance.
   //
   CxxScoped* FindTemplateAnalog(const CxxToken* item) const override;

   //  Overridden to return the file that declared the function.
   //
   CodeFile* GetDeclFile() const override;

   //  Adds a function declaration to ITEMS.
   //
   void GetDecls(CxxNamedSet& items) override;

   //  Overridden to return the file (if any) that defined the function.
   //
   CodeFile* GetDefnFile() const override;

   //  Overridden to return the function itself.
   //
   Function* GetFunction()
      const override { return const_cast< Function* >(this); }

   //  Overridden to return the definition if it is distinct from the
   //  declaration, and vice versa.
   //
   CxxNamed* GetMate() const override { return mate_; }

   //  Overridden to return the function's qualified name.
   //
   QualName* GetQualName() const override { return name_.get(); }

   //  Overridden to handle an inline friend function.
   //
   CxxScope* GetScope() const override;

   //  Overridden to return a template if this function
   //  (a) is a function template
   //  (b) is a function template instance
   //  (c) is a function in a class template
   //  (d) is a function in a class template instance
   //
   CxxScope* GetTemplate() const override;

   //  Overridden to return the template arguments for a function
   //  template instance.
   //
   TypeName* GetTemplateArgs() const override { return tspec_.get(); }

   //  Overridden to return this function if it is a template instance.
   //
   CxxScope* GetTemplateInstance() const override;

   //  Overridden to support function templates.
   //
   const TemplateParms* GetTemplateParms() const
      override { return parms_.get(); }

   //  Overridden to return the function's return type.
   //
   TypeSpec* GetTypeSpec() const override { return spec_.get(); }

   //  Overridden to update SYMBOLS with the type usage of each of the
   //  function's components.
   //
   void GetUsages(const CodeFile& file, CxxUsageSets& symbols) override;

   //  Overridden to indicate whether the function itself is const.
   //
   bool IsConst() const override { return const_; }

   //  Overridden to look for an implemented or defaulted function.
   //
   bool IsImplemented() const override;

   //  Overridden to determine if the function is unused.
   //
   bool IsUnused() const override;

   //  Overridden to indicate whether the function is tagged volatile.
   //
   bool IsVolatile() const override { return volatile_; }

   //  Overridden to search the function's arguments and code for ITEM.
   //
   bool LocateItem(const CxxToken* item, size_t& n) const override;

   //  Overridden to include VIA as a "this" argument.
   //
   StackArg MemberToArg
      (StackArg& via, TypeName* name, Cxx::Operator op) override;

   //  Overridden to return the function's name.
   //
   const std::string& Name() const override { return name_->Name(); }

   //  Overridden for when NAME refers to a function template instance.
   //
   bool NameRefersToItem(const std::string& name, const CxxScope* scope,
      CodeFile* file, SymbolView& view) const override;

   //  Overridden to find the item located at POS.
   //
   CxxToken* PosToItem(size_t pos) const override;

   //  Overridden to return the function's qualified name.
   //
   std::string QualifiedName(bool scopes, bool templates) const
      override { return name_->QualifiedName(scopes, templates); }

   //  Overridden to record usage of the function.
   //
   void RecordUsage() override;

   //  Overridden to support function templates.
   //
   void SetTemplateParms(TemplateParmsPtr& parms) override;

   //  Overridden to shrink containers.
   //
   void Shrink() override;

   //  Overridden to reveal that this is a function.
   //
   Cxx::ItemType Type() const override { return Cxx::Function; }

   //  Overridden to return the function's full root signature.  The
   //  function's name is omitted if ARG is set.
   //
   std::string TypeString(bool arg) const override;

   //  Overridden to update the function's location.
   //
   void UpdatePos(EditorAction action,
      size_t begin, size_t count, size_t from) const override;

   //  Tracks how many times the function was invoked, and propagates
   //  constructor and destructor invocations up the class hierarchy.
   //
   void WasCalled();

   //  Overridden to count a read as an invocation.
   //
   bool WasRead() override;

   //  Overridden  to append argument types if the function's name is ambiguous.
   //
   std::string XrefName(bool templates) const override;
private:
   //  Adds a "this" argument to the function if required.  This occurs
   //  immediately before compiling the function's code (in EnterBlock).
   //
   void AddThisArg();

   //  Returns true if the function could be "noexcept".
   //
   bool CanBeNoexcept() const;

   //  Checks if the function could be tagged "noexcept".
   //
   void CheckNoexcept() const;

   //  If the function is an override, registers it with its base class and
   //  logs a warning if the "virtual" or "override" tags are absent.
   //
   void CheckOverride();

   //  Tracks overrides of the function.  Overrides are registered against
   //  the closest base class that also defines the function, even if that
   //  definition is itself an override.  This makes it easier to find "final"
   //  functions, even if they are not tagged as such.
   //
   void AddOverride(Function* over) const;

   //  Removes OVER as an override of the function.
   //
   void EraseOverride(const Function* over) const;

   //  Returns true if this function's arguments match those of THAT.
   //
   bool ArgumentsMatch(const Function* that) const;

   //  Returns the next declaration of this function up the class hierarchy.
   //  For a destructor, returns the next destructor.  Returns nullptr for a
   //  constructor.
   //
   Function* FindBaseFunc() const;

   //  Invoked on a function's declaration when its definition (FUNC) is found.
   //
   void SetDefn(Function* func);

   //  Returns true if this function, located above CLS, is overridden by CLS
   //  or one of its subclasses.  Returns false if this function is *in* CLS.
   //
   bool IsOverriddenAtOrBelow(const Class* cls) const;

   //  Returns true if the function is undefined (has no code, is deleted, or
   //  is part of a typedef for a function signature).
   //
   bool IsUndefined() const;

   //  Registers a call to the base class's default constructor when a
   //  constructor does not call a base constructor explicitly.
   //
   void InvokeDefaultBaseCtor() const;

   //  Returns FUNC after setting MATCH to Incompatible if FUNC is nullptr.
   //  ARGS contains the arguments that were passed to CanInvokeWith.
   //
   static Function* FoundFunc
      (Function* func, const StackArgVector& args, TypeMatch& match);

   //  Determines if thatType (an argument passed to this function) matches
   //  --or can specialize--thisType, the argument expected by this function.
   //  tmpltParms contains the template parameters defined by this function.
   //  As the corresponding template arguments are found, they are added to
   //  tmpltArgs.  The result indicates how well thatType matches or specializes
   //  thisType.  Also sets argFound if thisType contains a template parameter.
   //
   static TypeMatch MatchTemplate(const std::string& thisType,
      const std::string& thatType, stringVector& tmpltParms,
      stringVector& tmpltArgs, bool& argFound);

   //  Instantiates a function template instance according to tmpltArgs, which
   //  contains the TypeString that specializes each template parameter in the
   //  same order that the parameters were defined.  If the function instance
   //  has already been instantiated, it is found and returned.
   //
   Function* InstantiateFunction(stringVector& tmpltArgs) const;

   //  Invoked when InstantiateFunction fails.
   //
   static Function* InstantiateError
      (const std::string& instName, NodeBase::debug64_t offset);

   //  Marks recvArg const if
   //  o it's a "this" argument and this function also has a const version, or
   //  o this function is virtual and INVOKER (the function invoking this one)
   //    is another instance of that function.
   //
   void AdjustRecvConstness(const Function* invoker, StackArg& recvArg) const;

   //  Invoked when the function accessed a non-public member in its class.
   //
   void SetNonPublic();

   //  Invoked when the function accessed a non-static member in its class.
   //
   void SetNonStatic();

   //  The copy constructor or copy operator is declared but not invoked.
   //  Returns true unless the destructor exists or the other copy function
   //  is invoked.
   //
   bool IsUnusedCopyFunction() const;

   //  If this is a function template, returns its first instance.
   //
   Function* FirstInstance() const;

   //  If this function belongs to a class template, returns its analog in
   //  the first class template instance.
   //
   Function* FirstInstanceInClass() const;

   //  Returns true if the Nth argument is unused.
   //
   bool ArgIsUnused(size_t n) const;

   //  Returns true if the Nth argument could be const.
   //
   bool ArgCouldBeConst(size_t n) const;

   //  Returns true if ARG is a template argument.
   //
   bool IsTemplateArg(const Argument* arg) const;

   //  Checks a function and logs any warnings that it detects.
   //
   void CheckArgs() const;
   void CheckCtor() const;
   void CheckDtor() const;
   Warning CheckIfDefined() const;
   void CheckIfPublicVirtual() const;
   void CheckForVirtualDefault() const;
   void CheckIfOverridden() const;
   void CheckIfCouldBeConst() const;
   void CheckMemberUsage() const;
   void CheckStatic() const;
   void CheckFree() const;

   //  Logs WARNING on the argument at INDEX, with the fields in CodeWarning
   //  being set as follows:
   //  o pos_ is the position of the argument
   //  o offset_ is the argument's index as seen in source code: it must be
   //    decremented to obtain its internal index unless the function has a
   //    "this" argument (see LogOffsetToArgIndex).
   //  o item_ is the actual function
   //
   void LogToArg(Warning warning, size_t index) const;

   //  Displays the function's definition.
   //
   void DisplayDefn(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const;

   //  Displays information about where the function is implemented, how many
   //  many times it was overridden, and how many times it was invoked.
   //
   void DisplayInfo(std::ostream& stream, const NodeBase::Flags& options) const;

   //  Overridden to support a function definition by setting LEFT and END
   //  to the locations of its left and right braces.
   //
   bool GetSpan(size_t& begin, size_t& left, size_t& end) const override;

   //  The function's name.
   //
   const QualNamePtr name_;

   //  The template parameters for a function template.
   //
   TemplateParmsPtr parms_;

   //  The name (with template arguments) of a function template instance.
   //  This is saved because its actual name is a single string containing
   //  the arguments.  The code supports such names rather than generating
   //  an artificial but legal C++ identifier.
   //
   TypeNamePtr tspec_;

   //  Set for a function tagged as extern.  ONLY SET ON THE DECLARATION.
   //
   bool extern_ : 1;

   //  Set for a function explicitly tagged as inline or constexpr.
   //  Not set for a function whose definition is inlined unless
   //  tagged as inline or constexpr.
   //
   bool inline_ : 1;

   //  Set for a function tagged as constexpr.
   //
   bool constexpr_ : 1;

   //  Set for a static function.  ONLY SET ON THE DECLARATION.
   //
   bool static_ : 1;

   //  Set for a virtual function, even if it is not tagged "virtual".
   //  ONLY SET ON THE DECLARATION.
   //
   bool virtual_ : 1;

   //  Set for an explicit constructor.  ONLY SET ON THE DECLARATION.
   //
   bool explicit_ : 1;

   //  Set for a const function.
   //
   bool const_ : 1;

   //  Set for a function tagged "volatile".
   //
   bool volatile_ : 1;

   //  Set for a function tagged "noexcept".
   //
   bool noexcept_ : 1;

   //  Set for an override, even if it is not tagged "override".
   //  ONLY SET ON THE DECLARATION.
   //
   bool override_ : 1;

   //  Set for a function tagged "final".  ONLY SET ON THE DECLARATION.
   //
   bool final_ : 1;

   //  Set for a pure virtual function.  ONLY SET ON THE DECLARATION.
   //
   bool pure_ : 1;

   //  Set if this is part of the typedef for a function signature.
   //
   const bool type_ : 1;

   //  Set if this is an inline friend function.
   //
   bool friend_ : 1;

   //  Set when EnterScope is invoked.
   //
   bool found_ : 1;

   //  Set if the function has a "this" argument.
   //
   bool this_ : 1;

   //  Set if the function uses a template parameter.
   //
   bool tparm_ : 1;

   //  Set if the function used a non-public member in its own class.
   //
   bool nonpublic_ : 1;

   //  Set if the function used a non-static member in its own class.
   //
   bool nonstatic_ : 1;

   //  Set for a constructor that was invoked implicitly.
   //
   bool implicit_ : 1;

   //  Set if this is the definition of a previously declared function.
   //
   bool defn_ : 1;

   //  Set if the function was defined as "= delete".
   //
   bool deleted_ : 1;

   //  Set if the function was defined as "= default".
   //
   bool defaulted_ : 1;

   //  How many times the function was invoked.  Only incremented on the
   //  declaration.
   //
   size_t calls_ : 16;

   //  If defn_ is set, the function's declaration.  If defn_ is not set,
   //  the function's definition (if distinct from its declaration).
   //
   Function* mate_;

   //  The type returned by the function.
   //
   const TypeSpecPtr spec_;

   //  The function's arguments.
   //
   ArgumentPtrVector args_;

   //  For a constructor, the call to the base class constructor.
   //
   ExprPtr call_;

   //  For a constructor, the member initialization list.
   //
   MemberInitPtrVector mems_;

   //  The function's implementation.
   //
   BlockPtr impl_;

   //  The position of an inline function.  This is saved so that it can
   //  be parsed after reaching the end of the class's declaration.
   //
   size_t pos_;

   //  The next function, up the class hierarchy, that this one overrides.
   //  ONLY SET ON THE DECLARATION.
   //
   Function* base_;

   //  The function template of which this is an instance.
   //
   Function* tmplt_;

   //  The code for a function template.
   //
   mutable NodeBase::stringPtr code_;

   //  A function template's instantiations.
   //
   mutable FunctionVector tmplts_;

   //  Direct overrides of the function.  ONLY USED BY THE DECLARATION.
   //
   mutable FunctionVector overs_;
};

//  Returns true if the definition of FUNC1 should precede that of FUNC2.  This
//  is NOT suitable for std::sort, because there are situations where it allows
//  FUNC1 to precede FUNC2 or vice versa.  For example, it simply returns true
//  if FUNC1 and FUNC2 are not in the same scope, as the sorting of functions
//  in different scopes is outside the scope of this function. :)
//
bool FuncDefnsAreSorted(const Function* func1, const Function* func2);

//  Returns the functions in DEFNS that belong to AREA.
//
FunctionVector FuncsInArea(const FunctionVector& defns, const CxxArea* area);

//------------------------------------------------------------------------------
//
//  A namespace definition: one occurrence of namespace NS { ... }.
//
class SpaceDefn : public CxxScope
{
public:
   //  NS aggregates *all* items defined in namespace NS, whereas this class
   //  represents a single occurrence of a namespace definition that defines
   //  some of the namespace's items.
   //
   explicit SpaceDefn(Namespace* ns);

   //  Not subclassed.
   //
   ~SpaceDefn();

   //  Overridden to add itself as a reference to space_.
   //
   void AddToXref(bool insert) override;

   //  Overridden to support the deletion of an empty namespace definition.
   //
   void Delete() override;

   //  Adds the namespace to ITEMS.
   //
   void GetDecls(CxxNamedSet& items) override;

   //  Overridden to forward to space_.
   //
   const std::string& Name() const override;

   //  Overridden to forward to space_.
   //
   std::string ScopedName(bool templates) const override;
private:
   //  Overridden to set LEFT and END to the positions of the left and right
   //  braces.
   //
   bool GetSpan(size_t& begin, size_t& left, size_t& end) const override;

   //  The primary class for the namespace.
   //
   Namespace* const space_;
};

//------------------------------------------------------------------------------
//
//  A function type.
//
class FuncSpec : public TypeSpec
{
public:
   //  Creates a function type with FUNC's signature.
   //
   explicit FuncSpec(FunctionPtr& func);

   //  Not subclassed.
   //
   ~FuncSpec();

   //  Deleted to prohibit copying.
   //
   FuncSpec(const FuncSpec& that) = delete;
private:
   //  The following are overridden to return the function signature.
   //
   Function* GetFuncSpec() const override { return func_.get(); }
   CxxScoped* Referent() const override { return func_.get(); }
   CxxToken* RootType() const override { return func_.get(); }

   //  The following are forwarded to the function.
   //
   void AddToXref(bool insert) override;
   void Check() const override;
   bool ContainsTemplateParameter() const override;
   void EnteringScope(const CxxScope* scope) override;
   bool IsConst() const override { return func_->IsConst(); }
   bool IsVolatile() const override { return func_->IsVolatile(); }
   const std::string& Name() const override { return func_->Name(); }
   CxxToken* PosToItem(size_t pos) const override;
   void Print
      (std::ostream& stream, const NodeBase::Flags& options) const override;
   void Shrink() override;
   std::string Trace() const override;
   std::string TypeString(bool arg) const override;
   void UpdatePos(EditorAction action,
      size_t begin, size_t count, size_t from) const override;

   //  The following are forwarded to the function's return type.
   //
   void AddArray(ArraySpecPtr& array) override;
   std::string AlignTemplateArg(const TypeSpec* thatArg) const override;
   TagCount Arrays() const override;
   void DisplayArrays(std::ostream& stream) const override;
   void DisplayTags(std::ostream& stream) const override;
   TypeTags GetAllTags() const override;
   TypeName* GetTemplateArgs() const override;
   TypeSpec* GetTypeSpec() const override;
   bool HasArrayDefn() const override;
   TagCount Ptrs(bool arrays) const override;
   TagCount Refs() const override;
   StackArg ResultType() const override;
   void SetPtrs(TagCount count) override;
   TypeTags* Tags() override;
   const TypeTags* Tags() const override;
   std::string TypeTagsString(const TypeTags& tags) const override;

   //  The following are forwarded to the function's return type but also
   //  generate a log because they may not be properly supported.
   //
   void FindReferent() override;
   void GetNames(stringVector& names) const override;
   void Instantiating(CxxScopedVector& locals) const override;
   TypeMatch MatchTemplateArg(const TypeSpec* that) const override;
   bool ItemIsTemplateArg(const CxxNamed* item) const override;
   bool MatchesExactly(const TypeSpec* that) const override;
   TypeMatch MatchTemplate(const TypeSpec* that, stringVector& tmpltParms,
      stringVector& tmpltArgs, bool& argFound) const override;
   bool NamesReferToArgs(const NameVector& names, const CxxScope* scope,
      CodeFile* file, size_t& index) const override;

   //  The following are forwarded to the function's return type but also
   //  generate a log because they should not be invoked.
   //
   void EnterArrays() const override;
   void SetReferent(CxxScoped* item, const SymbolView* view) const override;

   //  The following is not supported.  It generates a log and returns
   //  nullptr.
   //
   TypeSpec* Clone() const override;

   //  The function signature.
   //
   const FunctionPtr func_;
};
}
#endif
