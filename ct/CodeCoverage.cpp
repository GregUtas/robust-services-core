//==============================================================================
//
//  CodeCoverage.cpp
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
#include "CodeCoverage.h"
#include <cctype>
#include <cstdio>
#include <iomanip>
#include <ios>
#include <istream>
#include <sstream>
#include <vector>
#include "Debug.h"
#include "Element.h"
#include "Formatters.h"
#include "FunctionGuard.h"
#include "NbCliParms.h"
#include "Singleton.h"
#include "SysFile.h"
#include "TestDatabase.h"

using namespace NodeBase;
using namespace NodeTools;
using std::setw;
using std::string;

//------------------------------------------------------------------------------

namespace CodeTools
{
CodeCoverage::CodeCoverage()
{
   Debug::ft("CodeCoverage.ctor");
}

//------------------------------------------------------------------------------

CodeCoverage::~CodeCoverage()
{
   Debug::ftnt("CodeCoverage.dtor");
}

//------------------------------------------------------------------------------

word CodeCoverage::Build(std::ostringstream& expl)
{
   Debug::ft("CodeCoverage.Build");

   if(currFuncs_.empty())
   {
      expl << "Must run >check on all code files before >coverage build.";
      return -1;
   }

   auto testdb = Singleton< TestDatabase >::Instance();

   FunctionGuard guard(Guard_MakePreemptable);

   //  Find all *.funcs.txt files in the output directory.  If a test
   //  with the same file name exists, add it, along with the functions
   //  that it invoked, to the current database.
   //
   auto outdir = Element::OutputPath();
   std::set< string > traces;

   if(!SysFile::FindFiles(outdir.c_str(), ".funcs.txt", traces))
   {
      expl << "Could not open directory " << outdir;
      return -1;
   }

   auto count = 0;

   for(auto trace = traces.cbegin(); trace != traces.cend(); ++trace)
   {
      if(testdb->GetState(*trace) == TestDatabase::Invalid) continue;

      auto path = outdir + PATH_SEPARATOR + *trace + ".funcs.txt";
      auto stream = SysFile::CreateIstream(path.c_str());

      if(stream == nullptr)
      {
         expl << "Failed to open " << path << CRLF << spaces(2);
         continue;
      }

      currTests_.insert(*trace);
      ++count;

      //  Extract the function names from STREAM.  Each is preceded by two
      //  integers.
      //
      string input;
      string str;

      while(stream->peek() != EOF)
      {
         std::getline(*stream, input);

         str = strGet(input);
         if(str.empty() || !isdigit(str.front())) continue;
         str = strGet(input);
         if(str.empty() || !isdigit(str.front())) continue;
         str = strGet(input);
         if(str.empty()) continue;

         //  If INPUT isn't empty, append it to STR.  This function name
         //  contains an embedded space (and might have more of them).
         //  To simply Load, mangle its name by replacing embedded spaces
         //  with our "BLANK" character.
         //
         if(!input.empty())
         {
            str += Mangle(input);
            input.clear();
         }

         auto func = currFuncs_.find(str);

         if(func == currFuncs_.cend())
         {
            //  This function was not in the database, so add it.
            //
            FuncInfo info(UNHASHED);
            auto result = currFuncs_.insert(FuncData(str, info));
            func = result.first;
         }

         func->second.tests.insert(*trace);
      }
   }

   expl << count << " *.funcs.txt file(s) processed";
   return 0;
}

bool CodeCoverage::Commit(const Functions& funcs)
{
   Debug::ft("CodeCoverage.Commit");

   FunctionGuard guard(Guard_MakePreemptable);

   auto path = Element::InputPath() + PATH_SEPARATOR + "coverage.db.txt";
   auto stream = SysFile::CreateOstream(path.c_str(), true);
   if(stream == nullptr) return false;

   for(auto f = funcs.cbegin(); f != funcs.cend(); ++f)
   {
      *stream << f->first << SPACE << std::hex << f->second.hash << std::dec;

      auto& tests = f->second.tests;

      if(!tests.empty())
      {
         for(auto t = tests.cbegin(); t != tests.cend(); ++t)
         {
            *stream << SPACE << *t;
         }
      }

      *stream << SPACE << DELIMITER << CRLF;
   }

   *stream << DELIMITER << CRLF;
   return true;
}

//------------------------------------------------------------------------------

bool CodeCoverage::Defined(const string& func) const
{
   return (currFuncs_.find(Mangle(func)) != currFuncs_.cend());
}

//------------------------------------------------------------------------------

string CodeCoverage::Demangle(const string& s)
{
   string result(s);

   for(size_t i = 0; i < result.size(); ++i)
   {
      if(result[i] == BLANK) result[i] = SPACE;
   }

   return result;
}

//------------------------------------------------------------------------------

word CodeCoverage::Diff(std::ostringstream& expl) const
{
   Debug::ft("CodeCoverage.Diff");

   expl << "Added functions: ";
   auto found = false;

   for(auto c = currFuncs_.cbegin(); c != currFuncs_.cend(); ++c)
   {
      if(prevFuncs_.find(c->first) == prevFuncs_.cend())
      {
         expl << CRLF << spaces(2) << Demangle(c->first);
         found = true;
      }
   }

   if(!found) expl << "none";
   expl << CRLF << "Changed functions: ";
   found = false;

   for(auto c = currFuncs_.cbegin(); c != currFuncs_.cend(); ++c)
   {
      auto p = prevFuncs_.find(c->first);

      if((p != prevFuncs_.cend()) && (c->second.hash != p->second.hash))
      {
         expl << CRLF << spaces(2) << Demangle(c->first);
         found = true;
      }
   }

   if(!found) expl << "none";
   expl << CRLF << "Deleted functions: ";
   found = false;

   for(auto p = prevFuncs_.cbegin(); p != prevFuncs_.cend(); ++p)
   {
      if((p->second.hash != UNHASHED) &&
         (currFuncs_.find(p->first) == currFuncs_.cend()))
      {
         expl << CRLF << spaces(2) << Demangle(p->first);
         found = true;
      }
   }

   if(!found) expl << "none";
   return 0;
}

//------------------------------------------------------------------------------

word CodeCoverage::Erase(const string& func, string& expl)
{
   Debug::ft("CodeCoverage.Erase");

   if(prevFuncs_.empty())
   {
      auto rc = Load(expl);
      if(rc != 0) return rc;
   }

   auto name = Mangle(func);
   auto found = prevFuncs_.erase(name);

   if(found > 0)
   {
      if(Commit(prevFuncs_))
      {
         expl = SuccessExpl;
         return 0;
      }

      expl = CreateStreamFailure;
      return -7;
   }

   expl = "No such entry: " + func;
   return -1;
}

//------------------------------------------------------------------------------

CodeCoverage::LoadState CodeCoverage::GetError
   (const string& reason, word& rc, string& expl)
{
   Debug::ft("CodeCoverage.GetError");

   expl = reason;
   rc = -1;
   return LoadError;
}

//------------------------------------------------------------------------------

CodeCoverage::LoadState CodeCoverage::GetFunc
   (string& input, word& rc, string& expl)
{
   Debug::ft("CodeCoverage.GetFunc");

   auto func = strGet(input);
   if(func.empty()) return LoadFunction;
   if(func.front() == DELIMITER) return LoadDone;
   auto hash = strGet(input);
   if(hash.empty() || !isxdigit(hash.front()))
      return GetError("Hash value for function missing", rc, expl);
   uint32_t n = std::stoul(hash, nullptr, 16);
   FuncInfo info(n);
   auto result = prevFuncs_.insert(FuncData(func, info));
   if(!result.second)
      return GetError("Function name duplicated", rc, expl);
   loadFunc_ = result.first;
   return LoadTests;
}

//------------------------------------------------------------------------------

CodeCoverage::LoadState CodeCoverage::GetTests(string& input)
{
   Debug::ft("CodeCoverage.GetTests");

   auto test = strGet(input);
   if(test.empty()) return LoadTests;
   if(test.front() == DELIMITER) return LoadFunction;
   loadFunc_->second.tests.insert(test);
   prevTests_.insert(test);
   return LoadTests;
}

//------------------------------------------------------------------------------

bool CodeCoverage::Insert(const string& func, uint32_t hash)
{
   Debug::ft("CodeCoverage.Insert");

   auto name = Mangle(func);
   auto iter = currFuncs_.find(name);
   if(iter != currFuncs_.cend()) return false;

   FuncInfo info(hash);
   auto result = currFuncs_.insert(FuncData(name, info));
   return result.second;
}

//------------------------------------------------------------------------------

word CodeCoverage::Load(string& expl)
{
   Debug::ft("CodeCoverage.Load");

   FunctionGuard guard(Guard_MakePreemptable);

   auto path = Element::InputPath() + PATH_SEPARATOR + "coverage.db.txt";
   auto stream = SysFile::CreateIstream(path.c_str());
   if(stream == nullptr)
   {
      expl = NoFileExpl;
      return -2;
   }

   expl = "Error: explanation not provided.";
   word rc = -1;
   string input;
   auto state = LoadFunction;
   prevFuncs_.clear();
   prevTests_.clear();

   while(stream->peek() != EOF)
   {
      std::getline(*stream, input);

      while(!input.empty())
      {
         switch(state)
         {
         case LoadFunction:
            state = GetFunc(input, rc, expl);
            break;
         case LoadTests:
            state = GetTests(input);
            break;
         case LoadDone:
            expl = "Extra text in database: " + input;
            return -1;
         case LoadError:
         default:
            return rc;
         }
      }
   }

   if(state != LoadDone)
   {
      expl = "Parsing error: reached end of file unexpectedly.";
      return -1;
   }

   expl = SuccessExpl;
   return 0;
}

//------------------------------------------------------------------------------

string CodeCoverage::Mangle(const string& s)
{
   string result(s);

   for(size_t i = 0; i < result.size(); ++i)
   {
      if(result[i] == SPACE) result[i] = BLANK;
   }

   return result;
}

//------------------------------------------------------------------------------

word CodeCoverage::Merge(std::ostringstream& expl)
{
   Debug::ft("CodeCoverage.Merge");

   auto testdb = Singleton< TestDatabase >::Instance();
   std::set< string > inclTests;

   //  Update the current database with tests that appear only in the
   //  previous database.  Verify that a test is still in the database
   //  before adding it.
   //
   for(auto p = prevTests_.cbegin(); p != prevTests_.cend(); ++p)
   {
      if((currTests_.find(*p) == currTests_.cend()) &&
         (testdb->GetState(*p) != TestDatabase::Invalid))
      {
         currTests_.insert(*p);
         inclTests.insert(*p);
      }
   }

   //  If a function without a hash value appears only in the previous
   //  database, add it to the current database.  (A function *with* a
   //  hash value must have been deleted from the code base if it only
   //  appears in the previous database.)
   //
   for(auto p = prevFuncs_.cbegin(); p != prevFuncs_.cend(); ++p)
   {
      if((p->second.hash == UNHASHED) &&
         (currFuncs_.find(p->first) == currFuncs_.cend()))
      {
         currFuncs_.insert(*p);
      }
   }

   //  Look at functions that exist in both databases.  If a test invoked
   //  a function in the previous database, insert it as an invoker of that
   //  function in the current database *if the test was just added to the
   //  current database, above*.  (If the test wasn't added to the current
   //  database, it must have been deleted or re-executed; in the latter case,
   //  it has already been inserted as an invoker of all its functions.)
   //
   for(auto pf = prevFuncs_.cbegin(); pf != prevFuncs_.cend(); ++pf)
   {
      auto cf = currFuncs_.find(pf->first);
      if(cf == currFuncs_.cend()) continue;

      auto& tests = pf->second.tests;

      for(auto pt = tests.cbegin(); pt != tests.cend(); ++pt)
      {
         if(inclTests.find(*pt) != inclTests.cend())
            cf->second.tests.insert(*pt);
      }
   }

   if(!Commit(currFuncs_))
   {
      expl << CreateStreamFailure;
      return -7;
   }

   //  The latest functions and tests have now been included, so the current
   //  database is now also the "previous" one.
   //
   prevFuncs_ = currFuncs_;
   prevTests_ = currTests_;
   expl << SuccessExpl;
   return 0;
}

//------------------------------------------------------------------------------

word CodeCoverage::Query(string& expl)
{
   Debug::ft("CodeCoverage.Query");

   if(prevFuncs_.empty())
   {
      auto rc = Load(expl);
      if(rc != 0) return rc;
   }

   std::ostringstream stats;

   stats << "Functions: " << prevFuncs_.size() << CRLF;
   stats << "Tests per function:" << CRLF;

   const size_t MAX_TESTS = 10;
   size_t histogram[MAX_TESTS + 1] = { 0 };

   for(auto f = prevFuncs_.cbegin(); f != prevFuncs_.cend(); ++f)
   {
      if(f->second.tests.size() > MAX_TESTS)
         histogram[MAX_TESTS]++;
      else
         histogram[f->second.tests.size()]++;
   }

   for(auto i = 0; i < MAX_TESTS; ++i)
   {
      stats << setw(6) << i;
   }

   stats << setw(5) << MAX_TESTS << '+' << CRLF;

   for(auto i = 0; i <= MAX_TESTS; ++i)
   {
      stats << setw(6) << histogram[i];
   }

   expl = stats.str();
   return 0;
}

//------------------------------------------------------------------------------

word CodeCoverage::Report
   (word rc, const std::ostringstream& stream, string& expl)
{
   Debug::ft("CodeCoverage.Report");

   expl = stream.str();
   return rc;
}

//------------------------------------------------------------------------------

word CodeCoverage::Retest(std::ostringstream& expl) const
{
   Debug::ft("CodeCoverage.Retest");

   std::vector< Functions::const_iterator > modified;

   for(auto c = currFuncs_.cbegin(); c != currFuncs_.cend(); ++c)
   {
      if(prevFuncs_.find(c->first) == prevFuncs_.cend())
      {
         modified.push_back(c);
      }
   }

   for(auto c = currFuncs_.cbegin(); c != currFuncs_.cend(); ++c)
   {
      auto p = prevFuncs_.find(c->first);

      if((p != prevFuncs_.cend()) && (c->second.hash != p->second.hash))
      {
         modified.push_back(c);
      }
   }

   for(auto p = prevFuncs_.cbegin(); p != prevFuncs_.cend(); ++p)
   {
      if((p->second.hash != UNHASHED) &&
         (currFuncs_.find(p->first) == currFuncs_.cend()))
      {
         modified.push_back(p);
      }
   }

   if(modified.empty())
   {
      expl << "No functions require retesting.";
      return 0;
   }

   std::set< string > reexecute;
   std::set< string > uncovered;
   std::set< string > unknown;

   for(auto f = modified.cbegin(); f != modified.cend(); ++f)
   {
      auto& ft = (*f)->second.tests;

      if(ft.empty())
         uncovered.insert((*f)->first);
      else
         for(auto t = ft.cbegin(); t != ft.cend(); ++t) reexecute.insert(*t);
   }

   auto testdb = Singleton< TestDatabase >::Instance();
   std::ostringstream report;

   if(!reexecute.empty())
   {
      report << "Tests to re-execute for modified functions:";

      for(auto t = reexecute.cbegin(); t != reexecute.cend(); ++t)
      {
         if(testdb->SetState(*t, TestDatabase::Reexecute))
            report << CRLF << spaces(2) << *t;
         else
            unknown.insert(*t);
      }
   }

   if(!unknown.empty())
   {
      report << CRLF << "The following tests were not found in the database:";

      for(auto t = unknown.cbegin(); t != unknown.cend(); ++t)
      {
         report << CRLF << spaces(2) << *t;
      }
   }

   if(!uncovered.empty())
   {
      report << CRLF << "No tests exist for these modified functions:";

      for(auto f = uncovered.cbegin(); f != uncovered.cend(); ++f)
      {
         report << CRLF << spaces(2) << Demangle(*f);
      }
   }

   expl << report.str();
   return 0;
}

//------------------------------------------------------------------------------

void CodeCoverage::Shutdown(RestartLevel level)
{
   Debug::ft("CodeCoverage.Shutdown");

   prevFuncs_.clear();
   currFuncs_.clear();
   prevTests_.clear();
   currTests_.clear();
}

//------------------------------------------------------------------------------

word CodeCoverage::Under(size_t min, string& expl)
{
   Debug::ft("CodeCoverage.Under");

   expl.clear();

   if(prevFuncs_.empty())
   {
      auto rc = Load(expl);
      if(rc != 0) return rc;
   }

   for(auto f = prevFuncs_.cbegin(); f != prevFuncs_.cend(); ++f)
   {
      if(f->second.tests.size() < min)
      {
         expl.append(Demangle(f->first) + CRLF);
      }
   }

   if(expl.empty())
      expl = "No such functions found.";
   else
      expl.pop_back();

   return 0;
}

//------------------------------------------------------------------------------

word CodeCoverage::Update(string& expl)
{
   Debug::ft("CodeCoverage.Update");

   std::ostringstream stream;

   stream << "Importing previous database..." << CRLF;
   auto rc = Load(expl);
   stream << spaces(2) << expl;
   expl.clear();
   if(rc != 0) return Report(rc, stream, expl);
   stream << CRLF;

   stream << "Including OutputPath/*.funcs.txt files..." << CRLF << spaces(2);
   rc = Build(stream);
   if(rc != 0) return Report(rc, stream, expl);
   stream << CRLF;

   rc = Diff(stream);
   if(rc != 0) return Report(rc, stream, expl);
   stream << CRLF;

   rc = Retest(stream);
   if(rc != 0) return Report(rc, stream, expl);
   stream << CRLF;

   stream << "Exporting updated database..." << CRLF << spaces(2);
   rc = Merge(stream);
   return Report(rc, stream, expl);
}
}
