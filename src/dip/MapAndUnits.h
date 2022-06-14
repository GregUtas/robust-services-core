//==============================================================================
//
//  MapAndUnits.h
//
//  Diplomacy AI Client - Part of the DAIDE project (www.daide.org.uk).
//
//  (C) David Norman 2002 david@ellought.demon.co.uk
//  (C) Greg Utas 2019-2022 greg@pentennea.com
//
//  This software may be reused for non-commercial purposes without charge,
//  and without notifying the authors.  Use of any part of this software for
//  commercial purposes without permission from the authors is prohibited.
//
#ifndef MAPANDUNITS_H_INCLUDED
#define MAPANDUNITS_H_INCLUDED

#include <cstddef>
#include <map>
#include <string>
#include <vector>
#include "DipTypes.h"
#include "Location.h"
#include "Province.h"
#include "SysTypes.h"
#include "Token.h"
#include "TokenMessage.h"
#include "UnitOrder.h"
#include "WinterOrders.h"

namespace Diplomacy
{
   struct ConvoySubversion;
}

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace Diplomacy
{
//  For mapping a province to the order for the unit located in it.
//
typedef std::map<ProvinceId, UnitOrder> UnitOrderMap;

//  For mapping a power to its winter orders.
//
typedef std::map<PowerId, WinterOrders> WinterOrderMap;

//  For mapping a province that contains a convoying army to
//  a description of whether that army subverts a convoy.
//
typedef std::map<ProvinceId, ConvoySubversion> ConvoySubversionMap;

//  A power and the centres that it owns.  Note that the power is
//  a token, whereas the centre is simply an index (ProvinceId).
//
struct PowerCentres
{
   const Token power;
   std::vector<ProvinceId> centres;

   explicit PowerCentres(const Token& power) : power(power) { }
};

//  A unit and its location.
//
struct UnitLocation
{
   const Token unit;
   const Location loc;

   UnitLocation(const Token& unit, const Location& loc) :
      unit(unit), loc(loc) { }
};

//  A power and the units that it owns.
//
struct PowerUnits
{
   const Token power;
   std::vector<UnitLocation> units;

   explicit PowerUnits(const Token& power) : power(power) { }
};

//  A power and its orders.
//
struct PowerOrders
{
   const Token power;
   std::vector<UnitOrder> orders;

   explicit PowerOrders(const Token& power) : power(power) { }
};

//------------------------------------------------------------------------------
//
//  The game position, which includes map details and the position of units.
//    In a server, this stores the current position and orders as they arrive
//  from clients.  When it is time to execute the orders, it adjudicates them
//  so that the results can be sent back to the clients.
//    In a client, this also stores the current position.  The client's orders
//  are also stored here so that they can be sent to the server.  After the
//  server replies with the adjudication results, the position is updated.
//    A client can obtain a copy of this using clone().  This allows the client
//  to enter orders for any powers and adjudicate them to find out what the
//  resulting position would be.  To prevent memory leaks, a client *must*
//  call delete_clone on each clone.
//
class MapAndUnits
{
public:
   Province game_map[PROVINCE_MAX];  // map details
   ProvinceId number_of_provinces;   // number of provinces on map
   PowerId number_of_powers;         // number of powers at outset
   std::string map_name;             // map's name
   Token our_power;                  // power that we are playing
   int passcode;                     // our passcode
   TokenMessage variant;             // rules for this variant
   ProvinceSet home_centres;         // our home centres
   bool game_started;                // set when the game has begun
   bool game_over;                   // set when the game has ended
   Token curr_season;                // current season of play
   int curr_year;                    // current year of play
   UnitOrderMap units;               // non-dislodged units
   UnitOrderMap dislodged_units;     // dislodged units
   WinterOrderMap winter_orders;     // winter orders
   WinterOrders our_winter_orders;   // our winter orders
   UnitOrderMap prev_movements;      // results of previous movement turn
   UnitOrderMap prev_retreats;       // results of previous retreat turn
   WinterOrderMap prev_adjustments;  // results of previous adjustment turn
   Token prev_movement_season;       // season for previous movement results

   //  Server options.
   //
   bool check_on_submission;    // check orders when submitted (not an AOA game)
   bool check_on_adjudication;  // check orders when adjudicated (AOA game)

   //  The following sets are built after each turn.  A client may modify these
   //  while calculating orders to submit (e.g. to delete each unit as an order
   //  is decided upon for it).
   //
   UnitSet our_units;              // our units
   UnitSet our_dislodged_units;    // our units that must retreat or disband
   ProvinceSet open_home_centres;  // our home centres available for builds
   ProvinceSet our_centres;        // the centres that we currently own
   word our_number_of_disbands;    // disbands required (negative for builds)

   /////////////////////////////////////////////////////////////////////////////
   //
   //  FUNCTIONS
   //
   /////////////////////////////////////////////////////////////////////////////

   //  Returns the singleton instance, which users should treat as read-only.
   //
   static MapAndUnits* instance();

   //  Returns a copy of the current instance, which can be modified.  It
   //  must be freed with delete_clone to avoid leaking memory.
   //
   static MapAndUnits* create_clone();

   //  Deletes CLONE and sets it to nullptr.
   //
   static void delete_clone(MapAndUnits*& clone);

   //  Returns the token for PROVINCE.  Returns INVALID_TOKEN and generates
   //  a log if PROVINCE is out of range.
   //
   Token province_token(ProvinceId province) const;

   //  Used to process the type of message indicated.
   //
   size_t process_map(const TokenMessage& map);
   size_t process_mdf(const TokenMessage& mdf);
   size_t process_hlo(const TokenMessage& hlo);
   size_t process_now(const TokenMessage& now);
   size_t process_sco(const TokenMessage& sco);
   size_t process_ord(const TokenMessage& ord);

   //  Sets an adjustment order or the order for a unit.
   //
   bool set_hold_order(ProvinceId unit);
   bool set_move_order(ProvinceId unit, const Location& dest);
   bool set_support_to_hold_order(ProvinceId unit, ProvinceId client);
   bool set_support_to_move_order
      (ProvinceId unit, ProvinceId client, ProvinceId dest);
   bool set_convoy_order(ProvinceId unit, ProvinceId client, ProvinceId dest);
   bool set_move_by_convoy_order(ProvinceId unit,
      ProvinceId dest, size_t length, const ProvinceId fleets[]);
   bool set_move_by_single_step_convoy_order
      (ProvinceId unit, ProvinceId dest, ProvinceId fleet);
   bool set_disband_order(ProvinceId unit);
   bool set_retreat_order(ProvinceId unit, const Location& dest);
   void set_build_order(const Location& location);
   bool set_remove_order(ProvinceId unit);
   void set_waive_order();
   void set_multiple_waive_orders(size_t waives);
   void set_total_number_of_waive_orders(size_t waives);

   //  Finds any adjustment order for PROVINCE in our_winter_orders.  The
   //  order is a Location that is interpreted as described under the type
   //  Adjustments (see above).  The location contains NIL_PROVINCE if no
   //  adjustment order for PROVINCE was found.
   //
   Location find_adjustment(ProvinceId province) const;

   //  Cancels the adjustment order for PROVINCE in our_winter_orders.
   //
   bool cancel_build_order(ProvinceId province);
   bool cancel_remove_order(ProvinceId province);

   //  Cancels the adjustment order in a NOT SUB message.
   //
   bool unorder_adjustment(const TokenMessage& not_sub, PowerId power);

   //  Returns true if any units have had orders submitted.
   //
   bool any_orders_entered() const;

   //  Clears all orders.
   //
   void clear_all_orders();

   //  Returns all submitted orders in a SUB message that can be sent to
   //  the server.
   //
   TokenMessage build_sub() const;

   //  Accepts the orders in a SUB message on behalf of POWER.  Updates
   //  RESULTS with any errors detected, based upon whether orders are
   //  checked upon submission or adjudication.  Returns NO_ERROR unless
   //  SUB is not a SUB message.  The orders are *not* adjudicated, only
   //  checked and entered in each UnitOrder.
   //
   size_t process_sub(const TokenMessage& sub, PowerId power, Token results[]);

   //  Returns true if OPTION was among those that the HLO message specified,
   //  in which case SETTING (if provided) is updated to any value associated
   //  with the option (e.g. the game's numeric press level).
   //
   bool get_variant_setting
      (const Token& option, Token* setting = nullptr) const;

   //  Builds a NOW message that reports the location of all units.
   //
   void build_now(TokenMessage& now) const;

   //  Builds an SCO message that specifies the owner of each supply centre.
   //
   void build_sco(TokenMessage& sco) const;

   //  Displays an item as a string that the user can read.
   //
   std::string display_movement_result(const UnitOrder& unit) const;
   std::string display_movement_order(const UnitOrder& unit,
      const UnitOrderMap& unit_set) const;
   std::string display_retreat_result(const UnitOrder& unit) const;
   std::string display_province(ProvinceId province) const;

   //  Looks for a unit that ended the season in PROVINCE and returns its
   //  location when the season began.  Sets
   //  o is_new_build if the unit was just built in PROVINCE
   //  o retreated_to_province if the unit retreated to PROVINCE
   //  o moved_to_province if the unit moved to PROVINCE
   //  o unit_found if PROVINCE contains a unit (if no other flag is set,
   //    this means that the unit also started the season in PROVINCE)
   //  Returns {NIL_PROVINCE, INVALID_TOKEN} if PROVINCE is currently empty.
   //
   Location find_result_unit_initial_location(ProvinceId province,
      bool& is_new_build, bool& retreated_to_province,
      bool& moved_to_province, bool& unit_found) const;

   //  Returns the locations that are adjacent to LOCATION.
   //
   const LocationSet* get_neighbours(const Location& location) const;

   //  Returns the location to which the unit in PROVINCE could move.
   //  Returns nullptr if PROVINCE does not contain a unit.
   //
   const LocationSet* get_destinations(ProvinceId province) const;

   //  Returns the locations to which the unit in PROVINCE could retreat.
   //  Returns nullptr if PROVINCE does not contain a dislodged unit.
   //
   const LocationSet*
      get_dislodged_unit_destinations(ProvinceId province) const;

   //  Returns the powers and their adjudicated orders for the most
   //  recent SEASON (excluding Winter).
   //
   std::vector<PowerOrders> get_orders(const Token& season) const;

   //  Returns the powers and the units that they currently own.
   //
   std::vector<PowerUnits> get_units() const;

   //  Returns the powers and the centres that they currently own.
   //
   std::vector<PowerCentres> get_centres() const;

   //  Sets the order checking options.
   //
   void set_order_checking(bool on_submission, bool on_adjudication);

   //  Returns true if POWER has submitted an order for each of its units.
   //
   bool all_orders_received(PowerId power) const;

   //  Adjudicates the submitted orders.
   //
   void adjudicate();

   //  Builds ORD messages to report the results of adjudication.  Returns
   //  the number of messages built.
   //
   size_t get_adjudication_results(TokenMessage ord_messages[]) const;

   //  Applies the adjudication by moving all units to their new positions.
   //  Returns true if an SCO message should now be sent.
   //
   bool apply_adjudication();

   //  Encodes the position of a single unit for inclusion in a message.
   //
   TokenMessage encode_unit(const UnitOrder& unit) const;

   //  Returns the number of supply centres currently owned by POWER.
   //
   size_t get_centre_count(const Token& power) const;

   //  Returns the number of units currently owned by POWER.
   //
   size_t get_unit_count(const Token& power) const;
private:
   //  Constructor.  Private because this is a singleton.
   //
   MapAndUnits();

   //  Destructor.  Private because this is a singleton.  All our dynamically
   //  allocated data is managed by containers, which free their own memory.
   //
   ~MapAndUnits() = default;

   //  Copy constructor.
   //
   MapAndUnits(const MapAndUnits& that) = default;

   //  Copy operator.
   //
   MapAndUnits& operator=(const MapAndUnits& that) = default;

   //  Used to implement process_mdf.
   //
   size_t process_powers(const TokenMessage& powers);
   size_t process_provinces(const TokenMessage& provinces);
   size_t process_supply_centres(const TokenMessage& centres);
   size_t process_supply_centres_for_power(const TokenMessage& centres);
   size_t process_non_supply_centres(const TokenMessage& centres);
   size_t process_adjacencies(const TokenMessage& adjacencies);
   size_t process_province_adjacency(const TokenMessage& adjacency);

   //  Used to implement process_hlo.
   //
   void set_our_power(const Token& token);

   //  Used to implement process_now.
   //
   size_t process_now_unit(const TokenMessage& unit_parm);

   //  Used to implement process_sco.
   //
   size_t process_sco_for_power(const TokenMessage& sco_parm);

   //  Used to implement build_now.
   //
   TokenMessage encode_dislodged_unit(const UnitOrder& unit) const;

   //  Used to implement build_sub.
   //
   TokenMessage encode_movement_order(const UnitOrder& unit) const;
   TokenMessage encode_retreat_order(const UnitOrder& unit) const;

   //  Used to implement get_adjudication_results.
   //
   size_t get_movement_results(TokenMessage ord_messages[]) const;
   size_t get_retreat_results(TokenMessage ord_messages[]) const;
   size_t get_adjustment_results(TokenMessage ord_messages[]) const;
   TokenMessage encode_movement_result(const UnitOrder& unit) const;
   TokenMessage encode_retreat_result(const UnitOrder& unit) const;
   TokenMessage encode_build_result(PowerId power,
      const WinterOrders& orders, const Location& location) const;
   TokenMessage encode_waive(PowerId power) const;

   //  Used to implement process_sub.
   //
   Token process_order(const TokenMessage& order, PowerId power);
   UnitOrder* find_unit(const TokenMessage& unit_to_find,
      UnitOrderMap& units_map) const;

   //  Used during order checking.
   //
   bool can_move_to(const UnitOrder& unit, const Location& dest) const;
   bool can_move_to_province(const UnitOrder& unit, ProvinceId province) const;
   bool has_route_to_province(const UnitOrder& unit, ProvinceId province,
      ProvinceId exclude) const;

   //  Encodes the game turn (season and year) for inclusion in a message.
   //
   TokenMessage encode_turn() const;

   //  Encodes a location for inclusion in a message.
   //
   TokenMessage encode_location(const Location& location) const;

   /////////////////////////////////////////////////////////////////////////////
   //
   //  ADJUDICATION TYPES AND CONSTANTS
   //
   /////////////////////////////////////////////////////////////////////////////

   //  For mapping a province to the provinces whose units were
   //  ordered to move there.
   //
   typedef std::multimap<ProvinceId, ProvinceId> AttackMap;

   /////////////////////////////////////////////////////////////////////////////
   //
   //  ADJUDICATION FUNCTIONS
   //
   /////////////////////////////////////////////////////////////////////////////

   void adjudicate_moves();
   void adjudicate_retreats();
   void adjudicate_builds();
   void initialise_move_adjudication();
   void check_for_illegal_move_orders();
   void check_for_illegal_retreat_orders();
   void cancel_inconsistent_convoys();
   void cancel_inconsistent_supports();
   void direct_attacks_cut_support();
   void build_support_lists();
   void build_convoy_subversion_list();
   bool resolve_attacks_on_unsubverted_convoys();
   bool check_for_futile_convoys();
   bool check_for_indomitable_and_futile_convoys();
   void resolve_circles_of_subversion();
   void identify_attack_rings_and_head_to_head_battles();
   void advance_attack_rings();
   void resolve_unbalanced_head_to_head_battles();
   void resolve_balanced_head_to_head_battles();
   void fight_ordinary_battles();
   void apply_moves();
   void apply_retreats();
   void apply_builds();
   bool move_to_next_turn();
   bool update_sc_ownership();

   void cut_support(ProvinceId province);
   void resolve_attacks_on_province(ProvinceId province);
   bool resolve_attacks_on_occupied_province(ProvinceId province);
   void advance_unit(ProvinceId from_province);
   void bounce_attack(UnitOrder& unit);
   void bounce_all_attacks_on_province(ProvinceId dest);
   ProvinceId find_empty_province_invader(ProvinceId dest);
   ProvinceId find_dislodger(ProvinceId province, bool ignore_occupant = false);
   RingUnitStatus calc_ring_status(ProvinceId to_prov, ProvinceId from_prov);
   void generate_cd_disbands(PowerId power, WinterOrders& orders);
   size_t distance_from_home(const UnitOrder& unit) const;

   /////////////////////////////////////////////////////////////////////////////
   //
   //  ADJUDICATION DATA
   //
   /////////////////////////////////////////////////////////////////////////////

   //  Maps a destination province to each province trying to move to it.
   //
   AttackMap attacks;

   //  Provinces whose units are supporting an action.
   //
   UnitSet supporting_units;

   //  Provinces whose fleets are convoying.
   //
   UnitSet convoying_units;

   //  Provinces whose armies are convoying.
   //
   UnitSet convoyed_units;

   //  Provinces whose armies are subverting a convoy.
   //
   ConvoySubversionMap subversions;

   //  Provinces involved in an attack circuit.
   //
   UnitSet attack_rings;

   //  Provinces involved in a balanced head-to-head attack.
   //
   UnitSet balanced_head_to_heads;

   //  Provinces involved in a unbalanced head-to-head attack.
   //
   UnitSet unbalanced_head_to_heads;

   //  Provinces into which all moves were bounced.
   //
   ProvinceSet bounce_provinces;
};
}
#endif
