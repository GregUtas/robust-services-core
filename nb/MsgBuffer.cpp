//==============================================================================
//
//  MsgBuffer.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "MsgBuffer.h"
#include <ostream>
#include <string>
#include "Debug.h"
#include "NbPools.h"
#include "Singleton.h"
#include "SysTypes.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
fn_name MsgBuffer_ctor1 = "MsgBuffer.ctor";

MsgBuffer::MsgBuffer() : rxTicks_(Clock::TicksNow())
{
   Debug::ft(MsgBuffer_ctor1);
}

//------------------------------------------------------------------------------

fn_name MsgBuffer_ctor2 = "MsgBuffer.ctor(copy)";

MsgBuffer::MsgBuffer(const MsgBuffer& that) : rxTicks_(that.rxTicks_)
{
   Debug::ft(MsgBuffer_ctor2);
}

//------------------------------------------------------------------------------

fn_name MsgBuffer_dtor = "MsgBuffer.dtor";

MsgBuffer::~MsgBuffer()
{
   Debug::ft(MsgBuffer_dtor);
}

//------------------------------------------------------------------------------

void MsgBuffer::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Pooled::Display(stream, prefix, options);

   stream << prefix << "rxTicks : " << rxTicks_ << CRLF;
}

//------------------------------------------------------------------------------

TraceStatus MsgBuffer::GetStatus() const
{
   return TraceDefault;
}

//------------------------------------------------------------------------------

fn_name MsgBuffer_new = "MsgBuffer.operator new";

void* MsgBuffer::operator new(size_t size)
{
   Debug::ft(MsgBuffer_new);

   return Singleton< MsgBufferPool >::Instance()->DeqBlock(size);
}

//------------------------------------------------------------------------------

void MsgBuffer::Patch(sel_t selector, void* arguments)
{
   Pooled::Patch(selector, arguments);
}
}
