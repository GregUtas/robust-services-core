//==============================================================================
//
//  FunctionGuard.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "FunctionGuard.h"
#include "Debug.h"
#include "SysTypes.h"
#include "ThisThread.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
fn_name FunctionGuard_ctor = "FunctionGuard.ctor";

FunctionGuard::FunctionGuard(First first, bool invoke) : first_(NilFunction)
{
   Debug::ft(FunctionGuard_ctor);

   if(!invoke) return;
   first_ = first;

   switch(first_)
   {
   case MakeUnpreemptable:
      ThisThread::MakeUnpreemptable();
      return;
   case MakePreemptable:
      ThisThread::MakePreemptable();
      return;
   case MemUnprotect:
      ThisThread::MemUnprotect();
      return;
   }
}

//------------------------------------------------------------------------------

fn_name FunctionGuard_dtor = "FunctionGuard.dtor";

FunctionGuard::~FunctionGuard()
{
   Debug::ft(FunctionGuard_dtor);

   switch(first_)
   {
   case NilFunction:
      return;
   case MakeUnpreemptable:
      ThisThread::MakePreemptable();
      return;
   case MakePreemptable:
      ThisThread::MakeUnpreemptable();
      return;
   case MemUnprotect:
      ThisThread::MemProtect();
      return;
   }
}
}