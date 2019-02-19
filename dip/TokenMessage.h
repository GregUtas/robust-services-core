//==============================================================================
//
//  TokenMessage.h
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
#ifndef TOKENMESSAGE_H_INCLUDED
#define TOKENMESSAGE_H_INCLUDED

#include <cstddef>
#include <memory>
#include <string>
#include "DipTypes.h"

namespace Diplomacy
{
   class Token;
}

//------------------------------------------------------------------------------

namespace Diplomacy
{
//  Provides a wrapper for a sequence of tokens enclosed in parentheses.
//
class TokenMessage
{
public:
   //  Constructs an empty message.
   //
   TokenMessage();

   //  Constructs a message containing a single token.
   //
   TokenMessage(token_t raw);
   TokenMessage(const Token& token);

   //  Constructs a message from a sequence of tokens (STREAM).
   //  STREAM must end with TOKEN_END_OF_MESSAGE.
   //
   explicit TokenMessage(const Token* stream);

   //  Construct a message from a stream of LENGTH tokens.
   //
   TokenMessage(const Token* stream, size_t length);

   //  Copy constructor.
   //
   TokenMessage(const TokenMessage& that);

   //  Destructor.  Invokes clear().
   //
   ~TokenMessage();

   //  Copy assignment operator.
   //
   TokenMessage& operator=(const TokenMessage& that);

   //  Copies the message's tokens into TOKENS, which can hold up to MAX
   //  tokens.  Returns FALSE if the message is empty or contains more
   //  tokens than TOKENS can hold.  Returns true on success, in which
   //  case the last entry in TOKENS will be TOKEN_END_OF_MESSAGE.
   //
   bool get_tokens(Token tokens[], size_t max) const;

   //  Returns TRUE if the message is empty.
   //
   bool empty() const { return (length_ == 0); }

   //  Returns the length of the message.
   //
   size_t size() const { return length_; }

   //  Returns true if the message is a single token.
   //
   bool is_single_token() const { return (length_ == 1); }

   //  Returns the first token.
   //
   Token front() const;

   //  Returns the token at [INDEX].
   //
   Token operator[](size_t index) const;
   Token at(size_t index) const;

   //  Returns true if the message contains any parameters that are enclosed
   //  in parentheses.
   //
   bool has_nested_parms() const { return (length_ != parm_count_); }

   //  Returns the number of parameter.  A parameter is a single token or
   //  a stream of tokens enclosed by parentheses.  A parameter can itself
   //  contain nested parameters.
   //
   size_t parm_count() const { return parm_count_; }

   //  Returns the Nth parameter as a message.
   //
   TokenMessage get_parm(size_t n) const;

   //  Returns the index into the stream of tokens where the Nth parameter
   //  starts, omitting its leading parenthesis.
   //
   size_t parm_start(size_t n) const;

   //  Returns true if the Nth parameter is a single token.
   //
   bool parm_is_single_token(size_t n) const;

   //  Copies the stream of TOKENS into the message.  STREAM must end with
   //  TOKEN_END_OF_MESSAGE.  Returns the offset of any error, or NO_ERROR
   //  on success.
   //
   size_t set_from(const Token* stream);

   //  Copies the stream of TOKENS into the message.  STREAM is truncated
   //  at LENGTH.  Returns the offset of any error, or NO_ERROR on success.
   //
   size_t set_from(const Token* stream, size_t length);

   //  Returns the message as a readable string.
   //
   std::string to_str() const;

   //  Sets the message by interpreting TEXT, which is of the form returned
   //  to_str().  Returns the offset of any error, or NO_ERROR on success.
   //
   size_t set_from(const std::string& text);

   //  Sets the message to the string of ASCII tokens in TEXT.
   //
   void set_as_ascii(const std::string& text);

   //  Creates a copy of the message, but enclosed in parentheses.
   //
   TokenMessage enclose() const;

   //  Modifies the message by enclosing it in parentheses.
   //
   void enclose_this();

   //  Clears the message and frees its resources.
   //
   void clear();

   //  Logs the message.  EXPL will be the log's title.
   //
   void log(const std::string& expl) const;

   //  The + operators perform straight concatenation (i.e. append).
   //  The & operators enclose THAT in parentheses before appending.
   //
   TokenMessage operator+(const Token& token);
   TokenMessage operator+(const TokenMessage& that);
   TokenMessage operator&(const Token& token);
   TokenMessage operator&(const TokenMessage& that);

   //  Compares this message to THAT.
   //
   bool operator==(const TokenMessage& that) const;
   bool operator!=(const TokenMessage& that) const;
   bool operator<(const TokenMessage& that) const;
private:
   //  Finds the message's parameters if they have not already been
   //  found.
   //
   void find_parms() const;

   //  For storing the message's contents.
   //
   typedef std::unique_ptr< Token[] > TokensPtr;

   //  For storing the starting location of each of the message's
   //  parameters.
   //
   typedef std::unique_ptr< size_t[] > ParmBeginsPtr;

   //  The number of tokens in the message.
   //
   size_t length_;

   //  The stream of tokens.
   //
   TokensPtr message_;

   //  The number of parameters in the message.
   //
   size_t parm_count_;

   //  The index into message_ where each parameter begins.
   //
   mutable ParmBeginsPtr parm_begins_;
};
}
#endif
