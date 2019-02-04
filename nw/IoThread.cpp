//==============================================================================
//
//  IoThread.cpp
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
#include "IoThread.h"
#include <sstream>
#include <string>
#include "Debug.h"
#include "Formatters.h"
#include "InputHandler.h"
#include "IpBuffer.h"
#include "IpPort.h"
#include "IpService.h"
#include "Log.h"
#include "Memory.h"
#include "SysSocket.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NetworkBase
{
const size_t IoThread::MaxRxBuffSize = 64 * 1024;  // 64KB
const size_t IoThread::MaxTxBuffSize = 64 * 1024;  // 64KB

//------------------------------------------------------------------------------

fn_name IoThread_ctor = "IoThread.ctor";

IoThread::IoThread(const IpService* service, ipport_t port) :
   Thread(service->GetFaction()),
   port_(port),
   ipPort_(nullptr),
   rxSize_(service->RxSize()),
   txSize_(service->TxSize()),
   recvs_(0),
   ticks0_(0),
   buffer_(nullptr)
{
   Debug::ft(IoThread_ctor);

   if(rxSize_ == 0) rxSize_ = MaxRxBuffSize >> 2;

   if(rxSize_ > MaxRxBuffSize)
   {
      Debug::SwLog(IoThread_ctor, rxSize_, 0);
      rxSize_ = MaxRxBuffSize;
   }

   if(txSize_ == 0) txSize_ = MaxTxBuffSize >> 2;

   if(txSize_ > MaxTxBuffSize)
   {
      Debug::SwLog(IoThread_ctor, txSize_, 1);
      txSize_ = MaxTxBuffSize;
   }

   buffer_ = (byte_t*) Memory::Alloc(SysSocket::MaxMsgSize, MemDyn);
}

//------------------------------------------------------------------------------

fn_name IoThread_dtor = "IoThread.dtor";

IoThread::~IoThread()
{
   Debug::ft(IoThread_dtor);

   if(ipPort_ != nullptr) ipPort_->SetThread(nullptr);
}

//------------------------------------------------------------------------------

fn_name IoThread_ConditionalPause = "IoThread.ConditionalPause";

bool IoThread::ConditionalPause(word percent)
{
   Debug::ft(IoThread_ConditionalPause);

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
   stream << prefix << "rxSize : " << rxSize_ << CRLF;
   stream << prefix << "txSize : " << txSize_ << CRLF;
   stream << prefix << "host   : " << host_.to_str() << CRLF;
   stream << prefix << "recvs  : " << recvs_ << CRLF;
   stream << prefix << "txAddr : " << txAddr_.to_string() << CRLF;
   stream << prefix << "rxAddr : " << rxAddr_.to_string() << CRLF;
   stream << prefix << "ticks0 : " << ticks0_ << CRLF;
   stream << prefix << "buffer : " << strPtr(buffer_) << CRLF;
}

//------------------------------------------------------------------------------

fn_name IoThread_ExitOnRestart = "IoThread.ExitOnRestart";

bool IoThread::ExitOnRestart(RestartLevel level) const
{
   Debug::ft(IoThread_ExitOnRestart);

   //  Don't exit the thread during a warm restart.  Sessions survive, so
   //  we should continue to service our socket(s) as soon as the restart
   //  is over.
   //
   return (level >= RestartCold);
}

//------------------------------------------------------------------------------

fn_name IoThread_InsertSocket = "IoThread.InsertSocket";

bool IoThread::InsertSocket(SysSocket* socket)
{
   Debug::ft(IoThread_InsertSocket);

   return false;
}

//------------------------------------------------------------------------------

fn_name IoThread_InvokeHandler = "IoThread.InvokeHandler";

void IoThread::InvokeHandler(const IpPort& port,
   const byte_t* source, MsgSize size) const
{
   Debug::ft(IoThread_InvokeHandler);

   auto handler = port.GetHandler();

   while(size > 0)
   {
      byte_t* dest = nullptr;
      MsgSize rcvd = size;
      IpBufferPtr buff(handler->AllocBuff(source, size, dest, rcvd));
      if(buff == nullptr) return;
      if(rcvd == 0) return;

      //  If the input handler cannot receive the message, it should
      //  generate a log and delete the buffer.  Just returning a nil
      //  DEST is naughty.
      //
      if(dest == nullptr)
      {
         port.InvalidDiscarded();

         auto log = Log::Create("NO DESTINATION FROM INPUT HANDLER");

         if(log != nullptr)
         {
            *log << "port=" << port_;
            *log << " size=" << size << CRLF;
            buff->Display(*log, EMPTY_STR, Flags(Vb_Mask));
            Log::Spool(log);
         }

         return;
      }

      //  Copy RCVD bytes from SOURCE to DEST and pass the message to
      //  the input handler.
      //
      NetworkToHost(dest, source, rcvd, port.GetService()->WordSize());
      buff->SetRxAddr(rxAddr_);
      buff->SetTxAddr(txAddr_);
      buff->SetRxTicks(ticks0_);
      handler->ReceiveBuff(rcvd, buff, port.GetService()->GetFaction());

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
