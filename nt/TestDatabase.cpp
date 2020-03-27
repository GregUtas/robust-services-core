//==============================================================================
//
//  TestDatabase.cpp
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
#include "TestDatabase.h"
#include <cctype>
#include <cstddef>
#include <cstdio>
#include <iomanip>
#include <ios>
#include <istream>
#include <memory>
#include <set>
#include <sstream>
#include "Algorithms.h"
#include "Debug.h"
#include "Element.h"
#include "Formatters.h"
#include "FunctionGuard.h"
#include "NbCliParms.h"
#include "SysFile.h"

using namespace NodeBase;
using std::ostream;
using std::setw;
using std::string;

//------------------------------------------------------------------------------

namespace NodeTools
{
fixed_string StateStrings[TestDatabase::State_N + 1] =
{
   "invalid",
   "unreported",
   "failed",
   "re-execute",
   "passed",
   ERROR_STR
};

ostream& operator<<(ostream& stream, TestDatabase::State state)
{
   if((state >= 0) && (state < TestDatabase::State_N))
      stream << StateStrings[state];
   else
      stream << StateStrings[TestDatabase::State_N];
   return stream;
}

//------------------------------------------------------------------------------

fn_name TestDatabase_ctor = "TestDatabase.ctor";

TestDatabase::TestDatabase()
{
   Debug::ft(TestDatabase_ctor);

   Load();
   Update();
}

//------------------------------------------------------------------------------

fn_name TestDatabase_dtor = "TestDatabase.dtor";

TestDatabase::~TestDatabase()
{
   Debug::ft(TestDatabase_dtor);
}

//------------------------------------------------------------------------------

fn_name TestDatabase_Commit = "TestDatabase.Commit";

void TestDatabase::Commit() const
{
   Debug::ft(TestDatabase_Commit);

   FunctionGuard guard(FunctionGuard::MakePreemptable);

   auto path = Element::InputPath() + PATH_SEPARATOR + "testcase.db.txt";
   auto stream = SysFile::CreateOstream(path.c_str(), true);

   if(stream == nullptr)
   {
      auto expl = "Failed to create testcase database";
      Debug::SwLog(TestDatabase_Commit, expl, 0);
      return;
   }

   for(auto t = tests_.cbegin(); t != tests_.cend(); ++t)
   {
      *stream << t->first << SPACE << int(t->second.state) << SPACE;
      *stream << std::hex << t->second.hash << std::dec << CRLF;
   }

   *stream << DELIMITER << CRLF;
}

//------------------------------------------------------------------------------

fn_name TestDatabase_Erase = "TestDatabase.Erase";

word TestDatabase::Erase(const string& test, string& expl)
{
   Debug::ft(TestDatabase_Erase);

   auto item = tests_.find(test);

   if(item != tests_.end())
   {
      tests_.erase(item);
      Commit();
      expl = SuccessExpl;
      return 0;
   }

   expl = "That testcase is not in the database.";
   return 0;
}

//------------------------------------------------------------------------------

fn_name TestDatabase_GetError = "TestDatabase.GetError";

TestDatabase::LoadState TestDatabase::GetError(const string& reason)
{
   Debug::ft(TestDatabase_GetError);

   Debug::SwLog(TestDatabase_GetError, reason, 0);
   return LoadError;
}

//------------------------------------------------------------------------------

TestDatabase::State TestDatabase::GetState(const string& testcase)
{
   auto test = tests_.find(testcase);
   if(test == tests_.cend()) return Invalid;
   return test->second.state;
}

//------------------------------------------------------------------------------

fn_name TestDatabase_GetTest = "TestDatabase.GetTest";

TestDatabase::LoadState TestDatabase::GetTest(string& input)
{
   Debug::ft(TestDatabase_GetTest);

   auto name = strGet(input);
   if(name.front() == DELIMITER) return LoadDone;
   if(name.empty()) return GetError("Name not found");
   auto state = strGet(input);
   if(state.empty()) return GetError("State not found");
   if(!isdigit(state.front())) return GetError("State corrupted");
   auto s = static_cast< State >(std::stoi(state));
   if((s <= Invalid) || (s > Passed)) return GetError("State out of range");
   auto hash = strGet(input);
   if(hash.empty() || !isxdigit(hash.front()))
      return GetError("Testcase hash value not found");
   uint32_t n = std::stoul(hash, nullptr, 16);
   TestInfo info(s, n);
   auto result = tests_.insert(TestData(name, info));
   if(!result.second) return GetError("Testcase name duplicated");
   return GetTestcase;
}

//------------------------------------------------------------------------------

fn_name TestDatabase_Insert = "TestDatabase.Insert";

void TestDatabase::Insert(const string& test, const string& dir)
{
   Debug::ft(TestDatabase_Insert);

   auto state = Unreported;
   uint32_t hash = UNHASHED;

   //  If a script named TEST exists, calculate its hash value.
   //
   auto path = dir + PATH_SEPARATOR + test + ".txt";
   auto stream = SysFile::CreateIstream(path.c_str());

   if(stream != nullptr)
   {
      string contents;
      string input;

      while(stream->peek() != EOF)
      {
         std::getline(*stream, input);
         contents += input;
      }

      hash = stringHash(contents.c_str());
   }

   auto prev = tests_.find(test);

   if(prev != tests_.end())
   {
      //  TEST was already in the database.  If its hash value changed,
      //  update it and mark it unreported if it had previously passed.
      //
      if(prev->second.hash != hash)
      {
         prev->second.hash = hash;
         if(prev->second.state == Passed) prev->second.state = Reexecute;
      }
   }
   else
   {
      //  TEST was not in the database, so add it.
      //
      TestInfo info(state, hash);
      tests_.insert(TestData(test, info));
   }
}

//------------------------------------------------------------------------------

fn_name TestDatabase_Load = "TestDatabase.Load";

void TestDatabase::Load()
{
   Debug::ft(TestDatabase_Load);

   FunctionGuard guard(FunctionGuard::MakePreemptable);

   auto path = Element::InputPath() + PATH_SEPARATOR + "testcase.db.txt";
   auto stream = SysFile::CreateIstream(path.c_str());

   if(stream == nullptr)
   {
      auto expl = "Failed to load testcase database";
      Debug::SwLog(TestDatabase_Load, expl, 0);
      return;
   }

   string input;
   auto state = GetTestcase;
   tests_.clear();

   while(stream->peek() != EOF)
   {
      std::getline(*stream, input);

      while(!input.empty() && (state == GetTestcase))
      {
         state = GetTest(input);
      }
   }

   if(state == GetTestcase)
   {
      auto expl = "Reached end of database unexpectedly";
      Debug::SwLog(TestDatabase_Load, expl, 0);
   }

   stream.reset();
}

//------------------------------------------------------------------------------

fn_name TestDatabase_Query = "TestDatabase.Query";

word TestDatabase::Query(bool verbose, string& expl) const
{
   Debug::ft(TestDatabase_Query);

   size_t states[State_N] = { 0 };

   for(auto t = tests_.cbegin(); t != tests_.cend(); ++t)
   {
      ++states[t->second.state];
   }

   std::ostringstream stream;

   for(auto s = Invalid + 1; s < State_N; ++s)
   {
      auto state = static_cast< State >(s);
      stream << spaces(2) << state << ": " << states[s];
   }

   stream << CRLF;

   if(verbose)
   {
      stream << setw(40) << "Testcase" << spaces(3) << "State" << CRLF;

      for(auto t = tests_.cbegin(); t != tests_.cend(); ++t)
      {
         stream << setw(40) << t->first << spaces(3) << t->second.state << CRLF;
      }
   }

   expl = stream.str();
   return 0;
}

//------------------------------------------------------------------------------

fn_name TestDatabase_Retest = "TestDatabase.Retest";

word TestDatabase::Retest(string& expl) const
{
   Debug::ft(TestDatabase_Retest);

   size_t count = 0;
   std::ostringstream stream;

   for(auto t = tests_.cbegin(); t != tests_.cend(); ++t)
   {
      if(t->second.state != Passed)
      {
         stream << t->first << CRLF;
         ++count;
      }
   }

   if(count == 0)
   {
      expl = "No testcases require retesting.";
      return 0;
   }

   stream << "...total=" << count;
   expl = stream.str();
   return 0;
}

//------------------------------------------------------------------------------

fn_name TestDatabase_SetState = "TestDatabase.SetState";

void TestDatabase::SetState(const string& testcase, State next)
{
   Debug::ft(TestDatabase_SetState);

   auto test = tests_.find(testcase);

   if(test == tests_.end())
   {
      auto expl = "Non-existent testcase: " + testcase;
      Debug::SwLog(TestDatabase_SetState, expl, 0);
      return;
   };

   //  If the state has changed, update the database.  However,
   //  a failed testcase remains failed rather than being marked
   //  for re-execution.
   //
   auto curr = test->second.state;

   if(curr != next)
   {
      if((curr != Failed) || (next != Reexecute))
      {
         test->second.state = next;
         Commit();
      }
   }
}

//------------------------------------------------------------------------------

fn_name TestDatabase_Shutdown = "TestDatabase.Shutdown";

void TestDatabase::Shutdown(RestartLevel level)
{
   Debug::ft(TestDatabase_Shutdown);

   tests_.clear();
}

//------------------------------------------------------------------------------

fn_name TestDatabase_Update = "TestDatabase.Update";

void TestDatabase::Update()
{
   Debug::ft(TestDatabase_Update);

   FunctionGuard guard(FunctionGuard::MakePreemptable);

   //  Find all *.txt files in the input directory.
   //
   auto errors = 0;
   auto indir = Element::InputPath();
   std::set< string > files;

   if(!SysFile::FindFiles(indir.c_str(), ".txt", files))
   {
      auto expl = "Could not open directory " + indir;
      Debug::SwLog(TestDatabase_Update, expl, 0);
   }

   //  Search each *.txt file for the command "testcase begin", which
   //  precedes the name of a testcase, and add (update) the testcase
   //  to (in) the database.
   //
   for(auto f = files.cbegin(); f != files.cend(); ++f)
   {
      auto path = indir + PATH_SEPARATOR + *f + ".txt";
      auto stream = SysFile::CreateIstream(path.c_str());

      if(stream == nullptr)
      {
         ++errors;
         continue;
      }

      string input;

      while(stream->peek() != EOF)
      {
         std::getline(*stream, input);
         auto str = strGet(input);
         if(str != "testcase") continue;
         str = strGet(input);
         if(str != "begin") continue;
         str = strGet(input);
         if(str.empty()) continue;
         Insert(str, indir);
      }
   }

   if(errors > 0)
   {
      auto expl = "Errors opening files: " + std::to_string(errors);
      Debug::SwLog(TestDatabase_Update, expl, 0);
   }

   guard.Release();
   Commit();
}
}
