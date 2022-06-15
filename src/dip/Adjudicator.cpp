//==============================================================================
//
//  Adjudicator.cpp
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
#include "MapAndUnits.h"
#include <cstdint>
#include <set>
#include "Debug.h"

using namespace NodeBase;

//  Open questions.  Search on "<a>" etc for the source code locations.
//  <a> This line checks if a unit is supporting itself.  But isn't its own
//      province unreachable?  can_move_to_province should have returned false.
//  <b> Shouldn't all invocations of ConvoySubversions.clear() be replaced
//      by decrement()?
//  <c> This line implies that an adjacent location could be in the same
//      province.
//  <d> This line was absent from the original source but is present in
//      what is basically the same loop in process_order().
//  <e> If the unit is still in the province from which it was dislodged,
//      wouldn't can_move_to have returned false?
//  <f> If this is true, wouldn't the check that precedes it also be true?
//  <g> How could the dislodger be in the other province of a balanced
//      head-to-head battle?
//  <h> How could the dislodger of the stronger unit be the weaker one in
//      an unbalanced head-to-head battle?
//
//------------------------------------------------------------------------------

namespace Diplomacy
{
fn_name MapAndUnits_adjudicate = "MapAndUnits.adjudicate";

void MapAndUnits::adjudicate()
{
   Debug::ft(MapAndUnits_adjudicate);

   switch(curr_season.all())
   {
   case TOKEN_SEASON_SPR:
   case TOKEN_SEASON_FAL:
      adjudicate_moves();
      break;

   case TOKEN_SEASON_SUM:
   case TOKEN_SEASON_AUT:
      adjudicate_retreats();
      break;

   case TOKEN_SEASON_WIN:
      adjudicate_builds();
      break;

   default:
      Debug::SwLog(MapAndUnits_adjudicate, "invalid season", curr_season.all());
   }
}

//------------------------------------------------------------------------------

void MapAndUnits::adjudicate_builds()
{
   Debug::ft("MapAndUnits.adjudicate_builds");

   //  Check that each power has ordered enough builds or disbands.
   //
   for(PowerId p = 0; p < number_of_powers; ++p)
   {
      auto& orders = winter_orders[p];

      if(orders.is_building)
      {
         if(orders.adjustments.size() +
            orders.number_of_waives < orders.number_of_orders_required)
         {
            //  Too few builds ordered.  Waive the remaining builds.
            //
            orders.number_of_waives =
               orders.number_of_orders_required - orders.adjustments.size();
         }
      }
      else if(orders.adjustments.size() < orders.number_of_orders_required)
      {
         //  Too few disbands ordered.  Disband using the default rules.
         //
         generate_cd_disbands(p, orders);
      }
   }

   //  The builds are all valid now, so mark them as such.
   //
   for(PowerId p = 0; p < number_of_powers; ++p)
   {
      auto& orders = winter_orders[p];

      for(auto a = orders.adjustments.begin();
         a != orders.adjustments.end(); ++a)
      {
         a->second = TOKEN_RESULT_SUC;
      }
   }
}

//------------------------------------------------------------------------------

void MapAndUnits::adjudicate_moves()
{
   Debug::ft("MapAndUnits.adjudicate_moves");

   auto changes_made = true;
   auto futile_convoys_checked = false;
   auto futile_and_indomitable_convoys_checked = false;

   initialise_move_adjudication();

   if(check_on_adjudication)
   {
      check_for_illegal_move_orders();
   }

   cancel_inconsistent_convoys();
   cancel_inconsistent_supports();
   direct_attacks_cut_support();
   build_support_lists();
   build_convoy_subversion_list();

   while(changes_made)
   {
      changes_made = resolve_attacks_on_unsubverted_convoys();

      if(!changes_made && !futile_convoys_checked)
      {
         changes_made = check_for_futile_convoys();
         futile_convoys_checked = true;
      }

      if(!changes_made && !futile_and_indomitable_convoys_checked)
      {
         changes_made = check_for_indomitable_and_futile_convoys();
         futile_and_indomitable_convoys_checked = true;
      }
   }

   resolve_circles_of_subversion();
   identify_attack_rings_and_head_to_head_battles();
   advance_attack_rings();
   resolve_unbalanced_head_to_head_battles();
   resolve_balanced_head_to_head_battles();
   fight_ordinary_battles();
}

//------------------------------------------------------------------------------

void MapAndUnits::adjudicate_retreats()
{
   Debug::ft("MapAndUnits.adjudicate_retreats");

   AttackMap retreat_map;

   //  Initialise each dislodged unit.
   //
   for(auto u = dislodged_units.begin(); u != dislodged_units.end(); ++u)
   {
      auto& unit = u->second;

      unit.order_type_copy = unit.order;
      unit.bounce = false;
      unit.unit_moves = false;
   }

   if(check_on_adjudication)
   {
      check_for_illegal_retreat_orders();
   }

   //  Check each dislodged unit that was ordered to retreat.
   //
   for(auto u = dislodged_units.begin(); u != dislodged_units.end(); ++u)
   {
      auto& unit = u->second;

      if(unit.order_type_copy == RETREAT_ORDER)
      {
         //  See if another unit is trying to retreat to the same province.
         //
         auto r = retreat_map.find(unit.dest.province);

         if(r != retreat_map.end())
         {
            //  Yes, so bounce both units.
            //
            unit.bounce = true;

            auto& bouncing_unit = dislodged_units[r->second];

            bouncing_unit.unit_moves = false;
            bouncing_unit.bounce = true;
         }
         else
         {
            //  No, so assume the unit moves for now.  However, we may later
            //  discover another unit which is trying to retreat to the same
            //  province.
            //
            retreat_map.insert(AttackMap::value_type
               (unit.dest.province, unit.loc.province));
            unit.unit_moves = true;
         }
      }
   }
}

//------------------------------------------------------------------------------

void MapAndUnits::advance_attack_rings()
{
   Debug::ft("MapAndUnits.advance_attack_rings");

   UnitList::iterator ring_member_iterator;

   for(auto r = attack_rings.begin(); r != attack_rings.end(); ++r)
   {
      //  Build the list of units in the ring in reverse order.
      //
      auto first_province = *r;
      auto& ring_unit = units[first_province];
      auto ring_breaking_prov = NIL_PROVINCE;
      UnitList units_in_ring;

      do
      {
         units_in_ring.push_front(ring_unit.loc.province);

         //  This unit is the ring breaker if it can't advance.
         //
         ring_unit.ring_status =
            calc_ring_status(ring_unit.dest.province, ring_unit.loc.province);

         if((ring_unit.ring_status != RING_ADVANCES_REGARDLESS) &&
            (ring_unit.ring_status != RING_ADVANCES_IF_VACANT))
         {
            ring_breaking_prov = ring_unit.loc.province;
            ring_member_iterator = units_in_ring.begin();
         }

         ring_unit = units[ring_unit.dest.province];
      }
      while(ring_unit.loc.province != first_province);

      if(ring_breaking_prov == NIL_PROVINCE)
      {
         //  Each unit in the ring advances.
         //
         for(auto u = units_in_ring.begin(); u != units_in_ring.end(); ++u)
         {
            advance_unit(*u);
         }

         continue;  // on to next ring
      }

      //  Check the status of the ring breaker.
      //
      ring_unit = units[ring_breaking_prov];

      if(ring_unit.ring_status == STANDOFF_REGARDLESS)
      {
         bounce_all_attacks_on_province(ring_unit.dest.province);
      }
      else if(ring_unit.ring_status == SIDE_ADVANCES_REGARDLESS)
      {
         bounce_attack(ring_unit);
      }
      else
      {
         //  We don't know what happens in the province that this unit
         //  is moving to, so try the previous unit in the ring.
         //
         if(++ring_member_iterator == units_in_ring.end())
         {
            ring_member_iterator = units_in_ring.begin();
         }

         ring_unit = units[*ring_member_iterator];

         //  The unit after this one is not moving, so check this one.
         //
         if(ring_unit.ring_status == SIDE_ADVANCES_REGARDLESS)
         {
            bounce_attack(ring_unit);
         }
         else if(ring_unit.ring_status != RING_ADVANCES_REGARDLESS)
         {
            bounce_all_attacks_on_province(ring_unit.dest.province);
         }
         else
         {
            //  This unit will advance.  Work backwards until we find one
            //  that won't.
            do
            {
               if(++ring_member_iterator == units_in_ring.end())
               {
                  ring_member_iterator = units_in_ring.begin();
               }

               ring_unit = units[*ring_member_iterator];

               if((ring_unit.ring_status == SIDE_ADVANCES_REGARDLESS) ||
                  (ring_unit.ring_status == SIDE_ADVANCES_IF_VACANT))
               {
                  bounce_attack(ring_unit);
               }
               else if(ring_unit.ring_status == STANDOFF_REGARDLESS)
               {
                  bounce_all_attacks_on_province(ring_unit.dest.province);
               }
            }
            while((ring_unit.ring_status == RING_ADVANCES_IF_VACANT) ||
               (ring_unit.ring_status == RING_ADVANCES_REGARDLESS));
         }
      }
   }
}

//------------------------------------------------------------------------------

void MapAndUnits::advance_unit(ProvinceId from_province)
{
   Debug::ft("MapAndUnits.advance_unit");

   //  The unit in FROM_PROVINCE will move to its DEST, and all
   //  other units trying to move to DEST will be bounced.
   //
   auto& attacker = units[from_province];
   auto dest = attacker.dest.province;

   attacker.unit_moves = true;

   for(auto a = attacks.lower_bound(dest); a != attacks.upper_bound(dest); ++a)
   {
      auto& unit = units[a->second];

      if(unit.loc.province != from_province)
      {
         unit.mark_move_bounced();
      }
   }

   //  All attempts to move to DEST have now been resolved.
   //
   attacks.erase(dest);
}

//------------------------------------------------------------------------------

fn_name MapAndUnits_apply_adjudication = "MapAndUnits.apply_adjudication";

bool MapAndUnits::apply_adjudication()
{
   Debug::ft(MapAndUnits_apply_adjudication);

   switch(curr_season.all())
   {
   case TOKEN_SEASON_SPR:
   case TOKEN_SEASON_FAL:
      apply_moves();
      break;

   case TOKEN_SEASON_SUM:
   case TOKEN_SEASON_AUT:
      apply_retreats();
      break;

   case TOKEN_SEASON_WIN:
      apply_builds();
      break;

   default:
      Debug::SwLog(MapAndUnits_apply_adjudication,
         "invalid season", curr_season.all());
   }

   return move_to_next_turn();
}

//------------------------------------------------------------------------------

void MapAndUnits::apply_builds()
{
   Debug::ft("MapAndUnits.apply_builds");

   for(PowerId p = 0; p < number_of_powers; ++p)
   {
      auto& orders = winter_orders[p];

      for(auto a = orders.adjustments.begin();
         a != orders.adjustments.end(); ++a)
      {
         if(orders.is_building)
         {
            //  Add the newly constructed unit to the global set of units.
            //
            UnitOrder unit;

            unit.loc = a->first;
            unit.owner = p;

            if(unit.loc.coast == TOKEN_UNIT_AMY)
               unit.unit_type = TOKEN_UNIT_AMY;
            else
               unit.unit_type = TOKEN_UNIT_FLT;

            units.insert(UnitOrderMap::value_type(unit.loc.province, unit));
         }
         else
         {
            //  Erase the removed unit from the global set of units.
            //
            units.erase(a->first.province);
         }
      }
   }
}

//------------------------------------------------------------------------------

void MapAndUnits::apply_moves()
{
   Debug::ft("MapAndUnits.apply_moves");

   UnitOrderMap moved_units;

   //  Run through all the units.  Erase those whose locations changed, adding
   //  them to the list of moved or dislodged units as appropriate.
   //
   dislodged_units.clear();

   auto u = units.begin();

   while(u != units.end())
   {
      auto& unit = u->second;

      unit.order = NO_ORDER;

      if(unit.unit_moves)
      {
         moved_units.insert(UnitOrderMap::value_type(unit.dest.province, unit));
         u = units.erase(u);
      }
      else if(unit.dislodged)
      {
         dislodged_units.insert
            (UnitOrderMap::value_type(unit.loc.province, unit));
         u = units.erase(u);
      }
      else
      {
         ++u;
      }
   }

   //  Put the moved units in their new locations.  The dislodged units
   //  will await their retreat orders.
   //
   for(u = moved_units.begin(); u != moved_units.end(); ++u)
   {
      auto& unit = u->second;
      unit.loc = unit.dest;
      units.insert(UnitOrderMap::value_type(unit.dest.province, unit));
   }

   //  Provide the retreat options for each dislodged unit.
   //
   for(u = dislodged_units.begin(); u != dislodged_units.end(); ++u)
   {
      u->second.open_retreats.clear();

      auto& neighbours =
         game_map[u->second.loc.province].neighbours[u->second.loc.coast];

      for(auto n = neighbours.begin(); n != neighbours.end(); ++n)
      {
         if((n->province != u->second.dislodged_from) &&  // <c>
            (units.find(n->province) == units.end()) &&
            (bounce_provinces.find(n->province) == bounce_provinces.end()))
         {
            u->second.open_retreats.insert(*n);
         }
      }
   }
}

//------------------------------------------------------------------------------

void MapAndUnits::apply_retreats()
{
   Debug::ft("MapAndUnits.apply_retreats");

   //  Clear the order for all dislodged units.  Put each one that moved in
   //  its new location and clear the set of dislodged units when done.
   //
   for(auto u = dislodged_units.begin(); u != dislodged_units.end(); ++u)
   {
      auto& unit = u->second;

      unit.order = NO_ORDER;

      if(unit.unit_moves)
      {
         unit.loc = unit.dest;
         units.insert(UnitOrderMap::value_type(unit.dest.province, unit));
      }
   }

   dislodged_units.clear();
}

//------------------------------------------------------------------------------

void MapAndUnits::bounce_all_attacks_on_province(ProvinceId dest)
{
   Debug::ft("MapAndUnits.bounce_all_attacks_on_province");

   //  Bounce all moves to DEST.
   //
   for(auto a = attacks.lower_bound(dest); a != attacks.upper_bound(dest); ++a)
   {
      auto& unit = units[a->second];
      unit.mark_move_bounced();
   }

   //  Remove all bounced units from the attacker map and add DEST
   //  to the list of provinces to which moves were bounced.
   //
   attacks.erase(dest);
   bounce_provinces.insert(dest);
}

//------------------------------------------------------------------------------

void MapAndUnits::bounce_attack(UnitOrder& unit)
{
   Debug::ft("MapAndUnits.bounce_attack");

   //  Mark UNIT's move as bouncing and remove it from the list of
   //  attacks on its destination now that it has been resolved.
   //
   unit.mark_move_bounced();

   auto a = attacks.lower_bound(unit.dest.province);

   while(a != attacks.upper_bound(unit.dest.province))
   {
      if(a->second == unit.loc.province)
         a = attacks.erase(a);
      else
         ++a;
   }
}

//------------------------------------------------------------------------------

void MapAndUnits::build_convoy_subversion_list()
{
   Debug::ft("MapAndUnits.build_convoy_subversion_list");

   //  Check each convoying army to see if it subverts another convoy.
   //  If it does, record the army whose convoy is being subverted.
   //
   for(auto a = convoyed_units.begin(); a != convoyed_units.end(); ++a)
   {
      ConvoySubversion subversion;
      auto& army = units[*a];
      auto d = units.find(army.dest.province);

      if(d != units.end())
      {
         auto& defender = d->second;

         if(defender.owner != army.owner)
         {
            if(defender.order_type_copy == SUPPORT_TO_HOLD_ORDER)
            {
               auto& client = units[defender.client_loc];

               if(client.order_type_copy == CONVOY_ORDER)  // (a)
               {
                  subversion.subverted_army = client.client_loc;
               }
            }
            else if(defender.order_type_copy == SUPPORT_TO_MOVE_ORDER)
            {
               auto u = units.find(defender.client_dest);

               if(u != units.end())
               {
                  auto& client_target = u->second;

                  if(client_target.order_type_copy == CONVOY_ORDER)  // (b)
                  {
                     subversion.subverted_army = client_target.client_loc;
                  }
               }
            }
         }
      }

      //  Record the subversion against the convoying army that would cut
      //  a support as described above.  If the army would not cut such a
      //  support, the subverted_army field is still NIL_PROVINCE.
      //
      subversions[*a] = subversion;
   }

   //  Find each army that is subverting a convoy and mark that convoy as
   //  subverted (its key is its own convoying army).
   //
   for(auto s = subversions.begin(); s != subversions.end(); ++s)
   {
      if(s->second.subverted_army != NIL_PROVINCE)
      {
         auto subverted = subversions.find(s->second.subverted_army);
         subverted->second.subversion_type = SUBVERTED_CONVOY;
         subverted->second.number_of_subversions++;
      }
   }

   //  We're now finished with the convoyed_units set.  All further work on
   //  convoyed units is done through the subversion map.  There is no need
   //  to update convoyed_units for the rest of the adjudicator.
}

//------------------------------------------------------------------------------

void MapAndUnits::build_support_lists()
{
   Debug::ft("MapAndUnits.build_support_lists");

   //  Add each supporting unit to the set of supports for its client.
   //
   for(auto s = supporting_units.begin(); s != supporting_units.end(); ++s)
   {
      auto& supporter = units[*s];
      auto& client = units[supporter.client_loc];

      client.supports.insert(*s);

      //  A support to move is valid for dislodgement if
      //  o the attacked province is empty, or
      //  o the unit in the attacked province belongs to
      //    neither the supporter nor its client.
      //
      if(supporter.order_type_copy == SUPPORT_TO_MOVE_ORDER)
      {
         auto dest = units.find(supporter.client_dest);

         if(dest == units.end())
         {
            supporter.is_support_to_dislodge = true;
            ++client.supports_to_dislodge;
         }
         else
         {
            auto& defender = dest->second;

            if((supporter.owner != defender.owner) &&
               (client.owner != defender.owner))
            {
               supporter.is_support_to_dislodge = true;
               ++client.supports_to_dislodge;
            }
         }
      }
   }

   //  We're now finished with the supporting_units set.  All further work on
   //  supporting units is done through their clients, so there is no need to
   //  update supporting_units for the rest of the adjudicator.
}

//------------------------------------------------------------------------------

RingUnitStatus MapAndUnits::calc_ring_status
   (ProvinceId to_prov, ProvinceId from_prov)
{
   Debug::ft("MapAndUnits.calc_ring_status");

   size_t most_supports = 0;
   size_t most_supports_to_dislodge = 0;
   size_t second_most_supports = 0;
   ProvinceId most_supported_unit = NIL_PROVINCE;

   //  Find the strength of the most and second most supported units.
   //
   for(auto a = attacks.lower_bound(to_prov);
      a != attacks.upper_bound(to_prov); ++a)
   {
      auto& attacker = units[a->second];
      auto supports = attacker.supports.size();

      if(supports > most_supports)
      {
         second_most_supports = most_supports;
         most_supports = supports;
         most_supports_to_dislodge = attacker.supports_to_dislodge;
         most_supported_unit = a->second;
      }
      else if(supports > second_most_supports)
      {
         second_most_supports = attacker.supports.size();
      }
   }

   //  The status of the ring depends on the strength of the two strongest
   //  units that are trying to enter TO_PROV.
   //
   if(most_supports == second_most_supports)
   {
      return STANDOFF_REGARDLESS;  // standoff in TO_PROV
   }

   if(most_supported_unit == from_prov)
   {
      if((most_supports_to_dislodge > 0) &&
         (most_supports_to_dislodge > second_most_supports))
      {
         return RING_ADVANCES_REGARDLESS;  // FROM_PROV enters TO_PROV
      }
      else
      {
         return RING_ADVANCES_IF_VACANT;  // FROM_PROV enters TO_PROV only
                                          // if TO_PROV also moves
      }
   }

   if((most_supports_to_dislodge > 0) &&
      (most_supports_to_dislodge > second_most_supports))
   {
      return SIDE_ADVANCES_REGARDLESS;  // a unit outside ring enters TO_PROV
   }

   return SIDE_ADVANCES_IF_VACANT;  // a unit outside ring enters TO_PROV
                                    // only if TO_PROV also moves
}

//------------------------------------------------------------------------------

void MapAndUnits::cancel_inconsistent_convoys()
{
   Debug::ft("MapAndUnits.cancel_inconsistent_convoys");

   //  For all armies moving by convoy, check that all required fleets
   //  were ordered to convoy it.
   //
   auto a = convoyed_units.begin();

   while(a != convoyed_units.end())
   {
      auto order_ok = true;
      auto& army = units[*a];

      for(auto f = army.convoyers.begin(); f != army.convoyers.end(); ++f)
      {
         auto u = units.find(*f);

         if(u == units.end())
         {
            order_ok = false;
         }
         else
         {
            auto& fleet = u->second;

            if((fleet.order_type_copy != CONVOY_ORDER) ||
               (fleet.client_loc != army.loc.province) ||
               (fleet.client_dest != army.dest.province))
            {
               order_ok = false;
            }
         }
      }

      if(!order_ok)
      {
         army.order_type_copy = HOLD_NO_SUPPORT_ORDER;
         army.no_convoy = true;
         a = convoyed_units.erase(a);
      }
      else
      {
         ++a;
      }
   }

   //  For all convoying fleets, check that the army was ordered to
   //  make use of the convoy.
   //
   auto f = convoying_units.begin();

   while(f != convoying_units.end())
   {
      auto order_ok = true;
      auto& fleet = units[*f];
      auto u = units.find(fleet.client_loc);

      if(u == units.end())
      {
         order_ok = false;
      }
      else
      {
         auto& army = u->second;

         if((army.order != MOVE_BY_CONVOY_ORDER) ||
            (army.loc.province != fleet.client_loc) ||
            (army.dest.province != fleet.client_dest))
         {
            order_ok = false;
         }
         else if(army.order_type_copy != MOVE_BY_CONVOY_ORDER)
         {
            //  The army was ordered to convoy, but other fleets
            //  failed to complete the chain.
            //
            order_ok = false;
         }
      }

      if(!order_ok)
      {
         fleet.no_army_to_convoy = true;
         fleet.order_type_copy = HOLD_ORDER;
         f = convoying_units.erase(f);
      }
      else
      {
         ++f;
      }
   }

   //  We're now finished with the convoying_units set.  All further work on
   //  convoying units is done through each army's convoyers list.  There is
   //  no need to update convoying_units for the rest of the adjudicator.
}

//------------------------------------------------------------------------------

void MapAndUnits::cancel_inconsistent_supports()
{
   Debug::ft("MapAndUnits.cancel_inconsistent_supports");

   //  For all supports to hold, check that the client isn't moving.  For
   //  all supports to move, check that the client is moving as expected.
   //
   auto s = supporting_units.begin();

   while(s != supporting_units.end())
   {
      auto order_ok = true;
      auto& supporter = units[*s];
      auto c = units.find(supporter.client_loc);

      if(c == units.end())
      {
         //  The client does not exist.
         //
         order_ok = false;
         supporter.support_void = true;
      }
      else
      {
         auto& client = c->second;

         switch(supporter.order_type_copy)
         {
         case SUPPORT_TO_HOLD_ORDER:
            switch(client.order_type_copy)
            {
            case MOVE_ORDER:
            case MOVE_BY_CONVOY_ORDER:
            case HOLD_NO_SUPPORT_ORDER:  // client tried to move but failed
               order_ok = false;
               supporter.support_void = true;
            }
            break;

         case SUPPORT_TO_MOVE_ORDER:
            if((client.order != MOVE_ORDER) &&
               (client.order != MOVE_BY_CONVOY_ORDER))
            {
               //  The client wasn't ordered to move.
               //
               order_ok = false;
               supporter.support_void = true;
            }
            else if(client.dest.province != supporter.client_dest)
            {
               //  The client was ordered to move to a different location.
               //
               order_ok = false;
               supporter.support_void = true;
            }
            else if((client.order_type_copy != MOVE_ORDER) &&
                  (client.order_type_copy != MOVE_BY_CONVOY_ORDER))
            {
               //  The client was ordered as supported, but its move failed.
               //
               order_ok = false;
            }
         }
      }

      if(!order_ok)
      {
         //  The support failed, so the supporter will just hold.
         //
         supporter.order_type_copy = HOLD_ORDER;
         s = supporting_units.erase(s);
      }
      else
      {
         ++s;
      }
   }
}

//------------------------------------------------------------------------------

bool MapAndUnits::check_for_futile_convoys()
{
   Debug::ft("MapAndUnits.check_for_futile_convoys");

   //  Find each subverted convoy and try to resolve it by checking its
   //  fleets for dislodgement.
   //
   auto changes_made = false;
   auto s = subversions.begin();

   while(s != subversions.end())
   {
      auto subverting_army_province = s->first;
      auto subverted_army_province = s->second.subverted_army;

      if(subverted_army_province != NIL_PROVINCE)
      {
         auto& subverting_army = units[subverting_army_province];
         auto& defender = units[subverting_army.dest.province];
         auto subverted_client_province = defender.client_loc;
         auto& subverted_army = units[subverted_army_province];
         auto disrupted = false;

         //  Resolve the attacks on each fleet except the subverted one.
         //
         for(auto f = subverted_army.convoyers.begin();
            f != subverted_army.convoyers.end(); ++f)
         {
            if(*f != subverted_client_province)
            {
               if(resolve_attacks_on_occupied_province(*f))
               {
                  disrupted = true;
               }
            }
         }

         //  If the convoy was disrupted, revert all of its units to hold.
         //
         if(disrupted)
         {
            subverted_army.mark_convoy_disrupted();

            //  The subverted convoy was disrupted, so it cannot subvert a
            //  convoy itself.
            //
            auto& subverted_convoy = subversions[subverted_army_province];
            auto nonsubverted =
               subversions.find(subverted_convoy.subverted_army);
            auto& nonsubverted_convoy = nonsubverted->second;
            nonsubverted_convoy.clear();  // <b>

            //  The convoy that disrupted this one has had its subversion
            //  resolved.
            //
            s->second.subverted_army = NIL_PROVINCE;
            subversions.erase(s);
            changes_made = true;
         }
      }

      s = subversions.upper_bound(subverting_army_province);
   }

   return changes_made;
}

//------------------------------------------------------------------------------

void MapAndUnits::check_for_illegal_move_orders()
{
   Debug::ft("MapAndUnits.check_for_illegal_move_orders");

   for(auto u = units.begin(); u != units.end(); ++u)
   {
      auto& unit = u->second;

      switch(unit.order)
      {
      case HOLD_ORDER:
         break;

      case MOVE_ORDER:
         if(!can_move_to(unit, unit.dest))
         {
            unit.mark_move_illegal(TOKEN_ORDER_NOTE_FAR);
         }
         break;

      case SUPPORT_TO_HOLD_ORDER:
      {
         auto& client = units[unit.client_loc];

         if(!can_move_to_province(unit, client.loc.province))
            unit.mark_move_illegal(TOKEN_ORDER_NOTE_FAR);
         else if(client.loc.province == unit.loc.province)  // <a>
            unit.mark_move_illegal(TOKEN_ORDER_NOTE_FAR);
         break;
      }

      case SUPPORT_TO_MOVE_ORDER:
      {
         auto& client = units[unit.client_loc];

         if(!can_move_to_province(unit, unit.client_dest))
            unit.mark_move_illegal(TOKEN_ORDER_NOTE_FAR);
         else if(client.loc.province == unit.loc.province)
            unit.mark_move_illegal(TOKEN_ORDER_NOTE_FAR);
         break;
      }

      case CONVOY_ORDER:
      {
         auto& client = units[unit.client_loc];

         if(unit.unit_type != TOKEN_UNIT_FLT)
            unit.mark_move_illegal(TOKEN_ORDER_NOTE_NSF);
         else if(game_map[unit.loc.province].is_land)
            unit.mark_move_illegal(TOKEN_ORDER_NOTE_NAS);
         else if(client.unit_type != TOKEN_UNIT_AMY)
            unit.mark_move_illegal(TOKEN_ORDER_NOTE_NSA);
         break;
      }

      case MOVE_BY_CONVOY_ORDER:
      {
         if(unit.unit_type != TOKEN_UNIT_AMY)
         {
            unit.mark_move_illegal(TOKEN_ORDER_NOTE_NSA);
            break;
         }

         auto previous_province = unit.loc.province;
         UnitOrder* convoy_order = nullptr;

         for(auto f = unit.convoyers.begin(); f != unit.convoyers.end(); ++f)
         {
            auto fleet = units.find(*f);

            if(fleet != units.end())
            {
               convoy_order = &fleet->second;
            }

            if(convoy_order == nullptr)
            {
               unit.mark_move_illegal(TOKEN_ORDER_NOTE_NSF);
               break;
            }
            else if(game_map[convoy_order->loc.province].is_land)
            {
               unit.mark_move_illegal(TOKEN_ORDER_NOTE_NAS);
               break;
            }
            else if(!can_move_to_province(*convoy_order, previous_province))
            {
               unit.mark_move_illegal(TOKEN_ORDER_NOTE_FAR);
               break;
            }

            previous_province = convoy_order->loc.province;  // <d>
         }

         if(!unit.illegal_order)
         {
            if(!can_move_to_province(*convoy_order, unit.client_dest))
               unit.mark_move_illegal(TOKEN_ORDER_NOTE_FAR);
            else if(unit.dest.province == unit.loc.province)
               unit.mark_move_illegal(TOKEN_ORDER_NOTE_FAR);
         }

         break;
      }

      default:
         unit.order_type_copy = HOLD_ORDER;
      }
   }
}

//------------------------------------------------------------------------------

void MapAndUnits::check_for_illegal_retreat_orders()
{
   Debug::ft("MapAndUnits.check_for_illegal_retreat_orders");

   for(auto u = dislodged_units.begin(); u != dislodged_units.end(); ++u)
   {
      auto& unit = u->second;

      if(!can_move_to(unit, unit.dest))
      {
         unit.mark_move_illegal(TOKEN_ORDER_NOTE_FAR);
         return;
      }

      //  The unit can't retreat to
      //  o the province from which it was dislodged
      //  o an occupied province
      //  o a province that was left vacant because of a stand-off
      //
      if((unit.dislodged_from == unit.dest.province) ||  // <e>
         (units.find(unit.dest.province) != units.end()) ||
         (bounce_provinces.find(unit.dest.province) != bounce_provinces.end()))
      {
         unit.mark_move_illegal(TOKEN_ORDER_NOTE_NVR);
      }
   }
}

//------------------------------------------------------------------------------

bool MapAndUnits::check_for_indomitable_and_futile_convoys()
{
   Debug::ft("MapAndUnits.check_for_indomitable_and_futile_convoys");

   bool changes_made = false;

   //  Try to resolve each subverted convoy.
   //
   auto s = subversions.begin();

   while(s != subversions.end())
   {
      auto subverting_army_province = s->first;
      auto subverted_army_province = s->second.subverted_army;

      if(subverted_army_province != NIL_PROVINCE)
      {
         auto& subverting_army = units[subverting_army_province];
         auto& defender = units[subverting_army.dest.province];
         auto subverted_province = NIL_PROVINCE;
         UnitOrder* supported_fleet = nullptr;

         if(defender.order_type_copy == SUPPORT_TO_HOLD_ORDER)
         {
            subverted_province = defender.client_loc;
            supported_fleet = &units[subverted_province];
         }
         else if(defender.order_type_copy == SUPPORT_TO_MOVE_ORDER)
         {
            subverted_province = defender.client_dest;
            supported_fleet = &units[defender.client_loc];
         }

         if(subverted_province != NIL_PROVINCE)
         {
            auto& subverted_army = units[subverted_army_province];

            //  Find the convoy that this one subverts.
            //
            auto subverted = subversions.find(subverted_army_province);

            if(subverted != subversions.end())
            {
               auto& subverted_convoy = subverted->second;

               //  Find the dislodging unit when the defender's support is
               //  intact.  Then remove the defender's support, find the
               //  dislodging unit again, and restore the defender's support.
               //
               auto dislodger_if_not_cut = find_dislodger(subverted_province);

               supported_fleet->supports.erase(defender.loc.province);

               if(defender.is_support_to_dislodge)
               {
                  --supported_fleet->supports_to_dislodge;
               }

               auto dislodger_if_cut = find_dislodger(subverted_province);

               supported_fleet->supports.insert(defender.loc.province);

               if(defender.is_support_to_dislodge)
               {
                  ++supported_fleet->supports_to_dislodge;
               }

               //  Determine if the convoy is
               //  (a) futile: if dislodged with or without the support
               //  (b) indomitable: if dislodged in neither case
               //  (c) subverted: if dislodged only if the support is cut
               //      (in this case, it remains unresolved for now)
               //  (d) confused: if dislodged only if the support is NOT cut
               //      (this is a Pandin's Paradox scenario that will result
               //      in its failure, but without any dislodgements)
               //
               if(dislodger_if_not_cut != NIL_PROVINCE)
               {
                  if(dislodger_if_cut != NIL_PROVINCE)
                  {
                     subverted_army.mark_convoy_disrupted();  // (a)

                     //  This convoy was disrupted, so it cannot subvert a
                     //  convoy itself.
                     //
                     auto nonsubverted =
                        subversions.find(subverted_army_province);

                     if(nonsubverted != subversions.end())
                     {
                        auto& nonsubverted_convoy = nonsubverted->second;
                        nonsubverted_convoy.clear();  // <b>
                     }

                     //  The convoy that was subverting this one no longer
                     //  has a convoy to subvert, as this one will fail.
                     //
                     s->second.subverted_army = NIL_PROVINCE;
                     changes_made = true;
                  }
               }
               else if(dislodger_if_cut != NIL_PROVINCE)
               {
                  subverted_convoy.subversion_type = CONFUSED_CONVOY;  // (d)
               }
               else  // (c)
               {
                  //  This convoy will succeed on the next invocation of
                  //  resolve_attacks_on_unsubverted_convoys, when its
                  //  army will successfully cut support.  It is no longer
                  //  subverting a convoy, because its outcome is now known.
                  //
                  auto nonsubverted = subversions.find(subverted_army_province);

                  if(nonsubverted != subversions.end())
                  {
                     auto& nonsubverted_convoy = nonsubverted->second;
                     nonsubverted_convoy.clear();  // <b>
                  }

                  //  The convoy that was subverting this one no longer
                  //  has a convoy to subvert, as this one will succeed.
                  //
                  s->second.subverted_army = NIL_PROVINCE;
                  changes_made = true;
               }
            }
         }
      }

      s = subversions.upper_bound(subverting_army_province);
   }

   return changes_made;
}

//------------------------------------------------------------------------------

void MapAndUnits::cut_support(ProvinceId province)
{
   Debug::ft("MapAndUnits.cut_support");

   auto u = units.find(province);

   if(u != units.end())
   {
      auto& cut_unit = u->second;

      //  If the unit in PROVINCE is giving support, cut it.
      //
      if((cut_unit.order_type_copy == SUPPORT_TO_HOLD_ORDER) ||
         (cut_unit.order_type_copy == SUPPORT_TO_MOVE_ORDER))
      {
         auto& client = units[cut_unit.client_loc];

         client.supports.erase(cut_unit.loc.province);

         if(cut_unit.is_support_to_dislodge)
         {
            client.supports_to_dislodge--;
         }

         cut_unit.order_type_copy = HOLD_ORDER;
         cut_unit.support_cut = true;
      }
   }
}

//------------------------------------------------------------------------------

void MapAndUnits::direct_attacks_cut_support()
{
   Debug::ft("MapAndUnits.direct_attacks_cut_support");

   //  For each moving unit, see if its destination province is occupied.
   //
   for(auto a = attacks.begin(); a != attacks.end(); ++a)
   {
      auto& attacker = units[a->second];
      auto target = units.find(attacker.dest.province);

      if(target != units.end())
      {
         //  A unit in the destination province is under attack.  If it has
         //  been ordered to support, its support is cut if
         //  o it does not belong to the same power; and
         //  o it is not supporting an attack on the attacking unit.
         //
         auto& defender = target->second;

         if((defender.order_type_copy == SUPPORT_TO_HOLD_ORDER) ||
            (defender.order_type_copy == SUPPORT_TO_MOVE_ORDER))
         {
            if((defender.owner != attacker.owner) &&
               (defender.client_dest != attacker.loc.province))
            {
               defender.support_cut = true;
               defender.order_type_copy = HOLD_ORDER;
               supporting_units.erase(defender.loc.province);
            }
         }
      }
   }
}

//------------------------------------------------------------------------------

fn_name MapAndUnits_distance_from_home = "MapAndUnits.distance_from_home";

size_t MapAndUnits::distance_from_home(const UnitOrder& unit) const
{
   Debug::ft(MapAndUnits_distance_from_home);

   const auto& home_powers = game_map[unit.loc.province].home_powers;

   //  See if the unit is already in a home centre.
   //
   if(home_powers.find(unit.owner) == home_powers.end())
   {
      return 0;
   }

   //  Put all provinces at an "infinite" distance.  Set the unit's
   //  province to zero distance and repeatedly expand the set of
   //  reached provinces until a home centre is reached.
   //
   size_t distance[PROVINCE_MAX];

   for(ProvinceId p = 0; p < number_of_provinces; ++p)
   {
      distance[p] = SIZE_MAX;
   }

   distance[unit.loc.province] = 0;

   for(size_t d = 0; d < PROVINCE_MAX; ++d)
   {
      for(ProvinceId p = 0; p < number_of_provinces; ++p)
      {
         if(distance[p] == d)
         {
            //  Province P is at distance D, so it was reached during the
            //  previous iteration.  Now look at its unreached neighbours.
            //
            auto& neighbours = game_map[p].neighbours;

            for(auto n = neighbours.begin(); n != neighbours.end(); ++n)
            {
               for(auto adj = n->second.begin(); adj != n->second.end(); ++adj)
               {
                  if(distance[adj->province] == SIZE_MAX)
                  {
                     //  This province is currently unreached.  If it's
                     //  a home centre, we're done; otherwise add it to
                     //  the provinces at distance D + 1.
                     //
                     const auto& powers = game_map[adj->province].home_powers;

                     if(powers.find(unit.owner) != powers.end())
                     {
                        return (d + 1);
                     }

                     distance[adj->province] = d + 1;
                  }
               }
            }
         }
      }
   }

   //  The unit did not reach a home center!  Something is very wrong indeed.
   //
   Debug::SwLog(MapAndUnits_distance_from_home, "home centre not found", 0);
   return SIZE_MAX;
}

//------------------------------------------------------------------------------

void MapAndUnits::fight_ordinary_battles()
{
   Debug::ft("MapAndUnits.fight_ordinary_battles");

   //  AttackMap contains all units that are trying to move.  When a unit's
   //  move is resolved, it is removed from the map, so keep resolving moves
   //  until none remain.
   //
   while(!attacks.empty())
   {
      auto a = attacks.begin();
      resolve_attacks_on_province(a->first);
   }
}

//------------------------------------------------------------------------------

ProvinceId MapAndUnits::find_dislodger
   (ProvinceId province, bool ignore_occupant)
{
   Debug::ft("MapAndUnits.find_dislodger");

   size_t most_supports = 0;
   size_t most_supports_to_dislodge = 0;
   size_t second_most_supports = 0;
   ProvinceId most_supported = NIL_PROVINCE;

   //  Find the number of supports for the two strongest attacks.
   //
   for(auto a = attacks.lower_bound(province);
      a != attacks.upper_bound(province); ++a)
   {
      auto& attacker = units[a->second];
      auto attacker_supports = attacker.supports.size();

      if(attacker_supports > most_supports)
      {
         second_most_supports = most_supports;
         most_supports = attacker_supports;
         most_supports_to_dislodge = attacker.supports_to_dislodge;
         most_supported = a->second;
      }
      else if(attacker_supports > second_most_supports)
      {
         second_most_supports = attacker_supports;
      }
   }

   //  If we need to consider the occupant, compare it to the second
   //  strongest attack.
   //
   if(!ignore_occupant)
   {
      auto& occupant = units[province];
      auto occupant_supports = occupant.supports.size();

      if(occupant_supports > second_most_supports)
      {
         second_most_supports = occupant_supports;
      }
   }

   //  The strongest attack advances if it has more support than the
   //  second strongest.
   //
   if(most_supports_to_dislodge <= second_most_supports)  // <f>
   {
      return NIL_PROVINCE;
   }

   return most_supported;
}

//------------------------------------------------------------------------------

ProvinceId MapAndUnits::find_empty_province_invader(ProvinceId dest)
{
   Debug::ft("MapAndUnits.find_empty_province_invader");

   size_t most_supports = 0;
   size_t second_most_supports = 0;
   ProvinceId most_supported_prov = NIL_PROVINCE;

   //  Find the strength of the most supported and second
   //  most supported units that are trying to enter DEST.
   //
   for(auto a = attacks.lower_bound(dest); a != attacks.upper_bound(dest); ++a)
   {
      auto& attacker = units[a->second];
      auto supports = attacker.supports.size();

      if(supports > most_supports)
      {
         second_most_supports = most_supports;
         most_supports = supports;
         most_supported_prov = a->second;
      }
      else if(supports > second_most_supports)
      {
         second_most_supports = supports;
      }
   }

   //  If the strongest invasion isn't stronger
   //  than the second strongest, no one moves.
   //
   if(most_supports <= second_most_supports)
   {
      most_supported_prov = NIL_PROVINCE;
   }

   return most_supported_prov;
}

//------------------------------------------------------------------------------

void MapAndUnits::generate_cd_disbands(PowerId power, WinterOrders& orders)
{
   Debug::ft("MapAndUnits.generate_cd_disbands");

   typedef std::multimap<size_t, ProvinceId> DistanceFromHomeMap;

   DistanceFromHomeMap distances;

   //  For each of POWER's units, determine its distance from a home centre.
   //
   for(auto u = units.begin(); u != units.end(); ++u)
   {
      if(u->second.owner == power)
      {
         distances.insert(DistanceFromHomeMap::value_type
            (distance_from_home(u->second), u->first));
      }
   }

   //  Beginning with the unit farthest from a home centre, add disbands to
   //  ORDERS until the required number of disbands are present.
   //
   for(auto d = distances.rbegin(); d != distances.rend(); ++d)
   {
      if((orders.adjustments.size() < orders.number_of_orders_required) &&
         (orders.adjustments.find(units[d->second].loc) ==
         orders.adjustments.end()))
      {
         orders.adjustments.insert(Adjustments::value_type
            (units[d->second].loc, TOKEN_ORDER_NOTE_MBV));
      }
   }
}

//------------------------------------------------------------------------------

void MapAndUnits::identify_attack_rings_and_head_to_head_battles()
{
   Debug::ft("MapAndUnits.identify_attack_rings_and_head_to_head_battles");

   int move_counter = 0;

   //  Find head-to-head battles (A-B and B-A) and rings of attack (e.g. a
   //  circuit containing more than two units, such as A-B, B-C, and C-A).
   //
   for(auto a = attacks.begin(); a != attacks.end(); ++a)
   {
      auto& attacker = units[a->second];
      auto loop_found = false;
      auto chain_start = move_counter;
      auto chain_end_found = false;
      auto last_convoy = NIL_MOVE_NUMBER;

      while(!chain_end_found)
      {
         //  If the moving unit has a move number, it was already encountered.
         //  We've reached the end of the current chain.  And if the unit was
         //  found earlier within this chain, we've also found a loop.
         //
         if(attacker.move_number != NIL_MOVE_NUMBER)
         {
            chain_end_found = true;

            if(attacker.move_number >= chain_start)
            {
               loop_found = true;
            }
         }
         else if((attacker.order_type_copy != MOVE_ORDER) &&
               (attacker.order_type_copy != MOVE_BY_CONVOY_ORDER))
         {
            //  This unit will not move, which also means that we've reached
            //  the end of the current chain.
            //
            chain_end_found = true;
         }
         else
         {
            //  This is the first time that we've seen this unit.  Assign it a
            //  move number, which marks encountered units and detects loops.
            //
            attacker.move_number = move_counter;

            if(attacker.order_type_copy == MOVE_BY_CONVOY_ORDER)
            {
               last_convoy = move_counter;
            }

            ++move_counter;

            //  If the province to which this unit is moving contains a unit,
            //  continue the chain with that unit.
            //
            auto next = units.find(attacker.dest.province);

            if(next == units.end())
               chain_end_found = true;
            else
               attacker = next->second;
         }
      }

      //  A loop is either a ring of attacks or a head-to-head attack.  For a
      //  head-to-head attack, determine if it is balanced or unbalanced.
      //
      if(loop_found)
      {
         if((move_counter - attacker.move_number > 2) ||
            (last_convoy >= attacker.move_number))
         {
            //  The LAST_CONVOY check allows two units to exchange places
            //  (which is normally prohibited) if either is being convoyed
            //  (e.g. A Den-Kie VIA Bal, F Bal C Den-Kie, A Kie-Den).
            //
            attack_rings.insert(attacker.loc.province);
         }
         else
         {
            auto& other = units[attacker.dest.province];

            if(attacker.supports_to_dislodge > other.supports.size())
               unbalanced_head_to_heads.insert(attacker.loc.province);
            else if(other.supports_to_dislodge > attacker.supports.size())
               unbalanced_head_to_heads.insert(other.loc.province);
            else
               balanced_head_to_heads.insert(attacker.loc.province);
         }
      }
   }
}

//------------------------------------------------------------------------------

void MapAndUnits::initialise_move_adjudication()
{
   Debug::ft("MapAndUnits.initialise_move_adjudication");

   //  Clear all lists of units.
   //
   attacks.clear();
   supporting_units.clear();
   convoying_units.clear();
   convoyed_units.clear();
   subversions.clear();
   attack_rings.clear();
   balanced_head_to_heads.clear();
   unbalanced_head_to_heads.clear();
   bounce_provinces.clear();

   //  Set up units to start adjudicating.
   //
   for(auto u = units.begin(); u != units.end(); ++u)
   {
      auto& unit = u->second;

      unit.order_type_copy = unit.order;
      unit.supports.clear();
      unit.supports_to_dislodge = 0;
      unit.is_support_to_dislodge = false;
      unit.no_convoy = false;
      unit.no_army_to_convoy = false;
      unit.convoy_disrupted = false;
      unit.support_void = false;
      unit.support_cut = false;
      unit.bounce = false;
      unit.dislodged = false;
      unit.unit_moves = false;
      unit.move_number = NIL_MOVE_NUMBER;
      unit.illegal_order = false;

      //  Add the unit to the appropriate set based on its order.
      //
      switch(unit.order)
      {
      case MOVE_ORDER:
         attacks.insert(AttackMap::value_type(unit.dest.province, u->first));
         break;

      case SUPPORT_TO_HOLD_ORDER:
      case SUPPORT_TO_MOVE_ORDER:
         supporting_units.insert(u->first);
         break;

      case CONVOY_ORDER:
         convoying_units.insert(u->first);
         break;

      case MOVE_BY_CONVOY_ORDER:
         convoyed_units.insert(u->first);
         break;
      }
   }
}

//------------------------------------------------------------------------------

fn_name MapAndUnits_move_to_next_turn = "MapAndUnits.move_to_next_turn";

bool MapAndUnits::move_to_next_turn()
{
   Debug::ft(MapAndUnits_move_to_next_turn);

   auto new_turn_found = false;
   auto send_sco = false;

   //  Step through the seasons until we find one in which orders are
   //  required.  This is always the case in spring or fall.  Summer
   //  and autumn are only required if there are retreats, and winter
   //  is only required if there are adjustments.
   //
   while(!new_turn_found)
   {
      if(curr_season == TOKEN_SEASON_WIN)
      {
         curr_season = TOKEN_SEASON_SPR;
         curr_year++;
      }
      else
      {
         curr_season = curr_season.all() + 1;
      }

      switch(curr_season.all())
      {
      case TOKEN_SEASON_SPR:
      case TOKEN_SEASON_FAL:
         new_turn_found = true;
         break;

      case TOKEN_SEASON_SUM:
      case TOKEN_SEASON_AUT:
         if(!dislodged_units.empty())
         {
            new_turn_found = true;
         }
         break;

      case TOKEN_SEASON_WIN:
         if(update_sc_ownership())
         {
            new_turn_found = true;
         }

         //  Send an SCO message whether or not adjustments are required.
         //
         send_sco = true;
         break;

      default:
         Debug::SwLog(MapAndUnits_move_to_next_turn,
            "invalid season", curr_season.all());
         return false;
      }
   }

   return send_sco;
}

//------------------------------------------------------------------------------

bool MapAndUnits::resolve_attacks_on_occupied_province(ProvinceId province)
{
   Debug::ft("MapAndUnits.resolve_attacks_on_occupied_province");

   //  If no unit can dislodge the occupant, bounce all attempts to enter
   //  the province and report that its occupant was not dislodged.
   //
   auto dislodger = find_dislodger(province);

   if(dislodger == NIL_PROVINCE)
   {
      bounce_all_attacks_on_province(province);
      return false;
   }

   //  The occupant was dislodged, so cut any support that it is providing.
   //  Advance the successful attacker and dislodge the occupant.
   //
   cut_support(province);
   advance_unit(dislodger);

   auto& occupant = units[province];
   occupant.dislodged = true;
   occupant.dislodged_from = dislodger;
   return true;
}

//------------------------------------------------------------------------------

void MapAndUnits::resolve_attacks_on_province(ProvinceId province)
{
   Debug::ft("MapAndUnits.resolve_attacks_on_province");

   auto occupant = units.find(province);

   if(occupant != units.end())
   {
      //  If the unit that currently occupies the province is moving,
      //  resolve its move.  If it moves successfully, the province is
      //  then unoccupied.
      //
      auto& occupier = occupant->second;

      if(!occupier.unit_moves)
      {
         if((occupier.order_type_copy == MOVE_ORDER) ||
            (occupier.order_type_copy == MOVE_BY_CONVOY_ORDER))
         {
            resolve_attacks_on_province(occupier.dest.province);
         }
      }

      if(occupier.unit_moves)
      {
         occupant = units.end();
      }
   }

   //  If the province is still occupied, see if the occupant can be dislodged.
   //  If it is unoccupied, see if any unit can enter successfully: either all
   //  attempts to do so bounce, or one succeeds.
   //
   if(occupant != units.end())
   {
      resolve_attacks_on_occupied_province(province);
   }
   else
   {
      auto dislodger = find_empty_province_invader(province);

      if(dislodger == NIL_PROVINCE)
         bounce_all_attacks_on_province(province);
      else
         advance_unit(dislodger);
   }
}

//------------------------------------------------------------------------------

bool MapAndUnits::resolve_attacks_on_unsubverted_convoys()
{
   Debug::ft("MapAndUnits.resolve_attacks_on_unsubverted_convoys");

   //  Resolve attacks on each unsubverted convoy's fleets.
   //
   auto changes_made = false;
   auto s = subversions.begin();

   while(s != subversions.end())
   {
      if(s->second.subversion_type == UNSUBVERTED_CONVOY)
      {
         auto& army = units[s->first];
         auto disrupted = false;

         for(auto f = army.convoyers.begin(); f != army.convoyers.end(); ++f)
         {
            if(resolve_attacks_on_occupied_province(*f))
            {
               disrupted = true;
            }
         }

         //  If the convoy was disrupted, revert all of its units to hold.
         //  If it was not disrupted, cut any support being given by its
         //  destination's province, and add it to that province's attackers.
         //
         if(disrupted)
         {
            army.mark_convoy_disrupted();
         }
         else
         {
            cut_support(army.dest.province);
            attacks.insert(AttackMap::value_type
               (army.dest.province, army.loc.province));
         }

         //  If this army was subverting a convoy, it has now either cut any
         //  support at its destination or has had its convoy disrupted.  Its
         //  subversion has therefore been resolved.
         //
         if(s->second.subverted_army != NIL_PROVINCE)
         {
            auto& subverted_convoy = subversions[s->second.subverted_army];
            subverted_convoy.decrement();
         }

         //  This convoy has been processed.
         //
         s = subversions.erase(s);
         changes_made = true;
      }
      else
      {
         ++s;
      }
   }

   return changes_made;
}

//------------------------------------------------------------------------------

void MapAndUnits::resolve_balanced_head_to_head_battles()
{
   Debug::ft("MapAndUnits.resolve_balanced_head_to_head_battles");

   //  Consider each pair of units that are clashing head on (i.e. A-B
   //  and B-A).  The clash is balanced, so neither one will advance.
   //
   for(auto b = balanced_head_to_heads.begin();
      b != balanced_head_to_heads.end(); ++b)
   {
      auto& unit1 = units[*b];
      auto& unit2 = units[unit1.dest.province];

      //  See if either unit is being dislodged.
      //
      auto dislodger1 = find_dislodger(unit1.loc.province, true);
      auto dislodger2 = find_dislodger(unit2.loc.province, true);

      if((dislodger1 == NIL_PROVINCE) ||
         (dislodger1 == unit2.loc.province))  // <g>
      {
         //  Bounce all moves to the first province.
         //
         bounce_all_attacks_on_province(unit1.loc.province);
      }
      else
      {
         //  A unit not involved in the head-to-head is entering the
         //  first province.  Advance it and dislodge the occupant.
         //
         advance_unit(dislodger1);
         unit1.dislodged = true;
         unit1.dislodged_from = dislodger1;
      }

      if((dislodger2 == NIL_PROVINCE) ||
         (dislodger2 == unit1.loc.province))  // <g>
      {
         //  Bounce all moves to the second province.
         //
         bounce_all_attacks_on_province(unit2.loc.province);
      }
      else
      {
         //  A unit not involved in the head-to-head is entering the
         //  second province.  Advance it and dislodge the occupant.
         //
         advance_unit(dislodger2);
         unit2.dislodged = true;
         unit2.dislodged_from = dislodger2;
      }
   }
}

//------------------------------------------------------------------------------

void MapAndUnits::resolve_circles_of_subversion()
{
   Debug::ft("MapAndUnits.resolve_circles_of_subversion");

   auto s = subversions.begin();

   //  Continue until all subverted convoys have been resolved.
   //
   while(s != subversions.end())
   {
      //  Go through each convoy loop (subversion chain) to see
      //  if any convoy in the loop is confused.
      //
      auto subverting_army_province = s->first;
      auto confused_convoy_loop = false;
      auto next_convoyed_army = NIL_PROVINCE;

      do
      {
         if(s->second.subversion_type == CONFUSED_CONVOY)
         {
            confused_convoy_loop = true;
         }

         next_convoyed_army = s->second.subverted_army;
         s = subversions.find(next_convoyed_army);
      }
      while(next_convoyed_army != subverting_army_province);

      //  If any convoy in the loop is confused, all attacks on convoys
      //  in the loop fail.  (If no convoy in the loop is confused, the
      //  attacks on those convoys will be resolved during the next
      //  invocation of resolve_attacks_on_unsubverted_convoys.)
      //
      if(confused_convoy_loop)
      {
         s = subversions.begin();

         do
         {
            //  Bounce all attacks on this convoy's fleets and
            //  remove those attacks from the attackers map.
            //
            auto& subverting_army = units[s->first];

            for(auto f = subverting_army.convoyers.begin();
               f != subverting_army.convoyers.end(); ++f)
            {
               for(auto a = attacks.lower_bound(*f);
                  a != attacks.upper_bound(*f); ++a)
               {
                  auto& attacker = units[a->second];
                  attacker.mark_move_bounced();
               }

               attacks.erase(*f);
            }

            //  Continue with the next convoy in the loop.
            //
            next_convoyed_army = s->second.subverted_army;
            s = subversions.find(next_convoyed_army);
         }
         while(s != subversions.begin());
      }

      //  All convoys in the loop also fail.
      //
      s = subversions.begin();

      while(s != subversions.end())
      {
         auto& subverting_army = units[s->first];
         subverting_army.mark_convoy_disrupted();

         //  This subversion has now been resolved, so remove it from
         //  the list and continue with the next one.
         //
         next_convoyed_army = s->second.subverted_army;
         subversions.erase(s);
         s = subversions.find(next_convoyed_army);
      }

      //  Continue with the next subversion loop.
      //
      s = subversions.begin();
   }
}

//------------------------------------------------------------------------------

void MapAndUnits::resolve_unbalanced_head_to_head_battles()
{
   Debug::ft("MapAndUnits.resolve_unbalanced_head_to_head_battles");

   //  Consider each pair of units that are clashing head on (i.e. A-B and B-A).
   //  The clash is unbalanced, so the stronger will dislodge the weaker unless
   //  another attack on the weaker province is just as strong or stronger.
   //
   for(auto u = unbalanced_head_to_heads.begin();
      u != unbalanced_head_to_heads.end(); ++u)
   {
      auto& stronger = units[*u];
      auto& weaker = units[stronger.dest.province];

      //  If the stronger unit is the one that will dislodge the weaker, bounce
      //  and dislodge the weaker and advance the stronger.
      //
      auto dislodger_of_weaker = find_dislodger(weaker.loc.province, true);

      if(dislodger_of_weaker == stronger.loc.province)
      {
         bounce_attack(weaker);
         advance_unit(stronger.loc.province);
         weaker.dislodged = true;
         weaker.dislodged_from = stronger.loc.province;
      }
      else
      {
         //  Bounce the weaker unit and see if it was dislodged.
         //
         bounce_attack(weaker);

         if(dislodger_of_weaker != NIL_PROVINCE)
         {
            advance_unit(dislodger_of_weaker);
            weaker.dislodged = true;
            weaker.dislodged_from = dislodger_of_weaker;
         }
         else
         {
            //  No one dislodged the weaker unit, so the stronger unit's
            //  attack must have been equally matched by another.  The
            //  weaker unit is a "beleaguered garrison".
            //
            bounce_all_attacks_on_province(weaker.loc.province);
         }

         //  The stronger unit did not advance to the weaker province.  If the
         //  stronger unit was not dislodged, all moves to its province bounce.
         //
         auto dislodger_of_stronger =
            find_dislodger(stronger.loc.province, true);

         if((dislodger_of_stronger == NIL_PROVINCE) ||
            (dislodger_of_stronger == weaker.loc.province))  // <h>
         {
            bounce_all_attacks_on_province(stronger.loc.province);
         }
         else
         {
            advance_unit(dislodger_of_stronger);
            stronger.dislodged = true;
            stronger.dislodged_from = dislodger_of_stronger;
         }
      }
   }
}

//------------------------------------------------------------------------------

bool MapAndUnits::update_sc_ownership()
{
   Debug::ft("MapAndUnits.update_sc_ownership");

   size_t unit_count[POWER_MAX];
   size_t sc_count[POWER_MAX];
   auto orders_required = false;

   for(PowerId p = 0; p < number_of_powers; ++p)
   {
      unit_count[p] = 0;
      sc_count[p] = 0;
   }

   //  Update the ownership of all occupied provinces and count units.
   //
   for(auto u = units.begin(); u != units.end(); ++u)
   {
      auto& unit = u->second;

      game_map[unit.loc.province].owner = power_token(unit.owner);
      ++unit_count[unit.owner];
   }

   //  Count supply centres.
   //
   for(ProvinceId p = 0; p < number_of_provinces; ++p)
   {
      if((game_map[p].is_supply_centre) &&
         (game_map[p].owner != TOKEN_PARAMETER_UNO))
      {
         ++sc_count[game_map[p].owner.power_id()];
      }
   }

   //  Determine who is building and who is disbanding.  Clear each power's
   //  adjustment orders in preparation for the coming winter season.
   //
   for(PowerId p = 0; p < number_of_powers; ++p)
   {
      auto& orders = winter_orders[p];

      if(sc_count[p] > unit_count[p])
      {
         orders.is_building = true;
         orders.number_of_orders_required = sc_count[p] - unit_count[p];
      }
      else
      {
         orders.is_building = false;
         orders.number_of_orders_required = unit_count[p] - sc_count[p];
      }

      if(sc_count[p] != unit_count[p])
      {
         orders_required = true;
      }

      orders.number_of_waives = 0;
      orders.adjustments.clear();
   }

   return orders_required;
}
}
