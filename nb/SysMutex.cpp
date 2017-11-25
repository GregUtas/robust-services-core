//==============================================================================
//
//  SysMutex.cpp
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
