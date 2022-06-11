//==============================================================================
//
//  SysStackTrace.linux.cpp
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

#include "SysStackTrace.h"
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <cxxabi.h>
#include <execinfo.h>
#include <memory>
#include <ostream>
#include "Debug.h"
#include "Formatters.h"
#include "Log.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
//  The maximum number of frames that will be captured.  Some
//  number between this and 700 causes a fatal crash on Linux.
//
constexpr size_t MaxFrames = 680;

//  For holding stack frames.
//
typedef void* StackFrames[MaxFrames];

//  For holding stack frames.
//
typedef std::unique_ptr<void*[]> StackFramesPtr;

//------------------------------------------------------------------------------

static string GetFunction(string& func)
{
   auto begin = func.find('(');
   if(begin == string::npos) return func;
   auto end = func.find_first_of("+)");
   if(end == string::npos) return func;
   auto name = func.substr(begin + 1, end - begin - 1);
   SysStackTrace::Demangle(name);
   ReplaceScopeOperators(name);
   func = func.substr(0, begin + 1) + name + func.substr(end);
   return func;
}

//------------------------------------------------------------------------------

void SysStackTrace::Demangle(string& name) NO_FT
{
   int status = 0;
   auto buffer = __cxxabiv1::__cxa_demangle
      (name.c_str(), nullptr, nullptr, &status);
   if(status == 0) name = buffer;
   free(buffer);
}

//------------------------------------------------------------------------------

void SysStackTrace::Display(ostream& stream) NO_FT
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
         string func(fnames[f]);
         stream << prefix << GetFunction(func) << CRLF;
      }
   }

   //  Free the memory that backtrace_symbols() allocated for the function
   //  names using malloc().
   //
   free(fnames);
}

//------------------------------------------------------------------------------

fn_depth SysStackTrace::FuncDepth()
{
   //  Exclude this function from the depth count.  We're only interested
   //  in the current function's depth, so Frames needn't be per-thread.
   //
   static void* Frames[MaxFrames];

   auto depth = backtrace(Frames, MaxFrames);
   return (depth > 0 ? depth - 1 : 0);
}

//------------------------------------------------------------------------------

void SysStackTrace::Shutdown(RestartLevel level)
{
   Debug::ft("SysStackTrace.Shutdown");
}

//------------------------------------------------------------------------------

void SysStackTrace::Startup(RestartLevel level)
{
   Debug::ft("SysStackTrace.Startup");
}

//------------------------------------------------------------------------------

bool SysStackTrace::TrapIsOk() NO_FT
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
