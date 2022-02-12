//==============================================================================
//
//  IpPort.cpp
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
#include "IpPort.h"
#include "Dynamic.h"
#include <cstdint>
#include <sstream>
#include <string>
#include "Alarm.h"
#include "AlarmRegistry.h"
#include "Algorithms.h"
#include "Debug.h"
#include "Formatters.h"
#include "FunctionGuard.h"
#include "IoThread.h"
#include "IpPortRegistry.h"
#include "IpService.h"
#include "Log.h"
#include "NbSignals.h"
#include "NbTypes.h"
#include "NwLogs.h"
#include "Restart.h"
#include "Singleton.h"
#include "Statistics.h"

using namespace NodeBase;
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

IpPortStats::IpPortStats()
{
   Debug::ft("IpPortStats.ctor");

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

IpPortStats::~IpPortStats()
{
   Debug::ftnt("IpPortStats.dtor");
}

//==============================================================================

IpPort::IpPort(ipport_t port, const IpService* service) :
   port_(port),
   service_(service),
   handler_(nullptr),
   thread_(nullptr),
   socket_(nullptr),
   alarm_(nullptr)
{
   Debug::ft("IpPort.ctor");

   EnsureAlarm();
   stats_.reset(new IpPortStats);
   Singleton< IpPortRegistry >::Instance()->BindPort(*this);
}

//------------------------------------------------------------------------------

IpPort::~IpPort()
{
   Debug::ftnt("IpPort.dtor");

   //  Shut down any I/O thread.  It must delete its socket(s).
   //  Deregister the port.
   //
   if(thread_ != nullptr)
   {
      thread_->Raise(SIGCLOSE);
      SetThread(nullptr);
   }

   Singleton< IpPortRegistry >::Extant()->UnbindPort(*this);
}

//------------------------------------------------------------------------------

bool IpPort::BindHandler(InputHandler& handler)
{
   Debug::ft("IpPort.BindHandler");

   handler_.reset(&handler);

   //  If the port does not have an I/O thread, create one.
   //
   if(thread_ != nullptr) return true;
   SetThread(CreateIoThread());
   return (thread_ != nullptr);
}

//------------------------------------------------------------------------------

void IpPort::BytesRcvd(size_t count) const
{
   Debug::ft("IpPort.BytesRcvd");

   stats_->recvs_->Incr();
   stats_->bytesRcvd_->Add(count);
   stats_->maxBytesRcvd_->Update(count);
}

//------------------------------------------------------------------------------

void IpPort::BytesSent(size_t count) const
{
   Debug::ft("IpPort.BytesSent");

   stats_->sends_->Incr();
   stats_->bytesSent_->Add(count);
   stats_->maxBytesSent_->Update(count);
}

//------------------------------------------------------------------------------

void IpPort::ClearAlarm() const
{
   Debug::ft("IpPort.ClearAlarm");

   if(alarm_ == nullptr) return;

   auto log = alarm_->Create(NetworkLogGroup, NetworkServiceAvailable, NoAlarm);
   if(log == nullptr) return;

   *log << Log::Tab << "service=" << service_->Name();
   *log << '(' << service_->Protocol() << ')';
   *log << " port=" << port_;
   Log::Submit(log);
}

//------------------------------------------------------------------------------

SysTcpSocket* IpPort::CreateAppSocket()
{
   Debug::ft("IpPort.CreateAppSocket");

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
   stream << prefix << "alarm   : " << strObj(alarm_) << CRLF;
}

//------------------------------------------------------------------------------

void IpPort::DisplayStats(ostream& stream, const Flags& options) const
{
   Debug::ft("IpPort.DisplayStats");

   auto name = service_->Name();

   stream << spaces(2) << name << SPACE << strIndex(port_, 0, false) << CRLF;

   stats_->recvs_->DisplayStat(stream, options);
   stats_->bytesRcvd_->DisplayStat(stream, options);
   stats_->maxBytesRcvd_->DisplayStat(stream, options);
   stats_->maxRecvs_->DisplayStat(stream, options);
   stats_->discards_->DisplayStat(stream, options);
   stats_->rejects_->DisplayStat(stream, options);
   stats_->sends_->DisplayStat(stream, options);
   stats_->bytesSent_->DisplayStat(stream, options);
   stats_->maxBytesSent_->DisplayStat(stream, options);
   stats_->overflows_->DisplayStat(stream, options);
}

//------------------------------------------------------------------------------

void IpPort::EnsureAlarm()
{
   Debug::ft("IpPort.EnsureAlarm");

   //  If the port's alarm is not registered, create it.
   //
   auto reg = Singleton< AlarmRegistry >::Instance();
   auto alarmName = "PORT" + std::to_string(port_);
   alarm_ = reg->Find(alarmName);

   if(alarm_ == nullptr)
   {
      auto alarmExpl = "Service unavailable: " + string(service_->Name());
      FunctionGuard guard(Guard_ImmUnprotect);
      alarm_ = new Alarm(alarmName.c_str(), alarmExpl.c_str(), 0);
   }
}

//------------------------------------------------------------------------------

void IpPort::IngressDiscarded() const
{
   Debug::ft("IpPort.IngressDiscarded");

   stats_->rejects_->Incr();
}

//------------------------------------------------------------------------------

void IpPort::InvalidDiscarded() const
{
   Debug::ft("IpPort.InvalidDiscarded");

   stats_->discards_->Incr();
}

//------------------------------------------------------------------------------

ptrdiff_t IpPort::LinkDiff()
{
   uintptr_t local;
   auto fake = reinterpret_cast< const IpPort* >(&local);
   return ptrdiff(&fake->link_, fake);
}

//------------------------------------------------------------------------------

void IpPort::Patch(sel_t selector, void* arguments)
{
   Protected::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

void IpPort::PollArrayOverflow() const
{
   Debug::ft("IpPort.PollArrayOverflow");

   stats_->overflows_->Incr();
}

//------------------------------------------------------------------------------

bool IpPort::RaiseAlarm(word errval) const
{
   Debug::ft("IpPort.RaiseAlarm");

   if(alarm_ == nullptr) return false;

   auto log = alarm_->Create
      (NetworkLogGroup, NetworkServiceFailure, MajorAlarm);
   if(log == nullptr) return false;

   *log << Log::Tab << "service=" << service_->Name();
   *log << '(' << service_->Protocol() << ')';
   *log << " port=" << port_;
   *log << " errval=" << errval;
   Log::Submit(log);
   return false;
}

//------------------------------------------------------------------------------

void IpPort::RecvsInSequence(size_t count) const
{
   Debug::ft("IpPort.RecvsInSequence");

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
      FunctionGuard guard(Guard_MemUnprotect);
      socket_ = nullptr;
      return true;
   }

   //  The port must already have an input handler and I/O thread.
   //
   if(handler_ == nullptr)
   {
      Debug::SwLog(IpPort_SetSocket, "input handler not found", port_);
      return false;
   }

   if(thread_ == nullptr)
   {
      Debug::SwLog(IpPort_SetSocket, "I/O thread not found", port_);
      return false;
   }

   //  If a socket already exists, generate a log before overwriting it.
   //
   if(socket_ != nullptr)
   {
      Debug::SwLog(IpPort_SetSocket, "socket already exists", port_);
   }

   FunctionGuard guard(Guard_MemUnprotect);
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
      FunctionGuard guard(Guard_MemUnprotect);
      thread_ = nullptr;
      return;
   }

   //  If another thread already exists, generate a log before overwriting it.
   //
   if((thread_ != nullptr) && (thread_ != thread))
   {
      Debug::SwLog(IpPort_SetThread, "I/O thread already exists", port_);
   }

   FunctionGuard guard(Guard_MemUnprotect);
   thread_ = thread;
}

//------------------------------------------------------------------------------

void IpPort::Shutdown(RestartLevel level)
{
   Debug::ft("IpPort.Shutdown");

   if(Restart::ClearsMemory(MemType())) return;

   FunctionGuard guard(Guard_MemUnprotect);
   Restart::Release(stats_);

   if((thread_ == nullptr) || thread_->ExitOnRestart(level))
   {
      socket_ = nullptr;
   }
}

//------------------------------------------------------------------------------

void IpPort::Startup(RestartLevel level)
{
   Debug::ft("IpPort.Startup");

   FunctionGuard guard(Guard_MemUnprotect);

   EnsureAlarm();

   if(stats_ == nullptr)
   {
      stats_.reset(new IpPortStats);
   }

   //  If the port has an input handler, make sure that it has an I/O thread.
   //
   if((handler_ != nullptr) && (thread_ == nullptr))
   {
      SetThread(CreateIoThread());
   }
}

//------------------------------------------------------------------------------

void IpPort::UnbindHandler(const InputHandler& handler)
{
   Debug::ftnt("IpPort.UnbindHandler");

   //  Do nothing if a different handler is registered.
   //
   if(handler_.get() != &handler) return;

   //  If the port has an I/O thread, shut it down before releasing its
   //  input handler, which is currently undergoing deletion.
   //
   if(thread_ != nullptr)
   {
      thread_->Raise(SIGCLOSE);
      SetThread(nullptr);
   }

   handler_.release();
}
}
