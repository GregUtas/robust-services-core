//==============================================================================
//
//  AllocationException.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
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
fn_name AllocationException_ctor = "AllocationException.ctor";

AllocationException::AllocationException(MemoryType type, size_t size) :
   Exception(true, 1),
   type_(type),
   size_(size)
{
   Debug::ft(AllocationException_ctor);
}

//------------------------------------------------------------------------------

fn_name AllocationException_dtor = "AllocationException.dtor";

AllocationException::~AllocationException() noexcept
{
   Debug::ft(AllocationException_dtor);
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
