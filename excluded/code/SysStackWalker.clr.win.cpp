//==============================================================================
//
//  SysStackWalker.clr.win.cpp
//
//  Copyright (C) 2012-2014 Greg Utas.  All rights reserved.
//
//  This implementation of SysStackWalker contains managed code and therefore
//  requires the /clr compiler option.  It is slower than the pure C++ version
//  in SysStackWalker.win.cpp because
//  (a) function calls sometimes incur managed to native transitions;
//  (b) to find its depth on the stack, FuncDepth must create a full StackTrace
//      instance, whereas the pure C++ version can use the far more efficient
//      RtlCaptureStackBackTrace, which fails in a managed code environment.
//  This implementation has been preserved because it works even in a release
//  build, something that still needs to be tested using the pure C++ version.
//
#include "SysStackWalker.h"
#include "Debug.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

bool SysStackWalker::Initialized = false;

//------------------------------------------------------------------------------

const string SysStackWalker_Display = "SysStackWalker.Display";

void SysStackWalker::Display(ostream& stream, fn_depth_t omit)
{
   Debug::ft(&SysStackWalker_Display);

   using namespace System;
   using namespace System::Diagnostics;
   using namespace System::Reflection;
   using namespace Runtime::InteropServices;

   StackTrace ^st = gcnew StackTrace(true);
   StackFrame ^sf;
   MethodBase ^mb;
   String     ^mn, ^fn, ^s;

   auto max = st->FrameCount;
   auto xlo = omit + 1 + 12;
   auto xhi = max - 13;

   stream << "Function Traceback:" << endl;

   for(auto f = omit + 1; f < max; ++f)
   {
      if((f >= xlo) && (f <= xhi))
      {
         if(f == xlo)
         {
            stream << "  ..." << (xhi - xlo + 1);
            stream << " functions omitted." << endl;
         }

         continue;
      }

      sf = st->GetFrame(f);

      if(sf != nullptr)
      {
         mb = sf->GetMethod();
         if(mb != nullptr)
            mn = mb->ToString();
         else
            mn = "<unknown function>";
      }
      else
      {
         mn = "<unknown function>";
      }

      mn = "  " + mn + " @ ";

      fn = sf->GetFileName();

      if(fn != nullptr)
      {
         auto i = fn->LastIndexOf('\\');
         if(i >= 0) fn = fn->Remove(0, i + 1);
         fn += " + ";
         i = sf->GetFileLineNumber();

         if(i == 0)
            fn += "<unknown line>";
         else
            fn += i;
      }
      else
      {
         fn = "<unknown file>";
      }

      s = mn + fn;
      auto c = (const char*)(Marshal::StringToHGlobalAnsi(s)).ToPointer();
      string m = c;
      stream << m << endl;
      Marshal::FreeHGlobal(IntPtr((void*) c));
   }

   s = nullptr;
   fn = nullptr;
   mn = nullptr;
   mb = nullptr;
   sf = nullptr;
   st = nullptr;
}

//------------------------------------------------------------------------------

fn_depth_t SysStackWalker::FuncDepth()
{
   //  Counting stack frames by constructing a StackTrace is slow.
   //  Following stack pointers would be much faster, but it's also
   //  platform dependent and error prone.
   //
   using namespace System::Diagnostics;

   StackTrace ^st = gcnew StackTrace(false);  // false = omit symbol info
   fn_depth_t depth = st->FrameCount;
   st = nullptr;                              // in case this helps GC
   return depth - 1;                          // omit this function
}

//------------------------------------------------------------------------------

void SysStackWalker::Shutdown(RestartLevel level) { }

//------------------------------------------------------------------------------

void SysStackWalker::Startup(RestartLevel level) { }