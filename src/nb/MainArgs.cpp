//==============================================================================
//
//  MainArgs.cpp
//
//  Copyright (C) 2013-2022  Greg Utas
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
#include "MainArgs.h"
#include <cstring>
#include <ostream>
#include "Debug.h"
#include "Formatters.h"
#include "Singleton.h"

using std::ostream;
using std::string;

namespace NodeBase
{
//  For holding the arguments to main() until immutable memory is available.
//
static std::vector<string> MainParms;

//------------------------------------------------------------------------------

MainArgs::MainArgs()
{
   Debug::ft("MainArgs.ctor");

   //  Copy main()'s arguments from their temporary to their permanent location.
   //
   for(size_t i = 0; i < MainParms.size(); ++i)
   {
      ImmutableStr str(MainParms.at(i).c_str());
      args_.push_back(str);
   }
}

//------------------------------------------------------------------------------

fn_name MainArgs_dtor = "MainArgs.dtor";

MainArgs::~MainArgs()
{
   Debug::ftnt(MainArgs_dtor);

   Debug::SwLog(MainArgs_dtor, UnexpectedInvocation, 0);
}

//------------------------------------------------------------------------------

c_string MainArgs::At(size_t n)
{
   Debug::ftnt("MainArgs.At");

   auto reg = Singleton<MainArgs>::Extant();
   if(reg == nullptr) return MainParms.at(n).c_str();
   return reg->args_.at(n).c_str();
}

//------------------------------------------------------------------------------

void MainArgs::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Immutable::Display(stream, prefix, options);

   stream << prefix << "args : " << CRLF;

   for(size_t i = 0; i < args_.size(); ++i)
   {
      stream << spaces(2) << strIndex(i) << args_.at(i).c_str() << CRLF;
   }
}

//------------------------------------------------------------------------------

string MainArgs::Find(c_string tag)
{
   Debug::ft("MainArgs.Find");

   auto reg = Singleton<MainArgs>::Extant();

   if(reg == nullptr)
   {
      for(size_t i = 0; i < MainParms.size(); ++i)
      {
         if(MainParms.at(i).find(tag) == 0)
         {
            string value(MainParms.at(i).c_str());
            return value.substr(strlen(tag));
         }
      }
   }
   else
   {
      for(size_t i = 0; i < reg->args_.size(); ++i)
      {
         if(reg->args_.at(i).find(tag) == 0)
         {
            string value(reg->args_.at(i).c_str());
            return value.substr(strlen(tag));
         }
      }
   }

   return EMPTY_STR;
}

//------------------------------------------------------------------------------

void MainArgs::Patch(sel_t selector, void* arguments)
{
   Immutable::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

void MainArgs::PushBack(const string& arg)
{
   Debug::ft("MainArgs.PushBack");

   MainParms.push_back(arg);
}

//------------------------------------------------------------------------------

size_t MainArgs::Size()
{
   auto reg = Singleton<MainArgs>::Extant();
   if(reg == nullptr) return MainParms.size();
   return reg->args_.size();
}
}
