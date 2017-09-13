//==============================================================================
//
//  CfgParmRegistry.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "CfgParmRegistry.h"
#include <cstdio>
#include <cstring>
#include <istream>
#include <sstream>
#include "CfgParm.h"
#include "CfgTuple.h"
#include "Debug.h"
#include "Formatters.h"
#include "Log.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
fixed_string CfgParmRegistry::BackFromExePath = "rsc\\rsc\\";
fixed_string CfgParmRegistry::AppendToExePath = "/input/element.config.txt";

//------------------------------------------------------------------------------

fn_name CfgParmRegistry_ctor = "CfgParmRegistry.ctor";

CfgParmRegistry::CfgParmRegistry() : currLine_(0)
{
   Debug::ft(CfgParmRegistry_ctor);

   mainArgs_.reset(new std::vector< stringPtr >());
   configFileName_.reset(new string("element.config.txt"));
   tupleq_.Init(CfgTuple::LinkDiff());
   parmq_.Init(CfgParm::LinkDiff());
}

//------------------------------------------------------------------------------

fn_name CfgParmRegistry_dtor = "CfgParmRegistry.dtor";

CfgParmRegistry::~CfgParmRegistry()
{
   Debug::ft(CfgParmRegistry_dtor);

   //  This should not be invoked.  On a reload restart, all configuration
   //  parameters and tuples should be freed together, when the protected
   //  heap is deallocated.
   //
   Debug::SwErr(CfgParmRegistry_dtor, 0, 0);
}

//------------------------------------------------------------------------------

fn_name CfgParmRegistry_AddMainArg = "CfgParmRegistry.AddMainArg";

void CfgParmRegistry::AddMainArg(const string& arg)
{
   Debug::ft(CfgParmRegistry_AddMainArg);

   mainArgs_->push_back(stringPtr(new string(arg)));

   if(mainArgs_->size() == 1)
   {
      *configFileName_ = arg;

      auto pos = configFileName_->rfind(BackFromExePath);

      if(pos != string::npos)
      {
         pos += strlen(BackFromExePath);
         configFileName_->erase(pos);
      }

      configFileName_->append(AppendToExePath);
   }
}

//------------------------------------------------------------------------------

fn_name CfgParmRegistry_BadLine = "CfgParmRegistry.BadLine";

void CfgParmRegistry::BadLine(fixed_string reason, const string& input) const
{
   Debug::ft(CfgParmRegistry_BadLine);

   auto log = Log::Create(reason);

   if(log != nullptr)
   {
      *log << "errval=" << input << " line=" << currLine_ << CRLF;
      Log::Spool(log);
   }
}

//------------------------------------------------------------------------------

fn_name CfgParmRegistry_BindParm = "CfgParmRegistry.BindParm";

bool CfgParmRegistry::BindParm(CfgParm& parm)
{
   Debug::ft(CfgParmRegistry_BindParm);

   auto key0 = strLower(parm.Key());

   //  Register parameters by key, in alphabetical order.
   //
   CfgParm* prev = nullptr;

   for(auto next = parmq_.First(); next != nullptr; parmq_.Next(next))
   {
      auto key1 = strLower(next->Key());

      if(key0 < key1) break;

      if(key0 == key1)
      {
         Debug::SwErr(CfgParmRegistry_BindParm, key0, 0);
         return false;
      }

      prev = next;
   }

   parmq_.Insert(prev, parm);
   parm.SetFromTuple();
   return true;
}

//------------------------------------------------------------------------------

fn_name CfgParmRegistry_BindTuple = "CfgParmRegistry.BindTuple";

bool CfgParmRegistry::BindTuple(CfgTuple& tuple)
{
   Debug::ft(CfgParmRegistry_BindTuple);

   CfgTuple* prev = nullptr;
   auto key0 = strLower(tuple.Key());

   //  Register tuples by key, in alphabetical order.
   //
   for(auto next = tupleq_.First(); next != nullptr; tupleq_.Next(next))
   {
      auto key1 = strLower(next->Key());

      if(key0 < key1) break;

      if(key0 == key1)
      {
         auto log = Log::Create("CFGPARM KEY IN USE");

         if(log != nullptr)
         {
            *log << "errval=" << key0 << CRLF;
            Log::Spool(log);
         }

         return false;
      }

      prev = next;
   }

   tupleq_.Insert(prev, tuple);
   return true;
}

//------------------------------------------------------------------------------

void CfgParmRegistry::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Protected::Display(stream, prefix, options);

   stream << prefix << "mainArgs       : " << CRLF;
   for(size_t i = 0; i < mainArgs_->size(); ++i)
   {
      stream << spaces(2) << strIndex(i) << *mainArgs_->at(i) << CRLF;
   }

   stream << prefix << "configFileName : " << *configFileName_ << CRLF;
   stream << prefix << "stream         : " << stream_.get() << CRLF;
   stream << prefix << "currLine       : " << currLine_ << CRLF;
   stream << prefix << "tupleq         : " << CRLF;
   tupleq_.Display(stream, prefix + spaces(2), options);
   stream << prefix << "parmq          : " << CRLF;
   parmq_.Display(stream, prefix + spaces(2), options);
}

//------------------------------------------------------------------------------

fn_name CfgParmRegistry_FindParm = "CfgParmRegistry.FindParm";

CfgParm* CfgParmRegistry::FindParm(const string& key) const
{
   Debug::ft(CfgParmRegistry_FindParm);

   auto k = strLower(key);

   //  Find the parameter that matches KEY.
   //
   for(auto p = parmq_.First(); p != nullptr; parmq_.Next(p))
   {
      if(k == strLower(p->Key())) return p;
   }

   return nullptr;
}

//------------------------------------------------------------------------------

fn_name CfgParmRegistry_FindTuple = "CfgParmRegistry.FindTuple";

CfgTuple* CfgParmRegistry::FindTuple(const string& key) const
{
   Debug::ft(CfgParmRegistry_FindTuple);

   auto k = strLower(key);

   //  Find the tuple that matches KEY.
   //
   for(auto t = tupleq_.First(); t != nullptr; tupleq_.Next(t))
   {
      if(k == strLower(t->Key())) return t;
   }

   return nullptr;
}

//------------------------------------------------------------------------------

fn_name CfgParmRegistry_GetValue = "CfgParmRegistry.GetValue";

bool CfgParmRegistry::GetValue(const string& key, string& value) const
{
   Debug::ft(CfgParmRegistry_GetValue);

   //  Find the parameter that matches KEY and return its value.
   //
   auto key0 = strLower(key);
   auto p = FindParm(key0);

   if(p != nullptr)
   {
      value = p->GetCurr();
      return true;
   }

   return false;
}

//------------------------------------------------------------------------------

void CfgParmRegistry::ListParms(ostream& stream, const string& prefix) const
{
   for(auto p = parmq_.First(); p != nullptr; parmq_.Next(p))
   {
      auto s = p->GetCurr();
      stream << prefix << p->Key() << ": " << s << CRLF;
   }
}

//------------------------------------------------------------------------------

fn_name CfgParmRegistry_LoadNextTuple = "CfgParmRegistry.LoadNextTuple";

bool CfgParmRegistry::LoadNextTuple(string& key, string& value)
{
   Debug::ft(CfgParmRegistry_LoadNextTuple);

   string input;
   size_t keyBeg, keyEnd, valBeg, valEnd, extra;

   while(stream_->peek() != EOF)
   {
      //  Read lines from the configuration file until EOF is reached.
      //  Skip any line that is empty, that contains only blanks, or that
      //  has the comment character as its first non-blank character.
      //
      std::getline(*stream_, input);
      ++currLine_;

      if(input.empty()) continue;

      keyBeg = input.find_first_not_of(CfgTuple::ValidBlankChars());

      if(keyBeg == string::npos) continue;
      if(input[keyBeg] == CfgTuple::CommentChar) continue;

      //  The next key begins at input[keyBeg].  See where it ends.
      //
      keyEnd = input.find_first_not_of(CfgTuple::ValidNameChars(), keyBeg);

      if(keyEnd == string::npos)
      {
         BadLine("CFGPARM VALUE ABSENT", input);  // line only contains a key
         continue;
      }

      if(keyEnd == keyBeg)
      {
         BadLine("CFGPARM KEY INVALID", input);  // first character is invalid
         continue;
      }

      //  We have a key.  Now look for a value.
      //
      key = input.substr(keyBeg, keyEnd - keyBeg);

      valBeg = input.find_first_not_of(CfgTuple::ValidBlankChars(), keyEnd);

      if(valBeg == string::npos)
      {
         BadLine("CFGPARM VALUE ABSENT", input);  // line only contains a key
         continue;
      }

      valEnd = input.find_first_not_of(CfgTuple::ValidValueChars(), valBeg);

      if(valEnd == string::npos)
      {
         value = input.substr(valBeg);  // value occupies rest of line
         return true;
      }

      if(valEnd == valBeg)
      {
         BadLine("CFGPARM VALUE INVALID", input);  // first character invalid
         continue;
      }

      //  We have a value, but other stuff follows it.  That's OK as long
      //  as the trailing stuff only consists of blanks or a comment.
      //
      value = input.substr(valBeg, valEnd - valBeg);
      extra = input.find_first_not_of(CfgTuple::ValidBlankChars(), valEnd);
      if(extra == string::npos) return true;
      if(input[extra] == CfgTuple::CommentChar) return true;

      BadLine("CFGPARM EXTRA INPUT IGNORED", input.substr(extra));
      return true;
   }

   return false;
}

//------------------------------------------------------------------------------

fn_name CfgParmRegistry_LoadTuples = "CfgParmRegistry.LoadTuples";

void CfgParmRegistry::LoadTuples()
{
   Debug::ft(CfgParmRegistry_LoadTuples);

   string key;
   string value;

   //  Open the file that contains tuples for configuration parameters.
   //  Find its key-value pairs and create a tuple for each.  If a tuple
   //  with that key already exists, update its value so that the parameter
   //  can later be set to the value specified in the configuration file.
   //
   stream_ = SysFile::CreateIstream(configFileName_->c_str());

   if(stream_ == nullptr)
   {
      auto log = Log::Create("CFGPARM FILE NOT FOUND");

      if(log != nullptr)
      {
         *log << "path=" << *configFileName_ << CRLF;
      }

      Log::Spool(log);
      return;
   }

   currLine_ = 0;

   while(LoadNextTuple(key, value))
   {
      auto tuple = FindTuple(key);

      if(tuple != nullptr)
      {
         tuple->SetInput(value);
      }
      else
      {
         tuple = new CfgTuple(key, value);
         BindTuple(*tuple);
      }
   }

   stream_.reset();

   //  If a configuration parameter was registered *before* its tuple in the
   //  configuration file was loaded, ensure that its value matches the value
   //  now specified by the configuration file.
   //
   for(auto p = parmq_.First(); p != nullptr; parmq_.Next(p))
   {
      p->SetFromTuple();
   }
}

//------------------------------------------------------------------------------

void CfgParmRegistry::Patch(sel_t selector, void* arguments)
{
   Protected::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name CfgParmRegistry_Startup = "CfgParmRegistry.Startup";

void CfgParmRegistry::Startup(RestartLevel level)
{
   Debug::ft(CfgParmRegistry_Startup);

   //  Load configuration parameters when booting or during a reload restart.
   //  During less severe restarts, update any configuration parameters whose
   //  new value could only be assigned during a restart of this severity.
   //
   if(level >= RestartReload)
   {
      LoadTuples();
   }
   else
   {
      for(auto p = parmq_.First(); p != nullptr; parmq_.Next(p))
      {
         if((p->level_ != RestartNil) && (p->level_ <= level))
         {
            p->SetCurr();
         }
      }
   }
}

//------------------------------------------------------------------------------

fn_name CfgParmRegistry_UnbindParm = "CfgParmRegistry.UnbindParm";

void CfgParmRegistry::UnbindParm(CfgParm& parm)
{
   Debug::ft(CfgParmRegistry_UnbindParm);

   parmq_.Exq(parm);
}

//------------------------------------------------------------------------------

fn_name CfgParmRegistry_UnbindTuple = "CfgParmRegistry.UnbindTuple";

void CfgParmRegistry::UnbindTuple(CfgTuple& tuple)
{
   Debug::ft(CfgParmRegistry_UnbindTuple);

   tupleq_.Exq(tuple);
}
}
