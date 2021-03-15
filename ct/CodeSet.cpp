//==============================================================================
//
//  CodeSet.cpp
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
#include "CodeSet.h"
#include <ostream>
#include "Debug.h"
#include "Formatters.h"
#include "SetOperations.h"
#include "SysTypes.h"

using namespace NodeBase;
using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace CodeTools
{
CodeSet::CodeSet(const string& name, const LibItemSet* items) : LibrarySet(name)
{
   Debug::ft("CodeSet.ctor");

   if(items != nullptr) Items() = *items;
}

//------------------------------------------------------------------------------

CodeSet::~CodeSet()
{
   Debug::ftnt("CodeSet.dtor");
}

//------------------------------------------------------------------------------

LibrarySet* CodeSet::Assign(LibrarySet* that)
{
   Debug::ft("CodeSet.Assign");

   this->Items() = that->Items();
   return this;
}

//------------------------------------------------------------------------------

word CodeSet::Count(string& result) const
{
   Debug::ft("CodeSet.Count");

   return Counted(result, Items().size());
}

//------------------------------------------------------------------------------

LibrarySet* CodeSet::Difference(const LibrarySet* that) const
{
   Debug::ft("CodeSet.Difference");

   LibItemSet result;
   SetDifference(result, this->Items(), that->Items());
   return Create(TemporaryName(), &result);
}

//------------------------------------------------------------------------------

void CodeSet::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   LibrarySet::Display(stream, prefix, options);

   stream << prefix << "items (" << Items().size() << ") :" << CRLF;

   auto lead = prefix + spaces(2);

   for(auto i = Items().cbegin(); i != Items().cend(); ++i)
   {
      stream << lead << strObj(*i, false) << CRLF;
   }
}

//------------------------------------------------------------------------------

LibrarySet* CodeSet::Intersection(const LibrarySet* that) const
{
   Debug::ft("CodeSet.Intersection");

   LibItemSet result;
   SetIntersection(result, this->Items(), that->Items());
   return Create(TemporaryName(), &result);
}

//------------------------------------------------------------------------------

LibrarySet* CodeSet::Union(const LibrarySet* that) const
{
   Debug::ft("CodeSet.Union");

   LibItemSet result;
   SetUnion(result, this->Items(), that->Items());
   return Create(TemporaryName(), &result);
}
}
