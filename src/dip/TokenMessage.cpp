//==============================================================================
//
//  TokenMessage.cpp
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
#include "TokenMessage.h"
#include <cctype>
#include <iosfwd>
#include <map>
#include <sstream>
#include <utility>
#include "BaseBot.h"
#include "Debug.h"
#include "SysTypes.h"
#include "Token.h"
#include "TokenTextMap.h"

using std::string;
using namespace NodeBase;

//------------------------------------------------------------------------------

namespace Diplomacy
{
//  This is similar to memcpy but exists to eliminate compiler warnings
//  about memcpy-ing a class (Token) whose copy operator isn't trivial.
//
static void copy_tokens(Token* dest, const Token* source, int num)
{
   if(source < dest)
   {
      for(auto i = num - 1; i >= 0; --i) dest[i] = source[i];
   }
   else
   {
      for(auto i = 0; i < num; ++i) dest[i] = source[i];
   }
}

//------------------------------------------------------------------------------

TokenMessage::TokenMessage() :
   length_(0),
   parm_count_(0)
{
   Debug::ft("TokenMessage.ctor");
}

//------------------------------------------------------------------------------

TokenMessage::TokenMessage(token_t raw) :
   length_(0),
   parm_count_(0)
{
   Debug::ft("TokenMessage.ctor(token_t)");

   const Token token = Token(raw);
   set_from(&token, 1);
}

//------------------------------------------------------------------------------

TokenMessage::TokenMessage(const Token& token) :
   length_(0),
   parm_count_(0)
{
   Debug::ft("TokenMessage.ctor(token)");

   set_from(&token, 1);
}

//------------------------------------------------------------------------------

TokenMessage::TokenMessage(const Token* stream) :
   length_(0),
   parm_count_(0)
{
   Debug::ft("TokenMessage.ctor(message)");

   set_from(stream);
}

//------------------------------------------------------------------------------

TokenMessage::TokenMessage(const Token* stream, size_t length) :
   length_(0),
   parm_count_(0)
{
   Debug::ft("TokenMessage.ctor(stream)");

   set_from(stream, length);
}

//------------------------------------------------------------------------------

TokenMessage::~TokenMessage()
{
   Debug::ftnt("TokenMessage.dtor");

   clear();
}

//------------------------------------------------------------------------------

TokenMessage::TokenMessage(const TokenMessage& that) :
   length_(0),
   parm_count_(0)
{
   Debug::ft("TokenMessage.ctor(copy)");

   if(that.message_ != nullptr)
   {
      set_from(that.message_.get(), that.length_);
   }
}

//------------------------------------------------------------------------------

TokenMessage& TokenMessage::operator=(const TokenMessage& that)
{
   Debug::ft("TokenMessage.operator=(copy)");

   if(this == &that) return *this;

   clear();

   if(that.message_ != nullptr)
   {
      set_from(that.message_.get(), that.length_);
   }

   return *this;
}

//------------------------------------------------------------------------------

TokenMessage& TokenMessage::operator=(TokenMessage&& that)
{
   Debug::ft("TokenMessage.operator=(move)");

   if(this == &that) return *this;

   this->length_ = that.length_;
   this->message_ = std::move(that.message_);
   this->parm_count_ = that.parm_count_;
   this->parm_begins_ = std::move(that.parm_begins_);

   return *this;
}

//------------------------------------------------------------------------------

Token TokenMessage::at(size_t index) const
{
   return (index < length_ ? message_[index] : TOKEN_END_OF_MESSAGE);
}

//------------------------------------------------------------------------------

void TokenMessage::clear()
{
   length_ = 0;
   parm_count_ = 0;
   message_.reset();
   parm_begins_.reset();
}

//------------------------------------------------------------------------------

TokenMessage TokenMessage::enclose() const
{
   Debug::ft("TokenMessage.enclose");

   TokenMessage combined;

   if(message_ == nullptr)
   {
      //  Create an empty message surrounded by parentheses.
      //
      combined.message_ = TokensPtr(new Token[3]);
      combined.message_[0] = TOKEN_OPEN_BRACKET;
      combined.message_[1] = TOKEN_CLOSE_BRACKET;
      combined.message_[2] = TOKEN_END_OF_MESSAGE;
      combined.length_ = 2;
      combined.parm_count_ = 1;
   }
   else
   {
      //  Create a message that encloses this one in parentheses.
      //
      combined.message_ = TokensPtr(new Token[length_ + 3]);
      copy_tokens(&combined.message_[1], message_.get(), length_);
      combined.message_[0] = TOKEN_OPEN_BRACKET;
      combined.message_[length_ + 1] = TOKEN_CLOSE_BRACKET;
      combined.message_[length_ + 2] = TOKEN_END_OF_MESSAGE;
      combined.length_ = length_ + 2;
      combined.parm_count_ = 1;
   }

   return combined;
}

//------------------------------------------------------------------------------

void TokenMessage::enclose_this()
{
   Debug::ft("TokenMessage.enclose_this");

   if(message_ == nullptr)
   {
      //  Create an empty message.
      //
      message_ = TokensPtr(new Token[3]);
      message_[0] = TOKEN_OPEN_BRACKET;
      message_[1] = TOKEN_CLOSE_BRACKET;
      message_[2] = TOKEN_END_OF_MESSAGE;
      length_ = 2;
      parm_count_ = 1;

      if(parm_begins_ != nullptr)
      {
         parm_begins_[0] = 0;
         parm_begins_[1] = 2;
      }
   }
   else
   {
      TokensPtr combined(new Token[length_ + 3]);
      copy_tokens(&combined[1], message_.get(), length_);
      combined[0] = TOKEN_OPEN_BRACKET;
      combined[length_ + 1] = TOKEN_CLOSE_BRACKET;
      combined[length_ + 2] = TOKEN_END_OF_MESSAGE;

      //  Replace this message with the new one.
      //
      clear();
      message_.reset(combined.release());
      length_ = length_ + 2;
      parm_count_ = 1;
   }
}

//------------------------------------------------------------------------------

fn_name TokenMessage_find_parms = "TokenMessage.find_parms";

void TokenMessage::find_parms() const
{
   Debug::ft(TokenMessage_find_parms);

   if(parm_begins_ != nullptr) return;  // already found

   int nesting = 0;
   size_t parm_index = 0;

   parm_begins_ = ParmBeginsPtr(new size_t[parm_count_ + 1]);

   for(size_t index = 0; index < length_; ++index)
   {
      if(nesting == 0)
      {
         parm_begins_[parm_index] = index;
         ++parm_index;
      }

      if(message_[index] == TOKEN_OPEN_BRACKET)
      {
         ++nesting;
      }
      else if(message_[index] == TOKEN_CLOSE_BRACKET)
      {
         --nesting;
      }
   }

   if(parm_index != parm_count_)
   {
      Debug::SwLog
         (TokenMessage_find_parms, "invalid count", parm_index - parm_count_);
   }

   parm_begins_[parm_count_] = length_;
}

//------------------------------------------------------------------------------

Token TokenMessage::front() const
{
   return (length_ == 0 ? TOKEN_END_OF_MESSAGE : message_[0]);
}

//------------------------------------------------------------------------------

TokenMessage TokenMessage::get_parm(size_t n) const
{
   Debug::ft("TokenMessage.get_parm");

   TokenMessage parm;

   if(message_ == nullptr) return parm;

   find_parms();

   if(n < parm_count_)
   {
      auto length = parm_begins_[n + 1] - parm_begins_[n];

      //  If the parameter is a single token, just copy it.  If it's
      //  longer, copy it but omit the outer parentheses.
      //
      if(length == 1)
         parm.set_from(&message_[parm_begins_[n]], 1);
      else
         parm.set_from(&message_[parm_begins_[n] + 1], length - 2);
   }

   return parm;
}

//------------------------------------------------------------------------------

bool TokenMessage::get_tokens(Token tokens[], size_t max) const
{
   Debug::ft("TokenMessage.get_tokens");

   if(message_ == nullptr) return false;
   if(max < length_) return false;
   copy_tokens(tokens, message_.get(), length_ + 1);
   return true;
}

//------------------------------------------------------------------------------

void TokenMessage::log(const string& expl) const
{
   Debug::ft("TokenMessage.log");

   std::ostringstream stream;
   stream << expl << CRLF;
   stream << to_str() << CRLF;
   BaseBot::send_to_console(stream);
}

//------------------------------------------------------------------------------

Token TokenMessage::operator[](size_t index) const
{
   return (index < length_ ? message_[index] : TOKEN_END_OF_MESSAGE);
}

//------------------------------------------------------------------------------

TokenMessage TokenMessage::operator+(const Token& token)
{
   Debug::ft("TokenMessage.operator+(token)");

   TokenMessage message(token);
   return operator+(message);
}

//------------------------------------------------------------------------------

TokenMessage TokenMessage::operator+(const TokenMessage& that)
{
   Debug::ft("TokenMessage.operator+(message)");

   TokenMessage combined;

   if(message_ == nullptr)
   {
      combined = that;
   }
   else if(that.message_ == nullptr)
   {
      combined = *this;
   }
   else
   {
      combined.message_ = TokensPtr(new Token[length_ + that.length_ + 1]);
      copy_tokens(combined.message_.get(), message_.get(), length_);
      copy_tokens
         (&combined.message_[length_], that.message_.get(), that.length_);
      combined.message_[length_ + that.length_] = TOKEN_END_OF_MESSAGE;
      combined.length_ = length_ + that.length_;
      combined.parm_count_ = parm_count_ + that.parm_count_;
   }

   return combined;
}

//------------------------------------------------------------------------------

TokenMessage TokenMessage::operator&(const Token& token)
{
   Debug::ft("TokenMessage.operator&(token)");

   TokenMessage message(token);
   return operator&(message);
}

//------------------------------------------------------------------------------

TokenMessage TokenMessage::operator&(const TokenMessage& that)
{
   Debug::ft("TokenMessage.operator&(message)");

   TokenMessage combined;

   if(message_ == nullptr)
   {
      combined = that.enclose();
   }
   else if(that.message_ == nullptr)
   {
      combined = *this + that.enclose();
   }
   else
   {
      combined.message_ = TokensPtr(new Token[length_ + that.length_ + 3]);
      copy_tokens(combined.message_.get(), message_.get(), length_);
      copy_tokens
         (&combined.message_[length_ + 1], that.message_.get(), that.length_);
      combined.message_[length_] = TOKEN_OPEN_BRACKET;
      combined.message_[length_ + that.length_ + 1] = TOKEN_CLOSE_BRACKET;
      combined.message_[length_ + that.length_ + 2] = TOKEN_END_OF_MESSAGE;
      combined.length_ = length_ + that.length_ + 2;
      combined.parm_count_ = parm_count_ + 1;
   }

   return combined;
}

//------------------------------------------------------------------------------

bool TokenMessage::operator==(const TokenMessage& that) const
{
   if(length_ != that.length_)
   {
      return true;
   }

   for(size_t index = 0; index < length_; ++index)
   {
      if(message_[index] != that.message_[index])
      {
         return false;
      }
   }

   return true;
}

//------------------------------------------------------------------------------

bool TokenMessage::operator!=(const TokenMessage& that) const
{
   return !(operator==(that));
}

//------------------------------------------------------------------------------

bool TokenMessage::operator<(const TokenMessage& that) const
{
   //  If the other message is empty, this one is not less than it.
   //
   if(that.length_ == 0) return false;

   //  If this message is empty, it is less than the other one.
   //
   if(length_ == 0) return true;

   //  Compare the messages until either one runs out of tokens.
   //
   size_t limit = (length_ < that.length_ ? length_ : that.length_);

   for(size_t index = 0; index < limit; ++index)
   {
      if(message_[index] < that.message_[index]) return true;
      if(that.message_[index] < message_[index]) return false;
   }

   //  The messages still match, but one (or both) of them ran out of tokens.
   //  If the other message still has tokens, this message is the lesser.
   //
   return (limit < that.length_);
}

//------------------------------------------------------------------------------

bool TokenMessage::parm_is_single_token(size_t n) const
{
   Debug::ft("TokenMessage.parm_is_single_token");

   if(n >= parm_count_) return false;
   find_parms();
   return ((parm_begins_[n + 1] - parm_begins_[n]) == 1);
}

//------------------------------------------------------------------------------

size_t TokenMessage::parm_start(size_t n) const
{
   Debug::ft("TokenMessage.parm_start");

   if(n >= parm_count_) return 0;
   find_parms();

   if(parm_begins_[n + 1] - parm_begins_[n] > 1)
   {
      return parm_begins_[n] + 1;  // skip left parenthesis
   }

   return parm_begins_[n];  // parameter is a single token
}

//------------------------------------------------------------------------------

void TokenMessage::set_as_ascii(const string& text)
{
   Debug::ft("TokenMessage.set_as_ascii");

   std::unique_ptr<Token[]> tokens(new Token[text.size()]);

   for(size_t index = 0; index < text.size(); ++index)
   {
      tokens[index] = Token(CATEGORY_ASCII, text[index]);
   }

   set_from(tokens.get(), text.size());
   tokens.reset();
}

//------------------------------------------------------------------------------

size_t TokenMessage::set_from(const Token* stream)
{
   Debug::ft("TokenMessage.set_from(stream)");

   clear();

   auto location = NO_ERROR;
   size_t index = 0;
   int nesting = 0;

   //  Run through MESSAGE, counting parameters and checking for balanced
   //  parentheses.
   //
   while((stream[index] != TOKEN_END_OF_MESSAGE) && (location == NO_ERROR))
   {
      if(nesting == 0)
      {
         ++parm_count_;
      }

      if(stream[index] == TOKEN_OPEN_BRACKET)
      {
         ++nesting;
      }
      else if(stream[index] == TOKEN_CLOSE_BRACKET)
      {
         if(--nesting < 0)
         {
            location = index;  // unmatched right parenthesis
         }
      }

      ++index;
   }

   if(nesting != 0)  // unbalanced '('
   {
      parm_count_ = 0;
      location = index;
   }
   else
   {
      message_ = TokensPtr(new Token[index + 1]);
      copy_tokens(message_.get(), stream, index);
      message_[index] = TOKEN_END_OF_MESSAGE;
      length_ = index;
   }

   return location;
}

//------------------------------------------------------------------------------

size_t TokenMessage::set_from(const Token* stream, size_t length)
{
   Debug::ft("TokenMessage.set_from(stream, length)");

   size_t location = NO_ERROR;
   int nesting = 0;

   clear();

   //  Run through MESSAGE, counting parameters and checking for balanced
   //  parentheses.
   //
   for(size_t index = 0; ((index < length) && (location == NO_ERROR)); ++index)
   {
      if(nesting == 0)
      {
         ++parm_count_;
      }

      if(stream[index] == TOKEN_OPEN_BRACKET)
      {
         ++nesting;
      }
      else if(stream[index] == TOKEN_CLOSE_BRACKET)
      {
         if(--nesting < 0)
         {
            location = index;  // unmatched right parenthesis
         }
      }
   }

   if(nesting != 0)  // unmatched left parenthesis
   {
      parm_count_ = 0;
      location = length;
   }
   else
   {
      message_ = TokensPtr(new Token[length + 1]);
      copy_tokens(message_.get(), stream, length);
      message_[length] = TOKEN_END_OF_MESSAGE;
      length_ = length;
   }

   return location;
}

//------------------------------------------------------------------------------

size_t TokenMessage::set_from(const string& text)
{
   Debug::ft("TokenMessage.set_from(text)");

   auto location = NO_ERROR;
   size_t text_index = 0;
   size_t token_index = 0;
   int nesting = 0;
   std::unique_ptr<Token[]> tokens(new Token[text.size()]);
   auto text_to_token_map = &TokenTextMap::instance()->text_to_token_map();

   while((location == NO_ERROR) && (text_index < text.size()))
   {
      if(text[text_index] == SPACE)
      {
         ++text_index;  // skip blanks
      }
      else if(text[text_index] == '(')
      {
         tokens[token_index] = TOKEN_OPEN_BRACKET;
         ++nesting;
         ++token_index;
         ++text_index;
      }
      else if(text[text_index] == ')')
      {
         tokens[token_index] = TOKEN_CLOSE_BRACKET;

         if(--nesting < 0)
         {
            location = text_index;  // unmatched right parenthesis
         }

         ++token_index;
         ++text_index;
      }
      else if(text[text_index] == APOSTROPHE)
      {
         ++text_index;

         if((token_index > 0) &&
            (tokens[token_index - 1].category() == CATEGORY_ASCII))
         {
            //  Double apostrophe.  Insert a single one into the message.
            //
            tokens[token_index] = Token(CATEGORY_ASCII, APOSTROPHE);
            ++token_index;
         }

         //  Copy the rest of the quoted string into the message.
         //
         while((text_index < text.size()) && (text[text_index] != APOSTROPHE))
         {
            tokens[token_index] = Token(CATEGORY_ASCII, text[text_index]);
            ++token_index;
            ++text_index;
         }

         if(text[text_index] != APOSTROPHE)
            location = text_index;  // unmatched single quote
         else
            ++text_index;
      }
      else if(isalpha(text[text_index]))
      {
         //  Each token has a three-letter text representation.
         //
         string token_string;
         token_string.push_back(text[text_index]);
         token_string.push_back(text[text_index + 1]);
         token_string.push_back(text[text_index + 2]);

         auto token_iterator = text_to_token_map->find(token_string);

         if(token_iterator == text_to_token_map->end())
         {
            location = text_index;  // undefined token
         }
         else
         {
            tokens[token_index] = token_iterator->second;
            ++token_index;
            text_index += 3;
         }
      }
      else if(isdigit(text[text_index]) || (text[text_index] == '-'))
      {
         auto is_negative = false;
         int token_value = 0;

         if(text[text_index] == '-')
         {
            is_negative = true;
            ++text_index;
         }

         while(isdigit(text[text_index]))
         {
            token_value = token_value * 10 + (text[text_index] - '0');
            ++text_index;
         }

         if(is_negative) token_value = -token_value;

         if(tokens[token_index].set_number(token_value))
            ++token_index;
         else
            location = text_index;
      }
      else
      {
         location = text_index;  // illegal character
      }
   }

   if(location == NO_ERROR)
   {
      if(nesting != 0)
      {
         location = text.length();  // unmatched left parenthesis
      }
      else
      {
         location = set_from(tokens.get(), token_index);

         if(location != NO_ERROR)
         {
            //  This should never occur: any error should have been found above.
            //  Set the error location to the end of the string.
            //
            location = text.size();
         }
      }
   }

   tokens.reset();
   return location;
}

//------------------------------------------------------------------------------

string TokenMessage::to_str() const
{
   std::ostringstream message_as_text;
   auto is_ascii = false;

   for(size_t index = 0; index < length_; ++index)
   {
      if(is_ascii && (message_[index].category() != CATEGORY_ASCII))
      {
         //  An ASCII string has ended.
         //
         message_as_text << APOSTROPHE;
         is_ascii = false;
      }

      if(!is_ascii && (message_[index].category() == CATEGORY_ASCII))
      {
         //  An ASCII string has started.
         //
         message_as_text << APOSTROPHE;
         is_ascii = true;
      }

      //  Add the token, followed by a blank unless it's an ASCII character.
      //
      message_as_text << message_[index].to_str();
      if(!is_ascii) message_as_text << SPACE;
   }

   if(is_ascii)
   {
      //  The message ended with an ASCII string, so append a quote.
      //
      message_as_text << APOSTROPHE;
   }

   return message_as_text.str();
}
}
