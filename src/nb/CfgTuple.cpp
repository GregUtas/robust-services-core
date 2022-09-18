//==============================================================================
//
//  CfgTuple.cpp
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
#include "CfgTuple.h"
#include <cstdint>
#include <sstream>
#include "Algorithms.h"
#include "CfgParmRegistry.h"
#include "Debug.h"
#include "Log.h"
#include "NbLogs.h"
#include "Singleton.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
const char CfgTuple::CommentChar = '/';

//------------------------------------------------------------------------------

CfgTuple::CfgTuple(c_string key, c_string input) :
   key_(key),
   input_(input)
{
   Debug::ft("CfgTuple.ctor");

   if(key_.find_first_not_of(ValidKeyChars().c_str()) != string::npos)
   {
      auto log = Log::Create(ConfigLogGroup, ConfigKeyInvalid);

      if(log != nullptr)
      {
         *log << Log::Tab << "errval=" << key_;
         Log::Submit(log);
      }
   }
}

//------------------------------------------------------------------------------

fn_name CfgTuple_dtor = "CfgTuple.dtor";

CfgTuple::~CfgTuple()
{
   Debug::ftnt(CfgTuple_dtor);

   Debug::SwLog(CfgTuple_dtor, UnexpectedInvocation, 0);
   Singleton<CfgParmRegistry>::Extant()->UnbindTuple(*this);
}

//------------------------------------------------------------------------------

void CfgTuple::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Protected::Display(stream, prefix, options);

   stream << prefix << "key   : " << key_ << CRLF;
   stream << prefix << "input : " << input_ << CRLF;
   stream << prefix << "link  : " << link_.to_str() << CRLF;
}

//------------------------------------------------------------------------------

ptrdiff_t CfgTuple::LinkDiff()
{
   uintptr_t local;
   auto fake = reinterpret_cast<const CfgTuple*>(&local);
   return ptrdiff(&fake->link_, fake);
}

//------------------------------------------------------------------------------

void CfgTuple::Patch(sel_t selector, void* arguments)
{
   Protected::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

const string& CfgTuple::ValidBlankChars()
{
   //  Valid blank characters in a file that contains configuration tuples.
   //
   static const string BlankChars(" ");

   return BlankChars;
}

//------------------------------------------------------------------------------

const string& CfgTuple::ValidKeyChars()
{
   //  Valid characters in a configuration tuple's name.
   //
   static const string NameChars
      ("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-.");

   return NameChars;
}

//------------------------------------------------------------------------------

const string& CfgTuple::ValidValueChars()
{
   //  Valid characters in a configuration tuple's value.
   //
   static const string ValueChars(ValidKeyChars() + " :/\\");

   return ValueChars;
}
}
