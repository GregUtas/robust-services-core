//==============================================================================
//
//  Token.cpp
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
#include "Token.h"
#include <cstddef>
#include <map>
#include <sstream>
#include <utility>
#include "Debug.h"
#include "MapAndUnits.h"
#include "TokenMessage.h"
#include "TokenTextMap.h"

using std::ostream;
using std::string;
using namespace NodeBase;

//------------------------------------------------------------------------------

namespace Diplomacy
{
Token::Token(const category_t cat, const subtoken_t sub)
{
   token_.cat_ = cat;
   token_.sub_ = sub;
}

//------------------------------------------------------------------------------

Token& Token::operator=(const Token& that)
{
   if(&that != this) this->full_ = that.full_;
   return *this;
}

//------------------------------------------------------------------------------

int Token::get_number() const
{
   if(!is_number()) return INVALID_TOKEN;
   if((full_ & NEGATIVE_MASK) != NEGATIVE_MASK_CHECK) return full_;
   return (full_ | MAKE_NEGATIVE_MASK);
}

//------------------------------------------------------------------------------

bool Token::is_number() const
{
   return ((full_ & NUMBER_MASK) == NUMBER_MASK_CHECK);
}

//------------------------------------------------------------------------------

bool Token::is_power() const
{
   return (token_.cat_ == CATEGORY_POWER);
}

//------------------------------------------------------------------------------

bool Token::is_province() const
{
   return ((full_ & PROVINCE_MASK) == PROVINCE_MASK_CHECK);
}

//------------------------------------------------------------------------------

TokenMessage Token::operator+(const Token& that) const
{
   Debug::ft("Token.operator+(token)");

   TokenMessage message(*this);
   return message + that;
}

//------------------------------------------------------------------------------

TokenMessage Token::operator+(const TokenMessage& that) const
{
   Debug::ft("Token.operator+(message)");

   TokenMessage message(*this);
   return message + that;
}

//------------------------------------------------------------------------------

TokenMessage Token::operator&(const Token& that) const
{
   Debug::ft("Token.operator&(token)");

   TokenMessage message(*this);
   return message & that;
}

//------------------------------------------------------------------------------

TokenMessage Token::operator&(const TokenMessage& that) const
{
   Debug::ft("Token.operator&(message)");

   TokenMessage message(*this);
   return message & that;
}

//------------------------------------------------------------------------------

Token::SeasonType Token::order_season() const
{
   auto type = full_ & ORDER_TURN_MASK;
   if(type == ORDER_MOVE_TURN_CHECK) return MOVE_SEASON;
   if(type == ORDER_RETREAT_TURN_CHECK) return RETREAT_SEASON;
   if(type == ORDER_BUILD_TURN_CHECK) return BUILD_SEASON;
   return NOT_AN_ORDER;
}

//------------------------------------------------------------------------------

PowerId Token::power_id() const
{
   if(token_.cat_ == CATEGORY_POWER) return token_.sub_;
   return NIL_POWER;
}

//------------------------------------------------------------------------------

ProvinceId Token::province_id() const
{
   if((full_ & PROVINCE_MASK) == PROVINCE_MASK_CHECK) return token_.sub_;
   return NIL_PROVINCE;
}

//------------------------------------------------------------------------------

fn_name Token_set_number = "Token.set_number";

bool Token::set_number(int number)
{
   Debug::ft(Token_set_number);

   auto result = true;

   if(number > NUMERIC_MAX)
   {
      Debug::SwLog(Token_set_number, "invalid number", number);
      number = NUMERIC_MAX;
      result = false;
   }
   else if(number < NUMERIC_MIN)
   {
      Debug::SwLog(Token_set_number, "invalid number", number);
      number = NUMERIC_MIN;
      result = false;
   }

   full_ = number & ~NUMBER_MASK;
   return result;
}

//------------------------------------------------------------------------------

string Token::to_str() const
{
   if(token_.cat_ == CATEGORY_ASCII)
   {
      return string(1, token_.sub_);
   }

   if(is_number())
   {
      std::ostringstream stream;
      stream << get_number();
      return stream.str();
   }

   auto token_to_text_map = &TokenTextMap::instance()->token_to_text_map();
   auto token = token_to_text_map->find(full_);
   if(token != token_to_text_map->end()) return token->second;
   return INVALID_TOKEN_STR;
}

//==============================================================================

fixed_string INVALID_TOKEN_STR = "???";

constexpr size_t UNITS_SIZE = 2;

fixed_string units[] = { "A", "F" };

constexpr size_t COASTS_SIZE = 7;

fixed_string coasts[] =
   { "(nc)", "(nec)", "(ec)", "(sec)", "(sc)", "(swc)", "(wc)", "(nwc)" };

constexpr size_t SEASONS_SIZE = 4;

fixed_string seasons[] = { "Spring", "Summer", "Fall", "Autumn", "Winter" };

//------------------------------------------------------------------------------

ostream& operator<<(ostream& stream, const Token& token)
{
   auto cat = token.category();
   auto sub = token.subtoken();

   switch(cat)
   {
   case CATEGORY_UNIT:
      if(sub < UNITS_SIZE)
      {
         stream << units[sub];
         return stream;
      }
      break;

   case CATEGORY_COAST:
      sub = sub >> 1;  // surprise!

      if(sub < COASTS_SIZE)
      {
         stream << coasts[sub];
         return stream;
      }
      break;

   case CATEGORY_SEASON:
      if(sub <= SEASONS_SIZE)
      {
         stream << seasons[sub];
         return stream;
      }
      break;

   default:
      if((cat >= CATEGORY_PROVINCE_MIN) && (cat <= CATEGORY_PROVINCE_MAX))
      {
         stream << MapAndUnits::instance()->display_province(sub);
         return stream;
      }
   }

   stream << token.to_str();
   return stream;
}

//------------------------------------------------------------------------------

fn_name Diplomacy_power_token = "Diplomacy.power_token";

Token power_token(PowerId power)
{
   if((power < 0) || (power >= POWER_MAX))
   {
      Debug::SwLog(Diplomacy_power_token, "invalid power", power);
      return INVALID_TOKEN;
   }

   return Token(CATEGORY_POWER, power);
}

//------------------------------------------------------------------------------

Token province_token(ProvinceId province)
{
   return MapAndUnits::instance()->province_token(province);
}
}
