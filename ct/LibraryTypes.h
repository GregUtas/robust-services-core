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
#include <iosfwd>
#include <memory>
#include <set>
#include <vector>

//------------------------------------------------------------------------------

namespace CodeTools
{
//  Forward declarations.
//
class LibraryItem;
class LibrarySet;
class CodeFile;
class CodeDir;

using LibItemSet = std::set< LibraryItem* >;
using CodeDirPtr = std::unique_ptr< CodeDir >;
using CodeFilePtr = std::unique_ptr< CodeFile >;

//  What a set of library items can contain.
//
enum LibSetType
{
   DIR_SET,   // a set of directories
   FILE_SET,  // a set of files
   ITEM_SET,  // a set of C++ code items
   VAR_SET,   // a set of library variables
   ANY_SET,   // a set of directories or files
   ERR_SET    // illegal set
};

//  Inserts a string for TYPE into STREAM.
//
std::ostream& operator<<(std::ostream& stream, LibSetType type);

//  For sorting code files in build order.
//
struct FileLevel
{
   CodeFile* file;  // the file
   size_t level;    // the file's level in the build

   FileLevel(CodeFile* f, size_t l) : file(f), level(l) { }
};

using BuildOrder = std::vector< FileLevel >;

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
   OpDefinitions,
   OpDeclaredBy,
   OpReferencedBy,
   OpFileDeclarers,
   OpCodeDeclarers,
   OpFileReferencers,
   OpCodeReferencers,
   OpReferencedIn,
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
   ItemSetExpected,
   IncompatibleArguments,
   InterpreterError,
   LibExprErr_N
};

//  Inserts a string for ERR into STREAM.
//
std::ostream& operator<<(std::ostream& stream, LibExprErr err);

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
