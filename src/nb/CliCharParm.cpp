//==============================================================================
//
//  CliCharParm.cpp
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
#include "CliCharParm.h"
#include <cstddef>
#include <cstring>
#include <ostream>
#include <string>
#include "CliBuffer.h"
#include "CliCookie.h"
#include "CliThread.h"
#include "Debug.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Separates valid input characters in parameter help text.
//
constexpr char CharSeparator = '|';

//------------------------------------------------------------------------------

CliCharParm::CliCharParm
   (c_string help, c_string chars, bool opt, c_string tag) :
   CliParm(help, opt, tag),
   chars_(chars)
{
   Debug::ft("CliCharParm.ctor");

   Debug::Assert(chars_ != nullptr);
}

//------------------------------------------------------------------------------

CliCharParm::~CliCharParm()
{
   Debug::ftnt("CliCharParm.dtor");
}

//------------------------------------------------------------------------------

void CliCharParm::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   CliParm::Display(stream, prefix, options);

   stream << prefix << "chars : " << chars_ << CRLF;
}

//------------------------------------------------------------------------------

CliParm::Rc CliCharParm::GetCharParmRc(char& c, CliThread& cli) const
{
   Debug::ft("CliCharParm.GetCharParmRc");

   string s;
   string t;

   //  Get the next string after saving the current location in the buffer.
   //
   auto x = cli.ibuf->Pos();
   auto rc = cli.ibuf->GetStr(t, s);
   auto tagged = (!t.empty());

   //  If a tag was found, then it must match this parameter's tag before
   //  we bother to look for the parameter itself.
   //
   if(tagged)
   {
      auto tag = Tag();

      if((tag == nullptr) || (t.compare(tag) != 0))
      {
         cli.ibuf->SetPos(x);
         cli.Cookie().Advance();
         return None;
      }
   }

   if((rc == Ok) && (s.size() == 1))
   {
      //  A string was found.  See if its first character matches one of
      //  those in our list.
      //
      auto n = strlen(chars_);
      c = s.front();

      for(size_t i = 0; i < n; ++i)
      {
         if(chars_[i] == c)
         {
            cli.Cookie().Advance();
            return Ok;
         }
      }
   }

   //  A valid parameter was not found.  This is an error unless the
   //  parameter is optional and was untagged, in which case we report
   //  its absence after backing up if the skip character was entered.
   //
   if(IsOptional() && !tagged)
   {
      if(rc != Skip) cli.ibuf->SetPos(x);
      cli.Cookie().Advance();
      return None;
   }

   //  Error.  Highlight the location where a character was expected.
   //
   cli.Cookie().Advance();
   cli.ibuf->ErrorAtPos(cli, "Specific character expected", x);
   return Error;
}

//------------------------------------------------------------------------------

void CliCharParm::Patch(sel_t selector, void* arguments)
{
   CliParm::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

bool CliCharParm::ShowValues(string& values) const
{
   Debug::ft("CliCharParm.ShowValues");

   auto last = strlen(chars_) - 1;

   for(size_t i = 0; i <= last; ++i)
   {
      values += chars_[i];
      if(i < last) values += CharSeparator;
   }

   return true;
}
}
