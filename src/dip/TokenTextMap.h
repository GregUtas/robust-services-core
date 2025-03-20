//==============================================================================
//
//  TokenTextMap.h
//
//  Diplomacy AI Client - Part of the DAIDE project (www.daide.org.uk).
//
//  (C) David Norman 2002 david@ellought.demon.co.uk
//  (C) Greg Utas 2019-2025 greg@pentennea.com
//
//  This software may be reused for non-commercial purposes without charge,
//  and without notifying the authors.  Use of any part of this software for
//  commercial purposes without permission from the authors is prohibited.
//
#ifndef TOKENTEXTMAP_H_INCLUDED
#define TOKENTEXTMAP_H_INCLUDED

#include <map>
#include <string>
#include "DipTypes.h"

namespace Diplomacy
{
   class Token;
}

//------------------------------------------------------------------------------

namespace Diplomacy
{
//  Provides mappings between raw token values and their text representations.
//
class TokenTextMap
{
public:
   //  Types for the maps.
   //
   typedef std::map<Token, std::string> TokenToTextMap;
   typedef std::map<std::string, Token> TextToTokenMap;

   //  Returns the singleton instance of this class.
   //
   static TokenTextMap* instance();

   //  Provides read-only access to the token-to-text map.
   //
   const TokenToTextMap& token_to_text_map()
      const { return token_to_text_map_; }

   //  Provides read-only access to the text-to-token map.
   //
   const TextToTokenMap& text_to_token_map()
      const { return text_to_token_map_; }

   //  Removes all power and province tokens from the maps.
   //
   void erase_powers_and_provinces();

   //  Creates a mapping between TOKEN and TEXT.  Returns true on success.
   //  Returns false and generates a log if TOKEN or TEXT has already been
   //  inserted.
   //
   bool insert(const Token& token, const std::string& text);
private:
   //  Constructor.  Calls insert() to create a mapping between every token
   //  and its text representation.
   //
   TokenTextMap();

   //  Removes all tokens in CAT from the maps.
   //
   void erase(category_t cat);

   //  The mapping from each token to its text representation.
   //
   TokenToTextMap token_to_text_map_;

   //  The mapping from each string to the token it represents.
   //
   TextToTokenMap text_to_token_map_;
};
}
#endif
