//==============================================================================
//
//  MsgBuffer.h
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
#ifndef MSGBUFFER_H_INCLUDED
#define MSGBUFFER_H_INCLUDED

#include "Pooled.h"
#include <cstddef>
#include "Clock.h"
#include "ToolTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  MsgBuffer supports internal messages, such as those between threads.
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
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;

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
