//==============================================================================
//
//  CodeWarning.cpp
//
//  Copyright (C) 2013-2020  Greg Utas
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
#include <list>
#include <sstream>
#include "CodeDir.h"
#include "CodeFile.h"
#include "CodeFileSet.h"
#include "CxxArea.h"
#include "CxxNamed.h"
#include "CxxRoot.h"
#include "CxxScope.h"
#include "CxxScoped.h"
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
ostream& operator<<(ostream& stream, Warning warning)
{
   if((warning >= 0) && (warning < Warning_N))
      stream << CodeWarning::Expl(warning);
   else
      stream << CodeWarning::Expl(Warning_N);
   return stream;
}

//------------------------------------------------------------------------------

WarningAttrs::WarningAttrs(bool fix, uint8_t order, fixed_string expl) :
   fixable(fix),
   order(order),
   expl(expl)
{
}

//------------------------------------------------------------------------------

string WarningCode(Warning warning)
{
   std::ostringstream stream;

   stream << 'W' << setw(3) << std::setfill('0') << int(warning);
   return stream.str();
}

//==============================================================================

CodeWarning::AttrsMap CodeWarning::Attrs_ = AttrsMap();

size_t CodeWarning::LineTypeCounts_[] = { 0 };

size_t CodeWarning::WarningCounts_[] = { 0 };

std::vector< CodeWarning > CodeWarning::Warnings_ =
   std::vector< CodeWarning >();

//------------------------------------------------------------------------------

CodeWarning::CodeWarning(Warning warning, const CodeFile* file,
   size_t line, size_t pos, const CxxNamed* item, word offset,
   const string& info, bool hide) :
   warning_(warning),
   file_(file),
   line_(line),
   pos_(pos),
   item_(item),
   offset_(offset),
   info_(info),
   hide_(hide),
   status(NotSupported)
{
   Debug::ft("CodeWarning.ctor");

   if(Attrs_.at(warning).fixable) status = NotFixed;
}

//------------------------------------------------------------------------------

CodeWarning* CodeWarning::FindMateLog(std::string& expl) const
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
   return mateFile->FindLog(*this, mate, offset_, expl);
}

//------------------------------------------------------------------------------

CodeWarning* CodeWarning::FindRootLog(std::string& expl)
{
   Debug::ft("CodeWarning.FindRootLog");

   //  This warning is logged against the location that invokes a special
   //  member function and that class that did not declare that function.
   //  If its offset_ specifies that it is the former (-1), find the log
   //  associated with the class, which is the one to fix.  The logs could
   //  both appear in the same file, so offset_ is used to distinguish the
   //  logs so that the fix is only applied once.
   //
   if(offset_ == 0) return this;

   auto rootFile = item_->GetFile();
   if(rootFile == nullptr)
   {
      expl = "Class's file not found";
      return nullptr;
   }

   auto log = rootFile->FindLog(*this, item_, 0, expl);
   if(log == nullptr)
   {
      if(rootFile->IsSubsFile())
      {
         expl = "Cannot add a special member function to external class ";
         expl += *item_->Name() + '.';
      }
      else
      {
         expl = "Class's log not found";
      }
      return nullptr;
   }

   return log;
}

//------------------------------------------------------------------------------

word CodeWarning::FindWarning(const CodeWarning& log)
{
   for(size_t i = 0; i < Warnings_.size(); ++i)
   {
      if(Warnings_.at(i) == log) return i;
   }

   return -1;
}

//------------------------------------------------------------------------------

void CodeWarning::GenerateReport(ostream* stream, const SetOfIds& set)
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
   auto checkSet = new SetOfIds(set);
   auto checkFiles = new CodeFileSet(LibrarySet::TemporaryName(), checkSet);
   auto order = checkFiles->SortInBuildOrder();

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

   std::vector< CodeWarning > warnings;

   //  Count the total number of warnings of each type that appear in files
   //  belonging to the original SET, extracting them into the local set of
   //  warnings.  Exclude warnings that are to be hidden.
   //
   for(auto item = Warnings_.cbegin(); item != Warnings_.cend(); ++item)
   {
      if(!item->hide_ && (set.find(item->file_->Fid()) != set.cend()))
      {
         ++WarningCounts_[item->warning_];
         warnings.push_back(*item);
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
         *stream << (Attrs_.at(Warning(w)).fixable ? '*' : SPACE);
         *stream << setw(5) << WarningCode(Warning(w)) << setw(6)
            << WarningCounts_[w] << spaces(2) << Warning(w) << CRLF;
      }
   }

   *stream << string(120, '=') << CRLF;
   *stream << "WARNINGS SORTED BY TYPE/FILE/LINE" << CRLF;

   //  Sort and output the warnings by warning type/file/line.
   //
   std::sort(warnings.begin(), warnings.end(), IsSortedByType);

   auto item = warnings.cbegin();
   auto last = warnings.cend();

   while(item != last)
   {
      auto w = item->warning_;
      *stream << WarningCode(w) << SPACE << w << CRLF;

      do
      {
         *stream << spaces(2) << item->file_->Path();
         *stream << '(' << item->line_ + 1;
         if(item->offset_ > 0) *stream << '/' << item->offset_;
         *stream << "): ";

         if(item->HasCodeToDisplay())
         {
            *stream << item->file_->GetLexer().GetNthLine(item->line_);
         }

         if(item->HasInfoToDisplay()) *stream << item->info_;
         *stream << CRLF;
         ++item;
      }
      while((item != last) && (item->warning_ == w));
   }

   *stream << string(120, '=') << CRLF;
   *stream << "WARNINGS SORTED BY FILE/TYPE/LINE" << CRLF;

   //  Sort and output the warnings by file/warning_ type/line.
   //
   std::sort(warnings.begin(), warnings.end(), IsSortedByFile);

   item = warnings.cbegin();
   last = warnings.cend();

   while(item != last)
   {
      auto f = item->file_;
      *stream << f->Path() << CRLF;

      do
      {
         auto w = item->warning_;
         *stream << spaces(2) << WarningCode(w) << SPACE << w << CRLF;

         do
         {
            *stream << spaces(4) << item->line_ + 1;
            if(item->offset_ > 0) *stream << '/' << item->offset_;
            *stream << ": ";

            if((item->line_ != 0) || item->info_.empty())
            {
               *stream << f->GetLexer().GetNthLine(item->line_);
               if(!item->info_.empty()) *stream << " // ";
            }

            *stream << item->info_ << CRLF;
            ++item;
         }
         while((item != last) && (item->warning_ == w) && (item->file_ == f));
      }
      while((item != last) && (item->file_ == f));
   }

   checkFiles->Release();
}

//------------------------------------------------------------------------------

string CodeWarning::GetNewFuncName(string& expl) const
{
   Debug::ft("CodeWarning.GetNewFuncName");

   switch(warning_)
   {
   case DisplayNotOverridden:
      return "Display";
   case PatchNotOverridden:
      return "Patch";
   }

   expl = "The function associated with this warning is unknown.";
   return EMPTY_STR;
}

//------------------------------------------------------------------------------

void CodeWarning::GetWarnings
   (const CodeFile* file, std::vector< CodeWarning >& warnings)
{
   Debug::ft("CodeWarning.GetWarnings");

   for(auto w = Warnings_.cbegin(); w != Warnings_.cend(); ++w)
   {
      if(w->file_ == file)
      {
         warnings.push_back(*w);
      }
   }
}

//------------------------------------------------------------------------------

bool CodeWarning::HasWarning(const CodeFile* file, Warning warning)
{
   for(auto w = Warnings_.cbegin(); w != Warnings_.cend(); ++w)
   {
      if(w->file_ == file)
      {
         if((warning == AllWarnings) || (w->warning_ == warning)) return true;
      }
   }

   return false;
}

//------------------------------------------------------------------------------

void CodeWarning::Initialize()
{
   Debug::ft("CodeWarning.Initialize");

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

   Attrs_.insert(WarningPair(AllWarnings,
      WarningAttrs(T, X,
      "All warnings")));
   Attrs_.insert(WarningPair(UseOfNull,
      WarningAttrs(T, R,
      "Use of NULL")));
   Attrs_.insert(WarningPair(PtrTagDetached,
      WarningAttrs(T, R,
      "Pointer tag ('*') detached from type")));
   Attrs_.insert(WarningPair(RefTagDetached,
      WarningAttrs(T, R,
      "Reference tag ('&') detached from type")));
   Attrs_.insert(WarningPair(UseOfCast,
      WarningAttrs(F, X,
      "C-style cast")));
   Attrs_.insert(WarningPair(FunctionalCast,
      WarningAttrs(F, X,
      "Functional cast")));
   Attrs_.insert(WarningPair(ReinterpretCast,
      WarningAttrs(F, X,
      "reinterpret_cast")));
   Attrs_.insert(WarningPair(Downcasting,
      WarningAttrs(F, X,
      "Cast down the inheritance hierarchy")));
   Attrs_.insert(WarningPair(CastingAwayConstness,
      WarningAttrs(F, X,
      "Cast removes const qualification")));
   Attrs_.insert(WarningPair(PointerArithmetic,
      WarningAttrs(F, X,
      "Pointer arithmetic")));
   Attrs_.insert(WarningPair(RedundantSemicolon,
      WarningAttrs(T, E,
      "Semicolon not required")));
   Attrs_.insert(WarningPair(RedundantConst,
      WarningAttrs(T, E,
      "Redundant const in type specification")));
   Attrs_.insert(WarningPair(DefineNotAtFileScope,
      WarningAttrs(T, U,
      "#define appears within a class or function")));
   Attrs_.insert(WarningPair(IncludeFollowsCode,
      WarningAttrs(T, U,
      "#include appears after code")));
   Attrs_.insert(WarningPair(IncludeGuardMissing,
      WarningAttrs(T, C,
      "No #include guard found")));
   Attrs_.insert(WarningPair(IncludeNotSorted,
      WarningAttrs(T, U,
      "#include not sorted in standard order")));
   Attrs_.insert(WarningPair(IncludeDuplicated,
      WarningAttrs(T, D,
      "#include duplicated")));
   Attrs_.insert(WarningPair(IncludeAdd,
      WarningAttrs(T, C,
      "Add #include directive")));
   Attrs_.insert(WarningPair(IncludeRemove,
      WarningAttrs(T, D,
      "Remove #include directive")));
   Attrs_.insert(WarningPair(RemoveOverrideTag,
      WarningAttrs(T, E,
      "Remove override tag: function is final")));
   Attrs_.insert(WarningPair(UsingInHeader,
      WarningAttrs(T, I,
      "Using statement in header")));
   Attrs_.insert(WarningPair(UsingDuplicated,
      WarningAttrs(T, D,
      "Using statement duplicated")));
   Attrs_.insert(WarningPair(UsingAdd,
      WarningAttrs(T, C,
      "Add using statement")));
   Attrs_.insert(WarningPair(UsingRemove,
      WarningAttrs(T, D,
      "Remove using statement")));
   Attrs_.insert(WarningPair(ForwardAdd,
      WarningAttrs(T, C,
      "Add forward declaration")));
   Attrs_.insert(WarningPair(ForwardRemove,
      WarningAttrs(T, D,
      "Remove forward declaration")));
   Attrs_.insert(WarningPair(ArgumentUnused,
      WarningAttrs(T, E,
      "Unused argument")));
   Attrs_.insert(WarningPair(ClassUnused,
      WarningAttrs(T, D,
      "Unused class")));
   Attrs_.insert(WarningPair(DataUnused,
      WarningAttrs(T, D,
      "Unused data")));
   Attrs_.insert(WarningPair(EnumUnused,
      WarningAttrs(T, D,
      "Unused enum")));
   Attrs_.insert(WarningPair(EnumeratorUnused,
      WarningAttrs(T, D,
      "Unused enumerator")));
   Attrs_.insert(WarningPair(FriendUnused,
      WarningAttrs(T, D,
      "Unused friend declaration")));
   Attrs_.insert(WarningPair(FunctionUnused,
      WarningAttrs(T, D,
      "Unused function")));
   Attrs_.insert(WarningPair(TypedefUnused,
      WarningAttrs(T, D,
      "Unused typedef")));
   Attrs_.insert(WarningPair(ForwardUnresolved,
      WarningAttrs(T, D,
      "No referent for forward declaration")));
   Attrs_.insert(WarningPair(FriendUnresolved,
      WarningAttrs(T, D,
      "No referent for friend declaration")));
   Attrs_.insert(WarningPair(FriendAsForward,
      WarningAttrs(T, C,
      "Indirect reference relies on friend, not forward, declaration")));
   Attrs_.insert(WarningPair(HidesInheritedName,
      WarningAttrs(T, R,
      "Member hides inherited name")));
   Attrs_.insert(WarningPair(ClassCouldBeNamespace,
      WarningAttrs(T, R,
      "Class could be namespace")));
   Attrs_.insert(WarningPair(ClassCouldBeStruct,
      WarningAttrs(T, R,
      "Class could be struct")));
   Attrs_.insert(WarningPair(StructCouldBeClass,
      WarningAttrs(T, R,
      "Struct could be class")));
   Attrs_.insert(WarningPair(RedundantAccessControl,
      WarningAttrs(T, D,
      "Redundant access control")));
   Attrs_.insert(WarningPair(ItemCouldBePrivate,
      WarningAttrs(T, U,
      "Member could be private")));
   Attrs_.insert(WarningPair(ItemCouldBeProtected,
      WarningAttrs(T, U,
      "Member could be protected")));
   Attrs_.insert(WarningPair(PointerTypedef,
      WarningAttrs(F, X,
      "Typedef of pointer type")));
   Attrs_.insert(WarningPair(AnonymousEnum,
      WarningAttrs(T, I,
      "Anonymous enum")));
   Attrs_.insert(WarningPair(DataUninitialized,
      WarningAttrs(T, C,
      "Global data initialization not found")));
   Attrs_.insert(WarningPair(DataInitOnly,
      WarningAttrs(T, D,
      "Data is init-only")));
   Attrs_.insert(WarningPair(DataWriteOnly,
      WarningAttrs(T, D,
      "Data is write-only")));
   Attrs_.insert(WarningPair(GlobalStaticData,
      WarningAttrs(F, X,
      "Global static data")));
   Attrs_.insert(WarningPair(DataNotPrivate,
      WarningAttrs(F, X,
      "Data is not private")));
   Attrs_.insert(WarningPair(DataCannotBeConst,
      WarningAttrs(F, X,
      "DATA CANNOT BE CONST")));
   Attrs_.insert(WarningPair(DataCannotBeConstPtr,
      WarningAttrs(F, X,
      "DATA CANNOT BE CONST POINTER")));
   Attrs_.insert(WarningPair(DataCouldBeConst,
      WarningAttrs(T, I,
      "Data could be const")));
   Attrs_.insert(WarningPair(DataCouldBeConstPtr,
      WarningAttrs(T, I,
      "Data could be const pointer")));
   Attrs_.insert(WarningPair(DataNeedNotBeMutable,
      WarningAttrs(T, E,
      "Data need not be mutable")));
   Attrs_.insert(WarningPair(DefaultPODConstructor,
      WarningAttrs(T, C,
      "Default constructor invoked: POD members not initialized")));
   Attrs_.insert(WarningPair(DefaultConstructor,
      WarningAttrs(T, C,
      "Default constructor invoked")));
   Attrs_.insert(WarningPair(DefaultCopyConstructor,
      WarningAttrs(T, C,
      "Default copy constructor invoked")));
   Attrs_.insert(WarningPair(DefaultCopyOperator,
      WarningAttrs(T, C,
      "Default copy (assignment) operator invoked")));
   Attrs_.insert(WarningPair(PublicConstructor,
      WarningAttrs(F, X,
      "Base class constructor is public")));
   Attrs_.insert(WarningPair(NonExplicitConstructor,
      WarningAttrs(T, I,
      "Single-argument constructor is not explicit")));
   Attrs_.insert(WarningPair(MemberInitMissing,
      WarningAttrs(T, C,
      "Member not included in member initialization list")));
   Attrs_.insert(WarningPair(MemberInitNotSorted,
      WarningAttrs(T, U,
      "Member not sorted in standard order in member initialization list")));
   Attrs_.insert(WarningPair(DefaultDestructor,
      WarningAttrs(T, C,
      "Default destructor invoked")));
   Attrs_.insert(WarningPair(VirtualDestructor,
      WarningAttrs(T, U,
      "Base class virtual destructor is not public")));
   Attrs_.insert(WarningPair(NonVirtualDestructor,
      WarningAttrs(T, I,
      "Base class non-virtual destructor is public")));
   Attrs_.insert(WarningPair(VirtualFunctionInvoked,
      WarningAttrs(F, X,
      "Virtual function in own class invoked by constructor or destructor")));
   Attrs_.insert(WarningPair(RuleOf3DtorNoCopyCtor,
      WarningAttrs(F, X,
      "Destructor defined, but not copy constructor")));
   Attrs_.insert(WarningPair(RuleOf3DtorNoCopyOper,
      WarningAttrs(F, X,
      "Destructor defined, but not copy operator")));
   Attrs_.insert(WarningPair(RuleOf3CopyCtorNoOper,
      WarningAttrs(T, C,
      "Copy constructor defined, but not copy operator")));
   Attrs_.insert(WarningPair(RuleOf3CopyOperNoCtor,
      WarningAttrs(T, C,
      "Copy operator defined, but not copy constructor")));
   Attrs_.insert(WarningPair(OperatorOverloaded,
      WarningAttrs(F, X,
      "Overloading operator && or ||")));
   Attrs_.insert(WarningPair(FunctionNotDefined,
      WarningAttrs(T, D,
      "Function not implemented")));
   Attrs_.insert(WarningPair(PureVirtualNotDefined,
      WarningAttrs(T, C,
      "Pure virtual function not implemented")));
   Attrs_.insert(WarningPair(VirtualAndPublic,
      WarningAttrs(T, R,
      "Virtual function is public")));
   Attrs_.insert(WarningPair(BoolMixedWithNumeric,
      WarningAttrs(F, X,
      "Expression mixes bool with numeric")));
   Attrs_.insert(WarningPair(FunctionNotOverridden,
      WarningAttrs(T, X,
      "Virtual function has no overrides")));
   Attrs_.insert(WarningPair(RemoveVirtualTag,
      WarningAttrs(T, E,
      "Remove virtual tag: function is an override or final")));
   Attrs_.insert(WarningPair(OverrideTagMissing,
      WarningAttrs(T, I,
      "Function should be tagged as override")));
   Attrs_.insert(WarningPair(VoidAsArgument,
      WarningAttrs(T, E,
      "\"(void)\" as function argument")));
   Attrs_.insert(WarningPair(AnonymousArgument,
      WarningAttrs(T, I,
      "Anonymous argument")));
   Attrs_.insert(WarningPair(AdjacentArgumentTypes,
      WarningAttrs(F, X,
      "Adjacent arguments have the same type")));
   Attrs_.insert(WarningPair(DefinitionRenamesArgument,
      WarningAttrs(T, R,
      "Definition renames argument in declaration")));
   Attrs_.insert(WarningPair(OverrideRenamesArgument,
      WarningAttrs(T, R,
      "Override renames argument in direct base class")));
   Attrs_.insert(WarningPair(VirtualDefaultArgument,
      WarningAttrs(T, R,
      "Virtual function defines default argument")));
   Attrs_.insert(WarningPair(ArgumentCannotBeConst,
      WarningAttrs(F, X,
      "ARGUMENT CANNOT BE CONST")));
   Attrs_.insert(WarningPair(ArgumentCouldBeConstRef,
      WarningAttrs(T, I,
      "Object could be passed by const reference")));
   Attrs_.insert(WarningPair(ArgumentCouldBeConst,
      WarningAttrs(T, I,
      "Argument could be const")));
   Attrs_.insert(WarningPair(FunctionCannotBeConst,
      WarningAttrs(F, X,
      "FUNCTION CANNOT BE CONST")));
   Attrs_.insert(WarningPair(FunctionCouldBeConst,
      WarningAttrs(T, I,
      "Function could be const")));
   Attrs_.insert(WarningPair(FunctionCouldBeStatic,
      WarningAttrs(T, I,
      "Function could be static")));
   Attrs_.insert(WarningPair(FunctionCouldBeFree,
      WarningAttrs(T, R,
      "Function could be free")));
   Attrs_.insert(WarningPair(StaticFunctionViaMember,
      WarningAttrs(T, R,
      "Static function invoked via operator \".\" or \"->\"")));
   Attrs_.insert(WarningPair(NonBooleanConditional,
      WarningAttrs(F, X,
      "Non-boolean in conditional expression")));
   Attrs_.insert(WarningPair(EnumTypesDiffer,
      WarningAttrs(F, X,
      "Arguments to binary operator have different enum types")));
   Attrs_.insert(WarningPair(UseOfTab,
      WarningAttrs(T, R,
      "Tab character in source code")));
   Attrs_.insert(WarningPair(Indentation,
      WarningAttrs(T, R,
      "Line indentation is not a multiple of the standard value")));
   Attrs_.insert(WarningPair(TrailingSpace,
      WarningAttrs(T, E,
      "Line contains trailing space")));
   Attrs_.insert(WarningPair(AdjacentSpaces,
      WarningAttrs(T, E,
      "Line contains adjacent spaces")));
   Attrs_.insert(WarningPair(AddBlankLine,
      WarningAttrs(T, C,
      "Insertion of blank line recommended")));
   Attrs_.insert(WarningPair(RemoveBlankLine,
      WarningAttrs(T, D,
      "Deletion of blank line recommended")));
   Attrs_.insert(WarningPair(LineLength,
      WarningAttrs(T, S,
      "Line length exceeds the standard maximum")));
   Attrs_.insert(WarningPair(FunctionNotSorted,
      WarningAttrs(T, U,
      "Function not sorted in standard order")));
   Attrs_.insert(WarningPair(HeadingNotStandard,
      WarningAttrs(T, C,
      "File heading is not standard")));
   Attrs_.insert(WarningPair(IncludeGuardMisnamed,
      WarningAttrs(T, R,
      "Name of #include guard is not standard")));
   Attrs_.insert(WarningPair(DebugFtNotInvoked,
      WarningAttrs(T, C,
      "Function does not invoke Debug::ft")));
   Attrs_.insert(WarningPair(DebugFtNotFirst,
      WarningAttrs(F, X,
      "Function does not invoke Debug::ft as first statement")));
   Attrs_.insert(WarningPair(DebugFtNameMismatch,
      WarningAttrs(T, R,
      "Function name passed to Debug::ft is not standard")));
   Attrs_.insert(WarningPair(DebugFtNameDuplicated,
      WarningAttrs(T, R,
      "Function name passed to Debug::ft is used by another function")));
   Attrs_.insert(WarningPair(DisplayNotOverridden,
      WarningAttrs(T, C,
      "Override of Base.Display not found")));
   Attrs_.insert(WarningPair(PatchNotOverridden,
      WarningAttrs(T, C,
      "Override of Object.Patch not found")));
   Attrs_.insert(WarningPair(FunctionCouldBeDefaulted,
      WarningAttrs(T, R,
      "Function could be defaulted")));
   Attrs_.insert(WarningPair(InitCouldUseConstructor,
      WarningAttrs(T, R,
      "Initialization uses assignment operator")));
   Attrs_.insert(WarningPair(CouldBeNoexcept,
      WarningAttrs(T, I,
      "Function could be tagged noexcept")));
   Attrs_.insert(WarningPair(ShouldNotBeNoexcept,
      WarningAttrs(T, I,
      "Function should not be tagged noexcept")));
   Attrs_.insert(WarningPair(UseOfSlashAsterisk,
      WarningAttrs(T, R,
      "C-style comment")));
   Attrs_.insert(WarningPair(RemoveLineBreak,
      WarningAttrs(T, J,
      "Line can merge with the next line and be under the length limit")));
   Attrs_.insert(WarningPair(CopyCtorConstructsBase,
      WarningAttrs(T, C,
      "Copy/move constructor does not invoke base copy/move constructor")));
   Attrs_.insert(WarningPair(ValueArgumentModified,
      WarningAttrs(F, X,
      "Argument passed by value is modified")));
   Attrs_.insert(WarningPair(ReturnsNonConstMember,
      WarningAttrs(F, X,
      "Function returns non-const reference or pointer to member data")));
   Attrs_.insert(WarningPair(FunctionCouldBeMember,
      WarningAttrs(T, R,
      "Function could be a member of a class that is an indirect argument")));
   Attrs_.insert(WarningPair(ExplicitConstructor,
      WarningAttrs(T, E,
      "Constructor does not require explicit tag")));
   Attrs_.insert(WarningPair(BitwiseOperatorOnBoolean,
      WarningAttrs(T, R,
      "Operator | or & used on boolean")));
   Attrs_.insert(WarningPair(DebugFtCanBeLiteral,
      WarningAttrs(T, R,
      "Function name passed to Debug::ft could be inlined string literal")));
   Attrs_.insert(WarningPair(Warning_N,
      WarningAttrs(F, X,
      ERROR_STR)));
}

//------------------------------------------------------------------------------

void CodeWarning::Insert() const
{
   Debug::ft("CodeWarning.Insert");

   if((FindWarning(*this) < 0) && !Suppress())
   {
      Warnings_.push_back(*this);
   }
}

//------------------------------------------------------------------------------

bool CodeWarning::IsSortedByFile
   (const CodeWarning& log1, const CodeWarning& log2)
{
   auto result = strCompare(log1.file_->Path(), log2.file_->Path());
   if(result == -1) return true;
   if(result == 1) return false;
   if(log1.warning_ < log2.warning_) return true;
   if(log1.warning_ > log2.warning_) return false;
   if(log1.line_ < log2.line_) return true;
   if(log1.line_ > log2.line_) return false;
   if(log1.offset_ < log2.offset_) return true;
   if(log1.offset_ > log2.offset_) return false;
   if(log1.info_ < log2.info_) return true;
   if(log1.info_ > log2.info_) return false;
   return (&log1 < &log2);
}

//------------------------------------------------------------------------------

bool CodeWarning::IsSortedByType
   (const CodeWarning& log1, const CodeWarning& log2)
{
   if(log1.warning_ < log2.warning_) return true;
   if(log1.warning_ > log2.warning_) return false;
   auto result = strCompare(log1.file_->Path(), log2.file_->Path());
   if(result == -1) return true;
   if(result == 1) return false;
   if(log1.line_ < log2.line_) return true;
   if(log1.line_ > log2.line_) return false;
   if(log1.offset_ < log2.offset_) return true;
   if(log1.offset_ > log2.offset_) return false;
   if(log1.info_ < log2.info_) return true;
   if(log1.info_ > log2.info_) return false;
   return (&log1 < &log2);
}

//------------------------------------------------------------------------------

bool CodeWarning::IsSortedToFix
   (const CodeWarning& log1, const CodeWarning& log2)
{
   auto result = strCompare(log1.file_->Path(), log2.file_->Path());
   if(result == -1) return true;
   if(result == 1) return false;
   if(Attrs_.at(log1.warning_).order < Attrs_.at(log2.warning_).order)
      return true;
   if(Attrs_.at(log1.warning_).order > Attrs_.at(log2.warning_).order)
      return false;
   if(log1.line_ < log2.line_) return true;
   if(log1.line_ > log2.line_) return false;
   if(log1.pos_ > log2.pos_) return true;
   if(log1.pos_ < log2.pos_) return false;
   if(log1.offset_ > log2.offset_) return true;
   if(log1.offset_ < log2.offset_) return false;
   if(log1.warning_ < log2.warning_) return true;
   if(log1.warning_ > log2.warning_) return false;
   if(log1.info_ < log2.info_) return true;
   if(log1.info_ > log2.info_) return false;
   return (&log1 < &log2);
}

//------------------------------------------------------------------------------

std::vector< CodeWarning* > CodeWarning::LogsToFix(std::string& expl)
{
   Debug::ft("CodeWarning.LogsToFix");

   std::vector< CodeWarning* > logs;
   CodeWarning* log = nullptr;

   if(!Attrs_.at(warning_).fixable)
   {
      expl = "Fixing this type of warning is not supported.";
      return logs;
   }

   switch(warning_)
   {
   case DefaultConstructor:
   case DefaultCopyConstructor:
   case DefaultCopyOperator:
   case DefaultDestructor:
      logs.push_back(this);
      log = FindRootLog(expl);
      if(log != nullptr) logs.push_back(log);
      break;

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
   if(this->file_ != that.file_) return false;
   if(this->line_ != that.line_) return false;
   if(this->pos_ != that.pos_) return false;
   if(this->warning_ != that.warning_) return false;
   if(this->offset_ != that.offset_) return false;
   return (this->info_ == that.info_);
}

//------------------------------------------------------------------------------

bool CodeWarning::operator!=(const CodeWarning& that) const
{
   return !(*this == that);
}

//------------------------------------------------------------------------------

bool CodeWarning::Suppress() const
{
   auto& fn = file_->Name();

   switch(warning_)
   {
   case ArgumentUnused:
   {
      if(fn == "BaseBot.h") return true;
      auto func = static_cast< const Function* >(item_);
      auto index = func->LogOffsetToArgIndex(offset_);
      auto& arg = func->GetArgs().at(index);
      if(*arg->GetTypeSpec()->Name() == "nothrow_t") return true;
      break;
   }

   case DataUnused:
      if(fn == "Allocators.h") return true;
      break;

   case FunctionUnused:
      if(item_->Name()->find("operator new") == 0) return true;
      if(item_->Name()->find("operator delete") == 0) return true;
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

   case AnonymousArgument:
   {
      if(*item_->Name() == "operator++") return true;
      if(*item_->Name() == "operator--") return true;
      auto func = static_cast< const Function* >(item_);
      auto index = func->LogOffsetToArgIndex(offset_);
      auto& arg = func->GetArgs().at(index);
      if(*arg->GetTypeSpec()->Name() == "nothrow_t") return true;
      break;
   }

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
      auto dir = file_->Dir();
      return ((dir != nullptr) && (dir->Path().find("/dip") != string::npos));
   }

   case DebugFtNotInvoked:
   {
      auto func = static_cast< const Function* >(item_);
      auto impl = func->GetImpl();
      if(impl == nullptr) return true;
      if(impl->FirstStatement() == nullptr) return true;
      if(func->IsInTemplateInstance()) return true;
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

      auto name = func->Name();

      if(name->find("Display") == 0) return true;
      if(name->find("Print") == 0) return true;
      if(name->find("Output") == 0) return true;
      if(name->find("Show") == 0) return true;
      if(name->find("CellDiff") == 0) return true;
      if(name->find("LinkDiff") == 0) return true;
      if(name->find("Shrink") == 0) return true;
      if(name->compare("Patch") == 0) return true;
      if(name->compare("CreateText") == 0) return true;
      if(name->compare("CreateCliParm") == 0) return true;
      if(name->compare("AddToXref") == 0) return true;
      if(name->compare("Check") == 0) return true;
      if(name->compare("GetUsages") == 0) return true;
      if(name->compare("TypeString") == 0) return true;
      if(name->compare("operator<<") == 0) return true;
      if(name->compare("operator==") == 0) return true;
      if(name->compare("operator!=") == 0) return true;
      if(name->compare("operator<") == 0) return true;

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
            if(cls->DerivesFrom("PotsFeature")) return (*name == "Attrs");
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
      if(cls->Subclasses()->empty())
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
      auto sname = cls->GetSpace()->Name();

      if((*sname != "NodeBase") &&
         (*sname != "NetworkBase") &&
         (*sname != "SessionBase"))
      {
         return true;
      }

      //  Simple leaf classes do not need to override Patch.
      //
      if(cls->Subclasses()->empty())
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
      if(item_->Name()->find("operator delete") == 0)
      {
         auto func = static_cast< const Function* >(item_);
         return (func->MinArgs() > 1);
      }
      break;

   case RemoveLineBreak:
      if(fn == "BcStates.cpp") return true;
      if(fn == "CodeWarning.cpp") return true;
      break;
   }

   return false;
}
}
