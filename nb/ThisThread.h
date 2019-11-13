//==============================================================================
//
//  ThisThread.h
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
#ifndef THISTHREAD_H_INCLUDED
#define THISTHREAD_H_INCLUDED

#include <string>
#include "Clock.h"
#include "NbTypes.h"
#include "SysTypes.h"
#include "ToolTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
   //  Wrapper for Thread static functions.  It avoids the need for some
   //  application software to #include Thread.h.  The functions defined
   //  here simply invoke the eponymous functions on Thread.
   //
   //  The class FunctionGuard allows a function to guarantee pairing of
   //  MakePreemptable/MakeUnpreemptable and MemUnprotect/MemProtect.
   //
   namespace ThisThread
   {
      ThreadId RunningThreadId();
      void MakeUnpreemptable();
      void MakePreemptable();
      word RtcPercentUsed();
      DelayRc Pause(msecs_t msecs = TIMEOUT_IMMED);
      void PauseOver(word limit);
      bool EnterBlockingOperation(BlockingReason why, fn_name_arg func);
      void ExitBlockingOperation(fn_name_arg func);
      void MemUnprotect();
      void MemProtect();
      void IncludeInTrace();
      TraceRc StartTracing(const std::string& options);
      void StopTracing();
   }
}
#endif
