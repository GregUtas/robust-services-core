//==============================================================================
//
//  SbPools.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef SBPOOLS_H_INCLUDED
#define SBPOOLS_H_INCLUDED

#include "ObjectPool.h"
#include <cstddef>
#include "NbTypes.h"

namespace SessionBase
{
   class GlobalAddress;
   class MsgPort;
}

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace SessionBase
{
//  Pool for SbIpBuffer objects.
//
class SbIpBufferPool : public ObjectPool
{
   friend class Singleton< SbIpBufferPool >;
public:
   //> The size of SbIpBuffer blocks.
   //
   static const size_t BlockSize;

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;
private:
   //  Private because this singleton is not subclassed.
   //
   SbIpBufferPool();

   //  Private because this singleton is not subclassed.
   //
   ~SbIpBufferPool();
};

//------------------------------------------------------------------------------
//
//  Pool for Context objects.
//
class ContextPool : public ObjectPool
{
   friend class Singleton< ContextPool >;
public:
   //> The size of Context blocks.
   //
   static const size_t BlockSize;

   //  Overridden to claim blocks on work queues.
   //
   virtual void ClaimBlocks() override;

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;
private:
   //  Private because this singleton is not subclassed.
   //
   ContextPool();

   //  Private because this singleton is not subclassed.
   //
   ~ContextPool();
};

//------------------------------------------------------------------------------
//
//  Pool for Message objects.
//
class MessagePool : public ObjectPool
{
   friend class Singleton< MessagePool >;
public:
   //> The size of Message blocks.
   //
   static const size_t BlockSize;

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;
private:
   //  Private because this singleton is not subclassed.
   //
   MessagePool();

   //  Private because this singleton is not subclassed.
   //
   ~MessagePool();
};

//------------------------------------------------------------------------------
//
//  Pool for MsgPort objects.
//
class MsgPortPool : public ObjectPool
{
   friend class Singleton< MsgPortPool >;
public:
   //> The size of MsgPort blocks.
   //
   static const size_t BlockSize;

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
   virtual void Patch(sel_t selector, void* arguments) override;
private:
   //  Private because this singleton is not subclassed.
   //
   MsgPortPool();

   //  Private because this singleton is not subclassed.
   //
   ~MsgPortPool();
};

//------------------------------------------------------------------------------
//
//  Pool for ProtocolSM objects.
//
class ProtocolSMPool : public ObjectPool
{
   friend class Singleton< ProtocolSMPool >;
public:
   //> The size of ProtocolSM blocks.
   //
   static const size_t BlockSize;

   //  Overridden to claim objects in the PSM's context.
   //
   virtual void ClaimBlocks() override;

   //  Overridden to display member variables.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;
private:
   //  Private because this singleton is not subclassed.
   //
   ProtocolSMPool();

   //  Private because this singleton is not subclassed.
   //
   ~ProtocolSMPool();

   //  The identifier of the PSM currently being audited.
   //
   PooledObjectId psmToAudit_;
};

//------------------------------------------------------------------------------
//
//  Pool for Timer objects.
//
class TimerPool : public ObjectPool
{
   friend class Singleton< TimerPool >;
public:
   //> The size of Timer blocks.
   //
   static const size_t BlockSize;

   //  Increments the number of timeouts sent.
   //
   void IncrTimeouts() const;

   //  Overridden to display statistics.
   //
   virtual void DisplayStats(std::ostream& stream) const override;

   //  Overridden to claim blocks in the TimerRegistry.
   //
   virtual void ClaimBlocks() override;

   //  Overridden for restarts.
   //
   virtual void Startup(RestartLevel level) override;

   //  Overridden for restarts.
   //
   virtual void Shutdown(RestartLevel level) override;

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;
private:
   //  Private because this singleton is not subclassed.
   //
   TimerPool();

   //  Private because this singleton is not subclassed.
   //
   ~TimerPool();

   //  The number of timeouts sent.
   //
   CounterPtr timeouts_;
};

//------------------------------------------------------------------------------
//
//  Pool for ServiceSM objects.
//
class ServiceSMPool : public ObjectPool
{
   friend class Singleton< ServiceSMPool >;
public:
   //> The size of ServiceSM blocks.
   //
   static const size_t BlockSize;

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;
private:
   //  Private because this singleton is not subclassed.
   //
   ServiceSMPool();

   //  Private because this singleton is not subclassed.
   //
   ~ServiceSMPool();
};

//------------------------------------------------------------------------------
//
//  Pool for Event objects.
//
class EventPool : public ObjectPool
{
   friend class Singleton< EventPool >;
public:
   //> The size of Event blocks.
   //
   static const size_t BlockSize;

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;
private:
   //  Private because this singleton is not subclassed.
   //
   EventPool();

   //  Private because this singleton is not subclassed.
   //
   ~EventPool();
};

//------------------------------------------------------------------------------
//
//  Pool for BtIpBuffer objects.  These are used by the BuffTracer tool and
//  are identical to SbIpBuffers.  A separate pool is used so that tracing
//  cannot interfere with regular work.
//
class BtIpBufferPool : public ObjectPool
{
   friend class Singleton< BtIpBufferPool >;
public:
   //> The size of BtIpBuffer blocks.
   //
   static const size_t BlockSize;

   //  Overridden to claim blocks held by the trace buffer.
   //
   virtual void ClaimBlocks() override;

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;
private:
   //  Private because this singleton is not subclassed.
   //
   BtIpBufferPool();

   //  Private because this singleton is not subclassed.
   //
   ~BtIpBufferPool();
};
}
#endif
