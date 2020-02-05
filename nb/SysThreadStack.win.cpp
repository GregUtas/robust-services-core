//==============================================================================
//
//  SysThreadStack.win.cpp
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
#ifdef OS_WIN
#include "SysThreadStack.h"
#include <cstddef>
#include <cstring>
#include <sstream>
#include <string>
#include <windows.h>  // must precede dbghelp.h
#include <dbghelp.h>
#include "Debug.h"
#include "Formatters.h"
#include "Log.h"
#include "Memory.h"
#include "NbLogs.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Global data for capturing stack traces.
//
class StackInfo
{
public:
   //  Deleted because this class only has static members.
   //
   StackInfo() = delete;

   //  Loads symbol information on startup.
   //
   static DWORD Startup();

   //  Releases symbol information on shutdown.
   //
   static void Shutdown();

   //  Captures the running thread's stack frames.  Returns the number of
   //  functions on the stack.
   //
   static fn_depth GetFrames();

   //  Returns a handle to the stack frame at DEPTH.
   //
   static DWORD64 GetFrame(fn_depth depth);

   //  Returns the name of the function associated with FRAME.
   //
   static const char* GetFunction(DWORD64 frame);

   //  Returns the name of the file where the function associated with FRAME
   //  is implemented, updating LINE and DISP to the line number and offset
   //  within that file.
   //
   static const char* GetFileLoc(DWORD64 frame, DWORD& line, DWORD& disp);

   //> The maximum number of frames that RtlCaptureStackBackTrace can capture.
   //  On Windows XP, it's 62.  In later versions of Windows, it's UINT16_MAX.
   //
   static const size_t MaxFrames = 2048;
private:
   //  A handle to our process.
   //
   static HANDLE Process;

   //  Symbol information.
   //
   static SYMBOL_INFO* Symbols;

   //  The number of stack frames in Frames.
   //
   static WORD Depth;

   //  An array of pointers, one to each stack frame.
   //
   static void* Frames[MaxFrames];

   //  File name and line number information for a function.
   //
   static IMAGEHLP_LINE64 Source;
};

HANDLE StackInfo::Process = nullptr;
SYMBOL_INFO* StackInfo::Symbols = nullptr;
WORD StackInfo::Depth = 0;
void* StackInfo::Frames[MaxFrames] = { nullptr };
IMAGEHLP_LINE64 StackInfo::Source = { };

//------------------------------------------------------------------------------

const char* StackInfo::GetFileLoc(DWORD64 frame, DWORD& line, DWORD& disp)
{
   if(!SymGetLineFromAddr64(Process, frame, &disp, &Source)) return nullptr;
   line = Source.LineNumber;
   return Source.FileName;
}

//------------------------------------------------------------------------------

DWORD64 StackInfo::GetFrame(fn_depth depth)
{
   if(depth < Depth) return DWORD64(Frames[depth]);
   return 0;
}

//------------------------------------------------------------------------------

fn_depth StackInfo::GetFrames()
{
   Depth = RtlCaptureStackBackTrace(0, MaxFrames, Frames, nullptr);
   return Depth;
}

//------------------------------------------------------------------------------

const char* StackInfo::GetFunction(DWORD64 frame)
{
   if(!SymFromAddr(Process, frame, nullptr, Symbols)) return nullptr;
   return Symbols->Name;
}

//------------------------------------------------------------------------------

void StackInfo::Shutdown()
{
   Memory::Free(Symbols);
   Symbols = nullptr;
   SymCleanup(GetCurrentProcess());
}

//------------------------------------------------------------------------------

DWORD StackInfo::Startup()
{
   //  Allocate memory for, and load, symbol information.  We want to be able
   //  to map a function return address to a specific line number in a source
   //  code file, and we want demangled function names.
   //
   if(Symbols != nullptr) return 0;

   auto size = sizeof(SYMBOL_INFO) + MAX_SYM_NAME;
   Symbols = (SYMBOL_INFO*) Memory::Alloc(size, MemPerm, false);
   if(Symbols == nullptr) return ERROR_NOT_ENOUGH_MEMORY;

   Process = GetCurrentProcess();

   if(!SymInitialize(Process, nullptr, true))
   {
      Memory::Free(Symbols);
      Symbols = nullptr;
      return GetLastError();
   }

   auto options = SymGetOptions();
   options |= (SYMOPT_LOAD_LINES | SYMOPT_UNDNAME);
   SymSetOptions(options);

   //  Initialize other fields required when interpreting a stack frame.
   //
   Symbols->SizeOfStruct = sizeof(SYMBOL_INFO);
   Symbols->MaxNameLen = MAX_SYM_NAME;
   Source.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
   return 0;
}

//==============================================================================

fn_name SysThreadStack_Display = "SysThreadStack.Display";

void SysThreadStack::Display(ostream& stream, fn_depth omit)
{
   Debug::ft(SysThreadStack_Display);

   auto depth = StackInfo::GetFrames();
   if(depth == 0) return;

   //  XLO and XHI limit the traceback's display to 48 functions, namely
   //  the 28 uppermost and the 20 lowermost functions.
   //
   string prefix = Log::Tab + spaces(2);
   string name;
   DWORD line;
   DWORD disp;
   auto xlo = omit + 1 + 20;
   auto xhi = depth - 1 - 28;

   stream << Log::Tab << "Function Traceback:" << CRLF;

   for(auto f = omit + 1; f < depth; ++f)
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

         //  Get the name of the function associated with this stack frame.
         //  Modify the name by replacing each C++ scope operator with a dot.
         //
         auto frame = StackInfo::GetFrame(f);
         auto func = StackInfo::GetFunction(frame);

         if(func != nullptr)
         {
            name = func;
            ReplaceScopeOperators(name);
            stream << name << " @ ";

            //  Get the source code filename and line number where this
            //  function invoked the next one on the stack.  Modify the
            //  filename by removing the directory path.
            //
            auto file = StackInfo::GetFileLoc(frame, line, disp);

            if(file != nullptr)
            {
               name = file;
               auto pos = name.rfind(BACKSLASH);
               if(pos >= 0) name = name.erase(0, pos + 1);
               stream << name << " + " << line << '[' << disp << ']';
            }
            else
            {
               stream << "<unknown file> (err=" << GetLastError() << ')';
            }
         }
         else
         {
            stream << "<unknown function> (err=" << GetLastError() << ')';
         }

         stream << CRLF;
      }
   }
}

//------------------------------------------------------------------------------

fn_depth SysThreadStack::FuncDepth()
{
   //  Exclude this function from the depth count.
   //
   auto depth = StackInfo::GetFrames();
   return (depth > 0 ? depth - 1 : 0);
}

//------------------------------------------------------------------------------

fn_name SysThreadStack_Shutdown = "SysThreadStack.Shutdown";

void SysThreadStack::Shutdown(RestartLevel level)
{
   Debug::ft(SysThreadStack_Shutdown);

   //  When actually exiting the process, unload symbol information.
   //
   if(level >= RestartReboot)
   {
      StackInfo::Shutdown();
   }
}

//------------------------------------------------------------------------------

fn_name SysThreadStack_Startup = "SysThreadStack.Startup";

void SysThreadStack::Startup(RestartLevel level)
{
   Debug::ft(SysThreadStack_Startup);

   auto errval = StackInfo::Startup();
   if(errval == 0) return;

   auto log = Log::Create(NodeLogGroup, NodeNoSymbolInfo);

   if(log != nullptr)
   {
      *log << Log::Tab << "errval=" << errval;
      Log::Submit(log);
   }
}

//------------------------------------------------------------------------------

fn_name SysThreadStack_TrapIsOk = "SysThreadStack.TrapIsOk";

bool SysThreadStack::TrapIsOk()
{
   Debug::ft(SysThreadStack_TrapIsOk);

   //  Do not trap a thread that is currently executing a destructor.
   //
   auto depth = StackInfo::GetFrames();
   if(depth == 0) return true;

   for(auto f = 2; f < depth; ++f)
   {
      auto func = StackInfo::GetFunction(StackInfo::GetFrame(f));

      if(func != nullptr)
      {
         if(strchr(func, '~') != nullptr) return false;
         if(strstr(func, "operator delete") != nullptr) return false;
      }
   }

   return true;
}
}
#endif
