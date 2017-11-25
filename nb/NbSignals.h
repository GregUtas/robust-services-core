//==============================================================================
//
//  NbSignals.h
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
#ifndef NBSIGNALS_H_INCLUDED
#define NBSIGNALS_H_INCLUDED

//------------------------------------------------------------------------------

namespace NodeBase
{
   //  The following signals are proprietary and are used to throw a
   //  SignalException outside the signal handler.
   //
   enum NbSignalIds
   {
      SIGNIL = 0,       // nil signal
      SIGCLOSE = 120,   // exit thread (non-error)
      SIGYIELD = 121,   // ran unpreemptably too long
      SIGTRAPS = 122,   // trapped too many times
      SIGRETRAP = 123,  // trapped during recovery
      SIGSTACK1 = 124,  // stack overflow: attempt recovery
      SIGSTACK2 = 125,  // stack overflow: exit and recreate thread
      SIGPURGE = 126,   // thread killed or suicided
      SIGDELETED = 127  // thread unexpectedly deleted
   };

   //  Creates signals during system initialization.
   //
   void CreatePosixSignals();
}
#endif
