//==============================================================================
//
//  Parser.h
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
#ifndef PARSER_H_INCLUDED
#define PARSER_H_INCLUDED

#include <cstddef>
#include <iosfwd>
#include <string>
#include "CodeTypes.h"
#include "Cxx.h"
#include "CxxFwd.h"
#include "Lexer.h"
#include "LibraryTypes.h"
#include "SystemTime.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace CodeTools
{
//  C++ parser.  Its purpose is to support software analysis, so it assumes that
//  it is parsing code that successfully compiles and links.  It may therefore
//  accept constructs that would actually be illegal.  Conversely, it can reject
//  legal constructs that the code base does not use.  This keeps the grammar
//  manageable.
//
//  The parser is implemented using recursive descent.  Except for the analysis
//  of #include lists (CodeFile.Scan) and the preprocessing of empty macro names
//  (Lexer.Preprocess), it is a single-pass parser.  The grammar is documented
//  in the relevant parser functions.
//
//  The parser does not support the concept of "translation units".  All files
//  are parsed in one pass after analyzing #include directives to calculate a
//  global parse order.  All header files are parsed first.
//
//  The parser supports all C++ language features used in the RSC code base.
//  This is a subset of C++11, so there are many things that the parser does
//  not yet support.  See "RSC-Cpp11-Exclusions" in RSC's docs/ directory for
//  a description of what is not supported.
//
//  Comments in the CodeTools namespace that begin with "//c" describe other
//  enhancements that have not been implemented.
//
class Parser
{
public:
   //  Creates a parser.
   //
   Parser();

   //  Creates a parser that will parse a code fragment on behalf of another
   //  parser.  The scope in which to parse must be provided.
   //
   explicit Parser(CxxScope* scope);

   //  Not subclassed.
   //
   ~Parser();

   //  Deleted to prohibit copying.
   //
   Parser(const Parser& that) = delete;

   //  Deleted to prohibit copy assignment.
   //
   Parser& operator=(const Parser& that) = delete;

   //  Parses FILE.  Returns true on success.
   //
   bool Parse(CodeFile& file);

   //  Parses a class template instance starting at POS.
   //
   bool ParseClassInst(ClassInst* inst, size_t pos);

   //  Parses the function template instance identified by NAME, adding it to
   //  the namespace or class specified by AREA.  TMPLT is the function template
   //  on which it is based, and TYPE and CODE contain the instance's template
   //  arguments and code.  Returns the new function on success, or nullptr on
   //  failure.
   //
   Function* ParseFuncInst(const std::string& name, const Function* tmplt,
      CxxArea* area, const TypeName* type, const std::string* code);

   //  Returns true and creates ARG if CODE is a valid template argument.
   //
   bool ParseTemplateArg(const std::string& code, TemplateArgPtr& arg);

   //  Returns true and creates NAME if CODE is a valid name, maybe qualified.
   //
   bool ParseQualName(const std::string& code, QualNamePtr& name);

   //  If parsing a template instance for which NAME is an argument, returns
   //  that argument.
   //
   CxxScoped* ResolveInstanceArgument(const QualName* name) const;

   //  Replaces FUNC's implementation after it has been modified.  CODE is
   //  the full source code that contains FUNC's modified implementation.
   //
   bool ReplaceImpl(Function* func, const std::string& code);

   //  Parses the item that begins at or after POS in FILE, at file scope, in
   //  SPACE.  CODE is the full source code for FILE.  If FILE is nullptr, it
   //  is treated as a subsequent parse in the same file and namespace.
   //
   bool ParseFileItem(const std::string& code,
      size_t pos, CodeFile* file, Namespace* space);

   //  Parses the item that begins at or after POS in CLS's definition.
   //  ACCESS specifies its access control, and CODE is the full source
   //  code for the file in which CLS is defined.
   //
   bool ParseClassItem(const std::string& code,
      size_t pos, Class* cls, Cxx::Access access);

   //  Returns true if original source code is being parsed.
   //
   bool ParsingSourceCode() const { return (source_ == IsFile); }

   //  Returns true if a template instance is currently being parsed.
   //
   bool ParsingTemplateInstance()
      const { return ((source_ == IsClassInst) || (source_ == IsFuncInst)); }

   //  Returns the name of what is being parsed (e.g. a file or a template
   //  instance).
   //
   std::string GetVenue() const { return venue_; }

   //  Returns the line number associated with POS in what is being parsed.
   //  If POS is not specified, the parser's current location is used.
   //
   size_t GetLineNum(size_t pos = std::string::npos) const;

   //  Returns the time when the parse originally started.
   //
   static const NodeBase::SystemTime::Point& GetTime();

   //  Returns a string that specifies the parser's current position for the
   //  __LINE__ macro.  If parsing source code, this will be a numeric.  If
   //  parsing a template, it prefixes the template's name.
   //
   std::string GetLINE() const;

   //  Returns the parser's previous position within its Lexer.
   //
   size_t GetPrev() const { return lexer_.Prev(); }

   //  Zeroes the statistics.
   //
   static void ResetStats();

   //  Displays the statistics in STREAM.
   //
   static void DisplayStats(std::ostream& stream);
private:
   //  Things that can be parsed.
   //
   enum SourceType
   {
      IsUnknown,
      IsFile,       // source code in a file
      IsClassInst,  // code for a class template instance
      IsFuncInst,   // code for a function template instance
      IsTmpltArg,   // a string containing a template argument
      IsQualName    // a string containing a qualified name
   };

   //  Prepares to parse CODE, of type SOURCE.  PREPROCESS is set if the
   //  code should be preprocessed.  VENUE identifies the code for logging
   //  purposes, and INST is the template's name and arguments if parsing
   //  a template instance.
   //
   void Enter(SourceType source, const std::string& venue, const TypeName* inst,
      const std::string& code, bool preprocess, CodeFile* file = nullptr);

   //  Parses declarations at file scope.  SPACE is the current namespace.
   //
   void GetFileDecls(Namespace* space);

   //  Tries the parses associated with KWD, adding declarations to SPACE.
   //
   bool ParseInFile(Cxx::Keyword kwd, Namespace* space);

   //  Parsing of preprocessor directives.  HandleDirective is used when a '#'
   //  is encountered.  When handling a #define or #include, it returns true on
   //  success, but DIR is nonetheless nullptr.  The other functions handle the
   //  directive after which they are named.  Those with a DIR argument create
   //  and return a directive so that it can be added to the current scope.  A
   //  #define or #include is always added to the current file, regardless of
   //  the scope in which it appears.
   //
   bool HandleDirective(DirectivePtr& dir);
   bool HandleInclude();
   bool HandleDefine();
   bool HandleElif(DirectivePtr& dir);
   bool HandleElse(DirectivePtr& dir);
   bool HandleEndif(DirectivePtr& dir);
   bool HandleError(DirectivePtr& dir);
   bool HandleIf(DirectivePtr& dir);
   bool HandleIfdef(DirectivePtr& dir);
   bool HandleIfndef(DirectivePtr& dir);
   bool HandleLine(DirectivePtr& dir);
   bool HandlePragma(DirectivePtr& dir);
   bool HandleUndef(DirectivePtr& dir);

   //  Returns true and creates EXPR on finding a preprocessor expression.
   //  END delimits the expression, which should finish at END - 1.  The
   //  expression is evaluated when END is reached.
   //
   bool GetPreExpr(ExprPtr& expr, size_t end);

   //  Updates EXPR with the results of parsing an expression upon reaching
   //  an alphabetic character.  Used in preprocessor directives.
   //
   bool GetPreAlpha(const ExprPtr& expr);

   //  Errors associated with preprocessor directives.
   //
   enum DirectiveError
   {
      DirectiveMismatch = 1,
      SymbolExpected,
      FileExpected,
      ConditionExpected,
      ElifUnexpected,
      ElseUnexpected,
      EndifUnexpected,
      EndifExpected
   };

   //  Reports an error associated with a preprocessor directive and returns
   //  false.
   //
   bool Fault(DirectiveError err) const;

   //  Parses declarations in CLS (a class, struct, or union).
   //
   void GetMemberDecls(Class* cls);

   //  Tries the parses associated with KWD, adding declarations to CLS.
   //
   bool ParseInClass(Cxx::Keyword kwd, Class* cls);

   //  Parses statements in BLOCK (a code block).  BRACED is set if the
   //  block started with a left brace.  If it didn't, it contains only
   //  one statement.
   //
   bool GetStatements(Block* block, bool braced);

   //  Tries the parses associated with KWD, adding statements to BLOCK.
   //
   bool ParseInBlock(Cxx::Keyword kwd, Block* block);

   //  Returns true and creates USE or TYPE on finding a using statement.
   //
   bool GetUsing(UsingPtr& use, TypedefPtr& type);

   //  Returns true on finding a namespace definition.
   //
   bool GetNamespace();

   //  Returns true on finding a class declaration.  KWD is the keyword
   //  that was just found, and AREA is the namespace or class in which
   //  the class is being declared.
   //
   bool GetClass(Cxx::Keyword kwd, CxxArea* area);

   //  Returns true and creates CLS on finding a class definition.  If it
   //  is only a forward declaration, it is returned in FORW.  KWD is the
   //  keyword that was just found.
   //
   bool GetClassDefn(Cxx::Keyword kwd, ClassPtr& cls, ForwardPtr& forw);

   //  Returns true and creates BASE on finding a base class declaration.
   //
   bool GetBaseDecl(BaseDeclPtr& base);

   //  Looks for a function definition after FUNC's signature has been parsed.
   //
   void GetInline(Function* func);

   //  Returns true after successfully parsing CLS's inlines.
   //
   bool GetInlines(Class* cls);

   //  Returns true and creates DECL on finding a friend declaration.
   //
   bool GetFriend(FriendPtr& decl);

   //  Returns true and updates ACCESS on finding an access scope declaration.
   //  KWD is the keyword that was just found.
   //
   bool GetAccess(Cxx::Keyword kwd, Cxx::Access& access);

   //  Returns true and creates DECL on finding an enum.
   //
   bool GetEnum(EnumPtr& decl);

   //  Returns true on finding an enumerator defined by NAME and INIT.
   //
   bool GetEnumerator(std::string& name, ExprPtr& init);

   //  Returns true and creates TDEF on finding a typedef.
   //
   bool GetTypedef(TypedefPtr& type);

   //  Returns true and updates FUNC on finding a function specification.
   //  SPEC is the function's return type, which has already been parsed.
   //
   bool GetFuncSpec(TypeSpecPtr& spec, FunctionPtr& func);

   //  Updates ALIGN if it finds an alignas directive.  Returns false
   //  only if "alignas" was found but its parse failed.
   //
   bool GetAlignAs(AlignAsPtr& align);

   //  Returns true and creates DATA on finding a data declaration
   //  o at file scope
   //  o within a class
   //  o within a function
   //  KWD is the keyword that was found at the start of the possible data
   //  declaration, but the lexer has not consumed it.
   //
   bool GetSpaceData(Cxx::Keyword kwd, DataPtr& data);
   bool GetClassData(DataPtr& data);
   bool GetFuncData(DataPtr& data);

   //  Returns true and creates FUNC on finding a function declaration.
   //  KWD is the keyword that was found at the start of the potential
   //  function declaration, but the parse has not advanced over it.
   //
   bool GetFuncDecl(Cxx::Keyword kwd, FunctionPtr& func);

   //  Returns true and creates FUNC on finding a function definition.
   //  KWD is the keyword that was found at the start of the potential
   //  function definition, but the parse has not advanced over it.
   //
   bool GetFuncDefn(Cxx::Keyword kwd, FunctionPtr& func);

   //  Returns true and updates FUNC on finding a function implementation.
   //
   bool GetFuncImpl(Function* func);

   //  Returns true and updates FUNC if the function is deleted or defaulted.
   //
   bool GetFuncSpecial(Function* func);

   //  Returns true and creates FUNC on finding
   //  o GetCtorDecl: a constructor declaration
   //  o GetCtorDefn: a constructor implementation
   //  o GetCtorInit: a constructor initialization list
   //  o GetDtorDecl: a destructor declaration
   //  o GetDtorDefn: a destructor implementation
   //  o GetProcDecl: a function declaration
   //  o GetProcDefn: a function implementation
   //
   bool GetCtorDecl(FunctionPtr& func);
   bool GetCtorDefn(FunctionPtr& func);
   bool GetCtorInit(Function* func);
   bool GetDtorDecl(FunctionPtr& func);
   bool GetDtorDefn(FunctionPtr& func);
   bool GetProcDecl(FunctionPtr& func);
   bool GetProcDefn(FunctionPtr& func);

   //  Returns true and updates TOKEN after parsing a member initialization
   //  expression, either between parentheses or braces.
   //
   bool GetMemberInit(TokenPtr& token);

   //  Returns true and creates SPEC on finding a type specification.  ATTRS
   //  is used when parsing a data type or type returned by a function, when
   //  "const" and "volatile" may appear before keywords such as "static" and
   //  "extern".  If SPEC is a function type (FuncSpec), the second version
   //  returns the name, if any, assigned to that type.
   //
   bool GetTypeSpec(TypeSpecPtr& spec, KeywordSet* attrs = nullptr);
   bool GetTypeSpec(TypeSpecPtr& spec, std::string& name);

   //  Returns true and creates ARRAY on finding an array specification.
   //
   bool GetArraySpec(ArraySpecPtr& array);

   //  Returns true and updates FUNC on finding one or more function arguments.
   //
   bool GetArguments(Function* func);

   //  Returns true and creates DECL on finding a function argument.
   //
   bool GetArgument(ArgumentPtr& arg);

   //  Returns true and creates PARMS on finding a template declaration.
   //
   bool GetTemplateParms(TemplateParmsPtr& parms);

   //  Returns true and creates PARM on finding a template parameter.
   //
   bool GetTemplateParm(TemplateParmPtr& parm);

   //  Returns true and creates ARG on finding a template argument.
   //
   bool GetTemplateArg(TemplateArgPtr& arg);

   //  Returns true and creates or updates NAME on finding a name that could be
   //  qualified.  If NAME ends in "operator", the operator that follows it is
   //  also parsed.  A CONSTRAINT of TypeKeyword allows the name to be a type,
   //  such as int.
   //
   bool GetQualName(QualNamePtr& name, Constraint constraint = NonKeyword);

   //  Checks if NAME is a built-in type or a keyword that is an invalid type.
   //
   bool CheckType(const QualNamePtr& name);

   //  Invoked when NAME (of TYPE) began with long, short, signed, or unsigned.
   //
   bool GetCompoundType(const QualName* name, Cxx::Type type);

   //  Returns true and creates or updates TYPE on finding a typed name, which
   //  may include a template signature.  CONSTRAINT specifies whether the name
   //  may contain keywords that are types (e.g. int).
   //
   bool GetTypeName(TypeNamePtr& type, Constraint constraint = NonKeyword);

   //  Returns true and creates NAME on finding an identifier.  This is used to
   //  to parse constructor and destructor names, in which the contents of angle
   //  brackets are included in the name when parsing a template instance.
   //
   bool GetName(std::string& name);

   //  Updates SPEC with tags that follow the current parse location.  These
   //  include
   //  o an unbounded array tag ("[]")
   //  o pointer tags ("*"), each of which may be const
   //  o reference tags ("&" or "&&")
   //  o trailing "const" for the underlying type (any "const" that precedes
   //    or immediately follows the underlying type has already been parsed)
   //  Returns false if an error was detected.
   //
   bool GetTypeTags(TypeSpec* spec);

   //  Returns the number of pointers ('*') that follow the current parse
   //  location.
   //
   size_t GetPointers();

   //  Returns true and creates BLOCK on finding a code block.
   //
   bool GetBlock(BlockPtr& block);

   //  Returns true and creates EXPR on finding an expression.  END delimits
   //  the expression, which should finish at END - 1.  If FORCE is set, the
   //  expression is evaluated when END is reached.
   //
   bool GetCxxExpr(ExprPtr& expr, size_t end, bool force = true);

   //  Returns true and creates EXPR on finding an expression enclosed by
   //  parentheses.  If OMIT is set, the left parenthesis has already been
   //  parsed; otherwise, it must be the next character.  If OPT is set,
   //  the expression is optional.
   //
   bool GetParExpr(ExprPtr& expr, bool omit, bool opt = false);

   //  Updates EXPR with the results of parsing an expression upon reaching
   //  an alphabetic character.
   //
   bool GetCxxAlpha(const ExprPtr& expr);

   //  Updates EXPR with the results of parsing an expression upon reaching
   //  an alphabetic character that could be an encoding tag for a character
   //  or string literal.
   //
   bool GetCxxLiteralOrAlpha(const ExprPtr& expr);

   //  Updates EXPR with the results of parsing an expression upon reaching
   //  a punctuation character.  CXX is true when parsing C++ and is false
   //  when parsing preprocessor directives.
   //
   bool GetOp(const ExprPtr& expr, bool cxx);

   //  Updates EXPR with the results of parsing a literal.  CODE specifies
   //  the encoding tag, if any, that preceded a character or string literal.
   //  POS is the position of the ' or ".
   //
   bool GetNum(Expression* expr);
   bool GetChar(Expression* expr, Cxx::Encoding code, size_t pos);
   bool GetStr(Expression* expr, Cxx::Encoding code, size_t pos);

   //  Updates EXPR with the results of parsing an expression enclosed by
   //  parentheses.  It tries, in order,
   //  o GetArgList to handle a function call,
   //  o GetCast to handle a C-style cast, and
   //  o GetPrecedence to handle parentheses that the order of evaluation.
   //
   bool HandleParentheses(const ExprPtr& expr);
   bool GetArgList(TokenPtr& call);
   bool GetCast(Expression* expr);
   bool GetPrecedence(Expression* expr);

   //  Updates EXPR with the results of parsing an expression that begins
   //  with a '~' at POS, which could either be a ones complement operator
   //  or a direct destructor invocation.  POS was the location of the '~',
   //  which has already been parsed as an operator.
   //
   bool HandleTilde(Expression* expr, size_t pos);

   //  Updates EXPR with the results of parsing an array index found at POS.
   //
   bool GetSubscript(Expression* expr, size_t pos);

   //  Updates TOKEN with the results of parsing a brace initialization list.
   //  On success, updates END to the position of the closing brace.
   //
   bool GetBraceInit(TokenPtr& token, size_t& end);

   //  Updates EXPR with the results of parsing a brace initialization list,
   //  which is wrapped in the expression.
   //
   bool GetBraceInit(ExprPtr& expr);

   //  Updates EXPR with the results of parsing an expression that begins with
   //  the specified operator, which is located at POS.
   //
   bool GetAlignOf(Expression* expr, size_t pos);
   bool GetCxxCast(Expression* expr, Cxx::Operator op, size_t pos);
   bool GetConditional(Expression* expr, size_t pos);
   bool GetDefined(Expression* expr, size_t pos);
   bool GetDelete(Expression* expr, Cxx::Operator op, size_t pos);
   bool GetNew(Expression* expr, Cxx::Operator op, size_t pos);
   bool GetNoExcept(Expression* expr, size_t pos);
   bool GetSizeOf(Expression* expr, size_t pos);
   bool GetThrow(Expression* expr, size_t pos);
   bool GetTypeId(Expression* expr, size_t pos);

   //  Updates STATEMENT with the results of parsing a statement that begins
   //  with the specified keyword.  GetBasic handles assignments, function
   //  calls, null statements, and labels.
   //
   bool GetAsm(AsmPtr& statement);
   bool GetBasic(TokenPtr& statement);
   bool GetBreak(TokenPtr& statement);
   bool GetCase(TokenPtr& statement);
   bool GetCatch(TokenPtr& statement);
   bool GetContinue(TokenPtr& statement);
   bool GetDefault(TokenPtr& statement);
   bool GetDo(TokenPtr& statement);
   bool GetFor(TokenPtr& statement);
   bool GetGoto(TokenPtr& statement);
   bool GetIf(TokenPtr& statement);
   bool GetReturn(TokenPtr& statement);
   bool GetStaticAssert(StaticAssertPtr& statement);
   bool GetSwitch(TokenPtr& statement);
   bool GetTry(TokenPtr& statement);
   bool GetWhile(TokenPtr& statement);

   //  Updates STR to the next keyword.  Returns Cxx::NIL_KEYWORD if the
   //  next token is not a keyword.
   //
   Cxx::Keyword NextKeyword(std::string& str);

   //  Returns true if the next keyword is STR.
   //
   bool NextKeywordIs(NodeBase::c_string str);

   //  Returns the current parse position.
   //
   size_t CurrPos() const;

   //  Logs WARNING at POS.  If POS is not specified, the last position where
   //  parsing started is used.
   //
   void Log(Warning warning, size_t pos = std::string::npos) const;

   //  Invoked when an attempted parse fails.  Records CAUSE if POS is the
   //  farthest point reached in the parse and returns lexer_.Retreat(pos).
   //
   bool Backup(size_t pos, size_t cause);

   //  The same as Backup, but used when the lexer's current position has
   //  not changed (i.e. when the first thing searched for wasn't found).
   //
   static bool Backup(size_t cause);

   //  The same as Backup, but also deletes FUNC.  FUNC needs to be deleted
   //  immediately when it may have pushed a new scope, which must be popped
   //  in case another function parse is attempted.  If that parse succeeds,
   //  the Function constructor invokes OpenScope, picking up the scope set
   //  by the function that failed to parse.  This occurs because the failed
   //  function would normally not be deleted until its FunctionPtr releases
   //  it to acquire the new function, which can only occur *after* the new
   //  function has been constructed.
   //
   bool Backup(size_t pos, FunctionPtr& func, size_t cause);

   //  When the parsing of an expression fails, this is invoked to add the
   //  unparsed string to EXPR.  The string extends from the current parse
   //  location to END.  A "<@" prefix and "@>" suffix are also added to the
   //  string.  CAUSE is the same as for Backup and Retreat.  Returns false.
   //
   bool Skip(size_t end, Expression* expr, size_t cause = 0);

   //  Invoked when the parse fails.  VENUE identifies what was being parsed
   //  (usually venue_).
   //
   void Failure(const std::string& venue) const;

   //  Returns a string of blanks based on the depth of parsing.
   //
   static std::string Indent();

   //  Identifies what is being parsed.
   //
   SourceType source_;

   //  The name of the code being parsed.  Included in logs.
   //
   std::string venue_;

   //  The template's name and arguments if parsing a template instance.
   //
   const TypeName* inst_;

   //  The lexical analyzer.
   //
   Lexer lexer_;

   //  The time when the parse started.
   //
   const NodeBase::SystemTime::Point time_;

   //  The position where the most recent keyword began.
   //
   size_t kwdBegin_;

   //  The deepest point where backup occurred.
   //
   size_t farthest_;

   //  Why backup at farthest_ occurred.  This is simply a location in
   //  the parser code but could be mapped to a text explanation.
   //
   size_t cause_;
};
}
#endif
