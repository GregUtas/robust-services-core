//==============================================================================
//
//  PotsProfile.h
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
#ifndef POTSPROFILE_H_INCLUDED
#define POTSPROFILE_H_INCLUDED

#include "BcAddress.h"
#include <cstddef>
#include <memory>
#include "LocalAddress.h"
#include "PotsCircuit.h"
#include "PotsFeature.h"
#include "Q1Way.h"
#include "RegCell.h"
#include "SbTypes.h"

using namespace NodeBase;
using namespace CallBase;

//------------------------------------------------------------------------------

namespace PotsBase
{
//  A profile is created when a DN (directory number) is assigned to a
//  POTS circuit.  Instances are registered with PotsProfileRegistry.
//
class PotsProfile : public Address
{
   friend class Registry< PotsProfile >;
public:
   //  Profile states.
   //
   enum State
   {
      Idle,     // onhook, no SSM
      Lockout,  // offhook, no SSM
      Active    // has an SSM
   };

   //  Assigns DN to the new profile and creates a circuit for it.  In
   //  an actual system, circuits would be provisioned separately, along
   //  with associations between profiles and circuits.
   //
   explicit PotsProfile(DN dn);

   //  Deleted to prohibit copying.
   //
   PotsProfile(const PotsProfile& that) = delete;
   PotsProfile& operator=(const PotsProfile& that) = delete;

   //  Returns the profile's DN.
   //
   DN GetDN() const { return IndexToDN(dn_.GetId()); }

   //  Returns the circuit associated with the profile.
   //
   PotsCircuit* GetCircuit() const { return circuit_.get(); }

   //  Returns the profile's state.
   //
   State GetState() const { return state_; }

   //  Sets the profile's state.  PSM is the object that is receiving
   //  messages from the circuit.  If PSM is nullptr or its port's address
   //  does not match the address set in the profile, nothing happens.
   //
   void SetState(const ProtocolSM* psm, State state);

   //  Returns the address of the object that is receiving messages
   //  from the circuit when the profile is in the Active state.
   //
   const LocalAddress& ObjAddr() const { return objAddr_; }

   //  Sets PORT as the object that is receiving messages from the circuit.
   // If the profile is in the Idle state, it enters the Active state.
   //
   bool SetObjAddr(const MsgPort& port);

   //  If PSM is registered as receiving messages from the circuit, its
   //  address is cleared.  If the profile is in the Active state, it
   //  enters the Idle state.  PSM may be nullptr, but then false will
   //  be returned.
   //
   bool ClearObjAddr(const ProtocolSM* psm);

   //  If ADDR is registered as receiving messages from the circuit, it is
   //  cleared.  If the profile is in the Active state, it enters the Idle
   //  state.
   //
   bool ClearObjAddr(const LocalAddress& addr);

   //  Deletes the profile.
   //
   bool Deregister();

   //  Adds the feature identified by FID to the profile.
   //
   bool Subscribe(PotsFeature::Id fid, CliThread& cli);  //d

   //  Removes the feature identified by FID from the profile.
   //
   bool Unsubscribe(PotsFeature::Id fid);

   //  Returns true if the profile has been assigned the feature
   //  identified by FID.
   //
   bool HasFeature(PotsFeature::Id fid) const;

   //  Returns the profile for the feature identified by FID.
   //  Returns nullptr if that feature is not subscribed.
   //
   PotsFeatureProfile* FindFeature(PotsFeature::Id fid) const;

   //  Returns the offset to dn_.
   //
   static ptrdiff_t CellDiff();

   //  Overridden for restarts.
   //
   void Startup(RestartLevel level) override;

   //  Overridden for restarts.
   //
   void Shutdown(RestartLevel level) override;

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;
protected:
   //  Protected to restrict deletion to Deregister.  Virtual to
   //  allow subclassing.
   //
   virtual ~PotsProfile();
private:
   //  The profile's directory number.
   //
   RegCell dn_;

   //  The profile's state.
   //
   State state_;

   //  The circuit associated with the profile.
   //
   std::unique_ptr< PotsCircuit > circuit_;

   //  The features assigned to the profile.
   //
   Q1Way< PotsFeatureProfile > featureq_;

   //  The address of the object that is receiving messages from the circuit.
   //
   LocalAddress objAddr_;
};
}
#endif
