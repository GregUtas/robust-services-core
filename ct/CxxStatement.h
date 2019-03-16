//==============================================================================
//
//  CxxStatement.h
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
#ifndef CXXSTATEMENT_H_INCLUDED
#define CXXSTATEMENT_H_INCLUDED

#include "CxxToken.h"
#include <cstddef>
#include <iosfwd>
#include <string>
#include <utility>
#include "Cxx.h"
#include "CxxFwd.h"
#include "CxxScope.h"
#include "CxxScoped.h"

//------------------------------------------------------------------------------

namespace CodeTools
{
//  Base class for statements.
//
class CxxStatement : public CxxToken
{
public:
   virtual ~CxxStatement() = default;
   void EnterBlock() override;
protected:
   explicit CxxStatement(size_t pos);
private:
   const size_t pos_;
};

//------------------------------------------------------------------------------
//
//  Base class for statements that check a condition.
//
class Condition : public CxxStatement
{
public:
   virtual ~Condition() = default;
   void AddCondition(ExprPtr& c) { condition_ = std::move(c); }
   void Print
      (std::ostream& stream, const NodeBase::Flags& options) const override;
   void EnterBlock() override;
   void GetUsages(const CodeFile& file, CxxUsageSets& symbols) const override;
   void Shrink() override { ShrinkExpression(condition_); }
protected:
   explicit Condition(size_t pos);
   bool Show(std::ostream& stream) const;
private:
   ExprPtr condition_;
};

//------------------------------------------------------------------------------
//
//  A break statement.
//
class Break : public CxxStatement
{
public:
   explicit Break(size_t pos);
   ~Break() { CxxStats::Decr(CxxStats::BREAK); }
   void Print
      (std::ostream& stream, const NodeBase::Flags& options) const override;
   void EnterBlock() override { }
};

//------------------------------------------------------------------------------
//
//  A case label.
//
class Case : public CxxStatement
{
public:
   Case(ExprPtr& expression, size_t pos);
   ~Case() { CxxStats::Decr(CxxStats::CASE); }
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;
   void EnterBlock() override;
   void GetUsages(const CodeFile& file, CxxUsageSets& symbols) const override;
   bool InLine() const override { return false; }
   void Shrink() override { ShrinkExpression(expr_); }
private:
   const ExprPtr expr_;
};

//------------------------------------------------------------------------------
//
//  A catch statement.
//
class Catch : public CxxStatement
{
public:
   explicit Catch(size_t pos);
   ~Catch() { CxxStats::Decr(CxxStats::CATCH); }
   void AddArg(ArgumentPtr& a) { arg_ = std::move(a); }
   void AddHandler(BlockPtr& b) { handler_ = std::move(b); }
   void Check() const override;
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;
   void EnterBlock() override;
   void ExitBlock() override;
   void GetUsages(const CodeFile& file, CxxUsageSets& symbols) const override;
   bool InLine() const override { return false; }
   void Shrink() override;
private:
   ArgumentPtr arg_;
   BlockPtr handler_;
};

//------------------------------------------------------------------------------
//
//  A continue statement.
//
class Continue : public CxxStatement
{
public:
   explicit Continue(size_t pos);
   ~Continue() { CxxStats::Decr(CxxStats::CONTINUE); }
   void Print
      (std::ostream& stream, const NodeBase::Flags& options) const override;
   void EnterBlock() override { }
};

//------------------------------------------------------------------------------
//
//  A do statement.
//
class Do : public Condition
{
public:
   explicit Do(size_t pos);
   ~Do() { CxxStats::Decr(CxxStats::DO); }
   void AddLoop(BlockPtr& b) { loop_ = std::move(b); }
   void Check() const override;
   void Print
      (std::ostream& stream, const NodeBase::Flags& options) const override;
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;
   void EnterBlock() override;
   void GetUsages(const CodeFile& file, CxxUsageSets& symbols) const override;
   bool InLine() const override;
   void Shrink() override;
private:
   BlockPtr loop_;
};

//------------------------------------------------------------------------------
//
//  An expression statement (an assignment or a function call).
//
class Expr : public CxxStatement
{
public:
   Expr(ExprPtr& expression, size_t pos);
   ~Expr() { CxxStats::Decr(CxxStats::EXPR); }
   void Print
      (std::ostream& stream, const NodeBase::Flags& options) const override;
   void EnterBlock() override;
   void GetUsages(const CodeFile& file, CxxUsageSets& symbols) const override;
   void Shrink() override { ShrinkExpression(expr_); }
private:
   const ExprPtr expr_;
};

//------------------------------------------------------------------------------
//
//  A for statement.
//
class For : public Condition
{
public:
   explicit For(size_t pos);
   ~For() { CxxStats::Decr(CxxStats::FOR); }
   void AddInitial(TokenPtr& i) { initial_ = std::move(i); }
   void AddSubsequent(ExprPtr& s) { subsequent_ = std::move(s); }
   void AddLoop(BlockPtr& b) { loop_ = std::move(b); }
   void Check() const override;
   void Print
      (std::ostream& stream, const NodeBase::Flags& options) const override;
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;
   void EnterBlock() override;
   void ExitBlock() override;
   void GetUsages(const CodeFile& file, CxxUsageSets& symbols) const override;
   bool InLine() const override;
   void Shrink() override;
private:
   TokenPtr initial_;
   ExprPtr subsequent_;
   BlockPtr loop_;
};

//------------------------------------------------------------------------------
//
//  An if statement.
//
class If : public Condition
{
public:
   explicit If(size_t pos);
   ~If() { CxxStats::Decr(CxxStats::IF); }
   void AddThen(BlockPtr& b) { then_ = std::move(b); }
   void AddElse(BlockPtr& b) { else_ = std::move(b); }
   void SetElseIf() { elseif_ = true; }
   void Check() const override;
   void Print
      (std::ostream& stream, const NodeBase::Flags& options) const override;
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;
   void EnterBlock() override;
   void GetUsages(const CodeFile& file, CxxUsageSets& symbols) const override;
   bool InLine() const override;
   void Shrink() override;
   Cxx::ItemType Type() const override { return Cxx::If; }
private:
   BlockPtr then_;
   BlockPtr else_;
   bool elseif_;
};

//------------------------------------------------------------------------------
//
//  A label.
//
class Label : public CxxStatement
{
public:
   Label(std::string& name, size_t pos);
   ~Label() { CxxStats::Decr(CxxStats::LABEL); }
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;
   void EnterBlock() override;
   void ExitBlock() override;
   bool InLine() const override { return false; }
   void Shrink() override { name_.shrink_to_fit(); }
private:
   std::string name_;
};

//------------------------------------------------------------------------------
//
//  An empty statement.
//
class NoOp : public CxxStatement
{
public:
   explicit NoOp(size_t pos);
   ~NoOp() { CxxStats::Decr(CxxStats::NOOP); }
   void Print
      (std::ostream& stream, const NodeBase::Flags& options) const override;
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;
   void EnterBlock() override { }
   bool InLine() const override { return true; }
   Cxx::ItemType Type() const override { return Cxx::NoOp; }
};

//------------------------------------------------------------------------------
//
//  A return statement.
//
class Return : public CxxStatement
{
public:
   explicit Return(size_t pos);
   ~Return() { CxxStats::Decr(CxxStats::RETURN); }
   void AddExpr(ExprPtr& e) { expr_ = std::move(e); }
   void Print
      (std::ostream& stream, const NodeBase::Flags& options) const override;
   void EnterBlock() override;
   void GetUsages(const CodeFile& file, CxxUsageSets& symbols) const override;
   void Shrink() override { ShrinkExpression(expr_); }
private:
   ExprPtr expr_;
};

//------------------------------------------------------------------------------
//
//  A switch statement.
//
class Switch : public CxxStatement
{
public:
   explicit Switch(size_t pos);
   ~Switch() { CxxStats::Decr(CxxStats::SWITCH); }
   void AddExpr(ExprPtr& e) { expr_ = std::move(e); }
   void AddCases(BlockPtr& b) { cases_ = std::move(b); }
   void Check() const override;
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;
   void EnterBlock() override;
   void GetUsages(const CodeFile& file, CxxUsageSets& symbols) const override;
   bool InLine() const override { return false; }
   void Shrink() override;
private:
   ExprPtr expr_;
   BlockPtr cases_;
};

//------------------------------------------------------------------------------
//
//  A try statement.
//
class Try : public CxxStatement
{
public:
   explicit Try(size_t pos);
   ~Try() { CxxStats::Decr(CxxStats::TRY); }
   void AddTry(BlockPtr& b) { try_ = std::move(b); }
   void AddCatch(TokenPtr& t) { catches_.push_back(std::move(t)); }
   void Check() const override;
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;
   void EnterBlock() override;
   void ExitBlock() override;
   void GetUsages(const CodeFile& file, CxxUsageSets& symbols) const override;
   bool InLine() const override { return false; }
   void Shrink() override;
private:
   BlockPtr try_;
   TokenPtrVector catches_;
};

//------------------------------------------------------------------------------
//
//  A while statement.
//
class While : public Condition
{
public:
   explicit While(size_t pos);
   ~While() { CxxStats::Decr(CxxStats::WHILE); }
   void AddLoop(BlockPtr& b) { loop_ = std::move(b); }
   void Check() const override;
   void Print
      (std::ostream& stream, const NodeBase::Flags& options) const override;
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;
   void EnterBlock() override;
   void GetUsages(const CodeFile& file, CxxUsageSets& symbols) const override;
   bool InLine() const override;
   void Shrink() override;
private:
   BlockPtr loop_;
};
}
#endif
