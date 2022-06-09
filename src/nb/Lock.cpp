//==============================================================================
//
//  Lock.cpp
//
//  Copyright (C) 2013-2022  Greg Utas
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
#include "Lock.h"
#include <ostream>
#include "Debug.h"
#include "SysThread.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
LockGuard::LockGuard(Lock* lock) : lock_(lock)
{
   if(lock_ == nullptr) return;

   Debug::ft("LockGuard.ctor");

   lock_->Acquire();
}

//------------------------------------------------------------------------------

LockGuard::~LockGuard()
{
   Debug::ftnt("LockGuard.dtor");

   if(lock_ != nullptr) Release();
}

//------------------------------------------------------------------------------

void LockGuard::Release()
{
   if(lock_ != nullptr)
   {
      Debug::ftnt("LockGuard.Release");
      lock_->Release();
      lock_ = nullptr;
   }
}

//==============================================================================

Lock::Lock() : owner_(NIL_ID) { }

//------------------------------------------------------------------------------

Lock::~Lock()
{
   if(owner_ != NIL_ID)
   {
      Debug::SwLog("Lock.dtor", "lock has owner", owner_);
   }
}

//------------------------------------------------------------------------------

void Lock::Acquire()
{
   auto curr = SysThread::RunningThreadId();
   if(owner_ == curr) return;
   mutex_.lock();
}

//------------------------------------------------------------------------------

void Lock::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   stream << prefix << "owner : " << owner_ << CRLF;
}

//------------------------------------------------------------------------------

void Lock::Release()
{
   auto curr = SysThread::RunningThreadId();
   if(owner_ != curr) return;

   //  Clear owner_ first, in case releasing the mutex results in another
   //  thread acquiring it and running immediately, in which case it will
   //  set owner_ itself.
   //
   owner_ = NIL_ID;
   mutex_.unlock();
}
}
