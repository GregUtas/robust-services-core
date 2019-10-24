//==============================================================================
//
//  SbInvokerPools.h
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
#ifndef SBINVOKERPOOLS_H_INCLUDED
#define SBINVOKERPOOLS_H_INCLUDED

#include "InvokerPool.h"
#include "NbTypes.h"
#include "ObjectPool.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace SessionBase
{
//  The payload pool is for applications that perform work for end users.
//
class PayloadInvokerPool : public InvokerPool
{
   friend class NodeBase::Singleton< PayloadInvokerPool >;
public:
   //  Overridden to reject ingress work when the ingress work queue gets
   //  too long or the number of available Messages gets too low, in which
   //  case an alarm is also raised.
   //
   bool RejectIngressWork() const override;

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
private:
   //  Private because this singleton is not subclassed.
   //
   PayloadInvokerPool();

   //  Private because this singleton is not subclassed.
   //
   ~PayloadInvokerPool();

   //  The alarm that is raised when payload work enters overload.
   //
   NodeBase::Alarm* overloadAlarm_;

   //  Overridden to raise an alarm when DELAY is excessive.
   //
   void RecordDelay(MsgPriority prio, NodeBase::msecs_t delay) const override;

   //  The configuration parameter for the maximum length of
   //  this pool's ingress work queue.
   //
   NodeBase::CfgIntParmPtr noIngressQueueLength_;

   //  The configuration parameter for the number of SbIpBuffers
   //  reserved for non-ingress work.
   //
   NodeBase::CfgIntParmPtr noIngressMessageCount_;

   //  The maximum length allowed for the ingress work queue.
   //
   static NodeBase::word NoIngressQueueLength_;

   //  The number of SbIpBuffers reserved for non-ingress work.
   //
   static NodeBase::word NoIngressMessageCount_;
};
}
#endif
