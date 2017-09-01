//==============================================================================
//
//  MscContextPair.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
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
   virtual void Display(std::ostream& stream,
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
