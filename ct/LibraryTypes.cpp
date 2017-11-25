//==============================================================================
//
//  LibraryTypes.cpp
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
#include "LibraryTypes.h"

//------------------------------------------------------------------------------

namespace CodeTools
{
fixed_string LibExprErrStrings[LibExprErr_N + 1] =
{
   "OK.",                           // ExpressionOk
   "End of expression.",            // EndOfExpression
   "Expression missing.",           // EmptyExpression
   "Illegal character.",            // IllegalCharacter
   "Unexpected character.",         // UnexpectedCharacter
   "No such variable.",             // NoSuchVariable
   "Unmatched left parenthesis.",   // UnmatchedLeftPar
   "Unmatched right parenthesis.",  // UnmatchedRightPar
   "Missing left-hand argument.",   // LeftOperandMissing
   "Missing right-hand argument.",  // RightOperandMissing
   "Directory set expected.",       // DirSetExpected
   "File set expected.",            // FileSetExpected
   "Set types do not match.",       // IncompatibleArguments
   "Internal error.",               // InterpreterError
   ERROR_STR
};

const char* CodeTools::strError(LibExprErr err)
{
   if((err >= 0) && (err < LibExprErr_N)) return LibExprErrStrings[err];
   return LibExprErrStrings[LibExprErr_N];
}
}
