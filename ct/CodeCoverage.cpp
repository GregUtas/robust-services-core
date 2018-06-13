//==============================================================================
//
//  CodeCoverage.cpp
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
fn_name CodeCoverage_ctor = "CodeCoverage.ctor";

CodeCoverage::CodeCoverage()
{
   Debug::ft(CodeCoverage_ctor);
}

//------------------------------------------------------------------------------

fn_name CodeCoverage_dtor = "CodeCoverage.dtor";

CodeCoverage::~CodeCoverage()
{
   Debug::ft(CodeCoverage_dtor);
}

//------------------------------------------------------------------------------

fn_name CodeCoverage_Build = "CodeCoverage.Build";

word CodeCoverage::Build(std::ostringstream& expl)
{
   Debug::ft(CodeCoverage_Build);

   if(currFuncs_.empty())
   {
      expl << "Must run >check on all code files before >coverage build.";
      return -1;
   }

   auto testdb = Singleton< TestDatabase >::Instance();

   FunctionGuard guard(FunctionGuard::MakePreemptable);

   //  Find all *.funcs.txt files in the output directory.  If a testcase
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

      //  Extract the function names from STREAM.  They begin on the fourth
      //  line, and each one is preceded by two integers.
      //
      string input;
      string str;
      int discard = -4;

      while(stream->peek() != EOF)
      {
         std::getline(*stream, input);
         if(discard++ < 0) continue;

         str = strGet(input);
         if(str.empty() || !isdigit(str.front())) break;
         str = strGet(input);
         if(str.empty() || !isdigit(str.front())) break;
         str = strGet(input);
         if(str.empty()) break;

         //  If INPUT isn't empty, append it to STR.  The function name
         //  contains an embedded space (and might have more of them).
         //  Mangle replace spaces with "BLANK" to simplify Load.
         //
         if(!input.empty())
         {
            str += Mangle(input);
            input.clear();
         }

         auto func = currFuncs_.find(str);

         if(func == currFuncs_.cend())
         {
            //  This function was not in the database.  This occurs in the
            //  case of a function template or function in a class template,
            //  since these are not parsed.  Add the function.
            //
            auto info = FuncInfo(UNHASHED);
            auto result = currFuncs_.insert(FuncData(str, info));
            func = result.first;
         }

         func->second.tests.insert(*trace);
      }
   }

   expl << count << " *.funcs.txt file(s) processed";
   return 0;
}

//------------------------------------------------------------------------------

fn_name CodeCoverage_Commit = "CodeCoverage.Commit";

bool CodeCoverage::Commit(const Functions& funcs)
{
   Debug::ft(CodeCoverage_Commit);

   FunctionGuard guard(FunctionGuard::MakePreemptable);

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

string CodeCoverage::Demangle(const string& s)
{
   string result(s);

   for(auto i = 0; i < result.size(); ++i)
   {
      if(result[i] == BLANK) result[i] = SPACE;
   }

   return result;
}

//------------------------------------------------------------------------------

fn_name CodeCoverage_Diff = "CodeCoverage.Diff";

word CodeCoverage::Diff(std::ostringstream& expl) const
{
   Debug::ft(CodeCoverage_Diff);

   expl << "Added functions: ";
   auto found = false;

   for(auto c = currFuncs_.cbegin(); c != currFuncs_.cend(); ++c)
   {
      if(prevFuncs_.find(c->first) == prevFuncs_.cend())
      {
         expl << CRLF << spaces(2) << c->first;
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
         expl << CRLF << spaces(2) << c->first;
         found = true;
      }
   }

   if(!found) expl << "none";
   expl << CRLF << "Deleted functions: ";
   found = false;

   for(auto p = prevFuncs_.cbegin(); p!= prevFuncs_.cend(); ++p)
   {
      if((p->second.hash != UNHASHED) &&
         (currFuncs_.find(p->first) == currFuncs_.cend()))
      {
         expl << CRLF << spaces(2) << p->first;
         found = true;
      }
   }

   if(!found) expl << "none";
   return 0;
}

//------------------------------------------------------------------------------

fn_name CodeCoverage_Erase = "CodeCoverage.Erase";

word CodeCoverage::Erase(string& func, string& expl)
{
   Debug::ft(CodeCoverage_Erase);

   if(prevFuncs_.empty())
   {
      auto rc = Load(expl);
      if(rc != 0) return rc;
   }

   func = Mangle(func);
   auto found = prevFuncs_.erase(func);

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

fn_name CodeCoverage_GetError = "CodeCoverage.GetError";

CodeCoverage::LoadState CodeCoverage::GetError
   (const string& reason, word& rc, string& expl)
{
   Debug::ft(CodeCoverage_GetError);

   expl = reason;
   rc = -1;
   return LoadError;
}

//------------------------------------------------------------------------------

fn_name CodeCoverage_GetFunc = "CodeCoverage.GetFunc";

CodeCoverage::LoadState CodeCoverage::GetFunc
   (string& input, word& rc, string& expl)
{
   Debug::ft(CodeCoverage_GetFunc);

   auto func = strGet(input);
   if(func.empty()) return GetFunction;
   if(func.front() == DELIMITER) return LoadDone;
   auto hash = strGet(input);
   if(hash.empty() || !isxdigit(hash.front()))
      return GetError("Hash value for function missing", rc, expl);
   uint32_t n = std::stoul(hash, nullptr, 16);
   auto info = FuncInfo(n);
   auto result = prevFuncs_.insert(FuncData(func, info));
   if(!result.second)
      return GetError("Function name duplicated", rc, expl);
   loadFunc_ = result.first;
   return GetTestcases;
}

//------------------------------------------------------------------------------

fn_name CodeCoverage_GetTests = "CodeCoverage.GetTests";

CodeCoverage::LoadState CodeCoverage::GetTests(string& input)
{
   Debug::ft(CodeCoverage_GetTests);

   auto test = strGet(input);
   if(test.empty()) return GetTestcases;
   if(test.front() == DELIMITER) return GetFunction;
   loadFunc_->second.tests.insert(test);
   prevTests_.insert(test);
   return GetTestcases;
}

//------------------------------------------------------------------------------

fn_name CodeCoverage_Insert = "CodeCoverage.Insert";

bool CodeCoverage::Insert(const string& func, uint32_t hash, const string& file)
{
   Debug::ft(CodeCoverage_Insert);

   auto name = Mangle(func);
   auto iter = currFuncs_.find(name);

   if(iter != currFuncs_.cend())
   {
      return (iter->second.file == file);
   }

   auto info = FuncInfo(file, hash);
   auto result = currFuncs_.insert(FuncData(name, info));
   return result.second;
}

//------------------------------------------------------------------------------

fn_name CodeCoverage_Load = "CodeCoverage.Load";

word CodeCoverage::Load(string& expl)
{
   Debug::ft(CodeCoverage_Load);

   FunctionGuard guard(FunctionGuard::MakePreemptable);

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
   auto state = GetFunction;
   prevFuncs_.clear();
   prevTests_.clear();

   while(stream->peek() != EOF)
   {
      std::getline(*stream, input);

      while(!input.empty())
      {
         switch(state)
         {
         case GetFunction:
            state = GetFunc(input, rc, expl);
            break;
         case GetTestcases:
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

   for(auto i = 0; i < result.size(); ++i)
   {
      if(result[i] == SPACE) result[i] = BLANK;
   }

   return result;
}

//------------------------------------------------------------------------------

fn_name CodeCoverage_Merge = "CodeCoverage.Merge";

word CodeCoverage::Merge(std::ostringstream& expl)
{
   Debug::ft(CodeCoverage_Merge);

   auto testdb = Singleton< TestDatabase >::Instance();
   std::set< string > inclTests;

   //  Update the current database with testcases that appear only in
   //  the previous database.  Verify that a testcase is still in the
   //  testcase database before adding it.
   //
   for(auto p = prevTests_.cbegin(); p != prevTests_.cend(); ++p)
   {
      if((currTests_.find(*p) == currTests_.cend()) &
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

   //  Look at functions that exist in both databases.  If a testcase invoked
   //  a function in the previous database, insert it as an invoker of that
   //  function in the current database *if the testcase was just added to the
   //  current database, above*.  (If the testcase wasn't added to the current
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

   //  The latest functions and testcases have now been included, so the
   //  current database is now also the "previous" one.
   //
   prevFuncs_ = currFuncs_;
   prevTests_ = currTests_;
   expl << SuccessExpl;
   return 0;
}

//------------------------------------------------------------------------------

fn_name CodeCoverage_Query = "CodeCoverage.Query";

word CodeCoverage::Query(string& expl)
{
   Debug::ft(CodeCoverage_Query);

   if(prevFuncs_.empty())
   {
      auto rc = Load(expl);
      if(rc != 0) return rc;
   }

   std::ostringstream stats;

   stats << "Functions: " << prevFuncs_.size() << CRLF;
   stats << "Testcases per function:" << CRLF;

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

fn_name CodeCoverage_Report = "CodeCoverage.Report";

word CodeCoverage::Report
   (word rc, const std::ostringstream& stream, string& expl)
{
   Debug::ft(CodeCoverage_Report);

   expl = stream.str();
   return rc;
}

//------------------------------------------------------------------------------

fn_name CodeCoverage_Retest = "CodeCoverage.Retest";

word CodeCoverage::Retest(std::ostringstream& expl) const
{
   Debug::ft(CodeCoverage_Retest);

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

   for(auto p = prevFuncs_.cbegin(); p!= prevFuncs_.cend(); ++p)
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

   std::set< string > testless;
   std::set< string > tests;

   for(auto f = modified.cbegin(); f != modified.cend(); ++f)
   {
      auto& ft = (*f)->second.tests;

      if(ft.empty())
         testless.insert((*f)->first);
      else
         for(auto t = ft.cbegin(); t != ft.cend(); ++t) tests.insert(*t);
   }

   auto testdb = Singleton< TestDatabase >::Instance();
   std::ostringstream report;

   if(!tests.empty())
   {
      report << "Testcases to re-execute for modified functions:";

      for(auto t = tests.cbegin(); t != tests.cend(); ++t)
      {
         report << CRLF << spaces(2) << *t;
         testdb->SetState(*t, TestDatabase::Reexecute);
      }
   }

   if(!testless.empty())
   {
      if(!tests.empty()) report << CRLF;
      report << "No testcases exist for these modified functions:";

      for(auto f = testless.cbegin(); f != testless.cend(); ++f)
      {
         report << CRLF << spaces(2) << *f;
      }
   }

   expl << report.str();
   return 0;
}

//------------------------------------------------------------------------------

fn_name CodeCoverage_Shutdown = "CodeCoverage.Shutdown";

void CodeCoverage::Shutdown(RestartLevel level)
{
   Debug::ft(CodeCoverage_Shutdown);

   prevFuncs_.clear();
   currFuncs_.clear();
   prevTests_.clear();
   currTests_.clear();
}

//------------------------------------------------------------------------------

fn_name CodeCoverage_Under = "CodeCoverage.Under";

word CodeCoverage::Under(size_t min, string& expl)
{
   Debug::ft(CodeCoverage_Under);

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
         expl.append(f->first + CRLF);
      }
   }

   if(expl.empty())
      expl = "No such functions found.";
   else
      expl.pop_back();

   return 0;
}

//------------------------------------------------------------------------------

fn_name CodeCoverage_Update = "CodeCoverage.Update";

word CodeCoverage::Update(string& expl)
{
   Debug::ft(CodeCoverage_Update);

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