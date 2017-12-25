//==============================================================================
//
//  Parser.cpp
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
#include "Parser.h"
#include <cctype>
#include <cstdio>
#include <iosfwd>
#include <sstream>
#include <utility>
#include "CodeFile.h"
#include "CoutThread.h"
#include "CxxArea.h"
#include "CxxDirective.h"
#include "CxxExecute.h"
#include "CxxNamed.h"
#include "CxxRoot.h"
#include "CxxScope.h"
#include "CxxScoped.h"
#include "CxxStatement.h"
#include "CxxSymbols.h"
#include "CxxToken.h"
#include "Debug.h"
#include "Element.h"
#include "Formatters.h"
#include "Singleton.h"
#include "SysThreadStack.h"
#include "ThisThread.h"

using std::string;

//------------------------------------------------------------------------------

namespace CodeTools
{
fn_name Parser_ctor1 = "Parser.ctor(opts)";

Parser::Parser(const string& opts) :
   opts_(opts),
   depth_(0),
   kwdBegin_(string::npos),
   tmpltClassInst_(false),
   tmpltFuncInst_(false),
   type_(nullptr),
   pTrace_(nullptr)
{
   Debug::ft(Parser_ctor1);

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

fn_name Parser_ctor2 = "Parser.ctor(scope)";

Parser::Parser(CxxScope* scope) :
   opts_(EMPTY_STR),
   depth_(0),
   kwdBegin_(string::npos),
   tmpltClassInst_(false),
   tmpltFuncInst_(false),
   type_(nullptr),
   pTrace_(nullptr)
{
   Debug::ft(Parser_ctor2);

   //  Make this the active parser and set the scope for parsing.
   //
   Context::PushParser(this);
   Context::PushScope(scope);
}

//------------------------------------------------------------------------------

fn_name Parser_dtor = "Parser.dtor";

Parser::~Parser()
{
   Debug::ft(Parser_dtor);

   //  Remove the parser and close the parse trace file, if any.
   //
   if(Context::Optional() != nullptr) Report(EndifExpected);
   Context::PopParser(this);
   pTrace_.reset();
}

//------------------------------------------------------------------------------

fn_name Parser_CheckType = "Parser.CheckType";

bool Parser::CheckType(QualNamePtr& name)
{
   Debug::ft(Parser_CheckType);

   //  This only applies when TYPE is unqualified.
   //
   if(name->Names_size() != 1) return true;

   auto type = Lexer::GetType(*name->Name());

   switch(type)
   {
   case Cxx::NIL_TYPE:
      //
      //  NAME was not a reserved word, so assume it is a user-defined type.
      //
      return true;
   case Cxx::AUTO_TYPE:
      name->SetReferent(Singleton< CxxRoot >::Instance()->AutoTerm());
      return true;
   case Cxx::BOOL:
      name->SetReferent(Singleton< CxxRoot >::Instance()->BoolTerm());
      return true;
   case Cxx::CHAR:
      name->SetReferent(Singleton< CxxRoot >::Instance()->CharTerm());
      return true;
   case Cxx::DOUBLE:
      name->SetReferent(Singleton< CxxRoot >::Instance()->DoubleTerm());
      return true;
   case Cxx::FLOAT:
      name->SetReferent(Singleton< CxxRoot >::Instance()->FloatTerm());
      return true;
   case Cxx::INT:
      name->SetReferent(Singleton< CxxRoot >::Instance()->IntTerm());
      return true;
   case Cxx::NULLPTR_TYPE:
      name->SetReferent(Singleton< CxxRoot >::Instance()->NullptrtTerm());
      return true;
   case Cxx::VOID:
      name->SetReferent(Singleton< CxxRoot >::Instance()->VoidTerm());
      return true;
   case Cxx::LONG:
   case Cxx::SHORT:
   case Cxx::SIGNED:
   case Cxx::UNSIGNED:
      return GetCompoundType(name, type);
   case Cxx::NON_TYPE:
      //
      //  This screens out reserved words (delete, new, and throw) that can
      //  erroneously be parsed as types.  For example, "delete &x;" can be
      //  parsed as the data declaration "delete& x;".
      //
      return false;

   default:
      Debug::SwErr(Parser_CheckType, *name->Name(), type);
   }

   return false;
}

//------------------------------------------------------------------------------

fn_name Parser_Enter = "Parser.Enter";

void Parser::Enter(const string& code, bool preprocess)
{
   Debug::ft(Parser_Enter);

   lexer_.Initialize(&code);
   if(preprocess) lexer_.PreprocessSource();
}

//------------------------------------------------------------------------------

fn_name Parser_GetAccess = "Parser.GetAccess";

bool Parser::GetAccess(Cxx::Keyword kwd, Cxx::Access& access)
{
   Debug::ft(Parser_GetAccess);

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

fn_name Parser_GetArgList = "Parser.GetArgList";

bool Parser::GetArgList(TokenPtr& call)
{
   Debug::ft(Parser_GetArgList);

   auto start = lexer_.Curr();

   //  The left parenthesis has already been parsed.
   //
   auto temps = TokenPtrVector();
   ExprPtr expr;

   if(!lexer_.NextCharIs(')'))
   {
      while(true)
      {
         auto end = lexer_.FindFirstOf(",)");
         if(end == string::npos) return lexer_.Retreat(start);
         if(!GetCxxExpr(expr, end)) return lexer_.Retreat(start);
         auto arg = TokenPtr(expr.release());
         temps.push_back(std::move(arg));
         if(lexer_.NextCharIs(')')) break;
         if(!lexer_.NextCharIs(',')) return lexer_.Retreat(start);
      }
   }

   call.reset(new Operation(Cxx::FUNCTION_CALL));
   auto op = static_cast< Operation* >(call.get());

   for(size_t i = 0; i < temps.size(); ++i)
   {
      auto arg = TokenPtr(temps.at(i).release());
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
   auto start = lexer_.Curr();

   TypeSpecPtr typeSpec;
   string argName;
   ArraySpecPtr arraySpec;
   ExprPtr default;

   if(!GetTypeSpec(typeSpec, argName)) return lexer_.Retreat(start);

   //  If the argument was a function type, argName was set to its name,
   //  if any.  For other arguments, the name follows the TypeSpec.
   //
   if(typeSpec->GetFuncSpec() == nullptr)
   {
      if(!lexer_.GetName(argName))
      {
         arg.reset(new Argument(argName, typeSpec));
         SetContext(arg.get(), start);
         return Success(Parser_GetArgument, start);
      }
   }

   while(GetArraySpec(arraySpec)) typeSpec->AddArray(arraySpec);

   if(lexer_.NextStringIs("="))
   {
      //  Get the argument's default value.
      //
      auto end = lexer_.FindFirstOf(",)");
      if(end == string::npos) return lexer_.Retreat(start);
      if(!GetCxxExpr(default, end)) return lexer_.Retreat(start);
   }

   arg.reset(new Argument(argName, typeSpec));
   arg->SetDefault(default);
   SetContext(arg.get(), start);
   return Success(Parser_GetArgument, start);
}

//------------------------------------------------------------------------------

fn_name Parser_GetArguments = "Parser.GetArguments";

bool Parser::GetArguments(FunctionPtr& func)
{
   Debug::ft(Parser_GetArguments);

   //  <Arguments> = "(" [<Argument>] ["," <Argument>]* ")"
   //  The left parenthesis has already been parsed.
   //
   auto start = lexer_.Curr();

   ArgumentPtr arg;

   if(GetArgument(arg))
   {
      func->AddArg(arg);

      while(lexer_.NextCharIs(','))
      {
         if(!GetArgument(arg)) return lexer_.Retreat(start);
         func->AddArg(arg);
      }
   }

   if(!lexer_.NextCharIs(')')) return lexer_.Retreat(start);
   return Success(Parser_GetArguments, start);
}

//------------------------------------------------------------------------------

fn_name Parser_GetArraySpec = "Parser.GetArraySpec";

bool Parser::GetArraySpec(ArraySpecPtr& array)
{
   Debug::ft(Parser_GetArraySpec);

   //  <ArraySpec> = "[" [<Expr>] "]"
   //
   auto start = lexer_.Curr();

   //  If a left bracket is found, extract any expression between it
   //  and the right bracket.  Note that the expression can be empty.
   //
   ExprPtr size;
   if(!lexer_.NextCharIs('[')) return lexer_.Retreat(start);
   auto end = lexer_.FindClosing('[', ']');
   if(end == string::npos) return lexer_.Retreat(start);
   GetCxxExpr(size, end);
   if(!lexer_.NextCharIs(']')) return lexer_.Retreat(start);
   array.reset(new ArraySpec(size));
   return Success(Parser_GetArraySpec, start);
}

//------------------------------------------------------------------------------

fn_name Parser_GetBaseDecl = "Parser.GetBaseDecl";

bool Parser::GetBaseDecl(BaseDeclPtr& base)
{
   Debug::ft(Parser_GetBaseDecl);

   //  <BaseDecl> = ":" <Access> <QualName>
   //
   auto start = lexer_.Curr();

   Cxx::Access access;
   QualNamePtr baseName;
   if(!lexer_.NextStringIs(":")) return lexer_.Retreat(start);
   if(!lexer_.GetAccess(access)) return lexer_.Retreat(start);
   if(!GetQualName(baseName)) return lexer_.Retreat(start);
   base.reset(new BaseDecl(baseName, access));
   SetContext(base.get(), start);
   return Success(Parser_GetBaseDecl, start);
}

//------------------------------------------------------------------------------

fn_name Parser_GetBasic = "Parser.GetBasic";

bool Parser::GetBasic(TokenPtr& statement)
{
   Debug::ft(Parser_GetBasic);

   auto start = lexer_.Curr();

   //  An expression statement is an assignment, function call, or null
   //  statement.  The latter is a bare semicolon.
   //
   if(lexer_.NextCharIs(';'))
   {
      statement.reset(new NoOp(start));
      return Success(Parser_GetBasic, start);
   }

   ExprPtr expr;
   auto end = lexer_.FindFirstOf(";");
   if(end == string::npos) return lexer_.Retreat(start);
   if(!GetCxxExpr(expr, end, false)) return lexer_.Retreat(start);
   if(!lexer_.NextCharIs(';')) return lexer_.Retreat(start);

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
   auto start = lexer_.Curr();

   auto braced = lexer_.NextCharIs('{');
   block.reset(new Block(braced));
   SetContext(block.get(), start);
   Context::PushScope(block.get());

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
   if(braced && !lexer_.NextCharIs('}')) return lexer_.Retreat(start);
   return Success(Parser_GetBlock, start);
}

//------------------------------------------------------------------------------

fn_name Parser_GetBraceInit = "Parser.GetBraceInit";

bool Parser::GetBraceInit(ExprPtr& expr)
{
   Debug::ft(Parser_GetBraceInit);

   auto start = lexer_.Curr();

   //  The left brace has already been parsed.  A comma is actually allowed
   //  to follow the final item in the list, just before the closing brace.
   //
   auto end = lexer_.FindClosing('{', '}');
   if(end == string::npos) return lexer_.Retreat(start);

   auto temps = TokenPtrVector();
   ExprPtr item;

   if(!lexer_.NextCharIs('}'))
   {
      while(true)
      {
         auto next = lexer_.FindFirstOf(",}");
         if(next == string::npos) return lexer_.Retreat(start);
         if(!GetCxxExpr(item, next)) break;
         auto init = TokenPtr(item.release());
         temps.push_back(std::move(init));
         auto comma = lexer_.NextCharIs(',');
         auto brace = lexer_.NextCharIs('}');
         if(brace) break;
         if(!comma) return lexer_.Retreat(start);
      }
   }

   auto token = TokenPtr(new BraceInit);
   auto brace = static_cast< BraceInit* >(token.get());

   for(size_t i = 0; i < temps.size(); ++i)
   {
      auto init = TokenPtr(temps.at(i).release());
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
   auto start = lexer_.Curr();

   //  The "break" keyword has already been parsed.
   //
   if(!lexer_.NextCharIs(';')) return lexer_.Retreat(start);
   statement.reset(new Break(begin));
   return Success(Parser_GetBreak, begin);
}

//------------------------------------------------------------------------------

fn_name Parser_GetCase = "Parser.GetCase";

bool Parser::GetCase(TokenPtr& statement)
{
   Debug::ft(Parser_GetCase);

   auto begin = kwdBegin_;
   auto start = lexer_.Curr();

   //  The "case" keyword has already been parsed.
   //
   ExprPtr expr;
   auto end = lexer_.FindFirstOf(":");
   if(end == string::npos) return lexer_.Retreat(start);
   if(!GetCxxExpr(expr, end)) return lexer_.Retreat(start);
   if(!lexer_.NextCharIs(':')) return lexer_.Retreat(start);

   statement.reset(new Case(expr, begin));
   return Success(Parser_GetCase, begin);
}

//------------------------------------------------------------------------------

fn_name Parser_GetCast = "Parser.GetCast";

bool Parser::GetCast(ExprPtr& expr)
{
   Debug::ft(Parser_GetCast);

   auto start = lexer_.Curr();

   //  The left parenthesis has already been parsed.
   //
   TypeSpecPtr spec;
   ExprPtr item;
   if(!GetTypeSpec(spec)) return lexer_.Retreat(start);
   if(!lexer_.NextCharIs(')')) return lexer_.Retreat(start);
   if(!GetCxxExpr(item, expr->EndPos(), false)) return lexer_.Retreat(start);
   spec->Check();  //* delay until >check

   auto token = TokenPtr(new Operation(Cxx::CAST));
   auto cast = static_cast< Operation* >(token.get());
   auto arg1 = TokenPtr(spec.release());
   auto arg2 = TokenPtr(item.release());
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

   auto start = lexer_.Curr();

   ArgumentPtr arg;
   BlockPtr handler;
   if(!NextKeywordIs(CATCH_STR)) return lexer_.Retreat(start);
   if(!lexer_.NextCharIs('(')) return lexer_.Retreat(start);

   if(lexer_.Extract(lexer_.Curr(), 3) == ELLIPSES_STR)
   {
      auto end = lexer_.FindClosing('(', ')');
      if(end == string::npos) return lexer_.Retreat(start);
      lexer_.Reposition(end);
   }
   else
   {
      if(!GetArgument(arg)) return lexer_.Retreat(start);
   }

   if(!lexer_.NextCharIs(')')) return lexer_.Retreat(start);
   if(!GetBlock(handler)) return lexer_.Retreat(start);

   statement.reset(new Catch(start));
   auto c = static_cast< Catch* >(statement.get());
   c->AddArg(arg);
   c->AddHandler(handler);
   return Success(Parser_GetCatch, start);
}

//------------------------------------------------------------------------------

fn_name Parser_GetChar = "Parser.GetChar";

bool Parser::GetChar(ExprPtr& expr)
{
   Debug::ft(Parser_GetChar);

   char c;
   if(!lexer_.GetChar(c)) return false;
   auto item = TokenPtr(new CharLiteral(c));
   expr->AddItem(item);
   return true;
}

//------------------------------------------------------------------------------

fn_name Parser_GetClass = "Parser.GetClass";

bool Parser::GetClass(Cxx::Keyword kwd, CxxArea* area)
{
   Debug::ft(Parser_GetClass);

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

   //  <ClassData> = ["static"] ["mutable"] ["constexpr"] <TypeSpec> <Name>
   //                [<ArraySpec>] [":" <Expr>] ["=" <Expr>] ";"
   //
   auto start = lexer_.Curr();

   TypeSpecPtr typeSpec;
   string dataName;
   ArraySpecPtr arraySpec;
   ExprPtr width;
   ExprPtr init;

   auto stat = NextKeywordIs(STATIC_STR);
   auto mute = NextKeywordIs(MUTABLE_STR);
   auto cexpr = NextKeywordIs(CONSTEXPR_STR);
   if(!GetTypeSpec(typeSpec)) return lexer_.Retreat(start);
   if(!lexer_.GetName(dataName)) return lexer_.Retreat(start);
   while(GetArraySpec(arraySpec)) typeSpec->AddArray(arraySpec);

   if(lexer_.NextStringIs(":"))
   {
      //  Get the data's field width.
      //
      auto end = lexer_.FindFirstOf(";=");
      if(end == string::npos) return lexer_.Retreat(start);
      if(!GetCxxExpr(width, end)) return lexer_.Retreat(start);
   }

   if(lexer_.NextStringIs("="))
   {
      if(lexer_.NextCharIs('{'))
      {
         if(!GetBraceInit(init)) return lexer_.Retreat(start);
      }
      else
      {
         auto end = lexer_.FindFirstOf(";");
         if(end == string::npos) return lexer_.Retreat(start);
         if(!GetCxxExpr(init, end)) return lexer_.Retreat(start);
      }
   }

   if(!lexer_.NextCharIs(';')) return lexer_.Retreat(start);
   data.reset(new ClassData(dataName, typeSpec));
   SetContext(data.get(), start);
   data->SetStatic(stat);
   data->SetConstexpr(cexpr);
   static_cast< ClassData* >(data.get())->SetMutable(mute);
   static_cast< ClassData* >(data.get())->SetWidth(width);
   data->SetAssignment(init);
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
   auto start = lexer_.Curr();

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
      if(!GetTemplateParms(parms)) return lexer_.Retreat(start);
      begin = kwdBegin_;
      if(!lexer_.GetClassTag(tag)) return lexer_.Retreat(start);
   }

   QualNamePtr className;
   if(!GetQualName(className))
   {
      if(tag != Cxx::UnionType) return lexer_.Retreat(start);
      className.reset(new QualName(EMPTY_STR));
   }

   if(lexer_.NextCharIs(';'))
   {
      //  A forward declaration.
      //
      forw.reset(new Forward(className, tag));
      forw->SetTemplateParms(parms);
      SetContext(forw.get(), begin);
      return Success(Parser_GetClassDecl, begin);
   }

   BaseDeclPtr base;
   GetBaseDecl(base);
   if(!lexer_.NextCharIs('{')) return lexer_.Retreat(start);
   cls.reset(new Class(className, tag));
   cls->SetTemplateParms(parms);
   SetContext(cls.get(), begin);
   Context::PushScope(cls.get());
   cls->AddBase(base);
   GetMemberDecls(cls.get());
   Context::PopScope();
   if(!lexer_.NextCharIs('}')) return lexer_.Retreat(start);
   if(!lexer_.NextCharIs(';')) return lexer_.Retreat(start);
   return Success(Parser_GetClassDecl, begin);
}

//------------------------------------------------------------------------------

fn_name Parser_GetCompoundType = "Parser.GetCompoundType";

bool Parser::GetCompoundType(QualNamePtr& name, Cxx::Type type)
{
   Debug::ft(Parser_GetCompoundType);

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

bool Parser::GetConditional(ExprPtr& expr)
{
   Debug::ft(Parser_GetConditional);

   auto start = lexer_.Curr();

   //  The "?" has already been parsed and should have beeen preceded by
   //  a valid expression.
   //
   ExprPtr exp1;
   ExprPtr exp0;
   auto end = lexer_.FindFirstOf(":");
   if(end == string::npos) return lexer_.Retreat(start);
   if(!GetCxxExpr(exp1, end)) return lexer_.Retreat(start);
   if(!lexer_.NextCharIs(':')) return lexer_.Retreat(start);
   if(!GetCxxExpr(exp0, expr->EndPos(), false)) return lexer_.Retreat(start);

   auto token = TokenPtr(new Operation(Cxx::CONDITIONAL));
   auto cond = static_cast< Operation* >(token.get());
   auto test = TokenPtr(new Elision);
   auto value1 = TokenPtr(exp1.release());
   auto value0 = TokenPtr(exp0.release());
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
   auto start = lexer_.Curr();

   //  The "continue" keyword has already been parsed.
   //
   if(!lexer_.NextCharIs(';')) return lexer_.Retreat(start);
   statement.reset(new Continue(begin));
   return Success(Parser_GetContinue, begin);
}

//------------------------------------------------------------------------------

fn_name Parser_GetCtorDecl = "Parser.GetCtorDecl";

bool Parser::GetCtorDecl(FunctionPtr& func)
{
   Debug::ft(Parser_GetCtorDecl);

   //  <CtorDecl> = ["explicit"] ["constexpr"] <Name> <Arguments> [<CtorInit>]
   //
   auto start = lexer_.Curr();

   string name;
   auto expl = NextKeywordIs(EXPLICIT_STR);
   auto cexpr = NextKeywordIs(CONSTEXPR_STR);
   if(!GetName(name)) return lexer_.Retreat(start);
   if(!lexer_.NextCharIs('(')) return lexer_.Retreat(start);
   auto ctorName = QualNamePtr(new QualName(name));
   func.reset(new Function(ctorName));
   SetContext(func.get(), start);
   func->SetExplicit(expl);
   func->SetConstexpr(cexpr);
   if(cexpr) func->SetInline(true);
   if(!GetArguments(func)) return Retreat(start, func);
   auto noex = NextKeywordIs(NOEXCEPT_STR);
   if(!GetCtorInit(func)) return Retreat(start, func);
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
   auto start = lexer_.Curr();

   //  Whether this is a constructor or not, GetQualName will parse the final
   //  scope qualifier and function name, so verify that the function name is
   //  actually repeated.
   //
   QualNamePtr ctorName;
   if(!GetQualName(ctorName)) return lexer_.Retreat(start);
   if(!lexer_.NextCharIs('(')) return lexer_.Retreat(start);
   if(!ctorName->CheckCtorDefn()) return lexer_.Retreat(start);
   func.reset(new Function(ctorName));
   SetContext(func.get(), start);
   if(!GetArguments(func)) return Retreat(start, func);
   auto noex = NextKeywordIs(NOEXCEPT_STR);
   if(!GetCtorInit(func)) return Retreat(start, func);
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
   auto start = lexer_.Curr();
   if(!lexer_.NextStringIs(":")) return Success(Parser_GetCtorInit, start);

   auto end = lexer_.FindFirstOf("{");
   if(end == string::npos) return lexer_.Retreat(start);

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
      begin = lexer_.Curr();
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
            call = base->NameRefersToItem(name, func.get(), file, &view);
         }
      }

      if(call)
      {
         token = TokenPtr(baseName.release());
         auto init = ExprPtr(new Expression(end, true));
         init->AddItem(token);
         if(!lexer_.NextCharIs('(')) return lexer_.Retreat(start);
         if(!GetArgList(token)) return lexer_.Retreat(start);
         init->AddItem(token);
         func->SetBaseInit(init);
      }
      else
      {
         if(!lexer_.NextCharIs('(')) return lexer_.Retreat(start);
         end = lexer_.FindClosing('(', ')');
         if(end == string::npos) return lexer_.Retreat(start);
         if(!GetArgList(token)) return lexer_.Retreat(start);
         memberName = *baseName->Name();
         auto mem = MemberInitPtr(new MemberInit(memberName, token));
         mem->SetPos(Context::File(), begin);
         func->AddMemberInit(mem);
      }
   }

   while(lexer_.NextCharIs(','))
   {
      begin = lexer_.Curr();
      if(!lexer_.GetName(memberName)) return lexer_.Retreat(start);
      if(!lexer_.NextCharIs('(')) return lexer_.Retreat(start);
      end = lexer_.FindClosing('(', ')');
      if(end == string::npos) return lexer_.Retreat(start);
      if(!GetArgList(token)) return lexer_.Retreat(start);
      auto mem = MemberInitPtr(new MemberInit(memberName, token));
      mem->SetPos(Context::File(), begin);
      func->AddMemberInit(mem);
   }

   return Success(Parser_GetCtorInit, start);
}

//------------------------------------------------------------------------------

fn_name Parser_GetCxxAlpha = "Parser.GetCxxAlpha";

bool Parser::GetCxxAlpha(ExprPtr& expr)
{
   Debug::ft(Parser_GetCxxAlpha);

   auto start = lexer_.Curr();

   TokenPtr item;
   QualNamePtr qualName;
   if(!GetQualName(qualName)) return lexer_.Retreat(start);

   if(qualName->Names_size() == 1)
   {
      //  See if the name is actually a keyword or operator.
      //
      auto op = Lexer::GetReserved(*qualName->Name());

      switch(op)
      {
      case Cxx::NIL_OPERATOR:
         if(!CheckType(qualName)) return lexer_.Retreat(start);
         break;

      case Cxx::FALSE:
      case Cxx::TRUE:
         item.reset(new BoolLiteral(op == Cxx::TRUE));
         if(expr->AddItem(item)) return true;
         return lexer_.Retreat(start);

      case Cxx::NULLPTR:
         item.reset(new NullPtr);
         if(expr->AddItem(item)) return true;
         return lexer_.Retreat(start);

      case Cxx::OBJECT_CREATE:
         if(GetNew(expr, op)) return true;
         return lexer_.Retreat(start);

      case Cxx::OBJECT_DELETE:
         if(lexer_.NextStringIs(ARRAY_STR)) op = Cxx::OBJECT_DELETE_ARRAY;
         if(GetDelete(expr, op)) return true;
         return lexer_.Retreat(start);

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
            if(GetCxxCast(expr, op)) return true;
            return lexer_.Retreat(start);
         }

      case Cxx::THROW:
         if(GetThrow(expr)) return true;
         return lexer_.Retreat(start);

      case Cxx::SIZEOF_TYPE:
         if(GetSizeOf(expr)) return true;
         return lexer_.Retreat(start);

      case Cxx::TYPE_NAME:
         if(GetTypeId(expr)) return true;
         return lexer_.Retreat(start);

      default:
         Debug::SwErr(Parser_GetCxxAlpha, op, 0);
         return lexer_.Retreat(start);
      }
   }

   item.reset(qualName.release());
   if(expr->AddItem(item)) return true;
   return lexer_.Retreat(start);
}

//------------------------------------------------------------------------------

fn_name Parser_GetCxxCast = "Parser.GetCxxCast";

bool Parser::GetCxxCast(ExprPtr& expr, Cxx::Operator op)
{
   Debug::ft(Parser_GetCxxCast);

   auto start = lexer_.Curr();

   //  The cast operator has already been parsed.
   //
   TypeSpecPtr spec;
   ExprPtr item;
   if(!lexer_.NextCharIs('<')) return lexer_.Retreat(start);
   if(!GetTypeSpec(spec)) return lexer_.Retreat(start);
   if(!lexer_.NextCharIs('>')) return lexer_.Retreat(start);
   if(!GetParExpr(item, false)) return lexer_.Retreat(start);
   spec->Check();  //* delay until >check

   auto token = TokenPtr(new Operation(op));
   auto cast = static_cast< Operation* >(token.get());
   auto arg1 = TokenPtr(spec.release());
   auto arg2 = TokenPtr(item.release());
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

   auto start = lexer_.Curr();

   char c;
   expr.reset(new Expression(end, force));

   while(lexer_.CurrChar(c) < end)
   {
      switch(c)
      {
      case QUOTE:
         if(GetStr(expr)) break;
         return Punt(expr, end);

      case APOSTROPHE:
         if(GetChar(expr)) break;
         return Punt(expr, end);

      case '{':
         return false;

      case '_':
         if(GetCxxAlpha(expr)) break;
         return Punt(expr, end);

      default:
         if(ispunct(c))
         {
            if(GetOp(expr, true)) break;
            return lexer_.Retreat(start);
         }
         if(isdigit(c))
         {
            if(GetNum(expr)) break;
            return Punt(expr, end);
         }
         if(GetCxxAlpha(expr)) break;
         if(GetOp(expr, true)) break;
         return Punt(expr, end);
      }
   }

   if(expr->Empty())
   {
      expr.release();
      return lexer_.Retreat(start);
   }

   return Success(Parser_GetCxxExpr, start);
}

//------------------------------------------------------------------------------

fn_name Parser_GetDefault = "Parser.GetDefault";

bool Parser::GetDefault(TokenPtr& statement)
{
   Debug::ft(Parser_GetDefault);

   auto begin = kwdBegin_;
   auto start = lexer_.Curr();

   //  The "default" keyword has already been parsed.
   //
   if(!lexer_.NextCharIs(':')) return lexer_.Retreat(start);
   string label(DEFAULT_STR);
   statement.reset(new Label(label, begin));
   return Success(Parser_GetDefault, begin);
}

//------------------------------------------------------------------------------

fn_name Parser_GetDefined = "Parser.GetDefined";

bool Parser::GetDefined(ExprPtr& expr)
{
   Debug::ft(Parser_GetDefined);

   auto start = lexer_.Curr();

   //  The defined operator has already been parsed.  Parentheses
   //  around the argument are optional.
   //
   string name;

   auto par = lexer_.NextCharIs('(');
   if(!lexer_.GetName(name)) return lexer_.Retreat(start);
   if(par && !lexer_.NextCharIs(')')) return lexer_.Retreat(start);

   auto token = TokenPtr(new Operation(Cxx::DEFINED));
   auto op = static_cast< Operation* >(token.get());
   TokenPtr arg = MacroNamePtr(new MacroName(name));
   op->AddArg(arg, false);
   expr->AddItem(token);
   return Success(Parser_GetDefined, start);
}

//------------------------------------------------------------------------------

fn_name Parser_GetDelete = "Parser.GetDelete";

bool Parser::GetDelete(ExprPtr& expr, Cxx::Operator op)
{
   Debug::ft(Parser_GetDelete);

   auto start = lexer_.Curr();

   //  The delete operator has already been parsed.
   //
   ExprPtr item;
   if(!GetCxxExpr(item, expr->EndPos(), false)) return lexer_.Retreat(start);

   auto token = TokenPtr(new Operation(op));
   auto delOp = static_cast< Operation* >(token.get());
   auto arg = TokenPtr(item.release());
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
   auto start = lexer_.Curr();

   //  The "do" keyword has already been parsed.
   //
   BlockPtr loop;
   ExprPtr condition;
   if(!GetBlock(loop)) return lexer_.Retreat(start);
   if(!NextKeywordIs(WHILE_STR)) return lexer_.Retreat(start);
   if(!GetParExpr(condition, false)) return lexer_.Retreat(start);
   if(!lexer_.NextCharIs(';')) return lexer_.Retreat(start);

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

   //  <DtorDecl> = ["virtual"] "~" <Name> "(" ")" ["noexcept"]
   //
   auto start = lexer_.Curr();
   auto virt = NextKeywordIs(VIRTUAL_STR);
   if(!lexer_.NextCharIs('~')) return lexer_.Retreat(start);

   string name;
   if(!GetName(name)) return lexer_.Retreat(start);
   if(!lexer_.NextCharIs('(')) return lexer_.Retreat(start);
   if(!lexer_.NextCharIs(')')) return lexer_.Retreat(start);
   auto noex = NextKeywordIs(NOEXCEPT_STR);

   name.insert(0, 1, '~');
   auto dtorName = QualNamePtr(new QualName(name));
   func.reset(new Function(dtorName));
   func->SetVirtual(virt);
   func->SetNoexcept(noex);
   SetContext(func.get(), start);
   return Success(Parser_GetDtorDecl, start);
}

//------------------------------------------------------------------------------

fn_name Parser_GetDtorDefn = "Parser.GetDtorDefn";

bool Parser::GetDtorDefn(FunctionPtr& func)
{
   Debug::ft(Parser_GetDtorDefn);

   //  <DtorDefn> = <QualName> "::~" <Name> "(" ")" ["noexcept"]
   //
   auto start = lexer_.Curr();

   QualNamePtr dtorName;
   string name;
   if(!GetQualName(dtorName)) return lexer_.Retreat(start);
   if(!lexer_.NextStringIs("::~")) return lexer_.Retreat(start);
   if(!lexer_.GetName(name)) return lexer_.Retreat(start);
   if(!lexer_.NextCharIs('(')) return lexer_.Retreat(start);
   if(!lexer_.NextCharIs(')')) return lexer_.Retreat(start);
   auto noex = NextKeywordIs(NOEXCEPT_STR);

   name.insert(0, 1, '~');
   auto className = TypeNamePtr(new TypeName(name));
   dtorName->AddTypeName(className);
   func.reset(new Function(dtorName));
   func->SetNoexcept(noex);
   SetContext(func.get(), start);
   return Success(Parser_GetDtorDefn, start);
}

//------------------------------------------------------------------------------

fn_name Parser_GetEnum = "Parser.GetEnum";

bool Parser::GetEnum(EnumPtr& decl)
{
   Debug::ft(Parser_GetEnum);

   //  <Enum> = "enum" [<Name>] "{" <Enumerator> ["," <Enumerator>]* "}" ";"
   //  The "enum" keyword has already been parsed.  An enum without enumerators
   //  is legal but seems to be useless and is therefore not supported.  After
   //  the last enumerator, a comma can actually precede the brace.
   //
   auto begin = kwdBegin_;
   auto start = lexer_.Curr();

   string enumName;
   lexer_.GetName(enumName);
   if(!lexer_.NextCharIs('{')) return lexer_.Retreat(start);

   string etorName;
   ExprPtr etorInit;
   auto etorPos = lexer_.Curr();
   if(!GetEnumerator(etorName, etorInit)) return lexer_.Retreat(start);
   decl.reset(new Enum(enumName));
   SetContext(decl.get(), begin);
   decl->AddEnumerator(etorName, etorInit, etorPos);

   while(true)
   {
      if(!lexer_.NextCharIs(',')) break;
      etorPos = lexer_.Curr();
      if(!GetEnumerator(etorName, etorInit)) break;
      decl->AddEnumerator(etorName, etorInit, etorPos);
   }

   if(!lexer_.NextCharIs('}')) return lexer_.Retreat(start);
   if(!lexer_.NextCharIs(';')) return lexer_.Retreat(start);
   return Success(Parser_GetEnum, begin);
}

//------------------------------------------------------------------------------

fn_name Parser_GetEnumerator = "Parser.GetEnumerator";

bool Parser::GetEnumerator(string& name, ExprPtr& init)
{
   Debug::ft(Parser_GetEnumerator);

   auto start = lexer_.Curr();

   //  <Enumerator> = <Name> ["=" <Expr>]
   //
   if(!lexer_.GetName(name)) return lexer_.Retreat(start);

   if(lexer_.NextCharIs('='))
   {
      auto end = lexer_.FindFirstOf(",}");
      if(end == string::npos) return lexer_.Retreat(start);
      if(!GetCxxExpr(init, end)) return lexer_.Retreat(start);
   }

   return Success(Parser_GetEnumerator, start);
}

//------------------------------------------------------------------------------

fn_name Parser_GetFileDecls = "Parser.GetFileDecls";

void Parser::GetFileDecls(Namespace* space)
{
   Debug::ft(Parser_GetFileDecls);

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
   auto start = lexer_.Curr();

   //  The "for" keyword has already been parsed.
   //
   TokenPtr initial;
   DataPtr data;
   if(!lexer_.NextCharIs('(')) return lexer_.Retreat(start);
   if(GetFuncData(data))
   {
      initial.reset(data.release());
   }
   else
   {
      ExprPtr expr;
      auto end = lexer_.FindFirstOf(";");
      if(end == string::npos) return lexer_.Retreat(start);
      GetCxxExpr(expr, end);
      if(!lexer_.NextCharIs(';')) return lexer_.Retreat(start);
      initial.reset(expr.release());
   }

   ExprPtr condition;
   ExprPtr subsequent;
   BlockPtr loop;
   auto end = lexer_.FindFirstOf(";");
   if(end == string::npos) return lexer_.Retreat(start);
   GetCxxExpr(condition, end);
   if(!lexer_.NextCharIs(';')) return lexer_.Retreat(start);
   if(!GetParExpr(subsequent, true, true)) return lexer_.Retreat(start);
   if(!GetBlock(loop)) return lexer_.Retreat(start);

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
   auto start = lexer_.Curr();

   TemplateParmsPtr parms;
   if(GetTemplateParms(parms))
   {
      begin = kwdBegin_;
      if(!NextKeywordIs(FRIEND_STR)) return lexer_.Retreat(start);
   }

   decl.reset(new Friend);
   SetContext(decl.get(), begin);

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
      if(!GetQualName(friendName)) return lexer_.Retreat(start);
      if(!lexer_.NextCharIs(';')) return lexer_.Retreat(start);
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

   //  <FuncData> = ["static"] ["constexpr"] <TypeSpec>
   //               (<FuncData1> | <FuncData2>)
   //  <FuncData1> = <Name> "(" [<Expr>] ")" ";"
   //  <FuncData2> =        <Name> ([<ArraySpec>] ["=" <Expr>]
   //    ["," ["*"]* ["&"]* <Name> ([<ArraySpec>] ["=" <Expr>]]* ";"
   //  FuncData1 initializes the data with a parenthesized expression that
   //  directly follows the name.  It is sometimes a constructor call:
   //    i.e. Class name(args); instead of auto name = Class(args);
   //  FuncData2 allows multiple declarations, based on the same root type,
   //  in a list that separates each declaration with a comma:
   //    e.g. int i = 0, *j = nullptr, k[10] = { };
   //
   auto start = lexer_.Curr();

   TypeSpecPtr typeSpec;
   string dataName;

   auto stat = NextKeywordIs(STATIC_STR);
   auto cexpr = NextKeywordIs(CONSTEXPR_STR);
   if(!GetTypeSpec(typeSpec)) return lexer_.Retreat(start);
   if(!lexer_.GetName(dataName)) return lexer_.Retreat(start);
   if(lexer_.NextCharIs('('))
   {
      //  A parenthesized expression is initializing the data.  Parse it as
      //  an argument list in case it is a constructor call.
      //
      TokenPtr expr;
      auto end = lexer_.FindClosing('(', ')');
      if(end == string::npos) return lexer_.Retreat(start);
      if(!GetArgList(expr)) return lexer_.Retreat(start);
      if(!lexer_.NextCharIs(';')) return lexer_.Retreat(start);

      data.reset(new FuncData(dataName, typeSpec));
      SetContext(data.get(), start);
      data->SetStatic(stat);
      data->SetConstexpr(cexpr);
      static_cast< FuncData* >(data.get())->SetExpression(expr);
      SetContext(data.get(), start);
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
         GetPointers(typeSpec.get());
         GetReferences(typeSpec.get());
         if(!lexer_.GetName(dataName)) return lexer_.Retreat(start);
      }

      while(GetArraySpec(arraySpec)) typeSpec->AddArray(arraySpec);

      if(lexer_.NextStringIs("="))
      {
         if(lexer_.NextCharIs('{'))
         {
            if(!GetBraceInit(init)) return lexer_.Retreat(start);
         }
         else
         {
            auto end = lexer_.FindFirstOf(",;");
            if(end == string::npos) return lexer_.Retreat(start);
            if(!GetCxxExpr(init, end)) return lexer_.Retreat(start);
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
         auto subseq = DataPtr(new FuncData(dataName, typeSpec));
         curr = static_cast< FuncData* >(subseq.get());
         prev->SetNext(subseq);
      }

      SetContext(curr, start);
      curr->SetStatic(stat);
      curr->SetConstexpr(cexpr);
      curr->SetAssignment(init);
      prev = curr;
   }
   while(lexer_.NextCharIs(','));

   if(!lexer_.NextCharIs(';')) return lexer_.Retreat(start);
   return Success(Parser_GetFuncData, start);
}

//------------------------------------------------------------------------------

fn_name Parser_GetFuncDecl = "Parser.GetFuncDecl";

bool Parser::GetFuncDecl(Cxx::Keyword kwd, FunctionPtr& func)
{
   Debug::ft(Parser_GetFuncDecl);

   //  <FuncDecl> = ["extern"] [<TemplateParms>] ["inline"]
   //               (<CtorDecl> | <DtorDecl> | <ProcDecl>) (<FuncImpl> | ";")
   //
   auto start = lexer_.Curr();
   auto begin = start;
   auto found = false;
   auto extn = false;
   auto inln = false;

   TemplateParmsPtr parms;

   switch(kwd)
   {
   case Cxx::EXTERN:
      extn = true;
      if(GetTemplateParms(parms)) begin = lexer_.Curr();
      break;
   case Cxx::TEMPLATE:
      if(!GetTemplateParms(parms)) return lexer_.Retreat(start);
      begin = lexer_.Curr();
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
      inln = true;
      found = (GetCtorDecl(func) || GetDtorDecl(func) || GetProcDecl(func));
      break;
   }

   if(!found) return Retreat(start, func);
   func->SetTemplateParms(parms);
   func->SetExtern(extn);
   func->SetInline(inln);

   //  The next character should be a semicolon or left brace, depending
   //  on whether the function is only declared here or also defined.
   //
   if(lexer_.NextCharIs(';'))
   {
      func->SetDefnRange(begin, lexer_.rfind(';'));
   }
   else
   {
      auto pos = lexer_.Curr();
      if(!lexer_.NextCharIs('{')) return Retreat(start, func);

      auto end = lexer_.FindClosing('{', '}');
      if(end == string::npos) return Retreat(start, func);
      func->SetDefnRange(begin, end);

      //  Wait to parse a class's inlines until the rest of the
      //  class has been parsed.
      //
      if(func->AtFileScope())
      {
         lexer_.Reposition(pos);
         if(!GetFuncImpl(func.get())) return Retreat(start, func);
      }
      else
      {
         func->SetBracePos(pos);
         lexer_.Reposition(end + 1);
         if(lexer_.NextCharIs(';')) Log(RemoveSemicolon);
      }
   }

   return Success(Parser_GetFuncDecl, start);
}

//------------------------------------------------------------------------------

fn_name Parser_GetFuncDefn = "Parser.GetFuncDefn";

bool Parser::GetFuncDefn(Cxx::Keyword kwd, FunctionPtr& func)
{
   Debug::ft(Parser_GetFuncDefn);

   //  <FuncDefn> = [<TemplateParms>] ["inline"]
   //               (<CtorDefn> | <DtorDefn> | <ProcDefn>) <FuncImpl>
   //
   auto start = lexer_.Curr();
   auto begin = start;
   auto found = false;
   auto inln = false;

   TemplateParmsPtr parms;

   if(kwd == Cxx::TEMPLATE)
   {
      if(!GetTemplateParms(parms)) return lexer_.Retreat(start);
      begin = lexer_.Curr();
   }

   switch(kwd)
   {
   case Cxx::INLINE:
      inln = true;
   case Cxx::NIL_KEYWORD:
      found = (GetCtorDefn(func) || GetDtorDefn(func) || GetProcDefn(func));
      break;
   default:
      found = GetProcDefn(func);
   }

   if(!found) return Retreat(start, func);
   func->SetTemplateParms(parms);
   func->SetInline(inln);

   auto curr = lexer_.Curr();
   if(!lexer_.NextCharIs('{')) return Retreat(start, func);
   lexer_.Reposition(curr);
   if(!GetFuncImpl(func.get())) return Retreat(start, func);
   func->SetDefnRange(begin, lexer_.rfind('}'));
   return Success(Parser_GetFuncDefn, start);
}

//------------------------------------------------------------------------------

fn_name Parser_GetFuncImpl = "Parser.GetFuncImpl";

bool Parser::GetFuncImpl(Function* func)
{
   Debug::ft(Parser_GetFuncImpl);

   auto start = lexer_.Curr();

   Context::PushScope(func);

   BlockPtr block;
   if(!GetBlock(block))
   {
      //  The function implementation was not parsed successfully.
      //  Skip it and continue with the next item.
      //
      auto expl = *func->Name() + " failed near" + CRLF + lexer_.CurrLine();
      Debug::SwErr(Parser_GetFuncImpl, expl, 0);

      lexer_.Reposition(start);
      if(!lexer_.NextCharIs('{')) return false;
      auto end = lexer_.FindClosing('{', '}');
      lexer_.Reposition(end + 1);
      Context::PopScope();
      return true;
   }

   Context::PopScope();
   func->SetImpl(block);
   if(lexer_.NextCharIs(';')) Log(RemoveSemicolon);
   return Success(Parser_GetFuncImpl, start);
}

//------------------------------------------------------------------------------

fn_name Parser_GetFuncSpec = "Parser.GetFuncSpec";

bool Parser::GetFuncSpec(TypeSpecPtr& spec, FunctionPtr& func)
{
   Debug::ft(Parser_GetFuncSpec);

   //  <FuncSpec> = "(" "*" <Name> ")" <Argument>
   //  GetTypeSpec has already parsed the function's return type.
   //
   auto start = lexer_.Curr();
   if(!lexer_.NextCharIs('(')) return lexer_.Retreat(start);
   if(!lexer_.NextCharIs('*')) return lexer_.Retreat(start);

   string name;
   if(!lexer_.GetName(name)) return lexer_.Retreat(start);
   if(!lexer_.NextCharIs(')')) return lexer_.Retreat(start);
   if(!lexer_.NextCharIs('(')) return lexer_.Retreat(start);

   name.insert(0, "(*");
   name += ')';
   auto funcName = QualNamePtr(new QualName(name));
   func.reset(new Function(funcName, spec, true));
   SetContext(func.get(), start);
   if(!GetArguments(func)) return Retreat(start, func);
   return Success(Parser_GetFuncSpec, start);
}

//------------------------------------------------------------------------------

fn_name Parser_GetIf = "Parser.GetIf";

bool Parser::GetIf(TokenPtr& statement)
{
   Debug::ft(Parser_GetIf);

   auto begin = kwdBegin_;
   auto start = lexer_.Curr();

   //  The "if" keyword has already been parsed.
   //
   ExprPtr condition;
   BlockPtr thenBlock;
   BlockPtr elseBlock;
   if(!GetParExpr(condition, false)) return lexer_.Retreat(start);
   if(!GetBlock(thenBlock)) return lexer_.Retreat(start);
   if(NextKeywordIs(ELSE_STR))
   {
      if(!GetBlock(elseBlock)) return lexer_.Retreat(start);
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

fn_name Parser_GetInlines = "Parser.GetInlines";

bool Parser::GetInlines(Class* cls)
{
   Debug::ft(Parser_GetInlines);

   Context::PushScope(cls);

   auto end = lexer_.Curr();
   auto funcs = cls->Funcs();

   for(size_t i = 0; i < funcs->size(); ++i)
   {
      auto pos = funcs->at(i)->GetBracePos();

      if(pos != string::npos)
      {
         lexer_.Reposition(pos);
         GetFuncImpl(funcs->at(i).get());
      }
   }

   auto opers = cls->Opers();

   for(size_t i = 0; i < opers->size(); ++i)
   {
      auto pos = opers->at(i)->GetBracePos();

      if(pos != string::npos)
      {
         lexer_.Reposition(pos);
         GetFuncImpl(opers->at(i).get());
      }
   }

   auto friends = cls->Friends();

   for(size_t i = 0; i < friends->size(); ++i)
   {
      auto func = friends->at(i)->Inline();

      if(func != nullptr)
      {
         auto pos = func->GetBracePos();

         if(pos != string::npos)
         {
            lexer_.Reposition(pos);
            GetFuncImpl(func);
         }
      }
   }

   lexer_.Reposition(end);
   Context::PopScope();
   return true;
}

//------------------------------------------------------------------------------

fn_name Parser_GetMemberDecls = "Parser.GetMemberDecls";

void Parser::GetMemberDecls(Class* cls)
{
   Debug::ft(Parser_GetMemberDecls);

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

fn_name Parser_GetName = "Parser.GetName";

bool Parser::GetName(string& name)
{
   Debug::ft(Parser_GetName);

   if(!lexer_.GetName(name)) return false;

   if(tmpltClassInst_)
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
   auto start = lexer_.Curr();

   string name;
   if(!lexer_.GetName(name)) return lexer_.Retreat(start);
   if(!lexer_.NextCharIs('{')) return lexer_.Retreat(start);

   auto outer = Context::Scope();
   auto inner = static_cast< Namespace* >(outer)->EnsureNamespace(name);
   inner->SetPos(Context::File(), begin);
   Context::PushScope(inner);
   GetFileDecls(inner);
   Context::PopScope();

   if(!lexer_.NextCharIs('}')) return lexer_.Retreat(start);
   if(lexer_.NextCharIs(';')) Log(RemoveSemicolon);
   return Success(Parser_GetNamespace, begin);
}

//------------------------------------------------------------------------------

fn_name Parser_GetNew = "Parser.GetNew";

bool Parser::GetNew(ExprPtr& expr, Cxx::Operator op)
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
   auto token = TokenPtr(new Operation(op));
   auto newOp = static_cast< Operation* >(token.get());
   expr->AddItem(token);

   auto start = lexer_.Curr();

   //  See if there are arguments for operator new itself.  If not, add an
   //  empty function call so that the TypeSpec will always be the second
   //  argument.
   //
   if(lexer_.NextCharIs('('))
   {
      if(!GetArgList(token)) return lexer_.Retreat(start);
   }
   else
   {
      token.reset(new Operation(Cxx::FUNCTION_CALL));
   }

   newOp->AddArg(token, false);
   start = lexer_.Curr();

   //  Add the type that is being created.
   //
   TypeSpecPtr typeSpec;
   if(!GetTypeSpec(typeSpec)) return lexer_.Retreat(start);
   typeSpec->Check();  //* delay until >check
   token.reset(typeSpec.release());
   newOp->AddArg(token, false);

   start = lexer_.Curr();

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
         if(!GetArgList(token)) return lexer_.Retreat(start);
         newOp->AddArg(token, false);
      }
   }

   return Success(Parser_GetNew, start);
}

//------------------------------------------------------------------------------

fn_name Parser_GetNum = "Parser.GetNum";

bool Parser::GetNum(ExprPtr& expr)
{
   Debug::ft(Parser_GetNum);

   TokenPtr item;
   if(!lexer_.GetNum(item)) return false;
   expr->AddItem(item);
   return true;
}

//------------------------------------------------------------------------------

fn_name Parser_GetOp = "Parser.GetOp";

bool Parser::GetOp(ExprPtr& expr, bool cxx)
{
   Debug::ft(Parser_GetOp);

   auto start = lexer_.Curr();

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
      return GetSubscript(expr);
   case Cxx::CONDITIONAL:
      return GetConditional(expr);
   case Cxx::ONES_COMPLEMENT:
      return HandleTilde(expr, start);
   case Cxx::SCOPE_RESOLUTION:
      lexer_.Reposition(start);
      if(!GetQualName(qualName)) return false;
      item.reset(qualName.release());
      break;
   default:
      item.reset(new Operation(op));
   }

   if(expr->AddItem(item)) return true;
   return lexer_.Retreat(start);
}

//------------------------------------------------------------------------------

fn_name Parser_GetParExpr = "Parser.GetParExpr";

bool Parser::GetParExpr(ExprPtr& expr, bool omit, bool opt)
{
   Debug::ft(Parser_GetParExpr);

   auto start = lexer_.Curr();

   //  Parse the expression inside the parentheses.
   //
   if(!omit && !lexer_.NextCharIs('(')) return lexer_.Retreat(start);
   auto end = lexer_.FindClosing('(', ')');
   if(end == string::npos) return lexer_.Retreat(start);
   if(!GetCxxExpr(expr, end) && !opt) return lexer_.Retreat(start);
   if(!lexer_.NextCharIs(')')) return lexer_.Retreat(start);
   return true;
}

//------------------------------------------------------------------------------

fn_name Parser_GetPointers1 = "Parser.GetPointers";

size_t Parser::GetPointers()
{
   Debug::ft(Parser_GetPointers1);

   bool space;
   return lexer_.GetIndirectionLevel('*', space);
}

//------------------------------------------------------------------------------

fn_name Parser_GetPointers2 = "Parser.GetPointers(spec)";

void Parser::GetPointers(TypeSpec* spec)
{
   Debug::ft(Parser_GetPointers2);

   //  It is too early to invoke Log(PtrTagDetached) here.  The parser
   //  tries data declarations first, so the "*" may actually turn out
   //  to be a multiplication operator.
   //
   bool space1, space2;
   auto ptrs1 = lexer_.GetIndirectionLevel('*', space1);
   auto array = lexer_.NextStringIs(ARRAY_STR, false);
   auto ptrs2 = lexer_.GetIndirectionLevel('*', space2);
   if(space1 || space2) spec->SetPtrDetached(true);
   spec->SetPtrs(ptrs1 + ptrs2);
   if(array) spec->SetArrayPos(ptrs1);
}

//------------------------------------------------------------------------------

fn_name Parser_GetPos = "Parser.GetPos";

string Parser::GetPos() const
{
   Debug::ft(Parser_GetPos);

   std::ostringstream stream;

   if(tmpltClassInst_ || tmpltFuncInst_)
   {
      stream << "in template " << tmpltName_;
   }
   else
   {
      stream << Context::File()->GetLineNum(lexer_.Curr());
   }

   return stream.str();
}

//------------------------------------------------------------------------------

fn_name Parser_GetPreAlpha = "Parser.GetPreAlpha";

bool Parser::GetPreAlpha(ExprPtr& expr)
{
   Debug::ft(Parser_GetPreAlpha);

   auto start = lexer_.Curr();

   //  Look for "defined", which is actually an operator.
   //
   string name;
   if(!lexer_.GetName(name)) return lexer_.Retreat(start);

   if(name == DEFINED_STR)
   {
      if(GetDefined(expr)) return true;
      return lexer_.Retreat(start);
   }

   TokenPtr item = MacroNamePtr(new MacroName(name));
   if(expr->AddItem(item)) return true;
   return lexer_.Retreat(start);
}

//------------------------------------------------------------------------------

fn_name Parser_GetPrecedence = "Parser.GetPrecedence";

bool Parser::GetPrecedence(ExprPtr& expr)
{
   Debug::ft(Parser_GetPrecedence);

   auto start = lexer_.Curr();

   //  The left parenthesis has already been parsed.
   //
   ExprPtr item;
   if(!GetParExpr(item, true)) return lexer_.Retreat(start);

   auto token = TokenPtr(new Precedence(item));
   expr->AddItem(token);
   return Success(Parser_GetPrecedence, start);
}

//------------------------------------------------------------------------------

fn_name Parser_GetPreExpr = "Parser.GetPreExpr";

bool Parser::GetPreExpr(ExprPtr& expr, size_t end)
{
   Debug::ft(Parser_GetPreExpr);

   auto start = lexer_.Curr();

   char c;
   expr.reset(new Expression(end, true));

   while(lexer_.CurrChar(c) < end)
   {
      switch(c)
      {
      case QUOTE:
         if(GetStr(expr)) break;
         return Punt(expr, end);

      case APOSTROPHE:
         if(GetChar(expr)) break;
         return Punt(expr, end);

      case '{':
         return false;

      case '_':
         if(GetPreAlpha(expr)) break;
         return Punt(expr, end);

      default:
         if(ispunct(c))
         {
            if(GetOp(expr, false)) break;
            return lexer_.Retreat(start);
         }
         if(isdigit(c))
         {
            if(GetNum(expr)) break;
            return Punt(expr, end);
         }
         if(GetPreAlpha(expr)) break;
         if(GetOp(expr, false)) break;
         return Punt(expr, end);
      }
   }

   if(expr->Empty())
   {
      expr.release();
      return lexer_.Retreat(start);
   }

   return Success(Parser_GetPreExpr, start);
}

//------------------------------------------------------------------------------

fn_name Parser_GetProcDecl = "Parser.GetProcDecl";

bool Parser::GetProcDecl(FunctionPtr& func)
{
   Debug::ft(Parser_GetProcDecl);

   //  <ProcDecl> = ["static"] ["virtual"] ["explicit"] ["constexpr"]
   //               (<StdProc> | <ConvOper>)
   //               <Arguments> ["const"] ["noexcept"] ["= 0" | "override"]
   //  <StdProc> = <TypeSpec> (<Name> | "operator" <Operator>)
   //  <ConvOper> = "operator" <TypeSpec>
   //
   auto start = lexer_.Curr();
   auto stat = NextKeywordIs(STATIC_STR);
   auto virt = NextKeywordIs(VIRTUAL_STR);
   auto expl = NextKeywordIs(EXPLICIT_STR);
   auto cexpr = NextKeywordIs(CONSTEXPR_STR);
   auto oper = Cxx::NIL_OPERATOR;
   auto pure = false;

   TypeSpecPtr typeSpec;
   string name;

   if(NextKeywordIs(OPERATOR_STR))
   {
      if(!GetTypeSpec(typeSpec)) return lexer_.Retreat(start);
      name = OPERATOR_STR;
      oper = Cxx::CAST;
   }
   else
   {
      if(!GetTypeSpec(typeSpec)) return lexer_.Retreat(start);
      if(!lexer_.GetName(name, oper)) return lexer_.Retreat(start);
   }

   if(!lexer_.NextCharIs('(')) return lexer_.Retreat(start);
   auto funcName = QualNamePtr(new QualName(name));
   func.reset(new Function(funcName, typeSpec));
   SetContext(func.get(), start);
   if(!GetArguments(func)) return Retreat(start, func);
   func->SetOperator(oper);

   auto readonly = NextKeywordIs(CONST_STR);
   auto noex = NextKeywordIs(NOEXCEPT_STR);
   auto over = NextKeywordIs(OVERRIDE_STR);
   if(virt) pure = (lexer_.NextStringIs("=") && lexer_.NextCharIs('0'));

   func->SetStatic(stat, oper);
   func->SetVirtual(virt);
   func->SetExplicit(expl);
   func->SetConstexpr(cexpr);
   if(cexpr) func->SetInline(true);
   func->SetConst(readonly);
   func->SetNoexcept(noex);
   func->SetOverride(over);
   func->SetPure(pure);
   return Success(Parser_GetProcDecl, start);
}

//------------------------------------------------------------------------------

fn_name Parser_GetProcDefn = "Parser.GetProcDefn";

bool Parser::GetProcDefn(FunctionPtr& func)
{
   Debug::ft(Parser_GetProcDefn);

   //  <ProcDefn> = <TypeSpec> <QualName> <Arguments> ["const"] ["noexcept"]
   //
   auto start = lexer_.Curr();

   TypeSpecPtr typeSpec;
   if(!GetTypeSpec(typeSpec)) return lexer_.Retreat(start);

   //  If this is a function template instance, append the template
   //  arguments to the name.  GetQualName cannot be used because it
   //  will also parse the template arguments.
   //
   QualNamePtr funcName;
   if(tmpltFuncInst_)
   {
      string name;
      Cxx::Operator oper;
      if(!lexer_.GetName(name, oper)) return lexer_.Retreat(start);
      funcName.reset(new QualName(name));
      funcName->SetOperator(oper);
      string spec;
      if(!lexer_.GetTemplateSpec(spec)) return lexer_.Retreat(start);
      funcName->Append(spec);
   }
   else
   {
      if(!GetQualName(funcName)) return lexer_.Retreat(start);
   }
   if(!lexer_.NextCharIs('(')) return lexer_.Retreat(start);

   auto oper = funcName->Operator();
   func.reset(new Function(funcName, typeSpec));
   SetContext(func.get(), start);
   if(!GetArguments(func)) return Retreat(start, func);
   func->SetOperator(oper);

   auto readonly = NextKeywordIs(CONST_STR);
   auto noex = NextKeywordIs(NOEXCEPT_STR);
   func->SetConst(readonly);
   func->SetNoexcept(noex);
   return Success(Parser_GetProcDefn, start);
}

//------------------------------------------------------------------------------

fn_name Parser_GetQualName = "Parser.GetQualName";

bool Parser::GetQualName(QualNamePtr& name)
{
   Debug::ft(Parser_GetQualName);

   //  <QualName> = [ <TypeName> "::" ]* (<TypeName> | "operator" <Operator>)
   //
   auto start = lexer_.Curr();

   TypeNamePtr type;
   auto global = lexer_.NextStringIs(SCOPE_STR);
   if(!GetTypeName(type)) return lexer_.Retreat(start);
   name.reset(new QualName(type));
   name->SetGlobal(global);

   while(lexer_.NextStringIs(SCOPE_STR))
   {
      if(!GetTypeName(type)) return lexer_.Retreat(start);
      name->AddTypeName(type);
   }

   if(*name->Name() == OPERATOR_STR)
   {
      Cxx::Operator oper;

      if(!lexer_.GetOpOverride(oper))
      {
         Debug::SwErr(Parser_GetQualName, 0, 0);
         return lexer_.Retreat(start);
      }

      name->SetOperator(oper);
   }

   SetContext(name.get(), start);
   return true;
}

//------------------------------------------------------------------------------

fn_name Parser_GetReferences = "Parser.GetReferences";

void Parser::GetReferences(TypeSpec* spec)
{
   Debug::ft(Parser_GetReferences);

   //  It is too early to invoke Log(PtrTagDetached) here.  The parser
   //  tries data declarations first, so the "*" may actually turn out
   //  to be a multiplication operator.
   //
   bool space;
   auto refs = lexer_.GetIndirectionLevel('&', space);
   if(space) spec->SetRefDetached(true);
   spec->SetRefs(refs);
}

//------------------------------------------------------------------------------

fn_name Parser_GetReturn = "Parser.GetReturn";

bool Parser::GetReturn(TokenPtr& statement)
{
   Debug::ft(Parser_GetReturn);

   auto begin = kwdBegin_;
   auto start = lexer_.Curr();

   //  The "return" keyword has already been parsed.
   //
   ExprPtr expr;
   auto end = lexer_.FindFirstOf(";");
   if(end == string::npos) return lexer_.Retreat(start);
   GetCxxExpr(expr, end);
   if(!lexer_.NextCharIs(';')) return lexer_.Retreat(start);

   statement.reset(new Return(begin));
   static_cast< Return* >(statement.get())->AddExpr(expr);
   return Success(Parser_GetReturn, begin);
}

//------------------------------------------------------------------------------

fn_name Parser_GetSizeOf = "Parser.GetSizeOf";

bool Parser::GetSizeOf(ExprPtr& expr)
{
   Debug::ft(Parser_GetSizeOf);

   auto start = lexer_.Curr();

   //  The sizeof operator has already been parsed.  Its argument can be a
   //  name (e.g. a local or argument), a type, or an expression.
   //
   if(!lexer_.NextCharIs('(')) return lexer_.Retreat(start);
   auto mark = lexer_.Curr();
   TokenPtr arg;

   do
   {
      QualNamePtr name;
      if(GetQualName(name))
      {
         arg.reset(name.release());
         if(lexer_.NextCharIs(')')) break;
         lexer_.Reposition(mark);
      }

      TypeSpecPtr spec;
      if(GetTypeSpec(spec))
      {
         spec->Check();  //* delay until >check
         arg.reset(spec.release());
         if(lexer_.NextCharIs(')')) break;
         lexer_.Reposition(mark);
      }

      ExprPtr size;
      if(!GetParExpr(size, true)) return lexer_.Retreat(start);
      arg.reset(size.release());
   }
   while(false);

   auto token = TokenPtr(new Operation(Cxx::SIZEOF_TYPE));
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

   //  <SpaceData> = ["extern"] [<TemplateParms>] ["static"] ["constexpr"]
   //                <TypeSpec> <QualName> (<SpaceData1> | <SpaceData2>)
   //  <SpaceData1> = "(" [<Expr>] ")" ";"
   //  <SpaceData2> =  [<ArraySpec>] ["=" <Expr>] ";"
   //  SpaceData1 initializes the data parenthesized expression that
   //  directly follows the name.
   //
   auto start = lexer_.Curr();

   TemplateParmsPtr parms;
   TypeSpecPtr typeSpec;
   QualNamePtr dataName;
   ArraySpecPtr arraySpec;
   TokenPtr expr;
   ExprPtr init;
   auto extn = false;

   switch(kwd)
   {
   case Cxx::EXTERN:
      extn = true;
      break;
   case Cxx::TEMPLATE:
      if(!GetTemplateParms(parms)) return lexer_.Retreat(start);
      break;
   }

   auto stat = NextKeywordIs(STATIC_STR);
   auto cexpr = NextKeywordIs(CONSTEXPR_STR);
   if(!GetTypeSpec(typeSpec)) return lexer_.Retreat(start);
   if(!GetQualName(dataName)) return lexer_.Retreat(start);
   if(dataName->Operator() != Cxx::NIL_OPERATOR) return lexer_.Retreat(start);

   if(lexer_.NextCharIs('('))
   {
      auto end = lexer_.FindClosing('(', ')');
      if(end == string::npos) return lexer_.Retreat(start);
      if(!GetArgList(expr)) return lexer_.Retreat(start);
   }
   else
   {
      while(GetArraySpec(arraySpec)) typeSpec->AddArray(arraySpec);

      if(lexer_.NextStringIs("="))
      {
         if(lexer_.NextCharIs('{'))
         {
            if(!GetBraceInit(init)) return lexer_.Retreat(start);
         }
         else
         {
            auto end = lexer_.FindFirstOf(";");
            if(end == string::npos) return lexer_.Retreat(start);
            if(!GetCxxExpr(init, end)) return lexer_.Retreat(start);
         }
      }
   }

   if(!lexer_.NextCharIs(';')) return lexer_.Retreat(start);
   data.reset(new SpaceData(dataName, typeSpec));
   SetContext(data.get(), start);
   data->SetTemplateParms(parms);
   data->SetExtern(extn);
   data->SetStatic(stat);
   data->SetConstexpr(cexpr);
   data->SetExpression(expr);
   data->SetAssignment(init);
   return Success(Parser_GetSpaceData, start);
}

//------------------------------------------------------------------------------

fn_name Parser_GetStatements = "Parser.GetStatements";

bool Parser::GetStatements(BlockPtr& block, bool braced)
{
   Debug::ft(Parser_GetStatements);

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

fn_name Parser_GetStr = "Parser.GetStr";

bool Parser::GetStr(ExprPtr& expr)
{
   Debug::ft(Parser_GetStr);

   string s;
   if(!lexer_.GetStr(s)) return false;
   auto item = TokenPtr(new StrLiteral(s));
   expr->AddItem(item);
   return true;
}

//------------------------------------------------------------------------------

fn_name Parser_GetSubscript = "Parser.GetSubscript";

bool Parser::GetSubscript(ExprPtr& expr)
{
   Debug::ft(Parser_GetSubscript);

   auto start = lexer_.Curr();

   //  The left bracket has already been parsed.
   //
   ExprPtr item;
   auto end = lexer_.FindClosing('[', ']');
   if(end == string::npos) return lexer_.Retreat(start);
   if(!GetCxxExpr(item, end)) return lexer_.Retreat(start);
   if(!lexer_.NextCharIs(']')) return lexer_.Retreat(start);

   //  The array subscript operator is binary, so adding it to the expression
   //  causes it to take what preceded it (the array) as its first argument.
   //  Once that is finished, the expression for the array index can be added.
   //
   auto token = TokenPtr(new Operation(Cxx::ARRAY_SUBSCRIPT));
   auto op = static_cast< Operation* >(token.get());
   expr->AddItem(token);
   auto index = TokenPtr(item.release());
   op->AddArg(index, false);
   return Success(Parser_GetSubscript, start);
}

//------------------------------------------------------------------------------

fn_name Parser_GetSwitch = "Parser.GetSwitch";

bool Parser::GetSwitch(TokenPtr& statement)
{
   Debug::ft(Parser_GetSwitch);

   auto begin = kwdBegin_;
   auto start = lexer_.Curr();

   //  The "switch" keyword has already been parsed.
   //
   ExprPtr value;
   BlockPtr cases;
   if(!GetParExpr(value, false)) return lexer_.Retreat(start);
   if(!GetBlock(cases)) return lexer_.Retreat(start);

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

   //  <TemplateParm> = <ClassTag> <Name> ["*"]* ["=" <TypeName>]
   //
   auto start = lexer_.Curr();

   Cxx::ClassTag tag;
   if(!lexer_.GetClassTag(tag, true)) return lexer_.Retreat(start);

   string argName;
   if(!lexer_.GetName(argName)) return lexer_.Retreat(start);

   auto ptrs = GetPointers();

   TypeNamePtr type;

   if(lexer_.NextCharIs('='))
   {
      if(!GetTypeName(type)) return lexer_.Retreat(start);
   }

   parm.reset(new TemplateParm(argName, tag, ptrs, type));
   return Success(Parser_GetTemplateParm, start);
}

//------------------------------------------------------------------------------

fn_name Parser_GetTemplateParms = "Parser.GetTemplateParms";

bool Parser::GetTemplateParms(TemplateParmsPtr& parms)
{
   Debug::ft(Parser_GetTemplateParms);

   //  <TemplateParms> = "template" "<" <TemplateParm> ["," <TemplateParm>]* ">"
   //
   auto start = lexer_.Curr();
   if(!NextKeywordIs(TEMPLATE_STR)) return lexer_.Retreat(start);
   if(!lexer_.NextCharIs('<')) return lexer_.Retreat(start);

   TemplateParmPtr parm;
   if(!GetTemplateParm(parm)) return lexer_.Retreat(start);

   parms.reset(new TemplateParms(parm));

   while(lexer_.NextCharIs(','))
   {
      if(!GetTemplateParm(parm)) return lexer_.Retreat(start);
      parms->AddParm(parm);
   }

   if(!lexer_.NextCharIs('>')) return lexer_.Retreat(start);
   return Success(Parser_GetTemplateParms, start);
}

//------------------------------------------------------------------------------

fn_name Parser_GetThrow = "Parser.GetThrow";

bool Parser::GetThrow(ExprPtr& expr)
{
   Debug::ft(Parser_GetThrow);

   auto start = lexer_.Curr();

   //  The throw operator has already been parsed.
   //
   ExprPtr item;
   GetCxxExpr(item, expr->EndPos(), false);

   auto token = TokenPtr(new Operation(Cxx::THROW));
   auto op = static_cast< Operation* >(token.get());
   auto arg = TokenPtr(item.release());
   if(arg != nullptr) op->AddArg(arg, false);
   expr->AddItem(token);
   return Success(Parser_GetThrow, start);
}

//------------------------------------------------------------------------------

fn_name Parser_GetTime = "Parser.GetTime";

const SysTime* Parser::GetTime()
{
   Debug::ft(Parser_GetTime);

   return &Context::RootParser()->time_;
}

//------------------------------------------------------------------------------

fn_name Parser_GetTry = "Parser.GetTry";

bool Parser::GetTry(TokenPtr& statement)
{
   Debug::ft(Parser_GetTry);

   auto begin = kwdBegin_;
   auto start = lexer_.Curr();

   //  The "try" keyword has already been parsed.
   //
   BlockPtr work;
   TokenPtr trap;
   if(!GetBlock(work)) return lexer_.Retreat(start);

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

   //  <Typedef> = "typedef" <TypeSpec> [<Name>] [<ArraySpec>] ";"
   //  The "typedef" keyword has already been parsed.  <Name> is mandatory if
   //  (and only if) <TypeSpec> does not include a <FuncSpec> (function type).
   //
   auto begin = kwdBegin_;
   auto start = lexer_.Curr();

   TypeSpecPtr typeSpec;
   string typeName;
   if(!GetTypeSpec(typeSpec, typeName)) return lexer_.Retreat(start);

   //  If typeSpec was a function type, typeName was set to its name,
   //  if any.  For other typedefs, the name follows typeSpec.
   //
   if(typeSpec->GetFuncSpec() == nullptr)
   {
      if(!lexer_.GetName(typeName)) return lexer_.Retreat(start);
   }

   ArraySpecPtr arraySpec;
   while(GetArraySpec(arraySpec)) typeSpec->AddArray(arraySpec);
   if(!lexer_.NextCharIs(';')) return lexer_.Retreat(start);

   type.reset(new Typedef(typeName, typeSpec));
   SetContext(type.get(), begin);
   return Success(Parser_GetTypedef, begin);
}

//------------------------------------------------------------------------------

fn_name Parser_GetTypeId = "Parser.GetTypeId";

bool Parser::GetTypeId(ExprPtr& expr)
{
   Debug::ft(Parser_GetTypeId);

   auto start = lexer_.Curr();

   //  The typeid operator has already been parsed.
   //
   ExprPtr type;
   if(!GetParExpr(type, false)) return lexer_.Retreat(start);

   auto token = TokenPtr(new Operation(Cxx::TYPE_NAME));
   auto op = static_cast< Operation* >(token.get());
   auto arg = TokenPtr(type.release());
   op->AddArg(arg, false);
   expr->AddItem(token);
   return Success(Parser_GetTypeId, start);
}

//------------------------------------------------------------------------------

fn_name Parser_GetTypeName = "Parser.GetTypeName";

bool Parser::GetTypeName(TypeNamePtr& type)
{
   Debug::ft(Parser_GetTypeName);

   //  <TypeName> = <Name> ["<" <TypeSpec> ["," <TypeSpec>]* ">"]
   //
   auto start = lexer_.Curr();

   string name;
   if(!lexer_.GetName(name)) return lexer_.Retreat(start);
   type.reset(new TypeName(name));
   SetContext(type.get(), start);

   //  Before looking for a template argument after a '<', see if the '<' is
   //  actually part of an operator.
   //
   auto mark = lexer_.Curr();

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
      if(next == string::npos) return lexer_.Retreat(start);
      if(lexer_.At(next) != '>') return lexer_.Reposition(mark);

      do
      {
         TypeSpecPtr arg;
         if(!GetTypeSpec(arg)) return lexer_.Retreat(start);
         type->AddTemplateArg(arg);
      }
      while(lexer_.NextCharIs(','));

      if(!lexer_.NextCharIs('>')) return lexer_.Retreat(start);
   }

   return true;
}

//------------------------------------------------------------------------------

fn_name Parser_GetTypeSpec1 = "Parser.GetTypeSpec";

bool Parser::GetTypeSpec(TypeSpecPtr& spec)
{
   Debug::ft(Parser_GetTypeSpec1);

   //  <TypeSpec> = ["const"] <QualName> ["const"] ["*"]* ["const"]
   //               ["&" | "&&"] ["const"] ["[]"] [<FuncSpec>]
   //
   //  Regular types can use pointer ("*") and reference ("&") tags.
   //  Template arguments can use pointer and array ("[]") tags.
   //  This is not enforced because the code is known to parse.
   //
   auto start = lexer_.Curr();

   QualNamePtr typeName;
   auto readonly = NextKeywordIs(CONST_STR);
   if(!GetQualName(typeName)) return lexer_.Retreat(start);
   if(!CheckType(typeName)) return lexer_.Retreat(start);
   if(NextKeywordIs(CONST_STR))
   {
      if(readonly)
         Log(RedundantConst);
      else
         readonly = true;
   }
   spec.reset(new DataSpec(typeName));
   spec->SetConst(readonly);
   SetContext(spec.get(), start);

   GetPointers(spec.get());
   if(NextKeywordIs(CONST_STR))
   {
      if(spec->PtrCount(false) > 0)
      {
         spec->SetConstPtr(true);
      }
      else
      {
         if(spec->IsConst())
            Log(RedundantConst);
         else
            spec->SetConst(true);
      }
   }

   GetReferences(spec.get());
   if(NextKeywordIs(CONST_STR))
   {
      if(spec->PtrCount(false) > 0)
      {
         if(spec->IsConstPtr())
            Log(RedundantConst);
         else
            spec->SetConstPtr(true);
      }
      else
      {
         if(spec->IsConst())
            Log(RedundantConst);
         else
            spec->SetConst(true);
      }
   }

   //  Check if this is a function type.  If it is, it assumes ownership
   //  of SPEC as its return type.  Create a FuncSpec to wrap the entire
   //  function signature.
   //
   FunctionPtr func;
   if(GetFuncSpec(spec, func)) spec.reset(new FuncSpec(func));
   return Success(Parser_GetTypeSpec1, start);
}

//------------------------------------------------------------------------------

fn_name Parser_GetTypeSpec2 = "Parser.GetTypeSpec";

bool Parser::GetTypeSpec(TypeSpecPtr& spec, string& name)
{
   Debug::ft(Parser_GetTypeSpec2);

   GetTypeSpec(spec);
   if(spec == nullptr) return false;

   auto func = spec->GetFuncSpec();

   if(func != nullptr)
   {
      //  This is a function type.  Set NAME to the function type's
      //  name, if any, stripping the "(*" prefix and ")" suffix.
      //
      auto funcName = func->Name();
      if(funcName == nullptr) return true;
      name = *funcName;
      name.erase(0, 2);
      name.pop_back();
   }

   return true;
}

//------------------------------------------------------------------------------

fn_name Parser_GetUsing = "Parser.GetUsing";

bool Parser::GetUsing(UsingPtr& use)
{
   Debug::ft(Parser_GetUsing);

   //  <Using> = "using" ["namespace"] <QualName> ";"
   //  The "using" keyword has already been parsed.
   //
   auto begin = kwdBegin_;
   auto start = lexer_.Curr();

   QualNamePtr usingName;
   auto space = NextKeywordIs(NAMESPACE_STR);
   if(!GetQualName(usingName)) return lexer_.Retreat(start);
   if(!lexer_.NextCharIs(';')) return lexer_.Retreat(start);
   use.reset(new Using(usingName, space, Original));
   SetContext(use.get(), begin);
   return Success(Parser_GetUsing, begin);
}

//------------------------------------------------------------------------------

fn_name Parser_GetWhile = "Parser.GetWhile";

bool Parser::GetWhile(TokenPtr& statement)
{
   Debug::ft(Parser_GetWhile);

   auto begin = kwdBegin_;
   auto start = lexer_.Curr();

   //  The "while" keyword has already been parsed.
   //
   ExprPtr condition;
   BlockPtr loop;
   if(!GetParExpr(condition, false)) return lexer_.Retreat(start);
   if(!GetBlock(loop)) return lexer_.Retreat(start);

   statement.reset(new While(begin));
   auto w = static_cast< While* >(statement.get());
   w->AddCondition(condition);
   w->AddLoop(loop);
   return Success(Parser_GetWhile, begin);
}

//------------------------------------------------------------------------------

fn_name Parser_HandleDefine = "Parser.HandleDefine";

bool Parser::HandleDefine()
{
   Debug::ft(Parser_HandleDefine);

   //  <Define> = "#define" <Name> [<Expr>]
   //
   auto start = lexer_.Curr();
   auto end = lexer_.FindLineEnd(start);
   string name;

   if(!lexer_.NextStringIs(HASH_DEFINE_STR)) return Report(DirectiveMismatch);
   if(!lexer_.GetName(name)) return Report(SymbolExpected);
   ExprPtr expr;
   GetPreExpr(expr, end);

   //  See if NAME has already appeared as a macro name before creating it.
   //
   auto macro = Singleton< CxxSymbols >::Instance()->FindMacro(name);

   if(macro == nullptr)
   {
      auto def = MacroPtr(new Define(name, expr));
      macro = def.get();
      Singleton< CxxRoot >::Instance()->AddMacro(def);
   }
   else
   {
      macro->SetExpr(expr);
   }

   SetContext(macro, start);
   lexer_.PreprocessSource();
   return true;
}

//------------------------------------------------------------------------------

fn_name Parser_HandleDirective = "Parser.HandleDirective";

bool Parser::HandleDirective(DirectivePtr& dir)
{
   Debug::ft(Parser_HandleDirective);

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

fn_name Parser_HandleElif = "Parser.HandleElif";

bool Parser::HandleElif(DirectivePtr& dir)
{
   Debug::ft(Parser_HandleElif);

   //  <Elif> = "#elif" <Expr>
   //
   auto start = lexer_.Curr();
   auto end = lexer_.FindLineEnd(start);

   if(!lexer_.NextStringIs(HASH_ELIF_STR)) return Report(DirectiveMismatch);
   auto iff = Context::Optional();
   if(iff == nullptr) return Report(ElifUnexpected);
   ExprPtr expr;
   if(!GetPreExpr(expr, end)) return Report(ConditionExpected);

   auto elif = ElifPtr(new Elif);
   elif->AddCondition(expr);
   if(!iff->AddElif(elif.get())) return Report(ElifUnexpected);
   SetContext(elif.get(), start);
   lexer_.FindCode(elif.get(), elif->EnterScope());
   dir = std::move(elif);
   return true;
}

//------------------------------------------------------------------------------

fn_name Parser_HandleElse = "Parser.HandleElse";

bool Parser::HandleElse(DirectivePtr& dir)
{
   Debug::ft(Parser_HandleElse);

   //  <Else> = "#else"
   //
   auto start = lexer_.Curr();

   if(!lexer_.NextStringIs(HASH_ELSE_STR)) return Report(DirectiveMismatch);
   auto ifx = Context::Optional();
   if(ifx == nullptr) return Report(ElseUnexpected);

   auto els = ElsePtr(new Else);
   if(!ifx->AddElse(els.get())) return Report(ElseUnexpected);
   SetContext(els.get(), start);
   lexer_.FindCode(els.get(), els->EnterScope());
   dir = std::move(els);
   return true;
}

//------------------------------------------------------------------------------

fn_name Parser_HandleEndif = "Parser.HandleEndif";

bool Parser::HandleEndif(DirectivePtr& dir)
{
   Debug::ft(Parser_HandleEndif);

   //  <Endif> = "#endif"
   //
   auto start = lexer_.Curr();

   if(!lexer_.NextStringIs(HASH_ENDIF_STR)) return Report(DirectiveMismatch);
   if(!Context::PopOptional()) return Report(EndifUnexpected);

   auto endif = EndifPtr(new Endif);
   SetContext(endif.get(), start);
   dir = std::move(endif);
   return true;
}

//------------------------------------------------------------------------------

fn_name Parser_HandleError = "Parser.HandleError";

bool Parser::HandleError(DirectivePtr& dir)
{
   Debug::ft(Parser_HandleError);

   //  <Error> = "#error" <Text>
   //
   auto start = lexer_.Curr();

   if(!lexer_.NextStringIs(HASH_ERROR_STR)) return Report(DirectiveMismatch);
   auto begin = lexer_.Curr();
   auto end = lexer_.FindLineEnd(begin);
   auto text = lexer_.Extract(begin, end - begin);

   dir = ErrorPtr(new Error(text));
   SetContext(dir.get(), start);
   dir->EnterScope();
   return lexer_.Reposition(end);
}

//------------------------------------------------------------------------------

fn_name Parser_HandleIf = "Parser.HandleIf";

bool Parser::HandleIf(DirectivePtr& dir)
{
   Debug::ft(Parser_HandleIf);

   //  <If> = "#if" <Expr>
   //
   auto start = lexer_.Curr();
   auto end = lexer_.FindLineEnd(start);

   if(!lexer_.NextStringIs(HASH_IF_STR)) return Report(DirectiveMismatch);
   ExprPtr expr;
   if(!GetPreExpr(expr, end)) return Report(ConditionExpected);

   auto iff = IffPtr(new Iff);
   iff->AddCondition(expr);
   SetContext(iff.get(), start);
   lexer_.FindCode(iff.get(), iff->EnterScope());
   dir = std::move(iff);
   return true;
}

//------------------------------------------------------------------------------

fn_name Parser_HandleIfdef = "Parser.HandleIfdef";

bool Parser::HandleIfdef(DirectivePtr& dir)
{
   Debug::ft(Parser_HandleIfdef);

   //  <Ifdef> = "#ifdef" <Name>
   //
   auto start = lexer_.Curr();
   string symbol;

   if(!lexer_.NextStringIs(HASH_IFDEF_STR)) return Report(DirectiveMismatch);
   if(!lexer_.GetName(symbol)) return Report(SymbolExpected);

   auto ifdef = IfdefPtr(new Ifdef(symbol));
   SetContext(ifdef.get(), start);
   lexer_.FindCode(ifdef.get(), ifdef->EnterScope());
   dir = std::move(ifdef);
   return true;
}

//------------------------------------------------------------------------------

fn_name Parser_HandleIfndef = "Parser.HandleIfndef";

bool Parser::HandleIfndef(DirectivePtr& dir)
{
   Debug::ft(Parser_HandleIfndef);

   //  <Ifndef> = "#ifndef" <Name>
   //
   auto start = lexer_.Curr();
   string symbol;

   if(!lexer_.NextStringIs(HASH_IFNDEF_STR)) return Report(DirectiveMismatch);
   if(!lexer_.GetName(symbol)) return Report(SymbolExpected);

   auto ifndef = IfndefPtr(new Ifndef(symbol));
   SetContext(ifndef.get(), start);
   lexer_.FindCode(ifndef.get(), ifndef->EnterScope());
   dir = std::move(ifndef);
   return true;
}

//------------------------------------------------------------------------------

fn_name Parser_HandleInclude = "Parser.HandleInclude";

bool Parser::HandleInclude()
{
   Debug::ft(Parser_HandleInclude);

   //  <Include> = "#include" <FileName>
   //
   //  Note that #includes are handled before parsing, by CodeFile.Scan, because
   //  they allow the compile order to be calculated.  Here, we finally insert
   //  the #include as a statement in the code file.
   //
   auto start = lexer_.Curr();
   auto end = lexer_.FindLineEnd(start);
   string name;
   bool angle;

   if(!lexer_.NextStringIs(HASH_INCLUDE_STR)) return Report(DirectiveMismatch);
   if(!lexer_.GetIncludeFile(start, name, angle)) return Report(FileExpected);
   auto incl = Context::File()->InsertInclude(name);
   if(incl != nullptr) SetContext(incl, start);
   return lexer_.Reposition(end);
}

//------------------------------------------------------------------------------

fn_name Parser_HandleLine = "Parser.HandleLine";

bool Parser::HandleLine(DirectivePtr& dir)
{
   Debug::ft(Parser_HandleLine);

   //  <Line> = "#line" <Text>
   //
   auto start = lexer_.Curr();

   if(!lexer_.NextStringIs(HASH_LINE_STR)) return Report(DirectiveMismatch);
   auto begin = lexer_.Curr();
   auto end = lexer_.FindLineEnd(begin);
   auto text = lexer_.Extract(begin, end - begin);

   dir = LinePtr(new Line(text));
   SetContext(dir.get(), start);
   return lexer_.Reposition(end);
}

//------------------------------------------------------------------------------

fn_name Parser_HandleParentheses = "Parser.HandleParentheses";

bool Parser::HandleParentheses(ExprPtr& expr)
{
   Debug::ft(Parser_HandleParentheses);

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

fn_name Parser_HandlePragma = "Parser.HandlePragma";

bool Parser::HandlePragma(DirectivePtr& dir)
{
   Debug::ft(Parser_HandlePragma);

   //  <Pragma> = "#pragma" <Text>
   //
   auto start = lexer_.Curr();

   if(!lexer_.NextStringIs(HASH_PRAGMA_STR)) return Report(DirectiveMismatch);
   auto begin = lexer_.Curr();
   auto end = lexer_.FindLineEnd(begin);
   auto text = lexer_.Extract(begin, end - begin);

   dir = PragmaPtr(new Pragma(text));
   SetContext(dir.get(), start);
   return lexer_.Reposition(end);
}

//------------------------------------------------------------------------------

fn_name Parser_HandleTilde = "Parser.HandleTilde";

bool Parser::HandleTilde(ExprPtr& expr, size_t start)
{
   Debug::ft(Parser_HandleTilde);

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
            lexer_.Reposition(start);
            if(!GetQualName(name)) return lexer_.Retreat(start);
            item.reset(name.release());
         }
      }
   }

   //  If ITEM is still empty, the '~' should be a ones complement operator.
   //
   if(item == nullptr) item.reset(new Operation(Cxx::ONES_COMPLEMENT));
   if(expr->AddItem(item)) return true;
   return lexer_.Retreat(start);
}

//------------------------------------------------------------------------------

fn_name Parser_HandleUndef = "Parser.HandleUndef";

bool Parser::HandleUndef(DirectivePtr& dir)
{
   Debug::ft(Parser_HandleUndef);

   //  <Undef> = "#undef" <Name>
   //
   auto start = lexer_.Curr();
   string name;

   if(!lexer_.NextStringIs(HASH_UNDEF_STR)) return Report(DirectiveMismatch);
   if(!lexer_.GetName(name)) return Report(SymbolExpected);

   UndefPtr undef = UndefPtr(new Undef(name));
   SetContext(undef.get(), start);
   dir = std::move(undef);
   return true;
}

//------------------------------------------------------------------------------

std::string Parser::Indent()
{
   return spaces(2 * (Context::ParseDepth() - 1));
}

//------------------------------------------------------------------------------

fn_name Parser_Log = "Parser.Log";

void Parser::Log(Warning warning, size_t pos) const
{
   Debug::ft(Parser_Log);

   if(pos == string::npos) pos = lexer_.Prev();
   Context::File()->LogPos(pos, warning);
}

//------------------------------------------------------------------------------

fn_name Parser_NextKeyword = "Parser.NextKeyword";

Cxx::Keyword Parser::NextKeyword(string& str)
{
   Debug::ft(Parser_NextKeyword);

   auto kwd = lexer_.NextKeyword(str);
   if(kwd != Cxx::NIL_KEYWORD) kwdBegin_ = lexer_.Curr();
   return kwd;
}

//------------------------------------------------------------------------------

fn_name Parser_NextKeywordIs = "Parser.NextKeywordIs";

bool Parser::NextKeywordIs(fixed_string str)
{
   Debug::ft(Parser_NextKeywordIs);

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
   Debug::Progress(file.Name(), false, true);

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
   Context::PushScope(gns);
   Enter(*file.GetCode(), true);
   GetFileDecls(gns);
   Context::PopScope();
   if(traced) ThisThread::StopTracing();

   //  If the lexer reached the end of the file, the parse succeeded, so mark
   //  the file as parsed.  If the parse failed, indicate this on the console.
   //
   auto parsed = lexer_.Eof();
   Context::SetFile(nullptr);
   file.SetParsed(parsed);
   Debug::Progress((parsed ? EMPTY_STR : " **FAILED** "), true, true);

   if(!parsed)
   {
      auto expl = file.Name() + " failed near" + CRLF + lexer_.CurrLine();
      Debug::SwErr(Parser_Parse, expl, 0);
   }

   //  On success, delete the parse file if it is not supposed to be retained.
   //
   pTrace_.reset();
   if(parsed && !Context::OptionIsOn(SaveParseTrace)) remove(path.c_str());
   return parsed;
}

//------------------------------------------------------------------------------

fn_name Parser_ParseClassInst = "Parser.ParseClassInst";

bool Parser::ParseClassInst(ClassInst* inst, size_t pos)
{
   Debug::ft(Parser_ParseClassInst);

   auto name = inst->ScopedName(true);
   CoutThread::Spool(EMPTY_STR, true);
   Debug::Progress(Indent() + name, false, true);

   //  Initialize the parser.  If an "object code" file is being produced,
   //  insert the instance name.
   //
   tmpltClassInst_ = true;
   type_ = inst->GetTemplateArgs();
   tmpltName_ = name;
   Enter(*inst->GetCode(), true);
   lexer_.Reposition(pos);
   Context::Trace(CxxTrace::START_TEMPLATE, inst);

   //  Push the template instance as the current scope and start to parse it.
   //  The first thing that could be encountered is a base class declaration.
   do
   {
      BaseDeclPtr base;
      Context::PushScope(inst);
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
   Debug::Progress((parsed ? EMPTY_STR : " **FAILED** "), false, true);

   if(!parsed)
   {
      auto expl = *inst->Name() + " failed to parse";
      Debug::SwErr(Parser_ParseClassInst, expl, 0);
   }

   Context::Trace(CxxTrace::END_TEMPLATE);
   tmpltClassInst_ = false;
   return parsed;
}

//------------------------------------------------------------------------------

fn_name Parser_ParseFuncInst = "Parser.ParseFuncInst";

bool Parser::ParseFuncInst(const string& name,
   const TypeName* type, CxxArea* area, const stringPtr& code)
{
   Debug::ft(Parser_ParseFuncInst);

   CoutThread::Spool(EMPTY_STR, true);
   Debug::Progress(Indent() + name, false, true);

   //  Initialize the parser.  If an "object code" file is being produced,
   //  insert the instance name.
   //
   tmpltFuncInst_ = true;
   type_ = type;
   tmpltName_ = name;
   Enter(*code, true);
   Context::Trace(CxxTrace::START_TEMPLATE, 0, name);

   //  Parse the function definition.
   //
   Context::PushScope(area);
   string str;
   auto kwd = NextKeyword(str);

   FunctionPtr func;
   if(GetFuncDefn(kwd, func))
   {
      area->AddFunc(func);
   }

   Context::PopScope();

   //  The parse succeeded if the lexer reached the end of the code.  If the
   //  parse failed, indicate this on the console.  If an "object code" file
   //  is being produced, indicate that parsing of the template is complete.
   //
   auto parsed = lexer_.Eof();
   Debug::Progress((parsed ? EMPTY_STR : " **FAILED** "), false, true);

   if(!parsed)
   {
      auto expl = name + " failed to parse";
      Debug::SwErr(Parser_ParseFuncInst, expl, 0);
   }

   Context::Trace(CxxTrace::END_TEMPLATE);
   tmpltFuncInst_ = false;
   return parsed;
}

//------------------------------------------------------------------------------

fn_name Parser_ParseInBlock = "Parser.ParseInBlock";

bool Parser::ParseInBlock(Cxx::Keyword kwd, Block* block)
{
   Debug::ft(Parser_ParseInBlock);

   if(lexer_.Eof()) return false;

   DataPtr dataItem;
   DirectivePtr dirItem;
   EnumPtr enumItem;
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
         if(GetUsing(usingItem))
            return block->AddStatement(usingItem.release());
         break;
      case '-':
         Debug::SwErr(Parser_ParseInBlock, kwd, 0);
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
   DataPtr dataItem;
   DirectivePtr dirItem;
   EnumPtr enumItem;
   FriendPtr friendItem;
   FunctionPtr funcItem;
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
            return cls->AddDirective(dirItem);
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
         if(GetUsing(usingItem)) return cls->AddUsing(usingItem);
         break;
      case '-':
         Debug::SwErr(Parser_ParseInClass, kwd, 0);
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

   DataPtr dataItem;
   DirectivePtr dirItem;
   EnumPtr enumItem;
   FunctionPtr funcItem;
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
         if(GetUsing(usingItem)) return Context::AddUsing(usingItem);
         break;
      case '-':
         Debug::SwErr(Parser_ParseInFile, kwd, 0);
         return false;
      }

      targs.pop_back();
   }

   return false;
}

//------------------------------------------------------------------------------

fn_name Parser_ParseQualName = "Parser.ParseQualName";

bool Parser::ParseQualName(const string& code, QualNamePtr& name)
{
   Debug::ft(Parser_ParseQualName);

   Enter(code, false);
   return GetQualName(name);
}

//------------------------------------------------------------------------------

fn_name Parser_ParseTypeSpec = "Parser.ParseTypeSpec";

bool Parser::ParseTypeSpec(const string& code, TypeSpecPtr& spec)
{
   Debug::ft(Parser_ParseTypeSpec);

   Enter(code, false);
   auto parsed = GetTypeSpec(spec);
   spec->SetLocale(Cxx::TypeSpec);
   return parsed;
}

//------------------------------------------------------------------------------

fn_name Parser_Punt = "Parser.Punt";

bool Parser::Punt(ExprPtr& expr, size_t end)
{
   Debug::ft(Parser_Punt);

   auto start = lexer_.Curr();
   string punt = "<@ ";
   punt += lexer_.Extract(start, end - start);
   punt += " @>";

   auto item = TokenPtr(new StrLiteral(punt));
   expr->AddItem(item);
   lexer_.Reposition(end);
   return Success(Parser_Punt, start);
}

//------------------------------------------------------------------------------

fn_name Parser_Report = "Parser.Report";

bool Parser::Report(ErrorCode code)
{
   Debug::ft(Parser_Report);
   Context::SwErr(Parser_Report, "Parser error", code, WarningLog);
   return false;
}

//------------------------------------------------------------------------------

fn_name Parser_ResolveInstanceArgument = "Parser.ResolveInstanceArgument";

CxxNamed* Parser::ResolveInstanceArgument(const QualName* name) const
{
   Debug::ft(Parser_ResolveInstanceArgument);

   if((!tmpltClassInst_ && !tmpltFuncInst_)) return nullptr;

   auto fqName = name->ScopedName(true);
   auto args = type_->Args();

   for(auto a = args->cbegin(); a != args->cend(); ++a)
   {
      auto ref = (*a)->Referent();
      if((ref != nullptr) && (ref->ScopedName(true) == fqName)) return ref;
   }

   return nullptr;
}

//------------------------------------------------------------------------------

fn_name Parser_Retreat = "Parser.Retreat(item)";

bool Parser::Retreat(size_t pos, FunctionPtr& func)
{
   Debug::ft(Parser_Retreat);

   func.reset();
   return lexer_.Retreat(pos);
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
            name->SetReferent(base->uShortTerm());
            return true;
         case 1:
            name->SetReferent(base->uLongTerm());
            return true;
         case 2:
            name->SetReferent(base->uLongLongTerm());
            return true;
         }

         name->SetReferent(base->uIntTerm());
         return true;
      }

      switch(size)
      {
      case -1:
         name->SetReferent(base->ShortTerm());
         return true;
      case 1:
         name->SetReferent(base->LongTerm());
         return true;
      case 2:
         name->SetReferent(base->LongLongTerm());
         return true;
      }

      name->SetReferent(base->IntTerm());
      return true;

   case Cxx::CHAR:
      if(sign > 0)
         name->SetReferent(base->uCharTerm());
      else
         name->SetReferent(base->CharTerm());
      return true;

   case Cxx::DOUBLE:
      if(size == 0)
         name->SetReferent(base->DoubleTerm());
      else
         name->SetReferent(base->LongDoubleTerm());
      return true;

   default:
      Debug::SwErr(Parser_SetCompoundType, *name->Name(), type);
   }

   return false;
}

//------------------------------------------------------------------------------

fn_name Parser_SetContext = "Parser.SetContext";

void Parser::SetContext(CxxNamed* item, size_t pos) const
{
   Debug::ft(Parser_SetContext);

   //  Don't overwrite the scope if the item has already set it.
   //
   auto scope = item->GetScope();

   if(scope == nullptr)
   {
      scope = Context::Scope();
      item->SetScope(scope);
   }

   if(scope != nullptr) item->SetAccess(scope->GetCurrAccess());
   item->SetPos(Context::File(), pos);
   if(tmpltClassInst_ || tmpltFuncInst_) item->SetInternal();
}

//------------------------------------------------------------------------------

fn_name Parser_Success = "Parser.Success";

bool Parser::Success(const string& fn, size_t start) const
{
   Debug::ft(Parser_Success);

   if(!Context::OptionIsOn(TraceParse)) return true;
   if(tmpltClassInst_ || tmpltFuncInst_) return true;

   //  Note that when the parse advances over the first keyword expected by a
   //  function before invoking it, that keyword does not appear at the front
   //  of the parse string.
   //
   auto lead = spaces((SysThreadStack::FuncDepth() - depth_) << 1);

   *pTrace_ << lead << fn << ": ";

   auto prev = lexer_.Prev();
   auto count = (prev > start ? prev - start : 0);
   auto parsed = lexer_.Extract(start, count);
   auto size = parsed.size();

   if(size <= 80)
      *pTrace_ << parsed;
   else
      *pTrace_ << parsed.substr(0, 40) << "..." << parsed.substr(size - 40, 40);

   *pTrace_ << CRLF;
   return true;
}
}
