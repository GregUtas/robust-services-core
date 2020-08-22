//==============================================================================
//
//  Interpreter.h
//
//  Copyright (C) 2013-2020  Greg Utas
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

//------------------------------------------------------------------------------

namespace CodeTools
{
//  Used to evaluate the arguments to a library command.
//
class Interpreter : public NodeBase::Temporary
{
public:
   //  Sets the expression to be evaluated to EXPR, which
   //  started at OFFSET of the input line.
   //
   Interpreter(const std::string& expr, size_t offset);

   //  Not subclassed.
   //
   ~Interpreter();

   //  Evaluates the expression.
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
