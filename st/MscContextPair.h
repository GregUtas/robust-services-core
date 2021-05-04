//==============================================================================
//
//  MscContextPair.h
//
//  Copyright (C) 2013-2021  Greg Utas
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
#ifndef MSCCONTEXTPAIR_H_INCLUDED
#define MSCCONTEXTPAIR_H_INCLUDED

#include "Temporary.h"
#include <cstddef>
#include "Q1Link.h"

namespace SessionTools
{
   class MscContext;
}

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace SessionTools
{
//  Identifies a pair of communicating contexts in a message sequence chart.
//
class MscContextPair : public Temporary
{
public:
   //  Pairs CTX1 and CTX2 as communicating contexts.
   //
   MscContextPair(MscContext& ctx1, MscContext& ctx2);

   //  Not subclassed.
   //
   ~MscContextPair();

   //  Returns the communicating contexts.
   //
   void Contexts(MscContext*& ctx1, MscContext*& ctx2) const;

   //  Returns true if CTX1 and CTX2 are the addresses in this pair.
   //
   bool IsEqualTo(const MscContext& ctx1, const MscContext& ctx2) const;

   //  If CONTEXT is one of the addresses in this pair, returns its peer
   //  address, else returns nullptr.
   //
   MscContext* Peer(const MscContext& context) const;

   //  Returns the offset to link_.
   //
   static ptrdiff_t LinkDiff();

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;
private:
   //  First context in the pair of communicating contexts.
   //
   MscContext* const ctx1_;

   //  Second context in the pair of communicating contexts.
   //
   MscContext* const ctx2_;

   //  Next pair of contexts in the message sequence chart.
   //
   Q1Link link_;
};
}
#endif
