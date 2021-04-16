//==============================================================================
//
//  Parser.cpp
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
#include "Parser.h"
#include <cctype>
#include <cstdio>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <utility>
#include "CodeFile.h"
#include "CxxArea.h"
#include "CxxCharLiteral.h"
#include "CxxDirective.h"
#include "CxxExecute.h"
#include "CxxNamed.h"
#include "CxxRoot.h"
#include "CxxScope.h"
#include "CxxScoped.h"
#include "CxxStatement.h"
#include "CxxStrLiteral.h"
#include "CxxSymbols.h"
#include "CxxToken.h"
#include "Debug.h"
#include "Element.h"
#include "Formatters.h"
#include "Log.h"
#include "Singleton.h"
#include "SysFile.h"
#include "SysThreadStack.h"
#include "ThisThread.h"

using namespace NodeBase;
using std::ostream;
using std::setw;
using std::string;

//------------------------------------------------------------------------------

namespace CodeTools
{
uint32_t Parser::Backups[] = { 0 };

//------------------------------------------------------------------------------

Parser::Parser(const string& opts) :
   source_(IsUnknown),
   inst_(nullptr),
   opts_(opts),
   depth_(0),
   kwdBegin_(string::npos),
   farthest_(0),
   cause_(0),
   pTrace_(nullptr)
{
   Debug::ft("Parser.ctor(opts)");

   //  Save the options that control generation of a parse trace file (on
   //  a per-file basis) and object code file (a single file).  Make this
   //  the active parser.
   //
   if(opts_ != EMPTY_STR)
   {
      if((opts_.find(SaveParseTrace) != string::npos) &&
         (opts_.find(TraceParse) == string::npos))
      {
         opts_.push_back(TraceParse);
      }

      Context::SetOptions(opts_);
   }

   Context::PushParser(this);
}

//------------------------------------------------------------------------------

Parser::Parser(CxxScope* scope) :
   source_(IsUnknown),
   inst_(nullptr),
   opts_(EMPTY_STR),
   depth_(0),
   kwdBegin_(string::npos),
   farthest_(0),
   cause_(0),
   pTrace_(nullptr)
{
   Debug::ft("Parser.ctor(scope)");

   //  Make this the active parser and set the scope for parsing.
   //
   Context::PushParser(this);
   Context::PushScope(scope, false);
}

//------------------------------------------------------------------------------

Parser::~Parser()
{
   Debug::ftnt("Parser.dtor");

   //  Remove the parser and close the parse trace file, if any.
   //
   if(Context::Optional() != nullptr) Fault(EndifExpected);
   Context::PopParser(this);
   pTrace_.reset();
}

//------------------------------------------------------------------------------
//
//  Current causes are 1 to 259.
//
bool Parser::Backup(size_t cause)
{
   Debug::ft("Parser.Backup(cause)");

   ++Backups[cause];
   return false;
}

//------------------------------------------------------------------------------

bool Parser::Backup(size_t pos, size_t cause)
{
   Debug::ft("Parser.Backup(pos, cause)");

   auto curr = CurrPos();

   if(curr >= farthest_)
   {
      farthest_ = curr;
      cause_ = cause;
   }

   ++Backups[cause];
   return lexer_.Retreat(pos);
}

//------------------------------------------------------------------------------

bool Parser::Backup(size_t pos, FunctionPtr& func, size_t cause)
{
   Debug::ft("Parser.Backup(pos, cause, func)");

   func.reset();
   return Backup(pos, cause);
}

//------------------------------------------------------------------------------

fn_name Parser_CheckType = "Parser.CheckType";

bool Parser::CheckType(QualNamePtr& name)
{
   Debug::ft(Parser_CheckType);

   //  This only applies when TYPE is unqualified.
   //
   if(name->Size() != 1) return true;

   auto type = Cxx::GetType(name->Name());
   auto root = Singleton< CxxRoot >::Instance();

   switch(type)
   {
   case Cxx::NIL_TYPE:
      //
      //  NAME was not a reserved word, so assume it is a user-defined type.
      //
      return true;
   case Cxx::AUTO_TYPE:
      name->SetReferent(root->AutoTerm(), nullptr);
      return true;
   case Cxx::BOOL:
      name->SetReferent(root->BoolTerm(), nullptr);
      return true;
   case Cxx::CHAR:
      name->SetReferent(root->CharTerm(), nullptr);
      return true;
   case Cxx::CHAR16:
      name->SetReferent(root->Char16Term(), nullptr);
      return true;
   case Cxx::CHAR32:
      name->SetReferent(root->Char32Term(), nullptr);
      return true;
   case Cxx::DOUBLE:
      name->SetReferent(root->DoubleTerm(), nullptr);
      return true;
   case Cxx::FLOAT:
      name->SetReferent(root->FloatTerm(), nullptr);
      return true;
   case Cxx::INT:
      name->SetReferent(root->IntTerm(), nullptr);
      return true;
   case Cxx::NULLPTR_TYPE:
      name->SetReferent(root->NullptrtTerm(), nullptr);
      return true;
   case Cxx::VOID:
      name->SetReferent(root->VoidTerm(), nullptr);
      return true;
   case Cxx::LONG:
   case Cxx::SHORT:
   case Cxx::SIGNED:
   case Cxx::UNSIGNED:
      return GetCompoundType(name, type);
   case Cxx::WCHAR:
      name->SetReferent(root->wCharTerm(), nullptr);
      return true;

   case Cxx::NON_TYPE:
      //
      //  This screens out reserved words (delete, new, and throw) that can
      //  erroneously be parsed as types.  For example, "delete &x;" can be
      //  parsed as the data declaration "delete& x;".
      //
      return false;

   default:
      Debug::SwLog(Parser_CheckType, name->Name(), type, false);
   }

   return false;
}

//------------------------------------------------------------------------------

size_t Parser::CurrPos() const
{
   auto curr = lexer_.Curr();

   //  See if a tracepoint has been hit.
   //
   if(Context::CheckPos_)
   {
      Context::OnLine(lexer_.GetLineNum(curr), false);
   }

   return curr;
}

//------------------------------------------------------------------------------

void Parser::DisplayStats(ostream& stream)
{
   Debug::ft("Parser.DisplayStats");

   stream << "Cause       Count" << CRLF;

   for(size_t i = 0; i <= MaxCause; ++i)
   {
      if(Backups[i] > 0)
      {
         stream << setw(5) << i << setw(12) << Backups[i] << CRLF;
      }
   }
}

//------------------------------------------------------------------------------

void Parser::Enter(SourceType source, const string& venue,
   const TypeName* inst, const string& code, bool preprocess, CodeFile* file)
{
   Debug::ft("Parser.Enter");

   source_ = source;
   venue_ = venue;
   inst_ = inst;
   farthest_ = 0;
   cause_ = 0;
   lexer_.Initialize(code, file);
   if(preprocess) lexer_.PreprocessSource();
}

//------------------------------------------------------------------------------

fn_name Parser_Failure = "Parser.Failure";

void Parser::Failure(const string& venue) const
{
   Debug::ft(Parser_Failure);  //@

   auto code = lexer_.MarkPos(farthest_);
   auto line = lexer_.GetLineNum(farthest_);
   std::ostringstream text;
   text << venue << ", line " << line + 1 << ": " << code;
   Debug::SwLog(Parser_Failure, text.str(), cause_, false);
}

//------------------------------------------------------------------------------

fn_name Parser_Fault = "Parser.Fault";

bool Parser::Fault(DirectiveError err) const
{
   Debug::ft(Parser_Fault);

   auto curr = CurrPos();
   auto code = lexer_.MarkPos(curr);
   auto line = lexer_.GetLineNum(curr);
   std::ostringstream text;
   text << venue_ << ", line " << line + 1 << ':' << CRLF << Log::Tab << code;
   Debug::SwLog(Parser_Fault, text.str(), err, false);
   return false;
}

//------------------------------------------------------------------------------

bool Parser::GetAccess(Cxx::Keyword kwd, Cxx::Access& access)
{
   Debug::ft("Parser.GetAccess");

   //  <Access> = ("public" | "protected" | "private") ":"
   //  The keyword has already been parsed.
   //
   switch(kwd)
   {
   case Cxx::PUBLIC:
      access = Cxx::Public;
      break;
   case Cxx::PROTECTED:
      access = Cxx::Protected;
      break;
   case Cxx::PRIVATE:
      access = Cxx::Private;
      break;
   }

   return lexer_.NextStringIs(":");
}

//------------------------------------------------------------------------------

bool Parser::GetAlignAs(AlignAsPtr& align)
{
   Debug::ft("Parser.GetAlignAs");

   auto start = CurrPos();

   if(!NextKeywordIs(ALIGNAS_STR)) return true;
   if(!lexer_.NextCharIs('(')) return false;
   auto end = lexer_.FindClosing('(', ')');
   if(end == string::npos) return false;

   TokenPtr token;
   TypeSpecPtr spec;
   ExprPtr expr;

   if(GetTypeSpec(spec))
      token.reset(spec.release());
   else if(GetCxxExpr(expr, end))
      token.reset(expr.release());
   else
      return false;

   if(!lexer_.NextCharIs(')')) return false;
   token->SetContext(start);
   align.reset(new AlignAs(token));
   return true;
}

//------------------------------------------------------------------------------

fn_name Parser_GetAlignOf = "Parser.GetAlignOf";

bool Parser::GetAlignOf(ExprPtr& expr, size_t pos)
{
   Debug::ft(Parser_GetAlignOf);

   auto start = CurrPos();

   //  The alignof operator has already been parsed.  Its argument is a type.
   //
   TypeSpecPtr spec;
   if(!lexer_.NextCharIs('(')) return Backup(start, 233);
   if(!GetTypeSpec(spec)) return Backup(start, 234);
   if(!lexer_.NextCharIs(')')) return Backup(start, 235);

   TokenPtr arg;
   arg.reset(spec.release());

   TokenPtr token(new Operation(Cxx::ALIGNOF_TYPE));
   token->SetContext(pos);
   auto op = static_cast< Operation* >(token.get());
   op->AddArg(arg, false);
   expr->AddItem(token);
   return Success(Parser_GetAlignOf, start);
}

//------------------------------------------------------------------------------

fn_name Parser_GetArgList = "Parser.GetArgList";

bool Parser::GetArgList(TokenPtr& call)
{
   Debug::ft(Parser_GetArgList);

   auto prev = lexer_.Prev();
   auto start = CurrPos();

   //  The left parenthesis has already been parsed.
   //
   TokenPtrVector temps;
   ExprPtr expr;

   if(!lexer_.NextCharIs(')'))
   {
      while(true)
      {
         auto end = lexer_.FindFirstOf(",)");
         if(end == string::npos) return Backup(start, 1);
         if(!GetCxxExpr(expr, end)) return Backup(start, 2);
         TokenPtr arg(expr.release());
         temps.push_back(std::move(arg));
         if(lexer_.NextCharIs(')')) break;
         if(!lexer_.NextCharIs(',')) return Backup(start, 3);
      }
   }

   call.reset(new Operation(Cxx::FUNCTION_CALL));
   call->SetContext(prev);
   auto op = static_cast< Operation* >(call.get());

   for(size_t i = 0; i < temps.size(); ++i)
   {
      TokenPtr arg(temps.at(i).release());
      op->AddArg(arg, false);
   }

   return Success(Parser_GetArgList, start);
}

//------------------------------------------------------------------------------

fn_name Parser_GetArgument = "Parser.GetArgument";

bool Parser::GetArgument(ArgumentPtr& arg)
{
   Debug::ft(Parser_GetArgument);

   //  <Argument> = <TypeSpec> [<Name>] [<ArraySpec>] ["=" <Expr>]
   //
   auto start = CurrPos();

   TypeSpecPtr typeSpec;
   string argName;

   if(!GetTypeSpec(typeSpec, argName)) return Backup(4);
   auto pos = CurrPos();

   //  If the argument was a function type, argName was set to its name,
   //  if any.  For other arguments, the name follows the TypeSpec.
   //
   if(typeSpec->GetFuncSpec() == nullptr)
   {
      if(!lexer_.GetName(argName))
      {
         arg.reset(new Argument(argName, typeSpec));
         arg->SetContext(pos);
         return Success(Parser_GetArgument, start);
      }
   }

   ArraySpecPtr arraySpec;
   while(GetArraySpec(arraySpec)) typeSpec->AddArray(arraySpec);

   ExprPtr preset;
   if(lexer_.NextStringIs("="))
   {
      //  Get the argument's default value.
      //
      auto end = lexer_.FindFirstOf(",)");
      if(end == string::npos) return Backup(start, 5);
      if(!GetCxxExpr(preset, end)) return Backup(start, 6);
   }

   arg.reset(new Argument(argName, typeSpec));
   arg->SetContext(pos);
   arg->SetDefault(preset);
   return Success(Parser_GetArgument, start);
}

//------------------------------------------------------------------------------

fn_name Parser_GetArguments = "Parser.GetArguments";

bool Parser::GetArguments(FunctionPtr& func)
{
   Debug::ft(Parser_GetArguments);

   //  <Arguments> = "(" [<Argument>] ["," <Argument>]* ")"
   //  The left parenthesis has already been parsed.  Looking for the right
   //  parenthesis immediately is an optimization for the no arguments case.
   //
   auto start = CurrPos();
   if(lexer_.NextCharIs(')')) return Success(Parser_GetArguments, start);

   ArgumentPtr arg;

   if(GetArgument(arg))
   {
      func->AddArg(arg);

      while(lexer_.NextCharIs(','))
      {
         if(!GetArgument(arg)) return Backup(start, 7);
         func->AddArg(arg);
      }
   }

   if(!lexer_.NextCharIs(')')) return Backup(start, 8);
   return Success(Parser_GetArguments, start);
}

//------------------------------------------------------------------------------

fn_name Parser_GetArraySpec = "Parser.GetArraySpec";

bool Parser::GetArraySpec(ArraySpecPtr& array)
{
   Debug::ft(Parser_GetArraySpec);

   //  <ArraySpec> = "[" [<Expr>] "]"
   //
   auto start = CurrPos();

   //  If a left bracket is found, extract any expression between it
   //  and the right bracket.  Note that the expression can be empty.
   //
   if(!lexer_.NextCharIs('[')) return Backup(9);
   auto end = lexer_.FindClosing('[', ']');
   if(end == string::npos) return Backup(start, 10);

   ExprPtr size;
   GetCxxExpr(size, end);
   if(!lexer_.NextCharIs(']')) return Backup(start, 11);
   array.reset(new ArraySpec(size));
   array->SetContext(start);
   return Success(Parser_GetArraySpec, start);
}

//------------------------------------------------------------------------------

fn_name Parser_GetAsm = "Parser.GetAsm";

bool Parser::GetAsm(AsmPtr& statement)
{
   Debug::ft(Parser_GetAsm);

   //  The "asm" keyword has already been parsed.  It should be
   //  followed by a string within parentheses.
   //
   auto begin = kwdBegin_;
   auto start = CurrPos();

   if(!lexer_.NextCharIs('(')) return Backup(start, 236);
   auto rpar = lexer_.FindClosing('(', ')');
   if(rpar == string::npos) return Backup(start, 237);

   ExprPtr code;
   if(!GetCxxExpr(code, rpar)) return Backup(start, 238);
   if(!lexer_.NextCharIs(')')) return Backup(start, 239);
   if(!lexer_.NextCharIs(';')) return Backup(start, 240);
   statement.reset(new Asm(code));
   return Success(Parser_GetAsm, begin);
}

//------------------------------------------------------------------------------

fn_name Parser_GetBaseDecl = "Parser.GetBaseDecl";

bool Parser::GetBaseDecl(BaseDeclPtr& base)
{
   Debug::ft(Parser_GetBaseDecl);

   //  <BaseDecl> = ":" <Access> <QualName>
   //
   auto start = CurrPos();
   if(!lexer_.NextStringIs(":")) return Backup(12);

   Cxx::Access access;
   QualNamePtr baseName;
   if(!lexer_.GetAccess(access)) return Backup(start, 13);
   if(!GetQualName(baseName)) return Backup(start, 14);
   base.reset(new BaseDecl(baseName, access));
   base->SetContext(start);
   return Success(Parser_GetBaseDecl, start);
}

//------------------------------------------------------------------------------

fn_name Parser_GetBasic = "Parser.GetBasic";

bool Parser::GetBasic(TokenPtr& statement)
{
   Debug::ft(Parser_GetBasic);

   auto start = CurrPos();

   //  An expression statement is an assignment, function call, or null
   //  statement.  The latter is a bare semicolon.
   //
   if(lexer_.NextCharIs(';'))
   {
      statement.reset(new NoOp(start));
      return Success(Parser_GetBasic, start);
   }

   //  If the next sequence is a name followed by a ':', this is a label.
   //  It's treated as a statement, like "default" in a switch statement.
   //  But watch out for a scope resolution operator!
   //
   string name;
   if(lexer_.GetName(name))
   {
      if(lexer_.NextCharIs(':') && !lexer_.NextCharIs(':'))
      {
         statement.reset(new Label(name, start));
         return Success(Parser_GetBasic, start);
      }

      lexer_.Retreat(start);
   }

   ExprPtr expr;
   auto end = lexer_.FindFirstOf(";");
   if(end == string::npos) return Backup(start, 15);
   if(!GetCxxExpr(expr, end, false)) return Backup(start, 16);
   if(!lexer_.NextCharIs(';')) return Backup(start, 17);

   statement.reset(new Expr(expr, start));
   return Success(Parser_GetBasic, start);
}

//------------------------------------------------------------------------------

fn_name Parser_GetBlock = "Parser.GetBlock";

bool Parser::GetBlock(BlockPtr& block)
{
   Debug::ft(Parser_GetBlock);

   //  <Block> = ["{"] [<Statement> ";"]* ["}"]
   //
   auto start = CurrPos();

   auto braced = lexer_.NextCharIs('{');
   block.reset(new Block(braced));
   block->SetContext(start);
   Context::PushScope(block.get(), true);

   while(true)
   {
      GetStatements(block, braced);

      //  GetStatements stops if it reaches a nested block.  If the current
      //  block is braced, parse the nested block.  If not, return so that
      //  any pending statement (for example, an if or while) gets finalized
      //  with the current block, which consist of a single statement.  Not
      //  doing this would cause the nested block to also become part of the
      //  pending statement.
      //
      if(braced && (lexer_.CurrChar() == '{'))
      {
         BlockPtr nested;

         if(GetBlock(nested))
         {
            nested->SetNested();
            block->AddStatement(nested.release());
            continue;
         }
      }
      break;
   }

   Context::PopScope();
   if(braced && !lexer_.NextCharIs('}')) return Backup(start, 18);
   return Success(Parser_GetBlock, start);
}

//------------------------------------------------------------------------------

fn_name Parser_GetBraceInit = "Parser.GetBraceInit";

bool Parser::GetBraceInit(ExprPtr& expr)
{
   Debug::ft(Parser_GetBraceInit);

   auto prev = lexer_.Prev();
   auto start = CurrPos();

   //  The left brace has already been parsed.  A comma is actually allowed
   //  to follow the final item in the list, just before the closing brace.
   //
   auto end = lexer_.FindClosing('{', '}');
   if(end == string::npos) return Backup(start, 19);

   TokenPtrVector temps;
   ExprPtr item;

   if(!lexer_.NextCharIs('}'))
   {
      while(true)
      {
         auto next = lexer_.FindFirstOf(",}");
         if(next == string::npos) return Backup(start, 20);

         if(!GetCxxExpr(item, next))
         {
            if(!lexer_.NextCharIs('{')) break;
            if(!GetBraceInit(item)) break;
         }

         TokenPtr init(item.release());
         temps.push_back(std::move(init));
         auto comma = lexer_.NextCharIs(',');
         auto brace = lexer_.NextCharIs('}');
         if(brace) break;
         if(!comma) return Backup(start, 21);
      }
   }

   TokenPtr token(new BraceInit);
   token->SetContext(prev);
   auto brace = static_cast< BraceInit* >(token.get());

   for(size_t i = 0; i < temps.size(); ++i)
   {
      TokenPtr init(temps.at(i).release());
      brace->AddItem(init);
   }

   expr.reset(new Expression(end, true));
   expr->AddItem(token);
   return Success(Parser_GetBraceInit, start);
}

//------------------------------------------------------------------------------

fn_name Parser_GetBreak = "Parser.GetBreak";

bool Parser::GetBreak(TokenPtr& statement)
{
   Debug::ft(Parser_GetBreak);

   auto begin = kwdBegin_;
   auto start = CurrPos();

   //  The "break" keyword has already been parsed.
   //
   if(!lexer_.NextCharIs(';')) return Backup(start, 22);
   statement.reset(new Break(begin));
   return Success(Parser_GetBreak, begin);
}

//------------------------------------------------------------------------------

fn_name Parser_GetCase = "Parser.GetCase";

bool Parser::GetCase(TokenPtr& statement)
{
   Debug::ft(Parser_GetCase);

   auto begin = kwdBegin_;
   auto start = CurrPos();

   //  The "case" keyword has already been parsed.
   //
   ExprPtr expr;
   auto end = lexer_.FindFirstOf(":");
   if(end == string::npos) return Backup(start, 23);
   if(!GetCxxExpr(expr, end)) return Backup(start, 24);
   if(!lexer_.NextCharIs(':')) return Backup(start, 25);

   statement.reset(new Case(expr, begin));
   return Success(Parser_GetCase, begin);
}

//------------------------------------------------------------------------------

fn_name Parser_GetCast = "Parser.GetCast";

bool Parser::GetCast(ExprPtr& expr)
{
   Debug::ft(Parser_GetCast);

   auto prev = lexer_.Prev();
   auto start = CurrPos();

   //  The left parenthesis has already been parsed.
   //
   TypeSpecPtr spec;
   ExprPtr item;
   if(!GetTypeSpec(spec)) return Backup(26);
   if(!lexer_.NextCharIs(')')) return Backup(start, 27);
   if(!GetCxxExpr(item, expr->EndPos(), false)) return Backup(start, 28);

   TokenPtr token(new Operation(Cxx::CAST));
   token->SetContext(prev);
   auto cast = static_cast< Operation* >(token.get());
   TokenPtr arg1(spec.release());
   TokenPtr arg2(item.release());
   cast->AddArg(arg1, false);
   cast->AddArg(arg2, false);
   expr->AddItem(token);
   return Success(Parser_GetCast, start);
}

//------------------------------------------------------------------------------

fn_name Parser_GetCatch = "Parser.GetCatch";

bool Parser::GetCatch(TokenPtr& statement)
{
   Debug::ft(Parser_GetCatch);

   auto start = CurrPos();

   ArgumentPtr arg;
   BlockPtr handler;
   if(!NextKeywordIs(CATCH_STR)) return Backup(start, 29);
   if(!lexer_.NextCharIs('(')) return Backup(start, 30);

   if(lexer_.Substr(CurrPos(), 3) == ELLIPSES_STR)
   {
      auto end = lexer_.FindClosing('(', ')');
      if(end == string::npos) return Backup(start, 31);
      lexer_.Reposition(end);
   }
   else
   {
      if(!GetArgument(arg)) return Backup(start, 32);
   }

   if(!lexer_.NextCharIs(')')) return Backup(start, 33);
   if(!GetBlock(handler)) return Backup(start, 34);

   statement.reset(new Catch(start));
   auto c = static_cast< Catch* >(statement.get());
   c->AddArg(arg);
   c->AddHandler(handler);
   return Success(Parser_GetCatch, start);
}

//------------------------------------------------------------------------------

bool Parser::GetChar(ExprPtr& expr, Cxx::Encoding code, size_t pos)
{
   Debug::ft("Parser.GetChar");

   //  Extract the character that appears between two single quotation
   //  marks and wrap it in the appropriate type of character literal.
   //
   uint32_t c;
   if(!lexer_.ThisCharIs(APOSTROPHE)) return false;
   if(!lexer_.GetChar(c)) return false;
   if(!lexer_.NextCharIs(APOSTROPHE)) return false;

   TokenPtr item;

   switch(code)
   {
   case Cxx::ASCII:
   case Cxx::U8:
      item = TokenPtr(new CharLiteral(c));
      break;
   case Cxx::U16:
      item = TokenPtr(new u16CharLiteral(c));
      break;
   case Cxx::U32:
      item = TokenPtr(new u32CharLiteral(c));
      break;
   case Cxx::WIDE:
      item = TokenPtr(new wCharLiteral(c));
      break;
   default:
      return false;
   }

   item->SetContext(pos);
   expr->AddItem(item);
   return true;
}

//------------------------------------------------------------------------------

bool Parser::GetClass(Cxx::Keyword kwd, CxxArea* area)
{
   Debug::ft("Parser.GetClass");

   ClassPtr cls;
   ForwardPtr forw;

   if(GetClassDecl(kwd, cls, forw))
   {
      if(cls == nullptr) return area->AddForw(forw);

      auto c = cls.get();
      if(area->AddClass(cls)) return GetInlines(c);
   }

   return false;
}

//------------------------------------------------------------------------------

fn_name Parser_GetClassData = "Parser.GetClassData";

bool Parser::GetClassData(DataPtr& data)
{
   Debug::ft(Parser_GetClassData);

   //  <ClassData> = [<AlignAs>] ["static"] ["thread_local"] ["constexpr"]
   //                ["mutable"] <TypeSpec> <Name> [<ArraySpec>]
   //                [":" <Expr>] ["=" <Expr>] ";"
   //
   auto start = CurrPos();

   AlignAsPtr align;
   KeywordSet attrs;
   TypeSpecPtr typeSpec;
   string dataName;
   ArraySpecPtr arraySpec;
   ExprPtr width;
   ExprPtr init;

   if(!GetAlignAs(align)) return Backup(start, 251);
   lexer_.GetDataTags(attrs);
   auto stat = (attrs.find(Cxx::STATIC) != attrs.cend());
   auto tloc = (attrs.find(Cxx::THREAD_LOCAL) != attrs.cend());
   auto cexp = (attrs.find(Cxx::CONSTEXPR) != attrs.cend());
   auto mute = (attrs.find(Cxx::MUTABLE) != attrs.cend());
   if(!GetTypeSpec(typeSpec, &attrs)) return Backup(start, 35);
   auto pos = CurrPos();
   if(!lexer_.GetName(dataName)) return Backup(start, 36);
   while(GetArraySpec(arraySpec)) typeSpec->AddArray(arraySpec);

   if(lexer_.NextStringIs(":"))
   {
      //  Get the data's field width.
      //
      auto end = lexer_.FindFirstOf(";=");
      if(end == string::npos) return Backup(start, 37);
      if(!GetCxxExpr(width, end)) return Backup(start, 38);
   }

   auto eqpos = CurrPos();

   if(lexer_.NextStringIs("="))
   {
      if(lexer_.NextCharIs('{'))
      {
         if(!GetBraceInit(init)) return Backup(start, 39);
      }
      else
      {
         auto end = lexer_.FindFirstOf(";");
         if(end == string::npos) return Backup(start, 40);
         if(!GetCxxExpr(init, end)) return Backup(start, 41);
      }
   }

   if(!lexer_.NextCharIs(';')) return Backup(start, 42);
   data.reset(new ClassData(dataName, typeSpec));
   data->SetContext(pos);
   data->SetAlignment(align);
   data->SetStatic(stat);
   data->SetThreadLocal(tloc);
   data->SetConstexpr(cexp);
   static_cast< ClassData* >(data.get())->SetMutable(mute);
   static_cast< ClassData* >(data.get())->SetWidth(width);
   data->SetAssignment(init, eqpos);
   return Success(Parser_GetClassData, start);
}

//------------------------------------------------------------------------------

fn_name Parser_GetClassDecl = "Parser.GetClassDecl";

bool Parser::GetClassDecl(Cxx::Keyword kwd, ClassPtr& cls, ForwardPtr& forw)
{
   Debug::ft(Parser_GetClassDecl);

   //  <Class> = [<TemplateParms>] <ClassTag> <QualName>
   //            [ [<BaseDecl>] "{" [<MemberDecl>]* "}" ] ";"
   //  The initial keyword has already been parsed unless it is "template".
   //
   auto begin = kwdBegin_;
   auto start = CurrPos();

   TemplateParmsPtr parms;
   Cxx::ClassTag tag = Cxx::ClassType;

   switch(kwd)
   {
   case Cxx::STRUCT:
      tag = Cxx::StructType;
      break;
   case Cxx::UNION:
      tag = Cxx::UnionType;
      break;
   case Cxx::TEMPLATE:
      if(!GetTemplateParms(parms)) return Backup(start, 43);
      begin = CurrPos();
      if(!lexer_.GetClassTag(tag)) return Backup(start, 44);
   }

   AlignAsPtr align;
   QualNamePtr className;
   if(!GetAlignAs(align)) return Backup(start, 252);
   if(!GetQualName(className))
   {
      if(tag != Cxx::UnionType) return Backup(start, 45);
      className.reset(new QualName(EMPTY_STR));
      className->SetContext(CurrPos());
   }

   if(lexer_.NextCharIs(';'))
   {
      //  A forward declaration.
      //
      forw.reset(new Forward(className, tag));
      forw->SetContext(begin);
      forw->SetTemplateParms(parms);
      return Success(Parser_GetClassDecl, begin);
   }

   BaseDeclPtr base;
   GetBaseDecl(base);
   if(!lexer_.NextCharIs('{')) return Backup(start, 46);
   cls.reset(new Class(className, tag));
   cls->SetContext(begin);
   cls->SetTemplateParms(parms);
   Context::PushScope(cls.get(), false);
   cls->SetAlignment(align);
   cls->AddBase(base);
   GetMemberDecls(cls.get());
   Context::PopScope();
   if(!lexer_.NextCharIs('}')) return Backup(start, 47);
   if(!lexer_.NextCharIs(';')) return Backup(start, 48);
   return Success(Parser_GetClassDecl, begin);
}

//------------------------------------------------------------------------------

bool Parser::GetCompoundType(QualNamePtr& name, Cxx::Type type)
{
   Debug::ft("Parser.GetCompoundType");

   int sign = 0;  // -1 = signed, 0 = unspecified, 1 = unsigned
   int size = 0;  // -1 = short, 0 = unspecified, 1 = long, 2 = long long

   for(auto pass = 0; true; ++pass)
   {
      switch(type)
      {
      case Cxx::CHAR:
         if(size != 0) return false;
         if(pass > 0) name->Append(CHAR_STR, true);
         return SetCompoundType(name, Cxx::CHAR, 0, sign);

      case Cxx::DOUBLE:
         if((size < 0) || (size > 1) || (sign != 0)) return false;
         if(pass > 0) name->Append(DOUBLE_STR, true);
         return SetCompoundType(name, Cxx::DOUBLE, size, 0);

      case Cxx::INT:
         if(pass > 0) name->Append(INT_STR, true);
         return SetCompoundType(name, type, size, sign);

      case Cxx::LONG:
         if(size < 0) return false;
         if(size > 1) return false;
         if(pass > 0) name->Append(LONG_STR, true);
         ++size;
         break;

      case Cxx::SHORT:
         if(size != 0) return false;
         if(pass > 0) name->Append(SHORT_STR, true);
         size = -1;
         break;

      case Cxx::SIGNED:
         if(sign != 0) return false;
         if(pass > 0) name->Append(SIGNED_STR, true);
         sign = -1;
         break;

      case Cxx::UNSIGNED:
         if(sign != 0) return false;
         if(pass > 0) name->Append(UNSIGNED_STR, true);
         sign = 1;
         break;

      case Cxx::BOOL:
      case Cxx::FLOAT:
      case Cxx::VOID:
      case Cxx::NON_TYPE:
         return false;

      default:
         return SetCompoundType(name, Cxx::NIL_TYPE, size, sign);
      }

      type = lexer_.NextType();
   }
}

//------------------------------------------------------------------------------

fn_name Parser_GetConditional = "Parser.GetConditional";

bool Parser::GetConditional(ExprPtr& expr, size_t pos)
{
   Debug::ft(Parser_GetConditional);

   auto start = CurrPos();

   //  The "?" has already been parsed and should have been preceded by
   //  a valid expression.
   //
   ExprPtr exp1;
   ExprPtr exp0;
   auto end = lexer_.FindFirstOf(":");
   if(end == string::npos) return Backup(start, 49);
   if(!GetCxxExpr(exp1, end)) return Backup(start, 50);
   if(!lexer_.NextCharIs(':')) return Backup(start, 51);
   if(!GetCxxExpr(exp0, expr->EndPos(), false)) return Backup(start, 52);

   TokenPtr token(new Operation(Cxx::CONDITIONAL));
   token->SetContext(pos);
   auto cond = static_cast< Operation* >(token.get());
   TokenPtr test(new Elision);
   TokenPtr value1(exp1.release());
   TokenPtr value0(exp0.release());
   cond->AddArg(test, true);
   cond->AddArg(value1, false);
   cond->AddArg(value0, false);
   expr->AddItem(token);
   return Success(Parser_GetConditional, start);
}

//------------------------------------------------------------------------------

fn_name Parser_GetContinue = "Parser.GetContinue";

bool Parser::GetContinue(TokenPtr& statement)
{
   Debug::ft(Parser_GetContinue);

   auto begin = kwdBegin_;
   auto start = CurrPos();

   //  The "continue" keyword has already been parsed.
   //
   if(!lexer_.NextCharIs(';')) return Backup(start, 53);
   statement.reset(new Continue(begin));
   return Success(Parser_GetContinue, begin);
}

//------------------------------------------------------------------------------

fn_name Parser_GetCtorDecl = "Parser.GetCtorDecl";

bool Parser::GetCtorDecl(FunctionPtr& func)
{
   Debug::ft(Parser_GetCtorDecl);

   //  <CtorDecl> = ["inline"] ["explicit"] ["constexpr"]
   //               <Name> <Arguments> [<CtorInit>]
   //
   auto start = CurrPos();

   KeywordSet attrs;
   string name;
   lexer_.GetFuncFrontTags(attrs);
   auto inln = (attrs.find(Cxx::INLINE) != attrs.cend());
   auto expl = (attrs.find(Cxx::EXPLICIT) != attrs.cend());
   auto cexp = (attrs.find(Cxx::CONSTEXPR) != attrs.cend());
   auto pos = CurrPos();
   if(!GetName(name)) return Backup(start, 54);
   if(!lexer_.NextCharIs('(')) return Backup(start, 55);
   QualNamePtr ctorName(new QualName(name));
   ctorName->SetContext(pos);
   func.reset(new Function(ctorName));
   func->SetContext(start);
   func->SetInline(inln);
   func->SetExplicit(expl);
   func->SetConstexpr(cexp);
   if(cexp) func->SetInline(true);
   if(!GetArguments(func)) return Backup(start, func, 214);
   auto noex = NextKeywordIs(NOEXCEPT_STR);
   if(!GetCtorInit(func)) return Backup(start, func, 215);
   func->SetNoexcept(noex);
   return Success(Parser_GetCtorDecl, start);
}

//------------------------------------------------------------------------------

fn_name Parser_GetCtorDefn = "Parser.GetCtorDefn";

bool Parser::GetCtorDefn(FunctionPtr& func)
{
   Debug::ft(Parser_GetCtorDefn);

   //  <CtorDefn> = <QualName> "::" <Name> <Arguments> <CtorInit>
   //
   auto start = CurrPos();

   //  Whether this is a constructor or not, GetQualName will parse the final
   //  scope qualifier and function name, so verify that the function name is
   //  actually repeated.
   //
   QualNamePtr ctorName;
   if(!GetQualName(ctorName)) return Backup(start, 56);
   if(!lexer_.NextCharIs('(')) return Backup(start, 57);
   if(!ctorName->CheckCtorDefn()) return Backup(start, 58);
   func.reset(new Function(ctorName));
   func->SetContext(start);
   if(!GetArguments(func)) return Backup(start, func, 216);
   auto noex = NextKeywordIs(NOEXCEPT_STR);
   if(!GetCtorInit(func)) return Backup(start, func, 217);
   func->SetNoexcept(noex);
   return Success(Parser_GetCtorDefn, start);
}

//------------------------------------------------------------------------------

fn_name Parser_GetCtorInit = "Parser.GetCtorInit";

bool Parser::GetCtorInit(FunctionPtr& func)
{
   Debug::ft(Parser_GetCtorInit);

   //  <CtorInit> = [ ":" [<QualName> "(" <Expr> ")"]
   //                     ["," <Name> "(" <Expr> ")"]* ]
   //
   auto start = CurrPos();
   if(!lexer_.NextStringIs(":")) return Success(Parser_GetCtorInit, start);

   auto end = lexer_.FindFirstOf("{");
   if(end == string::npos) return Backup(start, 59);

   size_t begin;
   QualNamePtr baseName;
   string memberName;
   TokenPtr token;
   if(GetQualName(baseName))
   {
      //  See if baseName is a base class or a member.  If a member, parse
      //  the expression in parentheses as an argument list in case it's a
      //  constructor call.
      //
      begin = CurrPos();
      auto call = false;
      auto cls = func->GetClass();
      if(cls != nullptr)
      {
         auto base = cls->BaseClass();
         if(base != nullptr)
         {
            auto name = baseName->QualifiedName(true, true);
            auto file = Context::File();
            SymbolView view;
            call = base->NameRefersToItem(name, func.get(), file, view);
         }
      }

      if(call)
      {
         token = TokenPtr(baseName.release());
         ExprPtr init(new Expression(end, true));
         init->AddItem(token);
         if(!lexer_.NextCharIs('(')) return Backup(start, 60);
         if(!GetArgList(token)) return Backup(start, 61);
         init->AddItem(token);
         func->SetBaseInit(init);
      }
      else
      {
         if(!lexer_.NextCharIs('(')) return Backup(start, 62);
         end = lexer_.FindClosing('(', ')');
         if(end == string::npos) return Backup(start, 63);
         if(!GetArgList(token)) return Backup(start, 64);
         memberName = baseName->Name();
         MemberInitPtr mem(new MemberInit(func.get(), memberName, token));
         mem->SetContext(begin);
         func->AddMemberInit(mem);
      }
   }

   while(lexer_.NextCharIs(','))
   {
      begin = CurrPos();
      if(!lexer_.GetName(memberName)) return Backup(start, 65);
      if(!lexer_.NextCharIs('(')) return Backup(start, 66);
      end = lexer_.FindClosing('(', ')');
      if(end == string::npos) return Backup(start, 67);
      if(!GetArgList(token)) return Backup(start, 68);
      MemberInitPtr mem(new MemberInit(func.get(), memberName, token));
      mem->SetContext(begin);
      func->AddMemberInit(mem);
   }

   return Success(Parser_GetCtorInit, start);
}

//------------------------------------------------------------------------------

fn_name Parser_GetCxxAlpha = "Parser.GetCxxAlpha";

bool Parser::GetCxxAlpha(ExprPtr& expr)
{
   Debug::ft(Parser_GetCxxAlpha);

   auto start = CurrPos();

   TokenPtr item;
   QualNamePtr qualName;
   if(!GetQualName(qualName, AnyKeyword)) return Backup(start, 69);

   if(qualName->Size() == 1)
   {
      //  See if the name is actually a keyword or operator.
      //
      auto op = Cxx::GetReserved(qualName->Name());

      switch(op)
      {
      case Cxx::NIL_OPERATOR:
         if(!CheckType(qualName)) return Backup(start, 70);
         break;

      case Cxx::FALSE:
      case Cxx::TRUE:
         item.reset(new BoolLiteral(op == Cxx::TRUE));
         item->SetContext(qualName->GetPos());
         if(expr->AddItem(item)) return true;
         return Backup(start, 71);

      case Cxx::NULLPTR:
         item.reset(new NullPtr);
         item->SetContext(qualName->GetPos());
         if(expr->AddItem(item)) return true;
         return Backup(start, 72);

      case Cxx::OBJECT_CREATE:
         if(GetNew(expr, op, qualName->GetPos())) return true;
         return Backup(start, 73);

      case Cxx::OBJECT_DELETE:
         if(lexer_.NextStringIs(ARRAY_STR)) op = Cxx::OBJECT_DELETE_ARRAY;
         if(GetDelete(expr, op, qualName->GetPos())) return true;
         return Backup(start, 74);

      case Cxx::STATIC_CAST:
      case Cxx::CONST_CAST:
      case Cxx::DYNAMIC_CAST:
      case Cxx::REINTERPRET_CAST:
         //
         //  GetQualName also extracted what was in the angle brackets.
         //  Back up so that this cast operator can extract it.
         {
            lexer_.Reposition(start);
            auto pos = lexer_.FindFirstOf("<");
            lexer_.Reposition(pos);
            if(GetCxxCast(expr, op, qualName->GetPos())) return true;
            return Backup(start, 75);
         }

      case Cxx::SIZEOF_TYPE:
         if(GetSizeOf(expr, qualName->GetPos())) return true;
         return Backup(start, 77);

      case Cxx::ALIGNOF_TYPE:
         if(GetAlignOf(expr, qualName->GetPos())) return true;
         return Backup(start, 101);

      case Cxx::THROW:
         if(GetThrow(expr, qualName->GetPos())) return true;
         return Backup(start, 76);

      case Cxx::TYPE_NAME:
         if(GetTypeId(expr, qualName->GetPos())) return true;
         return Backup(start, 78);

      case Cxx::NOEXCEPT:
         if(GetNoExcept(expr, qualName->GetPos())) return true;
         return Backup(start, 228);

      default:
         Debug::SwLog(Parser_GetCxxAlpha, "unexpected operator", op, false);
         return Backup(start, 79);
      }
   }

   item.reset(qualName.release());
   if(expr->AddItem(item)) return true;
   return Backup(start, 80);
}

//------------------------------------------------------------------------------

fn_name Parser_GetCxxCast = "Parser.GetCxxCast";

bool Parser::GetCxxCast(ExprPtr& expr, Cxx::Operator op, size_t pos)
{
   Debug::ft(Parser_GetCxxCast);

   auto start = CurrPos();

   //  The cast operator has already been parsed.
   //
   TypeSpecPtr spec;
   ExprPtr item;
   if(!lexer_.NextCharIs('<')) return Backup(start, 81);
   if(!GetTypeSpec(spec)) return Backup(start, 82);
   if(!lexer_.NextCharIs('>')) return Backup(start, 83);
   if(!GetParExpr(item, false)) return Backup(start, 84);

   TokenPtr token(new Operation(op));
   token->SetContext(pos);
   auto cast = static_cast< Operation* >(token.get());
   TokenPtr arg1(spec.release());
   TokenPtr arg2(item.release());
   cast->AddArg(arg1, false);
   cast->AddArg(arg2, false);
   expr->AddItem(token);
   return Success(Parser_GetCxxCast, start);
}

//------------------------------------------------------------------------------

fn_name Parser_GetCxxExpr = "Parser.GetCxxExpr";

bool Parser::GetCxxExpr(ExprPtr& expr, size_t end, bool force)
{
   Debug::ft(Parser_GetCxxExpr);

   auto start = CurrPos();

   char c;
   expr.reset(new Expression(end, force));

   while(lexer_.CurrChar(c) < end)
   {
      switch(c)
      {
      case QUOTE:
         if(GetStr(expr, Cxx::ASCII, start)) break;
         return Skip(end, expr);

      case APOSTROPHE:
         if(GetChar(expr, Cxx::ASCII, start)) break;
         return Skip(end, expr);

      case '{':
         return false;

      case '_':
         if(GetCxxAlpha(expr)) break;
         return Skip(end, expr);

      case 'u':
      case 'U':
      case 'L':
         if(GetCxxLiteralOrAlpha(expr)) break;
         return Skip(end, expr);

      default:
         if(ispunct(c))
         {
            if(GetOp(expr, true)) break;
            return Backup(start, 85);
         }
         if(isdigit(c))
         {
            if(GetNum(expr)) break;
            return Skip(end, expr);
         }
         if(GetCxxAlpha(expr)) break;
         if(GetOp(expr, true)) break;
         return Skip(end, expr);
      }
   }

   if(expr->Empty())
   {
      expr.reset();
      return Backup(start, 86);
   }

   return Success(Parser_GetCxxExpr, start);
}

//------------------------------------------------------------------------------

bool Parser::GetCxxLiteralOrAlpha(ExprPtr& expr)
{
   Debug::ft("Parser.GetCxxLiteralOrAlpha");

   Cxx::Encoding code = Cxx::Encoding_N;
   char c;
   auto start = lexer_.CurrChar(c);

   switch(c)
   {
   case 'u':
      //
      //  Look for a "u" or "u8" tag.
      //
      if(lexer_.At(start + 1) == '8')
      {
         code = Cxx::U8;
         lexer_.Reposition(start + 2);
      }
      else
      {
         code = Cxx::U16;
         lexer_.Reposition(start + 1);
      }
      break;

   case 'U':
      code = Cxx::U32;
      lexer_.Reposition(start + 1);
      break;

   case 'L':
      code = Cxx::WIDE;
      lexer_.Reposition(start + 1);
      break;
   }

   if(code != Cxx::Encoding_N)
   {
      c = lexer_.CurrChar();
      if(c == QUOTE) return GetStr(expr, code, start);
      if(c == APOSTROPHE) return GetChar(expr, code, start);
   }

   //  This wasn't a character or string literal,
   //  so back up and look for an identifier.
   //
   lexer_.Reposition(start);
   return GetCxxAlpha(expr);
}

//------------------------------------------------------------------------------

fn_name Parser_GetDefault = "Parser.GetDefault";

bool Parser::GetDefault(TokenPtr& statement)
{
   Debug::ft(Parser_GetDefault);

   auto begin = kwdBegin_;
   auto start = CurrPos();

   //  The "default" keyword has already been parsed.
   //
   if(!lexer_.NextCharIs(':')) return Backup(start, 87);
   string label(DEFAULT_STR);
   statement.reset(new Label(label, begin));
   return Success(Parser_GetDefault, begin);
}

//------------------------------------------------------------------------------

fn_name Parser_GetDefined = "Parser.GetDefined";

bool Parser::GetDefined(ExprPtr& expr, size_t pos)
{
   Debug::ft(Parser_GetDefined);

   auto start = CurrPos();

   //  The defined operator has already been parsed.  Parentheses
   //  around the argument are optional.
   //
   string name;

   auto par = lexer_.NextCharIs('(');
   auto mpos = CurrPos();
   if(!lexer_.GetName(name)) return Backup(start, 88);
   if(par && !lexer_.NextCharIs(')')) return Backup(start, 89);

   TokenPtr token(new Operation(Cxx::DEFINED));
   token->SetContext(pos);
   auto op = static_cast< Operation* >(token.get());
   MacroNamePtr macro(new MacroName(name));
   macro->SetContext(mpos);
   TokenPtr arg = std::move(macro);
   op->AddArg(arg, false);
   expr->AddItem(token);
   return Success(Parser_GetDefined, start);
}

//------------------------------------------------------------------------------

fn_name Parser_GetDelete = "Parser.GetDelete";

bool Parser::GetDelete(ExprPtr& expr, Cxx::Operator op, size_t pos)
{
   Debug::ft(Parser_GetDelete);

   auto start = CurrPos();

   //  The delete operator has already been parsed.
   //
   ExprPtr item;
   if(!GetCxxExpr(item, expr->EndPos(), false)) return Backup(start, 90);

   TokenPtr token(new Operation(op));
   token->SetContext(pos);
   auto delOp = static_cast< Operation* >(token.get());
   TokenPtr arg(item.release());
   delOp->AddArg(arg, false);
   expr->AddItem(token);
   return Success(Parser_GetDelete, start);
}

//------------------------------------------------------------------------------

fn_name Parser_GetDo = "Parser.GetDo";

bool Parser::GetDo(TokenPtr& statement)
{
   Debug::ft(Parser_GetDo);

   auto begin = kwdBegin_;
   auto start = CurrPos();

   //  The "do" keyword has already been parsed.
   //
   BlockPtr loop;
   ExprPtr condition;
   if(!GetBlock(loop)) return Backup(start, 91);
   if(!NextKeywordIs(WHILE_STR)) return Backup(start, 92);
   if(!GetParExpr(condition, false)) return Backup(start, 93);
   if(!lexer_.NextCharIs(';')) return Backup(start, 94);

   statement.reset(new Do(begin));
   auto d = static_cast< Do* >(statement.get());
   d->AddLoop(loop);
   d->AddCondition(condition);
   return Success(Parser_GetDo, begin);
}

//------------------------------------------------------------------------------

fn_name Parser_GetDtorDecl = "Parser.GetDtorDecl";

bool Parser::GetDtorDecl(FunctionPtr& func)
{
   Debug::ft(Parser_GetDtorDecl);

   //  <DtorDecl> = ["inline"] ["virtual"] "~" <Name> "(" ")" ["noexcept"]
   //
   auto start = CurrPos();

   KeywordSet attrs;
   lexer_.GetFuncFrontTags(attrs);
   auto inln = (attrs.find(Cxx::INLINE) != attrs.cend());
   auto virt = (attrs.find(Cxx::VIRTUAL) != attrs.cend());
   if(!lexer_.NextCharIs('~')) return Backup(start, 95);

   string name;
   auto pos = CurrPos();
   if(!GetName(name)) return Backup(start, 96);
   name.insert(0, 1, '~');
   QualNamePtr dtorName(new QualName(name));
   dtorName->SetContext(pos);
   func.reset(new Function(dtorName));
   func->SetContext(start);
   func->SetInline(inln);
   func->SetVirtual(virt);
   if(!lexer_.NextCharIs('(')) return Backup(start, 97);
   if(!GetArguments(func)) return Backup(start, func, 98);
   auto noex = NextKeywordIs(NOEXCEPT_STR);
   func->SetNoexcept(noex);
   return Success(Parser_GetDtorDecl, start);
}

//------------------------------------------------------------------------------

fn_name Parser_GetDtorDefn = "Parser.GetDtorDefn";

bool Parser::GetDtorDefn(FunctionPtr& func)
{
   Debug::ft(Parser_GetDtorDefn);

   //  <DtorDefn> = <QualName> "::~" <Name> "(" ")" ["noexcept"]
   //
   //  The entire name, including the '~', is parsed as qualified name,
   //  so check that it actually contains a '~'.
   //
   auto start = CurrPos();

   QualNamePtr dtorName;
   if(!GetQualName(dtorName)) return Backup(start, 99);
   auto name = dtorName->QualifiedName(true, false);
   if(name.find('~') == string::npos) return Backup(start, 100);
   func.reset(new Function(dtorName));
   func->SetContext(start);
   if(!lexer_.NextCharIs('(')) return Backup(start, 102);
   if(!GetArguments(func)) return Backup(start, func, 103);
   auto noex = NextKeywordIs(NOEXCEPT_STR);
   func->SetNoexcept(noex);
   return Success(Parser_GetDtorDefn, start);
}

//------------------------------------------------------------------------------

fn_name Parser_GetEnum = "Parser.GetEnum";

bool Parser::GetEnum(EnumPtr& decl)
{
   Debug::ft(Parser_GetEnum);

   //  <Enum> = "enum" [<AlignAs>] [<Name>]
   //           "{" <Enumerator> ["," <Enumerator>]* "}" ";"
   //  The "enum" keyword has already been parsed.  An enum without enumerators
   //  is legal but seems to be useless and is therefore not supported.  After
   //  the last enumerator, a comma can actually precede the brace.
   //
   auto begin = kwdBegin_;
   auto start = CurrPos();

   AlignAsPtr align;
   string enumName;
   TypeSpecPtr typeSpec;
   if(!GetAlignAs(align)) return Backup(start, 253);
   lexer_.GetName(enumName);

   if(lexer_.NextCharIs(':'))
   {
      if(!GetTypeSpec(typeSpec)) return Backup(start, 229);
   }

   if(!lexer_.NextCharIs('{')) return Backup(start, 104);

   string etorName;
   ExprPtr etorInit;
   auto etorPos = CurrPos();
   if(!GetEnumerator(etorName, etorInit)) return Backup(start, 105);
   decl.reset(new Enum(enumName));
   decl->SetContext(begin);
   decl->SetAlignment(align);
   decl->AddType(typeSpec);
   decl->AddEnumerator(etorName, etorInit, etorPos);

   while(true)
   {
      if(!lexer_.NextCharIs(',')) break;
      etorPos = CurrPos();
      if(!GetEnumerator(etorName, etorInit)) break;
      decl->AddEnumerator(etorName, etorInit, etorPos);
   }

   if(!lexer_.NextCharIs('}')) return Backup(start, 106);
   if(!lexer_.NextCharIs(';')) return Backup(start, 107);
   return Success(Parser_GetEnum, begin);
}

//------------------------------------------------------------------------------

fn_name Parser_GetEnumerator = "Parser.GetEnumerator";

bool Parser::GetEnumerator(string& name, ExprPtr& init)
{
   Debug::ft(Parser_GetEnumerator);

   auto start = CurrPos();

   //  <Enumerator> = <Name> ["=" <Expr>]
   //
   if(!lexer_.GetName(name)) return Backup(start, 108);

   if(lexer_.NextCharIs('='))
   {
      auto end = lexer_.FindFirstOf(",}");
      if(end == string::npos) return Backup(start, 109);
      if(!GetCxxExpr(init, end)) return Backup(start, 110);
   }

   return Success(Parser_GetEnumerator, start);
}

//------------------------------------------------------------------------------

void Parser::GetFileDecls(Namespace* space)
{
   Debug::ft("Parser.GetFileDecls");

   string str;

   //  Keep fetching the next token, which should be a keyword or identifier.
   //  If there is one, step over it (if allowed) and try its possible parses.
   //  If there isn't one, we've reached something that we can't get beyond.
   //
   while(true)
   {
      auto kwd = NextKeyword(str);
      if(str.empty()) return;
      if(CxxWord::Attrs[kwd].advance) lexer_.Advance(str.size());
      if(!ParseInFile(kwd, space)) return;
   }
}

//------------------------------------------------------------------------------

fn_name Parser_GetFor = "Parser.GetFor";

bool Parser::GetFor(TokenPtr& statement)
{
   Debug::ft(Parser_GetFor);

   auto begin = kwdBegin_;
   auto start = CurrPos();

   //  The "for" keyword has already been parsed.
   //
   TokenPtr initial;
   DataPtr data;
   if(!lexer_.NextCharIs('(')) return Backup(start, 111);
   if(GetFuncData(data))
   {
      initial.reset(data.release());
   }
   else
   {
      ExprPtr expr;
      auto end = lexer_.FindFirstOf(";");
      if(end == string::npos) return Backup(start, 112);
      GetCxxExpr(expr, end);
      if(!lexer_.NextCharIs(';')) return Backup(start, 113);
      initial.reset(expr.release());
   }

   ExprPtr condition;
   ExprPtr subsequent;
   BlockPtr loop;
   auto end = lexer_.FindFirstOf(";");
   if(end == string::npos) return Backup(start, 114);
   GetCxxExpr(condition, end);
   if(!lexer_.NextCharIs(';')) return Backup(start, 115);
   if(!GetParExpr(subsequent, true, true)) return Backup(start, 116);
   if(!GetBlock(loop)) return Backup(start, 117);

   statement.reset(new For(begin));
   auto f = static_cast< For* >(statement.get());
   f->AddInitial(initial);
   f->AddCondition(condition);
   f->AddSubsequent(subsequent);
   f->AddLoop(loop);
   return Success(Parser_GetFor, begin);
}

//------------------------------------------------------------------------------

fn_name Parser_GetFriend = "Parser.GetFriend";

bool Parser::GetFriend(FriendPtr& decl)
{
   Debug::ft(Parser_GetFriend);

   //  <Friend> = [<TemplateParms>] "friend"
   //             (<FuncDecl> | [<ClassTag>] <QualName> ";")
   //  The "friend" keyword has already been parsed unless "template"
   //  precedes it.
   //
   auto begin = kwdBegin_;
   auto start = CurrPos();

   TemplateParmsPtr parms;
   if(GetTemplateParms(parms))
   {
      begin = CurrPos();
      if(!NextKeywordIs(FRIEND_STR)) return Backup(start, 118);
   }

   decl.reset(new Friend);
   decl->SetContext(begin);

   string str;
   FunctionPtr func;
   auto kwd = NextKeyword(str);

   if(GetFuncDecl(kwd, func))
   {
      decl->SetFunc(func);
   }
   else
   {
      Cxx::ClassTag tag;
      QualNamePtr friendName;
      if(lexer_.GetClassTag(tag)) decl->SetTag(tag);
      if(!GetQualName(friendName)) return Backup(start, 119);
      if(!lexer_.NextCharIs(';')) return Backup(start, 120);
      decl->SetName(friendName);
   }

   decl->SetTemplateParms(parms);
   return Success(Parser_GetFriend, begin);
}

//------------------------------------------------------------------------------

fn_name Parser_GetFuncData = "Parser.GetFuncData";

bool Parser::GetFuncData(DataPtr& data)
{
   Debug::ft(Parser_GetFuncData);

   //  <FuncData> = [<AlignAs>] ["static"] ["thread_local"] ["constexpr"]
   //               <TypeSpec> (<FuncData1> | <FuncData2>)
   //  <FuncData1> = <Name> "(" [<Expr>] ")" ";"
   //  <FuncData2> =        <Name> [<ArraySpec>] ["=" <Expr>]
   //    ["," ["*"]* ["&"]* <Name> [<ArraySpec>] ["=" <Expr>]]* ";"
   //  FuncData1 initializes the data with a parenthesized expression that
   //  directly follows the name.  It is sometimes a constructor call:
   //    i.e. Class name(args); instead of auto name = Class(args);
   //  FuncData2 allows multiple declarations, based on the same root type,
   //  in a list that separates each declaration with a comma:
   //    e.g. int i = 0, *j = nullptr, k[10] = { };
   //
   auto start = CurrPos();

   KeywordSet attrs;
   AlignAsPtr align;
   TypeSpecPtr typeSpec;
   string dataName;

   if(!GetAlignAs(align)) return Backup(start, 254);
   lexer_.GetDataTags(attrs);
   auto stat = (attrs.find(Cxx::STATIC) != attrs.cend());
   auto tloc = (attrs.find(Cxx::THREAD_LOCAL) != attrs.cend());
   auto cexp = (attrs.find(Cxx::CONSTEXPR) != attrs.cend());
   if(!GetTypeSpec(typeSpec, &attrs)) return Backup(start, 121);
   auto pos = CurrPos();
   if(!lexer_.GetName(dataName)) return Backup(start, 122);
   if(lexer_.NextCharIs('('))
   {
      //  A parenthesized expression is initializing the data.  Parse it as
      //  an argument list in case it is a constructor call.
      //
      TokenPtr expr;
      auto end = lexer_.FindClosing('(', ')');
      if(end == string::npos) return Backup(start, 123);
      if(!GetArgList(expr)) return Backup(start, 124);
      if(!lexer_.NextCharIs(';')) return Backup(start, 125);

      data.reset(new FuncData(dataName, typeSpec));
      data->SetContext(pos);
      data->SetAlignment(align);
      data->SetStatic(stat);
      data->SetThreadLocal(tloc);
      data->SetConstexpr(cexp);
      static_cast< FuncData* >(data.get())->SetExpression(expr);
      return Success(Parser_GetFuncData, start);
   }

   FuncData* prev = nullptr;
   FuncData* curr = nullptr;

   do
   {
      ArraySpecPtr arraySpec;
      ExprPtr init;

      if((dataName.empty()) && (typeSpec == nullptr))
      {
         //  This is a subsequent declaration of data with the same type as
         //  the first declaration.  The pointer and reference tags attached
         //  to this item's name override those of the original type, which
         //  is cloned and modified to create the subsequent declaration.
         //
         typeSpec.reset(prev->GetTypeSpec()->Clone());
         typeSpec->CopyContext(prev);
         *typeSpec->Tags() = TypeTags();
         GetTypeTags(typeSpec.get());
         pos = CurrPos();
         if(!lexer_.GetName(dataName)) return Backup(start, 126);
      }

      while(GetArraySpec(arraySpec)) typeSpec->AddArray(arraySpec);

      auto eqpos = CurrPos();

      if(lexer_.NextStringIs("="))
      {
         if(lexer_.NextCharIs('{'))
         {
            if(!GetBraceInit(init)) return Backup(start, 127);
         }
         else
         {
            auto end = lexer_.FindFirstOf(",;");
            if(end == string::npos) return Backup(start, 128);
            if(!GetCxxExpr(init, end)) return Backup(start, 129);
         }
      }

      //  The DATA argument returns the first declaration in any series.
      //  Subsequent declarations are placed in a queue that follows the
      //  first declaration.
      //
      if(data == nullptr)
      {
         data.reset(new FuncData(dataName, typeSpec));
         curr = static_cast< FuncData* >(data.get());
         curr->SetFirst(curr);
      }
      else
      {
         DataPtr subseq(new FuncData(dataName, typeSpec));
         curr = static_cast< FuncData* >(subseq.get());
         prev->SetNext(subseq);
      }

      curr->SetContext(pos);
      curr->SetStatic(stat);
      curr->SetConstexpr(cexp);
      curr->SetAssignment(init, eqpos);
      prev = curr;
   }
   while(lexer_.NextCharIs(','));

   if(!lexer_.NextCharIs(';')) return Backup(start, 130);
   return Success(Parser_GetFuncData, start);
}

//------------------------------------------------------------------------------

fn_name Parser_GetFuncDecl = "Parser.GetFuncDecl";

bool Parser::GetFuncDecl(Cxx::Keyword kwd, FunctionPtr& func)
{
   Debug::ft(Parser_GetFuncDecl);

   //  <FuncDecl> = ["extern"] [<TemplateParms>]
   //               (<CtorDecl> | <DtorDecl> | <ProcDecl>) (<FuncImpl> | ";")
   //
   auto start = CurrPos();
   auto found = false;
   auto extn = false;

   TemplateParmsPtr parms;

   switch(kwd)
   {
   case Cxx::EXTERN:
      extn = true;
      lexer_.Advance(strlen(EXTERN_STR));
      GetTemplateParms(parms);
      break;
   case Cxx::TEMPLATE:
      if(!GetTemplateParms(parms)) return Backup(start, 131);
      break;
   }

   //  At this point, "extern" and template parameters have been parsed.
   //  Now parse the function signature itself.
   //
   switch(kwd)
   {
   case Cxx::CONST:
   case Cxx::STATIC:
   case Cxx::EXTERN:
   case Cxx::OPERATOR:
   case Cxx::VOLATILE:
      found = GetProcDecl(func);
      break;
   case Cxx::NIL_KEYWORD:
   case Cxx::TEMPLATE:
   case Cxx::EXPLICIT:
   case Cxx::CONSTEXPR:
      found = (GetCtorDecl(func) || GetProcDecl(func));
      break;
   case Cxx::VIRTUAL:
      found = (GetDtorDecl(func) || GetProcDecl(func));
      break;
   case Cxx::NVDTOR:
      found = GetDtorDecl(func);
      break;
   case Cxx::INLINE:
      found = (GetCtorDecl(func) || GetDtorDecl(func) || GetProcDecl(func));
      break;
   }

   if(!found) return Backup(start, func, 218);
   func->SetTemplateParms(parms);
   if(extn) func->SetExtern(true);

   //  The next character should be a semicolon, equal sign, or left brace,
   //  depending on whether the function is only declared here, is deleted
   //  or defaulted, or is actually defined.
   //
   if(lexer_.NextCharIs(';')) return Success(Parser_GetFuncDecl, start);
   if(GetFuncSpecial(func)) return Success(Parser_GetFuncDecl, start);

   auto pos = CurrPos();
   if(!lexer_.NextCharIs('{')) return Backup(start, func, 219);

   auto end = lexer_.FindClosing('{', '}');
   if(end == string::npos) return Backup(start, func, 220);

   //  Wait to parse a class's inlines until the rest of the class has
   //  been parsed.
   //
   if(func->AtFileScope() || (source_ == IsFuncInst))
   {
      lexer_.Reposition(pos);
      if(!GetFuncImpl(func.get())) return Backup(start, func, 221);
   }
   else
   {
      func->SetBracePos(pos);
      lexer_.Reposition(end + 1);
      if(lexer_.NextCharIs(';')) Log(RedundantSemicolon);
   }

   return Success(Parser_GetFuncDecl, start);
}

//------------------------------------------------------------------------------

fn_name Parser_GetFuncDefn = "Parser.GetFuncDefn";

bool Parser::GetFuncDefn(Cxx::Keyword kwd, FunctionPtr& func)
{
   Debug::ft(Parser_GetFuncDefn);

   //  <FuncDefn> = [<TemplateParms>]
   //               (<CtorDefn> | <DtorDefn> | <ProcDefn>) <FuncImpl>
   //
   auto start = CurrPos();
   auto found = false;

   TemplateParmsPtr parms;

   if(kwd == Cxx::TEMPLATE)
   {
      if(!GetTemplateParms(parms)) return Backup(start, 132);
   }

   switch(kwd)
   {
   case Cxx::NIL_KEYWORD:
   case Cxx::INLINE:
      found = (GetCtorDefn(func) || GetDtorDefn(func) || GetProcDefn(func));
      break;
   default:
      found = GetProcDefn(func);
   }

   if(!found) return Backup(start, func, 222);
   func->SetTemplateParms(parms);
   if(GetFuncSpecial(func)) return Success(Parser_GetFuncDecl, start);

   auto curr = CurrPos();
   if(!lexer_.NextCharIs('{')) return Backup(start, func, 223);
   lexer_.Reposition(curr);
   if(!GetFuncImpl(func.get())) return Backup(start, func, 224);
   return Success(Parser_GetFuncDefn, start);
}

//------------------------------------------------------------------------------

fn_name Parser_GetFuncImpl = "Parser.GetFuncImpl";

bool Parser::GetFuncImpl(Function* func)
{
   Debug::ft(Parser_GetFuncImpl);

   auto start = CurrPos();

   Context::PushScope(func, true);

   BlockPtr block;
   if(!GetBlock(block))
   {
      //  The function implementation was not parsed successfully.
      //  Skip it and continue with the next item.
      //
      Failure(venue_ + ": " + func->Name());
      lexer_.Reposition(start);
      if(!lexer_.NextCharIs('{')) return false;
      auto end = lexer_.FindClosing('{', '}');
      lexer_.Reposition(end + 1);
      Context::PopScope();
      return true;
   }

   Context::PopScope();
   func->SetImpl(block);
   if(lexer_.NextCharIs(';')) Log(RedundantSemicolon);
   return Success(Parser_GetFuncImpl, start);
}

//------------------------------------------------------------------------------

fn_name Parser_GetFuncSpec = "Parser.GetFuncSpec";

bool Parser::GetFuncSpec(TypeSpecPtr& spec, FunctionPtr& func)
{
   Debug::ft(Parser_GetFuncSpec);

   //  <FuncSpec> = "(" "*" <Name> ")" <Arguments>
   //  GetTypeSpec has already parsed the function's return type.
   //
   auto start = CurrPos();
   if(!lexer_.NextCharIs('(')) return Backup(133);
   if(!lexer_.NextCharIs('*')) return Backup(start, 134);

   string name;
   auto pos = CurrPos();
   if(!lexer_.GetName(name)) return Backup(start, 135);
   if(!lexer_.NextCharIs(')')) return Backup(start, 136);
   if(!lexer_.NextCharIs('(')) return Backup(start, 137);

   name.insert(0, "(*");
   name += ')';
   QualNamePtr funcName(new QualName(name));
   funcName->SetContext(pos);
   func.reset(new Function(funcName, spec, true));
   func->SetContext(pos);
   if(!GetArguments(func)) return Backup(start, func, 225);
   return Success(Parser_GetFuncSpec, start);
}

//------------------------------------------------------------------------------

bool Parser::GetFuncSpecial(FunctionPtr& func)
{
   Debug::ft("Parser.GetFuncSpecial");

   //  Look for "= delete;" or "= default; ".
   //
   if(!lexer_.NextCharIs('=')) return false;

   string str;
   lexer_.NextKeyword(str);
   if(str == DEFAULT_STR)
      func->SetDefaulted();
   else if(str == DELETE_STR)
      func->SetDeleted();
   else return false;
   lexer_.Advance(str.size());
   if(!lexer_.NextCharIs(';')) return false;
   return true;
}

//------------------------------------------------------------------------------

fn_name Parser_GetGoto = "Parser.GetGoto";

bool Parser::GetGoto(TokenPtr& statement)
{
   Debug::ft(Parser_GetGoto);

   auto begin = kwdBegin_;
   auto start = CurrPos();

   //  The "goto" keyword has already been parsed.  Get the label.
   //
   string name;
   if(!lexer_.GetName(name)) return Backup(start, 249);
   if(!lexer_.NextCharIs(';')) return Backup(start, 250);
   statement.reset(new Goto(name, begin));
   return Success(Parser_GetGoto, begin);
}

//------------------------------------------------------------------------------

fn_name Parser_GetIf = "Parser.GetIf";

bool Parser::GetIf(TokenPtr& statement)
{
   Debug::ft(Parser_GetIf);

   auto begin = kwdBegin_;
   auto start = CurrPos();

   //  The "if" keyword has already been parsed.
   //
   ExprPtr condition;
   BlockPtr thenBlock;
   BlockPtr elseBlock;
   if(!GetParExpr(condition, false)) return Backup(start, 138);
   if(!GetBlock(thenBlock)) return Backup(start, 139);
   if(NextKeywordIs(ELSE_STR))
   {
      if(!GetBlock(elseBlock)) return Backup(start, 140);
      if(!elseBlock->IsBraced())
      {
         auto first = elseBlock->FirstStatement();
         if(first->Type() == Cxx::If)
         {
            static_cast< If* >(first)->SetElseIf();
         }
      }
   }

   statement.reset(new If(begin));
   auto i = static_cast< If* >(statement.get());
   i->AddCondition(condition);
   i->AddThen(thenBlock);
   i->AddElse(elseBlock);
   return Success(Parser_GetIf, begin);
}

//------------------------------------------------------------------------------

void Parser::GetInline(Function* func)
{
   Debug::ft("Parser.GetInline");

   auto pos = func->GetBracePos();

   if(pos != string::npos)
   {
      farthest_ = pos;
      lexer_.Reposition(pos);
      GetFuncImpl(func);
   }
}

//------------------------------------------------------------------------------

bool Parser::GetInlines(Class* cls)
{
   Debug::ft("Parser.GetInlines");

   //  This jumps around to parse functions, so adjust farthest_ accordingly.
   //
   Context::PushScope(cls, false);

   auto end = CurrPos();
   auto funcs = cls->Funcs();

   for(size_t i = 0; i < funcs->size(); ++i)
   {
      GetInline(funcs->at(i).get());
   }

   auto opers = cls->Opers();

   for(size_t i = 0; i < opers->size(); ++i)
   {
      GetInline(opers->at(i).get());
   }

   auto friends = cls->Friends();

   for(size_t i = 0; i < friends->size(); ++i)
   {
      auto func = friends->at(i)->Inline();
      if(func != nullptr) GetInline(func);
   }

   farthest_ = end;
   lexer_.Reposition(end);
   Context::PopScope();
   return true;
}

//------------------------------------------------------------------------------

string Parser::GetLINE() const
{
   Debug::ft("Parser.GetLINE");

   std::ostringstream stream;

   if(!ParsingSourceCode()) stream << venue_ << SPACE;
   stream << lexer_.GetLineNum(CurrPos()) + 1;
   return stream.str();
}

//------------------------------------------------------------------------------

size_t Parser::GetLineNum(size_t pos) const
{
   Debug::ft("Parser.GetLineNum");

   if(pos == string::npos) pos = CurrPos();
   return lexer_.GetLineNum(pos);
}

//------------------------------------------------------------------------------

void Parser::GetMemberDecls(Class* cls)
{
   Debug::ft("Parser.GetMemberDecls");

   string str;

   //  Keep fetching the next token, which should be a keyword or identifier.
   //  If there is one, step over it (if allowed) and try its possible parses.
   //  If there isn't one, we've reached something that we can't get beyond.
   //  That should be the end of the class.
   //
   while(true)
   {
      auto kwd = NextKeyword(str);
      if(str.empty()) return;
      if(CxxWord::Attrs[kwd].advance) lexer_.Advance(str.size());
      if(!ParseInClass(kwd, cls)) return;
   }
}

//------------------------------------------------------------------------------

bool Parser::GetName(string& name)
{
   Debug::ft("Parser.GetName");

   if(!lexer_.GetName(name)) return false;

   if(source_ == IsClassInst)
   {
      string spec;
      if(lexer_.GetTemplateSpec(spec)) name += spec;
   }

   return true;
}

//------------------------------------------------------------------------------

fn_name Parser_GetNamespace = "Parser.GetNamespace";

bool Parser::GetNamespace()
{
   Debug::ft(Parser_GetNamespace);

   //  <Namespace> = "namespace" <Name> "{" [<FileDecl>]* "}"
   //  The "namespace" keyword has already been parsed.
   //
   auto begin = kwdBegin_;
   auto start = CurrPos();

   string name;
   if(!lexer_.GetName(name)) return Backup(start, 141);
   if(!lexer_.NextCharIs('{')) return Backup(start, 142);

   auto outer = Context::Scope();
   auto inner = static_cast< Namespace* >(outer)->EnsureNamespace(name);
   inner->SetLoc(Context::File(), begin);
   Context::PushScope(inner, false);
   GetFileDecls(inner);
   Context::PopScope();

   if(!lexer_.NextCharIs('}')) return Backup(start, 143);
   if(lexer_.NextCharIs(';')) Log(RedundantSemicolon);
   return Success(Parser_GetNamespace, begin);
}

//------------------------------------------------------------------------------

fn_name Parser_GetNew = "Parser.GetNew";

bool Parser::GetNew(ExprPtr& expr, Cxx::Operator op, size_t pos)
{
   Debug::ft(Parser_GetNew);

   //    new = "new" ["(" <ArgList> ")"] <TypeSpec> ["(" <ArgList> ")"]
   //  new[] = "new" ["(" <ArgList> ")"] <TypeSpec> (<ArraySpec>)+
   //
   //  The operator has already been parsed.  It is always OBJECT_CREATE
   //  because, unlike delete[], the brackets for new[] are some distance
   //  away and aren't recognized when the new operator is extracted.  We
   //  must therefore determine whether this is new or new[].
   //
   //  The operator itself usually has no arguments, but they *are* possible.
   //  The TypeSpec is mandatory.  For new[], one or more ArraySpecs follow.
   //  For new, there could be arguments for a constructor call.  EXPR will
   //  therefore contain only a single Operation (either OBJECT_CREATE or
   //  OBJECT_CREATE_ARRAY), and all else will be added as arguments to
   //  that Operation.
   //
   //  The first argument contains the arguments for new or new[]; if there
   //  are none, an empty function call is put there as a placeholder.  The
   //  second argument is therefore always the TypeSpec.  For scalar new, the
   //  third argument (optional) is a function call containing the constructor
   //  arguments.  For new[], the third argument is an ArraySpec (mandatory),
   //  and any arguments after that are additional ArraySpecs.
   //
   TokenPtr token(new Operation(op));
   token->SetContext(pos);
   auto newOp = static_cast< Operation* >(token.get());
   expr->AddItem(token);

   auto start = CurrPos();

   //  See if there are arguments for operator new itself.  If not, add an
   //  empty function call so that the TypeSpec will always be the second
   //  argument.
   //
   if(lexer_.NextCharIs('('))
   {
      if(!GetArgList(token)) return Backup(start, 144);
   }
   else
   {
      token.reset(new Operation(Cxx::FUNCTION_CALL));
      token->SetContext(pos);
   }

   static_cast< Operation* >(token.get())->SetNew();
   newOp->AddArg(token, false);
   start = CurrPos();

   //  Add the type that is being created.
   //
   TypeSpecPtr typeSpec;
   if(!GetTypeSpec(typeSpec)) return Backup(start, 145);
   token.reset(typeSpec.release());
   newOp->AddArg(token, false);

   start = CurrPos();

   //  Look for an array spec.  If we find one, this is actually new[].
   //  It can have more array specs, but not constructor arguments.  If
   //  there is no array spec, this is scalar new, so see if there are
   //  constructor arguments.
   //
   ArraySpecPtr arraySpec;
   if(GetArraySpec(arraySpec))
   {
      newOp->SetOp(Cxx::OBJECT_CREATE_ARRAY);

      do
      {
         token.reset(arraySpec.release());
         newOp->AddArg(token, false);
      }
      while(GetArraySpec(arraySpec));
   }
   else
   {
      if(lexer_.NextCharIs('('))
      {
         if(!GetArgList(token)) return Backup(start, 146);
         newOp->AddArg(token, false);
      }
   }

   return Success(Parser_GetNew, start);
}

//------------------------------------------------------------------------------

fn_name Parser_GetNoExcept = "Parser.GetNoExcept";

bool Parser::GetNoExcept(ExprPtr& expr, size_t pos)
{
   Debug::ft(Parser_GetNoExcept);

   auto start = CurrPos();

   //  The noexcept operator has already been parsed.
   //
   ExprPtr item;
   GetCxxExpr(item, expr->EndPos(), false);

   TokenPtr token(new Operation(Cxx::NOEXCEPT));
   token->SetContext(pos);
   auto op = static_cast< Operation* >(token.get());
   TokenPtr arg(item.release());
   if(arg != nullptr) op->AddArg(arg, false);
   expr->AddItem(token);
   return Success(Parser_GetNoExcept, start);
}

//------------------------------------------------------------------------------

bool Parser::GetNum(ExprPtr& expr)
{
   Debug::ft("Parser.GetNum");

   TokenPtr item;
   if(!lexer_.GetNum(item)) return false;
   expr->AddItem(item);
   return true;
}

//------------------------------------------------------------------------------

bool Parser::GetOp(ExprPtr& expr, bool cxx)
{
   Debug::ft("Parser.GetOp");

   auto start = CurrPos();

   auto op = cxx ? lexer_.GetCxxOp() : lexer_.GetPreOp();
   if(op == Cxx::NIL_OPERATOR) return false;

   TokenPtr item;
   QualNamePtr qualName;

   //  Operators require special handling when a simple left
   //  to right parse is problematic:
   //  o "("   parse everything to the closing ")"
   //  o "["   parse everything to the closing "]"
   //  o "?"   parse the expressions separated by the ":"
   //  o "~"   ones complement operator or direct destructor invocation
   //  o "::"  back up and parse the entire qualified name
   //
   switch(op)
   {
   case Cxx::FUNCTION_CALL:
      return HandleParentheses(expr);
   case Cxx::ARRAY_SUBSCRIPT:
      return GetSubscript(expr, start);
   case Cxx::CONDITIONAL:
      return GetConditional(expr, start);
   case Cxx::ONES_COMPLEMENT:
      return HandleTilde(expr, start);
   case Cxx::SCOPE_RESOLUTION:
      lexer_.Reposition(start);
      if(!GetQualName(qualName)) return false;
      item.reset(qualName.release());
      break;
   default:
      item.reset(new Operation(op));
      item->SetContext(start);
   }

   if(expr->AddItem(item)) return true;
   return Backup(start, 147);
}

//------------------------------------------------------------------------------

bool Parser::GetParExpr(ExprPtr& expr, bool omit, bool opt)
{
   Debug::ft("Parser.GetParExpr");

   auto start = CurrPos();

   //  Parse the expression inside the parentheses.
   //
   if(!omit && !lexer_.NextCharIs('(')) return Backup(start, 148);
   auto end = lexer_.FindClosing('(', ')');
   if(end == string::npos) return Backup(start, 149);
   if(!GetCxxExpr(expr, end) && !opt) return Backup(start, 150);
   if(!lexer_.NextCharIs(')')) return Backup(start, 151);
   return true;
}

//------------------------------------------------------------------------------

size_t Parser::GetPointers()
{
   Debug::ft("Parser.GetPointers");

   bool space;
   return lexer_.GetIndirectionLevel('*', space);
}

//------------------------------------------------------------------------------

bool Parser::GetPreAlpha(ExprPtr& expr)
{
   Debug::ft("Parser.GetPreAlpha");

   auto start = CurrPos();

   //  Look for "defined", which is actually an operator.
   //
   string name;
   if(!lexer_.GetName(name)) return Backup(start, 152);

   if(name == DEFINED_STR)
   {
      if(GetDefined(expr, start)) return true;
      return Backup(start, 153);
   }

   MacroNamePtr macro(new MacroName(name));
   macro->SetContext(start);
   TokenPtr item = std::move(macro);
   if(expr->AddItem(item)) return true;
   return Backup(start, 154);
}

//------------------------------------------------------------------------------

fn_name Parser_GetPrecedence = "Parser.GetPrecedence";

bool Parser::GetPrecedence(ExprPtr& expr)
{
   Debug::ft(Parser_GetPrecedence);

   auto prev = lexer_.Prev();
   auto start = CurrPos();

   //  The left parenthesis has already been parsed.
   //
   ExprPtr item;
   if(!GetParExpr(item, true)) return Backup(start, 155);

   TokenPtr token(new Precedence(item));
   token->SetContext(prev);
   expr->AddItem(token);
   return Success(Parser_GetPrecedence, start);
}

//------------------------------------------------------------------------------

fn_name Parser_GetPreExpr = "Parser.GetPreExpr";

bool Parser::GetPreExpr(ExprPtr& expr, size_t end)
{
   Debug::ft(Parser_GetPreExpr);

   auto start = CurrPos();

   char c;
   expr.reset(new Expression(end, true));

   while(lexer_.CurrChar(c) < end)
   {
      switch(c)
      {
      case QUOTE:
         if(GetStr(expr, Cxx::ASCII, start)) break;
         return Skip(end, expr);

      case APOSTROPHE:
         if(GetChar(expr, Cxx::ASCII, start)) break;
         return Skip(end, expr);

      case '{':
         return false;

      case '_':
         if(GetPreAlpha(expr)) break;
         return Skip(end, expr);

      default:
         if(ispunct(c))
         {
            if(GetOp(expr, false)) break;
            return Backup(start, 156);
         }
         if(isdigit(c))
         {
            if(GetNum(expr)) break;
            return Skip(end, expr);
         }
         if(GetPreAlpha(expr)) break;
         if(GetOp(expr, false)) break;
         return Skip(end, expr);
      }
   }

   if(expr->Empty())
   {
      expr.reset();
      return Backup(start, 157);
   }

   return Success(Parser_GetPreExpr, start);
}

//------------------------------------------------------------------------------

fn_name Parser_GetProcDecl = "Parser.GetProcDecl";

bool Parser::GetProcDecl(FunctionPtr& func)
{
   Debug::ft(Parser_GetProcDecl);

   //  <ProcDecl> = ["inline"] ["static"] ["virtual"] ["explicit"] ["constexpr"]
   //               (<StdProc> | <ConvOper>) <Arguments> ["const"] {"volatile"]
   //               ["noexcept"] ["override"] ["final"] ["= 0"]
   //  <StdProc> = <TypeSpec> (<Name> | "operator" <Operator>)
   //  <ConvOper> = "operator" <TypeSpec>
   //
   auto start = CurrPos();

   KeywordSet attrs;
   lexer_.GetFuncFrontTags(attrs);
   auto oper = Cxx::NIL_OPERATOR;

   auto pos = start;
   TypeSpecPtr typeSpec;
   string name;
   QualNamePtr funcName;

   if(NextKeywordIs(OPERATOR_STR))
   {
      if(!GetTypeSpec(typeSpec, &attrs)) return Backup(start, 158);
      pos = CurrPos();
      name = OPERATOR_STR;
      oper = Cxx::CAST;
   }
   else
   {
      if(!GetTypeSpec(typeSpec, &attrs)) return Backup(start, 159);
      lexer_.GetFuncFrontTags(attrs);
      pos = CurrPos();

      if(source_ == IsFuncInst)
      {
         if(!lexer_.GetName(name, oper)) return Backup(start, 258);
         funcName.reset(new QualName(name));
         funcName->SetContext(pos);
         funcName->SetOperator(oper);
         string spec;
         if(!lexer_.GetTemplateSpec(spec)) return Backup(start, 259);
         funcName->Append(spec, false);
      }
      else
      {
         if(!lexer_.GetName(name, oper)) return Backup(start, 160);
      }
   }

   auto extn = (attrs.find(Cxx::EXTERN) != attrs.cend());
   auto inln = (attrs.find(Cxx::INLINE) != attrs.cend());
   auto stat = (attrs.find(Cxx::STATIC) != attrs.cend());
   auto virt = (attrs.find(Cxx::VIRTUAL) != attrs.cend());
   auto expl = (attrs.find(Cxx::EXPLICIT) != attrs.cend());
   auto cexp = (attrs.find(Cxx::CONSTEXPR) != attrs.cend());
   if(!lexer_.NextCharIs('(')) return Backup(start, 161);
   if(funcName == nullptr) funcName.reset(new QualName(name));
   funcName->SetContext(pos);
   func.reset(new Function(funcName, typeSpec));
   func->SetContext(pos);
   if(!GetArguments(func)) return Backup(start, func, 226);
   func->SetOperator(oper);

   lexer_.GetCVTags(attrs);
   auto readonly = (attrs.find(Cxx::CONST) != attrs.cend());
   auto unstable = (attrs.find(Cxx::VOLATILE) != attrs.cend());
   auto noex = NextKeywordIs(NOEXCEPT_STR);
   lexer_.GetFuncBackTags(attrs);
   auto over = (attrs.find(Cxx::OVERRIDE) != attrs.cend());
   auto final = (attrs.find(Cxx::FINAL) != attrs.cend());
   pos = CurrPos();
   auto pure = (lexer_.NextStringIs("=") && lexer_.NextCharIs('0'));
   if(!pure) lexer_.Reposition(pos);

   func->SetStatic(stat, oper);
   func->SetExtern(extn);
   func->SetInline(inln);
   func->SetVirtual(virt);
   func->SetExplicit(expl);
   func->SetConstexpr(cexp);
   if(cexp) func->SetInline(true);
   func->SetConst(readonly);
   func->SetVolatile(unstable);
   func->SetNoexcept(noex);
   func->SetOverride(over);
   func->SetFinal(final);
   func->SetPure(pure);
   return Success(Parser_GetProcDecl, start);
}

//------------------------------------------------------------------------------

fn_name Parser_GetProcDefn = "Parser.GetProcDefn";

bool Parser::GetProcDefn(FunctionPtr& func)
{
   Debug::ft(Parser_GetProcDefn);

   //  <ProcDefn> = <TypeSpec> <QualName> <Arguments>
   //               ["const"] {"volatile"] ["noexcept"]
   //
   auto start = CurrPos();

   TypeSpecPtr typeSpec;
   if(!GetTypeSpec(typeSpec)) return Backup(start, 162);
   auto pos = CurrPos();

   //  If this is a function template instance, append the template
   //  arguments to the name.  GetQualName cannot be used because it
   //  will also parse the template arguments.
   //
   QualNamePtr funcName;
   if(source_ == IsFuncInst)
   {
      string name;
      Cxx::Operator oper;
      if(!lexer_.GetName(name, oper)) return Backup(start, 163);
      funcName.reset(new QualName(name));
      funcName->SetContext(pos);
      funcName->SetOperator(oper);
      string spec;
      if(!lexer_.GetTemplateSpec(spec)) return Backup(start, 164);
      funcName->Append(spec, false);
   }
   else
   {
      if(!GetQualName(funcName)) return Backup(start, 165);
   }
   if(!lexer_.NextCharIs('(')) return Backup(start, 166);

   auto oper = funcName->Operator();
   func.reset(new Function(funcName, typeSpec));
   func->SetContext(pos);
   if(!GetArguments(func)) return Backup(start, func, 227);
   func->SetOperator(oper);

   KeywordSet attrs;
   lexer_.GetCVTags(attrs);
   auto readonly = (attrs.find(Cxx::CONST) != attrs.cend());
   auto unstable = (attrs.find(Cxx::VOLATILE) != attrs.cend());
   auto noex = NextKeywordIs(NOEXCEPT_STR);
   func->SetConst(readonly);
   func->SetVolatile(unstable);
   func->SetNoexcept(noex);
   return Success(Parser_GetProcDefn, start);
}

//------------------------------------------------------------------------------

fn_name Parser_GetQualName = "Parser.GetQualName";

bool Parser::GetQualName(QualNamePtr& name, Constraint constraint)
{
   Debug::ft(Parser_GetQualName);

   //  <QualName> = ["::"] [<TypeName> "::"]*
   //               (<TypeName> | "operator" <Operator>)
   //
   auto start = CurrPos();

   TypeNamePtr type;
   auto global = lexer_.NextStringIs(SCOPE_STR);
   if(!GetTypeName(type, constraint)) return Backup(start, 167);
   if(global) type->SetScoped();
   name.reset(new QualName(type));
   name->SetContext(start);

   while(lexer_.NextStringIs(SCOPE_STR))
   {
      if(!GetTypeName(type, constraint)) return Backup(start, 168);
      type->SetScoped();
      name->PushBack(type);
   }

   if(name->Name() == OPERATOR_STR)
   {
      Cxx::Operator oper;

      if(!lexer_.GetOpOverride(oper))
      {
         Debug::SwLog(Parser_GetQualName, "operator override?", 0, false);
         return Backup(start, 169);
      }

      name->SetOperator(oper);
   }

   return true;
}

//------------------------------------------------------------------------------

fn_name Parser_GetReturn = "Parser.GetReturn";

bool Parser::GetReturn(TokenPtr& statement)
{
   Debug::ft(Parser_GetReturn);

   auto begin = kwdBegin_;
   auto start = CurrPos();

   //  The "return" keyword has already been parsed.
   //
   ExprPtr expr;
   auto end = lexer_.FindFirstOf(";");
   if(end == string::npos) return Backup(start, 170);
   GetCxxExpr(expr, end);
   if(!lexer_.NextCharIs(';')) return Backup(start, 171);

   statement.reset(new Return(begin));
   static_cast< Return* >(statement.get())->AddExpr(expr);
   return Success(Parser_GetReturn, begin);
}

//------------------------------------------------------------------------------

fn_name Parser_GetSizeOf = "Parser.GetSizeOf";

bool Parser::GetSizeOf(ExprPtr& expr, size_t pos)
{
   Debug::ft(Parser_GetSizeOf);

   auto start = CurrPos();

   //  The sizeof operator has already been parsed.  Its argument can be a
   //  name (e.g. a local or argument), a type, or an expression.
   //
   if(!lexer_.NextCharIs('(')) return Backup(start, 172);
   auto mark = CurrPos();
   TokenPtr arg;

   do
   {
      QualNamePtr name;
      if(GetQualName(name, TypeKeyword))
      {
         arg.reset(name.release());
         if(lexer_.NextCharIs(')')) break;
         lexer_.Reposition(mark);
      }

      TypeSpecPtr spec;
      if(GetTypeSpec(spec))
      {
         arg.reset(spec.release());
         if(lexer_.NextCharIs(')')) break;
         lexer_.Reposition(mark);
      }

      ExprPtr size;
      if(!GetParExpr(size, true)) return Backup(start, 173);
      arg.reset(size.release());
   }
   while(false);

   TokenPtr token(new Operation(Cxx::SIZEOF_TYPE));
   token->SetContext(pos);
   auto op = static_cast< Operation* >(token.get());
   op->AddArg(arg, false);
   expr->AddItem(token);
   return Success(Parser_GetSizeOf, start);
}

//------------------------------------------------------------------------------

fn_name Parser_GetSpaceData = "Parser.GetSpaceData";

bool Parser::GetSpaceData(Cxx::Keyword kwd, DataPtr& data)
{
   Debug::ft(Parser_GetSpaceData);

   //  <SpaceData> = [<AlignAs>] [["extern"] | [<TemplateParms>]]
   //                ["static"] ["thread_local"] ["constexpr"]
   //                <TypeSpec> <QualName> (<SpaceData1> | <SpaceData2>)
   //  <SpaceData1> = "(" [<Expr>] ")" ";"
   //  <SpaceData2> =  [<ArraySpec>] ["=" <Expr>] ";"
   //  SpaceData1 initializes the data parenthesized expression that
   //  directly follows the name.
   //
   auto start = CurrPos();

   KeywordSet attrs;
   TemplateParmsPtr parms;
   AlignAsPtr align;
   TypeSpecPtr typeSpec;
   QualNamePtr dataName;
   ArraySpecPtr arraySpec;
   TokenPtr expr;
   ExprPtr init;

   if(kwd == Cxx::TEMPLATE)
   {
      if(!GetTemplateParms(parms)) return Backup(start, 174);
   }

   if(!GetAlignAs(align)) return Backup(start, 255);
   lexer_.GetDataTags(attrs);
   auto extn = (attrs.find(Cxx::EXTERN) != attrs.cend());
   auto stat = (attrs.find(Cxx::STATIC) != attrs.cend());
   auto tloc = (attrs.find(Cxx::THREAD_LOCAL) != attrs.cend());
   auto cexp = (attrs.find(Cxx::CONSTEXPR) != attrs.cend());
   if(!GetTypeSpec(typeSpec, &attrs)) return Backup(start, 175);
   auto pos = CurrPos();
   if(!GetQualName(dataName)) return Backup(start, 176);
   if(dataName->Operator() != Cxx::NIL_OPERATOR) return Backup(start, 177);

   auto eqpos = string::npos;

   if(lexer_.NextCharIs('('))
   {
      auto end = lexer_.FindClosing('(', ')');
      if(end == string::npos) return Backup(start, 178);
      if(!GetArgList(expr)) return Backup(start, 179);
   }
   else
   {
      while(GetArraySpec(arraySpec)) typeSpec->AddArray(arraySpec);

      eqpos = CurrPos();

      if(lexer_.NextStringIs("="))
      {
         if(lexer_.NextCharIs('{'))
         {
            if(!GetBraceInit(init)) return Backup(start, 180);
         }
         else
         {
            auto end = lexer_.FindFirstOf(";");
            if(end == string::npos) return Backup(start, 181);
            if(!GetCxxExpr(init, end)) return Backup(start, 182);
         }
      }
   }

   if(!lexer_.NextCharIs(';')) return Backup(start, 183);
   data.reset(new SpaceData(dataName, typeSpec));
   data->SetContext(pos);
   data->SetTemplateParms(parms);
   data->SetAlignment(align);
   data->SetExtern(extn);
   data->SetStatic(stat);
   data->SetThreadLocal(tloc);
   data->SetConstexpr(cexp);
   data->SetExpression(expr);
   data->SetAssignment(init, eqpos);
   return Success(Parser_GetSpaceData, start);
}

//------------------------------------------------------------------------------

bool Parser::GetStatements(BlockPtr& block, bool braced)
{
   Debug::ft("Parser.GetStatements");

   string str;

   //  Keep fetching the next item, which could be a keyword, operator, or
   //  identifier.  Step over a keyword (if allowed) and try the possible
   //  parses.
   //
   while(true)
   {
      auto kwd = NextKeyword(str);
      if(CxxWord::Attrs[kwd].advance) lexer_.Advance(str.size());
      if(!ParseInBlock(kwd, block.get())) return true;
      if(!braced) return true;
   }

   return true;
}

//------------------------------------------------------------------------------

fn_name Parser_GetStaticAssert = "Parser.GetStaticAssert";

bool Parser::GetStaticAssert(StaticAssertPtr& statement)
{
   Debug::ft(Parser_GetStaticAssert);

   auto begin = kwdBegin_;
   auto start = CurrPos();

   //  The "static_assert" keyword has already been parsed.  A boolean
   //  expression and string literal follow.
   //
   ExprPtr expr;
   if(!lexer_.NextCharIs('(')) return Backup(start, 241);
   auto rpar = lexer_.FindClosing('(', ')');
   if(rpar == string::npos) return Backup(start, 242);
   auto comma = lexer_.FindFirstOf(",");
   if(comma == string::npos) return Backup(start, 243);
   if(!GetCxxExpr(expr, comma)) return Backup(start, 244);
   if(!lexer_.NextCharIs(',')) return Backup(start, 245);

   ExprPtr message;
   if(!GetCxxExpr(message, rpar)) return Backup(start, 246);
   if(!lexer_.NextCharIs(')')) return Backup(start, 247);
   if(!lexer_.NextCharIs(';')) return Backup(start, 248);
   statement.reset(new StaticAssert(expr, message));
   statement->SetContext(begin);
   return Success(Parser_GetStaticAssert, begin);
}

//------------------------------------------------------------------------------

bool Parser::GetStr(ExprPtr& expr, Cxx::Encoding code, size_t pos)
{
   Debug::ft("Parser.GetStr");

   //  Extract the string that appears between two quotation marks
   //  and wrap it in the appropriate type of string literal.
   //
   if(!lexer_.ThisCharIs(QUOTE)) return false;

   StringLiteralPtr str;

   switch(code)
   {
   case Cxx::ASCII:
   case Cxx::U8:
      str = StringLiteralPtr(new StrLiteral);
      break;
   case Cxx::U16:
      str = StringLiteralPtr(new u16StrLiteral);
      break;
   case Cxx::U32:
      str = StringLiteralPtr(new u32StrLiteral);
      break;
   case Cxx::WIDE:
      str = StringLiteralPtr(new wStrLiteral);
      break;
   default:
      return false;
   }

   str->SetContext(pos);

   uint32_t c;

   while(true)
   {
      auto curr = lexer_.Curr();
      if(!lexer_.GetChar(c)) return false;

      if(c == QUOTE)
      {
         //  There are three cases:
         //  o If a backslash preceded the quote, add the quote to the literal.
         //  o If another quote follows the quote, continue the literal.
         //  o If neither of the above applies, the quote ended the literal.
         //
         if(lexer_.At(curr) == BACKSLASH)
         {
            str->PushBack(c);
         }
         else
         {
            //  Get the lexer's current position, which is the character
            //  directly after the quote.  It could be whitespace, so use
            //  Reposition to find the next character that will be parsed,
            //  which continues the string literal if it is also a quote.
            //
            curr = lexer_.Curr();
            lexer_.Reposition(curr);
            if(!lexer_.GetChar(c)) return false;
            if(c == QUOTE) continue;
            lexer_.Reposition(curr);
            break;
         }
      }
      else
      {
         str->PushBack(c);
      }
   }

   TokenPtr item(std::move(str));
   expr->AddItem(item);
   return true;
}

//------------------------------------------------------------------------------

fn_name Parser_GetSubscript = "Parser.GetSubscript";

bool Parser::GetSubscript(ExprPtr& expr, size_t pos)
{
   Debug::ft(Parser_GetSubscript);

   auto start = CurrPos();

   //  The left bracket has already been parsed.
   //
   ExprPtr item;
   auto end = lexer_.FindClosing('[', ']');
   if(end == string::npos) return Backup(start, 184);
   if(!GetCxxExpr(item, end)) return Backup(start, 185);
   if(!lexer_.NextCharIs(']')) return Backup(start, 186);

   //  The array subscript operator is binary, so adding it to the expression
   //  causes it to take what preceded it (the array) as its first argument.
   //  Once that is finished, the expression for the array index can be added.
   //
   TokenPtr token(new Operation(Cxx::ARRAY_SUBSCRIPT));
   token->SetContext(pos);
   auto op = static_cast< Operation* >(token.get());
   expr->AddItem(token);
   TokenPtr index(item.release());
   op->AddArg(index, false);
   return Success(Parser_GetSubscript, start);
}

//------------------------------------------------------------------------------

fn_name Parser_GetSwitch = "Parser.GetSwitch";

bool Parser::GetSwitch(TokenPtr& statement)
{
   Debug::ft(Parser_GetSwitch);

   auto begin = kwdBegin_;
   auto start = CurrPos();

   //  The "switch" keyword has already been parsed.
   //
   ExprPtr value;
   BlockPtr cases;
   if(!GetParExpr(value, false)) return Backup(start, 187);
   if(!GetBlock(cases)) return Backup(start, 188);

   statement.reset(new Switch(begin));
   auto s = static_cast< Switch* >(statement.get());
   s->AddExpr(value);
   s->AddCases(cases);
   return Success(Parser_GetSwitch, begin);
}

//------------------------------------------------------------------------------

fn_name Parser_GetTemplateParm = "Parser.GetTemplateParm";

bool Parser::GetTemplateParm(TemplateParmPtr& parm)
{
   Debug::ft(Parser_GetTemplateParm);

   //  <TemplateParm> = (<ClassTag> | <QualName>) <Name> ["*"]* ["=" <TypeName>]
   //
   auto start = CurrPos();

   Cxx::ClassTag tag;
   QualNamePtr type;

   if(!lexer_.GetClassTag(tag, true))
   {
      tag = Cxx::ClassTag_N;
      if(!GetQualName(type)) return Backup(start, 189);
   }

   string argName;
   if(!lexer_.GetName(argName)) return Backup(start, 190);

   auto ptrs = GetPointers();

   TypeSpecPtr preset;

   if(lexer_.NextCharIs('='))
   {
      if(!GetTypeSpec(preset)) return Backup(start, 191);
   }

   parm.reset(new TemplateParm(argName, tag, type, ptrs, preset));
   parm->SetContext(start);
   return Success(Parser_GetTemplateParm, start);
}

//------------------------------------------------------------------------------

fn_name Parser_GetTemplateParms = "Parser.GetTemplateParms";

bool Parser::GetTemplateParms(TemplateParmsPtr& parms)
{
   Debug::ft(Parser_GetTemplateParms);

   //  <TemplateParms> = "template" "<" <TemplateParm> ["," <TemplateParm>]* ">"
   //
   auto start = CurrPos();
   if(!NextKeywordIs(TEMPLATE_STR)) return Backup(192);
   if(!lexer_.NextCharIs('<')) return Backup(start, 193);

   TemplateParmPtr parm;
   if(!GetTemplateParm(parm)) return Backup(start, 194);

   parms.reset(new TemplateParms(parm));

   while(lexer_.NextCharIs(','))
   {
      if(!GetTemplateParm(parm)) return Backup(start, 195);
      parms->AddParm(parm);
   }

   if(!lexer_.NextCharIs('>')) return Backup(start, 196);
   return Success(Parser_GetTemplateParms, start);
}

//------------------------------------------------------------------------------

fn_name Parser_GetThrow = "Parser.GetThrow";

bool Parser::GetThrow(ExprPtr& expr, size_t pos)
{
   Debug::ft(Parser_GetThrow);

   auto start = CurrPos();

   //  The throw operator has already been parsed.
   //
   ExprPtr item;
   GetCxxExpr(item, expr->EndPos(), false);

   TokenPtr token(new Operation(Cxx::THROW));
   token->SetContext(pos);
   auto op = static_cast< Operation* >(token.get());
   TokenPtr arg(item.release());
   if(arg != nullptr) op->AddArg(arg, false);
   expr->AddItem(token);
   return Success(Parser_GetThrow, start);
}

//------------------------------------------------------------------------------

const SysTime* Parser::GetTime()
{
   Debug::ft("Parser.GetTime");

   return &Context::RootParser()->time_;
}

//------------------------------------------------------------------------------

fn_name Parser_GetTry = "Parser.GetTry";

bool Parser::GetTry(TokenPtr& statement)
{
   Debug::ft(Parser_GetTry);

   auto begin = kwdBegin_;
   auto start = CurrPos();

   //  The "try" keyword has already been parsed.
   //
   BlockPtr work;
   TokenPtr trap;
   if(!GetBlock(work)) return Backup(start, 197);

   statement.reset(new Try(begin));
   auto t = static_cast< Try* >(statement.get());
   t->AddTry(work);
   while(GetCatch(trap)) t->AddCatch(trap);
   return Success(Parser_GetTry, begin);
}

//------------------------------------------------------------------------------

fn_name Parser_GetTypedef = "Parser.GetTypedef";

bool Parser::GetTypedef(TypedefPtr& type)
{
   Debug::ft(Parser_GetTypedef);

   //  <Typedef> = "typedef" <TypeSpec> [<Name>] [<ArraySpec>]* [<AlignAs>] ";"
   //  The "typedef" keyword has already been parsed.  <Name> is mandatory if
   //  (and only if) <TypeSpec> does not include a <FuncSpec> (function type).
   //
   auto begin = kwdBegin_;
   auto start = CurrPos();

   TypeSpecPtr typeSpec;
   string typeName;
   if(!GetTypeSpec(typeSpec, typeName)) return Backup(start, 198);
   auto pos = CurrPos();

   //  If typeSpec was a function type, typeName was set to its name,
   //  if any.  For other typedefs, the name follows typeSpec.
   //
   if(typeSpec->GetFuncSpec() == nullptr)
   {
      if(!lexer_.GetName(typeName)) return Backup(start, 199);
   }

   ArraySpecPtr arraySpec;
   AlignAsPtr align;
   while(GetArraySpec(arraySpec)) typeSpec->AddArray(arraySpec);
   if(!GetAlignAs(align)) return Backup(start, 256);
   if(!lexer_.NextCharIs(';')) return Backup(start, 200);

   type.reset(new Typedef(typeName, typeSpec));
   type->SetContext(pos);
   type->SetAlignment(align);
   return Success(Parser_GetTypedef, begin);
}

//------------------------------------------------------------------------------

fn_name Parser_GetTypeId = "Parser.GetTypeId";

bool Parser::GetTypeId(ExprPtr& expr, size_t pos)
{
   Debug::ft(Parser_GetTypeId);

   auto start = CurrPos();

   //  The typeid operator has already been parsed.
   //
   ExprPtr type;
   if(!GetParExpr(type, false)) return Backup(start, 201);

   TokenPtr token(new Operation(Cxx::TYPE_NAME));
   token->SetContext(pos);
   auto op = static_cast< Operation* >(token.get());
   TokenPtr arg(type.release());
   op->AddArg(arg, false);
   expr->AddItem(token);
   return Success(Parser_GetTypeId, start);
}

//------------------------------------------------------------------------------

bool Parser::GetTypeName(TypeNamePtr& type, Constraint constraint)
{
   Debug::ft("Parser.GetTypeName");

   //  <TypeName> = <Name> ["<" <TypeSpec> ["," <TypeSpec>]* ">"]
   //
   auto start = CurrPos();

   string name;
   if(!lexer_.GetName(name, constraint)) return Backup(202);
   type.reset(new TypeName(name));
   type->SetContext(start);

   //  Before looking for a template argument after a '<', see if the '<' is
   //  actually part of an operator.
   //
   auto mark = CurrPos();

   if(lexer_.NextCharIs('<'))
   {
      //  Back up if this is actually an operator (LEFT_SHIFT, LESS_OR_EQUAL,
      //  or LESS).  The latter is nasty and its disambiguation may be wrong.
      //  It adds ";{" to ValidOpChars and removes ":,<*[]", assuming that the
      //  former cannot appear within template arguments, whereas the latter
      //  can (for scope resolution, argument separation, nested templates,
      //  pointer arguments, and array arguments, respectively).
      //
      if(lexer_.NextCharIs('<')) return lexer_.Reposition(mark);
      if(lexer_.NextCharIs('=')) return lexer_.Reposition(mark);
      auto next = lexer_.FindFirstOf(";{.=()!>&|+-~/%^?");
      if(next == string::npos) return Backup(start, 203);
      if(lexer_.At(next) != '>') return lexer_.Reposition(mark);

      do
      {
         TypeSpecPtr arg;
         if(!GetTypeSpec(arg)) return Backup(start, 204);
         type->AddTemplateArg(arg);
      }
      while(lexer_.NextCharIs(','));

      if(!lexer_.NextCharIs('>')) return Backup(start, 205);
   }

   return true;
}

//------------------------------------------------------------------------------

fn_name Parser_GetTypeSpec1 = "Parser.GetTypeSpec";

bool Parser::GetTypeSpec(TypeSpecPtr& spec, KeywordSet* attrs)
{
   Debug::ft(Parser_GetTypeSpec1);

   //  <TypeSpec> = ["const"] ["volatile"] <QualName> ["const"] ["volatile"]
   //               [<TypeTags>] [<FuncSpec>]
   //
   //  Regular types can use pointer ("*") and reference ("&") tags.
   //  Template arguments can use pointer and array ("[]") tags.
   //  This is not enforced because the code is known to parse.
   //
   auto start = CurrPos();

   KeywordSet tags;
   bool readonly;
   bool unstable;

   //  ATTRS is provided with data types and function return types,
   //  where "const" and "volatile" have already been parsed if they
   //  precede the type's name.
   //
   if(attrs != nullptr)
   {
      readonly = (attrs->find(Cxx::CONST) != attrs->cend());
      unstable = (attrs->find(Cxx::VOLATILE) != attrs->cend());
      attrs->erase(Cxx::CONST);
      attrs->erase(Cxx::VOLATILE);
   }
   else
   {
      lexer_.GetCVTags(tags);
      readonly = (tags.find(Cxx::CONST) != tags.cend());
      unstable = (tags.find(Cxx::VOLATILE) != tags.cend());
      tags.clear();
   }

   QualNamePtr typeName;
   if(!GetQualName(typeName, TypeKeyword)) return Backup(start, 206);
   if(!CheckType(typeName)) return Backup(start, 207);
   spec.reset(new DataSpec(typeName));
   spec->SetContext(start);

   auto pos = CurrPos();
   lexer_.GetCVTags(tags);
   if(tags.find(Cxx::CONST) != tags.cend())
   {
      if(readonly)
         Log(RedundantConst, pos);
      else
         readonly = true;
   }
   if(tags.find(Cxx::VOLATILE) != tags.cend()) unstable = true;

   spec->Tags()->SetConst(readonly);
   spec->Tags()->SetVolatile(unstable);
   GetTypeTags(spec.get());

   //  Check if this is a function type.  If it is, it assumes ownership
   //  of SPEC as its return type.  Create a FuncSpec to wrap the entire
   //  function signature.
   //
   FunctionPtr func;
   pos = CurrPos();
   if(GetFuncSpec(spec, func))
   {
      spec.reset(new FuncSpec(func));
      spec->SetContext(pos);
   }

   return Success(Parser_GetTypeSpec1, start);
}

//------------------------------------------------------------------------------

bool Parser::GetTypeSpec(TypeSpecPtr& spec, string& name)
{
   Debug::ft("Parser.GetTypeSpec(name)");

   if(!GetTypeSpec(spec)) return false;

   auto func = spec->GetFuncSpec();

   if(func != nullptr)
   {
      //  This is a function type.  Set NAME to the function type's
      //  name, if any, stripping the "(*" prefix and ")" suffix.
      //
      auto funcName = func->Name();
      if(funcName.empty()) return true;
      name = funcName;
      name.erase(0, 2);
      name.pop_back();
   }

   return true;
}

//------------------------------------------------------------------------------

bool Parser::GetTypeTags(TypeSpec* spec)
{
   Debug::ft("Parser.GetTypeTags");

   //  <TypeTags> = [["*"] ["const"] ["volatile"]]* ["[]"]
   //               ["&" | "&&"] ["const"] ["volatile"]
   //
   auto tags = spec->Tags();

   bool space;
   TagCount ptrs = 0;

   while(true)
   {
      //  Keep looking for a series of one or more pointer tags.
      //
      auto n = lexer_.GetIndirectionLevel('*', space);
      if(n == 0) break;
      if(space) tags->ptrDet_ = true;
      ptrs += n;

      //  If the next keywords are "const" and/or "volatile", apply
      //  them to the last pointer in the current series of pointers.
      //
      KeywordSet attrs;
      lexer_.GetCVTags(attrs);
      auto readonly = (attrs.find(Cxx::CONST) != attrs.cend());
      auto unstable = (attrs.find(Cxx::VOLATILE) != attrs.cend());
      tags->SetPointer(ptrs - 1, readonly, unstable);
   }

   //  Now look for an unbounded array tag.
   //
   if(lexer_.NextStringIs(ARRAY_STR, false))
   {
      tags->SetUnboundedArray();
   }

   //  Now look for references.
   //
   auto refs = lexer_.GetIndirectionLevel('&', space);
   if(space) tags->refDet_ = true;
   tags->SetRefs(refs);

   //  Now look for a trailing "const" and/or "volatile" that apply to
   //  the underlying type.
   //
   auto pos = CurrPos();
   KeywordSet attrs;
   auto readonly = (attrs.find(Cxx::CONST) != attrs.cend());
   auto unstable = (attrs.find(Cxx::VOLATILE) != attrs.cend());

   if(readonly)
   {
      if(tags->IsConst())
         Log(RedundantConst, pos);
      else
         tags->SetConst(true);
   }

   if(unstable) tags->SetVolatile(true);
   return true;
}

//------------------------------------------------------------------------------

fn_name Parser_GetUsing = "Parser.GetUsing";

bool Parser::GetUsing(UsingPtr& use, TypedefPtr& type)
{
   Debug::ft(Parser_GetUsing);

   //  <Using> = "using" (<UsingDecl> | <TypeAlias>) ";"
   //  <UsingDecl> = ["namespace"] <QualName>
   //  <TypeAlias> = <name> "=" <TypeSpec> [<ArraySpec>]* [<AlignAs>]
   //
   //  The "using" keyword has already been parsed.
   //
   auto begin = kwdBegin_;
   auto start = CurrPos();

   //  Start by looking for a using directive or declaration.
   //
   QualNamePtr usingName;
   auto space = NextKeywordIs(NAMESPACE_STR);

   if((GetQualName(usingName)) && lexer_.NextCharIs(';'))
   {
      use.reset(new Using(usingName, space));
      use->SetContext(begin);
      return Success(Parser_GetUsing, begin);
   }

   if(space) return Backup(start, 208);

   //  Look for a type alias.  If found, it is captured as a typedef.
   //
   lexer_.Reposition(start);
   string typeName;
   if(!lexer_.GetName(typeName)) return Backup(start, 209);
   if(!lexer_.NextCharIs('=')) return Backup(start, 230);

   TypeSpecPtr typeSpec;
   if(!GetTypeSpec(typeSpec, typeName)) return Backup(start, 231);

   ArraySpecPtr arraySpec;
   AlignAsPtr align;
   while(GetArraySpec(arraySpec)) typeSpec->AddArray(arraySpec);
   if(!GetAlignAs(align)) return Backup(start, 257);
   if(!lexer_.NextCharIs(';')) return Backup(start, 232);

   type.reset(new Typedef(typeName, typeSpec));
   type->SetUsing();
   type->SetContext(begin);
   type->SetAlignment(align);
   return Success(Parser_GetUsing, begin);
}

//------------------------------------------------------------------------------

fn_name Parser_GetWhile = "Parser.GetWhile";

bool Parser::GetWhile(TokenPtr& statement)
{
   Debug::ft(Parser_GetWhile);

   auto begin = kwdBegin_;
   auto start = CurrPos();

   //  The "while" keyword has already been parsed.
   //
   ExprPtr condition;
   BlockPtr loop;
   if(!GetParExpr(condition, false)) return Backup(start, 210);
   if(!GetBlock(loop)) return Backup(start, 211);

   statement.reset(new While(begin));
   auto w = static_cast< While* >(statement.get());
   w->AddCondition(condition);
   w->AddLoop(loop);
   return Success(Parser_GetWhile, begin);
}

//------------------------------------------------------------------------------

bool Parser::HandleDefine()
{
   Debug::ft("Parser.HandleDefine");

   //  <Define> = "#define" <Name> [<Expr>]
   //
   auto start = CurrPos();
   auto end = lexer_.FindLineEnd(start);
   string name;

   if(!lexer_.NextStringIs(HASH_DEFINE_STR)) return Fault(DirectiveMismatch);
   if(!lexer_.GetName(name)) return Fault(SymbolExpected);
   ExprPtr expr;
   GetPreExpr(expr, end);

   //  See if NAME has already appeared as a macro name before creating it.
   //
   auto macro = Singleton< CxxSymbols >::Instance()->FindMacro(name);

   if(macro == nullptr)
   {
      MacroPtr def(new Define(name, expr));
      def->SetContext(start);
      Singleton< CxxRoot >::Instance()->AddMacro(def);
   }
   else
   {
      macro->SetContext(start);
      macro->SetExpr(expr);
   }

   lexer_.PreprocessSource();
   return true;
}

//------------------------------------------------------------------------------

bool Parser::HandleDirective(DirectivePtr& dir)
{
   Debug::ft("Parser.HandleDirective");

   string str;
   auto kwd = lexer_.NextDirective(str);

   switch(kwd)
   {
   case Cxx::_DEFINE:
      return HandleDefine();
   case Cxx::_ELIF:
      return HandleElif(dir);
   case Cxx::_ELSE:
      return HandleElse(dir);
   case Cxx::_ERROR:
      return HandleError(dir);
   case Cxx::_ENDIF:
      return HandleEndif(dir);
   case Cxx::_IF:
      return HandleIf(dir);
   case Cxx::_IFDEF:
      return HandleIfdef(dir);
   case Cxx::_IFNDEF:
      return HandleIfndef(dir);
   case Cxx::_INCLUDE:
      return HandleInclude();
   case Cxx::_LINE:
      return HandleLine(dir);
   case Cxx::_PRAGMA:
      return HandlePragma(dir);
   case Cxx::_UNDEF:
      return HandleUndef(dir);
      break;
   }

   return lexer_.Skip();
}

//------------------------------------------------------------------------------

bool Parser::HandleElif(DirectivePtr& dir)
{
   Debug::ft("Parser.HandleElif");

   //  <Elif> = "#elif" <Expr>
   //
   auto start = CurrPos();
   auto end = lexer_.FindLineEnd(start);

   if(!lexer_.NextStringIs(HASH_ELIF_STR)) return Fault(DirectiveMismatch);
   auto iff = Context::Optional();
   if(iff == nullptr) return Fault(ElifUnexpected);
   ExprPtr expr;
   if(!GetPreExpr(expr, end)) return Fault(ConditionExpected);

   ElifPtr elif(new Elif);
   elif->SetContext(start);
   elif->AddCondition(expr);
   if(!iff->AddElif(elif.get())) return Fault(ElifUnexpected);
   lexer_.FindCode(elif.get(), elif->EnterScope());
   dir = std::move(elif);
   return true;
}

//------------------------------------------------------------------------------

bool Parser::HandleElse(DirectivePtr& dir)
{
   Debug::ft("Parser.HandleElse");

   //  <Else> = "#else"
   //
   auto start = CurrPos();

   if(!lexer_.NextStringIs(HASH_ELSE_STR)) return Fault(DirectiveMismatch);
   auto ifx = Context::Optional();
   if(ifx == nullptr) return Fault(ElseUnexpected);

   ElsePtr els(new Else);
   els->SetContext(start);
   if(!ifx->AddElse(els.get())) return Fault(ElseUnexpected);
   lexer_.FindCode(els.get(), els->EnterScope());
   dir = std::move(els);
   return true;
}

//------------------------------------------------------------------------------

bool Parser::HandleEndif(DirectivePtr& dir)
{
   Debug::ft("Parser.HandleEndif");

   //  <Endif> = "#endif"
   //
   auto start = CurrPos();

   if(!lexer_.NextStringIs(HASH_ENDIF_STR)) return Fault(DirectiveMismatch);
   auto ifx = Context::Optional();
   if(ifx == nullptr) return Fault(EndifUnexpected);
   Context::PopOptional();

   EndifPtr endif(new Endif);
   endif->SetContext(start);
   if(!ifx->AddEndif(endif.get())) return Fault(EndifUnexpected);
   dir = std::move(endif);
   return true;
}

//------------------------------------------------------------------------------

bool Parser::HandleError(DirectivePtr& dir)
{
   Debug::ft("Parser.HandleError");

   //  <Error> = "#error" <Text>
   //
   auto start = CurrPos();

   if(!lexer_.NextStringIs(HASH_ERROR_STR)) return Fault(DirectiveMismatch);
   auto begin = CurrPos();
   auto end = lexer_.FindLineEnd(begin);
   auto text = lexer_.Substr(begin, end - begin);

   dir = ErrorPtr(new Error(text));
   dir->SetContext(start);
   dir->EnterScope();
   return lexer_.Reposition(end);
}

//------------------------------------------------------------------------------

bool Parser::HandleIf(DirectivePtr& dir)
{
   Debug::ft("Parser.HandleIf");

   //  <If> = "#if" <Expr>
   //
   auto start = CurrPos();
   auto end = lexer_.FindLineEnd(start);

   if(!lexer_.NextStringIs(HASH_IF_STR)) return Fault(DirectiveMismatch);
   ExprPtr expr;
   if(!GetPreExpr(expr, end)) return Fault(ConditionExpected);

   IffPtr iff(new Iff);
   iff->SetContext(start);
   iff->AddCondition(expr);
   lexer_.FindCode(iff.get(), iff->EnterScope());
   dir = std::move(iff);
   return true;
}

//------------------------------------------------------------------------------

bool Parser::HandleIfdef(DirectivePtr& dir)
{
   Debug::ft("Parser.HandleIfdef");

   //  <Ifdef> = "#ifdef" <Name>
   //
   auto start = CurrPos();
   string symbol;

   if(!lexer_.NextStringIs(HASH_IFDEF_STR)) return Fault(DirectiveMismatch);
   auto pos = CurrPos();
   if(!lexer_.GetName(symbol)) return Fault(SymbolExpected);
   MacroNamePtr macro(new MacroName(symbol));
   macro->SetContext(pos);

   IfdefPtr ifdef(new Ifdef(macro));
   ifdef->SetContext(start);
   lexer_.FindCode(ifdef.get(), ifdef->EnterScope());
   dir = std::move(ifdef);
   return true;
}

//------------------------------------------------------------------------------

bool Parser::HandleIfndef(DirectivePtr& dir)
{
   Debug::ft("Parser.HandleIfndef");

   //  <Ifndef> = "#ifndef" <Name>
   //
   auto start = CurrPos();
   string symbol;

   if(!lexer_.NextStringIs(HASH_IFNDEF_STR)) return Fault(DirectiveMismatch);
   auto pos = CurrPos();
   if(!lexer_.GetName(symbol)) return Fault(SymbolExpected);
   MacroNamePtr macro(new MacroName(symbol));
   macro->SetContext(pos);

   IfndefPtr ifndef(new Ifndef(macro));
   ifndef->SetContext(start);
   lexer_.FindCode(ifndef.get(), ifndef->EnterScope());
   dir = std::move(ifndef);
   return true;
}

//------------------------------------------------------------------------------

bool Parser::HandleInclude()
{
   Debug::ft("Parser.HandleInclude");

   //  <Include> = "#include" <FileName>
   //
   //  Note that #includes are handled before parsing, by CodeFile.Scan, because
   //  they allow the compile order to be calculated.  Here, we finally insert
   //  the #include as a statement in the code file.
   //
   auto start = CurrPos();
   auto end = lexer_.FindLineEnd(start);
   string name;
   bool angle;

   if(!lexer_.NextStringIs(HASH_INCLUDE_STR)) return Fault(DirectiveMismatch);
   if(!lexer_.GetIncludeFile(start, name, angle)) return Fault(FileExpected);
   auto incl = Context::File()->InsertInclude(name);
   if(incl != nullptr) incl->SetContext(start);
   return lexer_.Reposition(end);
}

//------------------------------------------------------------------------------

bool Parser::HandleLine(DirectivePtr& dir)
{
   Debug::ft("Parser.HandleLine");

   //  <Line> = "#line" <Text>
   //
   auto start = CurrPos();

   if(!lexer_.NextStringIs(HASH_LINE_STR)) return Fault(DirectiveMismatch);
   auto begin = CurrPos();
   auto end = lexer_.FindLineEnd(begin);
   auto text = lexer_.Substr(begin, end - begin);

   dir = LinePtr(new Line(text));
   dir->SetContext(start);
   return lexer_.Reposition(end);
}

//------------------------------------------------------------------------------

bool Parser::HandleParentheses(ExprPtr& expr)
{
   Debug::ft("Parser.HandleParentheses");

   //  The left parenthesis has already been parsed.  It could introduce a
   //  function call, a C-style cast, or simply parentheses for precedence
   //  (evaluation order).
   //
   auto back = expr->Back();

   //  A function name must precede the arguments for a function call.  The
   //  name could follow a selection operator, as in a.f() or a->f(), which
   //  is why Back(), and not just back(), is used.
   //
   if((back != nullptr) && (back->Type() == Cxx::QualName))
   {
      TokenPtr call;

      if(GetArgList(call))
      {
         expr->AddItem(call);
         return true;
      }
   }

   if(GetCast(expr)) return true;
   return GetPrecedence(expr);
}

//------------------------------------------------------------------------------

bool Parser::HandlePragma(DirectivePtr& dir)
{
   Debug::ft("Parser.HandlePragma");

   //  <Pragma> = "#pragma" <Text>
   //
   auto start = CurrPos();

   if(!lexer_.NextStringIs(HASH_PRAGMA_STR)) return Fault(DirectiveMismatch);
   auto begin = CurrPos();
   auto end = lexer_.FindLineEnd(begin);
   auto text = lexer_.Substr(begin, end - begin);

   dir = PragmaPtr(new Pragma(text));
   dir->SetContext(start);
   return lexer_.Reposition(end);
}

//------------------------------------------------------------------------------

bool Parser::HandleTilde(ExprPtr& expr, size_t pos)
{
   Debug::ft("Parser.HandleTilde");

   TokenPtr item;

   //  If the last token in EXPR is a "." or "->" operator, this should
   //  be a direct destructor invocation.  Back up to the '~' and get
   //  the destructor's name.
   //
   auto token = expr->Back();

   if(token != nullptr)
   {
      if(token->Type() == Cxx::Operation)
      {
         auto op = static_cast< Operation* >(token)->Op();

         if((op == Cxx::POINTER_SELECT) || (op == Cxx::REFERENCE_SELECT))
         {
            QualNamePtr name;
            lexer_.Reposition(pos);
            if(!GetQualName(name)) return Backup(pos, 212);
            item.reset(name.release());
         }
      }
   }

   //  If ITEM is still empty, the '~' should be a ones complement operator.
   //
   if(item == nullptr)
   {
      item.reset(new Operation(Cxx::ONES_COMPLEMENT));
      item->SetContext(pos);
   }

   if(expr->AddItem(item)) return true;
   return Backup(pos, 213);
}

//------------------------------------------------------------------------------

bool Parser::HandleUndef(DirectivePtr& dir)
{
   Debug::ft("Parser.HandleUndef");

   //  <Undef> = "#undef" <Name>
   //
   auto start = CurrPos();
   string name;

   if(!lexer_.NextStringIs(HASH_UNDEF_STR)) return Fault(DirectiveMismatch);
   if(!lexer_.GetName(name)) return Fault(SymbolExpected);

   UndefPtr undef = UndefPtr(new Undef(name));
   undef->SetContext(start);
   dir = std::move(undef);
   return true;
}

//------------------------------------------------------------------------------

string Parser::Indent()
{
   return spaces(2 * (Context::ParseDepth() - 1));
}

//------------------------------------------------------------------------------

void Parser::Log(Warning warning, size_t pos) const
{
   Debug::ft("Parser.Log");

   if(pos == string::npos) pos = lexer_.Prev();
   Context::File()->LogPos(pos, warning);
}

//------------------------------------------------------------------------------

Cxx::Keyword Parser::NextKeyword(string& str)
{
   Debug::ft("Parser.NextKeyword");

   auto kwd = lexer_.NextKeyword(str);
   if(kwd != Cxx::NIL_KEYWORD) kwdBegin_ = CurrPos();
   return kwd;
}

//------------------------------------------------------------------------------

bool Parser::NextKeywordIs(fixed_string str)
{
   Debug::ft("Parser.NextKeywordIs");

   if(!lexer_.NextStringIs(str)) return false;
   kwdBegin_ = lexer_.Prev();
   return true;
}

//------------------------------------------------------------------------------

fn_name Parser_Parse = "Parser.Parse";

bool Parser::Parse(CodeFile& file)
{
   Debug::ft(Parser_Parse);

   //  Return if the file has already been parsed, else write its name to
   //  the console.
   //
   if(file.ParseStatus() != CodeFile::Unparsed) return true;
   Debug::Progress(file.Name());

   //  Create a parse trace file if requested.
   //
   auto path =
      Element::OutputPath() + PATH_SEPARATOR + file.Name() + ".parse.txt";
   if(Context::OptionIsOn(TraceParse))
   {
      pTrace_ = SysFile::CreateOstream(path.c_str(), true);
      if(pTrace_ == nullptr) return false;
   }

   //  Set up the trace environment and insert the name of the source code
   //  file into the "object code" file.
   //
   auto traced = Context::StartTracing();
   Context::Trace(CxxTrace::START_FILE, file);

   //  Initialize the parser and note the file being parsed.  Push the global
   //  namespace as the current scope and start parsing at file scope.
   //
   auto gns = Singleton< CxxRoot >::Instance()->GlobalNamespace();
   depth_ = SysThreadStack::FuncDepth();
   Context::SetFile(&file);
   Context::PushScope(gns, false);
   Enter(IsFile, file.Name(), nullptr, file.GetCode(), true, &file);
   GetFileDecls(gns);
   Context::PopScope();
   if(traced) ThisThread::StopTracing();

   //  If the lexer reached the end of the file, the parse succeeded, so mark
   //  the file as parsed.  If the parse failed, indicate this on the console.
   //
   auto parsed = lexer_.Eof();
   Context::SetFile(nullptr);
   file.SetParsed(parsed);
   Debug::Progress((parsed ? CRLF_STR : string(" **FAILED** ") + CRLF));
   if(!parsed) Failure(venue_);

   //  On success, delete the parse file if it is not supposed to be retained.
   //
   if(pTrace_ != nullptr)
   {
      pTrace_.reset();

      if(parsed && !Context::OptionIsOn(SaveParseTrace))
      {
         auto err = remove(path.c_str());

         if(err != 0)
         {
            std::ostringstream stream;
            stream << "failed to remove " << path.c_str();
            Debug::SwLog(Parser_Parse, stream.str(), err, false);
         }
      }
   }

   return parsed;
}

//------------------------------------------------------------------------------

bool Parser::ParseClassInst(ClassInst* inst, size_t pos)
{
   Debug::ft("Parser.ParseClassInst");

   auto name = inst->ScopedName(true);
   Debug::Progress(CRLF + Indent() + name);

   //  Initialize the parser.  If an "object code" file is being produced,
   //  insert the instance name.
   //
   Enter(IsClassInst, name, inst->GetTemplateArgs(), inst->GetCode(), true);
   lexer_.Reposition(pos);
   Context::Trace(CxxTrace::START_TEMPLATE, inst);

   //  Push the template instance as the current scope and start to parse it.
   //  The first thing that could be encountered is a base class declaration.
   do
   {
      BaseDeclPtr base;
      Context::PushScope(inst, false);
      GetBaseDecl(base);
      if(!lexer_.NextCharIs('{')) break;
      inst->AddBase(base);
      GetMemberDecls(inst);
      Context::PopScope();
      if(!lexer_.NextCharIs('}')) break;
      if(!lexer_.NextCharIs(';')) break;
      GetInlines(inst);
   }
   while(false);

   //  The parse succeeded if the lexer reached the end of the code.  If the
   //  parse failed, indicate this on the console.  If an "object code" file
   //  is being produced, indicate that parsing of the template is complete.
   //
   auto parsed = lexer_.Eof();
   Debug::Progress((parsed ? EMPTY_STR : " **FAILED** "));
   if(!parsed) Failure(venue_);
   Context::Trace(CxxTrace::END_TEMPLATE);
   return parsed;
}

//------------------------------------------------------------------------------

bool Parser::ParseFuncInst(const std::string& name, const Function* tmplt,
   CxxArea* area, const TypeName* type, const NodeBase::stringPtr& code)
{
   Debug::ft("Parser.ParseFuncInst");

   Debug::Progress(CRLF + Indent() + name);

   //  Initialize the parser.  If an "object code" file is being produced,
   //  insert the instance name.
   //
   Enter(IsFuncInst, name, type, *code, true);
   Context::Trace(CxxTrace::START_TEMPLATE, 0, name);

   //  Parse the function definition.
   //
   Context::PushScope(area, false);
   string str;
   auto kwd = NextKeyword(str);

   FunctionPtr func;
   auto parsed = GetFuncDefn(kwd, func);
   if(!parsed) parsed = GetFuncDecl(kwd, func);

   if(parsed)
   {
      func->SetAccess(tmplt->GetAccess());
      func->SetTemplateArgs(type);
      func->SetTemplate(const_cast< Function* >(tmplt));
      area->AddFunc(func);
   }

   Context::PopScope();

   //  The parse succeeded if the lexer reached the end of the code.  If the
   //  parse failed, indicate this on the console.  If an "object code" file
   //  is being produced, indicate that parsing of the template is complete.
   //
   parsed = lexer_.Eof();
   Debug::Progress((parsed ? EMPTY_STR : " **FAILED** "));
   if(!parsed) Failure(venue_);
   Context::Trace(CxxTrace::END_TEMPLATE);
   return parsed;
}

//------------------------------------------------------------------------------

fn_name Parser_ParseInBlock = "Parser.ParseInBlock";

bool Parser::ParseInBlock(Cxx::Keyword kwd, Block* block)
{
   Debug::ft(Parser_ParseInBlock);

   if(lexer_.Eof()) return false;

   AsmPtr assembler;
   DataPtr dataItem;
   DirectivePtr dirItem;
   EnumPtr enumItem;
   StaticAssertPtr assert;
   TokenPtr statement;
   TypedefPtr typeItem;
   UsingPtr usingItem;

   //  Get TARGS, in which each character specifies a parse to try for
   //  KWD.  Try them until one succeeds or the list is exhausted.
   //
   auto targs = CxxWord::Attrs[kwd].funcTarget;

   while(!targs.empty())
   {
      auto c = targs.back();

      switch(c)
      {
      case 'x':
         if(GetBasic(statement))
            return block->AddStatement(statement.release());
         break;
      case 'r':
         if(GetReturn(statement))
            return block->AddStatement(statement.release());
         break;
      case 'D':
         if(GetFuncData(dataItem))
            return block->AddStatement(dataItem.release());
         break;
      case 'i':
         if(GetIf(statement))
            return block->AddStatement(statement.release());
         break;
      case 'f':
         if(GetFor(statement))
            return block->AddStatement(statement.release());
         break;
      case 'w':
         if(GetWhile(statement))
            return block->AddStatement(statement.release());
         break;
      case 'b':
         if(GetBreak(statement))
            return block->AddStatement(statement.release());
         break;
      case 'c':
         if(GetCase(statement))
            return block->AddStatement(statement.release());
         break;
      case 's':
         if(GetSwitch(statement))
            return block->AddStatement(statement.release());
         break;
      case 'o':
         if(GetDefault(statement))
            return block->AddStatement(statement.release());
         break;
      case 'n':
         if(GetContinue(statement))
            return block->AddStatement(statement.release());
         break;
      case 'd':
         if(GetDo(statement))
            return block->AddStatement(statement.release());
         break;
      case 't':
         if(GetTry(statement))
            return block->AddStatement(statement.release());
         break;
      case 'g':
         if(GetGoto(statement))
            return block->AddStatement(statement.release());
         break;
      case 'E':
         if(GetEnum(enumItem))
            return block->AddStatement(enumItem.release());
         break;
      case 'H':
         if(HandleDirective(dirItem))
         {
            if(dirItem == nullptr) return true;
            return block->AddStatement(dirItem.release());
         }
         break;
      case 'T':
         if(GetTypedef(typeItem))
            return block->AddStatement(typeItem.release());
         break;
      case 'U':
         if(GetUsing(usingItem, typeItem))
         {
            if(usingItem != nullptr)
               return block->AddStatement(usingItem.release());
            else
               return block->AddStatement(typeItem.release());
         }
         break;
      case '$':
         if(GetStaticAssert(assert))
            return block->AddStatement(assert.release());
         break;
      case '@':
         if(GetAsm(assembler))
            return block->AddStatement(assembler.release());
         break;
      case '-':
         Debug::SwLog(Parser_ParseInBlock, "unexpected keyword", kwd, false);
         return false;
      }

      targs.pop_back();
   }

   return false;
}

//------------------------------------------------------------------------------

fn_name Parser_ParseInClass = "Parser.ParseInClass";

bool Parser::ParseInClass(Cxx::Keyword kwd, Class* cls)
{
   Debug::ft(Parser_ParseInClass);

   if(lexer_.Eof()) return false;

   Cxx::Access access;
   AsmPtr assembler;
   DataPtr dataItem;
   DirectivePtr dirItem;
   EnumPtr enumItem;
   FriendPtr friendItem;
   FunctionPtr funcItem;
   StaticAssertPtr assert;
   TypedefPtr typeItem;
   UsingPtr usingItem;

   //  Get TARGS, in which each character specifies a parse to try for
   //  KWD.  Try them until one succeeds or the list is exhausted.
   //
   auto targs = CxxWord::Attrs[kwd].classTarget;

   while(!targs.empty())
   {
      auto c = targs.back();

      switch(c)
      {
      case 'A':
         if(GetAccess(kwd, access)) return cls->SetCurrAccess(access);
         break;
      case 'C':
         return GetClass(kwd, cls);
      case 'D':
         if(GetClassData(dataItem)) return cls->AddData(dataItem);
         break;
      case 'E':
         if(GetEnum(enumItem)) return cls->AddEnum(enumItem);
         break;
      case 'F':
         if(GetFriend(friendItem)) return cls->AddFriend(friendItem);
         break;
      case 'H':
         if(HandleDirective(dirItem))
         {
            if(dirItem == nullptr) return true;
            return cls->GetFile()->InsertDirective(dirItem);
         }
         break;
      case 'N':
         if(GetNamespace()) return true;
         break;
      case 'P':
         if(GetFuncDecl(kwd, funcItem)) return cls->AddFunc(funcItem);
         break;
      case 'T':
         if(GetTypedef(typeItem)) return cls->AddType(typeItem);
         break;
      case 'U':
         if(GetUsing(usingItem, typeItem))
         {
            if(usingItem != nullptr)
               return cls->AddUsing(usingItem);
            else
               return cls->AddType(typeItem);
         }
         break;
      case '$':
         if(GetStaticAssert(assert))
            return cls->AddStaticAssert(assert);
         break;
      case '@':
         if(GetAsm(assembler))
            return cls->AddAsm(assembler);
         break;
      case '-':
         Debug::SwLog(Parser_ParseInClass, "unexpected keyword", kwd, false);
         return false;
      }

      targs.pop_back();
   }

   return false;
}

//------------------------------------------------------------------------------

fn_name Parser_ParseInFile = "Parser.ParseInFile";

bool Parser::ParseInFile(Cxx::Keyword kwd, Namespace* space)
{
   Debug::ft(Parser_ParseInFile);

   if(lexer_.Eof()) return false;

   AsmPtr assembler;
   DataPtr dataItem;
   DirectivePtr dirItem;
   EnumPtr enumItem;
   FunctionPtr funcItem;
   StaticAssertPtr assert;
   TypedefPtr typeItem;
   UsingPtr usingItem;

   //  Get TARGS, in which each character specifies a parse to try for
   //  KWD.  Try them until one succeeds or the list is exhausted.
   //
   auto targs = CxxWord::Attrs[kwd].fileTarget;

   while(!targs.empty())
   {
      auto c = targs.back();

      switch(c)
      {
      case 'C':
         if(GetClass(kwd, space)) return true;
         break;
      case 'D':
         if(GetSpaceData(kwd, dataItem)) return space->AddData(dataItem);
         break;
      case 'E':
         if(GetEnum(enumItem)) return space->AddEnum(enumItem);
         break;
      case 'H':
         if(HandleDirective(dirItem))
         {
            if(dirItem == nullptr) return true;
            return Context::File()->InsertDirective(dirItem);
         }
         break;
      case 'N':
         if(GetNamespace()) return true;
         break;
      case 'P':
         if(Context::File()->IsCpp())
         {
            if(GetFuncDefn(kwd, funcItem)) return space->AddFunc(funcItem);
            if(GetFuncDecl(kwd, funcItem)) return space->AddFunc(funcItem);
         }
         else
         {
            if(GetFuncDecl(kwd, funcItem)) return space->AddFunc(funcItem);
            if(GetFuncDefn(kwd, funcItem)) return space->AddFunc(funcItem);
         }
         break;
      case 'T':
         if(GetTypedef(typeItem)) return space->AddType(typeItem);
         break;
      case 'U':
         if(GetUsing(usingItem, typeItem))
         {
            if(usingItem != nullptr)
               return space->AddUsing(usingItem);
            else
               return space->AddType(typeItem);
         }
         break;
      case '$':
         if(GetStaticAssert(assert))
            return space->AddStaticAssert(assert);
         break;
      case '@':
         if(GetAsm(assembler))
            return space->AddAsm(assembler);
         break;
      case '-':
         Debug::SwLog(Parser_ParseInFile, "unexpected keyword", kwd, false);
         return false;
      }

      targs.pop_back();
   }

   return false;
}

//------------------------------------------------------------------------------

bool Parser::ParseQualName(const string& code, QualNamePtr& name)
{
   Debug::ft("Parser.ParseQualName");

   Enter(IsQualName, "internal QualName", nullptr, code, false);
   return GetQualName(name);
}

//------------------------------------------------------------------------------

bool Parser::ParseTypeSpec(const string& code, TypeSpecPtr& spec)
{
   Debug::ft("Parser.ParseTypeSpec");

   Enter(IsTypeSpec, "internal TypeSpec", nullptr, code, false);
   auto parsed = GetTypeSpec(spec);
   return parsed;
}

//------------------------------------------------------------------------------

void Parser::ResetStats()
{
   Debug::ft("Parser.ResetStats");

   for(size_t i = 0; i <= MaxCause; ++i) Backups[i] = 0;
}

//------------------------------------------------------------------------------

CxxScoped* Parser::ResolveInstanceArgument(const QualName* name) const
{
   Debug::ft("Parser.ResolveInstanceArgument");

   if(!ParsingTemplateInstance()) return nullptr;

   auto qname = name->QualifiedName(true, true);
   auto args = inst_->Args();

   for(auto a = args->cbegin(); a != args->cend(); ++a)
   {
      auto ref = (*a)->Referent();
      if((ref != nullptr) && (ref->ScopedName(true) == qname)) return ref;
   }

   return nullptr;
}

//------------------------------------------------------------------------------

fn_name Parser_SetCompoundType = "Parser.SetCompoundType";

bool Parser::SetCompoundType
   (QualNamePtr& name, Cxx::Type type, int size, int sign)
{
   Debug::ft(Parser_SetCompoundType);

   auto base = Singleton< CxxRoot >::Instance();

   switch(type)
   {
   case Cxx::NIL_TYPE:
   case Cxx::INT:
      if(sign > 0)
      {
         switch(size)
         {
         case -1:
            name->SetReferent(base->uShortTerm(), nullptr);
            return true;
         case 1:
            name->SetReferent(base->uLongTerm(), nullptr);
            return true;
         case 2:
            name->SetReferent(base->uLongLongTerm(), nullptr);
            return true;
         }

         name->SetReferent(base->uIntTerm(), nullptr);
         return true;
      }

      switch(size)
      {
      case -1:
         name->SetReferent(base->ShortTerm(), nullptr);
         return true;
      case 1:
         name->SetReferent(base->LongTerm(), nullptr);
         return true;
      case 2:
         name->SetReferent(base->LongLongTerm(), nullptr);
         return true;
      }

      name->SetReferent(base->IntTerm(), nullptr);
      return true;

   case Cxx::CHAR:
      if(sign > 0)
         name->SetReferent(base->uCharTerm(), nullptr);
      else
         name->SetReferent(base->CharTerm(), nullptr);
      return true;

   case Cxx::DOUBLE:
      if(size == 0)
         name->SetReferent(base->DoubleTerm(), nullptr);
      else
         name->SetReferent(base->LongDoubleTerm(), nullptr);
      return true;

   default:
      Debug::SwLog(Parser_SetCompoundType, name->Name(), type, false);
   }

   return false;
}

//------------------------------------------------------------------------------

fn_name Parser_Skip = "Parser.Skip";

bool Parser::Skip(size_t end, const ExprPtr& expr, size_t cause)
{
   Debug::ft(Parser_Skip);

   auto start = CurrPos();
   string code = "<@ ";
   code += lexer_.Substr(start, end - start);
   code += " @>";

   auto line = lexer_.GetLineNum(start);
   std::ostringstream text;
   text << venue_ << ", line " << line + 1 << ": " << code;
   Debug::SwLog(Parser_Skip, text.str(), cause, false);

   TokenPtr item(new StrLiteral(code));
   expr->AddItem(item);
   lexer_.Reposition(end);
   return Success(Parser_Skip, start);
}

//------------------------------------------------------------------------------

bool Parser::Success(fn_name_arg func, size_t start) const
{
   Debug::ft("Parser.Success");

   if(!Context::OptionIsOn(TraceParse)) return true;
   if(!ParsingSourceCode()) return true;

   //  Note that when the parse advances over the first keyword expected by a
   //  function before invoking it, that keyword does not appear at the front
   //  of the parse string.
   //
   auto lead = spaces((SysThreadStack::FuncDepth() - depth_) << 1);

   *pTrace_ << lead << func << ": ";

   auto prev = lexer_.Prev();
   auto count = (prev > start ? prev - start : 0);
   auto parsed = lexer_.Substr(start, count);
   auto size = parsed.size();

   if(size <= COUT_LENGTH_MAX)
      *pTrace_ << parsed;
   else
      *pTrace_ << parsed.substr(0, 40) << "..." << parsed.substr(size - 40, 40);

   *pTrace_ << CRLF;
   return true;
}
}
