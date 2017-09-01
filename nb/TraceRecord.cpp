//==============================================================================
//
//  TraceRecord.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "TraceRecord.h"
#include <ostream>
#include <string>
#include "Formatters.h"
#include "Singleton.h"
#include "TraceBuffer.h"
#include "TraceDump.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
TraceRecord::TraceRecord(size_t size, FlagId owner) :
   size_(int16_t(size)),
   owner_(owner),
   rid_(InvalidId)
{
}

//------------------------------------------------------------------------------

bool TraceRecord::Display(ostream& stream)
{
   stream << spaces(TraceDump::StartToEvt) << EventString() << TraceDump::Tab();
   return true;
}

//------------------------------------------------------------------------------

const char* TraceRecord::EventString() const
{
   static const string BlankEventStr(TraceDump::EvtWidth, SPACE);

   return BlankEventStr.c_str();
}

//------------------------------------------------------------------------------

void* TraceRecord::operator new(size_t size)
{
   return Singleton< TraceBuffer >::Instance()->AddRecord(size);
}

//------------------------------------------------------------------------------

void* TraceRecord::operator new(size_t size, void* where)
{
   return where;
}
}
