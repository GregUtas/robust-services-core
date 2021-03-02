//==============================================================================
//
//  LibraryTypes.h
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
#ifndef LIBRARYTYPES_H_INCLUDED
#define LIBRARYTYPES_H_INCLUDED

#include <cstddef>
#include <list>
#include <memory>
#include <set>
#include "SysTypes.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace CodeTools
{
//  What a set of library items can contain.
//
enum LibSetType
{
   ERR_SET,   // illegal set
   DIR_SET,   // a set of directories
   FILE_SET,  // a set of files
   VAR_SET,   // a set of variables
   ANY_SET    // a set of directories or files
};

//  Forward declarations.
//
class CodeDir;
class CodeFile;
class LibraryItem;
class LibrarySet;

using LibItemSet = std::set< LibraryItem* >;

using CodeDirPtr = std::unique_ptr< CodeDir >;
using CodeFilePtr = std::unique_ptr< CodeFile >;
using LibrarySetPtr = std::unique_ptr< LibrarySet >;

//  For sorting code files in build order.
//
struct FileLevel
{
   CodeFile* file;      // the file
   const size_t level;  // the file's level in the build

   FileLevel(CodeFile* f, size_t l) : file(f), level(l) { }
};

using BuildOrder = std::list< FileLevel >;

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
c_string strError(LibExprErr err);

//------------------------------------------------------------------------------
//
//  Options for the CLI >export command.
//
constexpr char NamespaceView = 'n';
constexpr char CanonicalFileView = 'c';
constexpr char OriginalFileView = 'o';
constexpr char ClassHierarchyView = 'h';
constexpr char ItemStatistics = 's';
constexpr char FileSymbolUsage = 'u';
constexpr char GlobalCrossReference = 'x';
}
#endif
