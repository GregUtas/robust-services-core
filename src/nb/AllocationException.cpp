//==============================================================================
//
//  AllocationException.cpp
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
#include "AllocationException.h"
#include <ostream>
#include <string>
#include "Debug.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
AllocationException::AllocationException(MemoryType type, size_t size) :
   Exception(true),
   type_(type),
   size_(size)
{
   Debug::ft("AllocationException.ctor");
}

//------------------------------------------------------------------------------

AllocationException::~AllocationException()
{
   Debug::ftnt("AllocationException.dtor");
}

//------------------------------------------------------------------------------

void AllocationException::Display(ostream& stream, const string& prefix) const
{
   Exception::Display(stream, prefix);

   stream << prefix << "type : " << type_ << CRLF;
   stream << prefix << "size : " << size_ << CRLF;
}

//------------------------------------------------------------------------------

fixed_string AllocationExceptionExpl = "Allocation Failure";

const char* AllocationException::what() const noexcept
{
   return AllocationExceptionExpl;
}
}
