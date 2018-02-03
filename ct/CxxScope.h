//==============================================================================
//
//  CxxScope.h
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
#ifndef CXXSCOPE_H_INCLUDED
#define CXXSCOPE_H_INCLUDED

#include "CxxScoped.h"
#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <string>
#include "CodeTypes.h"
#include "Cxx.h"
#include "CxxFwd.h"
#include "CxxNamed.h"
#include "CxxString.h"
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

   //  Returns true if NAME is either this scope or a subscope of it.
   //
   bool IsSubscopeOf(const std::string& name) const;

   //  Returns the distance to the superscope SCOPE.  Returns 0 if SCOPE is
   //  the same as this scope.  Returns NOT_A_SUBSCOPE if this scope is not
   //  a subcope of SCOPE.
   //
   Distance ScopeDistance(const CxxScope* scope) const;

   //  Returns true if NAME is a template parameter.  For example, returns
   //  true when "T" appears somewhere within
   //    template< typename T > class <name> { ... };
   //
   bool NameIsTemplateParm(const std::string& name) const;

   //  Returns the identifier of the file that declares the item if it is
   //  *not* the file that also defines the item.  This can only occur for
   //  static data (an initialization) or a function (an implementation).
   //  Returns NIL_ID in all other cases.
   //
   id_t GetDistinctDeclFid() const;

   //  Returns the current access control level when parsing within the scope.
   //
   virtual Cxx::Access GetCurrAccess() const { return Cxx::Private; }

   //  Returns the scope's using statement, if any, that makes NAME visible
   //  by matching NAME at least to PREFIX.
   //
   virtual Using* GetUsingFor(const std::string& name, size_t prefix)
      const { return nullptr; }

   //  Updates VIEW to indicate the accessibility of ITEM, which was declared
   //  in this scope, to SCOPE.  The default version, for use by functions
   //  and code blocks, returns Unrestricted if SCOPE either matches or is
   //  a subscope of this scope, and Inaccessible otherwise.
   //
   virtual void AccessibilityOf
      (const CxxScope* scope, const CxxScoped* item, SymbolView* view) const;
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

   //  Adds USE to the statements that are local to the currently executing
   //  block.
   //
   static void AddUsing(Using* use);

   //  Removes USE from the statements that are local to the currently
   //  executing block.
   //
   static void RemoveUsing(const Using* use);

   //  Clears any using statements that are local to code blocks.
   //
   static void ResetUsings();

   //  Overridden to log warnings within the code.
   //
   virtual void Check() const override;

   //  Overridden to display the block.  If CRLF is set, a block that contains
   //  one statement is displayed on a new line, else it is displayed in-line.
   //  A space or endline is inserted first, as appropriate, and an endline is
   //  always inserted afterwards.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden to invoke EnterBlock on each token in statements_, followed
   //  by ExitBlock after all the statements have been executed.
   //
   virtual void EnterBlock() override;

   //  Overridden to return the function in which the block appears.
   //
   virtual Function* GetFunction() const override;

   //  Overridden to update SYMBOLS with each statement's type usage.
   //
   virtual void GetUsages
      (const CodeFile& file, CxxUsageSets& symbols) const override;

   //  Overridden to look at using statements that are local to a function.
   //
   virtual Using* GetUsingFor
      (const std::string& name, size_t prefix) const override;

   //  Overridden to determine if in-line display is possible.
   //
   virtual bool InLine() const override;

   //  Overridden to return the block's name.
   //
   virtual const std::string* Name() const override { return &name_; }

   //  Overridden to display a block in-line if it has one statement or none.
   //
   virtual void Print
      (std::ostream& stream, const Flags& options) const override;

   //  Overridden to return the enclosing function's scoped name, followed by
   //  a string that signifies executable code rather than only the function.
   //
   virtual std::string ScopedName(bool templates) const override;

   //  Overridden to shrink containers.
   //
   virtual void Shrink() override;

   //  Overridden to reveal that this is a code block.
   //
   virtual Cxx::ItemType Type() const override { return Cxx::Block; }
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

   //  The using statements visible within the currently executing block.
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

   //  Specifies whether the data is extern.
   //
   void SetExtern(bool extn) { extern_ = extn; }

   //  Specifies whether the data is static.
   //
   void SetStatic(bool stat) { static_ = stat; }

   //  Specifies whether the data is initialized with a constexpr.
   //
   void SetConstexpr(bool cexpr) { constexpr_ = cexpr; }

   //  Sets the constructor arguments.
   //
   void SetExpression(TokenPtr& expr) { expr_.reset(expr.release()); }

   //  Sets the right-hand side of the assignment statement that
   //  initializes the data.
   //
   void SetAssignment(ExprPtr& expr);

   //  Returns true if the data is extern.
   //
   bool IsExtern() const { return extern_; }

   //  Returns true if the data is initialized with a constexpr.
   //
   bool IsConstexpr() const { return constexpr_; }

   //  Returns true if the data was initialized.
   //
   bool WasInited() const { return GetDecl()->inited_; }

   //  Invoked on a data declaration when its definition (DATA) is found.
   //
   void SetDefn(Data* data);

   //  Returns true if the data is default constructible.  This function's
   //  purpose is to determine if the data will be initialized if omitted
   //  from a constructor initialization list.  This is only the case for
   //  a class that has a default (zero-argument) constructor.
   //
   bool IsDefaultConstructible() const;

   //  Invoked when promoting a member of an anonymous union to CLS, its
   //  outer scope.  ACCESS is the union's access control.  FIRST and LAST
   //  are set if the member is the first and/or last member of the union.
   //
   virtual void Promote
      (Class* cls, Cxx::Access access, bool first, bool last) { }

   //  Returns true if the member defines a const string that is suitable
   //  for identifying FUNC when invoking Debug::ft.
   //
   bool CheckFunctionString(const Function* func) const;

   //  Overridden to set the type for an "auto" variable.
   //
   virtual CxxToken* AutoType() const override { return (CxxToken*) this; }

   //  Overridden to log warnings associated with the data's type.
   //
   virtual void Check() const override;

   //  Overridden to return the file that declared the data.
   //
   virtual CodeFile* GetDeclFile() const override;

   //  Overridden to return the file (if any) that defined (initialized) the
   //  data.
   //
   virtual CodeFile* GetDefnFile() const override;

   //  Overridden to return the data's underlying numeric type.
   //
   virtual Numeric GetNumeric() const override { return spec_->GetNumeric(); }

   //  Overridden to return the data's type.
   //
   virtual TypeSpec* GetTypeSpec() const override { return spec_.get(); }

   //  Overridden to search the data's type for template arguments.
   //
   virtual TypeName* GetTemplateArgs() const override;

   //  Overridden to update SYMBOLS with the data's type usage.
   //
   virtual void GetUsages
      (const CodeFile& file, CxxUsageSets& symbols) const override;

   //  Overridden to indicate whether the data is const.
   //
   virtual bool IsConst() const override;

   //  Overridden to indicate whether the data is a const pointer.
   //
   virtual bool IsConstPtr() const override;

   //  Returns true if the data's initialization is currently being executed.
   //
   virtual bool IsInitializing() const override { return initing_; }

   //  Overridden to return true if the data is static.
   //
   virtual bool IsStatic() const override { return static_; }

   //  Overridden to determine if the data is unused.
   //
   virtual bool IsUnused() const
      override { return ((reads_ == 0) && (writes_ == 0)); }

   //  Overridden to make const data writeable during initialization.
   //
   virtual StackArg NameToArg(Cxx::Operator op) override;

   //  Overridden to record that the data cannot be const.
   //
   virtual bool SetNonConst() override;

   //  Overridden to shrink containers.
   //
   virtual void Shrink() override;

   //  Overridden to reveal that this is a data item.
   //
   virtual Cxx::ItemType Type() const override { return Cxx::Data; }

   //  Overridden to return the data's full root type.
   //
   virtual std::string TypeString(bool arg) const override;

   //  Overridden to increment the number of times the data was read.
   //
   virtual bool WasRead() override;

   //  Overridden to increment the number of times the data was written.
   //
   virtual bool WasWritten(const StackArg* arg, bool passed) override;
protected:
   //  Creates a data item with SPEC.  Protected because this class is virtual.
   //
   explicit Data(TypeSpecPtr& spec);

   //  Returns true if this is a data declaration.
   //
   bool IsDecl() const { return !defn_; }

   //  Executes the data's initialization expression.  Invokes PushScope if
   //  PUSH is set.  Returns false if no form of initialization occurred.
   //
   bool ExecuteInit(bool push);

   //  Executes the initialization expression EXPR, which appeared in
   //  parentheses.  If the data's type is a class, EXPR is handled as
   //  a constructor call, else it is handled as a single-member brace
   //  initialization.  Returns true unless EXPR is nullptr.
   //
   bool InitByExpr(CxxToken* expr);

   //  Executes the default constructor that initializes the data.
   //  Returns true if a default constructor call was executed.
   //
   bool InitByDefault();

   //  Checks if the data is unused, init-only, or write-only.
   //
   void CheckUsage() const;

   //  Checks if the data could (or cannot, which indicates a compiler
   //  logic error) be const.  If COULD is not set, only "cannot" errors
   //  are logged.
   //
   void CheckConstness(bool could = true) const;

   //  Displays any parenthesized expression that initializes the data.
   //
   void DisplayExpression(std::ostream& stream, const Flags& options) const;

   //  Displays any assignment statement that initializes the data.
   //
   void DisplayAssignment(std::ostream& stream, const Flags& options) const;

   //  Displays read/write statistics.
   //
   void DisplayStats(std::ostream& stream, const Flags& options) const;
private:
   //  Returns the data's declaration.
   //
   const Data* GetDecl() const { return (defn_ ? mate_ : this); }
   Data* GetDecl() { return (defn_ ? mate_ : this); }

   //  Returns the data's definition (if distinct from its declaration),
   //  else its declaration.
   //
   const Data* GetDefn() const;

   //  Returns the name to be used in the initialization statement.
   //
   virtual void GetInitName(QualNamePtr& qualName) const;

   //  Overridden to return the data's type.
   //
   virtual CxxToken* RootType() const override { return spec_.get(); }

   //  Executes the assignment statement that initializes the data.
   //  Returns true if such a statement existed.
   //
   bool InitByAssign();

   //  Set for extern data.
   //
   bool extern_ : 1;

   //  Set for static data.
   //
   bool static_ : 1;

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

   //  The data's type.
   //
   const TypeSpecPtr spec_;

   //  The expression that initializes the data when
   //  the form <TypeSpec> <name>(<Expr>) is used.
   //
   TokenPtr expr_;

   //  The right-hand side of the assignment statement
   //  that initializes the data.
   //
   ExprPtr rhs_;

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
   virtual void Check() const override;

   //  Overridden to display the data declaration and definition.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden to add the item to the current scope.
   //
   virtual bool EnterScope() override;

   //  Overridden to return the item's qualified name.
   //
   virtual QualName* GetQualName() const override { return name_.get(); }

   //  Overriden to support static member data in a template.
   //
   virtual const TemplateParms* GetTemplateParms() const
      override { return parms_.get(); }

   //  Overridden to return the item's name.
   //
   virtual const std::string* Name() const override { return name_->Name(); }

   //  Overridden to return the item's qualified name.
   //
   virtual std::string QualifiedName(bool scopes, bool templates) const
      override { return name_->QualifiedName(scopes, templates); }

   //  Overridden to record usage of the item.
   //
   virtual void RecordUsage() const override { AddUsage(); }

   //  Overriden to support static member data in a template.
   //
   virtual void SetTemplateParms(TemplateParmsPtr& parms) override;

   //  Overridden to shrink containers.
   //
   virtual void Shrink() override;
private:
   //  Overridden to clone the qualified name.
   //
   virtual void GetInitName(QualNamePtr& qualName) const override;

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

   //  Sets the member initialization expression.
   //
   void SetInit(const MemberInit* init) { init_ = init; }

   //  Overridden to support an implicit "this".
   //
   virtual StackArg NameToArg(Cxx::Operator op) override;

   //  Overridden to mark the item as a member of the context class if
   //  it was pushed via "this".
   //
   virtual StackArg MemberToArg(StackArg& via, Cxx::Operator op) override;

   //  Overridden to log warnings associated with the declaration.
   //
   virtual void Check() const override;

   //  Overridden to display the data declaration and definition.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden to invoke EnterBlock on any field width expression.
   //
   virtual void EnterBlock() override;

   //  Overridden to add the item to the current scope.
   //
   virtual bool EnterScope() override;

   //  Overridden to update SYMBOLS with the data's type usage.
   //
   virtual void GetUsages
      (const CodeFile& file, CxxUsageSets& symbols) const override;

   //  Overridden to return the item's name.
   //
   virtual const std::string* Name() const override { return &name_; }

   //  Overridden to promote the member, which currently belongs to
   //  an anonymous union, to CLS, the union's outer scope.
   //
   virtual void Promote
      (Class* cls, Cxx::Access access, bool first, bool last) override;

   //  Overridden to record usage of the item.
   //
   virtual void RecordUsage() const override { AddUsage(); }

   //  Overridden to track usage of the "mutable" attribute.
   //
   virtual void WasMutated(const StackArg* arg) override;

   //  Overridden to shrink containers.
   //
   virtual void Shrink() override;

   //  Overridden to return the item's name.
   //
   virtual std::string Trace() const override {return *Name(); }

   //  Overridden to track usage of the "mutable" attribute.
   //
   virtual bool WasWritten(const StackArg* arg, bool passed) override;
private:
   //  Overridden to check that data members are private.
   //
   virtual void CheckAccessControl() const override;

   //  Checks that static data has an initialization statement.
   //
   void CheckIfInitialized() const;

   //  Checks if mutable data does not need to be mutable.
   //
   void CheckIfMutated() const;

   //  The data item's name.
   //
   std::string name_;

   //  The field width.
   //
   ExprPtr width_;

   //  The member initialization statement.
   //
   const MemberInit* init_;

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
   void SetNext(DataPtr& next);

   //  Sets the first data declaration in a series.
   //
   void SetFirst(const Data* first) { first_ = first; }

   //  Overridden to log warnings associated with the data.
   //
   virtual void Check() const override;

   //  Overridden to display the data declaration and definition.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden to make the item visible as a local.
   //
   virtual void EnterBlock() override;

   //  Overridden to remove the item as a local.
   //
   virtual void ExitBlock() override;

   //  Overridden to update SYMBOLS with the data's type usage.
   //
   virtual void GetUsages
      (const CodeFile& file, CxxUsageSets& symbols) const override;

   //  Overridden to indicate that inline display is supported.
   //
   virtual bool InLine() const override { return true; }

   //  Overridden to return the item's name.
   //
   virtual const std::string* Name() const override { return &name_; }

   //  Overridden to display the data declaration and definition.
   //
   virtual void Print
      (std::ostream& stream, const Flags& options) const override;

   //  Overridden to shrink containers.
   //
   virtual void Shrink() override;

   //  Overridden to return the item's name.
   //
   virtual std::string Trace() const override {return *Name(); }
private:
   //  Invoked by Display on each declaration in a possible series.
   //
   void DisplayItem(std::ostream& stream, const Flags& options) const;

   //  The data item's name.
   //
   std::string name_;

   //  The next declaration in a series.
   //
   DataPtr next_;

   //  The first declaration in a series.
   //
   const Data* first_;
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

   //  Specifies whether the function is tagged as exter.
   //
   void SetExtern(bool extn) { extern_ = extn; }

   //  Specifies whether the function is tagged as inline.
   //
   void SetInline(bool inln) { inline_ = inln; }

   //  Specifies whether the function is tagged as inline.
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

   //  Specifies whether the function is tagged "noexcept".
   //
   void SetNoexcept(bool noex) { noexcept_ = noex; }

   //  Specifies whether the function is tagged "override".
   //
   void SetOverride(bool over) { override_ = over; }

   //  Specifies whether the function is pure virtual.
   //
   void SetPure(bool pure) { pure_ = pure; }

   //  Sets the operator if the function is an operator overload.
   //
   void SetOperator(Cxx::Operator oper);

   //  Marks the function as an inline friend function.
   //
   void SetFriend() { friend_ = true; }

   //  Sets the base class constructor call.
   //
   void SetBaseInit(ExprPtr& init);

   //  Adds a member initialization to a constructor initialization list.
   //
   void AddMemberInit(MemberInitPtr& init);

   //  Sets the starting location (opening brace) of an inline function.
   //
   void SetBracePos(size_t pos) { pos_ = pos; }

   //  Sets the start and end positions for the function's definition.
   //
   void SetDefnRange(size_t begin, size_t end);

   //  Sets the function's implementation, which is immediately executed.
   //
   void SetImpl(BlockPtr& block);

   //  Returns true if the function was explicitly inlined.
   //
   bool IsInline() const { return inline_; }

   //  Returns true if the function was tagged "virtual".
   //
   bool IsVirtual() const { return virtual_; }

   //  Returns true if the function was tagged "explicit".
   //
   bool IsExplicit() const { return explicit_; }

   //  Returns the function's operator (Cxx::NIL_OPERATOR if not an operator).
   //
   Cxx::Operator Operator() const { return name_->Operator(); }

   //  Returns true if the function is an override.  This does not perform any
   //  analysis, but simply relies on the "override" tag, which should always
   //  be used, and which is internally set for a function that omits it.
   //
   bool IsOverride() const { return override_; }

   //  Deletes the single argument "(void)", which simplifies the comparison
   //  of function signatures.
   //
   void DeleteVoidArg();

   //  Returns the first declaration of this function in the class hierarchy
   //  (that is, virtual but not an override).  Returns the function itself
   //  if it is not an override.
   //
   Function* FindRootFunc() const;

   //  Returns true if this is a function template instance.
   //
   bool IsTemplateInstance() const { return tmplt_ != nullptr; }

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

   //  Returns the function's begin and end positions.
   //
   void GetDefnRange(size_t& begin, size_t& end) const;

   //  Decides if this function can be invoked with ARGS, whose TypeString
   //  results appear in argTypes.  Updates MATCH to indicate how well the
   //  arguments and the function match.  Returns the function itself--or,
   //  if it is a function template, the correct instance.  Returns nullptr
   //  if the function does not match ARGS.
   //
   Function* CanInvokeWith
      (StackArgVector& args, stringVector& argTypes, TypeMatch& match) const;

   //  Returns true if this is a constructor that can be invoked implicitly
   //  with the argument THAT, whose type is thatType.  Setting IMPLICIT
   //  considers a constructor even if it is tagged "explicit".
   //
   bool CanConstructFrom(const StackArg& that,
      const std::string& thatType, bool implicit = false) const;

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

   //  Invoked when the function accessed ITEM.
   //
   void ItemAccessed(const CxxNamed* item);

   //  Registers an invocation of the function.  ARGS is a list of resolved
   //  arguments, in the same order as declared by the function.  A read is
   //  noted for each argument, and a write if it was passed as a non-const
   //  pointer or reference.
   //
   void Invoke(StackArgVector* args);

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

   //  Returns true if the function is exempt from invoking Debug::ft.
   //
   bool IsExemptFromTracing() const;

   //  Displays the function's declaration.
   //
   void DisplayDecl(std::ostream& stream, const Flags& options) const;

   //  Overridden to log warnings associated with the function.
   //
   virtual void Check() const override;

   //  Overridden to skip destructors.
   //
   virtual void CheckAccessControl() const override;

   //  Overridden to not log an override for hiding an inherited name.
   //
   virtual void CheckIfHiding() const override;

   //  Overridden to determine if the function is used.
   //
   virtual void CheckIfUsed(Warning warning) const override;

   //  Overridden to display the function.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden to execute the function.
   //
   virtual void EnterBlock() override;

   //  Overridden to add the function to the current scope.
   //
   virtual bool EnterScope() override;

   //  Overridden to return the file that declared the function.
   //
   virtual CodeFile* GetDeclFile() const override;

   //  Overridden to return the file (if any) that defined the function.
   //
   virtual CodeFile* GetDefnFile() const override;

   //  Overridden to return the function itself.
   //
   virtual Function* GetFunction()
      const override { return const_cast< Function* >(this); }

   //  Overridden to return the function's qualified name.
   //
   virtual QualName* GetQualName() const override { return name_.get(); }

   //  Overridden to handle an inline friend function.
   //
   virtual CxxScope* GetScope() const override;

   //  Overridden to return a template if this function
   //  (a) is a function template
   //  (b) is a function template instance
   //  (c) is a function in a class template
   //  (d) is a function in a class template instance
   //
   virtual CxxScope* GetTemplate() const override;

   //  Overriden to support function templates.
   //
   virtual const TemplateParms* GetTemplateParms() const
      override { return parms_.get(); }

   //  Overridden to return the function's return type.
   //
   virtual TypeSpec* GetTypeSpec() const override { return spec_.get(); }

   //  Overridden to update SYMBOLS with the type usage of each of the
   //  function's components.
   //
   virtual void GetUsages
      (const CodeFile& file, CxxUsageSets& symbols) const override;

   //  Overridden to indicate whether the function itself is const.
   //
   virtual bool IsConst() const override { return const_; }

   //  Overridden to look for an implemented function.
   //
   virtual bool IsImplemented() const override { return impl_ != nullptr; }

   //  Overridden to return true if this is an instance of a function template.
   //
   virtual bool IsInTemplateInstance() const override;

   //  Overridden to return true if the function is static.
   //
   virtual bool IsStatic() const override { return static_; }

   //  Overridden to determine if the function is unused.
   //
   virtual bool IsUnused() const override;

   //  Overriden to include VIA as a "this" argument.
   //
   virtual StackArg MemberToArg(StackArg& via, Cxx::Operator op) override;

   //  Overridden to return the function's name.
   //
   virtual const std::string* Name() const override { return name_->Name(); }

   //  Overridden to return the function's qualified name.
   //
   virtual std::string QualifiedName(bool scopes, bool templates) const
      override { return name_->QualifiedName(scopes, templates); }

   //  Overridden to record usage of the function.
   //
   virtual void RecordUsage() const override;

   //  Overriden to support function templates.
   //
   virtual void SetTemplateParms(TemplateParmsPtr& parms) override;

   //  Overridden to shrink containers.
   //
   virtual void Shrink() override;

   //  Overridden to reveal that this is a function.
   //
   virtual Cxx::ItemType Type() const override { return Cxx::Function; }

   //  Overridden to return the function's full root signature.  The
   //  function's name is omitted if ARG is set.
   //
   virtual std::string TypeString(bool arg) const override;

   //  Overridden to track how many times the function was invoked.
   //
   virtual void WasCalled() override;

   //  Overridden to count a read as an invocation.
   //
   virtual bool WasRead() override { ++calls_; return true; }
private:
   //  Returns the function's declaration.
   //
   const Function* GetDecl() const { return (defn_ ? mate_ : this); }
   Function* GetDecl() { return (defn_ ? mate_ : this); }

   //  Adds a "this" argument to the function if required.  This occurs
   //  immediately before executing the function's code (in EnterBlock).
   //
   void AddThisArg();

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

   //  Returns the function's definition (if distinct from its declaration),
   //  else its declaration.
   //
   const Function* GetDefn() const;

   //  Returns true if this function is overridden by CLS or one of its
   //  subclasses.
   //
   bool IsOverriddenAtOrBelow(const Class* cls) const;

   //  Returns true if the function's purpose is probably to delete its default
   //  version (e.g. a private constructor or assignment operator).
   //
   bool IsDeleted() const;

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
   static TypeMatch MatchTemplate
      (const std::string& thisType, const std::string& thatType,
         stringVector& tmpltParms, stringVector& tmpltArgs, bool& argFound);

   //  Instantiates a function template instance according to tmpltArgs, which
   //  contains the TypeString that specializes each template parameter in the
   //  same order that the parameters were defined.  If the function instance
   //  has already been instantiated, it is found and returned.
   //
   Function* InstantiateFunction(stringVector& tmpltArgs) const;

   //  Invoked when InstantiateFunction fails.
   //
   static Function* InstantiateError
      (const std::string& instName, debug32_t offset);

   //  Sets the function template for a template instance.
   //
   void SetTemplate(Function* func) { tmplt_ = func; }

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

   //  Checks a function and logs any warnings that it detects.
   //
   void CheckArgs() const;
   void CheckCtor() const;
   void CheckDtor() const;
   void CheckIfDefined() const;
   void CheckIfPublicVirtual() const;
   void CheckForVirtualDefault() const;
   void CheckIfOverridden() const;
   void CheckIfCouldBeConst() const;
   void CheckMemberUsage() const;

   //  Determines how the function is associated with a template.
   //
   TemplateLocation GetTemplateLocation() const;

   //  Displays the function's definition.
   //
   void DisplayDefn(std::ostream& stream,
      const std::string& prefix, const Flags& options) const;

   //  Displays information about where the function is implemented, how many
   //  many times it was overridden, and how many times it was invoked.
   //
   void DisplayInfo(std::ostream& stream, const Flags& options) const;

   //  The function's name.
   //
   const QualNamePtr name_;

   //  The template parameters for a function template.
   //
   TemplateParmsPtr parms_;

   //  Set for a function tagged as extern.
   //
   bool extern_ : 1;

   //  Set for a function explicitly or implicitly tagged as inline.
   //
   bool inline_ : 1;

   //  Set for a function tagged as constexpr.
   //
   bool constexpr_ : 1;

   //  Set for a static function.
   //
   bool static_ : 1;

   //  Set for a virtual function.
   //
   bool virtual_ : 1;

   //  Set for an explicit constructor.
   //
   bool explicit_ : 1;

   //  Set for a const function.
   //
   bool const_ : 1;

   //  Set for a function tagged "noexcept".
   //
   bool noexcept_ : 1;

   //  Set for a function tagged "override".
   //
   bool override_ : 1;

   //  Set for a pure virtual function.
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

   //  Set if the function used a non-public member in its own class
   //
   bool nonpublic_ : 1;

   //  Set if the function used a non-static member in its own class
   //
   bool nonstatic_ : 1;

   //  How many times the function was invoked.
   //
   size_t calls_ : 15;

   //  Set if this is the definition of a previously declared function.
   //
   bool defn_ : 1;

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

   //  Where the function's definition begins.
   //
   size_t begin_;

   //  Where the function's definition ends.
   //
   size_t end_;

   //  The next function, up the class hierarchy, that this one overrides.
   //
   Function* base_;

   //  The function template of which this is an instance.
   //
   Function* tmplt_;

   //  The code for a function template.
   //
   mutable stringPtr code_;

   //  A function template's instantiations.
   //
   mutable FunctionVector tmplts_;

   //  Direct overrides of the function.
   //
   mutable FunctionVector overs_;
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
   ~FuncSpec() { CxxStats::Decr(CxxStats::FUNC_SPEC); }
private:
   //  The following are overridden to return the function signature.
   //
   virtual Function* GetFuncSpec() const override { return func_.get(); }
   virtual CxxNamed* Referent() const override { return func_.get(); }
   virtual CxxToken* RootType() const override { return func_.get(); }

   //  The following are forwarded to the function.
   //
   virtual void Print
      (std::ostream& stream, const Flags& options) const override;
   virtual void EnteringScope(const CxxScope* scope) override;
   virtual bool IsConst() const override;
   virtual const std::string* Name() const override;
   virtual std::string Trace() const override;
   virtual std::string TypeString(bool arg) const override;
   virtual void Shrink() override;

   //  The following are forwarded to the function's return type.
   //
   virtual void AddArray(ArraySpecPtr& array) override;
   virtual void AdjustPtrs(TagCount count) override;
   virtual TagCount ArrayCount() const override;
   virtual TagCount Arrays() const override;
   virtual std::string AlignTemplateArg(const TypeSpec* thatArg) const override;
   virtual void DisplayArrays(std::ostream& stream) const override;
   virtual void DisplayTags(std::ostream& stream) const override;
   virtual TypeTags GetTags() const override;
   virtual TypeSpec* GetTypeSpec() const override;
   virtual bool HasArrayDefn() const override;
   virtual bool IsConstPtr() const override;
   virtual TagCount PtrCount(bool arrays) const override;
   virtual TagCount Ptrs(bool arrays) const override;
   virtual TagCount RefCount() const override;
   virtual TagCount Refs() const override;
   virtual void RemoveRefs() override;
   virtual StackArg ResultType() const override;
   virtual std::string TypeTagsString(const TypeTags& tags) const override;

   //  The following are forwarded to the function's return type but also
   //  generate a log because they may not be properly supported.
   //
   virtual bool FindReferent() override;
   virtual TypeName* GetTemplateArgs() const override;
   virtual void Instantiating() const override;
   virtual TypeMatch MatchTemplateArg(const TypeSpec* that) const override;
   virtual bool MatchesExactly(const TypeSpec* that) const override;
   virtual TypeMatch MatchTemplate(TypeSpec* that, stringVector& tmpltParms,
      stringVector& tmpltArgs, bool& argFound) const override;

   //  The following are forwarded to the function's return type but also
   //  generate a log because they should not be invoked.
   //
   virtual void Check() const override;
   virtual void EnterArrays() const override;
   virtual void SetArrayPos(int8_t pos) override;
   virtual void SetConst(bool readonly) override;
   virtual void SetConstPtr(bool constptr) override;
   virtual void SetPtrDetached(bool on) override;
   virtual void SetPtrs(TagCount ptrs) override;
   virtual void SetRefDetached(bool on) override;
   virtual void SetRefs(TagCount refs) override;
   virtual void SetReferent(CxxNamed* ref, bool use) override;  //*

   //  The following is not supported.  It generates a log and returns
   //  nullptr.
   //
   virtual TypeSpec* Clone() const override;

   //  The function signature.
   //
   const FunctionPtr func_;
};
}
#endif
