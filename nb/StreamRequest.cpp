//==============================================================================
//
//  StreamRequest.cpp
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
#include "StreamRequest.h"
#include <string>
#include "Debug.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
fn_name StreamRequest_ctor1 = "StreamRequest.ctor";

StreamRequest::StreamRequest() : stream_(nullptr)
{
   Debug::ft(StreamRequest_ctor1);
}

//------------------------------------------------------------------------------

fn_name StreamRequest_ctor2 = "StreamRequest.ctor(copy)";

StreamRequest::StreamRequest(const StreamRequest& that) : MsgBuffer(that),
   stream_(nullptr)
{
   Debug::ft(StreamRequest_ctor2);
}

//------------------------------------------------------------------------------

fn_name StreamRequest_dtor = "StreamRequest.dtor";

StreamRequest::~StreamRequest()
{
   Debug::ft(StreamRequest_dtor);

   stream_.reset();
}

//------------------------------------------------------------------------------

fn_name StreamRequest_Cleanup = "StreamRequest.Cleanup";

void StreamRequest::Cleanup()
{
   Debug::ft(StreamRequest_Cleanup);

   stream_.reset();
   MsgBuffer::Cleanup();
}

//------------------------------------------------------------------------------

void StreamRequest::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   MsgBuffer::Display(stream, prefix, options);

   stream << prefix << "stream : " << stream_.get() << CRLF;
}

//------------------------------------------------------------------------------

void StreamRequest::Patch(sel_t selector, void* arguments)
{
   MsgBuffer::Patch(selector, arguments);
}
}
