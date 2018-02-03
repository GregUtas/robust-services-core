//==============================================================================
//
//  MsgContext.h
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
#ifndef MSGCONTEXT_H_INCLUDED
#define MSGCONTEXT_H_INCLUDED

#include "Context.h"
#include "NbTypes.h"
#include "SbTypes.h"

//------------------------------------------------------------------------------

namespace SessionBase
{
//  Supports a stateless context in which a subclass of MsgFactory receives
//  messages through its MsgFactory::ProcessIcMsg function.
//
class MsgContext : public Context
{
   friend class MsgFactory;
public:
   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;
protected:
   //  Protected to restrict creation.
   //
   explicit MsgContext(Faction faction);

   //  Protected to restrict deletion.  Virtual to allow subclassing.
   //
   virtual ~MsgContext();

   //  Returns the type of context.
   //
   virtual ContextType Type() const override { return SingleMsg; }

   //  Overridden to flag the context message as handled.
   //
   virtual void EndOfTransaction() override;
private:
   //  Overridden to handle the arrival of MSG.
   //
   virtual void ProcessIcMsg(Message& msg) override;
};
}
#endif
