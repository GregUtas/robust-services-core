//==============================================================================
//
//  CliPtrParm.cpp
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
#include "CliPtrParm.h"
#include <string>
#include "CliBuffer.h"
#include "CliCookie.h"
#include "CliIntParm.h"
#include "CliThread.h"
#include "Debug.h"

using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
CliPtrParm::CliPtrParm(c_string help, bool opt, c_string tag) :
   CliParm(help, opt, tag)
{
   Debug::ft("CliPtrParm.ctor");
}

//------------------------------------------------------------------------------

CliPtrParm::~CliPtrParm()
{
   Debug::ftnt("CliPtrParm.dtor");
}

//------------------------------------------------------------------------------

CliParm::Rc CliPtrParm::GetPtrParmRc(void*& p, CliThread& cli) const
{
   Debug::ft("CliPtrParm.GetPtrParmRc");

   p = nullptr;

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

   //  If the string is an integer within the pointer's range, return it.
   //
   if(rc == Ok)
   {
      word n;

      rc = CliBuffer::GetInt(s, n, true);

      if(rc == Ok)
      {
         p = (void*) n;
         cli.Cookie().Advance();
         return Ok;
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

   cli.ibuf->ErrorAtPos(cli, "Pointer expected or out of range", x);
   cli.Cookie().Advance();
   return Error;
}

//------------------------------------------------------------------------------

void CliPtrParm::Patch(sel_t selector, void* arguments)
{
   CliParm::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

bool CliPtrParm::ShowValues(string& values) const
{
   Debug::ft("CliPtrParm.ShowValues");

   values = CliIntParm::AnyHexParm;
   return false;
}
}
