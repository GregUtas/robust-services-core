//==============================================================================
//
//  Exception.cpp
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
#include "Exception.h"
#include <ios>
#include <utility>
#include "Debug.h"
#include "Duration.h"
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
   Thread::ResetDebugFlags();
   Debug::ft(Exception_ctor1);  //@

   //  Capturing a stack trace takes time, so give the thread an extra
   //  20 msecs.
   //
   Thread::ExtendTime(Duration(20, mSECS));

   //  When capturing the stack, exclude this constructor and those of
   //  our subclasses.
   //
   if(stack)
   {
      stack_.reset(new std::ostringstream);
      *stack_ << std::boolalpha << std::nouppercase;
      SysThreadStack::Display(*stack_, depth + 1);
   }
}

//------------------------------------------------------------------------------

fn_name Exception_ctor2 = "Exception.ctor(copy)";

Exception::Exception(const Exception& that) :
   exception(that),
   stack_(std::move(that.stack_))
{
   Debug::ft(Exception_ctor2);
}

//------------------------------------------------------------------------------

fn_name Exception_ctor3 = "Exception.ctor(move)";

Exception::Exception(Exception&& that) :
   exception(that),
   stack_(std::move(that.stack_))
{
   Debug::ft(Exception_ctor3);
}

//------------------------------------------------------------------------------

fn_name Exception_dtor = "Exception.dtor";

Exception::~Exception()
{
   Debug::ftnt(Exception_dtor);
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
