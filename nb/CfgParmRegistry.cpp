//==============================================================================
//
//  CfgParmRegistry.cpp
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
#include "CfgParmRegistry.h"
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <istream>
#include <sstream>
#include "CfgParm.h"
#include "CfgTuple.h"
#include "Debug.h"
#include "Formatters.h"
#include "Log.h"
#include "MainArgs.h"
#include "NbLogs.h"
#include "Restart.h"
#include "SysFile.h"
#include "SysTypes.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
//  A handle for reading the configuration file.
//
istreamPtr Stream_= nullptr;

//  The current line number in the configuration file.
//
size_t CurrLine_ = 0;

//> Used to derive the name of the file that contains this node's configuration
//  parameters.  It is created by modifying the first argument to main(), which
//  is the path to our executable, as follows:
//  o find the last occurrence of BackFromExePath and erase what *follows*
//    it (that is, retain BackFromExePath as a "suffix"), and then
//  o append AppendToExePath.
//
fixed_string BackFromExePath_ = "rsc/";
fixed_string AppendToExePath_ = "input/element.config.txt";

//------------------------------------------------------------------------------
//
//  Called by LoadNextTuple to flag invalid entries in ConfigFileName.
//  ID identifies the problem, and INPUT is the invalid entry.
//
void BadLine(LogId id, const string& input)
{
   Debug::ft("NodeBase.BadLine");

   auto log = Log::Create(ConfigLogGroup, id);

   if(log != nullptr)
   {
      *log << Log::Tab << "errval=" << input << " line=" << CurrLine_;
      Log::Submit(log);
   }
}

//==============================================================================

CfgParmRegistry::CfgParmRegistry()
{
   Debug::ft("CfgParmRegistry.ctor");

   tupleq_.Init(CfgTuple::LinkDiff());
   parmq_.Init(CfgParm::LinkDiff());

   string exe(MainArgs::At(0));
   SysFile::Normalize(exe);
   configFileName_ = exe.c_str();

   auto pos = configFileName_.rfind(BackFromExePath_);

   if(pos != string::npos)
      pos += strlen(BackFromExePath_);
   else
      pos = configFileName_.rfind('/') + 1;

   configFileName_.erase(pos);
   configFileName_.append(AppendToExePath_);
}

//------------------------------------------------------------------------------

fn_name CfgParmRegistry_dtor = "CfgParmRegistry.dtor";

CfgParmRegistry::~CfgParmRegistry()
{
   Debug::ftnt(CfgParmRegistry_dtor);

   Debug::SwLog(CfgParmRegistry_dtor, UnexpectedInvocation, 0);
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
         Debug::SwLog(CfgParmRegistry_BindParm, key0, 0);
         return false;
      }

      prev = next;
   }

   parmq_.Insert(prev, parm);
   parm.SetFromTuple();
   return true;
}

//------------------------------------------------------------------------------

bool CfgParmRegistry::BindTuple(CfgTuple& tuple)
{
   Debug::ft("CfgParmRegistry.BindTuple");

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
         auto log = Log::Create(ConfigLogGroup, ConfigKeyInUse);

         if(log != nullptr)
         {
            *log << Log::Tab << "errval=" << key0;
            Log::Submit(log);
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

   stream << prefix << "configFileName : " << configFileName_ << CRLF;
   stream << prefix << "tupleq : " << CRLF;
   tupleq_.Display(stream, prefix + spaces(2), options);
   stream << prefix << "parmq : " << CRLF;
   parmq_.Display(stream, prefix + spaces(2), options);
}

//------------------------------------------------------------------------------

CfgParm* CfgParmRegistry::FindParm(const string& key) const
{
   Debug::ft("CfgParmRegistry.FindParm");

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

CfgTuple* CfgParmRegistry::FindTuple(const string& key) const
{
   Debug::ft("CfgParmRegistry.FindTuple");

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

bool CfgParmRegistry::GetValue(const string& key, string& value) const
{
   Debug::ft("CfgParmRegistry.GetValue");

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

bool CfgParmRegistry::LoadNextTuple(string& key, string& value)
{
   Debug::ft("CfgParmRegistry.LoadNextTuple");

   string input;

   while(Stream_->peek() != EOF)
   {
      //  Read lines from the configuration file until EOF is reached.
      //  Skip any line that is empty, that contains only blanks, or that
      //  has the comment character as its first non-blank character.
      //
      std::getline(*Stream_, input);
      ++CurrLine_;

      if(input.empty()) continue;

      auto keyBeg = input.find_first_not_of(CfgTuple::ValidBlankChars());

      if(keyBeg == string::npos) continue;
      if(input[keyBeg] == CfgTuple::CommentChar) continue;

      //  The next key begins at input[keyBeg].  See where it ends.
      //
      auto keyEnd = input.find_first_not_of(CfgTuple::ValidKeyChars(), keyBeg);

      if(keyEnd == string::npos)
      {
         BadLine(ConfigValueMissing, input);  // line only contains a key
         continue;
      }

      if(keyEnd == keyBeg)
      {
         BadLine(ConfigKeyInvalid, input);  // first character invalid
         continue;
      }

      //  We have a key.  Now look for a value.
      //
      key = input.substr(keyBeg, keyEnd - keyBeg);

      auto valBeg =
         input.find_first_not_of(CfgTuple::ValidBlankChars(), keyEnd);

      if(valBeg == string::npos)
      {
         BadLine(ConfigValueMissing, input);  // line only contains a key
         continue;
      }

      auto valEnd =
         input.find_first_not_of(CfgTuple::ValidValueChars(), valBeg);

      if(valEnd == string::npos)
      {
         value = input.substr(valBeg);  // value occupies rest of line
         return true;
      }

      if(valEnd == valBeg)
      {
         BadLine(ConfigValueInvalid, input);  // first character invalid
         continue;
      }

      //  We have a value, but other stuff follows it.  That's OK as long
      //  as the trailing stuff only consists of blanks or a comment.
      //
      value = input.substr(valBeg, valEnd - valBeg);

      auto extra = input.find_first_not_of(CfgTuple::ValidBlankChars(), valEnd);
      if(extra == string::npos) return true;
      if(input[extra] == CfgTuple::CommentChar) return true;

      BadLine(ConfigExtraIgnored, input.substr(extra));
      return true;
   }

   return false;
}

//------------------------------------------------------------------------------

void CfgParmRegistry::LoadTuples()
{
   Debug::ft("CfgParmRegistry.LoadTuples");

   string key;
   string value;

   //  Open the file that contains tuples for configuration parameters.
   //  Find its key-value pairs and create a tuple for each.  If a tuple
   //  with that key already exists, update its value so that the parameter
   //  can later be set to the value specified in the configuration file.
   //
   Stream_ = SysFile::CreateIstream(configFileName_.c_str());

   if(Stream_ == nullptr)
   {
      auto log = Log::Create(ConfigLogGroup, ConfigFileNotFound);

      if(log != nullptr)
      {
         *log << Log::Tab << "path=" << configFileName_;
         Log::Submit(log);
      }

      return;
   }

   CurrLine_ = 0;

   while(LoadNextTuple(key, value))
   {
      auto tuple = FindTuple(key);

      if(tuple != nullptr)
      {
         tuple->SetInput(value.c_str());
      }
      else
      {
         tuple = new CfgTuple(key.c_str(), value.c_str());
         BindTuple(*tuple);
      }
   }

   Stream_.reset();

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

void CfgParmRegistry::Startup(RestartLevel level)
{
   Debug::ft("CfgParmRegistry.Startup");

   //  Load configuration parameters if the registry was created.  If the
   //  registry survived a restart, update any configuration parameters whose
   //  new value could only be assigned during a restart of this severity.
   //
   if(Restart::ClearsMemory(MemType()))
   {
      LoadTuples();
   }
   else
   {
      for(auto p = parmq_.First(); p != nullptr; parmq_.Next(p))
      {
         if((p->level_ != RestartNone) && (p->level_ <= level))
         {
            p->SetCurr();
         }
      }
   }
}

//------------------------------------------------------------------------------

void CfgParmRegistry::UnbindParm(CfgParm& parm)
{
   Debug::ftnt("CfgParmRegistry.UnbindParm");

   parmq_.Exq(parm);
}

//------------------------------------------------------------------------------

void CfgParmRegistry::UnbindTuple(CfgTuple& tuple)
{
   Debug::ftnt("CfgParmRegistry.UnbindTuple");

   tupleq_.Exq(tuple);
}
}
