//==============================================================================
//
//  UdpIoThread.h
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
#ifndef UDPIOTHREAD_H_INCLUDED
#define UDPIOTHREAD_H_INCLUDED

#include "IoThread.h"
#include "NwTypes.h"

namespace NetworkBase
{
   class UdpIpService;
}

//------------------------------------------------------------------------------

namespace NetworkBase
{
//  I/O thread for UDP-based protocols.
//
class UdpIoThread : public IoThread
{
public:
   //  Creates a UDP I/O thread, managed by DAEMON, that receives messages
   //  on PORT on behalf of SERVICE.
   //
   UdpIoThread
      (NodeBase::Daemon* daemon, const UdpIpService* service, ipport_t port);

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
protected:
   //  Protected to restrict deletion.
   //
   virtual ~UdpIoThread();

   //  Overridden to release resources in order to unblock.
   //
   void Unblock() override;
private:
   //  Releases resources when exiting or cleaning up the thread.
   //
   void ReleaseResources();

   //  Overridden to return a name for the thread.
   //
   NodeBase::c_string AbbrName() const override;

   //  Overridden to receive UDP messages on PORT.
   //
   void Enter() override;
};
}
#endif
