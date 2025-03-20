//==============================================================================
//
//  CallbackRequest.h
//
//  Copyright (C) 2013-2025  Greg Utas
//
//  This file is part of the Robust Services Core (RSC).
//
//  RSC is free software: you can redistribute it and/or modify it under the
//  terms of the Lesser GNU General Public License as published by the Free
//  Software Foundation, either version 3 of the License, or (at your option)
//  any later version.
//
//  RSC is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
//  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
//  details.
//
//  You should have received a copy of the Lesser GNU General Public License
//  along with RSC.  If not, see <http://www.gnu.org/licenses/>.
//
#ifndef CALLBACKREQUEST_H_INCLUDED
#define CALLBACKREQUEST_H_INCLUDED

#include <memory>

//------------------------------------------------------------------------------

namespace NodeBase
{
//  General interface for callbacks.  Subclasses add the data necessary to
//  complete a callback and give it to a client, who later calls Callback()
//  and deletes the request.
//
class CallbackRequest
{
public:
   //  Virtual to allow subclassing.
   //
   virtual ~CallbackRequest() = default;

   //  The callback.
   //
   virtual void Callback() { }
protected:
   //  Protected because this class is virtual.
   //
   CallbackRequest() = default;
};

//  A CallbackRequest is allocated from the general heap and should therefore
//  be managed by a unique_ptr.
//
typedef std::unique_ptr<CallbackRequest> CallbackRequestPtr;
}
#endif
