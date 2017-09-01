//==============================================================================
//
//  TcpIpPort.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef TCPIPPORT_H_INCLUDED
#define TCPIPPORT_H_INCLUDED

#include "IpPort.h"
#include "NwTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  An IP port that supports a TCP-based protocol.
//
class TcpIpPort : public IpPort
{
public:
   //  See IpPort's constructor.
   //
   TcpIpPort(ipport_t port, IpService* service);

   //  Not subclassed.
   //
   ~TcpIpPort();

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;
private:
   //  Overridden to create a TcpIoThread for the port.
   //
   virtual IoThread* CreateIoThread() override;

   //  Overridden to create a TCP socket for an application instance.
   //  Setting txSize to zero disables buffering of outgoing messages.
   //
   virtual SysSocket* CreateAppSocket(size_t rxSize, size_t txSize) override;
};
}
#endif
