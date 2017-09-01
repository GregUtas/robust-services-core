//==============================================================================
//
//  NbSignals.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
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
