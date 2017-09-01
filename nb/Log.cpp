//==============================================================================
//
//  Log.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "Log.h"
#include <ios>
#include <iosfwd>
#include <sstream>
#include "Clock.h"
#include "Debug.h"
#include "Element.h"
#include "LogThread.h"

using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
fn_name Log_Create = "Log.Create";

ostringstreamPtr Log::Create(fixed_string title)
{
   Debug::ft(Log_Create);

   //  Insert a blank line at the top of the log to separate it from the
   //  previous log (or the CLI prompt, if it is written to the console).
   //
   ostringstreamPtr stream(new std::ostringstream);

   if(stream != nullptr)
   {
      *stream << std::boolalpha << std::nouppercase << CRLF;
      *stream << title << SPACE << Element::strTimePlace() << CRLF;
   }

   return stream;
}

//------------------------------------------------------------------------------

string Log::FileName()
{
   return "logs" + Clock::TimeZeroStr();
}

//------------------------------------------------------------------------------

fn_name Log_Spool = "Log.Spool";

void Log::Spool(ostringstreamPtr& log)
{
   Debug::ft(Log_Spool);

   LogThread::Spool(log);
}
}
