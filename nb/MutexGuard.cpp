//==============================================================================
//
//  MutexGuard.cpp
//
//  Copyright (C) 2013-2021  Greg Utas
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
#include "MutexGuard.h"
#include "Debug.h"
#include "Duration.h"
#include "SysMutex.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
MutexGuard::MutexGuard(SysMutex* mutex) : mutex_(mutex)
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
}
