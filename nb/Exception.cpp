//==============================================================================
//
//  Exception.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "Exception.h"
#include <ios>
#include <memory>
#include <sstream>
#include <utility>
#include "Debug.h"
#include "SysThreadStack.h"
#include "Thread.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
fn_name Exception_ctor1 = "Exception.ctor";

Exception::Exception(bool stack, fn_depth depth) : stack_(nullptr)
{
   //  Reenable Debug functions before tracing this function.
   //
   Debug::Reset();
   Debug::ft(Exception_ctor1);

   //  Exception handling and stack walking are slow, so give the thread
   //  another 200 msecs.
   //
   Thread::ExtendTime(200);

   //  When capturing the stack, exclude this constructor and those of
   //  our subclasses.
   //
   if(stack)
   {
      stack_.reset(new std::ostringstream);
      if(stack_ == nullptr) return;
      *stack_ << std::boolalpha << std::nouppercase;
      SysThreadStack::Display(*stack_, depth + 1);
   }
}

//------------------------------------------------------------------------------

fn_name Exception_ctor2 = "Exception.ctor(copy)";

Exception::Exception(const Exception& that)
{
   Debug::ft(Exception_ctor2);

   this->stack_ = std::move(that.stack_);
}

//------------------------------------------------------------------------------

fn_name Exception_ctor3 = "Exception.ctor(move)";

Exception::Exception(Exception&& that)
{
   Debug::ft(Exception_ctor3);

   this->stack_ = std::move(that.stack_);
}

//------------------------------------------------------------------------------

fn_name Exception_dtor = "Exception.dtor";

Exception::~Exception() noexcept
{
   Debug::ft(Exception_dtor);
}

//------------------------------------------------------------------------------

void Exception::Display(ostream& stream, const string& prefix) const
{
   //  There is nothing to display; the stack is provided separately.
}

//------------------------------------------------------------------------------

fixed_string ExceptionExpl = "Unspecified Exception";

const char* Exception::what() const noexcept
{
   //  Subclasses are supposed to override this.
   //
   return ExceptionExpl;
}
}
