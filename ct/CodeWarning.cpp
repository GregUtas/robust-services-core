//==============================================================================
//
//  CodeWarning.cpp
//
//  Copyright (C) 2013-2022  Greg Utas
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
#include <cstring>
#include <iomanip>
#include <iterator>
#include <set>
#include <sstream>
#include "CodeDir.h"
#include "CodeFile.h"
#include "CodeFileSet.h"
#include "CxxArea.h"
#include "CxxNamed.h"
#include "CxxRoot.h"
#include "CxxScope.h"
#include "Debug.h"
#include "Formatters.h"
#include "Lexer.h"
#include "Singleton.h"
#include "ThisThread.h"

using namespace NodeBase;
using std::ostream;
using std::setw;
using std::string;

//------------------------------------------------------------------------------

namespace CodeTools
{
ostream& operator<<(ostream& stream, Warning warning)
{
   if((warning >= 0) && (warning < Warning_N))
      stream << CodeWarning::Expl(warning);
   else
      stream << CodeWarning::Expl(Warning_N);
   return stream;
}

//------------------------------------------------------------------------------

WarningAttrs::WarningAttrs(bool fixable, bool preserve, c_string expl) :
   fixable_(fixable),
   preserve_(preserve),
   expl_(expl)
{
}

//------------------------------------------------------------------------------

static string WarningCode(Warning warning)
{
   std::ostringstream stream;

   stream << 'W' << setw(3) << std::setfill('0') << int(warning);
   return stream.str();
}

//==============================================================================

CodeWarning::AttrsMap CodeWarning::Attrs_ = AttrsMap();

size_t CodeWarning::LineTypeCounts_[] = { 0 };

//  The total number of warnings of each type, globally.
//
static size_t WarningCounts_[Warning_N] = { 0 };

//------------------------------------------------------------------------------

CodeWarning::CodeWarning(Warning warning, CodeFile* file, size_t pos,
   const CxxToken* item, word offset, const string& info) :
   warning_(warning),
   item_(const_cast< CxxToken* >(item)),
   offset_(offset),
   info_(info),
   status_(NotSupported)
{
   Debug::ft("CodeWarning.ctor");

   //  Make the warning non-internal so that CxxLocation.UpdatePos will
   //  will update its position when code is edited.
   //
   loc_.SetLoc(file, pos, false);
   if(Attrs_.at(warning).fixable_) status_ = NotFixed;
}

//------------------------------------------------------------------------------

const CodeWarning* CodeWarning::FindMateLog(std::string& expl) const
{
   Debug::ft("CodeWarning.FindMateLog");

   //  Look for the mate item associated with this log.  Find its file and
   //  editor, and ask its editor find the log that corresponds to this one.
   //
   auto mate = item_->GetMate();
   if(mate == nullptr) return nullptr;
   auto mateFile = mate->GetFile();
   if(mateFile == nullptr)
   {
      expl = "Mate's file not found";
      return nullptr;
   }
   return FindWarning(mateFile, warning_, mate, offset_);
}

//------------------------------------------------------------------------------

const CodeWarning* CodeWarning::FindWarning(const CodeFile* file,
   Warning warning, const CxxToken* item, word offset)
{
   Debug::ft("CodeWarning.FindWarning");

   auto& warnings = file->GetWarnings();

   for(auto w = warnings.begin(); w != warnings.end(); ++w)
   {
      if((w->warning_ == warning) && (w->item_ == item) &&
         (w->offset_ == offset))
      {
         return &*w;
      }
   }

   return nullptr;
}

//------------------------------------------------------------------------------

void CodeWarning::GenerateReport(ostream* stream, const LibItemSet& files)
{
   Debug::ft("CodeWarning.GenerateReport");

   //  Clear any previous report's global counts.
   //
   for(auto t = 0; t < LineType_N; ++t) LineTypeCounts_[t] = 0;
   for(auto w = 0; w < Warning_N; ++w) WarningCounts_[w] = 0;

   //  Sort the files to be checked in build order.  This is important because
   //  recommendations about adding and removing #include directives and using
   //  statements, for example, are affected by earlier recommendations for
   //  #included files.
   //
   auto check = new CodeFileSet(LibrarySet::TemporaryName(), &files);
   auto order = check->SortInBuildOrder();

   //  Run a check on each file in ORDER, as well as on each C++ item.
   //
   for(auto f = order.cbegin(); f != order.cend(); ++f)
   {
      auto file = f->file;
      if(file->IsHeader()) file->Check(stream != nullptr);
      ThisThread::Pause();
   }

   for(auto f = order.cbegin(); f != order.cend(); ++f)
   {
      auto file = f->file;
      if(file->IsCpp()) file->Check(stream != nullptr);
      ThisThread::Pause();
   }

   Singleton< CxxRoot >::Instance()->Check(stream != nullptr);

   //  Return if a report is not required.
   //
   if(stream == nullptr) return;

   //  Count the lines of each type.
   //
   for(auto f = order.cbegin(); f != order.cend(); ++f)
   {
      f->file->GetLineCounts();
   }

   std::vector< const CodeWarning* > warnings;

   //  Count the total number of warnings of each type that appear in files
   //  belonging to the original SET, extracting them into the local set of
   //  warnings.  Don't count warnings that are informational.
   //
   for(auto f = files.cbegin(); f != files.cend(); ++f)
   {
      auto file = static_cast< CodeFile* >(*f);
      auto& logs = file->GetWarnings();

      for(size_t i = 0; i < logs.size(); ++i)
      {
         auto& log = logs[i];
         if(log.WasResolved()) continue;
         if(!log.IsInformational()) ++WarningCounts_[log.warning_];
         warnings.push_back(&log);
      }
   }

   //  Display the total number of lines of each type.
   //
   *stream << "LINE COUNTS" << CRLF;

   for(auto t = 0; t < LineType_N; ++t)
   {
      *stream << setw(8) << LineTypeCounts_[t]
         << spaces(3) << LineType(t) << CRLF;
   }

   //  Display the total number of warnings of each type.
   //
   *stream << CRLF << "WARNING COUNTS (* if supported by >fix)" << CRLF;

   for(auto w = 0; w < Warning_N; ++w)
   {
      if(WarningCounts_[w] != 0)
      {
         *stream << (Attrs_.at(Warning(w)).fixable_ ? '*' : SPACE);
         *stream << setw(5) << WarningCode(Warning(w)) << setw(6)
            << WarningCounts_[w] << spaces(2) << Warning(w) << CRLF;
      }
   }

   *stream << string(132, '=') << CRLF;
   *stream << "WARNINGS SORTED BY TYPE/FILE/LINE (i = informational)" << CRLF;

   //  Sort and output the warnings by warning type/file/line.
   //
   std::sort(warnings.begin(), warnings.end(), IsSortedByType);

   auto item = warnings.cbegin();
   auto last = warnings.cend();

   while(item != last)
   {
      auto w = (*item)->warning_;
      *stream << WarningCode(w) << SPACE << w << CRLF;

      do
      {
         auto f = (*item)->File();

         *stream << ((*item)->IsInformational() ? 'i' : SPACE);
         *stream << SPACE << f->Path();
         *stream << '(' << (*item)->Line() + 1;
         if((*item)->offset_ > 0) *stream << '/' << (*item)->offset_;
         *stream << "): ";

         if((*item)->HasCodeToDisplay())
         {
            *stream << f->GetLexer().GetCode((*item)->Pos(), false);
         }

         if((*item)->HasInfoToDisplay()) *stream << " // " << (*item)->info_;
         *stream << CRLF;
         ++item;
      }
      while((item != last) && ((*item)->warning_ == w));
   }

   *stream << string(132, '=') << CRLF;
   *stream << "WARNINGS SORTED BY FILE/TYPE/LINE (i = informational)" << CRLF;

   //  Sort and output the warnings by file/warning_ type/line.
   //
   std::sort(warnings.begin(), warnings.end(), IsSortedByFile);

   item = warnings.cbegin();
   last = warnings.cend();

   while(item != last)
   {
      auto f = (*item)->File();
      *stream << f->Path() << CRLF;

      do
      {
         auto w = (*item)->warning_;
         *stream << (Attrs_.at(Warning(w)).fixable_ ? '*' : SPACE);
         *stream << SPACE << WarningCode(w) << SPACE << w << CRLF;

         do
         {
            *stream << spaces(2);
            *stream << ((*item)->IsInformational() ? 'i' : SPACE);
            *stream << SPACE << (*item)->Line() + 1;
            if((*item)->offset_ > 0) *stream << '/' << (*item)->offset_;
            *stream << ": ";

            if((*item)->HasCodeToDisplay())
            {
               *stream << f->GetLexer().GetCode((*item)->Pos(), false);
            }

            if((*item)->HasInfoToDisplay()) *stream << " // " << (*item)->info_;
            *stream << CRLF;
            ++item;
         }
         while((item != last) &&
            ((*item)->warning_ == w) && ((*item)->File() == f));
      }
      while((item != last) && ((*item)->File() == f));
   }
}

//------------------------------------------------------------------------------

string CodeWarning::GetNewFuncName() const
{
   Debug::ft("CodeWarning.GetNewFuncName");

   switch(warning_)
   {
   case DisplayNotOverridden:
      return "Display";
   case PatchNotOverridden:
      return "Patch";
   }

   return EMPTY_STR;
}

//------------------------------------------------------------------------------

bool CodeWarning::HasCodeToDisplay() const
{
   return ((Pos() != string::npos) || info_.empty());
}

//------------------------------------------------------------------------------

bool CodeWarning::HasInfoToDisplay() const
{
   return (info_.find_first_not_of(SPACE) != std::string::npos);
}

//------------------------------------------------------------------------------

void CodeWarning::Initialize()
{
   Debug::ft("CodeWarning.Initialize");

   const auto F = false;
   const auto T = true;

   Attrs_.insert(WarningPair(AllWarnings,
      WarningAttrs(T, false,
      "All warnings")));
   Attrs_.insert(WarningPair(UseOfNull,
      WarningAttrs(T, false,
      "Use of NULL")));
   Attrs_.insert(WarningPair(PtrTagDetached,
      WarningAttrs(T, false,
      "Pointer tag ('*') detached from type")));
   Attrs_.insert(WarningPair(RefTagDetached,
      WarningAttrs(T, false,
      "Reference tag ('&') detached from type")));
   Attrs_.insert(WarningPair(UseOfCast,
      WarningAttrs(F, false,
      "C-style cast")));
   Attrs_.insert(WarningPair(FunctionalCast,
      WarningAttrs(F, true,
      "Functional cast")));
   Attrs_.insert(WarningPair(ReinterpretCast,
      WarningAttrs(F, false,
      "reinterpret_cast")));
   Attrs_.insert(WarningPair(Downcasting,
      WarningAttrs(F, true,
      "Cast down the inheritance hierarchy")));
   Attrs_.insert(WarningPair(CastingAwayConstness,
      WarningAttrs(F, true,
      "Cast removes const qualification")));
   Attrs_.insert(WarningPair(PointerArithmetic,
      WarningAttrs(F, true,
      "Pointer arithmetic")));
   Attrs_.insert(WarningPair(RedundantSemicolon,
      WarningAttrs(T, true,
      "Semicolon not required")));
   Attrs_.insert(WarningPair(RedundantConst,
      WarningAttrs(T, true,
      "Redundant const in type specification")));
   Attrs_.insert(WarningPair(DefineNotAtFileScope,
      WarningAttrs(F, true,
      "#define appears within a class or function")));
   Attrs_.insert(WarningPair(IncludeFollowsCode,
      WarningAttrs(T, false,
      "#include appears after code")));
   Attrs_.insert(WarningPair(IncludeGuardMissing,
      WarningAttrs(T, false,
      "No #include guard found")));
   Attrs_.insert(WarningPair(IncludeNotSorted,
      WarningAttrs(T, false,
      "#include not sorted in standard order")));
   Attrs_.insert(WarningPair(IncludeDuplicated,
      WarningAttrs(T, false,
      "#include duplicated")));
   Attrs_.insert(WarningPair(IncludeAdd,
      WarningAttrs(T, false,
      "Add #include directive")));
   Attrs_.insert(WarningPair(IncludeRemove,
      WarningAttrs(T, false,
      "Remove #include directive")));
   Attrs_.insert(WarningPair(RemoveOverrideTag,
      WarningAttrs(T, false,
      "Remove override tag: function is final")));
   Attrs_.insert(WarningPair(UsingInHeader,
      WarningAttrs(T, false,
      "Using statement in header")));
   Attrs_.insert(WarningPair(UsingDuplicated,
      WarningAttrs(T, false,
      "Using statement duplicated")));
   Attrs_.insert(WarningPair(UsingAdd,
      WarningAttrs(T, false,
      "Add using statement")));
   Attrs_.insert(WarningPair(UsingRemove,
      WarningAttrs(T, false,
      "Remove using statement")));
   Attrs_.insert(WarningPair(ForwardAdd,
      WarningAttrs(T, false,
      "Add forward declaration")));
   Attrs_.insert(WarningPair(ForwardRemove,
      WarningAttrs(T, false,
      "Remove forward declaration")));
   Attrs_.insert(WarningPair(ArgumentUnused,
      WarningAttrs(F, false,
      "Unused argument")));
   Attrs_.insert(WarningPair(ClassUnused,
      WarningAttrs(F, false,
      "Unused class")));
   Attrs_.insert(WarningPair(DataUnused,
      WarningAttrs(T, false,
      "Unused data")));
   Attrs_.insert(WarningPair(EnumUnused,
      WarningAttrs(T, false,
      "Unused enum")));
   Attrs_.insert(WarningPair(EnumeratorUnused,
      WarningAttrs(T, false,
      "Unused enumerator")));
   Attrs_.insert(WarningPair(FriendUnused,
      WarningAttrs(T, false,
      "Unused friend declaration")));
   Attrs_.insert(WarningPair(FunctionUnused,
      WarningAttrs(T, false,
      "Unused function")));
   Attrs_.insert(WarningPair(TypedefUnused,
      WarningAttrs(T, false,
      "Unused typedef")));
   Attrs_.insert(WarningPair(ForwardUnresolved,
      WarningAttrs(T, false,
      "No referent for forward declaration")));
   Attrs_.insert(WarningPair(FriendUnresolved,
      WarningAttrs(T, false,
      "No referent for friend declaration")));
   Attrs_.insert(WarningPair(FriendAsForward,
      WarningAttrs(T, true,
      "Indirect reference relies on friend, not forward, declaration")));
   Attrs_.insert(WarningPair(HidesInheritedName,
      WarningAttrs(F, false,
      "Member hides inherited name")));
   Attrs_.insert(WarningPair(ClassCouldBeNamespace,
      WarningAttrs(F, false,
      "Class could be namespace")));
   Attrs_.insert(WarningPair(ClassCouldBeStruct,
      WarningAttrs(T, false,
      "Class could be struct")));
   Attrs_.insert(WarningPair(StructCouldBeClass,
      WarningAttrs(T, false,
      "Struct could be class")));
   Attrs_.insert(WarningPair(RedundantAccessControl,
      WarningAttrs(T, true,
      "Redundant access control")));
   Attrs_.insert(WarningPair(ItemCouldBePrivate,
      WarningAttrs(T, false,
      "Member could be private")));
   Attrs_.insert(WarningPair(ItemCouldBeProtected,
      WarningAttrs(T, false,
      "Member could be protected")));
   Attrs_.insert(WarningPair(PointerTypedef,
      WarningAttrs(F, false,
      "Typedef of pointer type")));
   Attrs_.insert(WarningPair(AnonymousEnum,
      WarningAttrs(F, false,
      "Anonymous enum")));
   Attrs_.insert(WarningPair(DataUninitialized,
      WarningAttrs(F, false,
      "Global data initialization not found")));
   Attrs_.insert(WarningPair(DataInitOnly,
      WarningAttrs(T, false,
      "Data is init-only")));
   Attrs_.insert(WarningPair(DataWriteOnly,
      WarningAttrs(T, false,
      "Data is write-only")));
   Attrs_.insert(WarningPair(GlobalStaticData,
      WarningAttrs(F, false,
      "Global static data")));
   Attrs_.insert(WarningPair(DataNotPrivate,
      WarningAttrs(F, false,
      "Data is not private")));
   Attrs_.insert(WarningPair(DataCannotBeConst,
      WarningAttrs(F, false,
      "DATA CANNOT BE CONST")));
   Attrs_.insert(WarningPair(DataCannotBeConstPtr,
      WarningAttrs(F, false,
      "DATA CANNOT BE CONST POINTER")));
   Attrs_.insert(WarningPair(DataCouldBeConst,
      WarningAttrs(T, false,
      "Data could be const")));
   Attrs_.insert(WarningPair(DataCouldBeConstPtr,
      WarningAttrs(T, false,
      "Data could be const pointer")));
   Attrs_.insert(WarningPair(DataNeedNotBeMutable,
      WarningAttrs(T, false,
      "Data need not be mutable")));
   Attrs_.insert(WarningPair(ImplicitPODConstructor,
      WarningAttrs(F, true,
      "Implicit constructor invoked: POD members not initialized")));
   Attrs_.insert(WarningPair(ImplicitConstructor,
      WarningAttrs(T, true,
      "Implicit constructor invoked")));
   Attrs_.insert(WarningPair(ImplicitCopyConstructor,
      WarningAttrs(T, true,
      "Implicit copy constructor invoked")));
   Attrs_.insert(WarningPair(ImplicitCopyOperator,
      WarningAttrs(T, true,
      "Implicit copy (assignment) operator invoked")));
   Attrs_.insert(WarningPair(PublicConstructor,
      WarningAttrs(T, true,
      "Base class constructor is public")));
   Attrs_.insert(WarningPair(NonExplicitConstructor,
      WarningAttrs(T, false,
      "Single-argument constructor is not explicit")));
   Attrs_.insert(WarningPair(MemberInitMissing,
      WarningAttrs(F, false,
      "Member not included in member initialization list")));
   Attrs_.insert(WarningPair(MemberInitNotSorted,
      WarningAttrs(F, false,
      "Member not sorted in standard order in member initialization list")));
   Attrs_.insert(WarningPair(ImplicitDestructor,
      WarningAttrs(T, true,
      "Implicit destructor invoked")));
   Attrs_.insert(WarningPair(VirtualDestructor,
      WarningAttrs(T, false,
      "Base class virtual destructor is not public")));
   Attrs_.insert(WarningPair(NonVirtualDestructor,
      WarningAttrs(T, true,
      "Base class non-virtual destructor is public")));
   Attrs_.insert(WarningPair(VirtualFunctionInvoked,
      WarningAttrs(F, true,
      "Virtual function invoked by constructor or destructor")));
   Attrs_.insert(WarningPair(RuleOf3DtorNoCopyCtor,
      WarningAttrs(T, false,
      "Destructor defined, but not copy constructor")));
   Attrs_.insert(WarningPair(RuleOf3DtorNoCopyOper,
      WarningAttrs(T, false,
      "Destructor defined, but not copy operator")));
   Attrs_.insert(WarningPair(RuleOf3CopyCtorNoOper,
      WarningAttrs(T, false,
      "Copy constructor defined, but not copy operator")));
   Attrs_.insert(WarningPair(RuleOf3CopyOperNoCtor,
      WarningAttrs(T, false,
      "Copy operator defined, but not copy constructor")));
   Attrs_.insert(WarningPair(OperatorOverloaded,
      WarningAttrs(F, true,
      "Overloading operator && or ||")));
   Attrs_.insert(WarningPair(FunctionNotDefined,
      WarningAttrs(T, false,
      "Function not implemented")));
   Attrs_.insert(WarningPair(PureVirtualNotDefined,
      WarningAttrs(F, false,
      "Pure virtual function not implemented")));
   Attrs_.insert(WarningPair(VirtualAndPublic,
      WarningAttrs(F, false,
      "Virtual function is public")));
   Attrs_.insert(WarningPair(BoolMixedWithNumeric,
      WarningAttrs(F, true,
      "Expression mixes bool with numeric")));
   Attrs_.insert(WarningPair(FunctionNotOverridden,
      WarningAttrs(T, false,
      "Virtual function has no overrides")));
   Attrs_.insert(WarningPair(RemoveVirtualTag,
      WarningAttrs(T, false,
      "Remove virtual tag: function is an override or final")));
   Attrs_.insert(WarningPair(OverrideTagMissing,
      WarningAttrs(T, false,
      "Function should be tagged as override")));
   Attrs_.insert(WarningPair(VoidAsArgument,
      WarningAttrs(T, true,
      "\"(void)\" as function argument")));
   Attrs_.insert(WarningPair(AnonymousArgument,
      WarningAttrs(T, false,
      "Anonymous argument")));
   Attrs_.insert(WarningPair(AdjacentArgumentTypes,
      WarningAttrs(F, false,
      "Adjacent arguments have the same type")));
   Attrs_.insert(WarningPair(DefinitionRenamesArgument,
      WarningAttrs(T, false,
      "Definition renames argument in declaration")));
   Attrs_.insert(WarningPair(OverrideRenamesArgument,
      WarningAttrs(T, false,
      "Override renames argument in root base class")));
   Attrs_.insert(WarningPair(VirtualDefaultArgument,
      WarningAttrs(F, false,
      "Virtual function defines default argument")));
   Attrs_.insert(WarningPair(ArgumentCannotBeConst,
      WarningAttrs(F, false,
      "ARGUMENT CANNOT BE CONST")));
   Attrs_.insert(WarningPair(ArgumentCouldBeConstRef,
      WarningAttrs(T, false,
      "Object could be passed by const reference")));
   Attrs_.insert(WarningPair(ArgumentCouldBeConst,
      WarningAttrs(T, false,
      "Argument could be const")));
   Attrs_.insert(WarningPair(FunctionCannotBeConst,
      WarningAttrs(F, false,
      "FUNCTION CANNOT BE CONST")));
   Attrs_.insert(WarningPair(FunctionCouldBeConst,
      WarningAttrs(T, false,
      "Function could be const")));
   Attrs_.insert(WarningPair(FunctionCouldBeStatic,
      WarningAttrs(T, false,
      "Function could be static")));
   Attrs_.insert(WarningPair(FunctionCouldBeFree,
      WarningAttrs(T, false,
      "Function could be free")));
   Attrs_.insert(WarningPair(StaticFunctionViaMember,
      WarningAttrs(F, true,
      "Static function invoked via operator \".\" or \"->\"")));
   Attrs_.insert(WarningPair(NonBooleanConditional,
      WarningAttrs(F, true,
      "Non-boolean in conditional expression")));
   Attrs_.insert(WarningPair(EnumTypesDiffer,
      WarningAttrs(F, true,
      "Arguments to binary operator have different enum types")));
   Attrs_.insert(WarningPair(UseOfTab,
      WarningAttrs(T, false,
      "Tab character in source code")));
   Attrs_.insert(WarningPair(Indentation,
      WarningAttrs(T, false,
      "Line indentation is not standard")));
   Attrs_.insert(WarningPair(TrailingSpace,
      WarningAttrs(T, false,
      "Line contains trailing space")));
   Attrs_.insert(WarningPair(AdjacentSpaces,
      WarningAttrs(T, false,
      "Line contains adjacent spaces")));
   Attrs_.insert(WarningPair(AddBlankLine,
      WarningAttrs(T, false,
      "Insertion of blank line recommended")));
   Attrs_.insert(WarningPair(RemoveLine,
      WarningAttrs(T, false,
      "Deletion of line recommended")));
   Attrs_.insert(WarningPair(LineLength,
      WarningAttrs(F, false,
      "Line length exceeds the standard maximum")));
   Attrs_.insert(WarningPair(FunctionNotSorted,
      WarningAttrs(T, false,
      "Function not sorted in standard order")));
   Attrs_.insert(WarningPair(HeadingNotStandard,
      WarningAttrs(F, false,
      "File heading is not standard")));
   Attrs_.insert(WarningPair(IncludeGuardMisnamed,
      WarningAttrs(T, false,
      "Name of #include guard is not standard")));
   Attrs_.insert(WarningPair(DebugFtNotInvoked,
      WarningAttrs(T, false,
      "Function does not invoke Debug::ft")));
   Attrs_.insert(WarningPair(DebugFtNotFirst,
      WarningAttrs(F, false,
      "Function does not invoke Debug::ft as first statement")));
   Attrs_.insert(WarningPair(DebugFtNameMismatch,
      WarningAttrs(T, false,
      "Function name passed to Debug::ft is not standard")));
   Attrs_.insert(WarningPair(DebugFtNameDuplicated,
      WarningAttrs(T, false,
      "Function name passed to Debug::ft is used by another function")));
   Attrs_.insert(WarningPair(DisplayNotOverridden,
      WarningAttrs(F, false,
      "Override of Base.Display not found")));
   Attrs_.insert(WarningPair(PatchNotOverridden,
      WarningAttrs(T, false,
      "Override of Object.Patch not found")));
   Attrs_.insert(WarningPair(FunctionCouldBeDefaulted,
      WarningAttrs(T, false,
      "Function could be defaulted")));
   Attrs_.insert(WarningPair(InitCouldUseConstructor,
      WarningAttrs(T, true,
      "Initialization uses assignment operator")));
   Attrs_.insert(WarningPair(CouldBeNoexcept,
      WarningAttrs(T, false,
      "Function could be tagged noexcept")));
   Attrs_.insert(WarningPair(ShouldNotBeNoexcept,
      WarningAttrs(T, false,
      "Function should not be tagged noexcept")));
   Attrs_.insert(WarningPair(UseOfSlashAsterisk,
      WarningAttrs(T, false,
      "C-style comment")));
   Attrs_.insert(WarningPair(RemoveLineBreak,
      WarningAttrs(T, false,
      "Line can merge with the next line and be under the length limit")));
   Attrs_.insert(WarningPair(CopyCtorConstructsBase,
      WarningAttrs(F, false,
      "Copy/move constructor does not invoke base copy/move constructor")));
   Attrs_.insert(WarningPair(ValueArgumentModified,
      WarningAttrs(F, false,
      "Argument passed by value is modified")));
   Attrs_.insert(WarningPair(ReturnsNonConstMember,
      WarningAttrs(F, true,
      "Function returns non-const reference or pointer to member data")));
   Attrs_.insert(WarningPair(FunctionCouldBeMember,
      WarningAttrs(F, false,
      "Static member function has indirect argument for its class")));
   Attrs_.insert(WarningPair(ExplicitConstructor,
      WarningAttrs(T, false,
      "Constructor does not require explicit tag")));
   Attrs_.insert(WarningPair(BitwiseOperatorOnBoolean,
      WarningAttrs(F, true,
      "Operator | or & used on boolean")));
   Attrs_.insert(WarningPair(DebugFtCanBeLiteral,
      WarningAttrs(T, false,
      "Function name passed to Debug::ft could be inlined string literal")));
   Attrs_.insert(WarningPair(UnnecessaryCast,
      WarningAttrs(F, true,
      "Non-const cast is not a downcast")));
   Attrs_.insert(WarningPair(ExcessiveCast,
      WarningAttrs(F, true,
      "Use static_cast or dynamic_cast instead of more severe cast")));
   Attrs_.insert(WarningPair(DataCouldBeFree,
      WarningAttrs(T, false,
      "Data could be free")));
   Attrs_.insert(WarningPair(ConstructorNotPrivate,
      WarningAttrs(T, true,
      "Singleton's constructor should be private")));
   Attrs_.insert(WarningPair(DestructorNotPrivate,
      WarningAttrs(T, true,
      "Singleton's destructor should be private")));
   Attrs_.insert(WarningPair(RedundantScope,
      WarningAttrs(T, true,
      "Redundant scope")));
   Attrs_.insert(WarningPair(PreprocessorDirective,
      WarningAttrs(F, false,
      "C-style preprocessor directive")));
   Attrs_.insert(WarningPair(OperatorSpacing,
      WarningAttrs(T, false,
      "Add/remove spaces before/after operator")));
   Attrs_.insert(WarningPair(PunctuationSpacing,
      WarningAttrs(T, false,
      "Add/remove spaces before/after punctuation")));
   Attrs_.insert(WarningPair(CopyCtorNotDeleted,
      WarningAttrs(T, false,
      "Copy constructor should be deleted")));
   Attrs_.insert(WarningPair(CopyOperNotDeleted,
      WarningAttrs(T, false,
      "Copy operator should be deleted")));
   Attrs_.insert(WarningPair(CtorCouldBeDeleted,
      WarningAttrs(T, false,
      "Constructor could be deleted")));
   Attrs_.insert(WarningPair(NoJumpOrFallthrough,
      WarningAttrs(F, false,
      "No jump or fallthrough")));
   Attrs_.insert(WarningPair(OverrideNotSorted,
      WarningAttrs(T, false,
      "Override not sorted in standard order")));
   Attrs_.insert(WarningPair(DataShouldBeStatic,
      WarningAttrs(T, false,
      "Data at .cpp file scope is neither static nor extern")));
   Attrs_.insert(WarningPair(FunctionShouldBeStatic,
      WarningAttrs(T, false,
      "Function at .cpp file scope is neither static nor extern")));
   Attrs_.insert(WarningPair(FunctionCouldBeDemoted,
      WarningAttrs(F, true,
      "Function could be moved to a subclass")));
   Attrs_.insert(WarningPair(Warning_N,
      WarningAttrs(F, false,
      ERROR_STR)));
}

//------------------------------------------------------------------------------

void CodeWarning::Insert() const
{
   Debug::ft("CodeWarning.Insert");

   if(!Suppress())
   {
      File()->InsertWarning(*this);
   }
}

//------------------------------------------------------------------------------

bool CodeWarning::IsInformational() const
{
   return (offset_ < 0);
}

//------------------------------------------------------------------------------

bool CodeWarning::IsSortedByFile
   (const CodeWarning* log1, const CodeWarning* log2)
{
   auto result = strCompare(log1->File()->Path(), log2->File()->Path());
   if(result == -1) return true;
   if(result == 1) return false;
   if(log1->warning_ < log2->warning_) return true;
   if(log1->warning_ > log2->warning_) return false;
   if(log1->Pos() < log2->Pos()) return true;
   if(log1->Pos() > log2->Pos()) return false;
   if(log1->offset_ < log2->offset_) return true;
   if(log1->offset_ > log2->offset_) return false;
   if(log1->info_ < log2->info_) return true;
   if(log1->info_ > log2->info_) return false;
   return (log1 < log2);
}

//------------------------------------------------------------------------------

bool CodeWarning::IsSortedByType
   (const CodeWarning* log1, const CodeWarning* log2)
{
   if(log1->warning_ < log2->warning_) return true;
   if(log1->warning_ > log2->warning_) return false;
   auto result = strCompare(log1->File()->Path(), log2->File()->Path());
   if(result == -1) return true;
   if(result == 1) return false;
   if(log1->Pos() < log2->Pos()) return true;
   if(log1->Pos() > log2->Pos()) return false;
   if(log1->offset_ < log2->offset_) return true;
   if(log1->offset_ > log2->offset_) return false;
   if(log1->info_ < log2->info_) return true;
   if(log1->info_ > log2->info_) return false;
   return (log1 < log2);
}

//------------------------------------------------------------------------------

bool CodeWarning::IsSortedToFix
   (const CodeWarning& log1, const CodeWarning& log2)
{
   auto result = strCompare(log1.File()->Path(), log2.File()->Path());
   if(result == -1) return true;
   if(result == 1) return false;
   if(log1.Pos() < log2.Pos()) return true;
   if(log1.Pos() > log2.Pos()) return false;
   if(log1.offset_ > log2.offset_) return true;
   if(log1.offset_ < log2.offset_) return false;
   if(log1.warning_ < log2.warning_) return true;
   if(log1.warning_ > log2.warning_) return false;
   if(log1.info_ < log2.info_) return true;
   if(log1.info_ > log2.info_) return false;
   return (&log1 < &log2);
}

//------------------------------------------------------------------------------

void CodeWarning::ItemDeleted(const CxxToken* item) const
{
   if(item_ == item)
   {
      status_ = Nullified;
   }
}

//------------------------------------------------------------------------------

size_t CodeWarning::Line() const
{
   return loc_.GetFile()->GetLexer().GetLineNum(loc_.GetPos());
}

//------------------------------------------------------------------------------

std::vector< const CodeWarning* > CodeWarning::LogsToFix(string& expl) const
{
   Debug::ft("CodeWarning.LogsToFix");

   std::vector< const CodeWarning* > logs;
   const CodeWarning* log = nullptr;

   if(!Attrs_.at(warning_).fixable_)
   {
      expl = "Fixing this type of warning is not supported.";
      return logs;
   }

   switch(warning_)
   {
   case FunctionUnused:
   case ArgumentCouldBeConstRef:
   case ArgumentCouldBeConst:
   case FunctionCouldBeConst:
   case FunctionCouldBeStatic:
   case CouldBeNoexcept:
   case ShouldNotBeNoexcept:
   case FunctionCouldBeDefaulted:
      logs.push_back(this);
      log = FindMateLog(expl);
      if(log != nullptr) logs.push_back(log);
      break;

   default:
      logs.push_back(this);
   }

   return logs;
}

//------------------------------------------------------------------------------

bool CodeWarning::operator==(const CodeWarning& that) const
{
   if(this->warning_ != that.warning_) return false;
   if(this->item_ != that.item_) return false;
   if(this->File() != that.File()) return false;

   if(this->item_ == nullptr)
   {
      if(this->Pos() != that.Pos()) return false;
   }

   if(this->offset_ != that.offset_) return false;
   return (this->info_ == that.info_);
}

//------------------------------------------------------------------------------

bool CodeWarning::operator!=(const CodeWarning& that) const
{
   return !(*this == that);
}

//------------------------------------------------------------------------------

bool CodeWarning::Preserve() const
{
   switch(status_)
   {
   case Nullified:
      //
      //  The code associated with this warning was deleted, so discard it.
      //
      return false;

   case Pending:
   case Fixed:
      //
      //  Keep this warning.  It might be regenerated based on data gathered
      //  during compilation.  When this occurs, this instance will prevent a
      //  new version from being created and preserves its state.  (A pending
      //  warning might exist if a trap occurred before it could be committed,
      //  so treat it like a fixed warning.)
      //
      return true;
   }

   return Attrs_.at(warning_).preserve_;
}

//------------------------------------------------------------------------------

bool CodeWarning::Suppress() const
{
   auto& fn = File()->Name();

   switch(warning_)
   {
   case ArgumentUnused:
   {
      if(fn == "BaseBot.h") return true;
      break;
   }

   case DataUnused:
      if(fn == "Allocators.h") return true;
      break;

   case FunctionUnused:
      if(fn == "Allocators.h") return true;
      if(fn == "BaseBot.h") return true;
      if(fn == "MapAndUnits.h") return true;
      break;

   case TypedefUnused:
      if(fn == "Allocators.h") return true;
      break;

   case ItemCouldBePrivate:
      if(fn == "BaseBot.h") return true;
      if(fn == "MapAndUnits.h") return true;
      break;

   case DataInitOnly:
   {
      auto data = static_cast< const Data* >(item_);
      auto type = data->GetTypeSpec()->TypeString(true);
      if(type.find("FunctionGuard") != string::npos) return true;
      if(type.find("MutexGuard") != string::npos) return true;
      break;
   }

   case DataNotPrivate:
      if(fn == "BaseBot.h") return true;
      if(fn == "MapAndUnits.h") return true;
      break;

   case FunctionNotOverridden:
      if(fn == "BaseBot.h") return true;
      if(fn == "BcSessions.h") return true;
      if(fn == "ProxyBcSessions.h") return true;
      break;

   case FunctionCouldBeStatic:
   case FunctionCouldBeFree:
   case CouldBeNoexcept:
      if(fn == "Allocators.h") return true;
      break;

   case LineLength:
      if(fn.find("CliParms.cpp") != string::npos) return true;
      break;

   case HeadingNotStandard:
   {
      auto dir = File()->Dir();
      return ((dir != nullptr) && (dir->Path().find("/dip") != string::npos));
   }

   case DebugFtNotInvoked:
   {
      auto func = static_cast< Function* >(item_);
      auto impl = func->GetImpl();
      if(impl == nullptr) return true;
      if(impl->FirstStatement() == nullptr) return true;
      if(func->IsInternal()) return true;
      if(func->IsPureVirtual()) return true;

      auto file = func->GetImplFile();
      if(file != nullptr)
      {
         if(fn == "Algorithms.cpp") return true;
         if(fn == "Duration.cpp") return true;
         if(fn == "Formatters.cpp") return true;
         if(fn == "TimePoint.cpp") return true;
         if(fn == "TraceRecord.cpp") return true;
         if(fn == "CxxString.cpp") return true;
      }

      auto& name = func->Name();

      if(name.find("Display") == 0) return true;
      if(name.find("Print") == 0) return true;
      if(name.find("Output") == 0) return true;
      if(name.find("Show") == 0) return true;
      if(name.find("CellDiff") == 0) return true;
      if(name.find("LinkDiff") == 0) return true;
      if(name.find("Shrink") == 0) return true;
      if(name.compare("Patch") == 0) return true;
      if(name.compare("CreateText") == 0) return true;
      if(name.compare("CreateCliParm") == 0) return true;
      if(name.compare("UpdateXref") == 0) return true;
      if(name.compare("PosToItem") == 0) return true;
      if(name.compare("Check") == 0) return true;
      if(name.compare("GetUsages") == 0) return true;
      if(name.compare("InLine") == 0) return true;
      if(name.compare("TypeString") == 0) return true;
      if(name.compare("Trace") == 0) return true;
      if(name.compare("UpdatePos") == 0) return true;
      if(name.compare("operator<<") == 0) return true;
      if(name.compare("operator==") == 0) return true;
      if(name.compare("operator!=") == 0) return true;
      if(name.compare("operator<") == 0) return true;

      auto spec = func->GetTypeSpec();

      if(spec != nullptr)
      {
         auto type = spec->TypeString(true);
         if(type.find("string") != string::npos) return true;
         if(type.find("char*") != string::npos) return true;
      }

      auto cls = func->GetClass();
      if(cls != nullptr)
      {
         if(cls->DerivesFrom("TraceRecord")) return true;

         auto ft = func->FuncType();
         if((ft == FuncCtor) || (ft == FuncDtor))
         {
            if(cls->DerivesFrom("CliIntParm")) return true;
            if(cls->DerivesFrom("CliBoolParm")) return true;
            if(cls->DerivesFrom("CliText")) return true;
            if(cls->DerivesFrom("CliTextParm")) return true;
            if(cls->DerivesFrom("CliCharParm")) return true;
            if(cls->DerivesFrom("CliPtrParm")) return true;
            if(cls->DerivesFrom("EventHandler")) return true;
            if(cls->DerivesFrom("Signal")) return true;
            if(cls->DerivesFrom("Parameter")) return true;
            if(cls->DerivesFrom("BcState")) return true;
            if(cls->DerivesFrom("PotsFeature")) return (name == "Attrs");
         }
      }
      break;
   }

   case DisplayNotOverridden:
   {
      auto cls = static_cast< const Class* >(item_);

      //  Leaf classes for modules, statistics, and tools do not need
      //  to override Display.
      //
      if(!cls->IsBaseClass())
      {
         if(cls->DerivesFrom("Module")) return true;
         if(cls->DerivesFrom("Statistic")) return true;
         if(cls->DerivesFrom("StatisticsGroup")) return true;
         if(cls->DerivesFrom("Tool")) return true;
      }

      //  Classes that represent C++ language elements do not need to
      //  override Display.
      //
      if(cls->DerivesFrom("CxxToken")) return true;
      break;
   }

   case PatchNotOverridden:
   {
      //  Classes in example applications do not need to override Patch.
      //
      auto cls = static_cast< const Class* >(item_);
      auto& sname = cls->GetSpace()->Name();

      if((sname != "NodeBase") &&
         (sname != "NetworkBase") &&
         (sname != "SessionBase"))
      {
         return true;
      }

      //  Simple leaf classes do not need to override Patch.
      //
      if(!cls->IsBaseClass())
      {
         if(cls->DerivesFrom("CliParm")) return true;
         if(cls->DerivesFrom("CfgParm")) return true;
         if(cls->DerivesFrom("Statistic")) return true;
         if(cls->DerivesFrom("StatisticsGroup")) return true;
         if(cls->DerivesFrom("Tool")) return true;
         if(cls->DerivesFrom("EventHandler")) return true;
      }
      break;
   }

   case ShouldNotBeNoexcept:
      if(fn == "Allocators.h") return true;

      //  Placement delete must be noexcept.
      //
      if(item_->Name().find("operator delete") == 0)
      {
         auto func = static_cast< const Function* >(item_);
         return (func->MinArgs() > 1);
      }
      break;

   case RemoveLineBreak:
      if(fn == "BcStates.cpp") return true;
      if(fn == "CodeWarning.cpp") return true;
      break;

   case PreprocessorDirective:
      if(item_->Name() == "FIELD_LOAD") return true;
      break;

   case OperatorSpacing:
   case PunctuationSpacing:
      if(fn == "Cxx.cpp")
      {
         auto& lexer = loc_.GetFile()->GetLexer();
         auto size = strlen("CxxOp");
         auto code = lexer.Substr(loc_.GetPos() - size, size);
         if(code == "CxxOp") return true;
      }
      break;
   }

   return false;
}

//------------------------------------------------------------------------------

void CodeWarning::UpdatePos
   (EditorAction action, size_t begin, size_t count, size_t from) const
{
   loc_.UpdatePos(action, begin, count, from);
}

//------------------------------------------------------------------------------

bool CodeWarning::WasResolved() const
{
   return (status_ == Fixed) || (status_ == Nullified);
}
}
