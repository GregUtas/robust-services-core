//==============================================================================
//
//  Log.cpp
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
