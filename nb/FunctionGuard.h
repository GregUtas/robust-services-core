//==============================================================================
//
//  FunctionGuard.h
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
#ifndef FUNCTIONGUARD_H_INCLUDED
#define FUNCTIONGUARD_H_INCLUDED

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Invokes a function and, when it goes out of scope, the function's conjugate.
//
class FunctionGuard
{
public:
   enum First
   {
      NilFunction,        // no function to invoke or reverse
      MakeUnpreemptable,  // Thread::MakeUnpreemptable...MakePreemptable
      MakePreemptable,    // Thread::MakePreemptable...MakeUnpreemptable
      MemUnprotect        // Thread::MemUnprotect...MemProtect
   };

   //  Invokes the function associated with FIRST if INVOKE is set.
   //
   FunctionGuard(First first, bool invoke = true);

   //  Invokes the conjugate function if the constructor invoked FIRST.
   //
   ~FunctionGuard();

   //  Deleted to prohibit copying.
   //
   FunctionGuard(const FunctionGuard& that) = delete;
   FunctionGuard& operator=(const FunctionGuard& that) = delete;
private:
   //  The first function that was invoked.
   //
   First first_;
};
}
#endif