//==============================================================================
//
//  SbPools.h
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
#ifndef SBPOOLS_H_INCLUDED
#define SBPOOLS_H_INCLUDED

#include "ObjectPool.h"
#include "NbTypes.h"

namespace SessionBase
{
   class GlobalAddress;
   class MsgPort;
}

//------------------------------------------------------------------------------

namespace SessionBase
{
//  Pool for SbIpBuffer objects.
//
class SbIpBufferPool : public NodeBase::ObjectPool
{
   friend class NodeBase::Singleton< SbIpBufferPool >;
public:
   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
private:
   //  Private because this is a singleton.
   //
   SbIpBufferPool();

   //  Private because this is a singleton.
   //
   ~SbIpBufferPool();
};

//------------------------------------------------------------------------------
//
//  Pool for Context objects.
//
class ContextPool : public NodeBase::ObjectPool
{
   friend class NodeBase::Singleton< ContextPool >;
public:
   //  Overridden to claim blocks on work queues.
   //
   void ClaimBlocks() override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
private:
   //  Private because this is a singleton.
   //
   ContextPool();

   //  Private because this is a singleton.
   //
   ~ContextPool();
};

//------------------------------------------------------------------------------
//
//  Pool for Message objects.
//
class MessagePool : public NodeBase::ObjectPool
{
   friend class NodeBase::Singleton< MessagePool >;
public:
   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
private:
   //  Private because this is a singleton.
   //
   MessagePool();

   //  Private because this is a singleton.
   //
   ~MessagePool();
};

//------------------------------------------------------------------------------
//
//  Pool for MsgPort objects.
//
class MsgPortPool : public NodeBase::ObjectPool
{
   friend class NodeBase::Singleton< MsgPortPool >;
public:
   //  Finds the port that is communicating with remAddr.  This function is
   //  used when a port on another processor sends a subsequent message to a
   //  local port from which it has not yet received a reply.  Such a message
   //  does not contain the local port's address, so the local port must be
   //  found based on the remote port's address (remAddr), which the local
   //  port saved when it received remAddr's initial message.
   //
   MsgPort* FindPeerPort(const GlobalAddress& remAddr) const;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
private:
   //  Private because this is a singleton.
   //
   MsgPortPool();

   //  Private because this is a singleton.
   //
   ~MsgPortPool();
};

//------------------------------------------------------------------------------
//
//  Pool for ProtocolSM objects.
//
class ProtocolSMPool : public NodeBase::ObjectPool
{
   friend class NodeBase::Singleton< ProtocolSMPool >;
public:
   //  Overridden to claim objects in the PSM's context.
   //
   void ClaimBlocks() override;

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
private:
   //  Private because this is a singleton.
   //
   ProtocolSMPool();

   //  Private because this is a singleton.
   //
   ~ProtocolSMPool();
};

//------------------------------------------------------------------------------
//
//  Pool for Timer objects.
//
class TimerPool : public NodeBase::ObjectPool
{
   friend class NodeBase::Singleton< TimerPool >;
public:
   //  Increments the number of timeouts sent.
   //
   void IncrTimeouts() const;

   //  Overridden to claim blocks in the TimerRegistry.
   //
   void ClaimBlocks() override;

   //  Overridden to display statistics.
   //
   void DisplayStats
      (std::ostream& stream, const NodeBase::Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;

   //  Overridden for restarts.
   //
   void Shutdown(NodeBase::RestartLevel level) override;

   //  Overridden for restarts.
   //
   void Startup(NodeBase::RestartLevel level) override;
private:
   //  Private because this is a singleton.
   //
   TimerPool();

   //  Private because this is a singleton.
   //
   ~TimerPool();

   //  The number of timeouts sent.
   //
   NodeBase::CounterPtr timeouts_;
};

//------------------------------------------------------------------------------
//
//  Pool for ServiceSM objects.
//
class ServiceSMPool : public NodeBase::ObjectPool
{
   friend class NodeBase::Singleton< ServiceSMPool >;
public:
   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
private:
   //  Private because this is a singleton.
   //
   ServiceSMPool();

   //  Private because this is a singleton.
   //
   ~ServiceSMPool();
};

//------------------------------------------------------------------------------
//
//  Pool for Event objects.
//
class EventPool : public NodeBase::ObjectPool
{
   friend class NodeBase::Singleton< EventPool >;
public:
   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
private:
   //  Private because this is a singleton.
   //
   EventPool();

   //  Private because this is a singleton.
   //
   ~EventPool();
};

//------------------------------------------------------------------------------
//
//  Pool for BtIpBuffer objects.  These are used by the BufferTracer tool and
//  are identical to SbIpBuffers.  A separate pool is used so that tracing
//  cannot interfere with regular work.
//
class BtIpBufferPool : public NodeBase::ObjectPool
{
   friend class NodeBase::Singleton< BtIpBufferPool >;
public:
   //  Overridden to claim blocks held by the trace buffer.
   //
   void ClaimBlocks() override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
private:
   //  Private because this is a singleton.
   //
   BtIpBufferPool();

   //  Private because this is a singleton.
   //
   ~BtIpBufferPool();
};
}
#endif
