//==============================================================================
//
//  LibraryErrSet.cpp
//
//  Copyright (C) 2013-2021  Greg Utas
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
#include "LibraryErrSet.h"
#include <cstring>
#include <iosfwd>
#include <sstream>
#include <vector>
#include "CliBuffer.h"
#include "Debug.h"
#include "Formatters.h"

using namespace NodeBase;
using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace CodeTools
{
LibraryErrSet::LibraryErrSet(const string& name, LibExprErr err, size_t pos) :
   LibrarySet(name),
   err_(err),
   pos_(pos)
{
   Debug::ft("LibraryErrSet.ctor");
}

//------------------------------------------------------------------------------

LibraryErrSet::~LibraryErrSet()
{
   Debug::ftnt("LibraryErrSet.dtor");
}

//------------------------------------------------------------------------------

word LibraryErrSet::Check(CliThread& cli, ostream* stream, string& expl) const
{
   Debug::ft("LibraryErrSet.Check");

   return Error(expl);
}

//------------------------------------------------------------------------------

word LibraryErrSet::Count(string& result) const
{
   Debug::ft("LibraryErrSet.Count");

   return Error(result);
}

//------------------------------------------------------------------------------

word LibraryErrSet::Countlines(string& result) const
{
   Debug::ft("LibraryErrSet.Countlines");

   return Error(result);
}

//------------------------------------------------------------------------------

void LibraryErrSet::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   LibrarySet::Display(stream, prefix, options);

   stream << prefix << "err : " << err_ << CRLF;
   stream << prefix << "pos : " << pos_ << CRLF;
}

//------------------------------------------------------------------------------

word LibraryErrSet::Error(string& expl) const
{
   std::ostringstream stream;

   if(err_ == InterpreterError)
   {
      stream << err_;
      expl = stream.str();
      return -7;
   }

   auto pointer = CliBuffer::ErrorPointer;
   stream << spaces(pos_ - strlen(pointer)) << pointer << CRLF;
   stream << spaces(2) << err_;
   expl = stream.str();
   return -2;
}

//------------------------------------------------------------------------------

word LibraryErrSet::Fix(CliThread& cli, FixOptions& opts, string& expl) const
{
   Debug::ft("LibraryErrSet.Fix");

   return Error(expl);
}

//------------------------------------------------------------------------------

word LibraryErrSet::Format(string& expl) const
{
   Debug::ft("LibraryErrSet.Format");

   return Error(expl);
}

//------------------------------------------------------------------------------

word LibraryErrSet::Parse(string& expl, const string& opts) const
{
   Debug::ft("LibraryErrSet.Parse");

   return Error(expl);
}

//------------------------------------------------------------------------------

word LibraryErrSet::PreAssign(string& expl) const
{
   Debug::ft("LibraryErrSet.PreAssign");

   return Error(expl);
}

//------------------------------------------------------------------------------

word LibraryErrSet::Scan
   (ostream& stream, const string& pattern, string& expl) const
{
   Debug::ft("LibraryErrSet.Scan");

   return Error(expl);
}

//------------------------------------------------------------------------------

word LibraryErrSet::Sort(ostream& stream, string& expl) const
{
   Debug::ft("LibraryErrSet.Sort");

   return Error(expl);
}

//------------------------------------------------------------------------------

void LibraryErrSet::to_str(stringVector& strings, bool verbose) const
{
   Debug::ft("LibraryErrSet.to_str");

   string result;
   Error(result);
   strings.push_back(result);
}
}
