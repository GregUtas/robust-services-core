//==============================================================================
//
//  SsmContext.h
//
//  Copyright (C) 2013-2022  Greg Utas
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
#ifndef SSMCONTEXT_H_INCLUDED
#define SSMCONTEXT_H_INCLUDED

#include "PsmContext.h"
#include "NbTypes.h"
#include "SbTypes.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace SessionBase
{
//  Supports stateful contexts in which a subclass of SsmFactory creates a
//  root SSM that receives messages through its PSMs.
//
class SsmContext : public PsmContext
{
   friend class RootServiceSM;
   friend class SsmFactory;
public:
   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;

   //  Overridden to enumerate all objects that the context owns.
   //
   void GetSubtended(std::vector<Base*>& objects) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;

   //  Returns the root SSM.
   //
   RootServiceSM* RootSsm() const override { return root_; }
protected:
   //  Overridden to invoke EndOfTransaction on the root SSM.
   //
   void EndOfTransaction() override;

   //  Returns the type of context.
   //
   ContextType Type() const override { return MultiPort; }
private:
   //  Private to restrict creation.
   //
   explicit SsmContext(NodeBase::Faction faction);

   //  Private to restrict deletion.  Not subclassed.
   //
   ~SsmContext();

   //  Sets the root SSM.
   //
   void SetRoot(RootServiceSM* root);

   //  Generates a log containing SID, ERRVAL, and the context's
   //  message trace when an error occurs.
   //
   void OutputLog(ServiceId sid, NodeBase::word errval) const;

   //  Overridden to determine if the context should be deleted.
   //
   bool IsIdle() const override;

   //  Overridden to handle the arrival of MSG.
   //
   void ProcessIcMsg(Message& msg) override;

   //  The root SSM.
   //
   RootServiceSM* root_;
};
}
#endif
