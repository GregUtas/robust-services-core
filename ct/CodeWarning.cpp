//==============================================================================
//
//  CodeWarning.cpp
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
#include "CodeWarning.h"
#include <algorithm>
#include <iomanip>
#include <iterator>
#include <sstream>
#include <vector>
#include "CodeFile.h"
#include "CodeFileSet.h"
#include "CxxArea.h"
#include "CxxRoot.h"
#include "Debug.h"
#include "Formatters.h"
#include "Lexer.h"
#include "Library.h"
#include "Registry.h"
#include "Singleton.h"
#include "ThisThread.h"

using namespace NodeBase;
using std::ostream;
using std::setw;
using std::string;

//------------------------------------------------------------------------------

namespace CodeTools
{
CodeWarning::AttrsMap CodeWarning::Attrs = AttrsMap();

//------------------------------------------------------------------------------

CodeWarning::CodeWarning
   (bool fix, uint8_t order, bool unused, fixed_string title) :
   fixable(fix),
   order(order),
   unusedItem(unused),
   suppressed(false),
   title(title)
{
}

//------------------------------------------------------------------------------

fn_name CodeWarning_Initialize = "CodeWarning.Initialize";

void CodeWarning::Initialize()
{
   Debug::ft(CodeWarning_Initialize);

   //  The following constants define how warnings will be sorted before the
   //  editor fixes them.  Fixes that require more significant editing are
   //  assigned higher numbers so that they will occur later.  For example,
   //  inserting or removing a line feed no longer makes it possible to find
   //  the location of a warning based on its line number.
   //
   const int U = 0;  // leaves code unchanged
   const int D = 2;  // deletes one or more lines
   const int C = 2;  // creates one or more lines
   const int E = 4;  // erases within a line
   const int R = 4;  // replaces within a line
   const int I = 4;  // inserts within a line
   const int J = 6;  // joins two lines by removing a line feed
   const int S = 6;  // splits two lines by inserting a line feed
   const int X = 8;  // editor does not support fixing this warning
   const auto F = false;
   const auto T = true;

   Attrs.insert(WarningPair(AllWarnings,
      CodeWarning(T, X, F,
      "All warnings")));
   Attrs.insert(WarningPair(UseOfNull,
      CodeWarning(T, R, F,
      "Use of NULL")));
   Attrs.insert(WarningPair(PtrTagDetached,
      CodeWarning(T, R, F,
      "Pointer tag ('*') detached from type")));
   Attrs.insert(WarningPair(RefTagDetached,
      CodeWarning(T, R, F,
      "Reference tag ('&') detached from type")));
   Attrs.insert(WarningPair(UseOfCast,
      CodeWarning(F, X, F,
      "C-style cast")));
   Attrs.insert(WarningPair(FunctionalCast,
      CodeWarning(F, X, F,
      "Functional cast")));
   Attrs.insert(WarningPair(ReinterpretCast,
      CodeWarning(F, X, F,
      "reinterpret_cast")));
   Attrs.insert(WarningPair(Downcasting,
      CodeWarning(F, X, F,
      "Cast down the inheritance hierarchy")));
   Attrs.insert(WarningPair(CastingAwayConstness,
      CodeWarning(F, X, F,
      "Cast removes const qualification")));
   Attrs.insert(WarningPair(PointerArithmetic,
      CodeWarning(F, X, F,
      "Pointer arithmetic")));
   Attrs.insert(WarningPair(RedundantSemicolon,
      CodeWarning(T, E, F,
      "Semicolon not required")));
   Attrs.insert(WarningPair(RedundantConst,
      CodeWarning(T, E, F,
      "Redundant const in type specification")));
   Attrs.insert(WarningPair(DefineNotAtFileScope,
      CodeWarning(F, X, F,
      "#define appears within a class or function")));
   Attrs.insert(WarningPair(IncludeNotAtGlobalScope,
      CodeWarning(F, X, F,
      "No #include guard found")));
   Attrs.insert(WarningPair(IncludeGuardMissing,
      CodeWarning(T, C, F,
      "#include appears outside of global namespace")));
   Attrs.insert(WarningPair(IncludeNotSorted,
      CodeWarning(T, U, F,
      "#include not sorted in standard order")));
   Attrs.insert(WarningPair(IncludeDuplicated,
      CodeWarning(T, D, F,
      "#include duplicated")));
   Attrs.insert(WarningPair(IncludeAdd,
      CodeWarning(T, C, F,
      "Add #include directive")));
   Attrs.insert(WarningPair(IncludeRemove,
      CodeWarning(T, D, F,
      "Remove #include directive")));
   Attrs.insert(WarningPair(RemoveOverrideTag,
      CodeWarning(T, E, F,
      "Remove override tag: function is final")));
   Attrs.insert(WarningPair(UsingInHeader,
      CodeWarning(T, I, F,
      "Using statement in header")));
   Attrs.insert(WarningPair(UsingDuplicated,
      CodeWarning(T, D, F,
      "Using statement duplicated")));
   Attrs.insert(WarningPair(UsingAdd,
      CodeWarning(T, C, F,
      "Add using statement")));
   Attrs.insert(WarningPair(UsingRemove,
      CodeWarning(T, D, F,
      "Remove using statement")));
   Attrs.insert(WarningPair(ForwardAdd,
      CodeWarning(T, C, F,
      "Add forward declaration")));
   Attrs.insert(WarningPair(ForwardRemove,
      CodeWarning(T, D, F,
      "Remove forward declaration")));
   Attrs.insert(WarningPair(ArgumentUnused,
      CodeWarning(F, X, T,
      "Unused argument")));
   Attrs.insert(WarningPair(ClassUnused,
      CodeWarning(F, X, T,
      "Unused class")));
   Attrs.insert(WarningPair(DataUnused,
      CodeWarning(F, X, T,
      "Unused data")));
   Attrs.insert(WarningPair(EnumUnused,
      CodeWarning(T, D, T,
      "Unused enum")));
   Attrs.insert(WarningPair(EnumeratorUnused,
      CodeWarning(T, D, T,
      "Unused enumerator")));
   Attrs.insert(WarningPair(FriendUnused,
      CodeWarning(T, D, T,
      "Unused friend declaration")));
   Attrs.insert(WarningPair(FunctionUnused,
      CodeWarning(F, X, T,
      "Unused function")));
   Attrs.insert(WarningPair(TypedefUnused,
      CodeWarning(T, D, T,
      "Unused typedef")));
   Attrs.insert(WarningPair(ForwardUnresolved,
      CodeWarning(T, D, F,
      "No referent for forward declaration")));
   Attrs.insert(WarningPair(FriendUnresolved,
      CodeWarning(T, D, F,
      "No referent for friend declaration")));
   Attrs.insert(WarningPair(FriendAsForward,
      CodeWarning(F, X, F,
      "Indirect reference relies on friend, not forward, declaration")));
   Attrs.insert(WarningPair(HidesInheritedName,
      CodeWarning(F, X, F,
      "Member hides inherited name")));
   Attrs.insert(WarningPair(ClassCouldBeNamespace,
      CodeWarning(F, X, F,
      "Class could be namespace")));
   Attrs.insert(WarningPair(ClassCouldBeStruct,
      CodeWarning(T, R, F,
      "Class could be struct")));
   Attrs.insert(WarningPair(StructCouldBeClass,
      CodeWarning(T, R, F,
      "Struct could be class")));
   Attrs.insert(WarningPair(RedundantAccessControl,
      CodeWarning(T, D, F,
      "Redundant access control")));
   Attrs.insert(WarningPair(ItemCouldBePrivate,
      CodeWarning(F, X, F,
      "Member could be private")));
   Attrs.insert(WarningPair(ItemCouldBeProtected,
      CodeWarning(F, X, F,
      "Member could be protected")));
   Attrs.insert(WarningPair(PointerTypedef,
      CodeWarning(F, X, F,
      "Typedef of pointer type")));
   Attrs.insert(WarningPair(AnonymousEnum,
      CodeWarning(F, X, F,
      "Unnamed enum")));
   Attrs.insert(WarningPair(DataUninitialized,
      CodeWarning(F, X, F,
      "Global data initialization not found")));
   Attrs.insert(WarningPair(DataInitOnly,
      CodeWarning(F, X, F,
      "Data is init-only")));
   Attrs.insert(WarningPair(DataWriteOnly,
      CodeWarning(F, X, F,
      "Data is write-only")));
   Attrs.insert(WarningPair(GlobalStaticData,
      CodeWarning(F, X, F,
      "Global static data")));
   Attrs.insert(WarningPair(DataNotPrivate,
      CodeWarning(F, X, F,
      "Data is not private")));
   Attrs.insert(WarningPair(DataCannotBeConst,
      CodeWarning(F, X, F,
      "DATA CANNOT BE CONST")));
   Attrs.insert(WarningPair(DataCannotBeConstPtr,
      CodeWarning(F, X, F,
      "DATA CANNOT BE CONST POINTER")));
   Attrs.insert(WarningPair(DataCouldBeConst,
      CodeWarning(T, I, F,
      "Data could be const")));
   Attrs.insert(WarningPair(DataCouldBeConstPtr,
      CodeWarning(T, I, F,
      "Data could be const pointer")));
   Attrs.insert(WarningPair(DataNeedNotBeMutable,
      CodeWarning(T, E, F,
      "Data need not be mutable")));
   Attrs.insert(WarningPair(DefaultPODConstructor,
      CodeWarning(F, X, F,
      "Default constructor invoked: POD members not initialized")));
   Attrs.insert(WarningPair(DefaultConstructor,
      CodeWarning(T, C, F,
      "Default constructor invoked")));
   Attrs.insert(WarningPair(DefaultCopyConstructor,
      CodeWarning(T, C, F,
      "Default copy constructor invoked")));
   Attrs.insert(WarningPair(DefaultCopyOperator,
      CodeWarning(T, C, F,
      "Default copy (assignment) operator invoked")));
   Attrs.insert(WarningPair(PublicConstructor,
      CodeWarning(F, X, F,
      "Base class constructor is public")));
   Attrs.insert(WarningPair(NonExplicitConstructor,
      CodeWarning(T, I, F,
      "Single-argument constructor is not explicit")));
   Attrs.insert(WarningPair(MemberInitMissing,
      CodeWarning(F, X, F,
      "Member not included in member initialization list")));
   Attrs.insert(WarningPair(MemberInitNotSorted,
      CodeWarning(F, X, F,
      "Member not sorted in standard order in member initialization list")));
   Attrs.insert(WarningPair(DefaultDestructor,
      CodeWarning(T, C, F,
      "Default destructor invoked")));
   Attrs.insert(WarningPair(VirtualDestructor,
      CodeWarning(F, X, F,
      "Base class virtual destructor is not public")));
   Attrs.insert(WarningPair(NonVirtualDestructor,
      CodeWarning(F, X, F,
      "Base class non-virtual destructor is public")));
   Attrs.insert(WarningPair(VirtualFunctionInvoked,
      CodeWarning(F, X, F,
      "Virtual function in own class invoked by constructor or destructor")));
   Attrs.insert(WarningPair(RuleOf3DtorNoCopyCtor,
      CodeWarning(F, X, F,
      "Destructor defined, but not copy constructor")));
   Attrs.insert(WarningPair(RuleOf3DtorNoCopyOper,
      CodeWarning(F, X, F,
      "Destructor defined, but not copy operator")));
   Attrs.insert(WarningPair(RuleOf3CopyCtorNoOper,
      CodeWarning(T, I, F,
      "Copy constructor defined, but not copy operator")));
   Attrs.insert(WarningPair(RuleOf3CopyOperNoCtor,
      CodeWarning(T, I, F,
      "Copy operator defined, but not copy constructor")));
   Attrs.insert(WarningPair(OperatorOverloaded,
      CodeWarning(F, X, F,
      "Overloading operator && or ||")));
   Attrs.insert(WarningPair(FunctionNotDefined,
      CodeWarning(T, D, F,
      "Function not implemented")));
   Attrs.insert(WarningPair(PureVirtualNotDefined,
      CodeWarning(F, X, F,
      "Pure virtual function not implemented")));
   Attrs.insert(WarningPair(VirtualAndPublic,
      CodeWarning(F, X, F,
      "Virtual function is public")));
   Attrs.insert(WarningPair(VirtualOverloading,
      CodeWarning(F, X, F,
      "Virtual function is overloaded")));
   Attrs.insert(WarningPair(FunctionNotOverridden,
      CodeWarning(F, X, F,
      "Virtual function has no overrides")));
   Attrs.insert(WarningPair(RemoveVirtualTag,
      CodeWarning(T, E, F,
      "Remove virtual tag: function is an override or final")));
   Attrs.insert(WarningPair(OverrideTagMissing,
      CodeWarning(T, I, F,
      "Function should be tagged as override")));
   Attrs.insert(WarningPair(VoidAsArgument,
      CodeWarning(T, E, F,
      "\"(void)\" as function argument")));
   Attrs.insert(WarningPair(AnonymousArgument,
      CodeWarning(F, X, F,
      "Unnamed argument")));
   Attrs.insert(WarningPair(AdjacentArgumentTypes,
      CodeWarning(F, X, F,
      "Adjacent arguments have the same type")));
   Attrs.insert(WarningPair(DefinitionRenamesArgument,
      CodeWarning(T, R, F,
      "Definition renames argument in declaration")));
   Attrs.insert(WarningPair(OverrideRenamesArgument,
      CodeWarning(T, R, F,
      "Override renames argument in direct base class")));
   Attrs.insert(WarningPair(VirtualDefaultArgument,
      CodeWarning(F, X, F,
      "Virtual function defines default argument")));
   Attrs.insert(WarningPair(ArgumentCannotBeConst,
      CodeWarning(F, X, F,
      "ARGUMENT CANNOT BE CONST")));
   Attrs.insert(WarningPair(ArgumentCouldBeConstRef,
      CodeWarning(T, I, F,
      "Object could be passed by const reference")));
   Attrs.insert(WarningPair(ArgumentCouldBeConst,
      CodeWarning(T, I, F,
      "Argument could be const")));
   Attrs.insert(WarningPair(FunctionCannotBeConst,
      CodeWarning(F, X, F,
      "FUNCTION CANNOT BE CONST")));
   Attrs.insert(WarningPair(FunctionCouldBeConst,
      CodeWarning(T, I, F,
      "Function could be const")));
   Attrs.insert(WarningPair(FunctionCouldBeStatic,
      CodeWarning(T, I, F,
      "Function could be static")));
   Attrs.insert(WarningPair(FunctionCouldBeFree,
      CodeWarning(F, X, F,
      "Function could be free")));
   Attrs.insert(WarningPair(StaticFunctionViaMember,
      CodeWarning(F, X, F,
      "Static function invoked via operator \".\" or \"->\"")));
   Attrs.insert(WarningPair(NonBooleanConditional,
      CodeWarning(F, X, F,
      "Non-boolean in conditional expression")));
   Attrs.insert(WarningPair(EnumTypesDiffer,
      CodeWarning(F, X, F,
      "Arguments to binary operator have different enum types")));
   Attrs.insert(WarningPair(UseOfTab,
      CodeWarning(T, R, F,
      "Tab character in source code")));
   Attrs.insert(WarningPair(Indentation,
      CodeWarning(T, R, F,
      "Line indentation is not a multiple of the standard value")));
   Attrs.insert(WarningPair(TrailingSpace,
      CodeWarning(T, E, F,
      "Line contains trailing space")));
   Attrs.insert(WarningPair(AdjacentSpaces,
      CodeWarning(T, E, F,
      "Line contains adjacent spaces")));
   Attrs.insert(WarningPair(AddBlankLine,
      CodeWarning(T, C, F,
      "Insertion of blank line recommended")));
   Attrs.insert(WarningPair(RemoveBlankLine,
      CodeWarning(T, D, F,
      "Deletion of blank line recommended")));
   Attrs.insert(WarningPair(LineLength,
      CodeWarning(F, S, F,
      "Line length exceeds the standard maximum")));
   Attrs.insert(WarningPair(FunctionNotSorted,
      CodeWarning(F, X, F,
      "Function not sorted in standard order")));
   Attrs.insert(WarningPair(HeadingNotStandard,
      CodeWarning(F, X, F,
      "File heading is not standard")));
   Attrs.insert(WarningPair(IncludeGuardMisnamed,
      CodeWarning(T, R, F,
      "Name of #include guard is not standard")));
   Attrs.insert(WarningPair(DebugFtNotInvoked,
      CodeWarning(T, C, F,
      "Function does not invoke Debug::ft")));
   Attrs.insert(WarningPair(DebugFtNotFirst,
      CodeWarning(F, X, F,
      "Function does not invoke Debug::ft as first statement")));
   Attrs.insert(WarningPair(DebugFtNameMismatch,
      CodeWarning(T, R, F,
      "Function name passed to Debug::ft is not standard")));
   Attrs.insert(WarningPair(DebugFtNameDuplicated,
      CodeWarning(F, X, F,
      "Function name passed to Debug::ft is used by another function")));
   Attrs.insert(WarningPair(DisplayNotOverridden,
      CodeWarning(F, X, F,
      "Override of Base.Display not found")));
   Attrs.insert(WarningPair(PatchNotOverridden,
      CodeWarning(F, X, F,
      "Override of Object.Patch not found")));
   Attrs.insert(WarningPair(FunctionCouldBeDefaulted,
      CodeWarning(T, R, F,
      "Function could be defaulted")));
   Attrs.insert(WarningPair(InitCouldUseConstructor,
      CodeWarning(T, R, F,
      "Initialization uses assignment operator")));
   Attrs.insert(WarningPair(CouldBeNoexcept,
      CodeWarning(T, I, F,
      "Function could be tagged noexcept")));
   Attrs.insert(WarningPair(ShouldNotBeNoexcept,
      CodeWarning(T, I, F,
      "Function should not be tagged noexcept")));
   Attrs.insert(WarningPair(UseOfSlashAsterisk,
      CodeWarning(T, R, F,
      "C-style comment")));
   Attrs.insert(WarningPair(RemoveLineBreak,
      CodeWarning(T, J, F,
      "Line can merge with the next line and be under the length limit")));
   Attrs.insert(WarningPair(Warning_N,
      CodeWarning(F, X, F,
      ERROR_STR)));
}

//------------------------------------------------------------------------------

ostream& operator<<(ostream& stream, Warning warning)
{
   if((warning >= 0) && (warning < Warning_N))
      stream << CodeWarning::Attrs.at(warning).title;
   else
      stream << CodeWarning::Attrs.at(Warning_N).title;
   return stream;
}

//==============================================================================

WarningLog::WarningLog(Warning warning, const CodeFile* file,
   size_t line, size_t pos, const CxxNamed* item, word offset,
   const string& info, bool hide) :
   warning(warning),
   file(file),
   line(line),
   pos(pos),
   item(item),
   offset(offset),
   info(info),
   hide(hide),
   status(NotSupported)
{
   if(CodeWarning::Attrs.at(warning).fixable) status = NotFixed;
}

//------------------------------------------------------------------------------

bool WarningLog::operator==(const WarningLog& that) const
{
   if(this->file != that.file) return false;
   if(this->line != that.line) return false;
   if(this->pos != that.pos) return false;
   if(this->warning != that.warning) return false;
   if(this->offset != that.offset) return false;
   return (this->info == that.info);
}

//------------------------------------------------------------------------------

bool WarningLog::operator!=(const WarningLog& that) const
{
   return !(*this == that);
}

//==============================================================================

size_t CodeInfo::LineTypeCounts_[] = { 0 };

size_t CodeInfo::WarningCounts_[] = { 0 };

std::vector< WarningLog > CodeInfo::Warnings_ = std::vector< WarningLog >();

//------------------------------------------------------------------------------

fn_name CodeInfo_AddWarning = "CodeInfo.AddWarning";

void CodeInfo::AddWarning(const WarningLog& log)
{
   Debug::ft(CodeInfo_AddWarning);

   if(FindWarning(log) < 0) Warnings_.push_back(log);
}

//------------------------------------------------------------------------------

word CodeInfo::FindWarning(const WarningLog& log)
{
   for(size_t i = 0; i < Warnings_.size(); ++i)
   {
      if(Warnings_.at(i) == log) return i;
   }

   return -1;
}

//------------------------------------------------------------------------------

fn_name CodeInfo_GenerateReport = "CodeInfo.GenerateReport";

void CodeInfo::GenerateReport(ostream* stream, const SetOfIds& set)
{
   Debug::ft(CodeInfo_GenerateReport);

   //  Clear any previous report's global counts.
   //
   for(auto t = 0; t < LineType_N; ++t) LineTypeCounts_[t] = 0;
   for(auto w = 0; w < Warning_N; ++w) WarningCounts_[w] = 0;

   //  Sort the files to be checked in build order.  This is important because
   //  recommendations about adding and removing #include directives and using
   //  statements, for example, are affected by earlier recommendations for
   //  #included files.
   //
   auto checkSet = new SetOfIds(set);
   auto checkFiles = new CodeFileSet(LibrarySet::TemporaryName(), checkSet);
   auto order = static_cast< CodeFileSet* >(checkFiles)->SortInBuildOrder();

   //  Run a check on each file in ORDER, as well as on each C++ item.
   //
   auto& files = Singleton< Library >::Instance()->Files();

   for(auto f = order->cbegin(); f != order->cend(); ++f)
   {
      auto file = files.At(f->fid);
      if(file->IsHeader()) file->Check();
      ThisThread::Pause();
   }

   for(auto f = order->cbegin(); f != order->cend(); ++f)
   {
      auto file = files.At(f->fid);
      if(file->IsCpp()) file->Check();
      ThisThread::Pause();
   }

   Singleton< CxxRoot >::Instance()->GlobalNamespace()->Check();

   //  Return if a report is not required.
   //
   if(stream == nullptr) return;

   //  Count the lines of each type.
   //
   for(auto f = set.cbegin(); f != set.cend(); ++f)
   {
      files.At(*f)->GetLineCounts();
   }

   std::vector< WarningLog > warnings;

   //  Count the total number of warnings of each type that appear in files
   //  belonging to the original SET, extracing them into the local set of
   //  warnings.  Exclude warnings that are to be hidden.
   //
   for(auto item = Warnings_.cbegin(); item != Warnings_.cend(); ++item)
   {
      if(!item->hide && (set.find(item->file->Fid()) != set.cend()))
      {
         ++WarningCounts_[item->warning];
         warnings.push_back(*item);
      }
   }

   //  Display the total number of lines of each type.
   //
   *stream << "LINE COUNTS" << CRLF;

   for(auto t = 0; t < LineType_N; ++t)
   {
      *stream << setw(12) << LineType(t)
         << spaces(2) << setw(6) << LineTypeCounts_[t] << CRLF;
   }

   //  Display the total number of warnings of each type.
   //
   *stream << CRLF << "WARNING COUNTS" << CRLF;

   for(auto w = 0; w < Warning_N; ++w)
   {
      if(WarningCounts_[w] != 0)
      {
         *stream << setw(6) << WarningCode(Warning(w)) << setw(6)
            << WarningCounts_[w] << spaces(2) << Warning(w) << CRLF;
      }
   }

   *stream << string(120, '=') << CRLF;
   *stream << "WARNINGS SORTED BY TYPE/FILE/LINE" << CRLF;

   //  Sort and output the warnings by warning type/file/line.
   //
   std::sort(warnings.begin(), warnings.end(), IsSortedByWarning);

   auto item = warnings.cbegin();
   auto last = warnings.cend();

   while(item != last)
   {
      auto w = item->warning;
      *stream << WarningCode(w) << SPACE << w << CRLF;

      do
      {
         *stream << spaces(2) << item->file->FullName();
         *stream << '(' << item->line + 1;
         if(item->offset > 0) *stream << '/' << item->offset;
         *stream << "): ";

         if(item->DisplayCode())
         {
            *stream << item->file->GetLexer().GetNthLine(item->line);
         }

         if(item->DisplayInfo()) *stream << item->info;
         *stream << CRLF;
         ++item;
      }
      while((item != last) && (item->warning == w));
   }

   *stream << string(120, '=') << CRLF;
   *stream << "WARNINGS SORTED BY FILE/TYPE/LINE" << CRLF;

   //  Sort and output the warnings by file/warning type/line.
   //
   std::sort(warnings.begin(), warnings.end(), IsSortedByFile);

   item = warnings.cbegin();
   last = warnings.cend();

   while(item != last)
   {
      auto f = item->file;
      *stream << f->FullName() << CRLF;

      do
      {
         auto w = item->warning;
         *stream << spaces(2) << WarningCode(w) << SPACE << w << CRLF;

         do
         {
            *stream << spaces(4) << item->line + 1;
            if(item->offset > 0) *stream << '/' << item->offset;
            *stream << ": ";

            if((item->line != 0) || item->info.empty())
            {
               *stream << f->GetLexer().GetNthLine(item->line);
            }

            *stream << item->info << CRLF;
            ++item;
         }
         while((item != last) && (item->warning == w) && (item->file == f));
      }
      while((item != last) && (item->file == f));
   }
}

//------------------------------------------------------------------------------

fn_name CodeInfo_GetWarnings = "CodeInfo.GetWarnings";

void CodeInfo::GetWarnings(const CodeFile* file, WarningLogVector& warnings)
{
   Debug::ft(CodeInfo_GetWarnings);

   for(auto w = Warnings_.cbegin(); w != Warnings_.cend(); ++w)
   {
      if(w->file == file)
      {
         warnings.push_back(*w);
      }
   }
}

//------------------------------------------------------------------------------

bool CodeInfo::IsSortedByFile(const WarningLog& log1, const WarningLog& log2)
{
   auto result = strCompare(log1.file->FullName(), log2.file->FullName());
   if(result == -1) return true;
   if(result == 1) return false;
   if(log1.warning < log2.warning) return true;
   if(log1.warning > log2.warning) return false;
   if(log1.line < log2.line) return true;
   if(log1.line > log2.line) return false;
   if(log1.offset < log2.offset) return true;
   if(log1.offset > log2.offset) return false;
   if(log1.info < log2.info) return true;
   if(log1.info > log2.info) return false;
   return (&log1 < &log2);
}

//------------------------------------------------------------------------------

bool CodeInfo::IsSortedByWarning(const WarningLog& log1, const WarningLog& log2)
{
   if(log1.warning < log2.warning) return true;
   if(log1.warning > log2.warning) return false;
   auto result = strCompare(log1.file->FullName(), log2.file->FullName());
   if(result == -1) return true;
   if(result == 1) return false;
   if(log1.line < log2.line) return true;
   if(log1.line > log2.line) return false;
   if(log1.offset < log2.offset) return true;
   if(log1.offset > log2.offset) return false;
   if(log1.info < log2.info) return true;
   if(log1.info > log2.info) return false;
   return (&log1 < &log2);
}

//------------------------------------------------------------------------------

bool CodeInfo::IsSortedForFixing(const WarningLog& log1, const WarningLog& log2)
{
   auto result = strCompare(log1.file->FullName(), log2.file->FullName());
   if(result == -1) return true;
   if(result == 1) return false;
   auto& attrs = CodeWarning::Attrs;
   if(attrs.at(log1.warning).order < attrs.at(log2.warning).order) return true;
   if(attrs.at(log1.warning).order > attrs.at(log2.warning).order) return false;
   if(log1.line < log2.line) return true;
   if(log1.line > log2.line) return false;
   if(log1.pos > log2.pos) return true;
   if(log1.pos < log2.pos) return false;
   if(log1.offset > log2.offset) return true;
   if(log1.offset < log2.offset) return false;
   if(log1.warning < log2.warning) return true;
   if(log1.warning > log2.warning) return false;
   if(log1.info < log2.info) return true;
   if(log1.info > log2.info) return false;
   return (&log1 < &log2);
}

//------------------------------------------------------------------------------

string CodeInfo::WarningCode(Warning warning)
{
   std::ostringstream stream;

   stream << 'W' << setw(3) << std::setfill('0') << int(warning);
   return stream.str();
}
}
