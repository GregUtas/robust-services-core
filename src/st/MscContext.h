//==============================================================================
//
//  MscContext.h
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
#ifndef MSCCONTEXT_H_INCLUDED
#define MSCCONTEXT_H_INCLUDED

#include "Temporary.h"
#include <cstddef>
#include <cstdint>
#include <string>
#include "Q1Link.h"
#include "SbTypes.h"
#include "SysTypes.h"

using namespace NodeBase;
using namespace SessionBase;

//------------------------------------------------------------------------------

namespace SessionTools
{
//  A column in a message sequence chart (MSC).
//
typedef word MscColumn;

//  Nil column.
//
constexpr MscColumn NilMscColumn = -1;

//  The initial spacing between vertical lines.
//
constexpr MscColumn ColWidth = 31;

//------------------------------------------------------------------------------
//
//  In a message sequence chart, a context is a vertical line that shows the
//  messages sent and received by
//  o an SsmContext, PsmContext, or MsgContext in this processor
//  o a factory in another processor
//  o an unknown external context
//
class MscContext : public Temporary
{
public:
   //  Creates a context of TYPE, identified by CID and referenced by RCVR.
   //
   MscContext(const void* rcvr, ContextType type, uint16_t cid);

   //  Not subclassed.
   //
   ~MscContext();

   //  Updates CID if it was unavailable when the context was first created.
   //
   void SetCid(ServiceId sid) { cid_ = sid; }

   //  Returns true if the context matches RCVR (if not nullptr), else CID.
   //
   bool IsEqualTo(const void* rcvr, uint16_t cid) const;

   //  Returns true if the context is external.
   //
   bool IsExternal() const { return rcvr_ == nullptr; }

   //  Returns the column where this context appears.
   //
   MscColumn Column() const { return col_; }

   //  Assigns this context to COL and returns COL + ColWidth.
   //
   MscColumn SetColumn(MscColumn col);

   //  Returns the group assigned to this context.
   //
   int Group() const { return group_; }

   //  Attempts to add the context to GROUP:
   //  o An external context is always added to GROUP.
   //  o An internal context is only added if it does not yet have a group.
   //  Returns true if the context is internal and was assigned to GROUP.
   //
   bool SetGroup(int group);

   //  Clears the context's group if it is an external context.  These contexts
   //  may appear in more than one MSC.
   //
   void ClearGroup();

   //  Returns two strings for labelling the context's vertical line in the MSC.
   //
   void Names(std::string& text1, std::string& text2) const;

   //  Returns the offset to link_.
   //
   static ptrdiff_t LinkDiff();

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;
private:
   //  The object that implements the context (nullptr for a
   //  context that is external to this processor).
   //
   const void* const rcvr_;

   //  The type of context.
   //
   const ContextType type_;

   //  The context's identifier (a ServiceId or FactoryId).
   //  An unknown external context is encoded as a factory
   //  with NIL_ID.
   //
   uint16_t cid_;

   //  The context's column in the MSC.
   //
   MscColumn col_;

   //  The group to which the context belongs.
   //
   int group_;

   //  The next context in the MSC.
   //
   Q1Link link_;
};
}
#endif
