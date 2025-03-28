//==============================================================================
//
//  FunctionGuard.h
//
//  Copyright (C) 2013-2025  Greg Utas
//
//  This file is part of the Robust Services Core (RSC).
//
//  RSC is free software: you can redistribute it and/or modify it under the
//  terms of the Lesser GNU General Public License as published by the Free
//  Software Foundation, either version 3 of the License, or (at your option)
//  any later version.
//
//  RSC is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
//  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
//  details.
//
//  You should have received a copy of the Lesser GNU General Public License
//  along with RSC.  If not, see <http://www.gnu.org/licenses/>.
//
#ifndef FUNCTIONGUARD_H_INCLUDED
#define FUNCTIONGUARD_H_INCLUDED

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Functions supported by FunctionGuard.
//
enum GuardedFunction
{
   Guard_Nil,                // no function to invoke or reverse
   Guard_MakeUnpreemptable,  // ...Thread::MakePreemptable
   Guard_MakePreemptable,    // ...Thread::MakeUnpreemptable
   Guard_MemUnprotect,       // ...Thread::MemProtect
   Guard_ImmUnprotect        // ...Thread::ImmProtect
};

//------------------------------------------------------------------------------
//
//  Invokes a function and, when it goes out of scope, the function's conjugate.
//
class FunctionGuard
{
public:
   //  Invokes the function associated with FIRST if INVOKE is set.
   //
   FunctionGuard(GuardedFunction first, bool invoke = true);

   //  Invokes the conjugate function if the constructor invoked FIRST.
   //
   ~FunctionGuard();

   //  Deleted to prohibit copying.
   //
   FunctionGuard(const FunctionGuard& that) = delete;

   //  Deleted to prohibit copy assignment.
   //
   FunctionGuard& operator=(const FunctionGuard& that) = delete;

   //  Invokes the conjugate function if the constructor invoked FIRST.
   //  Used to invoke that function before the guard goes out of scope.
   //
   void Release();
private:
   //  The first function that was invoked.
   //
   GuardedFunction first_;
};
}
#endif
