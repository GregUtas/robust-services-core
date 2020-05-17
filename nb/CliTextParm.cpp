//==============================================================================
//
//  CliTextParm.cpp
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
#include "CliTextParm.h"
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <ios>
#include <memory>
#include <ostream>
#include <string>
#include "CliBuffer.h"
#include "CliCookie.h"
#include "CliText.h"
#include "CliThread.h"
#include "Debug.h"
#include "Formatters.h"
#include "SysFile.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
fn_name CliTextParm_ctor = "CliTextParm.ctor";

CliTextParm::CliTextParm(c_string help, bool opt, uint32_t size,
   c_string tag) : CliParm(help, opt, tag)
{
   Debug::ft(CliTextParm_ctor);

   strings_.Init(size, CliParm::CellDiff(), MemImmutable);
}

//------------------------------------------------------------------------------

fn_name CliTextParm_dtor = "CliTextParm.dtor";

CliTextParm::~CliTextParm()
{
   Debug::ftnt(CliTextParm_dtor);
}

//------------------------------------------------------------------------------

fn_name CliTextParm_AccessParm = "CliTextParm.AccessParm";

CliParm* CliTextParm::AccessParm(CliCookie& cookie, uint32_t depth) const
{
   Debug::ft(CliTextParm_AccessParm);

   //  If we are reading parameters that are associated with one of our
   //  strings, continue to search for more parameters.
   //
   auto t = strings_.At(cookie.Index(depth));
   if(t != nullptr) return t->AccessParm(cookie, depth + 1);
   return nullptr;
}

//------------------------------------------------------------------------------

fn_name CliTextParm_BindText = "CliTextParm.BindText";

bool CliTextParm::BindText(CliText& text, id_t index)
{
   Debug::ft(CliTextParm_BindText);

   //  Generate a log and fail if this entry would be unreachable.
   //  This occurs when
   //  o another entry already uses the same string, or
   //  o the last entry matches on any string.
   //
   auto s = text.Text();

   for(auto t = strings_.First(); t != nullptr; strings_.Next(t))
   {
      auto prev = t->Text();

      if((prev[0] == NUL) || (strcmp(prev, s) == 0))
      {
         Debug::SwLog(CliTextParm_BindText, t->GetId(), index);
         return false;
      }
   }

   text.SetId(index);
   strings_.Insert(text);
   return true;
}

//------------------------------------------------------------------------------

fn_name CliTextParm_Descend = "CliTextParm.Descend";

void CliTextParm::Descend(CliCookie& cookie, uint32_t index) const
{
   Debug::ft(CliTextParm_Descend);

   //  If the string that was just read takes no parameters, advance
   //  to the next parameter at this level, else descend two levels
   //  (to the string that was just found, and then to its parameters).
   //
   if(strings_.At(index)->Parms().Empty())
      cookie.Advance();
   else
      cookie.Descend(index);
}

//------------------------------------------------------------------------------

void CliTextParm::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   CliParm::Display(stream, prefix, options);

   stream << prefix << "strings : " << CRLF;
   strings_.Display(stream, prefix + spaces(2), options);
}

//------------------------------------------------------------------------------

fn_name CliTextParm_Explain = "CliTextParm.Explain";

void CliTextParm::Explain(ostream& stream, col_t indent) const
{
   Debug::ft(CliTextParm_Explain);

   //  We allow CliParm::Explain to invoke our ShowValues function if
   //  o any string is acceptable (size = 0), or
   //  o only one string is acceptable, and it binds the *same* help
   //    explanation as this parameter.
   //
   auto size = strings_.Size();

   if(size == 0) return CliParm::Explain(stream, indent);

   if((size == 1) && (Help() == strings_.First()->Help()))
   {
      return CliParm::Explain(stream, indent);
   }

   //  The text parameter accepts more than one string or a string that can
   //  be followed by its own parameters.  Display the description of these
   //  strings' purpose, followed by all of them.  Surround the strings with
   //  indicators that specify whether a choice is mandatory or optional.
   //
   auto opt = IsOptional();
   auto tag = Tag();

   if(indent < 2)
   {
      Debug::SwLog(CliTextParm_Explain, "invalid indent", indent);
      indent = 2;
   }

   stream << spaces(indent-2);
   if(opt && (tag != nullptr)) stream << tag << CliBuffer::OptTagChar;
   if(opt) stream << OptParmBegin; else stream << MandParmBegin;

   size_t w = indent - 1;
   if(opt && (tag != nullptr)) w += strlen(tag) + 1;
   stream << spaces(ParmWidth - w);
   stream << ParmExplPrefix;
   stream << Help() << CRLF;

   for(auto t = strings_.First(); t != nullptr; strings_.Next(t))
   {
      t->Explain(stream, indent);
   }

   stream << spaces(indent-2);
   if(opt) stream << OptParmEnd; else stream << MandParmEnd;
   stream << CRLF;
}

//------------------------------------------------------------------------------

fn_name CliTextParm_GetFileNameRc = "CliTextParm.GetFileNameRc";

CliTextParm::Rc CliTextParm::GetFileNameRc(string& s, CliThread& cli) const
{
   Debug::ft(CliTextParm_GetFileNameRc);

   auto rc = GetStringRc(s, cli);

   if(rc == Ok)
   {
      //  Open the file without purging it to confirm that the name (and path,
      //  if any) are valid.  If the file is empty, erase it.
      //
      auto stream = SysFile::CreateOstream(s.c_str());

      if(stream == nullptr)
      {
         cli.ibuf->ErrorAtPos(cli, "Could not open file: check name and path");
         cli.Cookie().Advance();
         return Error;
      }

      bool empty = (stream->tellp() == std::streampos(0));
      stream.reset();
      if(empty) remove(s.c_str());
   }

   return rc;
}

//------------------------------------------------------------------------------

fn_name CliTextParm_GetIdentifierRc = "CliTextParm.GetIdentifierRc";

CliParm::Rc CliTextParm::GetIdentifierRc(string& s, CliThread& cli,
   const string& valid, const string& exclude) const
{
   Debug::ft(CliTextParm_GetIdentifierRc);

   auto rc = GetStringRc(s, cli);

   if(rc == Ok)
   {
      auto x = cli.ibuf->Pos();

      if(exclude.find(s.front()) != string::npos)
      {
         cli.ibuf->ErrorAtPos(cli, "Illegal initial character", x);
         cli.Cookie().Advance();
         return Error;
      }

      auto i = s.find_first_not_of(valid);

      if(i != string::npos)
      {
         cli.ibuf->ErrorAtPos(cli, "Illegal character", x + i);
         cli.Cookie().Advance();
         return Error;
      }
   }

   return rc;
}

//------------------------------------------------------------------------------

fn_name CliTextParm_GetStringRc = "CliTextParm.GetStringRc";

CliParm::Rc CliTextParm::GetStringRc(string& s, CliThread& cli) const
{
   Debug::ft(CliTextParm_GetStringRc);

   string t;

   //  Get the next string after saving the current location in the buffer.
   //
   s.clear();
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

   //  Any string is acceptable here, so advance to the next parameter
   //  if one was found.
   //
   if(rc == Ok)
   {
      cli.Cookie().Advance();
      return Ok;
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

   //  Error.  Highlight the location where a string was expected.
   //
   cli.ibuf->ErrorAtPos(cli, "String expected", x);
   cli.Cookie().Advance();
   return Error;
}

//------------------------------------------------------------------------------

fn_name CliTextParm_GetTextParmRc = "CliTextParm.GetTextParmRc";

CliParm::Rc CliTextParm::GetTextParmRc(id_t& i, string& s, CliThread& cli) const
{
   Debug::ft(CliTextParm_GetTextParmRc);

   i = 0;

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
         s.clear();
         cli.ibuf->SetPos(x);
         cli.Cookie().Advance();
         return None;
      }
   }

   if(rc == Ok)
   {
      //  A string was found.  See if it matches one of those in the array
      //  of acceptable strings.  If it does, return its array index.
      //
      for(auto r = strings_.First(); r != nullptr; strings_.Next(r))
      {
         auto text = r->Text();

         if((text[0] == NUL) || (s.compare(text) == 0))
         {
            i = r->GetId();
            Descend(cli.Cookie(), i);
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
      s.clear();
      if(rc != Skip) cli.ibuf->SetPos(x);
      cli.Cookie().Advance();
      return None;
   }

   //  Error.  Highlight the location where a string was expected.
   //
   s.clear();
   cli.ibuf->ErrorAtPos(cli, "Specific string value expected", x);
   cli.Cookie().Advance();
   return Error;
}

//------------------------------------------------------------------------------

void CliTextParm::Patch(sel_t selector, void* arguments)
{
   CliParm::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name CliTextParm_ShowValues = "CliTextParm.ShowValues";

bool CliTextParm::ShowValues(string& values) const
{
   Debug::ft(CliTextParm_ShowValues);

   auto size = strings_.Size();

   if(size == 0)
   {
      //  Any string is acceptable.
      //
      values = AnyStringParm;
      return false;
   }

   if(size == 1)
   {
      auto t = strings_.First();

      if(t->Parms().Empty())
      {
         //  Only one string is acceptable, and it takes
         //  no parameters.  Simply display it.
         //
         t->ShowValues(values);
         return true;
      }
   }

   //  Our Explain function handles other cases without
   //  invoking CliParm::Explain, so we shouldn't get here.
   //
   Debug::SwLog(CliTextParm_ShowValues, strClass(this), size);
   return true;
}
}
