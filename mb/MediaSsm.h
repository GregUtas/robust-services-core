//==============================================================================
//
//  MediaSsm.h
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
#ifndef MEDIASSM_H_INCLUDED
#define MEDIASSM_H_INCLUDED

#include "RootServiceSM.h"
#include "SbTypes.h"
#include "Switch.h"

using namespace NodeBase;
using namespace SessionBase;

//------------------------------------------------------------------------------

namespace MediaBase
{
//  A root SSM that also supports media.
//
class MediaSsm : public RootServiceSM
{
   friend class MediaPsm;
public:
   //  Returns the PSM that interfaces to the media gateway.
   //
   ProtocolSM* MgwPsm() const { return mgwPsm_; }

   //  Sets the PSM that interfaces to the media gateway.
   //
   virtual bool SetMgwPsm(ProtocolSM* psm);

   //  Overridden to enumerate all objects that the SSM owns.
   //
   virtual void GetSubtended(Base* objects[], size_t& count) const override;

   //  Overridden to display member variables.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;
protected:
   //  Protected because this class is virtual.
   //
   explicit MediaSsm(ServiceId sid);

   //  Protected to restrict deletion. Virtual to allow subclassing.
   //
   virtual ~MediaSsm();

   //  Overridden to inform PSMs that are listening to exPsm and to handle
   //  deletion of the media gateway PSM.  May be overridden, but the base
   //  class version must be invoked,
   //
   virtual void PsmDeleted(ProtocolSM& exPsm) override;
private:
   //  Informs all PSMs that are listening to txPsm that they should now
   //  listen to txPort.
   //
   void NotifyListeners(const ProtocolSM& txPsm, Switch::PortId txPort) const;

   //  The PSM (if any) that is interfacing to the media gateway.
   //
   ProtocolSM* mgwPsm_;
};
}
#endif
