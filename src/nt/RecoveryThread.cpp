//==============================================================================
//
//  RecoveryThread.cpp
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
#include "RecoveryThread.h"
#include "Daemon.h"
#include "Protected.h"
#include <csignal>
#include <cstdlib>
#include <exception>
#include <ostream>
#include <string>
#include "Debug.h"
#include "Duration.h"
#include "FunctionGuard.h"
#include "Mutex.h"
#include "NbAppIds.h"
#include "Singleton.h"
#include "SoftwareException.h"
#include "SymbolRegistry.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeTools
{
//  Daemon for recreating RecoveryThread.
//
class RecoveryDaemon : public Daemon
{
   friend class Singleton<RecoveryDaemon>;

   RecoveryDaemon();
   ~RecoveryDaemon();
   Thread* CreateThread() override;
   AlarmStatus GetAlarmLevel() const override;
};

//------------------------------------------------------------------------------

fixed_string RecoveryDaemonName = "recover";

RecoveryDaemon::RecoveryDaemon() : Daemon(RecoveryDaemonName, 1)
{
   Debug::ft("RecoveryDaemon.ctor");

   auto reg = Singleton<SymbolRegistry>::Instance();
   reg->BindSymbol("recovery.daemon", Did(), false);
}

//------------------------------------------------------------------------------

RecoveryDaemon::~RecoveryDaemon()
{
   Debug::ftnt("RecoveryDaemon.dtor");
}

//------------------------------------------------------------------------------

Thread* RecoveryDaemon::CreateThread()
{
   Debug::ft("RecoveryDaemon.CreateThread");
   return Singleton<RecoveryThread>::Instance();
}

//------------------------------------------------------------------------------

AlarmStatus RecoveryDaemon::GetAlarmLevel() const
{
   Debug::ft("RecoveryDaemon.GetAlarmLevel");
   return MinorAlarm;
}

//------------------------------------------------------------------------------
//
//  Protected data for testing the mapping of SIGSEGV to SIGWRITE.
//
class ReadOnlyData : public Protected
{
public:
   ReadOnlyData() : data_(0) { }
   void SetData(int value) { data_ = value; }
private:
   int data_;
};

//------------------------------------------------------------------------------
//
//  Mutex for testing bad things occurring while holding a mutex.
//
static Mutex RecoveryMutex_("RecoveryTestMutex");

//==============================================================================

RecoveryThread::RecoveryThread() :
   Thread(LoadTestFaction, Singleton<RecoveryDaemon>::Instance()),
   test_(Sleep),
   signal_(0),
   prot_(nullptr)
{
   Debug::ft("RecoveryThread.ctor");

   auto reg = Singleton<SymbolRegistry>::Instance();
   reg->BindSymbol("recovery.thread", Tid(), false);

   //  Set ThreadCtorTrapFlag to cause a trap during thread creation.  This
   //  tests orphan recovery and a single daemon trap.  If ThreadCtorRetrapFlag
   //  is also set, it tests a double daemon trap, which should disable the
   //  daemon.  Reenabling the daemon will then recreate this thread.
   //
   if(Debug::SwFlagOn(ThreadCtorTrapFlag))
   {
      Debug::SetSwFlag(ThreadCtorTrapFlag, false);
      UseBadPointer();
   }

   if(Debug::SwFlagOn(ThreadCtorRetrapFlag))
   {
      Debug::SetSwFlag(ThreadCtorRetrapFlag, false);
      UseBadPointer();
   }

   SetInitialized();
}

//------------------------------------------------------------------------------

RecoveryThread::~RecoveryThread()
{
   Debug::ftnt("RecoveryThread.dtor");

   if(prot_ != nullptr)
   {
      FunctionGuard guard(Guard_MemUnprotect);
      delete prot_;
      prot_ = nullptr;
   }

   if(Debug::SwFlagOn(ThreadDtorTrapFlag))
   {
      Debug::SetSwFlag(ThreadDtorTrapFlag, false);
      UseBadPointer();
   }
}

//------------------------------------------------------------------------------

c_string RecoveryThread::AbbrName() const
{
   return RecoveryDaemonName;
}

//------------------------------------------------------------------------------

fn_name RecoveryThread_AcquireMutex = "RecoveryThread.AcquireMutex";

void RecoveryThread::AcquireMutex()
{
   Debug::ft(RecoveryThread_AcquireMutex);

   if(RecoveryMutex_.Acquire(TIMEOUT_IMMED))
   {
      Debug::SwLog(RecoveryThread_AcquireMutex, "acquire failed", 0);
   }
}

//------------------------------------------------------------------------------

void RecoveryThread::Destroy()
{
   Debug::ft("RecoveryThread.Destroy");

   Singleton<RecoveryThread>::Destroy();
}

//------------------------------------------------------------------------------

void RecoveryThread::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Thread::Display(stream, prefix, options);

   stream << prefix << "test   : " << int(test_) << CRLF;
   stream << prefix << "signal : " << signal_ << CRLF;
}

//------------------------------------------------------------------------------

void RecoveryThread::DoAbort()
{
   Debug::ft("RecoveryThread.DoAbort");

   std::abort();
}

//------------------------------------------------------------------------------

void RecoveryThread::DoDelete()
{
   Debug::ft("RecoveryThread.DoDelete");

   Singleton<RecoveryThread>::Destroy();
}

//------------------------------------------------------------------------------

int RecoveryThread::DoDivide(int dividend, int divisor)
{
   Debug::ft("RecoveryThread.DoDivide");

   return (dividend / divisor);
}

//------------------------------------------------------------------------------

void RecoveryThread::DoException()
{
   Debug::ft("RecoveryThread.DoException");

   throw SoftwareException("software exception test", 1);
}

//------------------------------------------------------------------------------

void RecoveryThread::DoRaise() const
{
   Debug::ft("RecoveryThread.DoRaise");

   raise(signal_);
}

//------------------------------------------------------------------------------

void RecoveryThread::DoTerminate()
{
   Debug::ft("RecoveryThread.DoTerminate");

   std::terminate();
}

//------------------------------------------------------------------------------

void RecoveryThread::DoTrap()
{
   Debug::ft("RecoveryThread.DoTrap");
   Raise(signal_);
}

//------------------------------------------------------------------------------

fn_name RecoveryThread_Enter = "RecoveryThread.Enter";

void RecoveryThread::Enter()
{
   while(true)
   {
      Debug::ft(RecoveryThread_Enter);

      //  Save and reset the test to be performed.  Otherwise, it will be
      //  immediately repeated upon reentering the thread after recovery.
      //
      auto test = test_;
      test_ = Sleep;

      //  Execute the requested test.
      //
      switch(test)
      {
      case Abort:
         DoAbort();
         break;
      case CtorTrap:
         Debug::SetSwFlag(ThreadCtorTrapFlag, true);
         return;
      case Delete:
         DoDelete();
         break;
      case DerefenceBadPtr:
         UseBadPointer();
         break;
      case DivideByZero:
         DoDivide(1, 0);
         break;
      case DtorTrap:
         Debug::SetSwFlag(ThreadDtorTrapFlag, true);
         return;
      case Exception:
         DoException();
         break;
      case InfiniteLoop:
         LoopForever();
         break;
      case MutexBlock:
         AcquireMutex();
         Pause(msecs_t(100));
         RecoveryMutex_.Release();
         break;
      case MutexExit:
         AcquireMutex();
         return;
      case MutexTrap:
         AcquireMutex();
         UseBadPointer();
         break;
      case OverflowStack:
         RecurseForever(1);
         break;
      case RaiseSignal:
         DoRaise();
         break;
      case Return:
         return;
      case Sleep:
         break;
      case Terminate:
         DoTerminate();
         break;
      case Trap:
         DoTrap();
         break;
      case Write:
         WriteToReadOnly();
         break;
      default:
         Debug::SwLog(RecoveryThread_Enter, "unexpected test", test);
      }

      //  Sleep until interrupted to perform the next test.  There is a timeout
      //  so that the thread will resume execution after it is deleted remotely
      //  (>recover delete f), after which it should exit.
      //
      Pause(msecs_t(5000));
   }
}

//------------------------------------------------------------------------------

fn_name RecoveryThread_LoopForever = "RecoveryThread.LoopForever";

void RecoveryThread::LoopForever()
{
   Debug::ft(RecoveryThread_LoopForever);

   while(true)
   {
      for(auto i = 0; i < 0x1000; ++i)
      {
         for(auto j = 0; j < 0x1000; ++j);
      }

      Debug::ft(RecoveryThread_LoopForever);
   }
}

//------------------------------------------------------------------------------

bool RecoveryThread::Recover()
{
   Debug::ft("RecoveryThread.Recover");

   if(Debug::SwFlagOn(ThreadRecoverTrapFlag)) UseBadPointer();
   return Debug::SwFlagOn(ThreadReenterFlag);
}

//------------------------------------------------------------------------------

void RecoveryThread::RecurseForever(size_t depth)
{
   Debug::ft("RecoveryThread.RecurseForever");

   RecurseForever(depth + 1);
}

//------------------------------------------------------------------------------

void RecoveryThread::UseBadPointer()
{
   Debug::ft("RecoveryThread.UseBadPointer");

   CauseTrap();
}

//------------------------------------------------------------------------------

void RecoveryThread::WriteToReadOnly()
{
   Debug::ft("RecoveryThread.WriteToReadOnly");

   if(prot_ == nullptr)
   {
      FunctionGuard guard(Guard_MemUnprotect);
      prot_ = new ReadOnlyData;
   }

   prot_->SetData(1);
}
}
