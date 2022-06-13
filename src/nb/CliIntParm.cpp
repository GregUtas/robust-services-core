//==============================================================================
//
//  CliIntParm.cpp
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
#include "CliIntParm.h"
#include <ios>
#include <iosfwd>
#include <sstream>
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
//  Represents an integer value in parameter help text.
//
static fixed_string AnyIntParm = "<int>";

//  Separates the minimum and maximum values in parameter help text.
//
constexpr char RangeSeparator = ':';

fixed_string CliIntParm::AnyHexParm = "<hex>";

//------------------------------------------------------------------------------

CliIntParm::CliIntParm(c_string help, word min, word max,
   bool opt, c_string tag, bool hex) : CliParm(help, opt, tag),
   min_(min),
   max_(max),
   hex_(hex)
{
   Debug::ft("CliIntParm.ctor");
}

//------------------------------------------------------------------------------

CliIntParm::~CliIntParm()
{
   Debug::ftnt("CliIntParm.dtor");
}

//------------------------------------------------------------------------------

void CliIntParm::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   CliParm::Display(stream, prefix, options);

   stream << prefix << "min : " << min_ << CRLF;
   stream << prefix << "max : " << max_ << CRLF;
   stream << prefix << "hex : " << hex_ << CRLF;
}

//------------------------------------------------------------------------------

CliParm::Rc CliIntParm::GetIntParmRc(word& n, CliThread& cli) const
{
   Debug::ft("CliIntParm.GetIntParmRc");

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

   //  If the string is an integer within the correct range, return it.
   //
   if(rc == Ok)
   {
      rc = CliBuffer::GetInt(s, n, hex_);

      if(rc == Ok)
      {
         if((n >= min_) && (n <= max_))
         {
            cli.Cookie().Advance();
            return Ok;
         }
      }

      n = 0;
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

   //  Error.  Highlight the location where an integer was expected.
   //
   cli.ibuf->ErrorAtPos(cli, "Integer expected or out of range", x);
   cli.Cookie().Advance();
   return Error;
}

//------------------------------------------------------------------------------

void CliIntParm::Patch(sel_t selector, void* arguments)
{
   CliParm::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

bool CliIntParm::ShowValues(string& values) const
{
   Debug::ft("CliIntParm.ShowValues");

   if((min_ == WORD_MIN) && (max_ == WORD_MAX))
   {
      if(hex_)
         values = AnyHexParm;
      else
         values = AnyIntParm;
      return false;
   }

   std::ostringstream stream;

   if(hex_) stream << std::nouppercase << std::hex;
   stream << min_ << RangeSeparator << max_;
   values = stream.str();
   return true;
}
}
