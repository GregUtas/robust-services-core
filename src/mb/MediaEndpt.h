//==============================================================================
//
//  MediaEndpt.h
//
//  Copyright (C) 2013-2025  Greg Utas
//
//  This file is part of the Robust Services Core (RSC).
//
//  RSC is free software: you can redistribute it and/or modify it under the
//  terms of the Lesser GNU General Public License as published by the Free
//  Software Foundation, either version 3 of the License, or (at your option)
//  any later version.
//
//  RSC is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
//  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
//  details.
//
//  You should have received a copy of the Lesser GNU General Public License
//  along with RSC.  If not, see <http://www.gnu.org/licenses/>.
//
#ifndef MEDIAENDPT_H_INCLUDED
#define MEDIAENDPT_H_INCLUDED

#include "Pooled.h"
#include <cstddef>
#include "SbTypes.h"

namespace MediaBase
{
   class MediaPsm;
}

using namespace NodeBase;
using namespace SessionBase;

//------------------------------------------------------------------------------

namespace MediaBase
{
//  Virtual base class for supporting media endpoints (e.g. in H.248).  Each
//  MediaEndpt (MEP) is owned by a MediaPsm that collaborates with the MEP.
//  The PSM overrides media functions to also invoke functions that the MEP
//  provides.  These include
//  o SetIcTone
//  o SetOgPsm
//  o SetOgPort
//  o SetOgTone
//
class MediaEndpt : public Pooled
{
   friend class MediaPsm;
public:
   static const StateId Idle = 0;

   //  Returns the MEP's state.
   //
   StateId GetState() const { return state_; }

   //  Updates the MEP's state.
   //
   virtual void SetState(StateId stid);

   //  Returns the MEP's PSM.
   //
   MediaPsm* Psm() const { return psm_; }

   //  Returns the MEP's media gateway PSM, which interfaces with a media
   //  gateway on behalf of a group of MEPs (e.g. by supporting an H.248
   //  context).
   //
   ProtocolSM* MgwPsm() const;

   //  Idles the MEP, which deletes itself at the end of the transaction.
   //  This function must be used (instead of the destructor) so that the
   //  MEP can generate any pending messages at the end of the transaction.
   //
   virtual void Deallocate();

   //  Overridden to obtain a MEP from its object pool.
   //
   static void* operator new(size_t size);

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden to select MEPs by FactoryId.
   //
   bool Passes(uint32_t selector) const override;
protected:
   //  Creates a MEP that is owned by PSM.  Protected because this class is
   //  virtual.
   //
   explicit MediaEndpt(MediaPsm& psm);

   //  Deregisters the MEP from its PSM.  Generates a log if the MEP is not
   //  in the idle state.  Protected to restrict deletion to Deallocate.
   //
   virtual ~MediaEndpt();

   //  Deletes the MEP at the end of the transaction in which Deallocate
   //  was invoked.  May also be overridden by subclasses that need to
   //  add media parameters to outgoing messages, but the base class
   //  version must be invoked.
   //
   virtual void EndOfTransaction();
private:
   //  Invoked so that the MEP can process any media parameter in MSG.
   //  Must be overridden by subclasses that need this capability.
   //
   virtual void ProcessIcMsg(Message& msg);

   //  The PSM that owns this MEP.
   //
   MediaPsm* const psm_;

   //  The MEP's state.
   //
   StateId state_;
};
}
#endif
