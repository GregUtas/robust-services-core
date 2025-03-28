//==============================================================================
//
//  CfgParmRegistry.cpp
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
#include "CfgParmRegistry.h"
#include <cstddef>
#include <cstdio>
#include <istream>
#include <sstream>
#include "CfgParm.h"
#include "CfgTuple.h"
#include "Debug.h"
#include "Element.h"
#include "FileSystem.h"
#include "Formatters.h"
#include "Log.h"
#include "MainArgs.h"
#include "NbLogs.h"
#include "Restart.h"
#include "SysTypes.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
//  A handle for reading the configuration file.
//
static istreamPtr Stream_ = nullptr;

//  The current line number in the configuration file.
//
static size_t CurrLine_ = 0;

//------------------------------------------------------------------------------
//
//  Called by LoadNextTuple to flag invalid entries in ConfigFileName.
//  ID identifies the problem, and INPUT is the invalid entry.
//
static void BadLine(LogId id, const string& input)
{
   Debug::ft("NodeBase.BadLine");

   auto log = Log::Create(ConfigLogGroup, id);

   if(log != nullptr)
   {
      *log << Log::Tab << "errval=" << input << " line=" << CurrLine_;
      Log::Submit(log);
   }
}

//------------------------------------------------------------------------------
//
//  Returns the path to the default configuration file.
//
static string DefaultConfigFile()
{
   Debug::ft("NodeBase.DefaultConfigFile");

   string filename(Element::InputPath().c_str());
   filename.push_back(PATH_SEPARATOR);
   filename.append("element.config.txt");
   return filename;
}

//------------------------------------------------------------------------------
//
//  Returns the path to the configuration file, if any, specified by a command
//  line parameter.
//
static string ExplicitConfigFile()
{
   Debug::ft("NodeBase.ExplicitConfigFile");

   return MainArgs::Find("c=");
}

//------------------------------------------------------------------------------
//
//  Returns the path to the configuration file, if any, specified by a command
//  line parameter, else the path to the default configuration file.
//
static string ConfigFile()
{
   Debug::ft("NodeBase.ConfigFile");

   auto filename = ExplicitConfigFile();
   if(!filename.empty()) return filename;
   return DefaultConfigFile();
}

//------------------------------------------------------------------------------
//
//  Called by LoadTuples to read the next tuple from the configuration
//  file.    Returns true if another valid tuple exists; updates KEY
//  and VALUE accordingly.  Returns false on EOF.  Calls BadLine to log
//  invalid entries, but continues to look for tuples.
//
static bool LoadNextTuple(string& key, string& value)
{
   Debug::ft("NodeBase.LoadNextTuple");

   string input;

   while(Stream_->peek() != EOF)
   {
      //  Read lines from the configuration file until EOF is reached.
      //  Skip any line that is empty, that contains only blanks, or that
      //  has the comment character as its first non-blank character.
      //
      FileSystem::GetLine(*Stream_, input);
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

      if(valEnd == valBeg)
      {
         BadLine(ConfigValueInvalid, input);  // first character invalid
         continue;
      }

      //  Make the value the rest of the input line and strip trailing blanks.
      //
      value = input.substr(valBeg, valEnd - valBeg);
      const auto& blanks = CfgTuple::ValidBlankChars();
      while(blanks.find(value.back()) != string::npos) value.pop_back();
      return true;
   }

   return false;
}

//------------------------------------------------------------------------------
//
//  Opens the configuration file.
//
static istreamPtr OpenConfigFile()
{
   Debug::ft("NodeBase.OpenConfigFile");

   auto filename = ExplicitConfigFile();

   if(!filename.empty())
   {
      auto file = FileSystem::CreateIstream(filename.c_str());
      if(file != nullptr) return file;
   }

   filename = DefaultConfigFile();
   auto file = FileSystem::CreateIstream(filename.c_str());
   return file;
}

//==============================================================================

CfgParmRegistry::CfgParmRegistry()
{
   Debug::ft("CfgParmRegistry.ctor");

   tupleq_.Init(CfgTuple::LinkDiff());
   parmq_.Init(CfgParm::LinkDiff());
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

   stream << prefix << "configFileName : " << ConfigFile() << CRLF;
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
   Stream_ = OpenConfigFile();

   if(Stream_ == nullptr)
   {
      auto log = Log::Create(ConfigLogGroup, ConfigFileNotFound);

      if(log != nullptr)
      {
         *log << Log::Tab << "path=" << ConfigFile();
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
