//==============================================================================
//
//  CxxString.h
//
//  Copyright (C) 2017  Greg Utas
//
//  This file is part of the Robust Services Core (RSC).
//
//  RSC is free software: you can redistribute it and/or modify it under the
//  terms of the GNU General Public License as published by the Free Software
//  Foundation, either version 3 of the License, or (at your option) any later
//  version.
//
//  RSC is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
//  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
//  details.
//
//  You should have received a copy of the GNU General Public License along
//  with RSC.  If not, see <http://www.gnu.org/licenses/>.
//
#ifndef CXXSTRING_H_INCLUDED
#define CXXSTRING_H_INCLUDED

#include <cstddef>
#include <string>
#include <vector>
#include "CodeTypes.h"

//------------------------------------------------------------------------------

namespace CodeTools
{
   //  For lists of strings.
   //
   typedef std::vector< std::string > stringVector;

   //  Returns the last string that follows a '.' in FILE.  Returns an empty
   //  strng if FILE contains no '.'
   //
   std::string GetFileExtension(const std::string& file);

   //  Returns the filename in PATH.  If PATH contains a forward or backward
   //  slash, the filename that follows it is extracted.  Any extension is
   //  retained.
   //
   std::string GetFileName(const std::string& path);

   //  Returns true if DIR appears in PATH.  Prefixes a '/' to DIR before
   //  searching PATH.
   //
   bool PathIncludes(const std::string& path, const std::string& dir);

   //  Returns true if FILE is a code file.
   //
   bool IsCodeFile(const std::string& file);

   //  Returns the index of the string in SV that matches S.  If no string
   //  in SV matches S, returns string::npos.
   //
   size_t FindIndex(const stringVector& sv, const std::string& s);

   //  Returns true if ID, in its entirety, is a valid identifier.
   //
   bool IsValidIdentifier(const std::string& id);

   //  Returns S after converting endlines to spaces and compressing adjacent
   //  spaces.
   //
   std::string Compress(const std::string& s);

   //  Concatentates a string of the form ("<string>"<whitespace>)*"<string>"
   //  by removing the quotation marks and whitespace between the strings.
   //  The quotation marks originally at the beginning and end of the string
   //  must not be included in S.
   //
   void Concatenate(std::string& s);

   //  Appends a scope resolution operator to SCOPE unless it is empty, and
   //  returns the resulting string.
   //
   std::string& Prefix(std::string& scope);
   std::string& Prefix(std::string&& scope);

   //  Modifies NAME by stripping off a scope resolution operator and whatever
   //  precedes it.  Does the same to any template arguments embedded in NAME.
   //  Returns the resulting string.
   //
   std::string Normalize(const std::string& name);

   //  Returns the location at which NAME matches a rear substring of fqName,
   //  in which case NAME could refer to fqName.  Returns string::npos if NAME
   //  cannot refer to fqName.
   //
   size_t NameCouldReferTo(const std::string& fqName, const std::string& name);

   //  Returns the last location where fqSuper matches a front substring of
   //  fqSub, in which case fqSuper is a superscope of fqSub.  TMPLT is set if
   //  a template should be considered a superscope of one of its instances.
   //  Returns string::npos if fqSuper cannot be a superscope of fqSub.
   //
   size_t CompareScopes
      (const std::string& fqSub, const std::string& fqSuper, bool tmplt);

   //  Updates TYPE based on PTRS.  If PTRS is 0, S is unchanged.  If PTRS is
   //  positive, that number of asterisks are appended to TYPE.  If PTRS is
   //  negative, that number of asterisks are removed from TYPE.  If TYPE has
   //  fewer than PTRS asterisks, a '@' is added for each "negative" pointer.
   //
   void AdjustPtrs(std::string& type, TagCount ptrs);

   //  Removes tags from TYPE (excluding any tags in template types).  This
   //  includes occurrences of "const", '*', and '&'.
   //
   std::string& RemoveTags(std::string& type);

   //  Removes reference tags from TYPE and returns the result.
   //
   std::string& RemoveRefs(std::string& type);

   //  Removes const qualifications from TYPE (excluding template types) and
   //  returns the result.
   //
   std::string RemoveConsts(const std::string& type);

   //  Removes all template parameters or arguments from TYPE and returns the
   //  result.
   //
   std::string& RemoveTemplates(std::string&& type);

   //  Returns the starting location of TARG within S.  Skips any TARG that
   //  appears after a // comment or within a string literal.
   //
   size_t FindSubstr(const std::string& s, const std::string& targ);

   //  Between positions BEGIN and END - 1 in CODE, replaces occurrences of S1
   //  with S2.  Returns the new location of END, accounting for replacements
   //  of S1 by S2.
   //
   size_t Replace(std::string& code,
      const std::string& s1, const std::string& s2, size_t begin, size_t end);
}
#endif
