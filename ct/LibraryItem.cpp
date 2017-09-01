//==============================================================================
//
//  LibraryItem.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "LibraryItem.h"
#include <ostream>
#include "Debug.h"
#include "SysTypes.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace CodeTools
{
fn_name LibraryItem_ctor = "LibraryItem.ctor";

LibraryItem::LibraryItem(const string& name) : name_(name)
{
   Debug::ft(LibraryItem_ctor);
}

//------------------------------------------------------------------------------

fn_name LibraryItem_dtor = "LibraryItem.dtor";

LibraryItem::~LibraryItem()
{
   Debug::ft(LibraryItem_dtor);
}

//------------------------------------------------------------------------------

void LibraryItem::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Temporary::Display(stream, prefix, options);

   stream << prefix << "name : " << name_ << CRLF;
}
}
