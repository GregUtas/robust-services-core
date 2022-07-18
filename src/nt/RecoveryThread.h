//==============================================================================
//
//  RecoveryThread.h
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
#ifndef RECOVERYTHREAD_H_INCLUDED
#define RECOVERYTHREAD_H_INCLUDED

#include "Thread.h"
#include <cstddef>
#include "NbTypes.h"
#include "SysTypes.h"

namespace NodeTools
{
   class ReadOnlyData;
}

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace NodeTools
{
//  Thread for testing the safety net.
//
class RecoveryThread : public Thread
{
   friend class Singleton<RecoveryThread>;
public:
   //  Safety net tests.
   //
   enum Test
   {
      Sleep,
      Abort,
      Create,
      CtorTrap,
      DtorTrap,
      Delete,
      DerefenceBadPtr,
      DivideByZero,
      Exception,
      InfiniteLoop,
      MutexBlock,
      MutexExit,
      MutexTrap,
      OverflowStack,
      RaiseSignal,
      Return,
      Terminate,
      Trap,
      Write
   };

   //  Specifies the test to be performed
   //
   void SetTest(Test test) { test_ = test; }

   //  Sets a POSIX signal to be raised.
   //
   void SetTestSignal(signal_t signal) { signal_ = signal; }

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;
private:
   //  Private because this is a singleton.
   //
   RecoveryThread();

   //  Private because this is a singleton.
   //
   ~RecoveryThread();

   //  Functions that perform specific tests.
   //
   static void AcquireMutex();
   static void DoAbort();
   static void DoDelete();
   static int DoDivide(int dividend, int divisor);
   static void DoException();
   static void DoTerminate();
   static void LoopForever();
   static void RecurseForever(size_t depth);
   static void UseBadPointer();
   void WriteToReadOnly();
   void DoRaise() const;
   void DoTrap();

   //  Overridden to return a name for the thread.
   //
   c_string AbbrName() const override;

   //  Overridden to delete the singleton.
   //
   void Destroy() override;

   //  Overridden to perform a safety net test when notified.
   //
   void Enter() override;

   //  Overridden to reenter or exit the thread based on the test being
   //  performed.
   //
   bool Recover() override;

   //  The test to be performed.
   //
   Test test_;

   //  The POSIX signal to be raised.
   //
   signal_t signal_;

   //  Some protected data for testing the mapping of SIGSEGV to SIGWRITE.
   //
   ReadOnlyData* prot_;
};
}
#endif
