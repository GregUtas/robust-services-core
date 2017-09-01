//==============================================================================
//
//  CliAppData.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
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
fn_name CliAppData_ctor = "CliAppData.ctor";

CliAppData::CliAppData(CliThread& cli, Id id) :
   cli_(&cli),
   id_(id)
{
   Debug::ft(CliAppData_ctor);

   cli_->SetAppData(this, id_);
}

//------------------------------------------------------------------------------

fn_name CliAppData_dtor = "CliAppData.dtor";

CliAppData::~CliAppData()
{
   Debug::ft(CliAppData_dtor);
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

fn_name CliAppData_EventOccurred = "CliAppData.EventOccurred";

void CliAppData::EventOccurred(Event evt)
{
   Debug::ft(CliAppData_EventOccurred);
}

//------------------------------------------------------------------------------

void CliAppData::Patch(sel_t selector, void* arguments)
{
   Temporary::Patch(selector, arguments);
}
}
