//==============================================================================
//
//  MsgBuffer.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef MSGBUFFER_H_INCLUDED
#define MSGBUFFER_H_INCLUDED

#include "Pooled.h"
#include <cstddef>
#include "Clock.h"
#include "ToolTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  MsgBuffer wraps a message that passes between NodeBase and the IP stack.
//  It allocates a buffer for a contiguous message that may include an
//  internal header.
//
class MsgBuffer : public Pooled
{
public:
   //  Allocates a message buffer.
   //
   MsgBuffer();

   //  Copy constructor.
   //
   MsgBuffer(const MsgBuffer& that);

   //  Virtual to allow subclassing.
   //
   virtual ~MsgBuffer();

   //  Returns the time when the message was created.
   //
   ticks_t RxTicks() const { return rxTicks_; }

   //  Modifies the time when the message was created.
   //
   void SetRxTicks(const ticks_t& ticks) { rxTicks_ = ticks; }

   //  Determines whether the buffer should be traced.  The default version
   //  returns TraceDefault and may be overridden as requred.
   //
   virtual TraceStatus GetStatus() const;

   //  Overridden to display member variables.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;

   //  Overridden to obtain a buffer from its object pool.
   //
   static void* operator new(size_t size);
private:
   //  The time when the message arrived at I/O level.
   //
   ticks_t rxTicks_;
};
}
#endif
