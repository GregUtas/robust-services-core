//==============================================================================
//
//  LibraryTypes.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef LIBRARYTYPES_H_INCLUDED
#define LIBRARYTYPES_H_INCLUDED

#include <cstddef>
#include <memory>
#include <set>
#include <vector>
#include "SysTypes.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace CodeTools
{
//  A synonym for a sorted list of unique file or directory identifiers.
//
typedef std::set< id_t > SetOfIds;

//  What a SetOfIds represents.
//
enum LibSetType
{
   ERR_SET,   // illegal expression
   DIR_SET,   // a set of directories
   FILE_SET,  // a set of files
   VAR_SET,   // a set of variables
   ANY_SET    // a set of directories or files
};

//  Synonym for a list of unique file or directory identifiers, sorted by
//  build order.
//
struct FileLevel
{
   id_t fid;      // the file's identifier
   size_t level;  // the file's level in the build
};

typedef std::vector< FileLevel > BuildOrder;
typedef std::unique_ptr< BuildOrder > BuildOrderPtr;

//  Tokens when parsing the expression associated with a library command.
//
enum LibTokenType
{
   OpNil,
   OpLeftPar,
   OpRightPar,
   OpIntersection,
   OpDifference,
   OpUnion,
   OpAutoUnion,
   OpDirectories,
   OpFiles,
   OpFileName,
   OpFileType,
   OpMatchString,
   OpFoundIn,
   OpImplements,
   OpUsedBy,
   OpUsers,
   OpAffectedBy,
   OpAffecters,
   OpCommonAffecters,
   OpNeededBy,
   OpNeeders,
   OpIdentifier,
   Operator_N = OpIdentifier  //  OpIdentifier is not actually an operator
};

//  Errors when parsing the expression associated with a library command.
//
enum LibExprErr
{
   ExpressionOk,
   EndOfExpression,
   EmptyExpression,
   IllegalCharacter,
   UnexpectedCharacter,
   NoSuchVariable,
   UnmatchedLeftPar,
   UnmatchedRightPar,
   LeftOperandMissing,
   RightOperandMissing,
   DirSetExpected,
   FileSetExpected,
   IncompatibleArguments,
   InterpreterError,
   LibExprErr_N
};

//  Returns a string that explains ERR.
//
const char* strError(LibExprErr err);
}
#endif
