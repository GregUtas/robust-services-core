//==============================================================================
//
//  SysTickTimer.cpp
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
#ifdef OS_WIN

#include "SysTickTimer.h"
#include <sys/timeb.h>
#include "Debug.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
SysTickTimer* SysTickTimer::Instance_ = nullptr;

//------------------------------------------------------------------------------

fn_name SysTickTimer_dtor = "SysTickTimer.dtor";

SysTickTimer::~SysTickTimer()
{
   Debug::ftnt(SysTickTimer_dtor);

   Debug::SwLog(SysTickTimer_dtor, UnexpectedInvocation, 0);
}

//------------------------------------------------------------------------------

SysTickTimer* SysTickTimer::Instance()
{
   if(Instance_ != nullptr) return Instance_;
   Instance_ = new SysTickTimer;
   return Instance_;
}
}
#endif
