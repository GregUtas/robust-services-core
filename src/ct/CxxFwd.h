//==============================================================================
//
//  CxxFwd.h
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
#ifndef CXXFWD_H_INCLUDED
#define CXXFWD_H_INCLUDED

#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

//------------------------------------------------------------------------------

namespace CodeTools
{
//  Forward declarations of classes that represent C++ language constructs.
//
class AlignAs;
class Argument;
class ArraySpec;
class Asm;
class BaseDecl;
class Block;
class Class;
class ClassData;
class ClassInst;
class CxxArea;
class CxxDirective;
class CxxLocation;
class CxxNamed;
class CxxScope;
class CxxScoped;
class CxxToken;
class Data;
class Define;
class Editor;
class Elif;
class Else;
class Endif;
class Enum;
class Enumerator;
class Error;
class Expression;
class Forward;
class Friend;
class FuncData;
class Function;
class Ifdef;
class Iff;
class Ifndef;
class Include;
class Line;
class Macro;
class MacroName;
class MemberInit;
class Namespace;
class Operation;
class OptionalCode;
class ParseFrame;
class Parser;
class Pragma;
class QualName;
class SpaceDefn;
class StackArg;
class StaticAssert;
class TemplateArg;
class TemplateParm;
class TemplateParms;
class Terminal;
class Typedef;
class TypeName;
class TypeSpec;
class Undef;
class Using;

//------------------------------------------------------------------------------
//
//  Types for unique_ptrs that own instances of the above classes.
//
typedef std::unique_ptr< AlignAs > AlignAsPtr;
typedef std::unique_ptr< Argument > ArgumentPtr;
typedef std::unique_ptr< ArraySpec > ArraySpecPtr;
typedef std::unique_ptr< Asm > AsmPtr;
typedef std::unique_ptr< BaseDecl > BaseDeclPtr;
typedef std::unique_ptr< Block > BlockPtr;
typedef std::unique_ptr< Class > ClassPtr;
typedef std::unique_ptr< ClassInst > ClassInstPtr;
typedef std::unique_ptr< CxxDirective > DirectivePtr;
typedef std::unique_ptr< CxxScope > ScopePtr;
typedef std::unique_ptr< CxxToken > TokenPtr;
typedef std::unique_ptr< Data > DataPtr;
typedef std::unique_ptr< Define > DefinePtr;
typedef std::unique_ptr< Elif > ElifPtr;
typedef std::unique_ptr< Else > ElsePtr;
typedef std::unique_ptr< Endif > EndifPtr;
typedef std::unique_ptr< Error > ErrorPtr;
typedef std::unique_ptr< Enum > EnumPtr;
typedef std::unique_ptr< Enumerator > EnumeratorPtr;
typedef std::unique_ptr< Expression > ExprPtr;
typedef std::unique_ptr< Forward > ForwardPtr;
typedef std::unique_ptr< Friend > FriendPtr;
typedef std::unique_ptr< FuncData > FuncDataPtr;
typedef std::unique_ptr< Function > FunctionPtr;
typedef std::unique_ptr< Ifdef > IfdefPtr;
typedef std::unique_ptr< Iff > IffPtr;
typedef std::unique_ptr< Ifndef > IfndefPtr;
typedef std::unique_ptr< Include > IncludePtr;
typedef std::unique_ptr< Line > LinePtr;
typedef std::unique_ptr< Macro > MacroPtr;
typedef std::unique_ptr< MacroName > MacroNamePtr;
typedef std::unique_ptr< MemberInit > MemberInitPtr;
typedef std::unique_ptr< Namespace > NamespacePtr;
typedef std::unique_ptr< ParseFrame > ParseFramePtr;
typedef std::unique_ptr< Parser > ParserPtr;
typedef std::unique_ptr< Pragma > PragmaPtr;
typedef std::unique_ptr< QualName > QualNamePtr;
typedef std::unique_ptr< SpaceDefn > SpaceDefnPtr;
typedef std::unique_ptr< StaticAssert > StaticAssertPtr;
typedef std::unique_ptr< TemplateArg > TemplateArgPtr;
typedef std::unique_ptr< TemplateParm > TemplateParmPtr;
typedef std::unique_ptr< TemplateParms > TemplateParmsPtr;
typedef std::unique_ptr< Terminal > TerminalPtr;
typedef std::unique_ptr< Typedef > TypedefPtr;
typedef std::unique_ptr< TypeName > TypeNamePtr;
typedef std::unique_ptr< TypeSpec > TypeSpecPtr;
typedef std::unique_ptr< Undef > UndefPtr;
typedef std::unique_ptr< Using > UsingPtr;

//------------------------------------------------------------------------------
//
//  Types for containers of the above classes.
//
typedef std::vector< ArraySpecPtr > ArraySpecPtrVector;
typedef std::vector< ArgumentPtr > ArgumentPtrVector;
typedef std::vector< AsmPtr > AsmPtrVector;
typedef std::vector< ClassPtr > ClassPtrVector;
typedef std::vector< ClassInstPtr > ClassInstPtrVector;
typedef std::vector< DataPtr > DataPtrVector;
typedef std::vector< DirectivePtr > DirectivePtrVector;
typedef std::vector< EnumPtr > EnumPtrVector;
typedef std::vector< EnumeratorPtr > EnumeratorPtrVector;
typedef std::vector< ForwardPtr > ForwardPtrVector;
typedef std::vector< FriendPtr > FriendPtrVector;
typedef std::vector< FunctionPtr > FunctionPtrVector;
typedef std::vector< IncludePtr > IncludePtrVector;
typedef std::vector< MacroPtr > MacroPtrVector;
typedef std::vector< MemberInitPtr > MemberInitPtrVector;
typedef std::vector< NamespacePtr > NamespacePtrVector;
typedef std::vector< ScopePtr > ScopePtrVector;
typedef std::vector< SpaceDefnPtr > SpaceDefnPtrVector;
typedef std::vector< StaticAssertPtr > StaticAssertPtrVector;
typedef std::vector< TemplateArgPtr > TemplateArgPtrVector;
typedef std::vector< TemplateParmPtr > TemplateParmPtrVector;
typedef std::vector< TokenPtr > TokenPtrVector;
typedef std::vector< TypedefPtr > TypedefPtrVector;
typedef std::vector< UsingPtr > UsingPtrVector;

typedef std::vector< Asm* > AsmVector;
typedef std::vector< Class* > ClassVector;
typedef std::vector< CxxScoped* > CxxScopedVector;
typedef std::vector< const CxxToken* > CxxTokenVector;
typedef std::vector< CxxToken* > CxxItemVector;
typedef std::vector< Data* > DataVector;
typedef std::vector< Elif* > ElifVector;
typedef std::vector< Enum* > EnumVector;
typedef std::vector< Forward* > ForwardVector;
typedef std::vector< Function* > FunctionVector;
typedef std::vector< Macro* > MacroVector;
typedef std::vector< SpaceDefn* > SpaceDefnVector;
typedef std::vector< StackArg > StackArgVector;
typedef std::vector< StaticAssert* > StaticAssertVector;
typedef std::vector< Typedef* > TypedefVector;
typedef std::vector< Using* > UsingVector;

typedef std::set< std::string > stringSet;
typedef std::set< CxxToken* > CxxTokenSet;
typedef std::set< CxxNamed* > CxxNamedSet;

typedef std::list< CxxToken* > CxxTokenList;

//------------------------------------------------------------------------------
//
//  For mapping template parameters to template arguments.
//
typedef std::map<std::string, std::string> TemplateParmToArgMap;
}
#endif
