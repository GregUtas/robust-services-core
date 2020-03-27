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
   default:
      Debug::SwLog(FunctionGuard_ctor, "unexpected function", first_);
   }
}

//------------------------------------------------------------------------------

fn_name FunctionGuard_dtor = "FunctionGuard.dtor";

FunctionGuard::~FunctionGuard()
{
   Debug::ft(FunctionGuard_dtor);

   if(first_ != NilFunction) Release();
}

//------------------------------------------------------------------------------

fn_name FunctionGuard_Release = "FunctionGuard.Release";

void FunctionGuard::Release()
{
   Debug::ft(FunctionGuard_Release);

   auto first = first_;
   first_ = NilFunction;

   switch(first)
   {
   case MakeUnpreemptable:
      ThisThread::MakePreemptable();
      return;
   case MakePreemptable:
      ThisThread::MakeUnpreemptable();
      return;
   case MemUnprotect:
      ThisThread::MemProtect();
      return;
   default:
      return;
   }
}
}