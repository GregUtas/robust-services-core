//==============================================================================
//
//  Mutex.cpp
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
#include "Mutex.h"
#include <chrono>
#include <cstdint>
#include <new>
#include <ostream>
#include "Algorithms.h"
#include "Debug.h"
#include "MutexRegistry.h"
#include "Singleton.h"
#include "SysThread.h"
#include "Thread.h"
#include "ThreadRegistry.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
MutexGuard::MutexGuard(Mutex* mutex) : mutex_(mutex)
{
   if(mutex_ == nullptr) return;

   Debug::ft("MutexGuard.ctor");

   mutex_->Acquire(TIMEOUT_NEVER);
}

//------------------------------------------------------------------------------

MutexGuard::~MutexGuard()
{
   Debug::ftnt("MutexGuard.dtor");

   if(mutex_ != nullptr) Release();
}

//------------------------------------------------------------------------------

void MutexGuard::Release()
{
   if(mutex_ != nullptr)
   {
      Debug::ftnt("MutexGuard.Release");
      mutex_->Release();
      mutex_ = nullptr;
   }
}

//==============================================================================

Mutex::Mutex(c_string name) :
   name_(name),
   nid_(NIL_ID),
   owner_(nullptr),
   locks_(0)
{
   Debug::ft("Mutex.ctor");

   Singleton<MutexRegistry>::Instance()->BindMutex(*this);
}

//------------------------------------------------------------------------------

fn_name Mutex_dtor = "Mutex.dtor";

Mutex::~Mutex()
{
   Debug::ftnt(Mutex_dtor);

   if(nid_ != NIL_ID)
   {
      Debug::SwLog(Mutex_dtor, name_, nid_);
   }

   Singleton<MutexRegistry>::Extant()->UnbindMutex(*this);
}

//------------------------------------------------------------------------------

bool Mutex::Acquire(const msecs_t& timeout)
{
   Debug::ftnt("Mutex.Acquire");

   auto curr = SysThread::RunningThreadId();

   if(nid_ == curr)
   {
      ++locks_;
      return true;
   }

   auto thr = Thread::RunningThread(std::nothrow);
   if(thr != nullptr) thr->UpdateMutex(this);
   auto locked = mutex_.try_lock_for(timeout);
   if(thr != nullptr) thr->UpdateMutex(nullptr);

   if(locked)
   {
      nid_ = curr;
      owner_ = thr;
      if(thr != nullptr) thr->UpdateMutexCount(true);
      locks_ = 1;
   }

   return locked;
}

//------------------------------------------------------------------------------

ptrdiff_t Mutex::CellDiff()
{
   uintptr_t local;
   auto fake = reinterpret_cast<const Mutex*>(&local);
   return ptrdiff(&fake->mid_, fake);
}

//------------------------------------------------------------------------------

void Mutex::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Permanent::Display(stream, prefix, options);

   stream << prefix << "name  : " << name_ << CRLF;
   stream << prefix << "mid   : " << mid_.to_str() << CRLF;
   stream << prefix << "nid   : " << nid_ << CRLF;
   stream << prefix << "owner : " << owner_ << CRLF;
   stream << prefix << "locks : " << locks_ << CRLF;
}

//------------------------------------------------------------------------------

Thread* Mutex::Owner() const
{
   if(owner_ != nullptr) return owner_;
   if(nid_ == NIL_ID) return nullptr;
   return Singleton<ThreadRegistry>::Instance()->FindThread(nid_);
}

//------------------------------------------------------------------------------

void Mutex::Patch(sel_t selector, void* arguments)
{
   Permanent::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name Mutex_Release = "Mutex.Release";

void Mutex::Release(bool abandon)
{
   Debug::ftnt(Mutex_Release);

   auto curr = SysThread::RunningThreadId();

   if(nid_ != curr)
   {
      Debug::SwLog(Mutex_Release, name_, pack2(curr, nid_));
      return;
   }

   if(!abandon && (--locks_ > 0)) return;

   //  Clear owner_ and nid_ first, in case releasing the mutex results in
   //  another thread acquiring the mutex, running immediately, and setting
   //  those fields to their new values.
   //
   if(owner_ != nullptr) owner_->UpdateMutexCount(false);
   owner_ = nullptr;
   nid_ = NIL_ID;
   mutex_.unlock();
}
}
