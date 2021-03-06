//==============================================================================
//
//  MediaPsm.h
//
//  Copyright (C) 2013-2020  Greg Utas
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
#ifndef MEDIAPSM_H_INCLUDED
#define MEDIAPSM_H_INCLUDED

#include "ProtocolSM.h"
#include "MediaParameter.h"
#include "SbTypes.h"
#include "Switch.h"
#include "Tones.h"

namespace MediaBase
{
   class MediaEndpt;
   class MediaSsm;
}

using namespace NodeBase;
using namespace SessionBase;

//------------------------------------------------------------------------------

namespace MediaBase
{
//  A PSM that also supports media.  In a protocol stack, only the uppermost
//  PSM may perform media operations.
//
class MediaPsm : public ProtocolSM
{
   friend class MediaSsm;
public:
   //  Makes the PSM behave as an edge PSM.  An edge PSM is associated with a
   //  circuit that has a dedicated port.  A new PSM is configured as a relay
   //  PSM, so this function should be invoked immediately after creating a
   //  PSM that is to serve as an edge PSM.  PORT is the circuit's port.
   //
   void MakeEdge(Switch::PortId port);

   //  Makes the PSM behave as a relay PSM.  A relay PSM only receives and
   //  sends port addresses in MediaInfo parameters that may ultimately reach
   //  edge PSMs.  A new PSM is configured as a relay PSM, so this function
   //  only needs to be invoked during multiplexer insertion, to transform a
   //  basic call UPSM (an edge) into a relay.
   //
   void MakeRelay();

   //  Listens to whatever is being transmitted by OGPSM.
   //
   virtual void SetOgPsm(MediaPsm* ogPsm);

   //  Transmits TONE out of the context.  If TONE is Tone::Media, then the
   //  media stream from ogPsm_ is transmitted.
   //
   virtual void SetOgTone(Tone::Id ogTone);

   //  Transmits TONE into the context, to any PSM that is listening.  If
   //  TONE is Tone::Media, then the PSM transmits its user's media stream.
   //
   virtual void SetIcTone(Tone::Id icTone);

   //  Sets up this PSM and OTHER to send and receive media to each other:
   //  o Uses SetOgPsm to make PSM and OTHER mutual peers.
   //  o Invokes EnableMedia on this PSM and OTHER.
   //
   void CreateMedia(MediaPsm& other);

   //  Ensures that this PSM and OTHER can send and receive media to each
   //  other:
   //  o If neither this PSM nor OTHER has a peer media PSM, uses SetOgPsm
   //    to make them mutual peers.  They may already have different peers
   //    (for example, ports on a conference circuit).
   //  o Invokes EnableMedia on this PSM and its peer.
   //  o Invokes EnableMedia on OTHER and its peer.
   //
   void EnsureMedia(MediaPsm& other);

   //  Sets up this PSM to send media to, and receive media from, OTHER:
   //  o Uses SetOgPsm to make PSM and OTHER mutual peers.
   //  o Invokes EnableMedia on this PSM.
   //  Note that this does not modify what OTHER is receiving and sending.
   //
   void EnableMedia(MediaPsm& other);

   //  Receives and transmits media.
   //
   void EnableMedia();

   //  Receives and transmits silence.
   //
   void DisableMedia();

   //  Copies ogMedia_ to PSM's icMedia_ and icMedia_ to PSM's ogMedia_.
   //  Used during multiplexer insertion prior to creating a media stream
   //  between the two PSMs.  Because the PSMs are synched, the streams can
   //  then be configured without causing any side effects.
   //
   void SynchRelay(MediaPsm& psm) const;

   //  Returns the PSM's MEP.
   //
   MediaEndpt* Mep() const { return mep_; }

   //  Sets the PSM's MEP.  For use by MEPs only.
   //
   void SetMep(MediaEndpt* mep);

   //  Returns the PSM to which this one is listening.
   //
   MediaPsm* GetOgPsm() const;

   //  Returns the tone to which this PSM is listening.
   //
   Tone::Id GetOgTone() const { return ogTone_; }

   //  Returns the media SSM on which this PSM is running.  This will be
   //  nullptr if the PSM is running on a test session.
   //
   MediaSsm* GetMediaSsm() const;

   //  Overridden to enumerate all objects that the PSM owns.
   //
   void GetSubtended(std::vector< Base* >& objects) const override;

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;
protected:
   //  Creates a PSM that will send an initial message.  The arguments are
   //  the same as those for the base class.  The PSM is configured as a
   //  relay PSM; SetEdge should be invoked immediately after its creation
   //  if it is to serve as an edge PSM.
   //
   explicit MediaPsm(FactoryId fid);

   //  Creates a PSM from an adjacent layer.  The arguments are the same
   //  as those for the base class.  The PSM is configured as a relay PSM;
   //  SetEdge should be invoked immediately after its creation if it is
   //  to serve as an edge PSM.
   //
   MediaPsm(FactoryId fid, ProtocolLayer& adj, bool upper);

   //  Protected to restrict deletion.  Virtual to allow subclassing.
   //
   virtual ~MediaPsm();

   //  Invoked when the user's media stream should now be taken from PORT.
   //  If OGPORT has changed from its previous value, a MediaInfo parameter
   //  will be sent.  For an edge PSM, Switch::MakeConn is also invoked.
   //
   void SetOgPort(Switch::PortId ogport);

   //  A subclass' ProcessIcMsg function invokes this to handle any incoming
   //  MediaInfo parameter.  PID is the parameter's identifier.
   //
   void UpdateIcMedia(TlvMessage& msg, ParameterId pid);

   //  A subclass' ProcessOgMsg function invokes this to include any pending
   //  MediaInfo parameter.  PID is the parameter's identifier.
   //
   void UpdateOgMedia(TlvMessage& msg, ParameterId pid);

   //  Copies ogMedia_ to PSM's ogMedia_.  Used during multiplexer
   //  insertion to synchronize a new edge PSM with one in another context.
   //
   void SynchEdge(MediaPsm& psm) const;

   //  Overridden to invoke ProcessIcMsg on the MEP.
   //
   Event* ReceiveMsg(Message& msg) override;

   //  Overridden to invoke EnsureMediaMsg if a media update is pending.
   //
   void PrepareOgMsgq() override;

   //  Overridden to invoke EndOfTransaction on the MEP.
   //
   void EndOfTransaction() override;
private:
   //  Invoked when outgoing media information has changed, which means
   //  that a MediaInfo parameter must be sent.  Unless one already exists,
   //  a subclass must create a suitable message to which this parameter
   //  can be added during ProcessOgMsg.
   //
   virtual void EnsureMediaMsg() = 0;

   //  Invoked when the port from which the PSM is transmitting changes.
   //  If the PSM is sending Tone::Media to listeners, they are told to
   //  listen to the new port.
   //
   void IcPortUpdated() const;

   //  Determines the port from which the PSM's listeners should receive.
   //
   Switch::PortId CalcIcPort() const;

   //  Set if this PSM is an edge media endpoint.
   //
   bool edge_;

   //  The PSM to which this one is listening.
   //
   MediaPsm* ogPsm_;

   //  The tone being sent to the user.
   //
   Tone::Id ogTone_;

   //  The tone being sent to listeners.
   //
   Tone::Id icTone_;

   //  Incoming media information.  "Incoming" means into a context,
   //  towards other PSMs in the same context.
   //
   MediaInfo icMedia_;

   //  The last outgoing media information that the PSM transmitted in a
   //  message. "Outgoing" means out of a context, towards a peer PSM in
   //  another context, or towards an external circuit.
   //
   MediaInfo ogMediaSent_;

   //  The outgoing media information as updated during this transaction.
   //  If it is different from ogMediaSent_ when the transaction ends, it
   //  causes a media update parameter to be sent.
   //
   MediaInfo ogMediaCurr_;

   //  The PSM's MEP, if any.
   //
   MediaEndpt* mep_;
};
}
#endif
