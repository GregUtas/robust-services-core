//==============================================================================
//
//  SysThreadStack.linux.cpp
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
#ifdef OS_LINUX

#include "SysThreadStack.h"
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <execinfo.h>
#include <memory>
#include <ostream>
#include <string>
#include "Debug.h"
#include "Formatters.h"
#include "Log.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
//  The maximum number of frames that will be .
//
constexpr size_t MaxFrames = 2048;

//  For holding stack frames.
//
typedef void* StackFrames[MaxFrames];

//  For holding stack frames.
//
typedef std::unique_ptr< void*[] > StackFramesPtr;

//------------------------------------------------------------------------------

void SysThreadStack::Display(ostream& stream) NO_FT
{
   StackFramesPtr frames(new StackFrames);

   auto depth = backtrace(frames.get(), MaxFrames);
   if(depth == 0)
   {
      stream << "function traceback unavailable" << CRLF;
      return;
   }

   auto fnames = backtrace_symbols(frames.get(), depth);
   if(fnames == nullptr)
   {
      stream << "function traceback unavailable" << CRLF;
      return;
   }

   //  XLO and XHI limit the traceback's display to 48 functions,
   //  namely the 28 uppermost and the 20 lowermost functions.
   //
   string prefix = Log::Tab + spaces(2);
   string name;
   auto xlo = 30;
   auto xhi = depth - 21;

   stream << Log::Tab << "Function Traceback:" << CRLF;

   for(auto f = 2; f < depth; ++f)
   {
      if((f >= xlo) && (f <= xhi))
      {
         if(f == xlo)
         {
            stream << prefix << "..." << (xhi - xlo + 1)
               << " functions omitted." << CRLF;
         }
      }
      else
      {
         stream << prefix;
         name = fnames[f];

         if(!name.empty())
         {
            ReplaceScopeOperators(name);
            stream << name;
         }
         else
         {
            stream << "unknown function";
         }

         stream << CRLF;
      }
   }

   //  Free the memory that backtrace_symbols() allocated for the function
   //  names using malloc().
   //
   free(fnames);
}

//------------------------------------------------------------------------------

fn_depth SysThreadStack::FuncDepth()
{
   //  Exclude this function from the depth count.  We're only interested
   //  in the current function's depth, so Frames needn't be per-thread.
   //
   static void* Frames[MaxFrames];

   auto depth = backtrace(Frames, MaxFrames);
   return (depth > 0 ? depth - 1 : 0);
}

//------------------------------------------------------------------------------

void SysThreadStack::Shutdown(RestartLevel level)
{
   Debug::ft("SysThreadStack.Shutdown");
}

//------------------------------------------------------------------------------

void SysThreadStack::Startup(RestartLevel level)
{
   Debug::ft("SysThreadStack.Startup");
}

//------------------------------------------------------------------------------

bool SysThreadStack::TrapIsOk() NO_FT
{
   StackFramesPtr frames(new StackFrames);

   auto depth = backtrace(frames.get(), MaxFrames);
   if(depth == 0) return true;

   auto fnames = backtrace_symbols(frames.get(), depth);
   if(fnames == nullptr) return true;

   for(auto f = 2; f < depth; ++f)
   {
      if(strchr(fnames[f], '~') != nullptr) return false;
      if(strstr(fnames[f], "operator delete") != nullptr) return false;
   }

   //  Free the memory that backtrace_symbols() allocated for the function
   //  names using malloc().
   //
   free(fnames);
   return true;
}
}
#endif
