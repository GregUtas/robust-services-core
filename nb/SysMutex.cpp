//==============================================================================
//
//  SysMutex.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "SysMutex.h"
#include <ostream>
#include <string>
#include "Singleton.h"
#include "SysTypes.h"
#include "ThreadRegistry.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
void SysMutex::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Permanent::Display(stream, prefix, options);

   stream << prefix << "mutex : " << mutex_ << CRLF;
   stream << prefix << "nid   : " << nid_ << CRLF;
   stream << prefix << "owner : " << owner_ << CRLF;
}

//------------------------------------------------------------------------------

Thread* SysMutex::Owner() const
{
   if(owner_ != nullptr) return owner_;
   if(nid_ == NIL_ID) return nullptr;
   return Singleton< ThreadRegistry >::Instance()->FindThread(nid_);
}

//------------------------------------------------------------------------------

void SysMutex::Patch(sel_t selector, void* arguments)
{
   Permanent::Patch(selector, arguments);
}
}
