//==============================================================================
//
//  TestDatabase.h
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
#ifndef TESTDATABASE_H_INCLUDED
#define TESTDATABASE_H_INCLUDED

#include "Temporary.h"
#include <cstdint>
#include <iosfwd>
#include <map>
#include <string>
#include <utility>
#include "NbTypes.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeTools
{
//  Database for code coverage, which maps functions to the tests that execute
//  them.
//
class TestDatabase : public NodeBase::Temporary
{
   friend class NodeBase::Singleton< TestDatabase >;
public:
   //  The state of a test.
   //
   enum State
   {
      Invalid,     // not found in database
      Unreported,  // test outcome not yet reported
      Failed,      // test failed
      Reexecute,   // test passed but should be re-executed
      Passed,      // test passed
      State_N      // number of states
   };

   //  Displays database statistics in EXPL.
   //
   NodeBase::word Query(bool verbose, std::string& expl) const;

   //  Updates EXPL with a list of tests that have not passed.
   //
   NodeBase::word Retest(std::string& expl) const;

   //  Removes TEST from the database.
   //
   NodeBase::word Erase(const std::string& testname, std::string& expl);

   //  Sets the state of a test to NEXT.  Returns false if the test is
   //  not in the database.
   //
   bool SetState(const std::string& testname, State next);

   //  Returns the state of a test.  Returns Invalid if the test is not
   //  in the database.
   //
   State GetState(const std::string& testname);

   //  Overridden for restarts.
   //
   void Shutdown(NodeBase::RestartLevel level) override;
private:
   //  Private because this singleton is not subclassed.
   //
   TestDatabase();

   //  Private because this singleton is not subclassed.
   //
   ~TestDatabase();

   enum LoadState
   {
      LoadTest,
      LoadDone,
      LoadError
   };

   //  Reads the test database from InputPath/test.db.txt.
   //
   void Load();

   //  Updates the database by accounting for tests that have been added,
   //  changed, or deleted.
   //
   void Update();

   //  Writes the database to InputPath/test.db.txt.
   //
   void Commit() const;

   //  Parses a line (INPUT) in the test database, which has the form
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

   //  Adds or updates a test when the command "tests begin TESTNAME" is
   //  found in a script located in DIR.
   //
   void Insert(const std::string& testname, const std::string& dir);

   //  Information about a test.
   //
   struct TestInfo
   {
      State state;    // state of test
      uint32_t hash;  // hash value for test's script

      TestInfo(State state, uint32_t hash) : state(state), hash(hash) { }
   };

   //  A tuple for a test's name and its associated information.
   //
   typedef std::pair< std::string, TestInfo > TestData;

   //  A database of tests.
   //
   typedef std::map< std::string, TestInfo > Tests;

   //  The tests in the database.
   //
   Tests tests_;
};

//  Inserts a string for STATE into STREAM.
//
std::ostream& operator<<(std::ostream& stream, TestDatabase::State state);
}
#endif
