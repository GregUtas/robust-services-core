//==============================================================================
//
//  CliParm.cpp
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
fixed_string CliParm::AnyStringParm = "<str>";
const col_t CliParm::ParmWidth = 17;
const char CliParm::MandParmBegin = '(';
const char CliParm::MandParmEnd = ')';
const char CliParm::OptParmBegin = '[';
const char CliParm::OptParmEnd = ']';

//------------------------------------------------------------------------------

fn_name CliParm_ctor = "CliParm.ctor";

CliParm::CliParm(c_string help, bool opt, c_string tag) :
   help_(help),
   opt_(opt),
   tag_(tag)
{
   Debug::ft(CliParm_ctor);

   Debug::Assert(help_ != nullptr);

   auto size = strlen(help_);
   auto total = ParmWidth + strlen(ParmExplPrefix) + size;

   if(size == 0)
      Debug::SwLog(CliParm_ctor, "help string empty", size);
   else if(total >= COUT_LENGTH_MAX)
      Debug::SwLog
         (CliParm_ctor, "help string too long", total - COUT_LENGTH_MAX + 1);
}

//------------------------------------------------------------------------------

fn_name CliParm_dtor = "CliParm.dtor";

CliParm::~CliParm()
{
   Debug::ftnt(CliParm_dtor);

   Debug::SwLog(CliParm_dtor, UnexpectedInvocation, 0);
}

//------------------------------------------------------------------------------

CliParm* CliParm::AccessParm(CliCookie& cookie, uint32_t depth) const
{
   Debug::ft("CliParm.AccessParm");

   //  AccessParm essentially finds subparameters.  A basic parameter (an int,
   //  bool, char, or pointer) has no subparameters, so return nullptr.  Only
   //  strings (CliText and its subclasses) support subparameters.
   //
   return nullptr;
}

//------------------------------------------------------------------------------

ptrdiff_t CliParm::CellDiff()
{
   uintptr_t local;
   auto fake = reinterpret_cast< const CliParm* >(&local);
   return ptrdiff(&fake->pid_, fake);
}

//------------------------------------------------------------------------------

void CliParm::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Immutable::Display(stream, prefix, options);

   stream << prefix << "pid : " << pid_.to_str() << CRLF;
   stream << prefix << "opt : " << opt_ << CRLF;
   if(tag_ == nullptr) return;
   stream << prefix << "tag : " << tag_ << CRLF;
}

//------------------------------------------------------------------------------

void CliParm::Explain(ostream& stream, col_t indent) const
{
   Debug::ft("CliParm.Explain");

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

   stream << buff.str() << spaces(ParmWidth - buff.str().size());
   stream << ParmExplPrefix << Help() << CRLF;
}

//------------------------------------------------------------------------------

bool CliParm::GetBoolParm(bool& b, CliThread& cli) const
{
   Debug::ft("CliParm.GetBoolParm");

   return (GetBoolParmRc(b, cli) == Ok);
}

//------------------------------------------------------------------------------

CliParm::Rc CliParm::GetBoolParmRc(bool& b, CliThread& cli) const
{
   Debug::ft("CliParm.GetBoolParmRc");

   return Mismatch(cli, "boolean");
}

//------------------------------------------------------------------------------

bool CliParm::GetCharParm(char& c, CliThread& cli) const
{
   Debug::ft("CliParm.GetCharParm");

   return (GetCharParmRc(c, cli) == Ok);
}

//------------------------------------------------------------------------------

CliParm::Rc CliParm::GetCharParmRc(char& c, CliThread& cli) const
{
   Debug::ft("CliParm.GetCharParmRc");

   return Mismatch(cli, "character");
}

//------------------------------------------------------------------------------

bool CliParm::GetFileName(string& s, CliThread& cli) const
{
   Debug::ft("CliParm.GetFileName");

   return (GetFileNameRc(s, cli) == Ok);
}

//------------------------------------------------------------------------------

CliParm::Rc CliParm::GetFileNameRc(string& s, CliThread& cli) const
{
   Debug::ft("CliParm.GetFileNameRc");

   return Mismatch(cli, "filename");
}

//------------------------------------------------------------------------------

bool CliParm::GetIdentifier(string& s, CliThread& cli,
   const string& valid, const string& exclude) const
{
   Debug::ft("CliParm.GetIdentifier");

   return (GetIdentifierRc(s, cli, valid, exclude) == Ok);
}

//------------------------------------------------------------------------------

CliParm::Rc CliParm::GetIdentifierRc(string& s, CliThread& cli,
   const string& valid, const string& exclude) const
{
   Debug::ft("CliParm.GetIdentifierRc");

   return Mismatch(cli, "identifier");
}

//------------------------------------------------------------------------------

bool CliParm::GetIntParm(word& n, CliThread& cli) const
{
   Debug::ft("CliParm.GetIntParm");

   return (GetIntParmRc(n, cli) == Ok);
}

//------------------------------------------------------------------------------

CliParm::Rc CliParm::GetIntParmRc(word& n, CliThread& cli) const
{
   Debug::ft("CliParm.GetIntParmRc");

   return Mismatch(cli, "integer");
}

//------------------------------------------------------------------------------

bool CliParm::GetPtrParm(void*& p, CliThread& cli) const
{
   Debug::ft("CliParm.GetPtrParm");

   return (GetPtrParmRc(p, cli) == Ok);
}

//------------------------------------------------------------------------------

CliParm::Rc CliParm::GetPtrParmRc(void*& p, CliThread& cli) const
{
   Debug::ft("CliParm.GetPtrParmRc");

   return Mismatch(cli, "pointer");
}

//------------------------------------------------------------------------------

bool CliParm::GetString(string& s, CliThread& cli) const
{
   Debug::ft("CliParm.GetString");

   return (GetStringRc(s, cli) == Ok);
}

//------------------------------------------------------------------------------

CliParm::Rc CliParm::GetStringRc(string& s, CliThread& cli) const
{
   Debug::ft("CliParm.GetStringRc");

   return Mismatch(cli, "string");
}

//------------------------------------------------------------------------------

bool CliParm::GetTextIndex(id_t& i, CliThread& cli) const
{
   Debug::ft("CliParm.GetTextIndex");

   string s;

   return (GetTextParmRc(i, s, cli) == Ok);
}

//------------------------------------------------------------------------------

CliParm::Rc CliParm::GetTextIndexRc(id_t& i, CliThread& cli) const
{
   Debug::ft("CliParm.GetTextIndexRc");

   string s;

   return GetTextParmRc(i, s, cli);
}

//------------------------------------------------------------------------------

bool CliParm::GetTextParm(id_t& i, string& s, CliThread& cli) const
{
   Debug::ft("CliParm.GetTextParm");

   return (GetTextParmRc(i, s, cli) == Ok);
}

//------------------------------------------------------------------------------

CliParm::Rc CliParm::GetTextParmRc(id_t& i, string& s, CliThread& cli) const
{
   Debug::ft("CliParm.GetTextParmRc");

   return Mismatch(cli, "text");
}

//------------------------------------------------------------------------------

CliParm::Rc CliParm::Mismatch(const CliThread& cli, const string& type)
{
   Debug::ft("CliParm.Mismatch");

   auto s = "Internal error: parameter mismatch when looking for " + type;
   cli.ibuf->ErrorAtPos(cli, s);
   return Error;
}

//------------------------------------------------------------------------------

void CliParm::Patch(sel_t selector, void* arguments)
{
   Immutable::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name CliParm_ShowValues = "CliParm.ShowValues";

bool CliParm::ShowValues(string& values) const
{
   Debug::ft(CliParm_ShowValues);

   Debug::SwLog(CliParm_ShowValues, strOver(this), 0);
   return false;
}
}
