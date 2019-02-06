//==============================================================================
//
//  IpPort.cpp
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
#include "IpPort.h"
#include "Dynamic.h"
#include <ostream>
#include <string>
#include "Algorithms.h"
#include "Debug.h"
#include "Formatters.h"
#include "InputHandler.h"
#include "IoThread.h"
#include "IpPortRegistry.h"
#include "IpService.h"
#include "NbSignals.h"
#include "NbTypes.h"
#include "Singleton.h"
#include "Statistics.h"
#include "SysSocket.h"
#include "SysTypes.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NetworkBase
{
//  Statistics for each IP port.
//
class IpPortStats : public Dynamic
{
public:
   IpPortStats();
   ~IpPortStats();

   CounterPtr       recvs_;
   AccumulatorPtr   bytesRcvd_;
   HighWatermarkPtr maxBytesRcvd_;
   HighWatermarkPtr maxRecvs_;
   CounterPtr       discards_;
   CounterPtr       rejects_;
   CounterPtr       sends_;
   AccumulatorPtr   bytesSent_;
   HighWatermarkPtr maxBytesSent_;
   CounterPtr       overflows_;
};

//------------------------------------------------------------------------------

fn_name IpPortStats_ctor = "IpPortStats.ctor";

IpPortStats::IpPortStats()
{
   Debug::ft(IpPortStats_ctor);

   recvs_.reset(new Counter("receive operations"));
   bytesRcvd_.reset(new Accumulator("bytes received"));
   maxBytesRcvd_.reset(new HighWatermark("most bytes received"));
   maxRecvs_.reset(new HighWatermark("most receives before yielding"));
   discards_.reset(new Counter("messages discarded by input handler"));
   rejects_.reset(new Counter("ingress work rejected by input handler"));
   sends_.reset(new Counter("send operations"));
   bytesSent_.reset(new Accumulator("bytes sent"));
   maxBytesSent_.reset(new HighWatermark("most bytes sent"));
   overflows_.reset(new Counter("connection rejected: socket array full"));
}

//------------------------------------------------------------------------------

fn_name IpPortStats_dtor = "IpPortStats.dtor";

IpPortStats::~IpPortStats()
{
   Debug::ft(IpPortStats_dtor);
}

//==============================================================================

fn_name IpPort_ctor = "IpPort.ctor";

IpPort::IpPort(ipport_t port, const IpService* service) :
   port_(port),
   service_(service),
   handler_(nullptr),
   thread_(nullptr),
   socket_(nullptr)
{
   Debug::ft(IpPort_ctor);

   stats_.reset(new IpPortStats);
   Singleton< IpPortRegistry >::Instance()->BindPort(*this);
}

//------------------------------------------------------------------------------

fn_name IpPort_dtor = "IpPort.dtor";

IpPort::~IpPort()
{
   Debug::ft(IpPort_dtor);

   //  Shut down any I/O thread.  It must delete its socket(s).
   //  Deregister the port.
   //
   if(thread_ != nullptr)
   {
      thread_->Raise(SIGCLOSE);
      thread_ = nullptr;
   }

   Singleton< IpPortRegistry >::Instance()->UnbindPort(*this);
}

//------------------------------------------------------------------------------

fn_name IpPort_BindHandler = "IpPort.BindHandler";

bool IpPort::BindHandler(InputHandler& handler)
{
   Debug::ft(IpPort_BindHandler);

   handler_.reset(&handler);

   //  If the port does not have an I/O thread, create one.
   //
   if(thread_ != nullptr) return true;
   thread_ = CreateIoThread();
   return (thread_ != nullptr);
}

//------------------------------------------------------------------------------

fn_name IpPort_BytesRcvd = "IpPort.BytesRcvd";

void IpPort::BytesRcvd(size_t count) const
{
   Debug::ft(IpPort_BytesRcvd);

   stats_->recvs_->Incr();
   stats_->bytesRcvd_->Add(count);
   stats_->maxBytesRcvd_->Update(count);
}

//------------------------------------------------------------------------------

fn_name IpPort_BytesSent = "IpPort.BytesSent";

void IpPort::BytesSent(size_t count) const
{
   Debug::ft(IpPort_BytesSent);

   stats_->sends_->Incr();
   stats_->bytesSent_->Add(count);
   stats_->maxBytesSent_->Update(count);
}

//------------------------------------------------------------------------------

fn_name IpPort_CreateAppSocket = "IpPort.CreateAppSocket";

SysTcpSocket* IpPort::CreateAppSocket()
{
   Debug::ft(IpPort_CreateAppSocket);

   //  This function must be overridden by ports that require it.
   //
   return nullptr;
}

//------------------------------------------------------------------------------

fn_name IpPort_CreateIoThread = "IpPort.CreateIoThread";

IoThread* IpPort::CreateIoThread()
{
   Debug::ft(IpPort_CreateIoThread);

   //  This function must be overridden by ports with input handlers.
   //
   Debug::SwLog(IpPort_CreateIoThread, service_->Name(), port_);
   return nullptr;
}

//------------------------------------------------------------------------------

size_t IpPort::Discards() const
{
   return stats_->rejects_->Curr();
}

//------------------------------------------------------------------------------

void IpPort::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Protected::Display(stream, prefix, options);

   stream << prefix << "link    : " << link_.to_str() << CRLF;
   stream << prefix << "port    : " << port_ << CRLF;
   stream << prefix << "service : " << strObj(service_) << CRLF;
   stream << prefix << "handler : " << strObj(handler_.get()) << CRLF;
   stream << prefix << "thread  : " << strObj(thread_) << CRLF;
   stream << prefix << "socket  : " << strObj(socket_) << CRLF;
}

//------------------------------------------------------------------------------

fn_name IpPort_DisplayStats = "IpPort.DisplayStats";

void IpPort::DisplayStats(ostream& stream) const
{
   Debug::ft(IpPort_DisplayStats);

   auto name = service_->Name();

   stream << spaces(2) << name << SPACE << strIndex(port_, 0, false) << CRLF;

   stats_->recvs_->DisplayStat(stream);
   stats_->bytesRcvd_->DisplayStat(stream);
   stats_->maxBytesRcvd_->DisplayStat(stream);
   stats_->maxRecvs_->DisplayStat(stream);
   stats_->discards_->DisplayStat(stream);
   stats_->rejects_->DisplayStat(stream);
   stats_->sends_->DisplayStat(stream);
   stats_->bytesSent_->DisplayStat(stream);
   stats_->maxBytesSent_->DisplayStat(stream);
   stats_->overflows_->DisplayStat(stream);
}

//------------------------------------------------------------------------------

fn_name IpPort_IngressDiscarded = "IpPort.IngressDiscarded";

void IpPort::IngressDiscarded() const
{
   Debug::ft(IpPort_IngressDiscarded);

   stats_->rejects_->Incr();
}

//------------------------------------------------------------------------------

fn_name IpPort_InvalidDiscarded = "IpPort.InvalidDiscarded";

void IpPort::InvalidDiscarded() const
{
   Debug::ft(IpPort_InvalidDiscarded);

   stats_->discards_->Incr();
}

//------------------------------------------------------------------------------

ptrdiff_t IpPort::LinkDiff()
{
   int local;
   auto fake = reinterpret_cast< const IpPort* >(&local);
   return ptrdiff(&fake->link_, fake);
}

//------------------------------------------------------------------------------

void IpPort::Patch(sel_t selector, void* arguments)
{
   Protected::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name IpPort_PollArrayOverflow = "IpPort.PollArrayOverflow";

void IpPort::PollArrayOverflow() const
{
   Debug::ft(IpPort_PollArrayOverflow);

   stats_->overflows_->Incr();
}

//------------------------------------------------------------------------------

fn_name IpPort_RecvsInSequence = "IpPort.RecvsInSequence";

void IpPort::RecvsInSequence(size_t count) const
{
   Debug::ft(IpPort_RecvsInSequence);

   stats_->maxRecvs_->Update(count);
}

//------------------------------------------------------------------------------

fn_name IpPort_SetSocket = "IpPort.SetSocket";

bool IpPort::SetSocket(SysSocket* socket)
{
   Debug::ft(IpPort_SetSocket);

   //  Handle deregistration.
   //
   if(socket == nullptr)
   {
      socket_ = nullptr;
      return true;
   }

   //  The port must already have an input handler and I/O thread.
   //
   if(handler_ == nullptr)
   {
      Debug::SwLog(IpPort_SetSocket, port_, 0);
      return false;
   }

   if(thread_ == nullptr)
   {
      Debug::SwLog(IpPort_SetSocket, port_, 1);
      return false;
   }

   //  If a socket already exists, generate a log before overwriting it.
   //
   if(socket_ != nullptr)
   {
      Debug::SwLog(IpPort_SetSocket, port_, 2);
   }

   socket_ = socket;
   return true;
}

//------------------------------------------------------------------------------

fn_name IpPort_SetThread = "IpPort.SetThread";

void IpPort::SetThread(IoThread* thread)
{
   Debug::ft(IpPort_SetThread);

   //  Handle deregistration.
   //
   if(thread == nullptr)
   {
      thread_ = nullptr;
      return;
   }

   //  If a thread already exists, generate a log before overwriting it.
   //
   if(thread_ != nullptr)
   {
      Debug::SwLog(IpPort_SetThread, port_, 0);
   }

   thread_ = thread;
}

//------------------------------------------------------------------------------

fn_name IpPort_Shutdown = "IpPort.Shutdown";

void IpPort::Shutdown(RestartLevel level)
{
   Debug::ft(IpPort_Shutdown);

   if(level < RestartCold) return;

   stats_.release();
   socket_ = nullptr;
}

//------------------------------------------------------------------------------

fn_name IpPort_Startup = "IpPort.Startup";

void IpPort::Startup(RestartLevel level)
{
   Debug::ft(IpPort_Startup);

   if(stats_ == nullptr) stats_.reset(new IpPortStats);

   //  If the port has an input handler, make sure that it has an I/O thread.
   //
   if((handler_ != nullptr) && (thread_ == nullptr))
   {
      thread_ = CreateIoThread();
   }
}

//------------------------------------------------------------------------------

fn_name IpPort_UnbindHandler = "IpPort.UnbindHandler";

void IpPort::UnbindHandler(const InputHandler& handler)
{
   Debug::ft(IpPort_UnbindHandler);

   //  Do nothing if a different handler is registered.
   //
   if(handler_.get() != &handler) return;

   //  If the port has an I/O thread, shut it down before releasing its
   //  input handler, which is currently undergoing deletion.
   //
   if(thread_ != nullptr)
   {
      thread_->Raise(SIGCLOSE);
      thread_ = nullptr;
   }

   handler_.release();
}
}
