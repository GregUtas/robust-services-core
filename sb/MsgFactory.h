//==============================================================================
//
//  MsgFactory.h
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
#ifndef MSGFACTORY_H_INCLUDED
#define MSGFACTORY_H_INCLUDED

#include "Factory.h"
#include "SbTypes.h"

//------------------------------------------------------------------------------

namespace SessionBase
{
//  Stateless applications that process a stream of independent messages
//  (a connectionless protocol) subclass from this.
//
class MsgFactory : public Factory
{
   friend class MsgContext;
public:
   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
protected:
   //  Protected because this is class is virtual.
   //
   MsgFactory(Id fid, ContextType type, ProtocolId prid, const char* name);

   //  Protected because subclasses should be singletons.
   //
   virtual ~MsgFactory();

   //  Overridden to create a MsgContext.
   //
   Context* AllocContext() const override;

   //  Captures the arrival of MSG at CTX.  TT is the record, if any, that
   //  was created to record the work.
   //
   static void CaptureMsg(Context& ctx, const Message& msg, TransTrace* tt);

   //  Overridden to handle a message arriving at a stateless context.
   //  Must NOT be overridden by applications.  Protected to restrict
   //  usage.
   //
   Rc ReceiveMsg
      (Message& msg, bool atIoLevel, TransTrace* tt, Context*& ctx) override;
private:
   //  Handles an incoming message in a stateless context.  The default
   //  version generates a log and must be overridden by a factory that
   //  uses stateless contexts.  Private to restrict usage.
   //
   virtual void ProcessIcMsg(Message& msg) const;
};
}
#endif
