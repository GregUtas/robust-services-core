//==============================================================================
//
//  IoThread.cpp
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
#include "IoThread.h"
#include <sstream>
#include <string>
#include "Debug.h"
#include "InputHandler.h"
#include "IpBuffer.h"
#include "IpPort.h"
#include "IpService.h"
#include "Log.h"
#include "Memory.h"
#include "NbTypes.h"
#include "NwLogs.h"
#include "SysSocket.h"

using namespace NodeBase;
using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NetworkBase
{
const size_t IoThread::MaxRxBuffSize = 64 * kBs;
const size_t IoThread::MaxTxBuffSize = 64 * kBs;

//------------------------------------------------------------------------------

fn_name IoThread_ctor = "IoThread.ctor";

IoThread::IoThread(Daemon* daemon, const IpService* service, ipport_t port) :
   Thread(service->GetFaction(), daemon),
   port_(port),
   ipPort_(nullptr),
   recvs_(0),
   buffer_(nullptr),
   rxSize_(service->RxSize()),
   txSize_(service->TxSize())
{
   Debug::ft(IoThread_ctor);

   if(rxSize_ == 0) rxSize_ = MaxRxBuffSize >> 2;

   if(rxSize_ > MaxRxBuffSize)
   {
      Debug::SwLog(IoThread_ctor, "rx size", rxSize_);
      rxSize_ = MaxRxBuffSize;
   }

   if(txSize_ == 0) txSize_ = MaxTxBuffSize >> 2;

   if(txSize_ > MaxTxBuffSize)
   {
      Debug::SwLog(IoThread_ctor, "tx size", txSize_);
      txSize_ = MaxTxBuffSize;
   }

   buffer_ = (byte_t*) Memory::Alloc(SysSocket::MaxMsgSize, MemDynamic);
}

//------------------------------------------------------------------------------

IoThread::~IoThread()
{
   Debug::ftnt("IoThread.dtor");

   if(ipPort_ != nullptr) ipPort_->SetThread(nullptr);
}

//------------------------------------------------------------------------------

bool IoThread::ConditionalPause(word percent)
{
   Debug::ft("IoThread.ConditionalPause");

   if(RtcPercentUsed() > percent)
   {
      ipPort_->RecvsInSequence(recvs_);
      Pause();
      recvs_ = 0;
      return true;
   }

   return false;
}

//------------------------------------------------------------------------------

void IoThread::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Thread::Display(stream, prefix, options);

   stream << prefix << "port   : " << port_ << CRLF;
   stream << prefix << "ipPort : " << ipPort_ << CRLF;
   stream << prefix << "self   : " << self_.to_str() << CRLF;
   stream << prefix << "recvs  : " << recvs_ << CRLF;
   stream << prefix << "txAddr : " << txAddr_.to_str(true) << CRLF;
   stream << prefix << "rxAddr : " << rxAddr_.to_str(true) << CRLF;
   stream << prefix << "time   : " << time_.Ticks() << CRLF;
   stream << prefix << "buffer : " << buffer_ << CRLF;
   stream << prefix << "rxSize : " << rxSize_ << CRLF;
   stream << prefix << "txSize : " << txSize_ << CRLF;
}

//------------------------------------------------------------------------------

bool IoThread::ExitOnRestart(RestartLevel level) const
{
   Debug::ft("IoThread.ExitOnRestart");

   //  Don't exit the thread during a warm restart.  Sessions survive, so
   //  we should continue to service our socket(s) as soon as the restart
   //  is over.
   //
   return (level >= RestartCold);
}

//------------------------------------------------------------------------------

void IoThread::InvokeHandler
   (const IpPort& port, const byte_t* source, size_t size) const
{
   Debug::ft("IoThread.InvokeHandler");

   auto handler = port.GetHandler();

   while(size > 0)
   {
      byte_t* dest = nullptr;
      auto rcvd = size;
      auto socket = rxAddr_.GetSocket();
      IpBufferPtr buff(handler->AllocBuff(source, size, dest, rcvd, socket));
      if(buff == nullptr) return;
      if(rcvd == 0) return;

      //  If the input handler cannot receive the message, it should
      //  generate a log and delete the buffer.  Just returning a nil
      //  DEST is naughty.
      //
      if(dest == nullptr)
      {
         port.InvalidDiscarded();

         auto log = Log::Create(NetworkLogGroup, NetworkNoDestination);

         if(log != nullptr)
         {
            *log << Log::Tab << "port=" << port_;
            *log << " size=" << size << CRLF;
            buff->Display(*log, Log::Tab, VerboseOpt);
            Log::Submit(log);
         }

         return;
      }

      //  Copy RCVD bytes from SOURCE to DEST and pass the message to
      //  the input handler.
      //
      handler->NetworkToHost(*buff, dest, source, rcvd);
      buff->SetRxAddr(rxAddr_);
      buff->SetTxAddr(txAddr_);
      buff->SetRxTime(time_);
      handler->ReceiveBuff(buff, rcvd, port.GetService()->GetFaction());

      if(rcvd >= size) return;
      source += rcvd;
      size -= rcvd;
   }
}

//------------------------------------------------------------------------------

void IoThread::Patch(sel_t selector, void* arguments)
{
   Thread::Patch(selector, arguments);
}
}
