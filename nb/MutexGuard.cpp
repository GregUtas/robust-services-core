//==============================================================================
//
//  MutexGuard.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "MutexGuard.h"
#include "Clock.h"
#include "Debug.h"
#include "SysMutex.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
fn_name MutexGuard_ctor = "MutexGuard.ctor";

MutexGuard::MutexGuard(SysMutex* mutex) : mutex_(mutex)
{
   if(mutex_ == nullptr) return;

   Debug::ft(MutexGuard_ctor);
   auto rc = mutex_->Acquire(TIMEOUT_NEVER);
   Debug::Assert(rc == SysMutex::Acquired);
}

//------------------------------------------------------------------------------

fn_name MutexGuard_dtor = "MutexGuard.dtor";

MutexGuard::~MutexGuard()
{
   if(mutex_ != nullptr)
   {
      Debug::ft(MutexGuard_dtor);
      mutex_->Release();
      mutex_ = nullptr;
   }
}
}