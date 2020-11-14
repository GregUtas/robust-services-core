//==============================================================================
//
//  TokenTextMap.cpp
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
#include "TokenTextMap.h"
#include <utility>
#include "Debug.h"
#include "SysTypes.h"
#include "Token.h"

using std::string;
using namespace NodeBase;

//-----------------------------------------------------------------------------

namespace Diplomacy
{
TokenTextMap::TokenTextMap()
{
   Debug::ft("TokenTextMap.ctor");

   //  Create mappings between each token and its text representation.
   //
   insert(TOKEN_OPEN_BRACKET, "(");
   insert(TOKEN_CLOSE_BRACKET, ")");

   insert(TOKEN_POWER_AUS, "AUS");
   insert(TOKEN_POWER_ENG, "ENG");
   insert(TOKEN_POWER_FRA, "FRA");
   insert(TOKEN_POWER_GER, "GER");
   insert(TOKEN_POWER_ITA, "ITA");
   insert(TOKEN_POWER_RUS, "RUS");
   insert(TOKEN_POWER_TUR, "TUR");

   insert(TOKEN_UNIT_AMY, "AMY");
   insert(TOKEN_UNIT_FLT, "FLT");

   insert(TOKEN_ORDER_CTO, "CTO");
   insert(TOKEN_ORDER_CVY, "CVY");
   insert(TOKEN_ORDER_HLD, "HLD");
   insert(TOKEN_ORDER_MTO, "MTO");
   insert(TOKEN_ORDER_SUP, "SUP");
   insert(TOKEN_ORDER_VIA, "VIA");
   insert(TOKEN_ORDER_DSB, "DSB");
   insert(TOKEN_ORDER_RTO, "RTO");
   insert(TOKEN_ORDER_BLD, "BLD");
   insert(TOKEN_ORDER_REM, "REM");
   insert(TOKEN_ORDER_WVE, "WVE");

   insert(TOKEN_ORDER_NOTE_MBV, "MBV");
   insert(TOKEN_ORDER_NOTE_BPR, "BPR");
   insert(TOKEN_ORDER_NOTE_CST, "CST");
   insert(TOKEN_ORDER_NOTE_ESC, "ESC");
   insert(TOKEN_ORDER_NOTE_FAR, "FAR");
   insert(TOKEN_ORDER_NOTE_HSC, "HSC");
   insert(TOKEN_ORDER_NOTE_NAS, "NAS");
   insert(TOKEN_ORDER_NOTE_NMB, "NMB");
   insert(TOKEN_ORDER_NOTE_NMR, "NMR");
   insert(TOKEN_ORDER_NOTE_NRN, "NRN");
   insert(TOKEN_ORDER_NOTE_NRS, "NRS");
   insert(TOKEN_ORDER_NOTE_NSA, "NSA");
   insert(TOKEN_ORDER_NOTE_NSC, "NSC");
   insert(TOKEN_ORDER_NOTE_NSF, "NSF");
   insert(TOKEN_ORDER_NOTE_NSP, "NSP");
   insert(TOKEN_ORDER_NOTE_NSU, "NSU");
   insert(TOKEN_ORDER_NOTE_NVR, "NVR");
   insert(TOKEN_ORDER_NOTE_NYU, "NYU");
   insert(TOKEN_ORDER_NOTE_YSC, "YSC");

   insert(TOKEN_RESULT_SUC, "SUC");
   insert(TOKEN_RESULT_BNC, "BNC");
   insert(TOKEN_RESULT_CUT, "CUT");
   insert(TOKEN_RESULT_DSR, "DSR");
   insert(TOKEN_RESULT_FLD, "FLD");
   insert(TOKEN_RESULT_NSO, "NSO");
   insert(TOKEN_RESULT_RET, "RET");

   insert(TOKEN_COAST_NCS, "NCS");
   insert(TOKEN_COAST_NEC, "NEC");
   insert(TOKEN_COAST_ECS, "ECS");
   insert(TOKEN_COAST_SEC, "SEC");
   insert(TOKEN_COAST_SCS, "SCS");
   insert(TOKEN_COAST_SWC, "SWC");
   insert(TOKEN_COAST_WCS, "WCS");
   insert(TOKEN_COAST_NWC, "NWC");

   insert(TOKEN_SEASON_SPR, "SPR");
   insert(TOKEN_SEASON_SUM, "SUM");
   insert(TOKEN_SEASON_FAL, "FAL");
   insert(TOKEN_SEASON_AUT, "AUT");
   insert(TOKEN_SEASON_WIN, "WIN");

   insert(TOKEN_COMMAND_CCD, "CCD");
   insert(TOKEN_COMMAND_DRW, "DRW");
   insert(TOKEN_COMMAND_FRM, "FRM");
   insert(TOKEN_COMMAND_GOF, "GOF");
   insert(TOKEN_COMMAND_HLO, "HLO");
   insert(TOKEN_COMMAND_HST, "HST");
   insert(TOKEN_COMMAND_HUH, "HUH");
   insert(TOKEN_COMMAND_IAM, "IAM");
   insert(TOKEN_COMMAND_LOD, "LOD");
   insert(TOKEN_COMMAND_MAP, "MAP");
   insert(TOKEN_COMMAND_MDF, "MDF");
   insert(TOKEN_COMMAND_MIS, "MIS");
   insert(TOKEN_COMMAND_NME, "NME");
   insert(TOKEN_COMMAND_NOT, "NOT");
   insert(TOKEN_COMMAND_NOW, "NOW");
   insert(TOKEN_COMMAND_OBS, "OBS");
   insert(TOKEN_COMMAND_OFF, "OFF");
   insert(TOKEN_COMMAND_ORD, "ORD");
   insert(TOKEN_COMMAND_OUT, "OUT");
   insert(TOKEN_COMMAND_PRN, "PRN");
   insert(TOKEN_COMMAND_REJ, "REJ");
   insert(TOKEN_COMMAND_SCO, "SCO");
   insert(TOKEN_COMMAND_SLO, "SLO");
   insert(TOKEN_COMMAND_SND, "SND");
   insert(TOKEN_COMMAND_SUB, "SUB");
   insert(TOKEN_COMMAND_SVE, "SVE");
   insert(TOKEN_COMMAND_THX, "THX");
   insert(TOKEN_COMMAND_TME, "TME");
   insert(TOKEN_COMMAND_YES, "YES");
   insert(TOKEN_COMMAND_ADM, "ADM");
   insert(TOKEN_COMMAND_SMR, "SMR");

   insert(TOKEN_PARAMETER_AOA, "AOA");
   insert(TOKEN_PARAMETER_BTL, "BTL");
   insert(TOKEN_PARAMETER_ERR, "ERR");
   insert(TOKEN_PARAMETER_LVL, "LVL");
   insert(TOKEN_PARAMETER_MRT, "MRT");
   insert(TOKEN_PARAMETER_MTL, "MTL");
   insert(TOKEN_PARAMETER_NPB, "NPB");
   insert(TOKEN_PARAMETER_NPR, "NPR");
   insert(TOKEN_PARAMETER_PDA, "PDA");
   insert(TOKEN_PARAMETER_PTL, "PTL");
   insert(TOKEN_PARAMETER_RTL, "RTL");
   insert(TOKEN_PARAMETER_UNO, "UNO");
   insert(TOKEN_PARAMETER_DSD, "DSD");

   insert(TOKEN_PRESS_ALY, "ALY");
   insert(TOKEN_PRESS_AND, "AND");
   insert(TOKEN_PRESS_BWX, "BWX");
   insert(TOKEN_PRESS_DMZ, "DMZ");
   insert(TOKEN_PRESS_ELS, "ELS");
   insert(TOKEN_PRESS_EXP, "EXP");
   insert(TOKEN_PRESS_FCT, "FCT");
   insert(TOKEN_PRESS_FOR, "FOR");
   insert(TOKEN_PRESS_FWD, "FWD");
   insert(TOKEN_PRESS_HOW, "HOW");
   insert(TOKEN_PRESS_IDK, "IDK");
   insert(TOKEN_PRESS_IFF, "IFF");
   insert(TOKEN_PRESS_INS, "INS");
   insert(TOKEN_PRESS_OCC, "OCC");
   insert(TOKEN_PRESS_ORR, "ORR");
   insert(TOKEN_PRESS_PCE, "PCE");
   insert(TOKEN_PRESS_POB, "POB");
   insert(TOKEN_PRESS_PRP, "PRP");
   insert(TOKEN_PRESS_QRY, "QRY");
   insert(TOKEN_PRESS_SCD, "SCD");
   insert(TOKEN_PRESS_SRY, "SRY");
   insert(TOKEN_PRESS_SUG, "SUG");
   insert(TOKEN_PRESS_THK, "THK");
   insert(TOKEN_PRESS_THN, "THN");
   insert(TOKEN_PRESS_TRY, "TRY");
   insert(TOKEN_PRESS_VSS, "VSS");
   insert(TOKEN_PRESS_WHT, "WHT");
   insert(TOKEN_PRESS_WHY, "WHY");
   insert(TOKEN_PRESS_XDO, "XDO");
   insert(TOKEN_PRESS_XOY, "XOY");
   insert(TOKEN_PRESS_YDO, "YDO");
   insert(TOKEN_PRESS_CHO, "CHO");
   insert(TOKEN_PRESS_BCC, "BCC");
   insert(TOKEN_PRESS_UNT, "UNT");

   insert(TOKEN_PROVINCE_BOH, "BOH");
   insert(TOKEN_PROVINCE_BUR, "BUR");
   insert(TOKEN_PROVINCE_GAL, "GAL");
   insert(TOKEN_PROVINCE_RUH, "RUH");
   insert(TOKEN_PROVINCE_SIL, "SIL");
   insert(TOKEN_PROVINCE_TYR, "TYR");
   insert(TOKEN_PROVINCE_UKR, "UKR");
   insert(TOKEN_PROVINCE_BUD, "BUD");
   insert(TOKEN_PROVINCE_MOS, "MOS");
   insert(TOKEN_PROVINCE_MUN, "MUN");
   insert(TOKEN_PROVINCE_PAR, "PAR");
   insert(TOKEN_PROVINCE_SER, "SER");
   insert(TOKEN_PROVINCE_VIE, "VIE");
   insert(TOKEN_PROVINCE_WAR, "WAR");
   insert(TOKEN_PROVINCE_ADR, "ADR");
   insert(TOKEN_PROVINCE_AEG, "AEG");
   insert(TOKEN_PROVINCE_BAL, "BAL");
   insert(TOKEN_PROVINCE_BAR, "BAR");
   insert(TOKEN_PROVINCE_BLA, "BLA");
   insert(TOKEN_PROVINCE_EAS, "EAS");
   insert(TOKEN_PROVINCE_ECH, "ECH");
   insert(TOKEN_PROVINCE_GOB, "GOB");
   insert(TOKEN_PROVINCE_GOL, "GOL");
   insert(TOKEN_PROVINCE_HEL, "HEL");
   insert(TOKEN_PROVINCE_ION, "ION");
   insert(TOKEN_PROVINCE_IRI, "IRI");
   insert(TOKEN_PROVINCE_MAO, "MAO");
   insert(TOKEN_PROVINCE_NAO, "NAO");
   insert(TOKEN_PROVINCE_NTH, "NTH");
   insert(TOKEN_PROVINCE_NWG, "NWG");
   insert(TOKEN_PROVINCE_SKA, "SKA");
   insert(TOKEN_PROVINCE_TYS, "TYS");
   insert(TOKEN_PROVINCE_WES, "WES");
   insert(TOKEN_PROVINCE_ALB, "ALB");
   insert(TOKEN_PROVINCE_APU, "APU");
   insert(TOKEN_PROVINCE_ARM, "ARM");
   insert(TOKEN_PROVINCE_CLY, "CLY");
   insert(TOKEN_PROVINCE_FIN, "FIN");
   insert(TOKEN_PROVINCE_GAS, "GAS");
   insert(TOKEN_PROVINCE_LVN, "LVN");
   insert(TOKEN_PROVINCE_NAF, "NAF");
   insert(TOKEN_PROVINCE_PIC, "PIC");
   insert(TOKEN_PROVINCE_PIE, "PIE");
   insert(TOKEN_PROVINCE_PRU, "PRU");
   insert(TOKEN_PROVINCE_SYR, "SYR");
   insert(TOKEN_PROVINCE_TUS, "TUS");
   insert(TOKEN_PROVINCE_WAL, "WAL");
   insert(TOKEN_PROVINCE_YOR, "YOR");
   insert(TOKEN_PROVINCE_ANK, "ANK");
   insert(TOKEN_PROVINCE_BEL, "BEL");
   insert(TOKEN_PROVINCE_BER, "BER");
   insert(TOKEN_PROVINCE_BRE, "BRE");
   insert(TOKEN_PROVINCE_CON, "CON");
   insert(TOKEN_PROVINCE_DEN, "DEN");
   insert(TOKEN_PROVINCE_EDI, "EDI");
   insert(TOKEN_PROVINCE_GRE, "GRE");
   insert(TOKEN_PROVINCE_HOL, "HOL");
   insert(TOKEN_PROVINCE_KIE, "KIE");
   insert(TOKEN_PROVINCE_LON, "LON");
   insert(TOKEN_PROVINCE_LVP, "LVP");
   insert(TOKEN_PROVINCE_MAR, "MAR");
   insert(TOKEN_PROVINCE_NAP, "NAP");
   insert(TOKEN_PROVINCE_NWY, "NWY");
   insert(TOKEN_PROVINCE_POR, "POR");
   insert(TOKEN_PROVINCE_ROM, "ROM");
   insert(TOKEN_PROVINCE_RUM, "RUM");
   insert(TOKEN_PROVINCE_SEV, "SEV");
   insert(TOKEN_PROVINCE_SMY, "SMY");
   insert(TOKEN_PROVINCE_SWE, "SWE");
   insert(TOKEN_PROVINCE_TRI, "TRI");
   insert(TOKEN_PROVINCE_TUN, "TUN");
   insert(TOKEN_PROVINCE_VEN, "VEN");
   insert(TOKEN_PROVINCE_BUL, "BUL");
   insert(TOKEN_PROVINCE_SPA, "SPA");
   insert(TOKEN_PROVINCE_STP, "STP");
}

//-----------------------------------------------------------------------------

void TokenTextMap::erase(category_t cat)
{
   Debug::ft("TokenTextMap.erase");

   token_t curr_category = cat << 8;
   token_t next_category = (cat + 1) << 8;

   //  Erase each token and string in CAT, and also erase the reverse mapping
   //  (string to token).
   //
   auto it = token_to_text_map_.lower_bound(curr_category);

   while((it != token_to_text_map_.end()) && (it->first < next_category))
   {
      text_to_token_map_.erase(it->second);
      it = token_to_text_map_.erase(it);
   }
}

//-----------------------------------------------------------------------------

void TokenTextMap::erase_powers_and_provinces()
{
   Debug::ft("TokenTextMap.erase_powers_and_provinces");

   erase(CATEGORY_POWER);

   for(auto cat = CATEGORY_PROVINCE_MIN; cat <= CATEGORY_PROVINCE_MAX; ++cat)
   {
      erase(cat);
   }
}

//-----------------------------------------------------------------------------

fn_name TokenTextMap_insert = "TokenTextMap.insert";

bool TokenTextMap::insert(const Token& token, const string& text)
{
   Debug::ft(TokenTextMap_insert);

   //  It is an error to insert the same token twice or to reuse an
   //  existing string for a different token.
   //
   if(token_to_text_map_.find(token) != token_to_text_map_.end())
   {
      Debug::SwLog(TokenTextMap_insert, "token already exists", token.all());
      return false;
   }

   if(text_to_token_map_.find(text) != text_to_token_map_.end())
   {
      string expl = "string already in use: " + text;
      Debug::SwLog(TokenTextMap_insert, expl, 1);
      return false;
   }

   token_to_text_map_[token] = text;
   text_to_token_map_[text] = token;
   return true;
}

//-----------------------------------------------------------------------------

TokenTextMap* TokenTextMap::instance()
{
   static TokenTextMap instance_;

   return &instance_;
}
}
