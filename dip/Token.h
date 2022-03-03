//==============================================================================
//
//  Token.h
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
#ifndef TOKEN_H_INCLUDED
#define TOKEN_H_INCLUDED

#include <iosfwd>
#include <string>
#include "DipTypes.h"
#include "SysTypes.h"

namespace Diplomacy
{
   class TokenMessage;
}

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace Diplomacy
{
//  Provides a wrapper for language tokens used in the protocol.
//
class Token
{
public:
   //  Constructs an empty token.
   //
   Token() : full_(INVALID_TOKEN) { }

   //  Constructs a token from its raw value.  Can be used implicitly.
   //
   Token(token_t token) : full_(token) { }

   //  Constructs a token from its category and subcategory.
   //
   Token(const category_t cat, const subtoken_t sub);

   //  Copy constructor.
   //
   Token(const Token& that) : full_(that.full_) { }

   //  Copy operator.
   //
   Token& operator=(const Token& that);

   //  Returns the token's raw value.
   //
   token_t all() const { return full_; }

   //  Returns the token's category.
   //
   category_t category() const { return token_.cat_; }

   //  Returns the token's subcategory.
   //
   subtoken_t subtoken() const { return token_.sub_; }

   //  Returns true if the token identifies a power.
   //
   bool is_power() const;

   //  Maps the token to a power.  Returns NIL_POWER if the token
   //  is not that of a power.
   //
   PowerId power_id() const;

   //  Returns true if the token identifies a province.
   //
   bool is_province() const;

   //  Maps the token to a province.  Returns NIL_PROVINCE if the
   //  token is not that of a province.
   //
   ProvinceId province_id() const;

   //  The maximum and minimum numeric values that a token can represent.
   //
   static const int NUMERIC_MAX = 8191;
   static const int NUMERIC_MIN = -8191;

   //  Returns true if the token is a numeric value.
   //
   bool is_number() const;

   //  Returns the token's numeric value.  Returns INVALID_TOKEN if the
   //  token is not that of a numeric value.
   //
   int get_number() const;

   //  Sets the token to the numeric value NUMBER.  Generates a log and
   //  sets the minimum or maximum value if NUMBER is out of range.
   //
   bool set_number(int number);

   //  The type of season in which an order token is valid.
   //
   enum SeasonType : int16_t
   {
      NOT_AN_ORDER,    // token is not an order type
      MOVE_SEASON,     // order is valid in spring or fall
      RETREAT_SEASON,  // order is valid in summer or autumn
      BUILD_SEASON     // order is valid in winter
   };

   //  Returns the type of season in which a move order is valid.
   //
   SeasonType order_season() const;

   //  Returns a string for displaying the token.
   //
   std::string to_str() const;

   //  Returns true if this token matches THAT.
   //
   bool operator==(const Token& that) const
   {
      return (this->full_ == that.full_);
   }

   //  Returns true if this token does not match THAT.
   //
   bool operator!=(const Token& that) const
   {
      return (this->full_ != that.full_);
   }

   //  Returns true if this token is less than THAT.
   //
   bool operator<(const Token& that) const
   {
      return (this->full_ < that.full_);
   }

   //  The + operators perform straight concatenation (i.e. append).
   //  The & operators enclose THAT in parentheses before appending.
   //
   TokenMessage operator+(const Token& that) const;
   TokenMessage operator&(const Token& that) const;
   TokenMessage operator+(const TokenMessage& that) const;
   TokenMessage operator&(const TokenMessage& that) const;
private:
   //  Masks and values used when checking various types of tokens.
   //
   static const token_t NUMBER_MASK = 0xC000;
   static const token_t NUMBER_MASK_CHECK = 0;
   static const token_t NEGATIVE_MASK = 0x2000;
   static const token_t NEGATIVE_MASK_CHECK = 0x2000;
   static const token_t MAKE_NEGATIVE_MASK = 0xE000;
   static const token_t PROVINCE_MASK = 0xF800;
   static const token_t PROVINCE_MASK_CHECK = 0x5000;
   static const token_t ORDER_TURN_MASK = 0xFFF0;
   static const token_t ORDER_MOVE_TURN_CHECK = 0x4320;
   static const token_t ORDER_RETREAT_TURN_CHECK = 0x4340;
   static const token_t ORDER_BUILD_TURN_CHECK = 0x4380;

   //  Used to access a token's subcategory and category.
   //
   struct split_t
   {
      subtoken_t sub_;  // low-order byte
      category_t cat_;  // high-order byte

      split_t() : sub_(INVALID_TOKEN & 0xff), cat_(INVALID_TOKEN >> 8) { }
   };

   union
   {
      token_t full_;   // raw value
      split_t token_;  // category and subcategory
   };
};

//  Returns the token for a power.  Returns INVALID_TOKEN and
//  generates a log if POWER is out of range.
//
Token power_token(PowerId power);

//  Returns the token for a province.  Returns INVALID_TOKEN if
//  PROVINCE is out of range.
//
Token province_token(ProvinceId province);

//  Inserts a string for TOKEN into STREAM.  This is usually, but not
//  always, its to_str() representation, so it should not be used to
//  build messages or display them verbatim.
//
std::ostream& operator<<(std::ostream& stream, const Token& token);

//  For displaying an invalid token.
//
extern fixed_string INVALID_TOKEN_STR;

//  Token categories.  Tokens in the range 0x5800 to 0x5FFF may be used
//  for internal purposes but must not be included in external messages.
//
constexpr category_t CATEGORY_NUMBER_MIN = 0x00;
constexpr category_t CATEGORY_NUMBER_MAX = 0x3F;
constexpr category_t CATEGORY_BRACKET = 0x40;
constexpr category_t CATEGORY_POWER = 0x41;
constexpr category_t CATEGORY_UNIT = 0x42;
constexpr category_t CATEGORY_ORDER = 0x43;
constexpr category_t CATEGORY_ORDER_NOTE = 0x44;
constexpr category_t CATEGORY_RESULT = 0x45;
constexpr category_t CATEGORY_COAST = 0x46;
constexpr category_t CATEGORY_SEASON = 0x47;
constexpr category_t CATEGORY_COMMAND = 0x48;
constexpr category_t CATEGORY_PARAMETER = 0x49;
constexpr category_t CATEGORY_PRESS = 0x4A;
constexpr category_t CATEGORY_ASCII = 0x4B;
constexpr category_t CATEGORY_PROVINCE_MIN = 0x50;
constexpr category_t CATEGORY_PROVINCE_MAX = 0x57;

constexpr token_t TOKEN_END_OF_MESSAGE = 0x5FFF;  // for internal use

constexpr token_t TOKEN_OPEN_BRACKET = 0x4000;
constexpr token_t TOKEN_CLOSE_BRACKET = 0x4001;

constexpr token_t TOKEN_POWER_AUS = 0x4100;  // Austria
constexpr token_t TOKEN_POWER_ENG = 0x4101;  // England
constexpr token_t TOKEN_POWER_FRA = 0x4102;  // France
constexpr token_t TOKEN_POWER_GER = 0x4103;  // Germany
constexpr token_t TOKEN_POWER_ITA = 0x4104;  // Italy
constexpr token_t TOKEN_POWER_RUS = 0x4105;  // Russia
constexpr token_t TOKEN_POWER_TUR = 0x4106;  // Turkey

constexpr token_t TOKEN_UNIT_AMY = 0x4200;  // army
constexpr token_t TOKEN_UNIT_FLT = 0x4201;  // fleet

constexpr token_t TOKEN_ORDER_CTO = 0x4320;  // convoy to
constexpr token_t TOKEN_ORDER_CVY = 0x4321;  // convoy
constexpr token_t TOKEN_ORDER_HLD = 0x4322;  // hold
constexpr token_t TOKEN_ORDER_MTO = 0x4323;  // move to
constexpr token_t TOKEN_ORDER_SUP = 0x4324;  // support
constexpr token_t TOKEN_ORDER_VIA = 0x4325;  // convoy via (with list of fleets)
constexpr token_t TOKEN_ORDER_DSB = 0x4340;  // disband
constexpr token_t TOKEN_ORDER_RTO = 0x4341;  // retreat to
constexpr token_t TOKEN_ORDER_BLD = 0x4380;  // build
constexpr token_t TOKEN_ORDER_REM = 0x4381;  // remove
constexpr token_t TOKEN_ORDER_WVE = 0x4382;  // waive

constexpr token_t TOKEN_ORDER_NOTE_MBV = 0x4400;  // move accepted
constexpr token_t TOKEN_ORDER_NOTE_BPR = 0x4401;  // (obsolete)
constexpr token_t TOKEN_ORDER_NOTE_CST = 0x4402;  // no coast specified
constexpr token_t TOKEN_ORDER_NOTE_ESC = 0x4403;  // not an empty supply centre
constexpr token_t TOKEN_ORDER_NOTE_FAR = 0x4404;  // not adjacent
constexpr token_t TOKEN_ORDER_NOTE_HSC = 0x4405;  // not a home supply centre
constexpr token_t TOKEN_ORDER_NOTE_NAS = 0x4406;  // not at sea (in CVY)
constexpr token_t TOKEN_ORDER_NOTE_NMB = 0x4407;  // no more builds allowed
constexpr token_t TOKEN_ORDER_NOTE_NMR = 0x4408;  // no more removals allowed
constexpr token_t TOKEN_ORDER_NOTE_NRN = 0x4409;  // no retreat needed
constexpr token_t TOKEN_ORDER_NOTE_NRS = 0x440A;  // not the right season
constexpr token_t TOKEN_ORDER_NOTE_NSA = 0x440B;  // no such army (in CTO/CVY)
constexpr token_t TOKEN_ORDER_NOTE_NSC = 0x440C;  // not a supply centre
constexpr token_t TOKEN_ORDER_NOTE_NSF = 0x440D;  // no such fleet (in CVY/VIA)
constexpr token_t TOKEN_ORDER_NOTE_NSP = 0x440E;  // no such province
constexpr token_t TOKEN_ORDER_NOTE_NSU = 0x4410;  // no such unit
constexpr token_t TOKEN_ORDER_NOTE_NVR = 0x4411;  // not a void retreat province
constexpr token_t TOKEN_ORDER_NOTE_NYU = 0x4412;  // not your unit
constexpr token_t TOKEN_ORDER_NOTE_YSC = 0x4413;  // not your supply centre

constexpr token_t TOKEN_RESULT_SUC = 0x4500;  // succeeded
constexpr token_t TOKEN_RESULT_BNC = 0x4501;  // bounced
constexpr token_t TOKEN_RESULT_CUT = 0x4502;  // support cut
constexpr token_t TOKEN_RESULT_DSR = 0x4503;  // fleet dislodged: CTO failed
constexpr token_t TOKEN_RESULT_FLD = 0x4504;  // (obsolete)
constexpr token_t TOKEN_RESULT_NSO = 0x4505;  // no such order (in SUP/CVY/CTO)
constexpr token_t TOKEN_RESULT_RET = 0x4506;  // dislodged: must retreat

constexpr token_t TOKEN_COAST_NCS = 0x4600;  // north coast
constexpr token_t TOKEN_COAST_NEC = 0x4602;  // northeast coast
constexpr token_t TOKEN_COAST_ECS = 0x4604;  // east coast
constexpr token_t TOKEN_COAST_SEC = 0x4606;  // southeast coast
constexpr token_t TOKEN_COAST_SCS = 0x4608;  // south coast
constexpr token_t TOKEN_COAST_SWC = 0x460A;  // southwest coast
constexpr token_t TOKEN_COAST_WCS = 0x460C;  // west coast
constexpr token_t TOKEN_COAST_NWC = 0x460E;  // northwest coast

constexpr token_t TOKEN_SEASON_SPR = 0x4700;  // spring moves
constexpr token_t TOKEN_SEASON_SUM = 0x4701;  // summer retreats
constexpr token_t TOKEN_SEASON_FAL = 0x4702;  // fall moves
constexpr token_t TOKEN_SEASON_AUT = 0x4703;  // autumn retreats
constexpr token_t TOKEN_SEASON_WIN = 0x4704;  // winter adjustments

constexpr token_t TOKEN_COMMAND_CCD = 0x4800;  // power in civil disorder
constexpr token_t TOKEN_COMMAND_DRW = 0x4801;  // draw
constexpr token_t TOKEN_COMMAND_FRM = 0x4802;  // message from
constexpr token_t TOKEN_COMMAND_GOF = 0x4803;  // go flag (ready to move now)
constexpr token_t TOKEN_COMMAND_HLO = 0x4804;  // hello (start of game)
constexpr token_t TOKEN_COMMAND_HST = 0x4805;  // history
constexpr token_t TOKEN_COMMAND_HUH = 0x4806;  // not understood
constexpr token_t TOKEN_COMMAND_IAM = 0x4807;  // I am
constexpr token_t TOKEN_COMMAND_LOD = 0x4808;  // load game
constexpr token_t TOKEN_COMMAND_MAP = 0x4809;  // map for game
constexpr token_t TOKEN_COMMAND_MDF = 0x480A;  // map definition
constexpr token_t TOKEN_COMMAND_MIS = 0x480B;  // missing orders
constexpr token_t TOKEN_COMMAND_NME = 0x480C;  // name
constexpr token_t TOKEN_COMMAND_NOT = 0x480D;  // logical not
constexpr token_t TOKEN_COMMAND_NOW = 0x480E;  // current position
constexpr token_t TOKEN_COMMAND_OBS = 0x480F;  // observer
constexpr token_t TOKEN_COMMAND_OFF = 0x4810;  // turn off (exit)
constexpr token_t TOKEN_COMMAND_ORD = 0x4811;  // order results
constexpr token_t TOKEN_COMMAND_OUT = 0x4812;  // power eliminated
constexpr token_t TOKEN_COMMAND_PRN = 0x4813;  // parenthesis error
constexpr token_t TOKEN_COMMAND_REJ = 0x4814;  // reject
constexpr token_t TOKEN_COMMAND_SCO = 0x4815;  // supply centre ownership
constexpr token_t TOKEN_COMMAND_SLO = 0x4816;  // solo
constexpr token_t TOKEN_COMMAND_SND = 0x4817;  // send message
constexpr token_t TOKEN_COMMAND_SUB = 0x4818;  // submit order
constexpr token_t TOKEN_COMMAND_SVE = 0x4819;  // save game
constexpr token_t TOKEN_COMMAND_THX = 0x481A;  // thanks for the order
constexpr token_t TOKEN_COMMAND_TME = 0x481B;  // time to deadline
constexpr token_t TOKEN_COMMAND_YES = 0x481C;  // accept
constexpr token_t TOKEN_COMMAND_ADM = 0x481D;  // administrative message
constexpr token_t TOKEN_COMMAND_SMR = 0x481E;  // summary of game outcome

constexpr token_t TOKEN_PARAMETER_AOA = 0x4900;  // any orders allowed
constexpr token_t TOKEN_PARAMETER_BTL = 0x4901;  // build time limit
constexpr token_t TOKEN_PARAMETER_ERR = 0x4902;  // error location
constexpr token_t TOKEN_PARAMETER_LVL = 0x4903;  // language level
constexpr token_t TOKEN_PARAMETER_MRT = 0x4904;  // must retreat to
constexpr token_t TOKEN_PARAMETER_MTL = 0x4905;  // move time limit
constexpr token_t TOKEN_PARAMETER_NPB = 0x4906;  // no press during builds
constexpr token_t TOKEN_PARAMETER_NPR = 0x4907;  // no press during retreats
constexpr token_t TOKEN_PARAMETER_PDA = 0x4908;  // partial draws allowed
constexpr token_t TOKEN_PARAMETER_PTL = 0x4909;  // press time limit
constexpr token_t TOKEN_PARAMETER_RTL = 0x490A;  // retreat time limit
constexpr token_t TOKEN_PARAMETER_UNO = 0x490B;  // unowned
constexpr token_t TOKEN_PARAMETER_DSD = 0x490D;  // deadline stops on disconnect

constexpr token_t TOKEN_PRESS_ALY = 0x4A00;  // ally
constexpr token_t TOKEN_PRESS_AND = 0x4A01;  // logical and
constexpr token_t TOKEN_PRESS_BWX = 0x4A02;  // none of your business
constexpr token_t TOKEN_PRESS_DMZ = 0x4A03;  // demilitarised zone
constexpr token_t TOKEN_PRESS_ELS = 0x4A04;  // else
constexpr token_t TOKEN_PRESS_EXP = 0x4A05;  // explain
constexpr token_t TOKEN_PRESS_FCT = 0x4A06;  // fact
constexpr token_t TOKEN_PRESS_FOR = 0x4A07;  // for specified turn
constexpr token_t TOKEN_PRESS_FWD = 0x4A08;  // request to forward
constexpr token_t TOKEN_PRESS_HOW = 0x4A09;  // how to attack
constexpr token_t TOKEN_PRESS_IDK = 0x4A0A;  // I don't know
constexpr token_t TOKEN_PRESS_IFF = 0x4A0B;  // if
constexpr token_t TOKEN_PRESS_INS = 0x4A0C;  // insist
constexpr token_t TOKEN_PRESS_OCC = 0x4A0E;  // occupy
constexpr token_t TOKEN_PRESS_ORR = 0x4A0F;  // logical or
constexpr token_t TOKEN_PRESS_PCE = 0x4A10;  // peace
constexpr token_t TOKEN_PRESS_POB = 0x4A11;  // position on board
constexpr token_t TOKEN_PRESS_PRP = 0x4A13;  // propose
constexpr token_t TOKEN_PRESS_QRY = 0x4A14;  // query
constexpr token_t TOKEN_PRESS_SCD = 0x4A15;  // supply centre distribution
constexpr token_t TOKEN_PRESS_SRY = 0x4A16;  // sorry
constexpr token_t TOKEN_PRESS_SUG = 0x4A17;  // suggest
constexpr token_t TOKEN_PRESS_THK = 0x4A18;  // think
constexpr token_t TOKEN_PRESS_THN = 0x4A19;  // then
constexpr token_t TOKEN_PRESS_TRY = 0x4A1A;  // try the following tokens
constexpr token_t TOKEN_PRESS_VSS = 0x4A1C;  // versus
constexpr token_t TOKEN_PRESS_WHT = 0x4A1D;  // what to do with
constexpr token_t TOKEN_PRESS_WHY = 0x4A1E;  // why
constexpr token_t TOKEN_PRESS_XDO = 0x4A1F;  // moves to do
constexpr token_t TOKEN_PRESS_XOY = 0x4A20;  // X owes Y
constexpr token_t TOKEN_PRESS_YDO = 0x4A21;  // you can order these units
constexpr token_t TOKEN_PRESS_CHO = 0x4A22;  // choose
constexpr token_t TOKEN_PRESS_BCC = 0x4A23;  // request to blind copy
constexpr token_t TOKEN_PRESS_UNT = 0x4A24;  // unit

constexpr token_t TOKEN_PROVINCE_BOH = 0x5000;  // Bohemia and all the other
constexpr token_t TOKEN_PROVINCE_BUR = 0x5001;  // provinces on the standard
constexpr token_t TOKEN_PROVINCE_GAL = 0x5002;  // map
constexpr token_t TOKEN_PROVINCE_RUH = 0x5003;
constexpr token_t TOKEN_PROVINCE_SIL = 0x5004;
constexpr token_t TOKEN_PROVINCE_TYR = 0x5005;
constexpr token_t TOKEN_PROVINCE_UKR = 0x5006;
constexpr token_t TOKEN_PROVINCE_BUD = 0x5107;
constexpr token_t TOKEN_PROVINCE_MOS = 0x5108;
constexpr token_t TOKEN_PROVINCE_MUN = 0x5109;
constexpr token_t TOKEN_PROVINCE_PAR = 0x510A;
constexpr token_t TOKEN_PROVINCE_SER = 0x510B;
constexpr token_t TOKEN_PROVINCE_VIE = 0x510C;
constexpr token_t TOKEN_PROVINCE_WAR = 0x510D;
constexpr token_t TOKEN_PROVINCE_ADR = 0x520E;
constexpr token_t TOKEN_PROVINCE_AEG = 0x520F;
constexpr token_t TOKEN_PROVINCE_BAL = 0x5210;
constexpr token_t TOKEN_PROVINCE_BAR = 0x5211;
constexpr token_t TOKEN_PROVINCE_BLA = 0x5212;
constexpr token_t TOKEN_PROVINCE_EAS = 0x5213;
constexpr token_t TOKEN_PROVINCE_ECH = 0x5214;
constexpr token_t TOKEN_PROVINCE_GOB = 0x5215;
constexpr token_t TOKEN_PROVINCE_GOL = 0x5216;
constexpr token_t TOKEN_PROVINCE_HEL = 0x5217;
constexpr token_t TOKEN_PROVINCE_ION = 0x5218;
constexpr token_t TOKEN_PROVINCE_IRI = 0x5219;
constexpr token_t TOKEN_PROVINCE_MAO = 0x521A;
constexpr token_t TOKEN_PROVINCE_NAO = 0x521B;
constexpr token_t TOKEN_PROVINCE_NTH = 0x521C;
constexpr token_t TOKEN_PROVINCE_NWG = 0x521D;
constexpr token_t TOKEN_PROVINCE_SKA = 0x521E;
constexpr token_t TOKEN_PROVINCE_TYS = 0x521F;
constexpr token_t TOKEN_PROVINCE_WES = 0x5220;
constexpr token_t TOKEN_PROVINCE_ALB = 0x5421;
constexpr token_t TOKEN_PROVINCE_APU = 0x5422;
constexpr token_t TOKEN_PROVINCE_ARM = 0x5423;
constexpr token_t TOKEN_PROVINCE_CLY = 0x5424;
constexpr token_t TOKEN_PROVINCE_FIN = 0x5425;
constexpr token_t TOKEN_PROVINCE_GAS = 0x5426;
constexpr token_t TOKEN_PROVINCE_LVN = 0x5427;
constexpr token_t TOKEN_PROVINCE_NAF = 0x5428;
constexpr token_t TOKEN_PROVINCE_PIC = 0x5429;
constexpr token_t TOKEN_PROVINCE_PIE = 0x542A;
constexpr token_t TOKEN_PROVINCE_PRU = 0x542B;
constexpr token_t TOKEN_PROVINCE_SYR = 0x542C;
constexpr token_t TOKEN_PROVINCE_TUS = 0x542D;
constexpr token_t TOKEN_PROVINCE_WAL = 0x542E;
constexpr token_t TOKEN_PROVINCE_YOR = 0x542F;
constexpr token_t TOKEN_PROVINCE_ANK = 0x5530;
constexpr token_t TOKEN_PROVINCE_BEL = 0x5531;
constexpr token_t TOKEN_PROVINCE_BER = 0x5532;
constexpr token_t TOKEN_PROVINCE_BRE = 0x5533;
constexpr token_t TOKEN_PROVINCE_CON = 0x5534;
constexpr token_t TOKEN_PROVINCE_DEN = 0x5535;
constexpr token_t TOKEN_PROVINCE_EDI = 0x5536;
constexpr token_t TOKEN_PROVINCE_GRE = 0x5537;
constexpr token_t TOKEN_PROVINCE_HOL = 0x5538;
constexpr token_t TOKEN_PROVINCE_KIE = 0x5539;
constexpr token_t TOKEN_PROVINCE_LON = 0x553A;
constexpr token_t TOKEN_PROVINCE_LVP = 0x553B;
constexpr token_t TOKEN_PROVINCE_MAR = 0x553C;
constexpr token_t TOKEN_PROVINCE_NAP = 0x553D;
constexpr token_t TOKEN_PROVINCE_NWY = 0x553E;
constexpr token_t TOKEN_PROVINCE_POR = 0x553F;
constexpr token_t TOKEN_PROVINCE_ROM = 0x5540;
constexpr token_t TOKEN_PROVINCE_RUM = 0x5541;
constexpr token_t TOKEN_PROVINCE_SEV = 0x5542;
constexpr token_t TOKEN_PROVINCE_SMY = 0x5543;
constexpr token_t TOKEN_PROVINCE_SWE = 0x5544;
constexpr token_t TOKEN_PROVINCE_TRI = 0x5545;
constexpr token_t TOKEN_PROVINCE_TUN = 0x5546;
constexpr token_t TOKEN_PROVINCE_VEN = 0x5547;
constexpr token_t TOKEN_PROVINCE_BUL = 0x5748;
constexpr token_t TOKEN_PROVINCE_SPA = 0x5749;
constexpr token_t TOKEN_PROVINCE_STP = 0x574A;
}
#endif
