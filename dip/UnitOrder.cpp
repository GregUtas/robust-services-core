//==============================================================================
//
//  UnitOrder.cpp
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
#include "UnitOrder.h"
#include <cstddef>
#include <ostream>
#include "Debug.h"
#include "MapAndUnits.h"
#include "SysTypes.h"
#include "TokenMessage.h"

using std::ostream;
using namespace NodeBase;

//------------------------------------------------------------------------------

namespace Diplomacy
{
UnitOrder::UnitOrder() :
   owner(INVALID_TOKEN),
   unit_type(INVALID_TOKEN),
   order(NO_ORDER),
   client_loc(NIL_PROVINCE),
   client_dest(NIL_PROVINCE),
   order_type_copy(NO_ORDER),
   supports_to_dislodge(0),
   dislodged_from(NIL_PROVINCE),
   is_support_to_dislodge(false),
   move_number(NIL_MOVE_NUMBER),
   ring_status(NIL_RING_STATUS),
   no_convoy(false),
   no_army_to_convoy(false),
   convoy_disrupted(false),
   support_void(false),
   support_cut(false),
   bounce(false),
   dislodged(false),
   unit_moves(false),
   illegal_order(false),
   illegal_reason(INVALID_TOKEN)
{
}

//------------------------------------------------------------------------------

void UnitOrder::decode_order(const TokenMessage& ord)
{
   Debug::ft("UnitOrder.decode_order");

   TokenMessage fleets;

   switch(ord.get_parm(1).front().all())
   {
   case TOKEN_ORDER_HLD:
      order = HOLD_ORDER;
      break;

   case TOKEN_ORDER_MTO:
      order = MOVE_ORDER;
      dest = Location(ord.get_parm(2), unit_type);
      break;

   case TOKEN_ORDER_SUP:
      if(ord.parm_count() == 3)
      {
         order = SUPPORT_TO_HOLD_ORDER;
         client_loc = Location(ord.get_parm(2)).province;
      }
      else
      {
         order = SUPPORT_TO_MOVE_ORDER;
         client_loc = Location(ord.get_parm(2)).province;
         client_dest = ord.get_parm(4).front().province_id();
      }
      break;

   case TOKEN_ORDER_CVY:
      order = CONVOY_ORDER;
      client_loc = Location(ord.get_parm(2)).province;
      client_dest = ord.get_parm(4).front().province_id();
      break;

   case TOKEN_ORDER_CTO:
      order = MOVE_BY_CONVOY_ORDER;
      dest = Location(ord.get_parm(2), unit_type);
      convoyers.clear();

      fleets = ord.get_parm(4);

      for(size_t f = 0; f < fleets.size(); ++f)
      {
         convoyers.push_back(fleets.at(f).province_id());
      }
      break;

   case TOKEN_ORDER_DSB:
      order = DISBAND_ORDER;
      break;

   case TOKEN_ORDER_RTO:
      order = RETREAT_ORDER;
      dest = Location(ord.get_parm(2), unit_type);
      break;
   }
}

//------------------------------------------------------------------------------

void UnitOrder::decode_result(const TokenMessage& result)
{
   Debug::ft("UnitOrder.decode_result");

   no_convoy = false;
   no_army_to_convoy = false;
   convoy_disrupted = false;
   support_void = false;
   support_cut = false;
   bounce = false;
   dislodged = false;
   unit_moves = false;
   illegal_order = false;

   for(size_t count = 0; count < result.size(); ++count)
   {
      auto result_token = result.at(count);

      switch(result_token.all())
      {
      case CATEGORY_ORDER_NOTE:
         illegal_order = true;
         illegal_reason = result_token;
         break;

      case TOKEN_RESULT_SUC:
         switch(order)
         {
         case MOVE_ORDER:
         case MOVE_BY_CONVOY_ORDER:
         case RETREAT_ORDER:
            unit_moves = true;
         }
         break;

      case TOKEN_RESULT_BNC:
         bounce = true;
         break;

      case TOKEN_RESULT_CUT:
         support_cut = true;
         break;

      case TOKEN_RESULT_DSR:
         convoy_disrupted = true;
         break;

      case TOKEN_RESULT_NSO:
         switch(order)
         {
         case SUPPORT_TO_HOLD_ORDER:
         case SUPPORT_TO_MOVE_ORDER:
            support_void = true;
            break;

         case CONVOY_ORDER:
            no_army_to_convoy = true;
            break;

         case MOVE_BY_CONVOY_ORDER:
            no_convoy = true;
            break;
         }
         break;

      case TOKEN_RESULT_RET:
         dislodged = true;
         break;
      }
   }
}

//------------------------------------------------------------------------------

void UnitOrder::mark_convoy_disrupted()
{
   Debug::ft("UnitOrder.mark_convoy_disrupted");

   auto& units = MapAndUnits::instance()->units;

   for(auto f = convoyers.begin(); f != convoyers.end(); ++f)
   {
      units[*f].order_type_copy = HOLD_ORDER;
   }

   convoy_disrupted = true;
   order_type_copy = HOLD_NO_SUPPORT_ORDER;
   supports.clear();
   supports_to_dislodge = 0;
}

//------------------------------------------------------------------------------

void UnitOrder::mark_move_bounced()
{
   Debug::ft("UnitOrder.mark_move_bounced");

   bounce = true;
   order_type_copy = HOLD_NO_SUPPORT_ORDER;
   supports.clear();
   supports_to_dislodge = 0;
}

//------------------------------------------------------------------------------

void UnitOrder::mark_move_illegal(const Token& reason)
{
   Debug::ft("UnitOrder.mark_move_illegal");

   illegal_order = true;
   illegal_reason = reason;
   order_type_copy = HOLD_ORDER;
}

//------------------------------------------------------------------------------

ostream& operator<<(ostream& stream, const UnitOrder& order)
{
   stream << order.unit_type << SPACE << order.loc;
   return stream;
}
}
