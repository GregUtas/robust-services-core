//==============================================================================
//
//  StreamRequest.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "StreamRequest.h"
#include <memory>
#include <ostream>
#include <sstream>
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
