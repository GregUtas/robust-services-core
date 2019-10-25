//==============================================================================
//
//  TraceDump.cpp
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
#include "TraceDump.h"
#include <ostream>
#include "Debug.h"
#include "FunctionTrace.h"
#include "Singleton.h"
#include "TraceBuffer.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
fixed_string StartOfTrace = "START OF TRACE";
fixed_string EndOfTrace = "END OF TRACE";

//                               1         2         3         4         5
//                      1234567890123456789012345678901234567890123456789012
fixed_string Header1 = "mm:ss.ttt  Thr  Event  TotalTime   NetTime  Function";
fixed_string Header2 = "---------  ---  -----  ---------   -------  --------";

//------------------------------------------------------------------------------

fixed_string BlockedStr = "Functions not captured because buffer was locked: ";

fn_name TraceDump_Generate = "TraceDump.Generate";

TraceRc TraceDump::Generate(ostream& stream, bool diff)
{
   Debug::ft(TraceDump_Generate);

   FunctionTrace::Postprocess();

   auto buff = Singleton< TraceBuffer >::Instance();

   stream << StartOfTrace << buff->strTimePlace() << CRLF << CRLF;

   auto blocks = buff->Blocks();
   auto overflow = buff->HasOverflowed();

   if(blocks > 0) stream << BlockedStr << blocks << CRLF;
   if(overflow) stream << TraceBuffer::OverflowStr << CRLF;
   if((blocks > 0) || overflow) stream << CRLF;

   stream << Header1 << CRLF;
   stream << Header2 << CRLF;

   //  Step through the trace buffer, displaying a trace record if the
   //  tool that created it is enabled.  This allows a single trace to
   //  be output several times, focusing on a different subset of the
   //  trace records each time.
   //
   buff->SetTool(ToolBuffer, true);
   buff->Lock();
   {
      TraceRecord* rec = nullptr;
      auto mask = Flags().set();

      for(buff->Next(rec, mask); rec != nullptr; buff->Next(rec, mask))
      {
         if(buff->ToolIsOn(rec->Owner()))
         {
            if(rec->Display(stream, diff)) stream << CRLF;
         }
      }
   }
   buff->Unlock();
   buff->SetTool(ToolBuffer, false);

   stream << EndOfTrace << CRLF;
   return TraceOk;
}

//------------------------------------------------------------------------------

const string& TraceDump::Tab()
{
   static const string TabStr(TabWidth, SPACE);

   return TabStr;
}
}
