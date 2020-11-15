//==============================================================================
//
//  Base.cpp
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
#include "Base.h"
#include <new>
#include <ostream>
#include "Debug.h"
#include "Formatters.h"
#include "NbTypes.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
void Base::ClaimBlocks()
{
   Debug::ft("Base.ClaimBlocks");

   std::vector< Base* > objects;

   //  Claim this object and all of the objects that it owns.
   //
   GetSubtended(objects);

   for(size_t i = 0; i < objects.size(); ++i)
   {
      objects[i]->Claim();
   }
}

//------------------------------------------------------------------------------

void Base::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   stream << prefix << strClass(this) << CRLF;
   stream << prefix << "this : " << this << CRLF;
}

//------------------------------------------------------------------------------

void Base::GetSubtended(std::vector< Base* >& objects) const
{
   Debug::ft("Base.GetSubtended");

   objects.push_back(const_cast< Base* >(this));
}

//------------------------------------------------------------------------------

void Base::LogSubtended(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Debug::ft("Base.LogSubtended");

   std::vector< Base* > objects;

   GetSubtended(objects);

   for(size_t i = 0; i < objects.size(); ++i)
   {
      if(i > 0) stream << prefix << string(60 - prefix.size(), '-') << CRLF;
      objects[i]->Display(stream, prefix, options);
   }
}

//------------------------------------------------------------------------------

void Base::Nullify(size_t n)
{
   //  Set this object's vptr to a value that will cause a trap if a virtual
   //  function on the object is invoked.
   //
   auto obj = reinterpret_cast< ObjectStruct* >(this);
   obj->vptr = BAD_POINTER;

   if(n > 0)
   {
      //  Nullify the object's data so that N bytes (including the vptr) end
      //  up being nullified.
      //
      n = (n >> BYTES_PER_WORD_LOG2) - 1;
      for(size_t i = 0; i < n; ++i) obj->data[i] = BAD_POINTER;
   }
}

//------------------------------------------------------------------------------

void* Base::operator new(size_t size)
{
   Debug::ft("Base.operator new");

   return ::operator new(size);
}

//------------------------------------------------------------------------------

void* Base::operator new[](size_t size)
{
   Debug::ft("Base.operator new[]");

   return ::operator new[](size);
}

//------------------------------------------------------------------------------

void* Base::operator new(size_t size, void* place)
{
   Debug::ft("Base.operator new(place)");

   return place;
}

//------------------------------------------------------------------------------

void* Base::operator new[](size_t size, void* place)
{
   Debug::ft("Base.operator new[](place)");

   return place;
}

//------------------------------------------------------------------------------

void Base::Output(ostream& stream, col_t indent, bool verbose) const
{
   auto opts = (verbose ? VerboseOpt : NoFlags);
   Display(stream, spaces(indent), opts);
}

//------------------------------------------------------------------------------

Base::vptr_t Base::Vptr() const
{
   //  Return this object's vptr.
   //
   auto obj = reinterpret_cast< const ObjectStruct* >(this);
   return obj->vptr;
}
}
