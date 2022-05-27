//==============================================================================
//
//  CxxStatement.h
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
   CxxStatement(const CxxStatement& that) = delete;
   CxxStatement& operator=(const CxxStatement& that) = delete;
   void Delete() override;
   void EnterBlock() override;
   CxxScope* GetScope() const override { return scope_; }
   void SetScope(CxxScope* scope) override { scope_ = scope; }
protected:
   explicit CxxStatement(size_t pos);

   //  Implements GetSpan for a statement sequence that starts at BEGIN.  If
   //  it is unbraced, LEFT is set to string::npos and END to the position of
   //  its semicolon.  If it contains multiple statements, LEFT is set to the
   //  position of its left brace and END to the position of its right brace.
   //
   bool GetSeqSpan(size_t begin, size_t& left, size_t& end) const;

   //  Implements GetSpan for a statement that consists of a parenthesized
   //  expression followed by a statement sequence.
   //
   bool GetParSpan(size_t& begin, size_t& left, size_t& end) const;

   //  Implements GetSpan for a label that ends with a colon.
   //
   bool GetColonSpan(size_t& begin, size_t& end) const;
private:
   //  Overridden to support a single statement that ends at a semicolon.
   //
   bool GetSpan(size_t& begin, size_t& left, size_t& end) const override;

   //  The Block in which the statement appeared.
   //
   CxxScope* scope_;
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
   void Check() const override;
   void EnterBlock() override;
   void GetUsages(const CodeFile& file, CxxUsageSets& symbols) override;
   CxxToken* PosToItem(size_t pos) const override;
   void Print
      (std::ostream& stream, const NodeBase::Flags& options) const override;
   void UpdatePos(EditorAction action,
      size_t begin, size_t count, size_t from) const override;
   void UpdateXref(bool insert) override;
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
   ~Break() = default;
   void EnterBlock() override { }
   void Print
      (std::ostream& stream, const NodeBase::Flags& options) const override;
};

//------------------------------------------------------------------------------
//
//  A case label.
//
class Case : public CxxStatement
{
public:
   Case(ExprPtr& expression, size_t pos);
   ~Case() = default;
   void Check() const override;
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;
   void EnterBlock() override;
   void GetUsages(const CodeFile& file, CxxUsageSets& symbols) override;
   bool InLine() const override { return false; }
   CxxToken* PosToItem(size_t pos) const override;
   void UpdatePos(EditorAction action,
      size_t begin, size_t count, size_t from) const override;
   void UpdateXref(bool insert) override;
private:
   bool GetSpan(size_t& begin, size_t& left, size_t& end) const override;

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
   ~Catch() = default;
   void AddArg(ArgumentPtr& a) { arg_ = std::move(a); }
   void AddHandler(BlockPtr& b) { handler_ = std::move(b); }
   void Check() const override;
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;
   void EnterBlock() override;
   void ExitBlock() const override;
   CxxScoped* FindNthItem(const std::string& name, size_t& n) const override;
   void GetUsages(const CodeFile& file, CxxUsageSets& symbols) override;
   bool InLine() const override { return false; }
   bool LocateItem(const CxxToken* item, size_t& n) const override;
   CxxToken* PosToItem(size_t pos) const override;
   void UpdatePos(EditorAction action,
      size_t begin, size_t count, size_t from) const override;
   void UpdateXref(bool insert) override;
private:
   bool GetSpan(size_t& begin, size_t& left, size_t& end) const override;

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
   ~Continue() = default;
   void EnterBlock() override { }
   void Print
      (std::ostream& stream, const NodeBase::Flags& options) const override;
};

//------------------------------------------------------------------------------
//
//  A do statement.
//
class Do : public Condition
{
public:
   explicit Do(size_t pos);
   ~Do() = default;
   void AddLoop(BlockPtr& b) { loop_ = std::move(b); }
   void Check() const override;
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;
   void EnterBlock() override;
   CxxScoped* FindNthItem(const std::string& name, size_t& n) const override;
   void GetUsages(const CodeFile& file, CxxUsageSets& symbols) override;
   bool InLine() const override;
   bool LocateItem(const CxxToken* item, size_t& n) const override;
   CxxToken* PosToItem(size_t pos) const override;
   void Print
      (std::ostream& stream, const NodeBase::Flags& options) const override;
   void UpdatePos(EditorAction action,
      size_t begin, size_t count, size_t from) const override;
   void UpdateXref(bool insert) override;
private:
   bool GetSpan(size_t& begin, size_t& left, size_t& end) const override;

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
   ~Expr() = default;
   void Check() const override;
   void EnterBlock() override;
   void GetUsages(const CodeFile& file, CxxUsageSets& symbols) override;
   CxxToken* PosToItem(size_t pos) const override;
   void Print
      (std::ostream& stream, const NodeBase::Flags& options) const override;
   void UpdatePos(EditorAction action,
      size_t begin, size_t count, size_t from) const override;
   void UpdateXref(bool insert) override;
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
   ~For() = default;
   void AddInitial(TokenPtr& i) { initial_ = std::move(i); }
   void AddSubsequent(ExprPtr& s) { subsequent_ = std::move(s); }
   void AddLoop(BlockPtr& b) { loop_ = std::move(b); }
   void Check() const override;
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;
   void EnterBlock() override;
   void ExitBlock() const override;
   CxxScoped* FindNthItem(const std::string& name, size_t& n) const override;
   void GetUsages(const CodeFile& file, CxxUsageSets& symbols) override;
   bool InLine() const override;
   bool LocateItem(const CxxToken* item, size_t& n) const override;
   CxxToken* PosToItem(size_t pos) const override;
   void Print
      (std::ostream& stream, const NodeBase::Flags& options) const override;
   void UpdatePos(EditorAction action,
      size_t begin, size_t count, size_t from) const override;
   void UpdateXref(bool insert) override;
private:
   bool GetSpan(size_t& begin, size_t& left, size_t& end) const override;

   TokenPtr initial_;
   ExprPtr subsequent_;
   BlockPtr loop_;
};

//------------------------------------------------------------------------------
//
//  A goto statement.
//
class Goto : public CxxStatement
{
public:
   Goto(std::string& label, size_t pos);
   ~Goto() = default;
   void EnterBlock() override;
   void Print
      (std::ostream& stream, const NodeBase::Flags& options) const override;
private:
   std::string label_;
};

//------------------------------------------------------------------------------
//
//  An if statement.
//
class If : public Condition
{
public:
   explicit If(size_t pos);
   ~If() = default;
   void AddThen(BlockPtr& b) { then_ = std::move(b); }
   void AddElse(BlockPtr& b) { else_ = std::move(b); }
   void SetElseIf() { elseif_ = true; }
   void Check() const override;
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;
   void EnterBlock() override;
   CxxScoped* FindNthItem(const std::string& name, size_t& n) const override;
   void GetUsages(const CodeFile& file, CxxUsageSets& symbols) override;
   bool InLine() const override;
   bool LocateItem(const CxxToken* item, size_t& n) const override;
   CxxToken* PosToItem(size_t pos) const override;
   void Print
      (std::ostream& stream, const NodeBase::Flags& options) const override;
   Cxx::ItemType Type() const override { return Cxx::If; }
   void UpdatePos(EditorAction action,
      size_t begin, size_t count, size_t from) const override;
   void UpdateXref(bool insert) override;
private:
   bool GetSpan(size_t& begin, size_t& left, size_t& end) const override;

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
   ~Label() = default;
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;
   void EnterBlock() override;
   void ExitBlock() const override;
   bool InLine() const override { return false; }
private:
   bool GetSpan(size_t& begin, size_t& left, size_t& end) const override;

   std::string name_;
};

//------------------------------------------------------------------------------
//
//  An empty statement.
//
class NoOp : public CxxStatement
{
public:
   NoOp(size_t pos, bool fallthrough);
   ~NoOp() = default;
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;
   void EnterBlock() override { }
   bool InLine() const override { return true; }
   void Print
      (std::ostream& stream, const NodeBase::Flags& options) const override;
   Cxx::ItemType Type() const override { return Cxx::NoOp; }
private:
   const bool fallthrough_;
};

//------------------------------------------------------------------------------
//
//  A return statement.
//
class Return : public CxxStatement
{
public:
   explicit Return(size_t pos);
   ~Return() = default;
   void AddExpr(ExprPtr& e) { expr_ = std::move(e); }
   void Check() const override;
   void EnterBlock() override;
   void GetUsages(const CodeFile& file, CxxUsageSets& symbols) override;
   CxxToken* PosToItem(size_t pos) const override;
   void Print
      (std::ostream& stream, const NodeBase::Flags& options) const override;
   void UpdatePos(EditorAction action,
      size_t begin, size_t count, size_t from) const override;
   void UpdateXref(bool insert) override;
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
   ~Switch() = default;
   void AddExpr(ExprPtr& e) { expr_ = std::move(e); }
   void AddCases(BlockPtr& b) { cases_ = std::move(b); }
   void Check() const override;
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;
   void EnterBlock() override;
   CxxScoped* FindNthItem(const std::string& name, size_t& n) const override;
   void GetUsages(const CodeFile& file, CxxUsageSets& symbols) override;
   bool InLine() const override { return false; }
   bool LocateItem(const CxxToken* item, size_t& n) const override;
   CxxToken* PosToItem(size_t pos) const override;
   void UpdatePos(EditorAction action,
      size_t begin, size_t count, size_t from) const override;
   void UpdateXref(bool insert) override;
private:
   bool GetSpan(size_t& begin, size_t& left, size_t& end) const override;

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
   ~Try() = default;
   void AddTry(BlockPtr& b) { try_ = std::move(b); }
   void AddCatch(TokenPtr& t) { catches_.push_back(std::move(t)); }
   void Check() const override;
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;
   void EnterBlock() override;
   void ExitBlock() const override;
   CxxScoped* FindNthItem(const std::string& name, size_t& n) const override;
   void GetUsages(const CodeFile& file, CxxUsageSets& symbols) override;
   bool InLine() const override { return false; }
   bool LocateItem(const CxxToken* item, size_t& n) const override;
   CxxToken* PosToItem(size_t pos) const override;
   void UpdatePos(EditorAction action,
      size_t begin, size_t count, size_t from) const override;
   void UpdateXref(bool insert) override;
private:
   bool GetSpan(size_t& begin, size_t& left, size_t& end) const override;

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
   ~While() = default;
   void AddLoop(BlockPtr& b) { loop_ = std::move(b); }
   void Check() const override;
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;
   void EnterBlock() override;
   CxxScoped* FindNthItem(const std::string& name, size_t& n) const override;
   void GetUsages(const CodeFile& file, CxxUsageSets& symbols) override;
   bool InLine() const override;
   bool LocateItem(const CxxToken* item, size_t& n) const override;
   CxxToken* PosToItem(size_t pos) const override;
   void Print
      (std::ostream& stream, const NodeBase::Flags& options) const override;
   void UpdatePos(EditorAction action,
      size_t begin, size_t count, size_t from) const override;
   void UpdateXref(bool insert) override;
private:
   bool GetSpan(size_t& begin, size_t& left, size_t& end) const override;

   BlockPtr loop_;
};
}
#endif
