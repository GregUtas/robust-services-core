//==============================================================================
//
//  Interpreter.cpp
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
#include "Interpreter.h"
#include <cctype>
#include "CodeDirSet.h"
#include "CodeFileSet.h"
#include "Debug.h"
#include "Library.h"
#include "LibraryErrSet.h"
#include "Singleton.h"
#include "Symbol.h"
#include "SysTypes.h"

using std::string;

//------------------------------------------------------------------------------

namespace CodeTools
{
//  Information about a library operator.
//
struct OperatorInfo
{
   //  If OP is a valid operator, updates TYPE and returns true,
   //  else returns false.
   //
   static bool GetType(const string& op, LibTokenType& type);

   //  Returns the attributes for TYPE.  Returns nullptr if TYPE
   //  is not a valid operator.
   //
   static const OperatorInfo* GetAttrs(LibTokenType type);

   //  The operator's symbol.
   //
   const string sym;

   //  How many arguments the operator takes.
   //
   const int args;

   //  The type of set returned by the operator.
   //
   const LibSetType lhs;

   //  The type of set that is valid for the first argument.
   //
   const LibSetType rhs1;

   //  The type of set that is valid for the second argument, if any.
   //
   const LibSetType rhs2;

   //  The array that contains the above attributes for each operator.
   //
   static const OperatorInfo Attrs[Operator_N];
private:
   //  Creates an instance with the specified attributes.
   //
   OperatorInfo(const string& sym, int args, LibSetType lhs,
      LibSetType rhs1, LibSetType rhs2);

   //  Define the copy operator to suppress the compiler warning caused
   //  by our const string member.
   //
   OperatorInfo& operator=(const OperatorInfo& that) = delete;
};

//------------------------------------------------------------------------------

const OperatorInfo OperatorInfo::Attrs[Operator_N] =
{
   //           sym args lhs       rhs1      rhs2
   OperatorInfo(" ",  0, ERR_SET,  ERR_SET,  ERR_SET),   // OpNil
   OperatorInfo("(",  0, ERR_SET,  ERR_SET,  ERR_SET),   // OpLeftPar
   OperatorInfo(")",  0, ERR_SET,  ERR_SET,  ERR_SET),   // OpRightPar
   OperatorInfo("&",  2, ANY_SET,  ANY_SET,  ANY_SET),   // OpIntersection
   OperatorInfo("-",  2, ANY_SET,  ANY_SET,  ANY_SET),   // OpDifference
   OperatorInfo("|",  2, ANY_SET,  ANY_SET,  ANY_SET),   // OpUnion
   OperatorInfo("|",  2, ANY_SET,  ANY_SET,  ANY_SET),   // OpAutoUnion
   OperatorInfo("d",  1, DIR_SET,  FILE_SET, ERR_SET),   // OpDirectories
   OperatorInfo("f",  1, FILE_SET, DIR_SET,  ERR_SET),   // OpFiles
   OperatorInfo("fn", 2, FILE_SET, FILE_SET, FILE_SET),  // OpFileName
   OperatorInfo("ft", 2, FILE_SET, FILE_SET, FILE_SET),  // OpFileType
   OperatorInfo("ms", 2, FILE_SET, FILE_SET, FILE_SET),  // OpMatchString
   OperatorInfo("in", 2, FILE_SET, FILE_SET, DIR_SET),   // OpFoundIn
   OperatorInfo("im", 1, FILE_SET, FILE_SET, ERR_SET),   // OpImplements
   OperatorInfo("ub", 1, FILE_SET, FILE_SET, ERR_SET),   // OpUsedBy
   OperatorInfo("us", 1, FILE_SET, FILE_SET, ERR_SET),   // OpUsers
   OperatorInfo("ab", 1, FILE_SET, FILE_SET, ERR_SET),   // OpAffectedBy
   OperatorInfo("as", 1, FILE_SET, FILE_SET, ERR_SET),   // OpAffecters
   OperatorInfo("ca", 1, FILE_SET, FILE_SET, ERR_SET),   // OpCommonAffecters
   OperatorInfo("nb", 1, FILE_SET, FILE_SET, ERR_SET),   // OpNeededBy
   OperatorInfo("ns", 1, FILE_SET, FILE_SET, ERR_SET)    // OpNeeders
};

//------------------------------------------------------------------------------

fn_name OperatorInfo_ctor = "OperatorInfo.ctor";

OperatorInfo::OperatorInfo(const string& s, int args, LibSetType lhs,
   LibSetType rhs1, LibSetType rhs2) :
   sym(s),
   args(args),
   lhs(lhs),
   rhs1(rhs1),
   rhs2(rhs2)
{
   Debug::ft(OperatorInfo_ctor);
}

//------------------------------------------------------------------------------

fn_name OperatorInfo_GetAttrs = "OperatorInfo.GetAttrs";

const OperatorInfo* OperatorInfo::GetAttrs(LibTokenType type)
{
   Debug::ft(OperatorInfo_GetAttrs);

   if((type > 0) && (type < Operator_N)) return &Attrs[type];
   return nullptr;
}

//------------------------------------------------------------------------------

fn_name OperatorInfo_GetType = "OperatorInfo.GetType";

bool OperatorInfo::GetType(const string& op, LibTokenType& type)
{
   Debug::ft(OperatorInfo_GetType);

   for(size_t i = 1; i < Operator_N; ++i)
   {
      if(Attrs[i].sym == op)
      {
         type = LibTokenType(i);
         return true;
      }
   }

   return false;
}

//==============================================================================
//
//  Library opcodes.
//
class LibraryOpcode
{
public:
   //  Constructs an opcode for OP, taking arguments from ARGS.
   //
   LibraryOpcode(LibTokenType op, std::stack< LibrarySet* >& args);

   //  Releases LibrarySets that were arguments, which causes them
   //  to be deleted if they were temporary.
   //
   ~LibraryOpcode();

   //  Executes the opcode.
   //
   void Execute();

   //  Returns any error that arose when executing the opcode.
   //
   LibExprErr GetError() const { return err_; }
private:
   //  Verifies that the type of argument ENTERED is the type that
   //  is ACCEPTED by this opcode.
   //
   bool CheckArgType(LibSetType accepted, LibSetType entered);

   //  The operation to perform.
   //
   const LibTokenType op_;

   //  Where to put the result.
   //
   LibrarySet* lhs_;

   //  The first argument.
   //
   LibrarySet* rhs1_;

   //  The second argument, if any.
   //
   LibrarySet* rhs2_;

   //  Any error that occurred when executing the opcode.
   //
   LibExprErr err_;
};

//------------------------------------------------------------------------------

fn_name LibraryOpcode_ctor = "LibraryOpcode.ctor";

LibraryOpcode::LibraryOpcode(LibTokenType op, std::stack< LibrarySet* >& args) :
   op_(op),
   lhs_(nullptr),
   rhs1_(nullptr),
   rhs2_(nullptr),
   err_(ExpressionOk)
{
   Debug::ft(LibraryOpcode_ctor);

   //  Access the operator's attributes.
   //
   auto attrs = OperatorInfo::GetAttrs(op);

   if(attrs == nullptr)
   {
      err_ = InterpreterError;
      return;
   }

   //  Pop the operand(s).
   //
   switch(attrs->args)
   {
   case 2:
      if(args.size() < 2)
      {
         err_ = RightOperandMissing;
         return;
      }
      rhs2_ = args.top();
      args.pop();

   case 1:
      if(args.size() < 1)
      {
         err_ = LeftOperandMissing;
         return;
      }
      rhs1_ = args.top();
      args.pop();
      break;

   default:
      err_ = InterpreterError;
      return;
   }

   //  Verify that the operand(s) are of the correct type.
   //
   auto type1 = rhs1_->GetType();
   if(!CheckArgType(attrs->rhs1, type1)) return;

   if(rhs2_ != nullptr)
   {
      auto type2 = rhs2_->GetType();
      if(!CheckArgType(attrs->rhs2, type2)) return;

      if((attrs->rhs2 == ANY_SET) && (type1 != type2))
      {
         err_ = IncompatibleArguments;
         return;
      }
   }

   //  Create the set that will hold the result.
   //
   switch(attrs->lhs)
   {
   case DIR_SET:
      lhs_ = new CodeDirSet(LibrarySet::TemporaryName(), nullptr);
      break;
   case FILE_SET:
      lhs_ = new CodeFileSet(LibrarySet::TemporaryName(), nullptr);
      break;
   case ANY_SET:
      if(type1 == DIR_SET)
         lhs_ = new CodeDirSet(LibrarySet::TemporaryName(), nullptr);
      else
         lhs_ = new CodeFileSet(LibrarySet::TemporaryName(), nullptr);
      break;
   default:
      err_ = InterpreterError;
      return;
   }

   args.push(lhs_);
}

//------------------------------------------------------------------------------

fn_name LibraryOpcode_dtor = "LibraryOpcode.dtor";

LibraryOpcode::~LibraryOpcode()
{
   Debug::ft(LibraryOpcode_dtor);

   //  lhs_ will become someone else's rhs_, so don't delete it.
   //  And until it does, the operand stack owns it.
   //
   if(rhs1_ != nullptr) rhs1_->Release();
   rhs1_ = nullptr;

   if(rhs2_ != nullptr) rhs2_->Release();
   rhs2_ = nullptr;
}

//------------------------------------------------------------------------------

fn_name LibraryOpcode_CheckArgType = "LibraryOpcode.CheckArgType";

bool LibraryOpcode::CheckArgType(LibSetType accepted, LibSetType entered)
{
   Debug::ft(LibraryOpcode_CheckArgType);

   switch(accepted)
   {
   case FILE_SET:
      if(entered == FILE_SET) return true;
      err_ = FileSetExpected;
      return false;

   case DIR_SET:
      if(entered == DIR_SET) return true;
      err_ = DirSetExpected;
      return false;

   case ANY_SET:
      return true;
   }

   err_ = InterpreterError;
   return false;
}

//------------------------------------------------------------------------------

fn_name LibraryOpcode_Execute = "LibraryOpcode.Execute";

void LibraryOpcode::Execute()
{
   Debug::ft(LibraryOpcode_Execute);

   LibrarySet* result = nullptr;

   switch(op_)
   {
   case OpIntersection:
      result = lhs_->Assign(rhs1_->Intersection(rhs2_));
      break;
   case OpDifference:
      result = lhs_->Assign(rhs1_->Difference(rhs2_));
      break;
   case OpUnion:
   case OpAutoUnion:
      result = lhs_->Assign(rhs1_->Union(rhs2_));
      break;
   case OpDirectories:
      result = lhs_->Assign(rhs1_->Directories());
      break;
   case OpFiles:
      result = lhs_->Assign(rhs1_->Files());
      break;
   case OpFileName:
      result = lhs_->Assign(rhs1_->FileName(rhs2_));
      break;
   case OpFileType:
      result = lhs_->Assign(rhs1_->FileType(rhs2_));
      break;
   case OpFoundIn:
      result = lhs_->Assign(rhs1_->FoundIn(rhs2_));
      break;
   case OpMatchString:
      result = lhs_->Assign(rhs1_->MatchString(rhs2_));
      break;
   case OpImplements:
      result = lhs_->Assign(rhs1_->Implements());
      break;
   case OpUsedBy:
      result = lhs_->Assign(rhs1_->UsedBy(false));
      break;
   case OpUsers:
      result = lhs_->Assign(rhs1_->Users(false));
      break;
   case OpAffectedBy:
      result = lhs_->Assign(rhs1_->AffectedBy());
      break;
   case OpAffecters:
      result = lhs_->Assign(rhs1_->Affecters());
      break;
   case OpCommonAffecters:
      result = lhs_->Assign(rhs1_->CommonAffecters());
      break;
   case OpNeededBy:
      result = lhs_->Assign(rhs1_->NeededBy());
      break;
   case OpNeeders:
      result = lhs_->Assign(rhs1_->Needers());
      break;
   }

   if(result == nullptr)
   {
      Debug::SwErr(LibraryOpcode_Execute, op_, 0);
      err_ = InterpreterError;
   }
}

//==============================================================================

const string BlankChars(" ");
const string PathChars(":/\\");
const string LibOpChars("()&-|");
const string LibIdChars(Symbol::ValidNameChars() + '$');
const string LegalChars(BlankChars + PathChars + LibOpChars + LibIdChars);

//------------------------------------------------------------------------------

fn_name Interpreter_ctor = "Interpreter.ctor";

Interpreter::Interpreter(const string& expr, size_t offset) :
   expr_(expr),
   offset_(offset),
   prev_(0),
   curr_(0),
   type_(OpNil)
{
   Debug::ft(Interpreter_ctor);
}

//------------------------------------------------------------------------------

fn_name Interpreter_dtor = "Interpreter.dtor";

Interpreter::~Interpreter()
{
   Debug::ft(Interpreter_dtor);

   //  Invoke Release on operands (LibrarySets).  This will cause
   //  a temporary to delete itself.
   //
   while(!operands_.empty())
   {
      auto operand = operands_.top();
      operands_.pop();
      operand->Release();
   }

   //  Delete any opcodes.  The opcode destructor invokes Release
   //  on its RHS LibrarySet(s).
   //
   while(!opcodes_.empty())
   {
      auto opcode = opcodes_.front();
      opcodes_.pop();
      delete opcode;
   }
}

//------------------------------------------------------------------------------

fn_name Interpreter_ApplyOperator = "Interpreter.ApplyOperator";

LibExprErr Interpreter::ApplyOperator(bool operand)
{
   Debug::ft(Interpreter_ApplyOperator);

   auto err = ExpressionOk;

   while(!operators_.empty() && (err == ExpressionOk))
   {
      auto op = operators_.top();

      switch(op)
      {
      case OpLeftPar:
         //
         //  There is nothing to do yet.  If a new operand was pushed onto
         //  the stack, push a set union operator for it before returning.
         //
         if(operand) operators_.push(OpAutoUnion);
         return ExpressionOk;

      case OpRightPar:
      case OpIdentifier:
         //
         //  These shouldn't be on the operator stack.
         //
         Debug::SwErr(Interpreter_ApplyOperator, op, 1);
         return InterpreterError;

      default:
         //
         //  Pop this operator, create an opcode for it, and add it to
         //  the opcode queue.  If an error didn't occur, the operator
         //  pushed an operand.
         //
         operators_.pop();
         auto opcode = new LibraryOpcode(op, operands_);
         opcodes_.push(opcode);
         operand = true;
         err = opcode->GetError();
      }
   }

   if(err != ExpressionOk) return err;

   //  The operator stack is empty, so there should be one pending operand.
   //  Push the union operator in case the next token is also an operand.
   //
   if(operands_.size() == 1)
   {
      operators_.push(OpAutoUnion);
      return ExpressionOk;
   }

   Debug::SwErr(Interpreter_ApplyOperator, operands_.size(), 0);
   return InterpreterError;
}

//------------------------------------------------------------------------------

fn_name Interpreter_CheckExpr = "Interpreter.CheckExpr";

LibExprErr Interpreter::CheckExpr()
{
   Debug::ft(Interpreter_CheckExpr);

   //  Check for an empty expression.
   //
   auto err = SkipBlanks();
   if(err != ExpressionOk) return err;

   //  Look for illegal characters.
   //
   auto next = expr_.find_first_not_of(LegalChars, curr_);

   if(next != string::npos)
   {
      curr_ = next;
      return IllegalCharacter;
   }

   //  Look for unmatched parentheses.
   //
   int pending = 0;

   for(auto i = curr_; i < expr_.size(); ++i)
   {
      switch(expr_[i])
      {
      case '(':
         ++pending;
         break;

      case ')':
         if(--pending < 0)
         {
            curr_ = i;
            return UnmatchedRightPar;
         }
      }
   }

   if(pending > 0)
   {
      curr_ = expr_.size();
      return UnmatchedLeftPar;
   }

   return ExpressionOk;
}

//------------------------------------------------------------------------------

fn_name Interpreter_Error = "Interpreter.Error";

LibrarySet* Interpreter::Error(LibExprErr err) const
{
   Debug::ft(Interpreter_Error);

   int loc = 0;

   switch(err)
   {
   case EndOfExpression:
      //
      //  If this was the error, it really meant...
      //
      err = EmptyExpression;
   case EmptyExpression:
      loc = 1;
      break;

   case IllegalCharacter:     // curr_ = location of character
   case UnexpectedCharacter:  // curr_ = location of character
   case UnmatchedLeftPar:     // curr_ = expr.size()
   case UnmatchedRightPar:    // curr_ = location of character
      loc = curr_;
      break;

   case NoSuchVariable:
   case LeftOperandMissing:
   case RightOperandMissing:
   case IncompatibleArguments:
      //
      //  The token itself was valid, so curr_ has advanced beyond it.
      //  Go back to where the token started.
      //
      loc = prev_;
      break;

   case DirSetExpected:
   case FileSetExpected:
      //
      //c To properly highlight where the error occurred, the operator
      //  that detected this problem should set curr_.  This would involve
      //  pushing prev_ onto the stack for each operand and operator.  Go
      //  back to the start of the *previous* token, which should be the
      //  operator that flagged this error.  First skip the blank(s) that
      //  preceded prev_, then skip characters to arrive at a blank, then
      //  step forward to the first character.
      //
      if(prev_ > 0)
      {
         loc = prev_ - 1;
         while((loc >= 0) && (expr_[loc] == SPACE)) --loc;
         while((loc >= 0) && (expr_[loc] != SPACE)) --loc;
         ++loc;
      }
   }

   if(loc > expr_.size()) loc = expr_.size();

   return new LibraryErrSet(LibrarySet::TemporaryName(), err, offset_ + loc);
}

//------------------------------------------------------------------------------

fn_name Interpreter_Evaluate = "Interpreter.Evaluate";

LibrarySet* Interpreter::Evaluate()
{
   Debug::ft(Interpreter_Evaluate);

   //  Run some basic checks to see if EXPR contains obvious errors.
   //
   auto err = CheckExpr();
   if(err != ExpressionOk) return Error(err);

   while(err == ExpressionOk)
   {
      err = GetToken();
      if(err != ExpressionOk) break;
      err = HandleToken();
   }

   //  A successful parse concludes with EndOfExpression and no pending
   //  operators except for an automatic set union.  There should also
   //  be one pending operand, which was either alone or pushed as the
   //  future result of the last opcode.
   //
   if(err != EndOfExpression) return Error(err);

   if(!operators_.empty())
   {
      if(operators_.top() == OpAutoUnion) operators_.pop();
      if(!operators_.empty()) return Error(RightOperandMissing);
   }

   switch(operands_.size())
   {
   case 0:
      //  The most likely cause of this is an expression containing
      //  nothing but parentheses.
      //
      return Error(EmptyExpression);
   case 1:
      break;
   default:
      Debug::SwErr(Interpreter_Evaluate, operands_.size(), 1);
      return Error(InterpreterError);
   }

   //  Execute the opcodes.  The result will end up in the operand that
   //  is currently alone on the stack.  The opcode destructor invokes
   //  Release on its RHS operand(s).
   //
   while(!opcodes_.empty())
   {
      auto opcode = opcodes_.front();
      opcodes_.pop();
      opcode->Execute();
      delete opcode;
   }

   //  Return the result.
   //
   auto result = operands_.top();
   operands_.pop();
   return result;
}

//------------------------------------------------------------------------------

fn_name Interpreter_GetToken = "Interpreter.GetToken";

LibExprErr Interpreter::GetToken()
{
   Debug::ft(Interpreter_GetToken);

   //  Skip over blanks and see if this gets us to the end of EXPR.
   //
   prev_ = curr_;
   auto err = SkipBlanks();
   if(err != ExpressionOk) return err;
   prev_ = curr_;

   //  If the next character is an operator symbol, report it immediately.
   //
   token_ = expr_[curr_];

   if(LibOpChars.find_first_of(token_) != string::npos)
   {
      if(OperatorInfo::GetType(token_, type_))
      {
         ++curr_;
         return ExpressionOk;
      }

      Debug::SwErr(Interpreter_GetToken, token_, 0);
      return InterpreterError;
   }

   //  Now we're looking for an identifier or alphabetic operator.  In either
   //  case, its first character needs to be valid for an identifier.
   //
   if(Symbol::InvalidInitialChars().find(expr_[curr_]) != string::npos)
   {
      token_ = expr_[curr_];
      return UnexpectedCharacter;
   }

   //  Find the next character that isn't legal for an identifier, extract the
   //  identifier, and see if it's actually an operator.  If it isn't, it must
   //  be an identifier.
   //
   auto next = expr_.find_first_not_of(LibIdChars + PathChars, curr_);
   token_ = expr_.substr(curr_, next - curr_);
   curr_ = next;

   if(OperatorInfo::GetType(token_, type_)) return ExpressionOk;

   type_ = OpIdentifier;
   return ExpressionOk;
}

//------------------------------------------------------------------------------

fn_name Interpreter_HandleToken = "Interpreter.HandleToken";

LibExprErr Interpreter::HandleToken()
{
   Debug::ft(Interpreter_HandleToken);

   auto operand = false;
   LibrarySet* set;

   switch(type_)
   {
   case OpIdentifier:
      //
      //  Find this operand or create it.  Push it onto the stack and apply
      //  any pending operator.
      //
      set = Singleton< Library >::Instance()->EnsureVar(token_);
      operand = true;

      if(set == nullptr)
      {
         //  This hack handles the string after the fn, ft, and ms operators.
         //  It creates a temporary variable whose name includes the string.
         //
         if(!operators_.empty())
         {
            if((operators_.top() == OpFileName) ||
               (operators_.top() == OpFileType) ||
               (operators_.top() == OpMatchString))
            {
               string suffix = LibrarySet::TemporaryChar + token_;
               set = new CodeFileSet(suffix, nullptr);
               operand = false;
            }
         }
      }

      if(set == nullptr) return NoSuchVariable;
      operands_.push(set);
      return ApplyOperator(operand);

   case OpRightPar:
      //
      //  There should at least be a '(' somewhere on the operator stack.
      //  It is an internal error if this is not so, because CheckExpr
      //  should have caught it.  However, CheckExpr does not screen out
      //  errors like "(f <dir> ft)", in which the operator on top of the
      //  stack is not the matching '(', but some other pending operator.
      //
      if(!operators_.empty())
      {
         auto op = operators_.top();

         //  There will usually be a pending set union operator that was
         //  pushed for the last operand preceding the right parenthesis.
         //
         if(op == OpAutoUnion)
         {
            operators_.pop();
            if(operators_.empty()) break;
            op = operators_.top();
            operand = true;
         }

         if(op == OpLeftPar)
         {
            operators_.pop();
            return ApplyOperator(operand);
         }

         return RightOperandMissing;
      }
      break;

   default:
      auto attrs = OperatorInfo::GetAttrs(type_);
      if(attrs == nullptr) break;

      switch(attrs->args)
      {
      case 0:
      case 1:
         //
         //  This is a prefix unary operator or left parenthesis.  Push
         //  it onto the stack and continue with the next token.
         //
         operators_.push(type_);
         return ExpressionOk;
      case 2:
         //
         //  This is an infix binary operator.  Push it onto the stack
         //  if a left operand is available.  It replaces an automatic
         //  set union operator.
         //
         if(!operators_.empty())
         {
            auto op = operators_.top();
            if(op == OpAutoUnion) operators_.pop();
         }
         else
         {
            --curr_;
            return LeftOperandMissing;
         }

         operators_.push(type_);
         return ExpressionOk;
      }
   }

   Debug::SwErr(Interpreter_HandleToken, type_, 0);
   return InterpreterError;
}

//------------------------------------------------------------------------------

fn_name Interpreter_IsOperator = "Interpreter.IsOperator";

bool Interpreter::IsOperator(const string& s)
{
   Debug::ft(Interpreter_IsOperator);

   for(auto i = 0; i < Operator_N; ++i)
   {
      if(OperatorInfo::Attrs[i].sym == s) return true;
   }

   return false;
}

//------------------------------------------------------------------------------

fn_name Interpreter_SkipBlanks = "Interpreter.SkipBlanks";

LibExprErr Interpreter::SkipBlanks()
{
   Debug::ft(Interpreter_SkipBlanks);

   //  Skip over blanks and see if this gets us to the end of EXPR.
   //
   auto size = expr_.size();

   while((curr_ <= size) && isspace(expr_[curr_])) ++curr_;

   if(curr_ >= size) return EndOfExpression;
   return ExpressionOk;
}
}
