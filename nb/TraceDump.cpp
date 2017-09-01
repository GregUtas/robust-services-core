//==============================================================================
//
//  TraceDump.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
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

//                          0         1         2         3         4         5
//                          0123456789012345678901234567890123456789012345678901
fixed_string TraceHeader1 = "mm:ss.ttt  Thr  Event  TotalTime   NetTime  Function";
fixed_string TraceHeader2 = "---------  ---  -----  ---------   -------  --------";

//------------------------------------------------------------------------------

fixed_string BlockedStr = "Functions not captured because buffer was locked: ";

fn_name TraceDump_Generate = "TraceDump.Generate";

TraceRc TraceDump::Generate(ostream& stream)
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

   stream << TraceHeader1 << CRLF;
   stream << TraceHeader2 << CRLF;

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
            if(rec->Display(stream)) stream << CRLF;
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
