//==============================================================================
//
//  TestDatabase.h
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
#ifndef TESTDATABASE_H_INCLUDED
#define TESTDATABASE_H_INCLUDED

#include "Temporary.h"
#include <cstdint>
#include <map>
#include <string>
#include <utility>
#include "NbTypes.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeTools
{
//  Database for code coverage, which maps functions to the testcases that
//  execute them.
//
class TestDatabase : public NodeBase::Temporary
{
   friend class NodeBase::Singleton< TestDatabase >;
public:
   //  The state of a testcase.
   //
   enum TestcaseState
   {
      Invalid,     // not found in any "testcase begin" command
      Unreported,  // test outcome not yet reported
      Failed,      // test failed
      Reexecute,   // test passed but should be re-executed
      Passed       // test passed
   };

   //  Displays database statistics in EXPL.
   //
   NodeBase::word Query(std::string& expl) const;

   //  Updates EXPL with a list of testcases that have not passed.
   //
   NodeBase::word Retest(std::string& expl) const;

   //  Removes invalid testcases from the database.
   //
   NodeBase::word Erase(std::string& expl);

   //  Updates the state of a testcase.
   //
   void SetState(const std::string& testcase, TestcaseState state);

   //  Returns the state of a testcase.  Returns Invalid if the testcase
   //  is not in the database.
   //
   TestcaseState GetState(const std::string& testcase);
private:
   //  Private because this singleton is not subclassed.
   //
   TestDatabase();

   enum LoadState
   {
      GetTestcase,
      LoadDone,
      LoadError
   };

   //  Reads the testcase database from InputPath/test.db.txt.
   //
   void Load();

   //  Updates the database by accounting for testcases that have been added,
   //  changed, or deleted.
   //
   void Update();

   //  Writes the database to InputPath/test.db.txt.
   //
   void Commit() const;

   //  Parses a line (INPUT) in the testcase database, which has the form
   //    [<TestName> <TestState> <TestHash>]* "$"
   //  Returns the next type of item to look for.
   //
   LoadState GetTest(std::string& input);

   //  Invoked to report a parsing error.  EXPL provides an explanation.
   //  Returns LoadError.
   //
   static LoadState GetError(const std::string& reason);

   // '$' is used as an end-of-record delimiter in the database.
   //
   static const char DELIMITER = '$';

   //  UINT32_MAX is used as the hash value for unhashed items.
   //
   static const uint32_t UNHASHED = UINT32_MAX;

   //  Adds or updates a testcase when the command  "testcase begin TEST"
   //  is found in a script located in DIR.
   //
   void AddTest(const std::string& test, const std::string& dir);

   //  Information about a testcase.
   //
   struct TestInfo
   {
      TestcaseState state;  // state of testcase
      const uint32_t hash;  // hash value for testcase's script

      TestInfo(TestcaseState state, uint32_t hash): state(state), hash(hash) { }
   };

   //  A tuple for a testcase's name and its associated information.
   //
   typedef std::pair< std::string, TestInfo > TestData;

   //  A database of testcases.
   //
   typedef std::map< std::string, TestInfo > Testcases;

   //  Testcases in the previous database.
   //
   Testcases tests_;
};
}
#endif
