//==============================================================================
//
//  CxxToken.h
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
#ifndef CXXTOKEN_H_INCLUDED
#define CXXTOKEN_H_INCLUDED

#include "LibraryItem.h"
#include <cstddef>
#include <cstdint>
#include <ios>
#include <iosfwd>
#include <ostream>
#include <string>
#include <utility>
#include "CodeTypes.h"
#include "Cxx.h"
#include "CxxExecute.h"
#include "CxxFwd.h"
#include "CxxLocation.h"
#include "LibraryTypes.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace CodeTools
{
//  The base class for all C++ entities created by the parser.
//
class CxxToken : public LibraryItem
{
public:
   //  Virtual to allow subclassing.
   //    Most items are owned in a unique_ptr, so if the owner is deleted, the
   //  item is deleted before the owner's destructor runs.  The item need not
   //  inform its owner of its deletion in this case.  But if the item is being
   //  deleted on its own, the owner needs to be informed.  The two scenarios
   //  are distinguished by invoking Delete (below) in the latter case.
   //    An item can be deleted when the parser backs up or when code is edited.
   //  Destructors are written assuming that items are deleted in the reverse
   //  order of how they were encountered in code.  References to an item must
   //  therefore be deleted first, followed by any distinct definition, and
   //  finally the declaration.  Definitions and declarations erase their
   //  entries in the file or scope where they appear, and declarations also
   //  erase themselves from the symbol table.  An item that refers to another
   //  one erases itself from its referent's cross-reference.
   //
   virtual ~CxxToken();

   //  Deletes the item and informs its owner of its deletion.  The default
   //  version simply deletes the item and must be overridden by an item that
   //  the Editor can delete separately from its owner (e.g. when deleting an
   //  unused item from its class or namespace).
   //
   virtual void Delete();

   //  Sets the file and offset at which this item was found.
   //
   virtual void SetLoc(CodeFile* file, size_t pos) const;
   void SetLoc(CodeFile* file, size_t pos, bool internal) const;

   //  Returns the item's location information.
   //
   const CxxLocation& GetLoc() const { return loc_; }

   //  Invokes SetLoc(Context::File(), pos).
   //
   virtual void SetContext(size_t pos);

   //  Invokes SetLoc(file, pos).  Used when editing compiled code.
   //
   void SetContext(CodeFile* file, size_t pos);

   //  Sets the item's context based on THAT.  Typically used when an item
   //  is created internally (e.g. during template instantiation), in which
   //  case INTERNAL is set.
   //
   virtual void CopyContext(const CxxToken* that, bool internal);

   //  Creates and returns a copy of the item.  Serves as a copy constructor,
   //  which cannot be invoked directly on this class (because it is virtual).
   //  The default version generates a log and returns nullptr.
   //
   virtual CxxToken* Clone() const;

   //  Constructs an argument based on the item's referent.  The default
   //  version generates a log and returns NilStackArg.
   //
   virtual StackArg ResultType() const;

   //  Returns the file in which this item was found.
   //
   CodeFile* GetFile() const { return loc_.GetFile(); }

   //  Returns the offset at which the item was found.
   //
   size_t GetPos() const { return loc_.GetPos(); }

   //  Returns true if the item appeared in internally generated code.
   //
   bool IsInternal() const { return loc_.IsInternal(); }

   //  Sets BEGIN and END to where the item begins and ends, and LEFT to the
   //  position of its opening left brace (if applicable, else string::npos).
   //  If LEFT applies, END will be the position of the matching right brace.
   //  This is invoked when preparing to cut the item's code, so it should
   //  include leading and/or trailing punctuation where appropriate.
   //
   bool GetSpan3(size_t& begin, size_t& left, size_t& end) const;

   //  Used when the LEFT argument for GetSpan3 (above) is not of interest.
   //
   bool GetSpan2(size_t& begin, size_t& end) const;

   //  Returns the item's type.
   //
   virtual Cxx::ItemType Type() const { return Cxx::Undefined; }

   //  Records the scope where the item appeared.
   //
   virtual void SetScope(CxxScope* scope) { }

   //  Returns the scope (namespace, class, or block) where the item is
   //  declared.
   //
   virtual CxxScope* GetScope() const { return nullptr; }

   //  Returns true if the type is a forward declaration: namely, if its
   //  Type() is Cxx::Forward or Cxx::Friend.  Declarations of data and
   //  functions return false, as this only applies to classes.
   //
   virtual bool IsForward() const { return false; }

   //  Returns the item's qualified name member, if any.
   //
   virtual QualName* GetQualName() const { return nullptr; }

   //  Converts a type to a string, expanding typedefs and preserving pointers.
   //  ARG is set if the string will be used to compare argument types.  The
   //  default version generates a log and returns ERROR_STR.
   //
   virtual std::string TypeString(bool arg) const;

   //  Returns the item's type specification.
   //
   virtual TypeSpec* GetTypeSpec() const { return nullptr; }

   //  If the item is a class (and not a pointer or reference to a class),
   //  returns that class.  Returns nullptr otherwise.  The default version
   //  invokes DirectClass on GetTypeSpec.
   //
   virtual Class* DirectClass() const;

   //  Returns true if the item is const.
   //
   virtual bool IsConst() const { return false; }

   //  Returns true if the item is volatile.
   //
   virtual bool IsVolatile() const { return false; }

   //  Returns true if the item's outermost pointer is const.
   //
   virtual bool IsConstPtr() const { return false; }

   //  Returns true if the item's outermost pointer is volatile.
   //
   virtual bool IsVolatilePtr() const { return false; }

   //  Returns true if the item's Nth (0<=n<=MAX_PTRS) pointer is const.
   //
   virtual bool IsConstPtr(size_t n) const { return false; }

   //  Returns true if the item's Nth (0<=n<=MAX_PTRS) pointer is volatile.
   //
   virtual bool IsVolatilePtr(size_t n) const { return false; }

   //  Returns true if the item is static.  Note that, for the purposes
   //  of this function:
   //  o only data and functions can be classified as non-static;
   //  o class membership for non-static data and functions must be
   //    checked separately, using if(item->GetClass() != nullptr).
   //
   virtual bool IsStatic() const { return true; }

   //  Returns true if the item is extern.
   //
   virtual bool IsExtern() const { return false; }

   //  Sets the access control that applies to the item.
   //
   virtual void SetAccess(Cxx::Access access) { }

   //  Returns the access control that applies to the item.
   //
   virtual Cxx::Access GetAccess() const { return Cxx::Public; }

   //  Returns true if the item's type is "auto" and its actual type has
   //  yet to be determined.
   //
   virtual bool IsAuto() const { return false; }

   //  Returns true if the item is indirect (that is, if it is a pointer
   //  or reference).  If ARRAYS is set, an array is considered indirect.
   //
   virtual bool IsIndirect(bool arrays) const { return false; }

   //  Invoked when an object is created on the stack or from the heap.
   //
   virtual void Creating() { }

   //  Invoked to instantiate a class template instance when it is declared as
   //  a member or named in executable code.
   //
   virtual void Instantiate() { }

   //  Returns true if the item is undergoing initialization.
   //
   virtual bool IsInitializing() const { return false; }

   //  Returns the type to assign to an "auto" variable when the item is
   //  the result of an expression.
   //
   virtual CxxToken* AutoType() const { return nullptr; }

   //  Returns the namespace in which the item was declared.  Returns the
   //  item itself if it is a namespace.
   //
   virtual Namespace* GetSpace() const { return nullptr; }

   //  Returns the area (namespace or class) in which the item was declared.
   //  Returns the item itself if it is an area.
   //
   virtual CxxArea* GetArea() const { return nullptr; }

   //  Returns the class in which the item was declared.  Returns the item
   //  itself if it is a class.
   //
   virtual Class* GetClass() const { return nullptr; }

   //  Returns the item's mate.  Returns nullptr unless the item is declared
   //  and defined separately, in which case it returns the other instance.
   //
   virtual CxxScope* GetMate() const { return nullptr; }

   //  Returns the class in which the item was declared.  Returns the outer
   //  class, if any, if the item itself is a class.
   //
   virtual Class* Declarer() const { return GetClass(); }

   //  Returns the template, if any, associated with a class or function.
   //
   virtual CxxScope* GetTemplate() const { return nullptr; }

   //  Returns the template arguments, if any, associated with an item.
   //
   virtual const TemplateArgPtrVector* Args() const { return nullptr; }

   //  Returns the template specification associated with the item, if any.
   //  The default version invokes GetQualName and, if the result is not
   //  nullptr, asks it for its template arguments.
   //
   virtual TypeName* GetTemplatedName() const;

   //  If the item is, or belongs to, a template instance, returns the instance.
   //
   virtual CxxScope* GetTemplateInstance() const;

   //  Returns true if the item is, or belongs to, a template instance.
   //
   bool IsInTemplateInstance() const;

   //  If this item appears in a template instance, returns the template
   //  item that corresponds to ITEM.
   //
   virtual CxxScoped* FindTemplateAnalog(const CxxToken* item) const;

   //  Returns details about how the item can be converted to an integer.
   //  By default, conversion is not possible.
   //
   virtual Numeric GetNumeric() const { return Numeric::Nil; }

   //  Updates TYPES with the types to which the item can be converted.
   //  EXPL is set if explicit conversion is required.
   //
   virtual void GetConvertibleTypes(StackArgVector& types, bool expl) { }

   //  Returns what the item refers to.
   //
   virtual CxxScoped* Referent() const { return nullptr; }

   //  Returns true if all components in the item have referents.  The default
   //  version returns false if Referent() returns nullptr.
   //
   virtual bool VerifyReferents() const;

   //  Invoked before adding the item to the current scope (Context::Scope()).
   //  Returning false indicates that
   //  o for a C++ item, that it is a definition of a previous declaration;
   //  o for a preprocessor directive, that the code that follows it should
   //    not be compiled.
   //
   virtual bool EnterScope() { return true; }

   //  Invoked when the item is reached when compiling the code block (if any)
   //  in which the item appears.  The default version generates a log.
   //
   virtual void EnterBlock();

   //  If the item is located in a code block, this is invoked when compilation
   //  of the block ends, which corresponds the block going out of scope.
   //
   virtual void ExitBlock() const { }

   //  Returns true if a unary operator can be appended after this item.
   //  Returning false converts an (ambiguous) unary operator to a binary
   //  operator that takes this item as its first argument.
   //
   virtual bool AppendUnary() { return false; }

   //  Returns the last item created during parsing.
   //
   virtual CxxToken* Back() { return this; }

   //  Invoked when the item is read.  Returns true to record the read in the
   //  compilation trace.  The default version returns false.
   //
   virtual bool WasRead() { return false; }

   //  Invoked when the item is modified.  Returns true to record a write in
   //  the compilation trace.  The default version generates a log and returns
   //  false, and must therefore be overridden by items that can be modified.
   //  ARG is the stack variable through which the item was modified.  The item
   //  itself is usually arg.item, but ARG is nullptr for a function's "this"
   //  Argument, and arg.item is a data member's outer class when that entire
   //  class was the target of a block copy.  DIRECT is set when the item was
   //  (or could be) modified, and INDIRECT is set when the item is a pointer
   //  and what it refers to was (or could have been) modified.
   //
   virtual bool WasWritten(const StackArg* arg, bool direct, bool indirect);

   //  Invoked when it is determined that an item cannot be const.  Returning
   //  false indicates that the item is actually const, which generates a log.
   //
   virtual bool SetNonConst() { return true; }

   //  Invoked instead of SetNonConst when ARG is marked mutable and cannot
   //  be const.  The non-const item can either be ARG.item or ARG.via_.
   //
   virtual void WasMutated(const StackArg* arg) { }

   //  Records that the item was used when compiling code in the context file.
   //  This is used to record usages (for #include purposes) that are difficult
   //  to detect simply on the basis of symbol usage.
   //
   virtual void RecordUsage() { }

   //  Invokes CxxScoped.UpdateReference on items that this one references.
   //
   virtual void UpdateXref(bool insert) { }

   //  Updates SYMBOLS with how this item (in FILE) used other types.  See
   //  CxxUsageSets for how various uses of a type are distinguished.
   //
   virtual void GetUsages(const CodeFile& file, CxxUsageSets& symbols) { }

   //  Returns the item, if any, that begins at POS in the item's file; the
   //  item therefore sees if POS matches itself or its subtending items.
   //    Objects in the class Expression cannot be found with this function
   //  because they begin at the same position as their first component, which
   //  would then be masked.  Masking does occur, however, with QualName (which
   //  masks its first TypeName), Expr (which can also mask a TypeName) and
   //  DataSpec (which masks its QualName).  Some items can only be found at
   //  the punctuation associated with them:
   //  o ArraySpec: [
   //  o Block: {
   //  o BraceInit: {
   //  o Operation: the operator
   //  o Precedence: (
   //  o TemplateParms: <
   //
   virtual CxxToken* PosToItem(size_t pos) const;

   //  Searches this item for ITEM.  Returns true if it was found.  Increments
   //  N each time that an item with the same name was encountered.
   //
   virtual bool LocateItem(const CxxToken* item, size_t& n)
      const { return false; }

   //  Searches this item for the Nth occurrence of an item that matches NAME.
   //  Returns that item if found, else returns nullptr.
   //
   virtual CxxScoped* FindNthItem(const std::string& name, size_t& n)
      const { return nullptr; }

   //  Logs code warnings associated with the item.
   //
   virtual void Check() const { }

   //  Returns a string that describes the item during a compilation trace.
   //
   virtual std::string Trace() const { return NodeBase::EMPTY_STR; }

   //  Returns the item's cross-reference (the items that reference it).
   //
   virtual CxxTokenSet* Xref() const { return nullptr; }

   //  Returns true if the item can be displayed in-line.
   //
   virtual bool InLine() const { return true; }

   //  Displays the item in-line.  Should not add leading or trailing spaces
   //  or an endline.  The output should be compilable.
   //
   virtual void Print
      (std::ostream& stream, const NodeBase::Flags& options) const;

   //  Invokes Referent to find what the item refers to.  If the result is a
   //  forward or friend declaration, *its* referent is found in an attempt to
   //  locate the actual definition.
   //
   CxxScoped* ReferentDefn() const;

   //  Returns the item's underlying type.  It uses RootType (below) to follow
   //  a data item or function argument to its underlying type, through typedef
   //  chains and forward and friend declarations.  The result, however, can
   //  still be a forward or friend declaration that has not yet been resolved
   //  to a class definition.
   //
   CxxToken* Root() const;

   //  Returns true if the item is a pointer.  ARRAYS is set if an array
   //  should be considered a pointer.
   //
   bool IsPointer(bool arrays) const;

   //  Returns true if the item's type is POD (plain ordinary data).
   //
   virtual bool IsPOD() const { return GetNumeric().IsPOD(); }

   //  Logs WARNING at the position where this item is located.  ITEM,
   //  OFFSET, and INFO are specific to WARNING.  If ITEM is nullptr,
   //  it is set to this item.
   //
   void Log(Warning warning, const CxxToken* item = nullptr,
      NodeBase::word offset = 0,
      const std::string& info = NodeBase::EMPTY_STR) const;

   //  Invoked during editing when ACTION has occurred in the item's file.
   //  o Erased: COUNT characters erased at BEGIN
   //  o Inserted: COUNT characters inserted at BEGIN
   //  o Pasted: COUNT characters originally at FROM inserted at BEGIN
   //
   virtual void UpdatePos(EditorAction action,
      size_t begin, size_t count, size_t from) const;

   //  Returns a string that identifies the item's source code location.
   //
   std::string strLocation() const;

   //  Outputs PREFIX, invokes Print(stream, options) above, and inserts an
   //  endline.  This is the appropriate implementation for items that can be
   //  displayed inline or separately.  See CodeDisplayOptions for OPTIONS.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;
protected:
   //  Protected because this class is virtual.
   //
   CxxToken();

   //  Copy constructor.
   //
   CxxToken(const CxxToken& that);

   //  Copy operator.
   //
   CxxToken& operator=(const CxxToken& that);

   //  Overridden by subclasses to support GetSpan2 and GetSpan3 (above).  It
   //  should not be invoked directly, because GetSpan2 and GetSpan3 set all
   //  arguments to string::npos, which is useful for error checking and when
   //  LEFT does not apply.  The default version invokes GetSpanFailure.
   //
   virtual bool GetSpan(size_t& begin, size_t& left, size_t& end) const;

   //  Implements GetSpan for a simple item that ends at a semicolon.
   //
   bool GetSemiSpan(size_t& begin, size_t& end) const;

   //  Marks the item as having been generated internally.
   //
   void SetInternal(bool internal) const { loc_.SetInternal(internal); }
private:
   //  Returns the item's underlying type.  Can return nullptr, whereas Root
   //  (above) returns the item that returned nullptr.
   //
   virtual CxxToken* RootType() const { return const_cast< CxxToken* >(this); }

   //  The location where the item appeared.
   //
   mutable CxxLocation loc_;
};

//  For sorting items by GetFile() and GetPos().
//
bool IsSortedByFilePos(const CxxToken* item1, const CxxToken* item2);

//  For sorting items by GetPos() when all are in the same file.
//
bool IsSortedByPos(const CxxToken* item1, const CxxToken* item2);

//------------------------------------------------------------------------------
//
//  A literal.
//
class Literal : public CxxToken
{
public:
   //  Virtual to allow subclassing.
   //
   virtual ~Literal() = default;

   //  Overridden to set the type for an "auto" variable.
   //
   CxxToken* AutoType() const override;

   //  Overridden to push the literal onto the stack.
   //
   void EnterBlock() override;

   //  Overridden to return the literal as it appeared in the source code.
   //
   const std::string& Name() const override;

   //  Overridden to return the output of the literal's Print function.
   //
   std::string Trace() const override;

   //  Overridden to treat a literal as its underlying type.
   //
   Cxx::ItemType Type() const override;
protected:
   //  Protected because this class is virtual.
   //
   Literal();
private:
   //  Overridden to return the literal's underlying type.
   //
   CxxToken* RootType() const override;
};

//------------------------------------------------------------------------------
//
//  An integer literal.
//
class IntLiteral : public Literal
{
public:
   //  Bases for an integer literal.
   //
   enum Radix : unsigned int
   {
      DEC,  // no prefix
      HEX,  // "0x" prefix
      OCT   // "0" prefix
   };

   //  Suffixes that specify an integer literal's size.
   //
   enum Size : unsigned int
   {
      SIZE_I,  // no suffix
      SIZE_L,  // "L" suffix
      SIZE_LL  // "LL" suffix
   };

   //  Tags for an integer literal.
   //
   struct Tags
   {
      const Radix radix_ : 8;
      const bool unsigned_ : 8;  // "U" suffix
      const Size size_ : 8;

      Tags(Radix r, bool u, Size s) : radix_(r), unsigned_(u), size_(s) { }
      ~Tags() = default;
      Tags(const Tags& that) = default;
      Tags& operator=(const Tags& that) = delete;
   };

   IntLiteral(int64_t num, const Tags& tags) : num_(num), tags_(tags) { }
   ~IntLiteral() = default;
   CxxToken* Clone() const override;
   void Print
      (std::ostream& stream, const NodeBase::Flags& options) const override;
   CxxScoped* Referent() const override;
   std::string TypeString(bool arg) const override;
private:
   Numeric BaseNumeric() const;
   Numeric GetNumeric() const override;
   const int64_t num_;
   const Tags tags_;
};

//------------------------------------------------------------------------------
//
//  A floating point literal.
//
class FloatLiteral : public Literal
{
public:
   //  Suffixes that specify a floating point literal's size.
   //
   enum Size : unsigned int
   {
      SIZE_D,  // no suffix (double)
      SIZE_F,  // "F" suffix (float)
      SIZE_L   // "L" suffix (long double)
   };

   //  Tags for an floating point literal.
   //
   struct Tags
   {
      bool exp_ : 8;  // included an exponent
      Size size_ : 8;

      Tags(bool e, Size s) : exp_(e), size_(s) { }
      ~Tags() = default;
      Tags(const Tags& that) = default;
      Tags& operator=(const Tags& that) = default;
   };

   FloatLiteral(long double num, const Tags& tags) : num_(num), tags_(tags) { }
   ~FloatLiteral() = default;
   CxxToken* Clone() const override;
   void Print
      (std::ostream& stream, const NodeBase::Flags& options) const override;
   CxxScoped* Referent() const override;
   std::string TypeString(bool arg) const override;
private:
   Numeric GetNumeric() const override;
   const long double num_;
   const Tags tags_;
};

//------------------------------------------------------------------------------
//
//  A bool literal ("true" or "false").
//
class BoolLiteral : public Literal
{
public:
   explicit BoolLiteral(bool b) : b_(b) { }
   ~BoolLiteral() = default;
   CxxToken* Clone() const override;
   void Print(std::ostream& stream, const NodeBase::Flags& options) const
      override { stream << std::boolalpha << b_; }
   CxxScoped* Referent() const override;
   std::string TypeString(bool arg) const override { return BOOL_STR; }
private:
   Numeric GetNumeric() const override { return Numeric::Bool; }
   const bool b_;
};

//------------------------------------------------------------------------------
//
//  Base class for string literals.
//
class StringLiteral : public Literal
{
public:
   //  Virtual to allow subclassing.
   //
   virtual ~StringLiteral() = default;

   //  This allows a string literal to be assembled one character (C) at
   //  a time without knowing the actual type of the literal's class.
   //
   virtual void PushBack(uint32_t c) = 0;

   //  Overridden to reveal that this is a string literal.
   //
   Cxx::ItemType Type() const override { return Cxx::StringLiteral; }
protected:
   //  Protected because this class is virtual.
   //
   StringLiteral() = default;
};

//------------------------------------------------------------------------------
//
//  For nullptr.
//
class NullPtr : public Literal
{
public:
   NullPtr() = default;
   ~NullPtr() = default;
   bool IsConstPtr() const override { return true; }
   bool IsConstPtr(size_t n) const override { return true; }
   void Print(std::ostream& stream, const NodeBase::Flags& options) const
      override { stream << NULLPTR_STR; }
   CxxScoped* Referent() const override;
   std::string TypeString(bool arg) const override { return NULLPTR_T_STR; }
private:
   Numeric GetNumeric() const override { return Numeric::Pointer; }
};

//------------------------------------------------------------------------------
//
//  An operator.
//
class Operation : public CxxToken
{
public:
   //  Creates a token when OP is encountered.
   //
   explicit Operation(Cxx::Operator op);

   //  Not subclassed.
   //
   ~Operation();

   //  Deleted to prohibit copying.
   //
   Operation(const Operation& that) = delete;

   //  Deleted to prohibit copy assignment.
   //
   Operation& operator=(const Operation& that) = delete;

   //  Adds ARG as one of the operator's arguments.  PREFIXED is set if the
   //  argument appears before the operator.
   //
   void AddArg(TokenPtr& arg, bool prefixed);

   //  Returns the current number of arguments found for the operator.
   //
   size_t ArgsSize() const { return args_.size(); }

   //  Returns the first argument to the operation.
   //
   CxxToken* FrontArg() const { return args_.front().get(); }

   //  An ambiguous operator token (* & + -) is initially assumed to be unary.
   //  If this proves to be incorrect, this function switches the operator to
   //  its binary interpretation.
   //
   bool MakeBinary();

   //  Invoked when a unary operator appears after the start of an expression.
   //  The previous operator should elide forward, taking the result of the
   //  unary operator as its next argument.  Returns false on an error, which
   //  probably means that the previous operator already has enough arguments.
   //
   bool ElideForward();

   //  Returns the operator.
   //
   Cxx::Operator Op() const { return op_; }

   //  Changes the operator.
   //
   void SetOp(Cxx::Operator op) { op_ = op; }

   //  Sets the flag for the function call to operator new.
   //
   void SetNew() { fcnew_ = true; }

   //  Compiles the operation.  Obtains its arguments from the stack.
   //
   void Execute() const;

   //  Invoked when a unary operator is encountered.  This operator returns
   //  true if it will elide forward to the unary, and false if the new
   //  operator must actually be binary.
   //
   bool AppendUnary() override;

   //  Overridden to return the operator, when it can accept more arguments,
   //  or the last argument, when no more arguments can be accepted.
   //
   CxxToken* Back() override;

   //  Overridden to log warnings associated with the operator.
   //
   void Check() const override;

   //  Overridden to push this operator and its arguments onto the stack.
   //
   void EnterBlock() override;

   //  Overridden to update SYMBOLS with each argument's type usage.
   //
   void GetUsages(const CodeFile& file, CxxUsageSets& symbols) override;

   //  Overridden to find the item located at POS.
   //
   CxxToken* PosToItem(size_t pos) const override;

   //  Overridden to display the operator and its arguments.
   //
   void Print
      (std::ostream& stream, const NodeBase::Flags& options) const override;

   //  Overridden to return the operator's symbol.
   //
   std::string Trace() const override;

   //  Overridden to reveal that this is an operation.
   //
   Cxx::ItemType Type() const override { return Cxx::Operation; }

   //  Overridden to update the operations's location.
   //
   void UpdatePos(EditorAction action,
      size_t begin, size_t count, size_t from) const override;

   //  Overridden to add each argument's components to cross-references.
   //
   void UpdateXref(bool insert) override;
private:
   //  Returns the number of arguments that the operator can still accept.
   //  Returns SIZE_MAX if the operator takes a variable number of arguments
   //  and has the minimum number required.
   //
   size_t ArgCapacity() const;

   //  Pushes the operator onto the stack after executing any operators
   //  that are already on the stack and that have priority.
   //
   void Push() const;

   //  Pushes the operation's arguments onto the stack.
   //
   void PushArgs() const;

   //  If ARG1's root item is a class, pushes the class member specified
   //  by ARG2 onto the stack.
   //
   void PushMember(StackArg& arg1, const StackArg& arg2) const;

   //  Handles the invocation of a function call.
   //
   void ExecuteCall() const;

   //  Finds the version of operator new, new[], delete, or delete[] to
   //  invoke for ARG.  DEL is set if looking for delete/delete[].  POD
   //  is updated to true when ARG is not a class, and so class overrides
   //  of the operators were not considered.  Returns nullptr on failure.
   //
   Function* FindNewOrDelete(const StackArg& arg, bool del, bool& pod) const;

   //  Handles the invocation of operator new.
   //
   void ExecuteNew() const;

   //  Handles the invocation of operator delete on ARG.
   //
   void ExecuteDelete(const StackArg& arg) const;

   //  If ARG1 overloads the operator, this invokes the overload function
   //  and returns T, else it returns false.  If the operator is binary,
   //  ARG2 is its other argument.  NAME is the function name.
   //
   bool ExecuteOverload
      (const std::string& name, StackArg& arg1, const StackArg* arg2) const;

   //  If the operator is overloaded, returns true after handling it.
   //  The overloads are for unary and binary operators, respectively.
   //
   bool IsOverloaded(StackArg& arg) const;
   bool IsOverloaded(StackArg& arg1, StackArg& arg2) const;

   //  Pushes the result of applying the operator to LHS and RHS.
   //
   void PushResult(StackArg& lhs, StackArg& rhs) const;

   //  Generates any log that applies to a cast operation from inArg to outArg.
   //
   void CheckCast(const StackArg& inArg, const StackArg& outArg) const;

   //  Generates a log when a bitwise operator is used on a boolean.
   //
   void CheckBitwiseOp(const StackArg& arg1, const StackArg& arg2) const;

   //  Displays operator new or operator new[].
   //
   void DisplayNew(std::ostream& stream) const;

   //  Displays the argument at INDEX.  Displays ERROR_STR if the argument
   //  does not exist.
   //
   void DisplayArg(std::ostream& stream, size_t index) const;

   //  The operator.
   //
   Cxx::Operator op_ : 8;

   //  Set for a function call associated with operator new.
   //
   bool fcnew_;

   //  The overload that implemented the operator, if any.  Recorded for
   //  symbol usage purposes.
   //
   mutable Function* overload_;

   //  The operator's arguments.
   //
   TokenPtrVector args_;
};

//------------------------------------------------------------------------------
//
//  An expression.  It is rather general and can appear, for example,
//  o on the right of an assignment operator
//  o within parentheses, brackets, or a brace initialization list
//  o as an argument to a function call
//
class Expression : public CxxToken
{
public:
   //  Creates an expression bounded by END within the source code.  END is
   //  the position of a delimiter, typically one of the following: )]},:;
   //  If FORCE is set, evaluation of the argument/operator stack is forced
   //  at the end of the expression.
   //
   Expression(size_t end, bool force);

   //  Not subclassed.
   //
   ~Expression() = default;

   //  Adds ITEM to the expression.
   //
   bool AddItem(TokenPtr& item);

   //  Returns the boundary for parsing the expression.
   //
   size_t EndPos() const { return end_; }

   //  Returns true if the expression is empty.
   //
   bool Empty() const { return items_.empty(); }

   //  Pushes StartOfExpr_ onto the operator stack.  It marks the start
   //  of an expression, which must be fully evaluated before operators
   //  already on the stack can be popped and executed.
   //
   static void Start();

   //  Overridden to return the last item in the expression.
   //
   CxxToken* Back() override;

   //  Overridden to log warnings associated with the expression.
   //
   void Check() const override;

   //  Overridden to return a copy of the expression.
   //
   CxxToken* Clone() const override;

   //  Overridden to invoke Context::Execute after invoking EnterBlock on
   //  each token in items_.
   //
   void EnterBlock() override;

   //  Overridden to update SYMBOLS with each token's type usage.
   //
   void GetUsages(const CodeFile& file, CxxUsageSets& symbols) override;

   //  Overridden to return the expression as it appeared in the source code.
   //
   const std::string& Name() const override;

   //  Overridden to find the item located at POS.
   //
   CxxToken* PosToItem(size_t pos) const override;

   //  Overridden to display the expression.
   //
   void Print
      (std::ostream& stream, const NodeBase::Flags& options) const override;

   //  Overridden to return the expression's type. This is currently
   //  supported only for an expression that contains a single literal.
   //
   StackArg ResultType() const override;

   //  Overridden to display the expression.
   //
   std::string Trace() const override;

   //  Overridden to update the expression's location.
   //
   void UpdatePos(EditorAction action,
      size_t begin, size_t count, size_t from) const override;

   //  Overridden to add each token's components to cross-references.
   //
   void UpdateXref(bool insert) override;
private:
   //  Adds ITEM to the expression when it is known to be a unary operator.
   //
   bool AddUnaryOp(TokenPtr& item);

   //  Adds ITEM to the expression when it is known to be a binary operator.
   //
   bool AddBinaryOp(TokenPtr& item);

   //  Adds ITEM to the expression when it is known to be a n-ary operator.
   //
   bool AddVariableOp(TokenPtr& item);

   //  The tokens in the expression.
   //
   TokenPtrVector items_;

   //  Where the expression ends.  The character at this position is *not*
   //  part of the expression.
   //
   const size_t end_;

   //  Set if the evaluation of the expression should be forced at end_.
   //
   const bool force_;
};

//------------------------------------------------------------------------------
//
//  The size of an array.
//
class ArraySpec : public CxxToken
{
public:
   //  Creates an array whose size is specified by EXPR.
   //
   explicit ArraySpec(ExprPtr& expr);

   //  Not subclassed.
   //
   ~ArraySpec() = default;

   //  Overridden to log warnings associated with expr_.
   //
   void Check() const override;

   //  Overridden to invoke EnterBlock on expr_.
   //
   void EnterBlock() override;

   //  Overridden to update SYMBOLS with the specification's type usage.
   //
   void GetUsages(const CodeFile& file, CxxUsageSets& symbols) override;

   //  Overridden to find the item located at POS.
   //
   CxxToken* PosToItem(size_t pos) const override;

   //  Overridden to display the array's size within brackets.
   //
   void Print
      (std::ostream& stream, const NodeBase::Flags& options) const override;

   //  Overridden to return "[]" if ARG is false and "*" if it is true.
   //
   std::string TypeString(bool arg) const override;

   //  Overridden to update the specification's location.
   //
   void UpdatePos(EditorAction action,
      size_t begin, size_t count, size_t from) const override;

   //  Overridden to add the specification's components to cross-references.
   //
   void UpdateXref(bool insert) override;
private:
   //  The expression that specifies the array's size.
   //
   const ExprPtr expr_;
};

//------------------------------------------------------------------------------
//
//  An argument created for an operator to indicate that this argument will
//  come from the result of the previous or next operation.
//
class Elision : public CxxToken
{
public:
   Elision() = default;
   ~Elision() = default;
   void EnterBlock() override { }
   CxxToken* PosToItem(size_t pos) const override { return nullptr; }
   void Print
      (std::ostream& stream, const NodeBase::Flags& options) const override { }
   Cxx::ItemType Type() const override { return Cxx::Elision; }
};

//------------------------------------------------------------------------------
//
//  Created for an expression that is enclosed in parentheses.
//
class Precedence : public CxxToken
{
public:
   explicit Precedence(ExprPtr& expr) : expr_(std::move(expr)) { }
   ~Precedence() = default;
   void Check() const override;
   void EnterBlock() override;
   void GetUsages(const CodeFile& file, CxxUsageSets& symbols) override;
   CxxToken* PosToItem(size_t pos) const override;
   void Print
      (std::ostream& stream, const NodeBase::Flags& options) const override;
   void UpdatePos(EditorAction action,
      size_t begin, size_t count, size_t from) const override;
   void UpdateXref(bool insert) override;
private:
   const ExprPtr expr_;
};

//------------------------------------------------------------------------------
//
//  A brace initialization list.  This is a series of comma-delimited
//  expressions that initialize a class or array.
//
class BraceInit : public CxxToken
{
public:
   BraceInit();
   ~BraceInit() = default;
   void AddItem(TokenPtr& item) { items_.push_back(std::move(item)); }
   void Check() const override;
   void EnterBlock() override;
   void GetUsages(const CodeFile& file, CxxUsageSets& symbols) override;
   CxxToken* PosToItem(size_t pos) const override;
   void Print
      (std::ostream& stream, const NodeBase::Flags& options) const override;
   void UpdatePos(EditorAction action,
      size_t begin, size_t count, size_t from) const override;
   void UpdateXref(bool insert) override;
private:
   TokenPtrVector items_;
};

//------------------------------------------------------------------------------
//
//  An alignment directive ("alignas" keyword), which can be either
//  an expression (ExprPtr) or type specification (TypeSpecPtr).
//
class AlignAs : public CxxToken
{
public:
   explicit AlignAs(TokenPtr& token);
   ~AlignAs() = default;
   void Check() const override;
   void EnterBlock() override;
   void GetUsages(const CodeFile& file, CxxUsageSets& symbols) override;
   CxxToken* PosToItem(size_t pos) const override;
   void Print
      (std::ostream& stream, const NodeBase::Flags& options) const override;
   void UpdatePos(EditorAction action,
      size_t begin, size_t count, size_t from) const override;
   void UpdateXref(bool insert) override;
private:
   const TokenPtr token_;
};

//------------------------------------------------------------------------------
//
//  Inline assembly code ("asm" keyword).
//
class Asm : public CxxToken
{
public:
   explicit Asm(ExprPtr& code);
   ~Asm() = default;
   void EnterBlock() override { }
   bool EnterScope() override;
   void Print
      (std::ostream& stream, const NodeBase::Flags& options) const override;
   void UpdatePos(EditorAction action,
      size_t begin, size_t count, size_t from) const override;
private:
   bool GetSpan(size_t& begin, size_t& left, size_t& end) const override;

   const ExprPtr code_;
};

//------------------------------------------------------------------------------
//
//  A compile-time assertion ("static_assert" keyword), which contains
//  a boolean expression followed by a string literal.
//
class StaticAssert : public CxxToken
{
public:
   StaticAssert(ExprPtr& expr, ExprPtr& message);
   ~StaticAssert() = default;
   void Check() const override;
   void EnterBlock() override;
   bool EnterScope() override;
   void GetUsages(const CodeFile& file, CxxUsageSets& symbols) override;
   CxxToken* PosToItem(size_t pos) const override;
   void Print
      (std::ostream& stream, const NodeBase::Flags& options) const override;
   void UpdatePos(EditorAction action,
      size_t begin, size_t count, size_t from) const override;
   void UpdateXref(bool insert) override;
private:
   bool GetSpan(size_t& begin, size_t& left, size_t& end) const override;

   const ExprPtr expr_;
   const ExprPtr message_;
};
}
#endif
