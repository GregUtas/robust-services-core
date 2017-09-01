//==============================================================================
//
//  FunctionGuard.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
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
private:
   //  Overridden to prohibit copying.
   //
   FunctionGuard(const FunctionGuard& that);
   void operator=(const FunctionGuard& that);

   //  The first function that was invoked.
   //
   First first_;
};
}
#endif