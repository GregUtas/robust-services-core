//==============================================================================
//
//  UdpIoThread.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef UDPIOTHREAD_H_INCLUDED
#define UDPIOTHREAD_H_INCLUDED

#include "IoThread.h"
#include <cstddef>
#include "NbTypes.h"
#include "NwTypes.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  I/O thread for UDP-based protocols.
//
class UdpIoThread : public IoThread
{
public:
   //  Creates an I/O thread that will receive UDP messages.  The arguments
   //  are described in the base class.
   //
   UdpIoThread(Faction faction, ipport_t port, size_t rxSize, size_t txSize);

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;
protected:
   //  Protected to restrict deletion.
   //
   virtual ~UdpIoThread();

   //  Overridden to release resources in order to unblock.
   //
   virtual void Unblock() override;

   //  Overridden to release resources during error recovery.
   //
   virtual void Cleanup() override;
private:
   //  Overridden to return a name for the thread.
   //
   virtual const char* AbbrName() const override;

   //  Overridden to receive UDP messages on PORT.
   //
   virtual void Enter() override;

   //  Generates a log when an error forces the thread to exit.
   //
   void OutputLog(debug32_t errval) const;

   //  Releases resources when exiting or cleaning up the thread.
   //
   void ReleaseResources();
};
}
#endif
