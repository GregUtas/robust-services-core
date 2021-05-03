//==============================================================================
//
//  MapAndUnits.cpp
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
#include "MapAndUnits.h"
#include <cctype>
#include <iosfwd>
#include <list>
#include <sstream>
#include "ConvoySubversion.h"
#include "Debug.h"

using std::string;
using namespace NodeBase;

//------------------------------------------------------------------------------

namespace Diplomacy
{
MapAndUnits::MapAndUnits() :
   number_of_provinces(0),
   number_of_powers(0),
   passcode(0),
   game_started(false),
   game_over(false),
   curr_year(0),
   check_on_submission(true),
   check_on_adjudication(false),
   our_number_of_disbands(0)
{
   Debug::ft("MapAndUnits.ctor");
}

//------------------------------------------------------------------------------

bool MapAndUnits::all_orders_received(PowerId power) const
{
   Debug::ft("MapAndUnits.all_orders_received");

   switch(curr_season.all())
   {
   case TOKEN_SEASON_SPR:
   case TOKEN_SEASON_FAL:
      for(auto u = units.begin(); u != units.end(); ++u)
      {
         if((u->second.owner == power) && (u->second.order == NO_ORDER))
         {
            return false;
         }
      }
      break;

   case TOKEN_SEASON_SUM:
   case TOKEN_SEASON_AUT:
      for(auto u = dislodged_units.begin(); u != dislodged_units.end(); ++u)
      {
         if((u->second.owner == power) && (u->second.order == NO_ORDER))
         {
            return false;
         }
      }
      break;

   case TOKEN_SEASON_WIN:
      auto w = winter_orders.find(power);

      if(w != winter_orders.end())
      {
         if(w->second.number_of_orders_required >
            (w->second.adjustments.size() + w->second.number_of_waives))
         {
            return false;
         }
      }
   }

   return true;
}

//------------------------------------------------------------------------------

fn_name MapAndUnits_any_orders_entered = "MapAndUnits.any_orders_entered";

bool MapAndUnits::any_orders_entered() const
{
   Debug::ft(MapAndUnits_any_orders_entered);

   switch(curr_season.all())
   {
   case TOKEN_SEASON_SPR:
   case TOKEN_SEASON_FAL:
      for(auto u = units.begin(); u != units.end(); ++u)
      {
         if(u->second.order != NO_ORDER)
         {
            return true;
         }
      }
      return false;

   case TOKEN_SEASON_SUM:
   case TOKEN_SEASON_AUT:
      for(auto u = dislodged_units.begin(); u != dislodged_units.end(); ++u)
      {
         if(u->second.order != NO_ORDER)
         {
            return true;
         }
      }
      return false;

   case TOKEN_SEASON_WIN:
      return (!our_winter_orders.adjustments.empty() ||
         (our_winter_orders.number_of_waives != 0));
   }

   Debug::SwLog(MapAndUnits_any_orders_entered,
      "invalid season", curr_season.all());
   return false;
}

//------------------------------------------------------------------------------

void MapAndUnits::build_now(TokenMessage& now) const
{
   Debug::ft("MapAndUnits.build_now");

   now = Token(TOKEN_COMMAND_NOW) + encode_turn();

   for(auto u = units.begin(); u != units.end(); ++u)
   {
      now = now + encode_unit(u->second);
   }

   for(auto u = dislodged_units.begin(); u != dislodged_units.end(); ++u)
   {
      now = now + encode_dislodged_unit(u->second);
   }
}

//------------------------------------------------------------------------------

void MapAndUnits::build_sco(TokenMessage& sco) const
{
   Debug::ft("MapAndUnits.build_sco");

   TokenMessage power_owned_scs[POWER_MAX];
   TokenMessage unowned_scs;

   for(ProvinceId p = 0; p < number_of_provinces; ++p)
   {
      if(game_map[p].is_supply_centre)
      {
         if(game_map[p].owner == TOKEN_PARAMETER_UNO)
         {
            unowned_scs = unowned_scs + game_map[p].token;
         }
         else
         {
            auto owner = game_map[p].owner.power_id();
            power_owned_scs[owner] = power_owned_scs[owner] + game_map[p].token;
         }
      }
   }

   sco = TOKEN_COMMAND_SCO;

   for(PowerId p = 0; p < number_of_powers; ++p)
   {
      if(!power_owned_scs[p].empty())
      {
         sco = sco & (power_token(p) + power_owned_scs[p]);
      }
   }

   if(!unowned_scs.empty())
   {
      sco = sco & (Token(TOKEN_PARAMETER_UNO) + unowned_scs);
   }
}

//------------------------------------------------------------------------------

TokenMessage MapAndUnits::build_sub() const
{
   Debug::ft("MapAndUnits.build_sub");

   TokenMessage sub(TOKEN_COMMAND_SUB);
   TokenMessage unit_order;

   switch(curr_season.all())
   {
   case TOKEN_SEASON_SPR:
   case TOKEN_SEASON_FAL:
      for(auto u = units.begin(); u != units.end(); ++u)
      {
         if((u->second.owner == our_power.power_id()) &&
            (u->second.order != NO_ORDER))
         {
            unit_order = encode_movement_order(u->second);
            sub = sub & unit_order;
         }
      }
      break;

   case TOKEN_SEASON_SUM:
   case TOKEN_SEASON_AUT:
      for(auto u = dislodged_units.begin(); u != dislodged_units.end(); ++u)
      {
         if((u->second.owner == our_power.power_id()) &&
            (u->second.order != NO_ORDER))
         {
            unit_order = encode_retreat_order(u->second);
            sub = sub & unit_order;
         }
      }
      break;

   case TOKEN_SEASON_WIN:
      for(auto o = our_winter_orders.adjustments.begin();
         o != our_winter_orders.adjustments.end(); ++o)
      {
         unit_order = our_power;

         if(o->first.coast == TOKEN_UNIT_AMY)
            unit_order = unit_order + Token(TOKEN_UNIT_AMY);
         else
            unit_order = unit_order + Token(TOKEN_UNIT_FLT);

         unit_order = unit_order + encode_location(o->first);
         unit_order.enclose_this();

         if(our_winter_orders.is_building)
            unit_order = unit_order + Token(TOKEN_ORDER_BLD);
         else
            unit_order = unit_order + Token(TOKEN_ORDER_REM);

         sub = sub & unit_order;
      }

      for(size_t w = 0; w < our_winter_orders.number_of_waives; ++w)
      {
         unit_order = our_power + Token(TOKEN_ORDER_WVE);
         sub = sub & unit_order;
      }
   }

   return sub;
}

//------------------------------------------------------------------------------

bool MapAndUnits::can_move_to(const UnitOrder& unit, const Location& dest) const
{
   auto& neighbours = game_map[unit.loc.province].neighbours;
   auto reachable = neighbours.find(unit.loc.coast);

   if(reachable != neighbours.end())
   {
      if(reachable->second.find(dest) != reachable->second.end())
      {
         return true;
      }
   }

   return false;
}

//------------------------------------------------------------------------------

bool MapAndUnits::can_move_to_province
   (const UnitOrder& unit, ProvinceId province) const
{
   auto& neighbours = game_map[unit.loc.province].neighbours;
   auto reachable = neighbours.find(unit.loc.coast);

   if(reachable != neighbours.end())
   {
      auto& adjacencies = reachable->second;
      Location first_coast(province, 0);
      auto match = adjacencies.lower_bound(first_coast);

      if((match != adjacencies.end()) && (match->province == province))
      {
         return true;
      }
   }

   return false;
}

//------------------------------------------------------------------------------

bool MapAndUnits::cancel_build_order(ProvinceId province)
{
   Debug::ft("MapAndUnits.cancel_build_order");

   Location first_coast(province, 0);
   auto match = our_winter_orders.adjustments.lower_bound(first_coast);

   if((match != our_winter_orders.adjustments.end()) &&
      (match->first.province == province))
   {
      our_winter_orders.adjustments.erase(match);
      return true;
   }

   return false;
}

//------------------------------------------------------------------------------

bool MapAndUnits::cancel_remove_order(ProvinceId location)
{
   Debug::ft("MapAndUnits.cancel_remove_order");

   //  This is exactly the same as removing a build order.
   //
   return cancel_build_order(location);
}

//------------------------------------------------------------------------------

void MapAndUnits::clear_all_orders()
{
   Debug::ft("MapAndUnits.clear_all_orders");

   for(auto u = units.begin(); u != units.end(); ++u)
   {
      u->second.order = NO_ORDER;
   }

   for(auto u = dislodged_units.begin(); u != dislodged_units.end(); ++u)
   {
      u->second.order = NO_ORDER;
   }

   our_winter_orders.adjustments.clear();
   our_winter_orders.number_of_waives = 0;
}

//------------------------------------------------------------------------------

MapAndUnits* MapAndUnits::create_clone()
{
   Debug::ft("MapAndUnits.create_clone");

   auto original = instance();
   auto duplicate = new MapAndUnits;

   *duplicate = *original;
   return duplicate;
}

//------------------------------------------------------------------------------

void MapAndUnits::delete_clone(MapAndUnits*& clone)
{
   Debug::ft("MapAndUnits.delete_clone");

   if(clone == instance()) return;  // wanker!
   delete clone;
   clone = nullptr;
}

//------------------------------------------------------------------------------

string MapAndUnits::display_movement_order
   (const UnitOrder& unit, const UnitOrderMap& unit_set) const
{
   std::ostringstream stream;

   switch(unit.order)
   {
   case HOLD_ORDER:
      stream << unit << " holds";
      break;

   case MOVE_ORDER:
      stream << unit << " - " << unit.dest;
      break;

   case SUPPORT_TO_HOLD_ORDER:
      stream << unit << " s " << unit_set.at(unit.client_loc);
      break;

   case SUPPORT_TO_MOVE_ORDER:
      stream << unit << " s " << unit_set.at(unit.client_loc) <<
         " - " << display_province(unit.client_dest);
      break;

   case CONVOY_ORDER:
      stream << unit << " c " << unit_set.at(unit.client_loc) <<
         " - " << display_province(unit.client_dest);
      break;

   case MOVE_BY_CONVOY_ORDER:
      stream << unit;

      for(auto f = unit.convoyers.begin(); f != unit.convoyers.end(); ++f)
      {
         stream << " - " << display_province(*f);
      }

      stream << " - " << unit.dest;
      break;
   }

   return stream.str();
}

//------------------------------------------------------------------------------

string MapAndUnits::display_movement_result(const UnitOrder& unit) const
{
   std::ostringstream stream;

   stream << display_movement_order(unit, prev_movements);

   if(unit.bounce)
      stream << " [bounce]";
   else if(unit.support_cut)
      stream << " [cut]";
   else if(unit.no_convoy || unit.no_army_to_convoy || unit.support_void)
      stream << " [void]";
   else if(unit.convoy_disrupted)
      stream << " [disrupted]";
   else if(unit.illegal_order)
      stream << " [illegal]";

   if(unit.dislodged)
   {
      stream << " [dislodged]";
   }

   return stream.str();
}

//------------------------------------------------------------------------------

string MapAndUnits::display_province(ProvinceId province) const
{
   //  Display land provinces in mixed case and sea provinces in upper case.
   //  Remove any trailing blank(s) from the name.
   //
   auto province_str = province_token(province).to_str();

   if(game_map[province].is_land)
   {
      for(size_t i = 1; i < province_str.size(); ++i)
      {
         if(province_str[i] != SPACE)
         {
            province_str[i] = char(tolower(province_str[i]));
         }
      }
   }

   while(province_str.back() == SPACE)
   {
      province_str.pop_back();
   }

   return province_str;
}

//------------------------------------------------------------------------------

string MapAndUnits::display_retreat_order(const UnitOrder& unit) const
{
   std::ostringstream stream;

   switch(unit.order)
   {
   case DISBAND_ORDER:
      stream << unit << " disbands";
      break;

   case RETREAT_ORDER:
      stream << unit << " - " << unit.dest;
      break;
   }

   return stream.str();
}

//------------------------------------------------------------------------------

string MapAndUnits::display_retreat_result(const UnitOrder& unit) const
{
   std::ostringstream stream;

   stream << display_retreat_order(unit);

   if(unit.bounce)
   {
      stream << " [bounce] [disbanded]";
   }

   return stream.str();
}

//------------------------------------------------------------------------------

TokenMessage MapAndUnits::encode_build_result
   (PowerId power, const WinterOrders& orders, const Location& location) const
{
   Debug::ft("MapAndUnits.encode_build_result");

   TokenMessage order = power_token(power);

   if(location.coast == TOKEN_UNIT_AMY)
      order = order + Token(TOKEN_UNIT_AMY);
   else
      order = order + Token(TOKEN_UNIT_FLT);

   order = order + encode_location(location);
   order.enclose_this();

   if(orders.is_building)
      order = order + Token(TOKEN_ORDER_BLD);
   else
      order = order + Token(TOKEN_ORDER_REM);

   auto msg = Token(TOKEN_COMMAND_ORD) + encode_turn() &
      order & Token(TOKEN_RESULT_SUC);
   return msg;
}

//------------------------------------------------------------------------------

TokenMessage MapAndUnits::encode_dislodged_unit(const UnitOrder& unit) const
{
   Debug::ft("MapAndUnits.encode_dislodged_unit");

   TokenMessage retreat_locations;

   TokenMessage msg(power_token(unit.owner));
   msg = msg + unit.unit_type + encode_location(unit.loc) +
      Token(TOKEN_PARAMETER_MRT);

   for(auto r = unit.open_retreats.begin(); r != unit.open_retreats.end(); ++r)
   {
      retreat_locations = retreat_locations + encode_location(*r);
   }

   msg = msg & retreat_locations;
   return msg.enclose();
}

//------------------------------------------------------------------------------

TokenMessage MapAndUnits::encode_location(const Location& location) const
{
   TokenMessage msg(game_map[location.province].token);

   if(location.coast.category() == CATEGORY_COAST)
   {
      msg = msg + location.coast;
      msg.enclose_this();
   }

   return msg;
}

//------------------------------------------------------------------------------

fn_name MapAndUnits_encode_movement_order = "MapAndUnits.encode_movement_order";

TokenMessage MapAndUnits::encode_movement_order(const UnitOrder& unit) const
{
   Debug::ft(MapAndUnits_encode_movement_order);

   auto order = encode_unit(unit);
   TokenMessage convoy_via;

   switch(unit.order)
   {
   case NO_ORDER:
   case HOLD_ORDER:
      order = order + Token(TOKEN_ORDER_HLD);
      break;

   case MOVE_ORDER:
      order = order + Token(TOKEN_ORDER_MTO) + encode_location(unit.dest);
      break;

   case SUPPORT_TO_HOLD_ORDER:
      order = order + Token(TOKEN_ORDER_SUP) +
         encode_unit(units.at(unit.client_loc));
      break;

   case SUPPORT_TO_MOVE_ORDER:
      order = order + Token(TOKEN_ORDER_SUP) +
         encode_unit(units.at(unit.client_loc)) +
         Token(TOKEN_ORDER_MTO) + game_map[unit.client_dest].token;
      break;

   case CONVOY_ORDER:
      order = order + Token(TOKEN_ORDER_CVY) +
         encode_unit(units.at(unit.client_loc)) +
         Token(TOKEN_ORDER_CTO) + game_map[unit.client_dest].token;
      break;

   case MOVE_BY_CONVOY_ORDER:
      order = order + Token(TOKEN_ORDER_CTO) + encode_location(unit.dest);

      for(auto f = unit.convoyers.begin(); f != unit.convoyers.end(); ++f)
      {
         convoy_via = convoy_via + game_map[*f].token;
      }

      order = order + Token(TOKEN_ORDER_VIA) & convoy_via;
      break;

   default:
      Debug::SwLog(MapAndUnits_encode_movement_order,
         "invalid order", unit.order);
      order.clear();
   }

   return order;
}

//------------------------------------------------------------------------------

fn_name MapAndUnits_encode_movement_result =
   "MapAndUnits.encode_movement_result";

TokenMessage MapAndUnits::encode_movement_result(const UnitOrder& unit) const
{
   Debug::ft(MapAndUnits_encode_movement_result);

   auto order = encode_unit(unit);
   TokenMessage result;
   TokenMessage convoy_via;

   switch(unit.order)
   {
   case NO_ORDER:
   case HOLD_ORDER:
      order = order + Token(TOKEN_ORDER_HLD);

      if(!unit.dislodged)
      {
         result = TOKEN_RESULT_SUC;
      }
      break;

   case MOVE_ORDER:
      order = order + Token(TOKEN_ORDER_MTO) + encode_location(unit.dest);

      if(unit.bounce)
         result = TOKEN_RESULT_BNC;
      else if(unit.illegal_order)
         result = unit.illegal_reason;
      else
         result = TOKEN_RESULT_SUC;
      break;

   case SUPPORT_TO_HOLD_ORDER:
      order = order + Token(TOKEN_ORDER_SUP) +
         encode_unit(units.at(unit.client_loc));

      if(unit.support_cut)
         result = TOKEN_RESULT_CUT;
      else if(unit.support_void)
         result = TOKEN_RESULT_NSO;
      else if(unit.illegal_order)
         result = unit.illegal_reason;
      else
         result = TOKEN_RESULT_SUC;
      break;

   case SUPPORT_TO_MOVE_ORDER:
      order = order + Token(TOKEN_ORDER_SUP) +
         encode_unit(units.at(unit.client_loc)) +
         Token(TOKEN_ORDER_MTO) + game_map[unit.client_dest].token;

      if(unit.support_cut)
         result = TOKEN_RESULT_CUT;
      else if(unit.support_void)
         result = TOKEN_RESULT_NSO;
      else if(unit.illegal_order)
         result = unit.illegal_reason;
      else
         result = TOKEN_RESULT_SUC;
      break;

   case CONVOY_ORDER:
   {
      order = order + Token(TOKEN_ORDER_CVY) +
         encode_unit(units.at(unit.client_loc)) +
         Token(TOKEN_ORDER_CTO) + game_map[unit.client_dest].token;

      if(unit.no_army_to_convoy)
         result = TOKEN_RESULT_NSO;
      else if(unit.illegal_order)
         result = unit.illegal_reason;
      else if(!unit.dislodged)
         result = TOKEN_RESULT_SUC;
      break;
   }

   case MOVE_BY_CONVOY_ORDER:
      order = order + Token(TOKEN_ORDER_CTO) + encode_location(unit.dest);

      for(auto f = unit.convoyers.begin(); f != unit.convoyers.end(); ++f)
      {
         convoy_via = convoy_via + game_map[*f].token;
      }

      order = order + Token(TOKEN_ORDER_VIA) & convoy_via;

      if(unit.no_convoy)
         result = TOKEN_RESULT_NSO;
      else if(unit.convoy_disrupted)
         result = TOKEN_RESULT_DSR;
      else if(unit.bounce)
         result = TOKEN_RESULT_BNC;
      else if(unit.illegal_order)
         result = unit.illegal_reason;
      else
         result = TOKEN_RESULT_SUC;
      break;

   default:
      Debug::SwLog(MapAndUnits_encode_movement_result,
         "invalid order", unit.order);
      return result;  // empty
   }

   if(unit.dislodged)
   {
      result = result + Token(TOKEN_RESULT_RET);
   }

   auto msg = TokenMessage(TOKEN_COMMAND_ORD) + encode_turn() & order & result;
   return msg;
}

//------------------------------------------------------------------------------

fn_name MapAndUnits_encode_retreat_order = "MapAndUnits.encode_retreat_order";

TokenMessage MapAndUnits::encode_retreat_order(const UnitOrder& unit) const
{
   Debug::ft(MapAndUnits_encode_retreat_order);

   auto order = encode_unit(unit);

   switch(unit.order)
   {
   case NO_ORDER:
   case DISBAND_ORDER:
      order = order + Token(TOKEN_ORDER_DSB);
      break;

   case RETREAT_ORDER:
      order = order + Token(TOKEN_ORDER_RTO) + encode_location(unit.dest);
      break;

   default:
      Debug::SwLog(MapAndUnits_encode_retreat_order,
         "invalid order", unit.order);
      order.clear();
   }

   return order;
}

//------------------------------------------------------------------------------

fn_name MapAndUnits_encode_retreat_result = "MapAndUnits.encode_retreat_result";

TokenMessage MapAndUnits::encode_retreat_result(const UnitOrder& unit) const
{
   Debug::ft(MapAndUnits_encode_retreat_result);

   auto order = encode_unit(unit);
   TokenMessage result;

   switch(unit.order)
   {
   case NO_ORDER:
   case DISBAND_ORDER:
      order = order + Token(TOKEN_ORDER_DSB);
      result = TOKEN_RESULT_SUC;
      break;

   case RETREAT_ORDER:
      order = order + Token(TOKEN_ORDER_RTO) + encode_location(unit.dest);

      if(unit.bounce)
         result = TOKEN_RESULT_BNC;
      else if(unit.illegal_order)
         result = unit.illegal_reason;
      else
         result = TOKEN_RESULT_SUC;
      break;

   default:
      Debug::SwLog(MapAndUnits_encode_retreat_result,
         "invalid order", unit.order);
      return result;  // empty
   }

   auto msg = Token(TOKEN_COMMAND_ORD) + encode_turn() & order & result;
   return msg;
}

//------------------------------------------------------------------------------

TokenMessage MapAndUnits::encode_turn() const
{
   Debug::ft("MapAndUnits.encode_turn");

   Token year;

   year.set_number(curr_year);

   auto turn = curr_season + year;
   turn.enclose_this();
   return turn;
}

//------------------------------------------------------------------------------

TokenMessage MapAndUnits::encode_unit(const UnitOrder& unit) const
{
   TokenMessage unit_message(power_token(unit.owner));

   unit_message = unit_message + unit.unit_type + encode_location(unit.loc);
   unit_message.enclose_this();
   return unit_message;
}

//------------------------------------------------------------------------------

TokenMessage MapAndUnits::encode_waive(PowerId power) const
{
   Debug::ft("MapAndUnits.encode_waive");

   auto order = power_token(power) + Token(TOKEN_ORDER_WVE);
   auto msg = Token(TOKEN_COMMAND_ORD) +
      encode_turn() & order & Token(TOKEN_RESULT_SUC);
   return msg;
}

//------------------------------------------------------------------------------

Location MapAndUnits::find_adjustment(ProvinceId province) const
{
   Debug::ft("MapAndUnits.find_adjustment");

   Location first_coast(province, 0);
   auto match = our_winter_orders.adjustments.lower_bound(first_coast);

   if((match != our_winter_orders.adjustments.end()) &&
      (match->first.province == province))
   {
      return match->first;
   }

   return Location();
}

//------------------------------------------------------------------------------

Location MapAndUnits::find_result_unit_initial_location
   (ProvinceId province, bool& is_new_build, bool& retreated_to_province,
   bool& moved_to_province, bool& unit_found) const
{
   Debug::ft("MapAndUnits.find_result_unit_initial_location");

   is_new_build = false;
   retreated_to_province = false;
   moved_to_province = false;
   unit_found = false;

   for(auto r = prev_adjustments.begin(); r != prev_adjustments.end(); ++r)
   {
      if(!r->second.is_building) continue;

      for(auto b = r->second.adjustments.begin();
         b != r->second.adjustments.end(); ++b)
      {
         if(b->first.province == province)
         {
            unit_found = true;
            is_new_build = true;
            return b->first;
         }
      }
   }

   for(auto r = prev_retreats.begin(); r != prev_retreats.end(); ++r)
   {
      if((r->second.dest.province == province) && (r->second.unit_moves))
      {
         unit_found = true;
         retreated_to_province = true;
         return r->second.loc;
      }
   }

   for(auto r = prev_movements.begin(); r != prev_movements.end(); ++r)
   {
      if((r->second.dest.province == province) && (r->second.unit_moves))
      {
         unit_found = true;
         moved_to_province = true;
         return r->second.loc;
      }

      if((r->second.loc.province == province) &&
         !r->second.unit_moves && !r->second.dislodged)
      {
         unit_found = true;
         return r->second.loc;
      }
   }

   return Location();
}

//------------------------------------------------------------------------------

UnitOrder* MapAndUnits::find_unit
   (const TokenMessage& unit_to_find, UnitOrderMap& units_map) const
{
   Debug::ft("MapAndUnits.find_unit");

   if(unit_to_find.parm_count() != 3)
   {
      return nullptr;
   }

   auto owner = unit_to_find.get_parm(0);
   auto unit_type = unit_to_find.get_parm(1);

   if(!owner.is_single_token() || !unit_type.is_single_token())
   {
      return nullptr;
   }

   auto location = unit_to_find.get_parm(2);
   auto province_token = location.front();
   Token coast;

   if(location.is_single_token())
      coast = unit_type.front();
   else
      coast = location.at(1);

   auto unit = units_map.find(province_token.province_id());

   if(unit == units_map.end())
   {
      return nullptr;
   }

   if((province_token.province_id() >= number_of_provinces) ||
      (unit->second.loc.coast != coast) ||
      (unit->second.owner != owner.front().power_id()) ||
      (unit->second.unit_type != unit_type.front()))
   {
      return nullptr;
   }

   return &unit->second;
}

//------------------------------------------------------------------------------

size_t MapAndUnits::get_adjudication_results(TokenMessage ord_messages[]) const
{
   Debug::ft("MapAndUnits.get_adjudication_results");

   switch(curr_season.all())
   {
   case TOKEN_SEASON_SPR:
   case TOKEN_SEASON_FAL:
      return get_movement_results(ord_messages);

   case TOKEN_SEASON_SUM:
   case TOKEN_SEASON_AUT:
      return get_retreat_results(ord_messages);

   case TOKEN_SEASON_WIN:
      return get_adjustment_results(ord_messages);
   }

   return 0;
}

//------------------------------------------------------------------------------

size_t MapAndUnits::get_adjustment_results(TokenMessage ord_messages[]) const
{
   Debug::ft("MapAndUnits.get_adjustment_results");

   size_t count = 0;

   for(PowerId p = 0; p < number_of_powers; ++p)
   {
      auto& orders = winter_orders.at(p);

      for(auto o = orders.adjustments.begin();
         o != orders.adjustments.end(); ++o, ++count)
      {
         ord_messages[count] = encode_build_result(p, orders, o->first);
      }

      if(orders.is_building)
      {
         for(size_t w = 0; w < orders.number_of_waives; ++w, ++count)
         {
            ord_messages[count] = encode_waive(p);
         }
      }
   }

   return count;
}

//------------------------------------------------------------------------------

size_t MapAndUnits::get_centre_count(Token power) const
{
   Debug::ft("MapAndUnits.get_centre_count");

   size_t count = 0;

   for(ProvinceId p = 0; p < number_of_provinces; ++p)
   {
      if(game_map[p].is_supply_centre && (game_map[p].owner == power))
      {
         ++count;
      }
   }

   return count;
}

//------------------------------------------------------------------------------

std::vector< PowerCentres > MapAndUnits::get_centres() const
{
   Debug::ft("MapAndUnits.get_centres");

   std::vector< PowerCentres > owners;

   //  Create an entry for each power, and also UNO (for unowned centres).
   //
   for(PowerId p = 0; p < number_of_powers; ++p)
   {
      PowerCentres centres(power_token(p));
      owners.push_back(centres);
   }

   PowerCentres centres(TOKEN_PARAMETER_UNO);
   owners.push_back(centres);

   //  Record the owner of each supply centre.
   //
   for(ProvinceId p = 0; p < number_of_provinces; ++p)
   {
      auto& province = game_map[p];

      if(province.is_supply_centre)
      {
         auto owner = province.owner;
         auto index = (owner == TOKEN_PARAMETER_UNO ?
            number_of_powers : owner.power_id());
         owners.at(index).centres.push_back(p);
      }
   }

   return owners;
}

//------------------------------------------------------------------------------

const LocationSet* MapAndUnits::get_destinations(ProvinceId province) const
{
   Debug::ft("MapAndUnits.get_destinations");

   //  If PROVINCE contains a unit, return its possible destinations.
   //
   auto unit = units.find(province);

   if(unit == units.end())
   {
      return nullptr;
   }

   return get_neighbours(unit->second.loc);
}

//------------------------------------------------------------------------------

const LocationSet* MapAndUnits::get_dislodged_unit_destinations
   (ProvinceId province) const
{
   Debug::ft("MapAndUnits.get_dislodged_unit_destinations");

   auto unit = dislodged_units.find(province);

   if(unit == dislodged_units.end())
   {
      return nullptr;
   }

   return get_neighbours(unit->second.loc);
}

//------------------------------------------------------------------------------

size_t MapAndUnits::get_movement_results(TokenMessage ord_messages[]) const
{
   Debug::ft("MapAndUnits.get_movement_results");

   size_t count = 0;

   for(auto u = units.begin(); u != units.end(); ++u, ++count)
   {
      ord_messages[count] = encode_movement_result(u->second);
   }

   return count;
}

//------------------------------------------------------------------------------

const LocationSet* MapAndUnits::get_neighbours(const Location& location) const
{
   return &game_map[location.province].neighbours.at(location.coast);
}

//------------------------------------------------------------------------------

fn_name MapAndUnits_get_orders = "MapAndUnits.get_orders";

std::vector< PowerOrders > MapAndUnits::get_orders(const Token& season) const
{
   Debug::ft(MapAndUnits_get_orders);

   std::vector< PowerOrders > powers;

   for(PowerId p = 0; p < number_of_powers; ++p)
   {
      PowerOrders orders(power_token(p));
      powers.push_back(orders);
   }

   switch(season.all())
   {
   case TOKEN_SEASON_SPR:
   case TOKEN_SEASON_FAL:
   {
      for(auto m = prev_movements.cbegin(); m != prev_movements.cend(); ++m)
      {
         auto p = m->second.owner;
         powers.at(p).orders.push_back(m->second);
      }
      break;
   }

   case TOKEN_SEASON_SUM:
   case TOKEN_SEASON_AUT:
   {
      for(auto r = prev_retreats.cbegin(); r != prev_retreats.cend(); ++r)
      {
         auto p = r->second.owner;
         powers.at(p).orders.push_back(r->second);
      }
      break;
   }

   default:
      string expl = "invalid season" + season.to_str();
      Debug::SwLog(MapAndUnits_get_orders, expl, 0);
   }

   return powers;
}

//------------------------------------------------------------------------------

size_t MapAndUnits::get_retreat_results(TokenMessage ord_messages[]) const
{
   Debug::ft("MapAndUnits.get_retreat_results");

   size_t n = 0;

   for(auto u = dislodged_units.begin(); u != dislodged_units.end(); ++u, ++n)
   {
      ord_messages[n] = encode_retreat_result(u->second);
   }

   return n;
}

//------------------------------------------------------------------------------

size_t MapAndUnits::get_unit_count(Token power) const
{
   Debug::ft("MapAndUnits.get_unit_count");

   size_t count = 0;

   for(auto u = units.begin(); u != units.end(); ++u)
   {
      if(u->second.owner == power.power_id())
      {
         ++count;
      }
   }

   return count;
}

//------------------------------------------------------------------------------

std::vector< PowerUnits > MapAndUnits::get_units() const
{
   Debug::ft("MapAndUnits.get_units");

   std::vector< PowerUnits > owners;

   //  Create an entry for each power.
   //
   for(PowerId p = 0; p < number_of_powers; ++p)
   {
      PowerUnits pieces(power_token(p));
      owners.push_back(pieces);
   }

   //  Record the owner of each unit.
   //
   for(ProvinceId p = 0; p < number_of_provinces; ++p)
   {
      auto unit = units.find(p);

      if(unit != units.cend())
      {
         UnitLocation unitloc(unit->second.unit_type, unit->second.loc);
         owners.at(unit->second.owner).units.push_back(unitloc);
      }
   }

   return owners;
}

//------------------------------------------------------------------------------

bool MapAndUnits::get_variant_setting(const Token& option, Token* setting) const
{
   Debug::ft("MapAndUnits.get_variant_setting");

   for(size_t count = 0; count < variant.parm_count(); ++count)
   {
      auto var = variant.get_parm(count);

      if(var.front() == option)
      {
         if((var.size() > 1) && (setting != nullptr))
         {
            *setting = var.at(1);
         }

         return true;
      }
   }

   return false;
}

//------------------------------------------------------------------------------

bool MapAndUnits::has_route_to_province
   (const UnitOrder& unit, ProvinceId province, ProvinceId exclude) const
{
   Debug::ft("MapAndUnits.has_route_to_province");

   //  Check for a direct move.
   //
   if(can_move_to_province(unit, province))
   {
      return true;
   }

   //  Check for a convoy route.
   //
   if(!game_map[province].is_land || (unit.unit_type != TOKEN_UNIT_AMY))
   {
      return false;
   }

   ProvinceSet discards;    // provinces that have been checked
   ProvinceSet candidates;  // provinces remaining to be checked

   discards.insert(unit.loc.province);

   //  All adjacent provinces are candidates.
   //
   auto& neighbours1 = game_map[unit.loc.province].neighbours;

   for(auto n = neighbours1.begin(); n != neighbours1.end(); ++n)
   {
      for(auto loc = n->second.begin(); loc != n->second.end(); ++loc)
      {
         candidates.insert(loc->province);
      }
   }

   //  If there is a province to avoid, discard it.  This prevents any
   //  route from going through it.  This is used to stop a fleet from
   //  supporting a convoyed move that must be convoyed by that fleet.
   //
   if(exclude != NIL_PROVINCE)
   {
      discards.insert(exclude);
   }

   //  Keep going until all provinces have been checked.
   //
   while(!candidates.empty())
   {
      auto candidate = *candidates.begin();
      candidates.erase(candidates.begin());

      if(discards.find(candidate) == discards.end())
      {
         discards.insert(candidate);

         //  See if the convoy has reached its destination, which must be
         //  a land province.
         //
         if(game_map[candidate].is_land)
         {
            if(candidate == province)
            {
               return true;
            }
         }
         else
         {
            //  If this sea province is occupied, check all of its adjacent
            //  provinces.
            //
            if(units.find(candidate) != units.end())
            {
               auto& neighbours2 = game_map[candidate].neighbours;

               for(auto n = neighbours2.begin(); n != neighbours2.end(); ++n)
               {
                  for(auto loc = n->second.begin();
                     loc != n->second.end(); ++loc)
                  {
                     candidates.insert(loc->province);
                  }
               }
            }
         }
      }
   }

   return false;
}

//------------------------------------------------------------------------------

MapAndUnits* MapAndUnits::instance()
{
   static MapAndUnits instance_;

   return &instance_;
}

//------------------------------------------------------------------------------

size_t MapAndUnits::process_adjacencies(const TokenMessage& adjacencies)
{
   Debug::ft("MapAndUnits.process_adjacencies");

   for(size_t count = 0; count < adjacencies.parm_count(); ++count)
   {
      auto adjacency = adjacencies.get_parm(count);
      auto error = process_province_adjacency(adjacency);

      if(error != NO_ERROR)
      {
         return error + adjacencies.parm_start(count);
      }
   }

   return NO_ERROR;
}

//------------------------------------------------------------------------------

size_t MapAndUnits::process_hlo(const TokenMessage& hlo)
{
   Debug::ft("MapAndUnits.process_hlo");

   set_our_power(hlo.get_parm(1).front());
   passcode = hlo.get_parm(2).front().get_number();
   variant = hlo.get_parm(3);
   game_started = true;

   return NO_ERROR;
}

//------------------------------------------------------------------------------

size_t MapAndUnits::process_map(const TokenMessage& map)
{
   Debug::ft("MapAndUnits.process_map");

   auto name = map.get_parm(1);
   map_name = name.to_str().c_str();

   auto begin = map_name.find(APOSTROPHE);

   if(begin != string::npos)
   {
      auto end = map_name.find_last_of(APOSTROPHE);

      if((end != string::npos) && (end > begin))
      {
         map_name = map_name.substr(begin + 1, end - begin - 1);
      }
   }

   return NO_ERROR;
}

//------------------------------------------------------------------------------

size_t MapAndUnits::process_mdf(const TokenMessage& mdf)
{
   Debug::ft("MapAndUnits.process_mdf");

   if(mdf.parm_count() != 4)
   {
      return 0;
   }

   auto signal = mdf.get_parm(0);
   auto powers = mdf.get_parm(1);
   auto provinces = mdf.get_parm(2);
   auto adjacencies = mdf.get_parm(3);

   if(!signal.is_single_token() || (signal.front() != TOKEN_COMMAND_MDF))
   {
      return 0;
   }

   auto error = process_powers(powers);

   if(error != NO_ERROR)
   {
      return error + mdf.parm_start(1);
   }

   error = process_provinces(provinces);

   if(error != NO_ERROR)
   {
      return error + mdf.parm_start(2);
   }

   error = process_adjacencies(adjacencies);

   if(error != NO_ERROR)
   {
      return error + mdf.parm_start(3);
   }

   return NO_ERROR;
}

//------------------------------------------------------------------------------

size_t MapAndUnits::process_non_supply_centres(const TokenMessage& centres)
{
   Debug::ft("MapAndUnits.process_non_supply_centres");

   for(size_t count = 0; count < centres.size(); ++count)
   {
      auto token = centres.at(count);
      auto p = token.province_id();

      if(p != NIL_PROVINCE)
      {
         auto& province = game_map[p];

         if(!province.is_valid)
         {
            province.is_valid = true;
            province.token = token;
            province.owner = TOKEN_PARAMETER_UNO;
         }
         else
         {
            return count;
         }
      }
      else if(token != TOKEN_PARAMETER_UNO)
      {
         return count;
      }
   }

   return NO_ERROR;
}

//------------------------------------------------------------------------------

fn_name MapAndUnits_process_now = "MapAndUnits.process_now";

size_t MapAndUnits::process_now(const TokenMessage& now)
{
   Debug::ft(MapAndUnits_process_now);

   if(number_of_provinces == 0)
   {
      Debug::SwLog(MapAndUnits_process_now, "map has no provinces", 0);
      return NO_ERROR;
   }

   auto signal = now.get_parm(0);

   if(!signal.is_single_token() || (signal.front() != TOKEN_COMMAND_NOW))
   {
      return 0;
   }

   auto turn = now.get_parm(1);

   curr_season = turn.at(0);
   curr_year = turn.at(1).get_number();

   units.clear();
   dislodged_units.clear();
   our_units.clear();
   our_dislodged_units.clear();
   open_home_centres.clear();
   our_winter_orders.adjustments.clear();
   our_winter_orders.number_of_waives = 0;

   //  Save our current unit positions.
   //
   for(size_t count = 2; count < now.parm_count(); ++count)
   {
      auto unit_parm = now.get_parm(count);
      auto error = process_now_unit(unit_parm);

      if(error != NO_ERROR)
      {
         return error + now.parm_start(count);
      }
   }

   //  If it's a winter turn, update centre ownership and the number
   //  of builds and disbands required.
   //
   if(curr_season == TOKEN_SEASON_WIN)
   {
      update_sc_ownership();
   }

   if(our_power != INVALID_TOKEN)
   {
      open_home_centres.clear();

      for(auto c = home_centres.begin(); c != home_centres.end(); ++c)
      {
         auto& province = game_map[*c];

         if((province.owner == our_power) && (units.find(*c) == units.end()))
         {
            open_home_centres.insert(*c);
         }
      }

      our_number_of_disbands = our_units.size() - our_centres.size();
   }

   return NO_ERROR;
}

//------------------------------------------------------------------------------

size_t MapAndUnits::process_now_unit(const TokenMessage& unit_parm)
{
   Debug::ft("MapAndUnits.process_now_unit");

   ProvinceId pid = NIL_PROVINCE;
   Token coast;
   UnitOrder unit;

   auto owner = unit_parm.at(0).power_id();

   if((owner == NIL_POWER) || (owner >= number_of_powers))
   {
      return 0;
   }

   auto unit_type = unit_parm.at(1);
   auto location = unit_parm.get_parm(2);

   if(location.is_single_token())
   {
      pid = location.front().province_id();
      coast = unit_type;
   }
   else
   {
      if(unit_type != TOKEN_UNIT_FLT)
      {
         return 2;
      }

      pid = location.at(0).province_id();
      coast = location.at(1);
   }

   if((pid == NIL_PROVINCE) || (pid >= number_of_provinces))
   {
      return 2;
   }

   auto& province = game_map[pid];

   if(province.neighbours.find(coast) == province.neighbours.end())
   {
      return 2;
   }

   unit.loc.province = pid;
   unit.owner = owner;
   unit.unit_type = unit_type;
   unit.loc.coast = coast;

   if(unit_parm.parm_count() == 5)
   {
      //  The unit was dislodged.
      //
      if(unit_parm.get_parm(3).front() != TOKEN_PARAMETER_MRT)
      {
         return unit_parm.parm_start(3);
      }
      else
      {
         auto retreat_options = unit_parm.get_parm(4);

         unit.open_retreats.clear();

         for(size_t count = 0; count < retreat_options.parm_count(); ++count)
         {
            auto loc = retreat_options.get_parm(count);
            unit.open_retreats.insert(Location(loc, unit_type));
         }
      }

      dislodged_units[pid] = unit;

      if(owner == our_power.power_id())
      {
         our_dislodged_units.insert(pid);
      }
   }
   else
   {
      //  The unit was not dislodged.
      //
      units[pid] = unit;

      if(owner == our_power.power_id())
      {
         our_units.insert(pid);
      }
   }

   return NO_ERROR;
}

//------------------------------------------------------------------------------

fn_name MapAndUnits_process_ord = "MapAndUnits.process_ord";

size_t MapAndUnits::process_ord(const TokenMessage& ord)
{
   Debug::ft(MapAndUnits_process_ord);

   if(number_of_provinces == 0)
   {
      Debug::SwLog(MapAndUnits_process_ord, "map has no provinces", 0);
      return NO_ERROR;
   }

   if(ord.parm_count() != 4)
   {
      return 0;
   }

   auto signal = ord.get_parm(0);

   if(!signal.is_single_token() || (signal.front() != TOKEN_COMMAND_ORD))
   {
      return 0;
   }

   auto turn = ord.get_parm(1);
   auto order = ord.get_parm(2);
   auto result = ord.get_parm(3);
   auto season = turn.front();

   //  If this is the first result to be processed in a Spring or Fall
   //  season, clear the previous results.
   //
   if(season != prev_movement_season)
   {
      if((season == TOKEN_SEASON_SPR) || (season == TOKEN_SEASON_FAL))
      {
         prev_movement_season = season;
         prev_movements.clear();
         prev_retreats.clear();
         prev_adjustments.clear();
      }
   }

   auto unit = order.get_parm(0);
   auto order_type = order.get_parm(1).front();
   auto power = unit.at(0).power_id();

   if(season == TOKEN_SEASON_WIN)
   {
      //  Find the adjustment orders for this power.  If they don't exist,
      //  create them.
      //
      auto orders = prev_adjustments.find(power);

      if(orders == prev_adjustments.end())
      {
         WinterOrders new_adjustment_orders;

         orders = prev_adjustments.insert
            (WinterOrderMap::value_type(power, new_adjustment_orders)).first;
      }

      switch(order_type.all())
      {
      case TOKEN_ORDER_WVE:
         orders->second.number_of_waives++;
         break;

      case TOKEN_ORDER_BLD:
         orders->second.adjustments.insert(Adjustments::value_type
            (Location(unit), TOKEN_RESULT_SUC));
         orders->second.is_building = true;
         break;

      default:
         orders->second.adjustments.insert(Adjustments::value_type
            (Location(unit), TOKEN_RESULT_SUC));
         orders->second.is_building = false;
      }
   }
   else
   {
      UnitOrder new_unit;  // new unit to add to the results

      new_unit.loc = Location(unit);
      new_unit.owner = power;
      new_unit.unit_type = unit.at(1);

      new_unit.decode_order(order);
      new_unit.decode_result(result);

      if((season == TOKEN_SEASON_SPR) || (season == TOKEN_SEASON_FAL))
      {
         prev_movements.insert
            (UnitOrderMap::value_type(new_unit.loc.province, new_unit));
      }
      else
      {
         prev_retreats.insert
            (UnitOrderMap::value_type(new_unit.loc.province, new_unit));
      }
   }

   return NO_ERROR;
}

//------------------------------------------------------------------------------

fn_name MapAndUnits_process_order = "MapAndUnits.process_order";

Token MapAndUnits::process_order(const TokenMessage& order, PowerId power)
{
   Debug::ft(MapAndUnits_process_order);

   UnitOrder* unit = nullptr;
   WinterOrders* winter = nullptr;

   auto order_token_message = order.get_parm(1);
   auto order_token = order_token_message.front();

   switch(order_token.order_season())
   {
   case Token::MOVE_SEASON:
      if((curr_season != TOKEN_SEASON_SPR) && (curr_season != TOKEN_SEASON_FAL))
      {
         return TOKEN_ORDER_NOTE_NRS;
      }

      unit = find_unit(order.get_parm(0), units);

      if(unit == nullptr)
         return TOKEN_ORDER_NOTE_NSU;
      else if(unit->owner != power)
         return TOKEN_ORDER_NOTE_NYU;
      break;

   case Token::RETREAT_SEASON:
      if((curr_season != TOKEN_SEASON_SUM) && (curr_season != TOKEN_SEASON_AUT))
      {
         return TOKEN_ORDER_NOTE_NRS;
      }

      unit = find_unit(order.get_parm(0), dislodged_units);

      if(unit == nullptr)
         return TOKEN_ORDER_NOTE_NRN;
      else if(unit->owner != power)
         return TOKEN_ORDER_NOTE_NYU;
      break;

   case Token::BUILD_SEASON:
      if(curr_season != TOKEN_SEASON_WIN)
         return TOKEN_ORDER_NOTE_NRS;
      else
         winter = &winter_orders[power];
      break;

   default:
      Debug::SwLog(MapAndUnits_process_order,
         "invalid season type", order_token.order_season());
      return TOKEN_ORDER_NOTE_NRS;
   }

   switch(order_token.all())
   {
   case TOKEN_ORDER_HLD:
      if(order_token == TOKEN_ORDER_HLD)
      {
         unit->order = HOLD_ORDER;
      }
      break;

   case TOKEN_ORDER_MTO:
   {
      Location dest(order.get_parm(2), unit->unit_type);

      if(check_on_submission && !can_move_to(*unit, dest))
      {
         return TOKEN_ORDER_NOTE_FAR;
      }

      unit->order = MOVE_ORDER;
      unit->dest = dest;
      break;
   }

   case TOKEN_ORDER_SUP:
   {
      auto client = find_unit(order.get_parm(2), units);

      if(order.parm_count() == 3)
      {
         //  Support to hold.
         //
         if(client == nullptr)
         {
            return TOKEN_ORDER_NOTE_NSU;
         }
         else if(check_on_submission)
         {
            if(!can_move_to_province(*unit, client->loc.province))
               return TOKEN_ORDER_NOTE_FAR;
            else if(client->loc.province == unit->loc.province)  // <a>
               return TOKEN_ORDER_NOTE_FAR;
         }

         unit->order = SUPPORT_TO_HOLD_ORDER;
         unit->client_loc = client->loc.province;
      }
      else
      {
         //  Support to move.
         //
         client = find_unit(order.get_parm(2), units);
         auto client_dest = order.get_parm(4).front();

         if(client == nullptr)
         {
            return TOKEN_ORDER_NOTE_NSU;
         }
         else if(check_on_submission)
         {
            if(!has_route_to_province
               (*client, client_dest.province_id(), unit->loc.province))
               return TOKEN_ORDER_NOTE_FAR;
            else if(!can_move_to_province(*unit, client_dest.province_id()))
               return TOKEN_ORDER_NOTE_FAR;
            else if(client->loc.province == unit->loc.province)
               return TOKEN_ORDER_NOTE_FAR;
         }

         unit->order = SUPPORT_TO_MOVE_ORDER;
         unit->client_loc = client->loc.province;
         unit->client_dest = client_dest.province_id();
      }
      break;
   }

   case TOKEN_ORDER_CVY:
   {
      auto client = find_unit(order.get_parm(2), units);
      auto client_dest = order.get_parm(4).front();

      if(client == nullptr)
      {
         return TOKEN_ORDER_NOTE_NSU;
      }
      else if(check_on_submission)
      {
         if(unit->unit_type != TOKEN_UNIT_FLT)
            return TOKEN_ORDER_NOTE_NSF;
         else if(game_map[unit->loc.province].is_land)
            return TOKEN_ORDER_NOTE_NAS;
         else if(client->unit_type != TOKEN_UNIT_AMY)
            return TOKEN_ORDER_NOTE_NSA;
         else if(!has_route_to_province
               (*client, client_dest.province_id(), NIL_PROVINCE))
            return TOKEN_ORDER_NOTE_FAR;
      }

      unit->order = CONVOY_ORDER;
      unit->client_loc = client->loc.province;
      unit->client_dest = client_dest.province_id();
      break;
   }

   case TOKEN_ORDER_CTO:
   {
      if(check_on_submission && (unit->unit_type != TOKEN_UNIT_AMY))
      {
         return TOKEN_ORDER_NOTE_NSA;
      }

      auto dest = order.get_parm(2).front();
      auto vias = order.get_parm(4);
      auto previous_province = unit->loc.province;
      UnitOrder* convoy_order = nullptr;

      if(check_on_submission)
      {
         for(size_t v = 0; v < vias.size(); ++v)
         {
            auto fleet = units.find(vias.get_parm(v).front().province_id());

            if(fleet == units.end())
               convoy_order = nullptr;
            else
               convoy_order = &fleet->second;

            if(convoy_order == nullptr)
               return TOKEN_ORDER_NOTE_NSF;
            else if(game_map[convoy_order->loc.province].is_land)
               return TOKEN_ORDER_NOTE_NAS;
            else if(!can_move_to_province(*convoy_order, previous_province))
               return TOKEN_ORDER_NOTE_FAR;

            previous_province = convoy_order->loc.province;
         }

         if(dest.province_id() == unit->loc.province)
         {
            return TOKEN_ORDER_NOTE_FAR;
         }
      }

      if(check_on_submission)
      {
         if(!can_move_to_province(*convoy_order, dest.province_id()))
         {
            return TOKEN_ORDER_NOTE_FAR;
         }
      }

      unit->order = MOVE_BY_CONVOY_ORDER;
      unit->dest.province = dest.province_id();
      unit->dest.coast = TOKEN_UNIT_AMY;
      unit->convoyers.clear();

      for(size_t v = 0; v < vias.size(); ++v)
      {
         auto convoying_province = vias.at(v).province_id();
         unit->convoyers.push_back(convoying_province);
      }
      break;
   }

   case TOKEN_ORDER_RTO:
   {
      Location dest(order.get_parm(2), unit->unit_type);

      if(check_on_submission)
      {
         if(!can_move_to(*unit, dest))
            return TOKEN_ORDER_NOTE_FAR;
         else if(unit->open_retreats.find(dest) == unit->open_retreats.end())
            return TOKEN_ORDER_NOTE_NVR;
      }

      unit->order = RETREAT_ORDER;
      unit->dest = dest;
      break;
   }

   case TOKEN_ORDER_DSB:
      unit->order = DISBAND_ORDER;
      break;

   case TOKEN_ORDER_BLD:
   {
      if(!winter->is_building)
         return TOKEN_ORDER_NOTE_NMB;
      else if(winter->adjustments.size() +
            winter->number_of_waives >= winter->number_of_orders_required)
         return TOKEN_ORDER_NOTE_NMB;

      auto winter_order = order.get_parm(0);
      Location build_loc;

      if(winter_order.parm_is_single_token(2))
      {
         build_loc.province = winter_order.at(2).province_id();
         build_loc.coast = winter_order.at(1);
      }
      else
      {
         build_loc.province = winter_order.get_parm(2).at(0).province_id();
         build_loc.coast = winter_order.get_parm(2).at(1);
      }

      if(winter_order.at(0).power_id() != power)
         return TOKEN_ORDER_NOTE_NYU;
      else if(!game_map[build_loc.province].is_supply_centre)
         return TOKEN_ORDER_NOTE_NSC;
      else if(game_map[build_loc.province].home_powers.find(power) ==
            game_map[build_loc.province].home_powers.end())
         return TOKEN_ORDER_NOTE_HSC;
      else if(game_map[build_loc.province].owner.power_id() != power)
         return TOKEN_ORDER_NOTE_YSC;
      else if(units.find(build_loc.province) != units.end())
         return TOKEN_ORDER_NOTE_ESC;
      else if(game_map[build_loc.province].neighbours.find(build_loc.coast) ==
            game_map[build_loc.province].neighbours.end())
         return TOKEN_ORDER_NOTE_CST;

      Location first_coast(build_loc.province, 0);
      auto match = winter->adjustments.lower_bound(first_coast);

      if(check_on_submission &&
         (match != winter->adjustments.end()) &&
         (match->first.province == first_coast.province))
      {
         return TOKEN_ORDER_NOTE_ESC;
      }

      winter->adjustments.insert
         (Adjustments::value_type(build_loc, TOKEN_ORDER_NOTE_MBV));
      break;
   }

   case TOKEN_ORDER_REM:
      if(winter->is_building)
         return TOKEN_ORDER_NOTE_NMR;
      else if(winter->adjustments.size() >= winter->number_of_orders_required)
         return TOKEN_ORDER_NOTE_NMR;

      unit = find_unit(order.get_parm(0), units);

      if(unit == nullptr)
         return TOKEN_ORDER_NOTE_NSU;
      else if(unit->owner != power)
         return TOKEN_ORDER_NOTE_NYU;

      winter->adjustments.insert
         (Adjustments::value_type(unit->loc, TOKEN_ORDER_NOTE_MBV));
      break;

   case TOKEN_ORDER_WVE:
      if(!winter->is_building)
         return TOKEN_ORDER_NOTE_NMB;
      else if(winter->adjustments.size() +
            winter->number_of_waives >= winter->number_of_orders_required)
         return TOKEN_ORDER_NOTE_NMB;
      else if(order.at(0).power_id() != power)
         return TOKEN_ORDER_NOTE_NYU;

      winter->number_of_waives++;
      break;

   default:
      Debug::SwLog(MapAndUnits_process_order,
         "invalid order type", order_token.all());
      return TOKEN_ORDER_NOTE_NRS;
   }

   return TOKEN_ORDER_NOTE_MBV;
}

//------------------------------------------------------------------------------

size_t MapAndUnits::process_powers(const TokenMessage& powers)
{
   Debug::ft("MapAndUnits.process_powers");

   bool power_used[POWER_MAX];
   auto error = NO_ERROR;

   number_of_powers = powers.size();

   for(PowerId p = 0; p < number_of_powers; ++p)
   {
      power_used[p] = false;
   }

   for(PowerId p = 0; p < number_of_powers; ++p)
   {
      auto power = powers.at(p).power_id();

      if((power == NIL_POWER) ||
         (power >= number_of_powers) ||
         (power_used[power]))
      {
         return p;
      }

      power_used[power] = true;
   }

   return error;
}

//------------------------------------------------------------------------------

size_t MapAndUnits::process_province_adjacency(const TokenMessage& adjacency)
{
   Debug::ft("MapAndUnits.process_province_adjacency");

   auto p = adjacency.at(0).province_id();

   if(p == NIL_PROVINCE)
   {
      return 0;
   }

   auto& province = game_map[p];

   if(!province.is_valid || !province.neighbours.empty())
   {
      return 0;
   }

   for(size_t count = 1; count < adjacency.parm_count(); ++count)
   {
      auto adjacency_list = adjacency.get_parm(count);
      auto error = province.process_adjacency_list(adjacency_list);

      if(error != NO_ERROR)
      {
         return error + adjacency.parm_start(count);
      }
   }

   return NO_ERROR;
}

//------------------------------------------------------------------------------

size_t MapAndUnits::process_provinces(const TokenMessage& provinces)
{
   Debug::ft("MapAndUnits.process_provinces");

   for(size_t p = 0; p < PROVINCE_MAX; ++p)
   {
      game_map[p] = Province();  // reset to nil values
   }

   auto supply_centres = provinces.get_parm(0);
   auto non_supply_centres = provinces.get_parm(1);
   auto error = process_supply_centres(supply_centres);

   if(error != NO_ERROR)
   {
      return error + provinces.parm_start(0);
   }

   error = process_non_supply_centres(non_supply_centres);

   if(error != NO_ERROR)
   {
      return error + provinces.parm_start(1);
   }

   number_of_provinces = 0;

   //  Verify that all valid provinces have identifiers in the range
   //  0...N, where N + 1 is the number of provinces.
   //
   for(size_t p = 0; p < PROVINCE_MAX; ++p)
   {
      if(!game_map[p].is_valid)
      {
         if(number_of_provinces == 0)
         {
            number_of_provinces = p;
         }
      }
      else
      {
         if(number_of_provinces != 0)
         {
            return provinces.parm_start(1) - 1;
         }
      }
   }

   return NO_ERROR;
}

//------------------------------------------------------------------------------

fn_name MapAndUnits_process_sco = "MapAndUnits.process_sco";

size_t MapAndUnits::process_sco(const TokenMessage& sco)
{
   Debug::ft(MapAndUnits_process_sco);

   if(number_of_provinces == 0)
   {
      Debug::SwLog(MapAndUnits_process_sco, "map has no provinces", 0);
      return NO_ERROR;
   }

   auto signal = sco.get_parm(0);

   if(!signal.is_single_token() || (signal.front() != TOKEN_COMMAND_SCO))
   {
      return 0;
   }

   our_centres.clear();

   for(size_t count = 1; count < sco.parm_count(); ++count)
   {
      auto sco_for_power = sco.get_parm(count);
      auto error = process_sco_for_power(sco_for_power);

      if(error != NO_ERROR)
      {
         return error + sco.parm_start(count);
      }
   }

   return NO_ERROR;
}

//------------------------------------------------------------------------------

size_t MapAndUnits::process_sco_for_power(const TokenMessage& sco_parm)
{
   Debug::ft("MapAndUnits.process_sco_for_power");

   auto power = sco_parm.front();

   for(size_t count = 1; count < sco_parm.size(); ++count)
   {
      auto province = sco_parm.at(count).province_id();

      if((province == NIL_PROVINCE) || (province >= number_of_provinces))
      {
         return count;
      }

      game_map[province].owner = power;

      if(power == our_power)
      {
         our_centres.insert(province);
      }
   }

   return NO_ERROR;
}

//------------------------------------------------------------------------------

size_t MapAndUnits::process_sub
   (const TokenMessage& sub, PowerId power, Token results[])
{
   Debug::ft("MapAndUnits.process_sub");

   auto signal = sub.get_parm(0);

   if(!signal.is_single_token() || (signal.front() != TOKEN_COMMAND_SUB))
   {
      return 0;
   }

   for(size_t count = 1; count < sub.parm_count(); ++count)
   {
      auto order = sub.get_parm(count);
      results[count - 1] = process_order(order, power);
   }

   return NO_ERROR;
}

//------------------------------------------------------------------------------

size_t MapAndUnits::process_supply_centres(const TokenMessage& centres)
{
   Debug::ft("MapAndUnits.process_supply_centres");

   for(size_t count = 0; count < centres.parm_count(); ++count)
   {
      auto error = process_supply_centres_for_power(centres.get_parm(count));

      if(error != NO_ERROR)
      {
         return error + centres.parm_start(count);
      }
   }

   return NO_ERROR;
}

//------------------------------------------------------------------------------

size_t MapAndUnits::process_supply_centres_for_power
   (const TokenMessage& centres)
{
   Debug::ft("MapAndUnits.process_supply_centres_for_power");

   PowerSet home_powers;
   Token power = TOKEN_PARAMETER_UNO;  // default = unowned (neutral)

   for(size_t count = 0; count < centres.parm_count(); ++count)
   {
      auto parm = centres.get_parm(count);

      if(parm.is_single_token())
      {
         auto token = parm.front();
         auto p = token.power_id();

         if(p != NIL_POWER)
         {
            if(p < number_of_powers)
            {
               home_powers.insert(p);
               power = token;
            }
            else
            {
               return centres.parm_start(count);
            }
         }
         else if(token.is_province())
         {
            auto& province = game_map[token.province_id()];

            if(!province.is_valid)
            {
               province.is_valid = true;
               province.is_supply_centre = true;
               province.token = token;
               province.owner = power;
               province.home_powers = home_powers;
            }
            else
            {
               return centres.parm_start(count);
            }
         }
         else if(token != TOKEN_PARAMETER_UNO)
         {
            return centres.parm_start(count);
         }
      }
      else
      {
         //  This home centre must be shared by multiple powers.
         //
         for(size_t subparm = 0; subparm < parm.size(); ++subparm)
         {
            auto token = parm.at(subparm);
            auto p = token.power_id();

            if(p != NIL_POWER)
            {
               if(p < number_of_powers)
               {
                  home_powers.insert(token.power_id());
                  power = token;
               }
               else
               {
                  return subparm + centres.parm_start(count);
               }
            }
            else
            {
               return centres.parm_start(count);
            }
         }
      }
   }

   return NO_ERROR;
}

//------------------------------------------------------------------------------

fn_name MapAndUnits_province_token = "MapAndUnits.province";

Token MapAndUnits::province_token(ProvinceId province) const
{
   if((province < 0) || (province >= PROVINCE_MAX))
   {
      Debug::SwLog(MapAndUnits_province_token, "invalid province", province);
      return INVALID_TOKEN;
   }

   return game_map[province].token;
}

//------------------------------------------------------------------------------

void MapAndUnits::set_build_order(Location location)
{
   Debug::ft("MapAndUnits.set_build_order");

   //  If a build order for this province has already been submitted, erase
   //  it.  Note that the build might have been on a different coast.
   //
   Location first_coast(location.province, 0);
   auto match = our_winter_orders.adjustments.lower_bound(first_coast);

   if((match != our_winter_orders.adjustments.end()) &&
      (match->first.province == location.province))
   {
      our_winter_orders.adjustments.erase(match);
   }

   our_winter_orders.adjustments.insert
      (Adjustments::value_type(location, Token(0)));
   our_winter_orders.is_building = true;
}

//------------------------------------------------------------------------------

bool MapAndUnits::set_convoy_order
   (ProvinceId unit, ProvinceId client, ProvinceId dest)
{
   Debug::ft("MapAndUnits.set_convoy_order");

   auto unit_to_order = units.find(unit);

   if(unit_to_order == units.end())
   {
      return false;
   }

   unit_to_order->second.order = CONVOY_ORDER;
   unit_to_order->second.client_loc = client;
   unit_to_order->second.client_dest = dest;
   return true;
}

//------------------------------------------------------------------------------

bool MapAndUnits::set_disband_order(ProvinceId unit)
{
   Debug::ft("MapAndUnits.set_disband_order");

   auto unit_to_order = dislodged_units.find(unit);

   if(unit_to_order == dislodged_units.end())
   {
      return false;
   }

   unit_to_order->second.order = DISBAND_ORDER;
   return true;
}

//------------------------------------------------------------------------------

bool MapAndUnits::set_hold_order(ProvinceId unit)
{
   Debug::ft("MapAndUnits.set_hold_order");

   auto unit_to_order = units.find(unit);

   if(unit_to_order == units.end())
   {
      return false;
   }

   unit_to_order->second.order = HOLD_ORDER;
   return true;
}

//------------------------------------------------------------------------------

bool MapAndUnits::set_move_by_convoy_order
   (ProvinceId unit, ProvinceId dest, size_t length, const ProvinceId fleets[])
{
   Debug::ft("MapAndUnits.set_move_by_convoy_order");

   auto unit_to_order = units.find(unit);

   if(unit_to_order == units.end())
   {
      return false;
   }
   else
   {
      unit_to_order->second.order = MOVE_BY_CONVOY_ORDER;
      unit_to_order->second.dest.province = dest;
      unit_to_order->second.dest.coast = TOKEN_UNIT_AMY;
      unit_to_order->second.convoyers.clear();

      for(size_t f = 0; f < length; ++f)
      {
         unit_to_order->second.convoyers.push_back(fleets[f]);
      }
   }

   return true;
}

//------------------------------------------------------------------------------

bool MapAndUnits::set_move_by_single_step_convoy_order
   (ProvinceId unit, ProvinceId dest, ProvinceId fleet)
{
   return set_move_by_convoy_order(unit, dest, 1, &fleet);
}

//------------------------------------------------------------------------------

bool MapAndUnits::set_move_order(ProvinceId unit, Location dest)
{
   Debug::ft("MapAndUnits.set_move_order");

   auto unit_to_order = units.find(unit);

   if(unit_to_order == units.end())
   {
      return false;
   }

   unit_to_order->second.order = MOVE_ORDER;
   unit_to_order->second.dest = dest;
   return true;
}

//------------------------------------------------------------------------------

void MapAndUnits::set_multiple_waive_orders(size_t waives)
{
   Debug::ft("MapAndUnits.set_multiple_waive_orders");

   our_winter_orders.number_of_waives += waives;
}

//------------------------------------------------------------------------------

void MapAndUnits::set_order_checking(bool on_submission, bool on_adjudication)
{
   Debug::ft("MapAndUnits.set_order_checking");

   check_on_submission = on_submission;
   check_on_adjudication = on_adjudication;
}

//------------------------------------------------------------------------------

void MapAndUnits::set_our_power(const Token& token)
{
   Debug::ft("MapAndUnits.set_our_power");

   our_power = token;

   //  Build our list of home centres.
   //
   home_centres.clear();

   if(token.is_power())  // can also be OBS
   {
      auto power = token.power_id();

      for(ProvinceId p = 0; p < number_of_provinces; ++p)
      {
         auto& province = game_map[p];

         if(province.home_powers.find(power) != province.home_powers.end())
         {
            home_centres.insert(p);
         }
      }
   }
}

//------------------------------------------------------------------------------

bool MapAndUnits::set_remove_order(ProvinceId unit)
{
   Debug::ft("MapAndUnits.set_remove_order");

   auto unit_to_order = units.find(unit);

   if(unit_to_order == units.end())
   {
      return false;
   }

   our_winter_orders.adjustments.insert
      (Adjustments::value_type(unit_to_order->second.loc, Token(0)));
   our_winter_orders.is_building = false;
   return true;
}

//------------------------------------------------------------------------------

bool MapAndUnits::set_retreat_order(ProvinceId unit, Location dest)
{
   Debug::ft("MapAndUnits.set_retreat_order");

   auto unit_to_order = dislodged_units.find(unit);

   if(unit_to_order == dislodged_units.end())
   {
      return false;
   }

   unit_to_order->second.order = RETREAT_ORDER;
   unit_to_order->second.dest = dest;
   return true;
}

//------------------------------------------------------------------------------

bool MapAndUnits::set_support_to_hold_order(ProvinceId unit, ProvinceId client)
{
   Debug::ft("MapAndUnits.set_support_to_hold_order");

   auto unit_to_order = units.find(unit);

   if(unit_to_order == units.end())
   {
      return false;
   }

   unit_to_order->second.order = SUPPORT_TO_HOLD_ORDER;
   unit_to_order->second.client_loc = client;
   return true;
}

//------------------------------------------------------------------------------

bool MapAndUnits::set_support_to_move_order
   (ProvinceId unit, ProvinceId client, ProvinceId dest)
{
   Debug::ft("MapAndUnits.set_support_to_move_order");

   auto unit_to_order = units.find(unit);

   if(unit_to_order == units.end())
   {
      return false;
   }

   unit_to_order->second.order = SUPPORT_TO_MOVE_ORDER;
   unit_to_order->second.client_loc = client;
   unit_to_order->second.client_dest = dest;
   return true;
}

//------------------------------------------------------------------------------

void MapAndUnits::set_total_number_of_waive_orders(size_t waives)
{
   Debug::ft("MapAndUnits.set_total_number_of_waive_orders");

   our_winter_orders.number_of_waives = waives;
}

//------------------------------------------------------------------------------

void MapAndUnits::set_waive_order()
{
   Debug::ft("MapAndUnits.set_waive_order");

   our_winter_orders.number_of_waives++;
}

//------------------------------------------------------------------------------

bool MapAndUnits::unorder_adjustment(const TokenMessage& not_sub, PowerId power)
{
   Debug::ft("MapAndUnits.unorder_adjustment");

   if(curr_season != TOKEN_SEASON_WIN)
   {
      return false;
   }

   auto sub = not_sub.get_parm(1);
   auto order = sub.get_parm(1);
   auto order_token_message = order.get_parm(1);
   auto order_token = order_token_message.front();

   if(order_token.order_season() != Token::BUILD_SEASON)
   {
      return false;
   }

   auto& winter = winter_orders[power];

   if((order_token == TOKEN_ORDER_BLD) || (order_token == TOKEN_ORDER_REM))
   {
      if(winter.is_building ^ (order_token == TOKEN_ORDER_BLD))
      {
         return false;
      }

      Location build_loc;
      auto winter_order = order.get_parm(0);

      if(winter_order.parm_is_single_token(2))
      {
         build_loc.province = winter_order.at(2).province_id();
         build_loc.coast = winter_order.at(1);
      }
      else
      {
         build_loc.province = winter_order.get_parm(2).at(0).province_id();
         build_loc.coast = winter_order.get_parm(2).at(1);
      }

      if(winter_order.at(0).power_id() != power)
      {
         return false;
      }

      auto match = winter.adjustments.find(build_loc);

      if(match != winter.adjustments.end())
      {
         //  Found the matching build/removal.  Delete it.
         //
         winter.adjustments.erase(match);
         return true;
      }

      return false;
   }

   if(order_token == TOKEN_ORDER_WVE)
   {
      if(!winter.is_building || (winter.number_of_waives == 0))
      {
         return false;
      }

      if(order.at(0).power_id() != power)
      {
         return false;
      }

      winter.number_of_waives--;
      return true;
   }

   return false;
}
}
