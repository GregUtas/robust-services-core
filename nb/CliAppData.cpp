//==============================================================================
//
//  CliAppData.cpp
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
#include "CliAppData.h"
#include <ostream>
#include <string>
#include "CliThread.h"
#include "Debug.h"
#include "SysTypes.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
CliAppData::CliAppData(CliThread& cli, Id id) :
   cli_(&cli),
   id_(id)
{
   Debug::ft("CliAppData.ctor");

   cli_->SetAppData(this, id_);
}

//------------------------------------------------------------------------------

CliAppData::~CliAppData()
{
   Debug::ftnt("CliAppData.dtor");
}

//------------------------------------------------------------------------------

void CliAppData::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Temporary::Display(stream, prefix, options);

   stream << prefix << "cli : " << cli_ << CRLF;
   stream << prefix << "id  : " << id_ << CRLF;
}

//------------------------------------------------------------------------------

void CliAppData::EventOccurred(Event event)
{
   Debug::ft("CliAppData.EventOccurred");
}

//------------------------------------------------------------------------------

void CliAppData::Patch(sel_t selector, void* arguments)
{
   Temporary::Patch(selector, arguments);
}
}
