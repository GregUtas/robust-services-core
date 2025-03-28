//==============================================================================
//
//  CodeCoverage.h
//
//  Copyright (C) 2013-2025  Greg Utas
//
//  This file is part of the Robust Services Core (RSC).
//
//  RSC is free software: you can redistribute it and/or modify it under the
//  terms of the Lesser GNU General Public License as published by the Free
//  Software Foundation, either version 3 of the License, or (at your option)
//  any later version.
//
//  RSC is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
//  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
//  details.
//
//  You should have received a copy of the Lesser GNU General Public License
//  along with RSC.  If not, see <http://www.gnu.org/licenses/>.
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
//  Database for code coverage, which maps functions to the tests that execute
//  them.
//
class CodeCoverage : public NodeBase::Temporary
{
   friend class NodeBase::Singleton<CodeCoverage>;
public:
   //  Deleted to prohibit copying.
   //
   CodeCoverage(const CodeCoverage& that) = delete;

   //  Deleted to prohibit copy assignment.
   //
   CodeCoverage& operator=(const CodeCoverage& that) = delete;

   //  Adds FUNC to the functions that invoke Debug::ft.  HASH is the hash value
   //  for its source code.  Returns false if FUNC is already in use.
   //
   bool Insert(const std::string& func, uint32_t hash);

   //  Updates EXPL with the outcome of loading the database.  Returns a
   //  non-zero value on failure.
   //
   NodeBase::word Load(std::string& expl);

   //  Provides database information in EXPL.
   //
   NodeBase::word Query(std::string& expl);

   //  Updates EXPL with the functions that are invoked by fewer than MIN tests.
   //
   NodeBase::word Under(size_t min, std::string& expl);

   //  Erases FUNC from the database.
   //
   NodeBase::word Erase(const std::string& func, std::string& expl);

   //  Updates the database by invoking Load (if necessary), followed by Build,
   //  Diff, Retest, Merge, and Commit (see below).  Updates EXPL with details
   //  about what changed.  Returns a non-zero value on failure.
   //
   NodeBase::word Update(std::string& expl);

   //  Overridden for restarts.
   //
   void Shutdown(NodeBase::RestartLevel level) override;
private:
   //  Private because this is a singleton.
   //
   CodeCoverage();

   //  Private because this is a singleton.
   //
   ~CodeCoverage();

   //  Updates EXPL with the outcome of adding test output (*.funcs.txt files)
   //  in the output directory to the database.  Returns a non-zero value on
   //  failure.
   //
   NodeBase::word Build(std::ostringstream& expl);

   //  Updates EXPL with a list of functions that have been added, changed,
   //  or deleted.
   //
   NodeBase::word Diff(std::ostringstream& expl) const;

   //  Updates EXPL with a list of tests for functions that have been added,
   //  changed, or deleted.  Marks those tests for re-execution in the test
   //  database.
   //
   NodeBase::word Retest(std::ostringstream& expl) const;

   //  Merges the previous and current databases and commits the result.
   //
   NodeBase::word Merge(std::ostringstream& expl);

   //  The code coverage database has the form
   //    [<FuncName> <FuncHash> [<TestName>]* "$"]* "$"
   //  The following enum is used when parsing the database:
   //
   enum LoadState
   {
      LoadFunction,  // look for a <FuncName> <FuncHash> pair
      LoadTests,     // look for a [<TestName>]* "$" sequence
      LoadDone,      // final "$" encountered
      LoadError      // error occurred
   };

   //  Looks for a <FuncName> <FuncHash> pair (or the final "$").  INPUT
   //  is the current line in the database.  Returns the next item to
   //  look for, and updates RC and EXPL to report an error or success.
   //
   LoadState GetFunc(std::string& input, NodeBase::word& rc, std::string& expl);

   //  Looks for a [<TestName>]* "$" sequence.  INPUT is what remains of
   //  the current line in the database.  Returns the next item to look
   //  for.
   //
   LoadState GetTests(std::string& input);

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

   //  Replaces each space in S with BLANK and returns the result.
   //
   static std::string Mangle(const std::string& s);

   //  Replaces each BLANK in S with a space and returns the result.
   //
   static std::string Demangle(const std::string& s);

   //  Information about a function.
   //
   struct FuncInfo
   {
      const uint32_t hash;          // hash value for function's code
      std::set<std::string> tests;  // tests that invoke the function

      explicit FuncInfo(uint32_t hash) : hash(hash) { }
   };

   //  A tuple for a function's name and its associated information.
   //
   typedef std::pair<std::string, FuncInfo> FuncData;

   //  A database of functions that invoke Debug::ft.
   //
   typedef std::map<std::string, FuncInfo> Functions;

   //  Commits the database referenced by FUNCS.  Returns false if
   //  the database could not be written.
   //
   static bool Commit(const Functions& funcs);

   //  Functions in the previous database.
   //
   Functions prevFuncs_;

   //  Functions in the current database.
   //
   Functions currFuncs_;

   //  Tests in the previous database.
   //
   std::set<std::string> prevTests_;

   //  Tests in the current database.
   //
   std::set<std::string> currTests_;

   //  The current function whose set of tests is being loaded.
   //
   Functions::iterator loadFunc_;
};
}
#endif
