//==============================================================================
//
//  StreamRequest.h
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
#ifndef STREAMREQUEST_H_INCLUDED
#define STREAMREQUEST_H_INCLUDED

#include "MsgBuffer.h"
#include <utility>
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  This is used to pass an ostringstream to another thread that will later
//  write it to an output stream (e.g. console or file).  The ostringstream
//  must be owned by a unique_ptr (ostringstreamPtr).  After creating the
//  request and giving it the ostringstreamPtr, queue the request for the
//  target thread:
//
//    auto req = new StreamRequest;
//    if(req != nullptr)
//    {
//       req->GiveStream(stream);
//       thread->EnqMsg(*req);
//    }
//    else stream.reset();
//
class StreamRequest : public MsgBuffer
{
public:
   //  Creates a request.
   //
   StreamRequest();

   //  Copy constructor.  Buffers support copying so that trace tools can
   //  capture them.  The ostringstream is not copied, however.
   //
   StreamRequest(const StreamRequest& that);

   //  Deletes the stream if still owned.  Virtual to allow subclassing.
   //
   virtual ~StreamRequest();

   //  Gives ownership of the stream to the request.
   //
   void GiveStream(ostringstreamPtr& stream)
   {
      stream_ = std::move(stream);
   }

   //  Takes ownership of the stream from the request.
   //
   ostringstreamPtr TakeStream() { return std::move(stream_); }

   //  Overridden to display member variables.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;
protected:
   //  Overridden to delete stream_ during error recovery.
   //
   virtual void Cleanup() override;
private:
   //  The stream to be output.
   //
   ostringstreamPtr stream_;
};
}
#endif
