//==============================================================================
//
//  MediaEndpt.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
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

   //  Overridden to display member variables.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden to obtain a MEP from its object pool.
   //
   static void* operator new(size_t size);
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
   MediaPsm* psm_;

   //  The MEP's state.
   //
   StateId state_;
};
}
#endif
