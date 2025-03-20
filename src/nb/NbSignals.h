//==============================================================================
//
//  NbSignals.h
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
#ifndef NBSIGNALS_H_INCLUDED
#define NBSIGNALS_H_INCLUDED

#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  The following signals are proprietary and are used to throw a
//  SignalException outside the signal handler.
//
constexpr signal_t SIGNIL = 0;        // nil signal
constexpr signal_t SIGWRITE = 121;    // write to protected memory
constexpr signal_t SIGCLOSE = 122;    // exit thread (non-error)
constexpr signal_t SIGYIELD = 123;    // ran unpreemptably too long
constexpr signal_t SIGSTACK1 = 124;   // stack overflow: attempt recovery
constexpr signal_t SIGSTACK2 = 125;   // stack overflow: exit thread
constexpr signal_t SIGPURGE = 126;    // thread killed or suicided
constexpr signal_t SIGDELETED = 127;  // thread unexpectedly deleted

//  Creates signals during system initialization.
//
void CreatePosixSignals();
}
#endif
