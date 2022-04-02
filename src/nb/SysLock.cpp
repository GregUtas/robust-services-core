//==============================================================================
//
//  SysLock.cpp
//
//  Copyright (C) 2013-2022  Greg Utas
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
#include "SysLock.h"
#include <ostream>

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
void SysLock::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   stream << prefix << "mutex : " << mutex_ << CRLF;
   stream << prefix << "owner : " << owner_ << CRLF;
}
}
