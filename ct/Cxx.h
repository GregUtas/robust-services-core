//==============================================================================
//
//  Cxx.h
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
#ifndef CXX_H_INCLUDED
#define CXX_H_INCLUDED

#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <string>
#include "CodeTypes.h"

//------------------------------------------------------------------------------

namespace CodeTools
{
//  Class for wrapping C++ definitions.
//
namespace Cxx
{
   enum Directive
   {
      _DEFINE,
      _ERROR,
      _ELIF,
      _ELSE,
      _ENDIF,
      _IF,
      _IFDEF,
      _IFNDEF,
      _INCLUDE,
      _LINE,
      _PRAGMA,
      _UNDEF,
      NIL_DIRECTIVE
   };

   enum Keyword
   {
      AUTO,
      BREAK,
      CASE,
      CLASS,
      CONST,
      CONSTEXPR,
      CONTINUE,
      DEFAULT,
      DO,
      ENUM,
      EXPLICIT,
      EXTERN,
      FOR,
      FRIEND,
      HASH,        // treats preprocessor '#' as keyword
      IF,
      INLINE,
      MUTABLE,
      NAMESPACE,
      OPERATOR,
      PRIVATE,
      PROTECTED,
      PUBLIC,
      RETURN,
      STATIC,
      STRUCT,
      SWITCH,
      TEMPLATE,
      TRY,
      TYPEDEF,
      UNION,
      USING,
      VIRTUAL,
      WHILE,
      NVDTOR,      // treats non-virtual destructor's '~' as keyword
      NIL_KEYWORD  // no keyword found; also used as maximum value
   };

   enum Operator
   {
      SCOPE_RESOLUTION,         // n::t
      REFERENCE_SELECT,         // r.m
      POINTER_SELECT,           // p->m
      ARRAY_SUBSCRIPT,          // a[i]
      FUNCTION_CALL,            // f(a)
      POSTFIX_INCREMENT,        // i++
      POSTFIX_DECREMENT,        // i--
      DEFINED,                  // defined(a)
      TYPE_NAME,                // typeid(a)
      CONST_CAST,               // const_cast< t >(a)
      DYNAMIC_CAST,             // dynamic_cast< t >(a)
      REINTERPRET_CAST,         // reinterpret_cast< t >(a)
      STATIC_CAST,              // static_cast< t >(a)
      SIZEOF_TYPE,              // sizeof(a)
      PREFIX_INCREMENT,         // ++i
      PREFIX_DECREMENT,         // --i
      ONES_COMPLEMENT,          // ~i
      LOGICAL_NOT,              // !i
      UNARY_PLUS,               // +i
      UNARY_MINUS,              // -i
      ADDRESS_OF,               // &a
      INDIRECTION,              // *p
      OBJECT_CREATE,            // new t
      OBJECT_CREATE_ARRAY,      // new[] t
      OBJECT_DELETE,            // delete p
      OBJECT_DELETE_ARRAY,      // delete[] p
      CAST,                     // (t)
      REFERENCE_SELECT_MEMBER,  // r.*m
      POINTER_SELECT_MEMBER,    // p->*m
      MULTIPLY,                 // i * j
      DIVIDE,                   // i / j
      MODULO,                   // i % j
      ADD,                      // i + j
      SUBTRACT,                 // i - j
      LEFT_SHIFT,               // i << j
      RIGHT_SHIFT,              // i >> j
      LESS,                     // i < j
      LESS_OR_EQUAL,            // i <= j
      GREATER,                  // i > j
      GREATER_OR_EQUAL,         // i >= j
      EQUALITY,                 // i == j
      INEQUALITY,               // i != j
      BITWISE_AND,              // i & j
      BITWISE_XOR,              // i ^ j
      BITWISE_OR,               // i | j
      LOGICAL_AND,              // b && c
      LOGICAL_OR,               // b || c
      CONDITIONAL,              // b ? i : j
      ASSIGN,                   // i = j
      MULTIPLY_ASSIGN,          // i *= j
      DIVIDE_ASSIGN,            // i /= j
      MODULO_ASSIGN,            // i %= j
      ADD_ASSIGN,               // i += j
      SUBTRACT_ASSIGN,          // i -= j
      LEFT_SHIFT_ASSIGN,        // i <<= j
      RIGHT_SHIFT_ASSIGN,       // i >>= j
      BITWISE_AND_ASSIGN,       // i &= j
      BITWISE_XOR_ASSIGN,       // i ^= j
      BITWISE_OR_ASSIGN,        // i |= j
      THROW,                    // throw e
      STATEMENT_SEPARATOR,      // ,
      START_OF_EXPRESSION,      // pushed onto operator stack for new expression
      FALSE,                    // parsed with alphanumeric operators
      TRUE,                     // parsed with alphanumeric operators
      NULLPTR,                  // parsed with alphanumeric operators
      NIL_OPERATOR              // no operator found; also used as maximum value
   };

   //  Keywords for built-in types (Terminals).
   //
   enum Type
   {
      AUTO_TYPE,
      BOOL,
      CHAR,
      DOUBLE,
      FLOAT,
      INT,
      LONG,
      NULLPTR_TYPE,
      SHORT,
      SIGNED,
      UNSIGNED,
      VOID,
      NON_TYPE,  // a keyword that can erroneously be parsed as a type
      NIL_TYPE   // none of the above
   };

   //  Class types.
   //
   enum ClassTag
   {
      Typename,
      ClassType,
      StructType,
      UnionType,
      ClassTag_N
   };

   //  Access control.
   //
   enum Access
   {
      Private,
      Protected,
      Public,
      Access_N
   };

   //  C++ item types (subclasses of CxxToken).  Unless a subclass has a value
   //  here, it is "Undefined".  This type is used to avoid dynamic casts and,
   //  sometimes, to determine policy when defining a virtual function for that
   //  purpose would be burdensome.
   //
   enum ItemType
   {
      Undefined,  // none of the following
      Terminal,
      Class,
      Argument,
      Block,
      Data,
      Enum,
      Enumerator,
      Forward,
      Friend,
      Function,
      Macro,
      Namespace,
      QualName,
      Typedef,
      TypeSpec,
      If,
      NoOp,
      Operation,
      Elision
   };

   //  Inserts a string for ACCESS into STREAM.
   //
   std::ostream& operator<<(std::ostream& stream, Access access);

   //  Inserts a string for TAG into STREAM.
   //
   std::ostream& operator<<(std::ostream& stream, ClassTag tag);
}

//------------------------------------------------------------------------------
//
//  Attributes of C++ keywords.
//
struct CxxWord
{
   //  What to look for when a particular keyword is found at file scope,
   //  in a class, and in a function, respectively:
   //      A (access control)   b (break)
   //      C (class)            c (catch)
   //      D (data)             d (do)
   //      E (enum)             f (for)
   //      F (friend)           i (if)
   //      H (# directive)      n (continue)
   //      N (namespace)        r (return)
   //      P (function)         s (switch)
   //      T (typedef)          t (try)
   //      U (using)            w (while)
   //      - (error)            x (basic statement)
   //
   const std::string fileTarget;
   const std::string classTarget;
   const std::string funcTarget;

   //  Set if the parse should advance past the keyword when it is found.
   //  The parsing routines for member data and functions always parse an
   //  entire declaration.  Any keyword which can begin such a declaration
   //  must therefore have this field set to false.
   //
   const bool advance;

   //  The array that contains the above attributes for each operator.
   //
   static const CxxWord Attrs[Cxx::NIL_KEYWORD + 1];
private:
   //  Constructs a keyword with the specified attributes.
   //
   CxxWord(const std::string& file,
      const std::string& cls, const std::string& func, bool adv);

   //  Define the copy operator to suppress the compiler warning caused
   //  by our const string member.
   //
   CxxWord& operator=(const CxxWord& that) = delete;
};

//------------------------------------------------------------------------------
//
//  Attributes of C++ characters.
//
struct CxxChar
{
   //  Set if valid as the first character in an identifier.
   //
   bool validFirst;

   //  Set if valid as a subsequent character in an identifier.
   //
   bool validNext;

   //  Set if valid in an operator.
   //
   bool validOp;

   //  Set if valid in an integer literal.
   //
   bool validInt;

   //  The numeric value in an integer literal.  -1 if invalid.
   //
   int8_t intValue;

   //  The numeric value in a hex literal.  -1 if invalid.
   //
   int8_t hexValue;

   //  The numeric value in an octal literal.  -1 if invalid.
   //
   int8_t octValue;

   //  Initalizes Attrs.
   //
   static void Initialize();

   //  The array that contains the above attributes for each operator.
   //
   static CxxChar Attrs[UINT8_MAX + 1];
};

//------------------------------------------------------------------------------
//
//  Attributes of C++ operators.
//
struct CxxOp
{
   //  OPER was selected before the number of arguments was known.  Now
   //  that the number is known, verify that it is correct, updating it
   //  if it was ambiguous before ARGS was known.
   //
   static void UpdateOperator(Cxx::Operator& oper, size_t args);

   //  Returns the function name for overloading OPER.
   //
   static std::string OperatorToName(Cxx::Operator oper);

   //  If NAME is that of an operator function, returns the operator,
   //  else returns Cxx::NIL_OPERATOR.  Note that some operators are
   //  ambiguous and will therefore never be returned:
   //  o operator() is FUNCTION_CALL, never CAST
   //  o operator-- is POSTFIX_INCREMENT, never PREFIX_INCREMENT
   //  o operator-- is POSTFIX_DECREMENT, never PREFIX_DECREMENT
   //  o operator+ is UNARY_PLUS, never ADD
   //  o operator- is UNARY_MINUS, never SUBTRACT
   //  o operator& is ADDRESS_OF, never BITWISE_AND
   //  o operator* is INDIRECTION, never MULTIPLY
   //
   static Cxx::Operator NameToOperator(const std::string& name);

   //  The string used for the operator.
   //
   const std::string symbol;

   //  How many arguments the operator takes (0 = a variable number).
   //
   const size_t arguments;

   //  The operator's priority.
   //
   const size_t priority;

   //  Set if the operator can be overloaded.
   //
   const bool overloadable;

   //  Set if the operator is pushed when the operator on top of the stack
   //  hasthe same priority.  This is known as right-to-left associativity
   //  and prevents, for example, **a from trying to execute the first *
   //  before an argument has been pushed onto the stack.
   //
   const bool rightToLeft;

   //  Set if the operator can take two rvalues.
   //
   const bool symmetric;

   //  The array that contains the above attributes for each operator.
   //
   static const CxxOp Attrs[Cxx::NIL_OPERATOR + 1];
private:
   //  Constructs an operator with the specified attributes.
   //
   CxxOp(const std::string& sym, size_t args,
      size_t prio, bool over, bool push, bool symm);

   //  Define the copy operator to suppress the compiler warning caused
   //  by our const string member.
   //
   CxxOp& operator=(const CxxOp& that) = delete;
};

//------------------------------------------------------------------------------
//
//  Representation of a numeric value, possibly involving a conversion.
//
class Numeric
{
public:
   enum NumericType
   {
      NIL,
      INT,
      FLOAT,
      PTR,
      ENUM
   };

   //  Sets each attribute.
   //
   Numeric(NumericType type, size_t width, bool sign)
      : type_(type), bitWidth_(width), signed_(sign) { }

   //  Returns the basic type.
   //
   NumericType Type() const { return type_; }

   //  Returns true if this is a POD type.
   //
   bool IsPOD() const { return (type_ != NIL); }

   //  Returns the level of compatibility when assigning THAT to this item.
   //
   TypeMatch CalcMatchWith(const Numeric* that) const;

   //  Pre-defined Numerics for various types.
   //
   static const Numeric Nil;
   static const Numeric Bool;
   static const Numeric Char;
   static const Numeric Double;
   static const Numeric Enum;
   static const Numeric Float;
   static const Numeric Int;
   static const Numeric Long;
   static const Numeric LongDouble;
   static const Numeric LongLong;
   static const Numeric Pointer;
   static const Numeric Short;
   static const Numeric uChar;
   static const Numeric uInt;
   static const Numeric uLong;
   static const Numeric uLongLong;
   static const Numeric uShort;
private:
   //  The underlying type.
   //
   NumericType type_ : 8;

   //  The number of bits in the type.
   //
   size_t bitWidth_ : 8;

   //  Set if signed (else unsigned).
   //
   bool signed_ : 8;
};

//------------------------------------------------------------------------------
//
//  Statistics for memory usage by classes that represent C++ items.
//
class CxxStats
{
public:
   //  The classes whose memory usage is tracked.
   //
   enum Item
   {
      MACRO_NAME,
      IF_DIRECTIVE,
      IFDEF_DIRECTIVE,
      IFNDEF_DIRECTIVE,
      ELIF_DIRECTIVE,
      ELSE_DIRECTIVE,
      ENDIF_DIRECTIVE,
      DEFINE_DIRECTIVE,
      UNDEF_DIRECTIVE,
      INCLUDE_DIRECTIVE,
      ERROR_DIRECTIVE,
      LINE_DIRECTIVE,
      PRAGMA_DIRECTIVE,
      INT_LITERAL,
      FLOAT_LITERAL,
      BOOL_LITERAL,
      CHAR_LITERAL,
      STR_LITERAL,
      NULLPTR,
      OPERATION,
      ELISION,
      PRECEDENCE,
      BRACE_INIT,
      EXPRESSION,
      ARRAY_SPEC,
      TEMPLATE_PARMS,
      MEMBER_INIT,
      QUAL_NAME,
      TEMPLATE_PARM,
      TYPE_NAME,
      DATA_SPEC,
      FUNC_SPEC,
      USING_DECL,
      ARG_DECL,
      BASE_DECL,
      ENUM_DECL,
      ENUM_MEM,
      FORWARD_DECL,
      FRIEND_DECL,
      TERMINAL_DECL,
      TYPE_DECL,
      BREAK,
      CASE,
      CATCH,
      CONTINUE,
      DO,
      EXPR,
      FOR,
      IF,
      LABEL,
      NOOP,
      RETURN,
      SWITCH,
      TRY,
      WHILE,
      BLOCK_DECL,
      CLASS_DATA,
      FILE_DATA,
      FUNC_DATA,
      FUNC_DECL,
      CLASS_DECL,
      CLASS_INST,
      SPACE_DECL,
      CODE_FILE,
      CXX_SYMBOLS,
      Item_N
   };

   //  Invoked when an item is allocated.
   //
   static void Incr(Item item) { ++Info[item].in_use; }

   //  Invoked when an item is deleted.
   //
   static void Decr(Item item) { --Info[item].in_use; }

   //  Invoked by the item's Shrink function to note the size of its containers.
   //
   static void Strings(Item item, size_t size) { Info[item].strings += size; }
   static void Vectors(Item item, size_t size) { Info[item].vectors += size; }

   //  Resets the string and vector usage for each item.
   //
   static void Shrink();

   //  Displays the statistics.
   //
   static void Display(std::ostream& stream);
private:
   //  Constructs an entry with the specified attributes.
   //
   CxxStats(const std::string& item, size_t bytes);

   //  Define the copy operator to suppress the compiler warning caused
   //  by our const string member.
   //
   CxxStats& operator=(const CxxStats& that) = delete;

   //  The item's name.
   //
   const std::string name;

   //  The class's size.
   //
   const size_t size;

   //  The number of items of this type that are currently allocated.
   //
   size_t in_use;

   //  The size of all strings associated with this type of item.
   //
   size_t strings;

   //  The size of all vectors associated with this type of item.
   //
   size_t vectors;

   //  The array that contains the above attributes for each operator.
   //
   static CxxStats Info[Item_N];
};
}
#endif
