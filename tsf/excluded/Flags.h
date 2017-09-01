//==============================================================================
//
//  Flags.h
//
//  Copyright (C) 2012-2015 Greg Utas.  All rights reserved.
//
#ifndef FLAGS_H_INCLUDED
#define FLAGS_H_INCLUDED

#include "NbTypes.h"
#include "SysDefs.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Base class for a set of flags.
//
class Flags
{
public:
   //  A group of flags, each implemented as a bit within a word.
   //e Implement CountOnes and LeastSignificantReset.
   //
   typedef uword Bits;

   static const Bits AllFlags = UWORD_MAX;
   static const Bits NoFlags = 0;

   //  Initializes all flags to MASK (default = all flags off).
   //
   explicit Flags(Bits mask = 0) : all_(mask) { }

   //  Returns the mask that accesses FLAG.
   //
   static Bits Mask(FlagId flag)
   { return (1 << flag); }

   //  Sets FLAG.
   //
   void SetFlag(FlagId flag)
   { all_ |= Mask(flag); }

   //  Sets the flags in MASK.
   //
   void SetFlags(Bits mask)
   { all_ |= mask; }

   //  Clears FLAG.
   //
   void ClearFlag(FlagId flag)
   { all_ &= ~Mask(flag); }

   //  Clears all of the flags.
   //
   void ClearAll()
   { all_ = NoFlags; }

   //  Returns true if FLAG is set.
   //
   bool FlagOn(FlagId flag) const
   { return ((all_ & Mask(flag)) != 0); }

   //  Returns true if all the flags in MASK are set.
   //
   bool FlagsOn(Bits mask) const
   { return ((all_ & mask) == mask); }

   //  Returns true if FLAG is the only flag that is set.
   //
   bool OnlyFlagOn(FlagId flag) const
   { return ((all_ & Mask(flag)) == all_); }

   //  Returns true if no flag is set.
   //
   bool NoFlagOn() const
   { return (all_ == NoFlags); }

   //  Returns the entire set of flags.
   //
   Bits All() const { return all_; }
private:
   //  The set of flags.
   //
   Bits all_;
};
}
#endif
