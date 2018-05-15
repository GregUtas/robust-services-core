//==============================================================================
//
//  CliParm.cpp
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
#include "CliParm.h"
#include <cstring>
#include <sstream>
#include "Algorithms.h"
#include "CliBuffer.h"
#include "CliThread.h"
#include "Debug.h"
#include "Formatters.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
fixed_string CliParm::ParmExplPrefix = " : ";
fixed_string CliParm::AnyStringParm  = "<str>";
const col_t CliParm::ParmWidth = 17;
const char CliParm::MandParmBegin = '(';
const char CliParm::MandParmEnd = ')';
const char CliParm::OptParmBegin = '[';
const char CliParm::OptParmEnd = ']';

//------------------------------------------------------------------------------

fn_name CliParm_ctor = "CliParm.ctor";

CliParm::CliParm(const char* help, bool opt, const char* tag) :
   help_(help),
   opt_(opt),
   tag_(tag)
{
   Debug::ft(CliParm_ctor);

   Debug::Assert(help_ != nullptr);

   auto len = ParmWidth + strlen(ParmExplPrefix) + strlen(help_);

   if((len == 0) || (len >= 80))
   {
      Debug::SwErr(CliParm_ctor, len, 0);
   }
}

//------------------------------------------------------------------------------

fn_name CliParm_dtor = "CliParm.dtor";

CliParm::~CliParm()
{
   Debug::ft(CliParm_dtor);
}

//------------------------------------------------------------------------------

fn_name CliParm_AccessParm = "CliParm.AccessParm";

CliParm* CliParm::AccessParm(CliCookie& cookie, size_t depth) const
{
   Debug::ft(CliParm_AccessParm);

   //  AccessParm essentially finds subparameters.  A basic parameter (an int,
   //  bool, char, or pointer) has no subparameters, so return nullptr.  Only
   //  strings (CliText and its subclasses) support subparameters.
   //
   return nullptr;
}

//------------------------------------------------------------------------------

ptrdiff_t CliParm::CellDiff()
{
   int local;
   auto fake = reinterpret_cast< const CliParm* >(&local);
   return ptrdiff(&fake->pid_, fake);
}

//------------------------------------------------------------------------------

void CliParm::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Protected::Display(stream, prefix, options);

   stream << prefix << "pid : " << pid_.to_str() << CRLF;
   stream << prefix << "opt : " << opt_ << CRLF;
   if(tag_ == nullptr) return;
   stream << prefix << "tag : " << tag_ << CRLF;
}

//------------------------------------------------------------------------------

fn_name CliParm_Explain = "CliParm.Explain";

void CliParm::Explain(ostream& stream, col_t indent) const
{
   Debug::ft(CliParm_Explain);

   std::ostringstream buff;
   string values;

   auto mand = ShowValues(values);
   auto opt = IsOptional();
   auto tag = Tag();

   //  Display the parameter's legal values, surrounded by indicators
   //  that specify whether it is mandatory or optional, and followed
   //  by an explanation of its purpose.
   //
   buff << spaces(indent);

   if(opt)
   {
      if(tag != nullptr) buff << tag << CliBuffer::OptTagChar;
      buff << OptParmBegin;
   }
   else if(mand)
   {
      buff << MandParmBegin;
   }

   buff << values;

   if(opt)
      buff << OptParmEnd;
   else if(mand)
      buff << MandParmEnd;

   stream << buff.str();
   auto width = buff.str().size();
   if(width < ParmWidth) stream << spaces(ParmWidth - width);
   stream << ParmExplPrefix;
   stream << Help() << CRLF;
}

//------------------------------------------------------------------------------

fn_name CliParm_GetBoolParm = "CliParm.GetBoolParm";

bool CliParm::GetBoolParm(bool& b, CliThread& cli) const
{
   Debug::ft(CliParm_GetBoolParm);

   return (GetBoolParmRc(b, cli) == Ok);
}

//------------------------------------------------------------------------------

fn_name CliParm_GetBoolParmRc = "CliParm.GetBoolParmRc";

CliParm::Rc CliParm::GetBoolParmRc(bool& b, CliThread& cli) const
{
   Debug::ft(CliParm_GetBoolParmRc);

   return Mismatch(cli, "boolean");
}

//------------------------------------------------------------------------------

fn_name CliParm_GetCharParm = "CliParm.GetCharParm";

bool CliParm::GetCharParm(char& c, CliThread& cli) const
{
   Debug::ft(CliParm_GetCharParm);

   return (GetCharParmRc(c, cli) == Ok);
}

//------------------------------------------------------------------------------

fn_name CliParm_GetCharParmRc = "CliParm.GetCharParmRc";

CliParm::Rc CliParm::GetCharParmRc(char& c, CliThread& cli) const
{
   Debug::ft(CliParm_GetCharParmRc);

   return Mismatch(cli, "character");
}

//------------------------------------------------------------------------------

fn_name CliParm_GetFileName = "CliParm.GetFileName";

bool CliParm::GetFileName(string& s, CliThread& cli) const
{
   Debug::ft(CliParm_GetFileName);

   return (GetFileNameRc(s, cli) == Ok);
}

//------------------------------------------------------------------------------

fn_name CliParm_GetFileNameRc = "CliParm.GetFileNameRc";

CliParm::Rc CliParm::GetFileNameRc(string& s, CliThread& cli) const
{
   Debug::ft(CliParm_GetFileNameRc);

   return Mismatch(cli, "filename");
}

//------------------------------------------------------------------------------

fn_name CliParm_GetIdentifier = "CliParm.GetIdentifier";

bool CliParm::GetIdentifier(string& s, CliThread& cli,
   const string& valid, const string& exclude) const
{
   Debug::ft(CliParm_GetIdentifier);

   return (GetIdentifierRc(s, cli, valid, exclude) == Ok);
}

//------------------------------------------------------------------------------

fn_name CliParm_GetIdentifierRc = "CliParm.GetIdentifierRc";

CliParm::Rc CliParm::GetIdentifierRc(string& s, CliThread& cli,
   const string& valid, const string& exclude) const
{
   Debug::ft(CliParm_GetIdentifierRc);

   return Mismatch(cli, "identifier");
}

//------------------------------------------------------------------------------

fn_name CliParm_GetIntParm = "CliParm.GetIntParm";

bool CliParm::GetIntParm(word& n, CliThread& cli) const
{
   Debug::ft(CliParm_GetIntParm);

   return (GetIntParmRc(n, cli) == Ok);
}

//------------------------------------------------------------------------------

fn_name CliParm_GetIntParmRc = "CliParm.GetIntParmRc";

CliParm::Rc CliParm::GetIntParmRc(word& n, CliThread& cli) const
{
   Debug::ft(CliParm_GetIntParmRc);

   return Mismatch(cli, "integer");
}

//------------------------------------------------------------------------------

fn_name CliParm_GetPtrParm = "CliParm.GetPtrParm";

bool CliParm::GetPtrParm(void*& p, CliThread& cli) const
{
   Debug::ft(CliParm_GetPtrParm);

   return (GetPtrParmRc(p, cli) == Ok);
}

//------------------------------------------------------------------------------

fn_name CliParm_GetPtrParmRc = "CliParm.GetPtrParmRc";

CliParm::Rc CliParm::GetPtrParmRc(void*& p, CliThread& cli) const
{
   Debug::ft(CliParm_GetPtrParmRc);

   return Mismatch(cli, "pointer");
}

//------------------------------------------------------------------------------

fn_name CliParm_GetString = "CliParm.GetString";

bool CliParm::GetString(string& s, CliThread& cli) const
{
   Debug::ft(CliParm_GetString);

   return (GetStringRc(s, cli) == Ok);
}

//------------------------------------------------------------------------------

fn_name CliParm_GetStringRc = "CliParm.GetStringRc";

CliParm::Rc CliParm::GetStringRc(string& s, CliThread& cli) const
{
   Debug::ft(CliParm_GetStringRc);

   return Mismatch(cli, "string");
}

//------------------------------------------------------------------------------

fn_name CliParm_GetTextIndex = "CliParm.GetTextIndex";

bool CliParm::GetTextIndex(id_t& i, CliThread& cli) const
{
   Debug::ft(CliParm_GetTextIndex);

   string s;

   return (GetTextParmRc(i, s, cli) == Ok);
}

//------------------------------------------------------------------------------

fn_name CliParm_GetTextIndexRc = "CliParm.GetTextIndexRc";

CliParm::Rc CliParm::GetTextIndexRc(id_t& i, CliThread& cli) const
{
   Debug::ft(CliParm_GetTextIndexRc);

   string s;

   return GetTextParmRc(i, s, cli);
}

//------------------------------------------------------------------------------

fn_name CliParm_GetTextParm = "CliParm.GetTextParm";

bool CliParm::GetTextParm(id_t& i, string& s, CliThread& cli) const
{
   Debug::ft(CliParm_GetTextParm);

   return (GetTextParmRc(i, s, cli) == Ok);
}

//------------------------------------------------------------------------------

fn_name CliParm_GetTextParmRc = "CliParm.GetTextParmRc";

CliParm::Rc CliParm::GetTextParmRc(id_t& i, string& s, CliThread& cli) const
{
   Debug::ft(CliParm_GetTextParmRc);

   return Mismatch(cli, "text");
}

//------------------------------------------------------------------------------

fn_name CliParm_Mismatch = "CliParm.Mismatch";

CliParm::Rc CliParm::Mismatch(const CliThread& cli, const string& type)
{
   Debug::ft(CliParm_Mismatch);

   auto s = "Internal error: parameter mismatch when looking for " + type;
   cli.ibuf->ErrorAtPos(cli, s);
   return Error;
}

//------------------------------------------------------------------------------

void CliParm::Patch(sel_t selector, void* arguments)
{
   Protected::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name CliParm_ShowValues = "CliParm.ShowValues";

bool CliParm::ShowValues(string& values) const
{
   Debug::ft(CliParm_ShowValues);

   //  This is a pure virtual function.
   //
   Debug::SwErr(CliParm_ShowValues, 0, 0, ErrorLog);
   return false;
}
}
