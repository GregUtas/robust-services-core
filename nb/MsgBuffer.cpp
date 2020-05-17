//==============================================================================
//
//  MsgBuffer.cpp
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

MsgBuffer::MsgBuffer() : rxTime_(TimePoint::Now())
{
   Debug::ft(MsgBuffer_ctor1);
}

//------------------------------------------------------------------------------

fn_name MsgBuffer_ctor2 = "MsgBuffer.ctor(copy)";

MsgBuffer::MsgBuffer(const MsgBuffer& that) : rxTime_(that.rxTime_)
{
   Debug::ft(MsgBuffer_ctor2);
}

//------------------------------------------------------------------------------

fn_name MsgBuffer_dtor = "MsgBuffer.dtor";

MsgBuffer::~MsgBuffer()
{
   Debug::ftnt(MsgBuffer_dtor);
}

//------------------------------------------------------------------------------

void MsgBuffer::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Pooled::Display(stream, prefix, options);

   stream << prefix << "rxTime : " << rxTime_.Ticks() << CRLF;
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
