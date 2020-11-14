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
CodeSet::CodeSet(const string& name, SetOfIds* set) : LibrarySet(name),
   set_(set)
{
   Debug::ft("CodeSet.ctor");

   if(set_ == nullptr) set_.reset(new SetOfIds);
}

//------------------------------------------------------------------------------

CodeSet::~CodeSet()
{
   Debug::ftnt("CodeSet.dtor");
}

//------------------------------------------------------------------------------

LibrarySet* CodeSet::Assign(LibrarySet* rhs)
{
   Debug::ft("CodeSet.Assign");

   auto that = static_cast< CodeSet* >(rhs);

   if(that->IsTemporary())
   {
      set_.reset(that->set_.release());
      that->Release();
   }
   else
   {
      *set_ = *that->set_;
   }

   return this;
}

//------------------------------------------------------------------------------

word CodeSet::Count(string& result) const
{
   Debug::ft("CodeSet.Count");

   auto count = set_->size();
   return Counted(result, &count);
}

//------------------------------------------------------------------------------

LibrarySet* CodeSet::Difference(const LibrarySet* rhs) const
{
   Debug::ft("CodeSet.Difference");

   auto result = new SetOfIds;
   auto that = static_cast< const CodeSet* >(rhs);
   SetDifference(*result, *this->set_, *that->set_);
   return Create(TemporaryName(), result);
}

//------------------------------------------------------------------------------

void CodeSet::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   LibrarySet::Display(stream, prefix, options);

   stream << prefix << "set  : " << set_.get() << CRLF;

   if(set_ != nullptr)
   {
      stream << prefix << spaces(2) << "size  : " << set_->size() << CRLF;
      stream << prefix << spaces(2) << "items : " << CRLF;

      auto lead = prefix + spaces(4);

      for(auto i = set_->cbegin(); i != set_->cend(); ++i)
      {
         stream << lead << *i << CRLF;
      }
   }
}

//------------------------------------------------------------------------------

LibrarySet* CodeSet::Intersection(const LibrarySet* rhs) const
{
   Debug::ft("CodeSet.Intersection");

   auto result = new SetOfIds;
   auto that = static_cast< const CodeSet* >(rhs);
   SetIntersection(*result, *this->set_, *that->set_);
   return Create(TemporaryName(), result);
}

//------------------------------------------------------------------------------

LibrarySet* CodeSet::Union(const LibrarySet* rhs) const
{
   Debug::ft("CodeSet.Union");

   auto result = new SetOfIds;
   auto that = static_cast< const CodeSet* >(rhs);
   SetUnion(*result, *this->set_, *that->set_);
   return Create(TemporaryName(), result);
}
}
