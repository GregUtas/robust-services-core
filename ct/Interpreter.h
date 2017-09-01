//==============================================================================
//
//  Interpreter.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef INTERPRETER_H_INCLUDED
#define INTERPRETER_H_INCLUDED

#include "Temporary.h"
#include <cstddef>
#include <queue>
#include <stack>
#include <string>
#include "LibraryTypes.h"

namespace CodeTools
{
   class LibraryOpcode;
   class LibrarySet;
}

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace CodeTools
{
//  Used to evaluate the arguments to a library command.
//
class Interpreter : public Temporary
{
public:
   //  Sets the expression to be evaluated to EXPR, which
   //  started at OFFSET of the input line.
   //
   Interpreter(const std::string& expr, size_t offset);

   //  Not subclassed.
   //
   ~Interpreter();

   //  Evalutes the expression.
   //
   LibrarySet* Evaluate();

   //  Returns true if S is an operator.
   //
   static bool IsOperator(const std::string& s);
private:
   //  Checks the expression for obvious errors.
   //
   LibExprErr CheckExpr();

   //  Advances over any blanks.
   //
   LibExprErr SkipBlanks();

   //  Finds the next token.
   //
   LibExprErr GetToken();

   //  Processes the token that was just found.
   //
   LibExprErr HandleToken();

   //  Applies any operator on top of the stack.  OPERAND is set
   //  if an operand was just pushed onto the stack.
   //
   LibExprErr ApplyOperator(bool operand);

   //  Creates and returns a LibraryErrSet when ERR occurs.
   //
   LibrarySet* Error(LibExprErr err) const;

   //  Overridden to prohibit copying.
   //
   Interpreter(const Interpreter& that);
   void operator=(const Interpreter& that);

   //  The expression to be evaluated.
   //
   const std::string& expr_;

   //  The original offset of expr_ in the input stream.
   //
   const size_t offset_;

   //  The previous parse location in expr_.
   //
   size_t prev_;

   //  The current parse location in expr_.
   //
   size_t curr_;

   //  The type of token being processed.
   //
   LibTokenType type_;

   //  The actual token being processed.
   //
   std::string token_;

   //  The stack of pending operators.
   //
   std::stack< LibTokenType > operators_;

   //  The stack of pending operands.
   //
   std::stack< LibrarySet* > operands_;

   //  The opcodes generated so far.
   //
   std::queue< LibraryOpcode* > opcodes_;
};
}
#endif
