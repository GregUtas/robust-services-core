//==============================================================================
//
//  LibraryTypes.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
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
