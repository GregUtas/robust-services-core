//==============================================================================
//
//  UnitOrder.h
//
//  Diplomacy AI Client - Part of the DAIDE project (www.daide.org.uk).
//
//  (C) David Norman 2002 david@ellought.demon.co.uk
//  (C) Greg Utas 2019 greg@pentennea.com
//
//  This software may be reused for non-commercial purposes without charge,
//  and without notifying the authors.  Use of any part of this software for
//  commercial purposes without permission from the authors is prohibited.
//
#ifndef UNITORDER_H_INCLUDED
#define UNITORDER_H_INCLUDED

#include <cstdint>
#include <iosfwd>
#include "DipTypes.h"
#include "Location.h"
#include "Token.h"

//------------------------------------------------------------------------------

namespace Diplomacy
{
//  Information about a unit.
//
enum OrderType : uint8_t
{
   NO_ORDER,               // no order specified
   HOLD_ORDER,
   MOVE_ORDER,
   SUPPORT_TO_HOLD_ORDER,
   SUPPORT_TO_MOVE_ORDER,
   CONVOY_ORDER,
   MOVE_BY_CONVOY_ORDER,
   RETREAT_ORDER,
   DISBAND_ORDER,
   HOLD_NO_SUPPORT_ORDER   // internal to adjudicator
};

//  Used by the adjudicator to resolve a circular attack (e.g. A-B, B-C, C-A,
//  possibly with supports and units outside the ring also trying to enter A,
//  B, or C).  The status indicates what will happen in a destination province
//  that is a member of the ring.
//
enum RingUnitStatus : uint8_t
{
   NIL_RING_STATUS,           // default value if not calculated
   RING_ADVANCES_REGARDLESS,  // unit in ring advances to destination
   RING_ADVANCES_IF_VACANT,   // unit in ring advances iff destination does
   STANDOFF_REGARDLESS,       // stand-off in destination
   SIDE_ADVANCES_IF_VACANT,   // unit outside ring advances to destination
   SIDE_ADVANCES_REGARDLESS   // unit outside ring advances iff destination does
};

//  The order for a unit.
//
struct UnitOrder
{
   Location loc;            // unit's location
   PowerId owner;           // unit's owner
   Token unit_type;         // unit's type

   OrderType order;         // unit's order
   Location dest;           // unit's destination
   ProvinceId client_loc;   // location of unit being supported or convoyed
   ProvinceId client_dest;  // destination of supported or convoyed unit
   UnitList convoyers;      // fleets specified by an army's convoy order

   //  Used during adjudication.
   //
   UnitSet supports;              // supporters
   OrderType order_type_copy;     // can revert to HOLD or HOLD_NO_SUPPORT
   uint8_t supports_to_dislodge;  // net number of supports for unit's move
   ProvinceId dislodged_from;     // province unit was dislodged from
   bool is_support_to_dislodge;   // set if giving support to dislodge
   int8_t move_number;            // used to detect rings and head-to-heads
   RingUnitStatus ring_status;    // unit's status within a ring of attack

   //  Results of adjudication.
   //
   bool no_convoy;             // not all fleets ordered the convoy
   bool no_army_to_convoy;     // army not ordered to use the convoy, or
                               //   other fleets broke the route
   bool convoy_disrupted;      // convoying fleet dislodged: convoy failed
   bool support_void;          // supported unit did something else
   bool support_cut;           // support cut by an attack
   bool bounce;                // move bounced
   bool dislodged;             // unit was dislodged
   bool unit_moves;            // move was successful
   bool illegal_order;         // illegal order (only occurs in an AOA game)
   Token illegal_reason;       // reason that order was illegal
   LocationSet open_retreats;  // locations to which unit may retreat

   //  Initializes fields to default values.
   //
   UnitOrder();

   //  Marks a move illegal for REASON.
   //
   void mark_move_illegal(const Token& reason);

   //  Bounces the unit's move.
   //
   void mark_move_bounced();

   //  Disrupts the unit's convoy.
   //
   void mark_convoy_disrupted();

   //  Updates the unit with the order specified in an ORD.
   //
   void decode_order(const TokenMessage& ord);

   //  Updates the unit with the order result specified in an ORD.
   //
   void decode_result(const TokenMessage& result);
};

//  Inserts a string that describes the unit and its location into STREAM.
//
std::ostream& operator<<(std::ostream& stream, const UnitOrder& order);
}
#endif
