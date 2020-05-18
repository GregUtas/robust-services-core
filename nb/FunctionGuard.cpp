//==============================================================================
//
//  FunctionGuard.cpp
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
#include "FunctionGuard.h"
#include "Debug.h"
#include "SysTypes.h"
#include "Thread.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
fn_name FunctionGuard_ctor = "FunctionGuard.ctor";

FunctionGuard::FunctionGuard(GuardedFunction first, bool invoke) :
   first_(Guard_Nil)
{
   Debug::ft(FunctionGuard_ctor);

   if(!invoke) return;

   switch(first)
   {
   case Guard_MakeUnpreemptable:
      first_ = first;
      Thread::MakeUnpreemptable();
      return;
   case Guard_MakePreemptable:
      first_ = first;
      Thread::MakePreemptable();
      return;
   case Guard_MemUnprotect:
      first_ = first;
      Thread::MemUnprotect();
      return;
   case Guard_ImmUnprotect:
      first_ = first;
      Thread::ImmUnprotect();
      return;
   default:
      Debug::SwLog(FunctionGuard_ctor, "unexpected function", first_);
   }
}

//------------------------------------------------------------------------------

fn_name FunctionGuard_dtor = "FunctionGuard.dtor";

FunctionGuard::~FunctionGuard()
{
   Debug::ftnt(FunctionGuard_dtor);

   if(first_ != Guard_Nil) Release();
}

//------------------------------------------------------------------------------

fn_name FunctionGuard_Release = "FunctionGuard.Release";

void FunctionGuard::Release()
{
   Debug::ftnt(FunctionGuard_Release);

   auto first = first_;
   first_ = Guard_Nil;

   switch(first)
   {
   case Guard_MakeUnpreemptable:
      Thread::MakePreemptable();
      return;
   case Guard_MakePreemptable:
      Thread::MakeUnpreemptable();
      return;
   case Guard_MemUnprotect:
      Thread::MemProtect();
      return;
   case Guard_ImmUnprotect:
      Thread::ImmProtect();
      return;
   default:
      return;
   }
}
}
