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
#include <cstdio>
#include <ios>
#include <istream>
#include <memory>
#include <ostream>
#include <set>
#include "Algorithms.h"
#include "Debug.h"
#include "Element.h"
#include "Formatters.h"
#include "SysFile.h"

using namespace NodeBase;
using std::string;

//------------------------------------------------------------------------------

namespace NodeTools
{
fn_name TestDatabase_ctor = "TestDatabase.ctor";

TestDatabase::TestDatabase()
{
   Debug::ft(TestDatabase_ctor);

   Load();
   Update();
}

//------------------------------------------------------------------------------

fn_name TestDatabase_AddTest = "TestDatabase.AddTest";

void TestDatabase::AddTest(const std::string& test, const std::string& dir)
{
   Debug::ft(TestDatabase_AddTest);

   TestcaseState state = Invalid;
   uint32_t hash = UNHASHED;

   //  If a script named TEST exists, calculate its hash value.
   //
   auto file = dir + PATH_SEPARATOR + test + ".txt";
   auto stream = SysFile::CreateIstream(file.c_str());
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
      state = Unreported;
   }

   //  If TEST is already in the database, update its state:
   //  o if its script was not found, invalidate it
   //  o if its script was modified, flag it for rexecution
   //  If TEST was not in the database, insert it.
   //
   auto prev = tests_.find(test);
   if(prev != tests_.end())
   {
      if(stream == nullptr)
      {
         prev->second.state = Invalid;
      }
      else
      {
         if(prev->second.hash != hash)
            prev->second.state = Reexecute;
      }
   }
   else
   {
      auto info = TestInfo(state, hash);
      tests_.insert(TestData(test, info));
   }
}

//------------------------------------------------------------------------------

fn_name TestDatabase_Commit = "TestDatabase.Commit";

void TestDatabase::Commit() const
{
   Debug::ft(TestDatabase_Commit);

   auto file = Element::InputPath() + PATH_SEPARATOR + "test.db.txt";
   auto stream = SysFile::CreateOstream(file.c_str(), false);

   if(stream == nullptr)
   {
      auto expl = "Failed to create testcase database";
      Debug::SwErr(TestDatabase_Commit, expl, 0);
      return;
   }

   for(auto t = tests_.cbegin(); t != tests_.cend(); ++t)
   {
      *stream << t->first << SPACE << t->second.state << SPACE;
      *stream << std::hex << t->second.hash << std::dec << CRLF;
   }

   *stream << DELIMITER << CRLF;
}

//------------------------------------------------------------------------------

fn_name TestDatabase_Erase = "TestDatabase.Erase";

word TestDatabase::Erase(string& expl)
{
   Debug::ft(TestDatabase_Erase);

   auto size = tests_.size();
   auto t = tests_.cbegin();

   while(t != tests_.cend())
   {
      if(t->second.state == Invalid)
         t = tests_.erase(t);
      else
         ++t;
   }

   if(tests_.size() < size) Commit();
   return 0;
}

//------------------------------------------------------------------------------

fn_name TestDatabase_GetError = "TestDatabase.GetError";

TestDatabase::LoadState TestDatabase::GetError(const string& reason)
{
   Debug::ft(TestDatabase_GetError);

   Debug::SwErr(TestDatabase_GetError, reason, 0);
   return LoadError;
}

//------------------------------------------------------------------------------

TestDatabase::TestcaseState TestDatabase::GetState(const string& testcase)
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
   TestcaseState s = static_cast< TestcaseState >(std::stol(state));
   if((s <= Invalid) || (s > Passed)) return GetError("State out of range");
   auto hash = strGet(input);
   if(hash.empty() || !isxdigit(hash.front()))
      return GetError("Testcase hash value not found");
   uint32_t n = std::stoul(hash, nullptr, 16);
   auto info = TestInfo(s, n);
   auto result = tests_.insert(TestData(name, info));
   if(!result.second) return GetError("Testcase name duplicated");
   return GetTestcase;
}

//------------------------------------------------------------------------------

fn_name TestDatabase_Load = "TestDatabase.Load";

void TestDatabase::Load()
{
   Debug::ft(TestDatabase_Load);

   auto file = Element::InputPath() + PATH_SEPARATOR + "test.db.txt";
   auto stream = SysFile::CreateIstream(file.c_str());

   if(stream == nullptr)
   {
      auto expl = "Failed to load testcase database";
      Debug::SwErr(TestDatabase_Load, expl, 0);
      return;
   }

   string expl;
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
      expl = "Reached end of database unexpectedly";
      Debug::SwErr(TestDatabase_Load, expl, 0);
   }

   stream.reset();
   Update();
}

//------------------------------------------------------------------------------

fn_name TestDatabase_Query = "TestDatabase.Query";

word TestDatabase::Query(string& expl) const
{
   Debug::ft(TestDatabase_Query);

   //* To be implemented.

   return 0;
}

//------------------------------------------------------------------------------

fn_name TestDatabase_Retest = "TestDatabase.Retest";

word TestDatabase::Retest(string& expl) const
{
   Debug::ft(TestDatabase_Retest);

   for(auto t = tests_.cbegin(); t != tests_.cend(); ++t)
   {
      switch(t->second.state)
      {
      case Invalid:
      case Passed:
         break;
      default:
         expl.append(t->first + CRLF);
      }
   }

   if(expl.empty())
      expl = "No testcases require retesting.";
   else
      expl.pop_back();

   return 0;
}

//------------------------------------------------------------------------------

fn_name TestDatabase_SetState = "TestDatabase.SetState";

void TestDatabase::SetState(const string& testcase, TestcaseState state)
{
   Debug::ft(TestDatabase_SetState);

   auto test = tests_.find(testcase);

   if(test == tests_.end())
   {
      auto expl = "Non-existent testcase: " + testcase;
      Debug::SwErr(TestDatabase_SetState, expl, 0);
      return;
   };

   test->second.state = state;
   Commit();
}

//------------------------------------------------------------------------------

fn_name TestDatabase_Update = "TestDatabase.Update";

void TestDatabase::Update()
{
   Debug::ft(TestDatabase_Update);

   //  Find all *.txt files in the input directory.
   //
   auto errors = 0;
   auto indir = Element::InputPath();
   std::set< string > files;

   if(!SysFile::FindFiles(indir.c_str(), ".txt", files))
   {
      auto expl = "Could not open directory " + indir;
      Debug::SwErr(TestDatabase_Update, expl, 0);
   }

   //  Search each *.txt file for the command "testcase begin", which
   //  precedes the name of a testcase, and add (update) the testcase
   //  to (in) the database.
   //
   for(auto f = files.cbegin(); f != files.cend(); ++f)
   {
      auto file = indir + PATH_SEPARATOR + *f;
      auto stream = SysFile::CreateIstream(file.c_str());
      if(stream == nullptr)
      {
         ++errors;
         continue;
      }

      //  Extract the function names from STREAM.  They begin on the fourth
      //  line, and each one is preceded by two integers.
      //
      string input;

      while(stream->peek() != EOF)
      {
         std::getline(*stream, input);
         auto str = strGet(input);
         if(str != "testcase") break;
         str = strGet(input);
         if(str != "begin") break;
         str = strGet(input);
         if(str.empty()) break;
         AddTest(str, indir);
      }
   }

   if(errors > 0)
   {
      auto expl = "Errors opening files: " + std::to_string(errors);
      Debug::SwErr(TestDatabase_Update, expl, 0);
   }

   Commit();
}
}
