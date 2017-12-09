//==============================================================================
//
//  CxxToken.h
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
#ifndef CXXTOKEN_H_INCLUDED
#define CXXTOKEN_H_INCLUDED

#include "Base.h"
#include <cstddef>
#include <cstdint>
#include <ios>
#include <iosfwd>
#include <ostream>
#include <string>
#include <utility>
#include "CodeTypes.h"
#include "Cxx.h"
#include "CxxFwd.h"
#include "SysTypes.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace CodeTools
{
//  For assembling the symbols used by a file.
//
struct CxxUsageSets
{
   CxxNamedSet bases;      // types used as base class
   CxxNamedSet directs;    // types used directly
   CxxNamedSet indirects;  // types named in a pointer or reference
   CxxNamedSet forwards;   // types resolved via a forward declaration
   CxxNamedSet friends;    // types resolved via a friend declaration
   CxxNamedSet users;      // names resolved via a using statement

   //  Adds ITEM to the specified set (AddForward adds ITEM to FRIENDS if
   //  it is a friend declaration).  These functions exist so that a debug
   //  breakpoint can be set within them to find the origin of an item.
   //
   void AddBase(const CxxNamed* item);
   void AddDirect(const CxxNamed* item);
   void AddIndirect(const CxxNamed* item);
   void AddForward(const CxxNamed* item);
   void AddUser(const CxxNamed* item);
};

//------------------------------------------------------------------------------
//
//  The base class for all C++ entities created by the parser.
//
class CxxToken : public Base
{
public:
   //  Virtual to allow subclassing.
   //
   virtual ~CxxToken();

   //  Returns the item's type.
   //
   virtual Cxx::ItemType Type() const { return Cxx::Undefined; }

   //  Returns the item's name.  The default version generates a log and
   //  returns nullptr.  All subclasses of CxxNamed have names, although
   //  the name may be an empty string (e.g. for an unnamed argument).
   //
   virtual const std::string* Name() const;

   //  Returns the item's qualified name member, if any.
   //
   virtual QualName* GetQualName() const { return nullptr; }

   //  Converts a type to a string, expanding typedefs and preserving pointers.
   //  ARG is set if the string will be used to compare argument types.
   //
   virtual std::string TypeString(bool arg) const { return ERROR_STR; }

   //  Returns the item's type specification.
   //
   virtual TypeSpec* GetTypeSpec() const { return nullptr; }

   //  Returns true if the item is const.
   //
   virtual bool IsConst() const { return false; }

   //  Returns true if the item is a const pointer.
   //
   virtual bool IsConstPtr() const { return false; }

   //  Returns true if the item's type is "auto" and its actual type has
   //  yet to be determined.
   //
   virtual bool IsAuto() const { return false; }

   //  Returns true if the item is indirect (that is, if it is a pointer
   //  or reference).
   //
   virtual bool IsIndirect() const { return false; }

   //  Returns true if the item is undergoing initialization.
   //
   virtual bool IsInitializing() const { return false; }

   //  Returns true if the item is a pointer.
   //
   bool IsPointer() const { return (GetNumeric().Type() == Numeric::PTR); }

   //  Returns the type to assign to an "auto" variable when the item is
   //  the result of an expression.
   //
   virtual CxxToken* AutoType() const { return nullptr; }

   //  Returns the namespace in which the item was declared.  Returns the
   //  item itself if it is a namespace.
   //
   virtual Namespace* GetSpace() const { return nullptr; }

   //  Returns the class in which the item was declared.  Returns the item
   //  itself if it is a class.
   //
   virtual Class* GetClass() const { return nullptr; }

   //  Returns the class in which the item was declared.  Returns the outer
   //  class, if any, if the item itself is a class.
   //
   virtual Class* Declarer() const { return GetClass(); }

   //  Returns the template specification associated with the item, if any.
   //  The default implementation invokes GetQualName and, if the result is
   //  not nullptr, asks it for its template arguments.
   //
   virtual TypeName* GetTemplateArgs() const;

   //  Returns details about how the item can be converted to an integer.
   //  By default, conversion is not possible.
   //
   virtual Numeric GetNumeric() const { return Numeric::Nil; }

   //  Updates TYPES with the types to which the item can be converted.
   //
   virtual void GetConvertibleTypes(StackArgVector& types) { }

   //  Returns what the item refers to.  The default version generates a
   //  log and returns nullptr.
   //
   virtual CxxNamed* Referent() const;

   //  If the item is located in a code block, this is invoked when analysis
   //  of the block begins, which corresponds to the block coming into scope.
   //  The default version generates a log.
   //
   virtual void EnterBlock();

   //  If the item is located in a code block, this is invoked when analysis
   //  of the block ends, which corresponds the block going out of scope.
   //
   virtual void ExitBlock() { }

   //  Returns true if a unary operator can be appended after this item.
   //  Returning false converts an (ambiguous) unary operator to a binary
   //  operator that takes this item as its first argument.
   //
   virtual bool AppendUnary() { return false; }

   //  Returns the last item created during parsing.
   //
   virtual CxxToken* Back() { return this; }

   //  Invoked when the item is read.  Returns true to record the read in the
   //  execution trace.  The default version returns false.
   //
   virtual bool WasRead() { return false; }

   //  Invoked when the item is modified.  Returns true to record a write in
   //  the execution trace.  The default version generates a log and returns
   //  false, and must therefore be overridden by items that can be modified.
   //  ARG is the stack variable through which the item was modified.  The item
   //  itself is usually arg.item, but ARG is nullptr for a function's "this"
   //  Argument, and arg.item is a data member's outer class when that entire
   //  class was the target of a block copy.  PASSED is set when the item was
   //  passed to a non-const reference or pointer instead of definitely being
   //  modified.
   //
   virtual bool WasWritten(const StackArg* arg, bool passed);

   //  Invoked when it is determined that an item cannot be const.  Returning
   //  false indicates that the item is actually const, which generates a log.
   //
   virtual bool SetNonConst() { return true; }

   //  Invoked instead of SetNonConst when ARG is marked mutable and cannot
   //  be const.  The non-const item can either be ARG.item or ARG.via_.
   //
   virtual void WasMutated(const StackArg* arg) { }

   //  Records that the item was used when executing code in the context file.
   //  This is used to record usages (for #include purposes) that are difficult
   //  to detect simply on the basis of symbol usage.
   //
   virtual void RecordUsage() const { }

   //  Updates SYMBOLS with how this item used other types within FILE.  See
   //  UsageType for a list of how various uses of a type are distinguished.
   //
   virtual void GetUsages(const CodeFile& file, CxxUsageSets& symbols) const { }

   //  Logs code warnings associated with the item.
   //
   virtual void Check() const { }

   //  Returns a string that describes the item during an execution trace.
   //
   virtual std::string Trace() const { return EMPTY_STR; }

   //  Returns true if the item can be displayed in-line.
   //
   virtual bool InLine() const { return true; }

   //  Displays the item in-line.  Should not add leading or trailing spaces
   //  or an endline.  The output should be compilable.
   //
   virtual void Print(std::ostream& stream) const;

   //  Shrinks the item's containers (e.g. strings and vectors) to the minimum
   //  size required for their current contents.
   //
   virtual void Shrink() { }

   //  Returns the item's underlying type.  It uses RootType (below) to follow
   //  a data item or function argument to its underlying type, through typedef
   //  chains and forward and friend declarations.  The result, however, can
   //  still be a forward or friend declaration that has not yet been resolved
   //  to a class.
   //
   CxxToken* Root() const;

   //  Returns true if the item's type is POD.
   //
   bool IsPOD() const { return GetNumeric().IsPOD(); }

   //  Outputs PREFIX, invokes Print(stream) above, and inserts an endline.
   //  This is the appropriate implementation for items that can be displayed
   //  inline or separately.  See CodeDisplayOptions for OPTIONS flags.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;
protected:
   //  Protected because this class is virtual.
   //
   CxxToken();

   //  If the item is a class (and not a pointer or reference to a class),
   //  returns that class.  Returns nullptr otherwise.  The default version
   //  invokes DirectClass on GetTypeSpec.
   //
   virtual Class* DirectClass() const;

   //  Shrinks the specified container.
   //
   static void ShrinkExpression(const ExprPtr& expr);
   static void ShrinkTokens(const TokenPtrVector& tokens);
private:
   //  Overridden to prohibit copying.
   //
   CxxToken(const CxxToken& that);
   void operator=(const CxxToken& that);

   //  Returns the item's underlying type.  Can return nullptr, whereas Root
   //  (above) returns the item that returned nullptr.
   //
   virtual CxxToken* RootType() const { return const_cast< CxxToken* >(this); }

   //  Invoked when a function call is made to the item.
   //
   virtual void WasCalled() { }
};

//------------------------------------------------------------------------------
//
//  A literal.
//
class Literal : public CxxToken
{
public:
   //  Virtual to allow subclassing.
   //
   virtual ~Literal() { }

   //  Overridden to set the type for an "auto" variable.
   //
   virtual CxxToken* AutoType() const override;

   //  Overridden to push the literal onto the stack.
   //
   virtual void EnterBlock() override;

   //  Overridden to return the referent's name.
   //
   virtual const std::string* Name() const override;

   //  Overridden to return the output of the literal's Print function.
   //
   virtual std::string Trace() const override;

   //  Overridden to treat a literal as its underlying type.
   //
   virtual Cxx::ItemType Type() const override;
protected:
   //  Protected because this class is virtual.
   //
   Literal();
private:
   //  Overridden to return the literal's underlying type.
   //
   virtual CxxToken* RootType() const override;
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
   enum Radix
   {
      DEC,  // no prefix
      HEX,  // "0x" prefix
      OCT   // "0" prefix
   };

   //  Suffixes that specify an integer literal's size.
   //
   enum Size
   {
      SIZE_I,  // no suffix
      SIZE_L,  // "L" suffix
      SIZE_LL  // "LL" suffix
   };

   //  Tags for an integer literal.
   //
   struct Tags
   {
      Radix radix_ : 8;
      bool unsigned_ : 8;  // "U" suffix
      Size size_ : 8;
   };

   IntLiteral(int64_t num, const Tags& tags)
      : num_(num), tags_(tags) { CxxStats::Incr(CxxStats::INT_LITERAL); }
   ~IntLiteral() { CxxStats::Decr(CxxStats::INT_LITERAL); }
   virtual void Print(std::ostream& stream) const override;
   virtual CxxNamed* Referent() const override;
   virtual std::string TypeString(bool arg) const override;
private:
   virtual Numeric GetNumeric() const override;
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
   enum Size
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
   };

   FloatLiteral(long double num, const Tags& tags)
      : num_(num), tags_(tags) { CxxStats::Incr(CxxStats::FLOAT_LITERAL); }
   ~FloatLiteral() { CxxStats::Decr(CxxStats::FLOAT_LITERAL); }
   virtual void Print(std::ostream& stream) const override;
   virtual CxxNamed* Referent() const override;
   virtual std::string TypeString(bool arg) const override;
private:
   virtual Numeric GetNumeric() const override;
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
   explicit BoolLiteral(bool b)
      : b_(b) { CxxStats::Incr(CxxStats::BOOL_LITERAL); }
   ~BoolLiteral() { CxxStats::Decr(CxxStats::BOOL_LITERAL); }
   virtual void Print(std::ostream& stream) const
      override { stream << std::boolalpha << b_; }
   virtual CxxNamed* Referent() const override;
   virtual std::string TypeString(bool arg) const override { return BOOL_STR; }
private:
   virtual Numeric GetNumeric() const override { return Numeric::Bool; }
   const bool b_;
};

//------------------------------------------------------------------------------
//
//  A character literal ('c').
//
class CharLiteral : public Literal
{
public:
   explicit CharLiteral(char c)
      : c_(c) { CxxStats::Incr(CxxStats::CHAR_LITERAL); }
   ~CharLiteral() { CxxStats::Decr(CxxStats::CHAR_LITERAL); }
   virtual void Print(std::ostream& stream) const override;
   virtual CxxNamed* Referent() const override;
   virtual std::string TypeString(bool arg) const override { return CHAR_STR; }
private:
   virtual Numeric GetNumeric() const override { return Numeric::Char; }
   const char c_;
};

//------------------------------------------------------------------------------
//
//  A string literal ("s").
//
class StrLiteral : public Literal
{
public:
   explicit StrLiteral(const std::string& s)
      : str_(s) { CxxStats::Incr(CxxStats::STR_LITERAL); }
   ~StrLiteral() { CxxStats::Decr(CxxStats::STR_LITERAL); }
   std::string GetStr() const { return str_; }
   virtual TypeSpec* GetTypeSpec() const override;
   virtual void Print(std::ostream& stream) const
      override { stream << QUOTE << str_ << QUOTE; }
   virtual CxxNamed* Referent() const override;
   virtual void Shrink() override;
   virtual std::string TypeString(bool arg) const override { return "char*"; }
   static CxxNamed* GetReferent();
private:
   static TypeSpecPtr CreateRef();
   virtual Numeric GetNumeric() const override { return Numeric::Pointer; }
   std::string str_;
   static const TypeSpecPtr Ref_;
};

//------------------------------------------------------------------------------
//
//  For "nullptr".
//
class NullPtr : public Literal
{
public:
   NullPtr() { CxxStats::Incr(CxxStats::NULLPTR); }
   ~NullPtr() { CxxStats::Decr(CxxStats::NULLPTR); }
   virtual void Print(std::ostream& stream) const
      override { stream << NULLPTR_STR; }
   virtual bool IsConstPtr() const override { return true; }
   virtual CxxNamed* Referent() const override;
   virtual std::string TypeString(bool arg)
      const override { return NULLPTR_T_STR; }
private:
   virtual Numeric GetNumeric() const override { return Numeric::Pointer; }
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
   ~Operation() { CxxStats::Decr(CxxStats::OPERATION); }

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

   //  Executes the operation.  Obtains its arguments from the stack.
   //
   void Execute() const;

   //  Invoked when a unary operator is encountered.  This operator returns
   //  true if it will elide forward to the unary, and false if the new
   //  operator must actually be binary.
   //
   virtual bool AppendUnary() override;

   //  Overridden to return the operator, when it can accept more arguments,
   //  or the last argument, when no more arguments can be accepted.
   //
   virtual CxxToken* Back() override;

   //  Overridden to display the operator and its arguments.
   //
   virtual void Print(std::ostream& stream) const override;

   //  Overridden to push this operator and its arguments onto the stack.
   //
   virtual void EnterBlock() override;

   //  Overridden to update SYMBOLS with each argument's type usage.
   //
   virtual void GetUsages
      (const CodeFile& file, CxxUsageSets& symbols) const override;

   //  Overridden to shrink the tokens.
   //
   virtual void Shrink() override;

   //  Overridden to return the operator's symbol.
   //
   virtual std::string Trace() const override;

   //  Overridden to reveal that this is an operation.
   //
   virtual Cxx::ItemType Type() const override { return Cxx::Operation; }
private:
   //  Returns the number of arguments that the operator can still accept.
   //  Returns -1 if the operator takes a variable number of arguments and
   //  has the minimum number required.
   //
   int ArgCapacity() const;

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

   //  Pushes a TypeSpec that references NAME onto the argument stack.
   //
   static void PushType(const std::string& name);

   //  Handles the execution of a function call.
   //
   static void ExecuteCall();

   //  Finds the version of operator new, new[], delete, or delete[] to
   //  invoke for ARG.  DEL is set if looking for delete/delete[].  POD
   //  is updated to true when ARG is not a class, and so class overrides
   //  of the operators were not considered.  Returns nullptr on failure.
   //
   Function* FindNewOrDelete(const StackArg& arg, bool del, bool& pod) const;

   //  Handles the execution of operator new.
   //
   void ExecuteNew() const;

   //  Handles the execution of operator delete on ARG.
   //
   void ExecuteDelete(const StackArg& arg) const;

   //  If ARG1 overloads the operator, this executes the overload function
   //  and returns T, else it returns false.  If the operator is binary,
   //  ARG2 is its other argument.  NAME is the function name.
   //
   bool ExecuteOverload
      (const std::string& name, StackArg& arg1, const StackArg* arg2) const;

   //  If the operator is overloaded, returns TRUE after handling it.
   //  The overloads are for unary and binary operators, respectively.
   //
   bool IsOverloaded(StackArg& arg) const;
   bool IsOverloaded(StackArg& arg1, StackArg& arg2) const;

   //  Pushes the result of applying the operator to LHS and RHS.
   //
   void PushResult(const StackArg& lhs, const StackArg& rhs) const;

   //  Generates any log that applies to a cast operation from inArg to outArg.
   //
   void CheckCast(const StackArg& inArg, const StackArg& outArg) const;

   //  Displays operator new or operator new[].
   //
   void DisplayNew(std::ostream& stream) const;

   //  Displays the argument at INDEX.  Displays ERROR_STR if the argument
   //  does not exist.
   //
   void DisplayArg(std::ostream& stream, size_t index) const;

   //  The operator.
   //
   Cxx::Operator op_;

   //  The operator's arguments.
   //
   TokenPtrVector args_;
};

//------------------------------------------------------------------------------
//
//  An argument created for an operator to indicate that this argument will
//  come from the result of the previous or next operation.
//
class Elision : public CxxToken
{
public:
   Elision() { CxxStats::Incr(CxxStats::ELISION); }
   ~Elision() { CxxStats::Decr(CxxStats::ELISION); }
   virtual void Print(std::ostream& stream) const override { }
   virtual void EnterBlock() override { }
   virtual Cxx::ItemType Type() const override { return Cxx::Elision; }
};

//------------------------------------------------------------------------------
//
//  Created for an expression that is enclosed in parentheses.
//
class Precedence : public CxxToken
{
public:
   explicit Precedence(ExprPtr& expr)
      : expr_(std::move(expr)) { CxxStats::Incr(CxxStats::PRECEDENCE); }
   ~Precedence() { CxxStats::Decr(CxxStats::PRECEDENCE); }
   virtual void Print(std::ostream& stream) const override;
   virtual void EnterBlock() override;
   virtual void GetUsages
      (const CodeFile& file, CxxUsageSets& symbols) const override;
   virtual void Shrink() override { ShrinkExpression(expr_); }
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
   ~BraceInit() { CxxStats::Decr(CxxStats::BRACE_INIT); }
   void AddItem(TokenPtr& item) { items_.push_back(std::move(item)); }
   virtual void Print(std::ostream& stream) const override;
   virtual void EnterBlock() override;
   virtual void GetUsages
      (const CodeFile& file, CxxUsageSets& symbols) const override;
   virtual void Shrink() override;
private:
   TokenPtrVector items_;
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
   ~Expression() { CxxStats::Decr(CxxStats::EXPRESSION); }

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
   virtual CxxToken* Back() override;

   //  Overridden to display the expression.
   //
   virtual void Print(std::ostream& stream) const override;

   //  Overridden to invoke Context::Execute after invoking EnterBlock on
   //  each token in items_.
   //
   virtual void EnterBlock() override;

   //  Overridden to update SYMBOLS with each token's type usage.
   //
   virtual void GetUsages
      (const CodeFile& file, CxxUsageSets& symbols) const override;

   //  Overridden to shrink the tokens.
   //
   virtual void Shrink() override;

   //  Overridden to display the expression.
   //
   virtual std::string Trace() const override;
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

   //  Pushed onto the stack to mark the start of a new expression.
   //
   static const TokenPtr StartOfExpr;
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
   ~ArraySpec() { CxxStats::Decr(CxxStats::ARRAY_SPEC); }

   //  Overridden to display the array's size within brackets.
   //
   virtual void Print(std::ostream& stream) const override;

   //  Overridden to invoke EnterBlock on expr_.
   //
   virtual void EnterBlock() override;

   //  Overridden to update SYMBOLS with the specification's type usage.
   //
   virtual void GetUsages
      (const CodeFile& file, CxxUsageSets& symbols) const override;

   //  Overridden to shrink the array expression.
   //
   virtual void Shrink() override { ShrinkExpression(expr_); }

   //  Overridden to return "[]" if ARG is false and "*" if it is true.
   //
   virtual std::string TypeString(bool arg) const override;
private:
   //  The expression that specifies the array's size.
   //
   const ExprPtr expr_;
};
}
#endif
