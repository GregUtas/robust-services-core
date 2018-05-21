//==============================================================================
//
//  CodeCoverage.h
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
#ifndef CODECOVERAGE_H_INCLUDED
#define CODECOVERAGE_H_INCLUDED

#include "Temporary.h"
#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <map>
#include <set>
#include <string>
#include <utility>
#include "NbTypes.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace CodeTools
{
//  Database for code coverage, which maps functions to the testcases that
//  execute them.
//
class CodeCoverage : public NodeBase::Temporary
{
   friend class NodeBase::Singleton< CodeCoverage >;
public:
   //  Adds FN, in namespace NS, to the functions that invoke Debug::ft.
   //  HASH is the hash value for its source code.  Returns FALSE if FN
   //  is already in use within a different namespace.
   //
   bool AddFunc(const std::string& fn, uint32_t hash, const std::string& ns);

   //  Adds testcase output in the output directory to the database.  Returns
   //  a non-zero value on failure and updates EXPL with an explanation.
   //
   NodeBase::word Build(std::string& expl);

   //  Reads a code coverage database from STREAM.  Returns a non-zero value
   //  on failure and updates EXPL with an explanation.
   //
   NodeBase::word Load(std::istream& stream, std::string& expl);

   //  Displays database statistics in EXPL.
   //
   NodeBase::word Query(std::string& expl) const;

   //  Lists functions that are invoked by fewer than MIN testcases in EXPL.
   //
   NodeBase::word Under(size_t min, std::string& expl) const;

   //  Updates EXPL with a list of functions and testcases that have been
   //  added, changed, or deleted.
   //
   NodeBase::word Diff(std::string& expl) const;

   //  Updates EXPL with a list of testcases for functions that have been
   //  added, changed, or deleted.
   //
   NodeBase::word Retest(std::string& expl) const;

   //  Erases ITEM (a function if FUNC is set, else a testcase) from a
   //  database (the previous database if PREV is set, else the current
   //  database).
   //
   NodeBase::word Erase
      (std::string& item, bool prev, bool func, std::string& expl);

   //  Writes the current database to STREAM after including items that appear
   //  only in the previous database.  Returns a non-zero value on failure and
   //  updates EXPL with an explanation.
   //
   NodeBase::word Dump(std::ostream& stream, std::string& expl);
private:
   //  Private because this singleton is not subclassed.
   //
   CodeCoverage();

   enum LoadState
   {
      GetFunction,
      GetTestcases,
      GetTestcase,
      LoadDone,
      LoadError
   };

   //  The following functions parse a code coverage database, which
   //  has the form
   //    [<FuncName> <FuncHash> [<TestName>]* "$"]*
   //    [<TestName> <TestHash>]* "$"
   //  Each function takes INPUT, which is what remains of the current
   //  line in the database, and it returns the next type of item to look
   //  for.  RC and EXPL are updated to report an error or success.

   //  Looks for a <FuncName> <FuncHash> pair.
   //
   LoadState GetFunc(std::string& input,
      NodeBase::word& rc, std::string& expl);

   //  Looks for the <TestName>* sequence that follows a function.
   //
   LoadState GetTests(std::string& input) const;

   //  Looks for a <TestName> <TestHash> pair.
   //
   LoadState GetTest(std::string& input,
      NodeBase::word& rc, std::string& expl);

   //  Invoked to report a parsing error.  Sets EXPL to REASON, RC to -1,
   //  and returns LoadError.
   //
   static LoadState GetError(const std::string& reason,
      NodeBase::word& rc, std::string& expl);

   // '$' is used as an end-of-record delimiter in the database.
   //
   static const char DELIMITER = '$';

   // '`' is used to replace a space in a function name.
   //
   static const char BLANK = '`';

   //  UINT32_MAX is used as the hash value for unhashed items.
   //
   static const uint32_t UNHASHED = UINT32_MAX;

   //  Replaces each space in S with Blank and returns the result.
   //
   static std::string Mangle(const std::string& s);

   //  Replaces each Blank in S with a space and returns the result.
   //
   static std::string Demangle(const std::string& s);

   //  Adds TEST, whose script appears in DIR if FOUND is set, to the current
   //  database.  Returns true on success.
   //
   bool AddTest(const std::string& test, bool found, const std::string& dir);

   //  Information about a function.
   //
   struct FuncInfo
   {
      const std::string ns;         // name of function's namespace
      const uint32_t hash;          // hash value for function's code
      std::set<std::string> tests;  // tests that invoke the function

      FuncInfo(const std::string& ns, uint32_t hash): ns(ns), hash(hash) { }
      explicit FuncInfo(uint32_t hash): hash(hash) { }
   };

   //  A tuple for a function's name and its associated information.
   //
   typedef std::pair< std::string, FuncInfo > FuncData;

   //  A database of functions that invoke Debug::ft.
   //
   typedef std::map< std::string, FuncInfo > Functions;

   //  Iterators for accessing functions.
   //
   typedef Functions::iterator FuncIter;
   typedef Functions::const_iterator ConstFuncIter;

   //  Information about a testcase.
   //
   struct TestInfo
   {
      const uint32_t hash;  // hash value for testcase script

      explicit TestInfo(uint32_t hash): hash(hash) { }
   };

   //  A tuple for a testcase's name and its associated information.
   //
   typedef std::pair< std::string, TestInfo > TestData;

   //  A database of testcases.
   //
   typedef std::map< std::string, TestInfo > Testcases;

   //  Functions in the previous database.
   //
   Functions prevFuncs_;

   //  Functions in the current database.
   //
   Functions currFuncs_;

   //  Testcases in the previous database.
   //
   Testcases prevTests_;

   //  Testcases in the current database.
   //
   Testcases currTests_;

   //  The current function whose testcase set is being loaded.
   //
   FuncIter currFunc_;
};
}
#endif