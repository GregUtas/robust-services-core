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
#include "CodeFile.h"
#include "CodeFileSet.h"
#include "CxxArea.h"
#include "CxxRoot.h"
#include "CxxScope.h"
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

WarningAttrs::WarningAttrs
   (bool fix, uint8_t order, bool unused, fixed_string expl) noexcept :
   fixable(fix),
   order(order),
   unusedItem(unused),
   suppressed(false),
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

fn_name CodeWarning_ctor = "CodeWarning.ctor";

CodeWarning::CodeWarning(Warning warning, const CodeFile* file,
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
   Debug::ft(CodeWarning_ctor);

   if(Attrs_.at(warning).fixable) status = NotFixed;
}

//------------------------------------------------------------------------------

fn_name CodeWarning_FindMateLog = "CodeWarning.FindMateLog";

CodeWarning* CodeWarning::FindMateLog(std::string& expl) const
{
   Debug::ft(CodeWarning_FindMateLog);

   //  Look for the mate item associated with this log.  Find its file and
   //  editor, and ask its editor find the log that corresponds to this one.
   //
   auto mate = item->GetMate();
   if(mate == nullptr) return nullptr;
   auto mateFile = mate->GetFile();
   if(mateFile == nullptr)
   {
      expl = "Mate's file not found";
      return nullptr;
   }
   return mateFile->FindLog(*this, mate, offset, expl);
}

//------------------------------------------------------------------------------

fn_name CodeWarning_FindRootLog = "CodeWarning.FindRootLog";

CodeWarning* CodeWarning::FindRootLog(std::string& expl)
{
   Debug::ft(CodeWarning_FindRootLog);

   //  This warning is logged against the location that invokes a special
   //  member function and that class that did not declare that function.
   //  If its .offset specifies that it is the former (-1), find the log
   //  associated with the class, which is the one to fix.  The logs could
   //  both appear in the same file, so .offset is used to distinguish the
   //  logs so that the fix is only applied once.
   //
   if(offset == 0) return this;

   auto rootFile = item->GetFile();
   if(rootFile == nullptr)
   {
      expl = "Class's file not found";
      return nullptr;
   }

   auto log = rootFile->FindLog(*this, item, 0, expl);
   if(log == nullptr)
   {
      if(rootFile->IsSubsFile())
      {
         expl = "Cannot add a special member function to external class ";
         expl += *item->Name() + '.';
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

fn_name CodeWarning_GenerateReport = "CodeWarning.GenerateReport";

void CodeWarning::GenerateReport(ostream* stream, const SetOfIds& set)
{
   Debug::ft(CodeWarning_GenerateReport);

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

   std::vector< CodeWarning > warnings;

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
      *stream << setw(8) << LineTypeCounts_[t]
         << spaces(3) << LineType(t) << CRLF;
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
   std::sort(warnings.begin(), warnings.end(), IsSortedByType);

   auto item = warnings.cbegin();
   auto last = warnings.cend();

   while(item != last)
   {
      auto w = item->warning;
      *stream << WarningCode(w) << SPACE << w << CRLF;

      do
      {
         *stream << spaces(2) << item->file->Path();
         *stream << '(' << item->line + 1;
         if(item->offset > 0) *stream << '/' << item->offset;
         *stream << "): ";

         if(item->HasCodeToDisplay())
         {
            *stream << item->file->GetLexer().GetNthLine(item->line);
         }

         if(item->HasInfoToDisplay()) *stream << item->info;
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
      *stream << f->Path() << CRLF;

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
               if(!item->info.empty()) *stream << " // ";
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

fn_name CodeWarning_GetWarnings = "CodeWarning.GetWarnings";

void CodeWarning::GetWarnings
   (const CodeFile* file, std::vector< CodeWarning >& warnings)
{
   Debug::ft(CodeWarning_GetWarnings);

   for(auto w = Warnings_.cbegin(); w != Warnings_.cend(); ++w)
   {
      if(w->file == file)
      {
         warnings.push_back(*w);
      }
   }
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

   Attrs_.insert(WarningPair(AllWarnings,
      WarningAttrs(T, X, F,
      "All warnings")));
   Attrs_.insert(WarningPair(UseOfNull,
      WarningAttrs(T, R, F,
      "Use of NULL")));
   Attrs_.insert(WarningPair(PtrTagDetached,
      WarningAttrs(T, R, F,
      "Pointer tag ('*') detached from type")));
   Attrs_.insert(WarningPair(RefTagDetached,
      WarningAttrs(T, R, F,
      "Reference tag ('&') detached from type")));
   Attrs_.insert(WarningPair(UseOfCast,
      WarningAttrs(F, X, F,
      "C-style cast")));
   Attrs_.insert(WarningPair(FunctionalCast,
      WarningAttrs(F, X, F,
      "Functional cast")));
   Attrs_.insert(WarningPair(ReinterpretCast,
      WarningAttrs(F, X, F,
      "reinterpret_cast")));
   Attrs_.insert(WarningPair(Downcasting,
      WarningAttrs(F, X, F,
      "Cast down the inheritance hierarchy")));
   Attrs_.insert(WarningPair(CastingAwayConstness,
      WarningAttrs(F, X, F,
      "Cast removes const qualification")));
   Attrs_.insert(WarningPair(PointerArithmetic,
      WarningAttrs(F, X, F,
      "Pointer arithmetic")));
   Attrs_.insert(WarningPair(RedundantSemicolon,
      WarningAttrs(T, E, F,
      "Semicolon not required")));
   Attrs_.insert(WarningPair(RedundantConst,
      WarningAttrs(T, E, F,
      "Redundant const in type specification")));
   Attrs_.insert(WarningPair(DefineNotAtFileScope,
      WarningAttrs(F, X, F,
      "#define appears within a class or function")));
   Attrs_.insert(WarningPair(IncludeNotAtGlobalScope,
      WarningAttrs(F, X, F,
      "No #include guard found")));
   Attrs_.insert(WarningPair(IncludeGuardMissing,
      WarningAttrs(T, C, F,
      "#include appears outside of global namespace")));
   Attrs_.insert(WarningPair(IncludeNotSorted,
      WarningAttrs(T, U, F,
      "#include not sorted in standard order")));
   Attrs_.insert(WarningPair(IncludeDuplicated,
      WarningAttrs(T, D, F,
      "#include duplicated")));
   Attrs_.insert(WarningPair(IncludeAdd,
      WarningAttrs(T, C, F,
      "Add #include directive")));
   Attrs_.insert(WarningPair(IncludeRemove,
      WarningAttrs(T, D, F,
      "Remove #include directive")));
   Attrs_.insert(WarningPair(RemoveOverrideTag,
      WarningAttrs(T, E, F,
      "Remove override tag: function is final")));
   Attrs_.insert(WarningPair(UsingInHeader,
      WarningAttrs(T, I, F,
      "Using statement in header")));
   Attrs_.insert(WarningPair(UsingDuplicated,
      WarningAttrs(T, D, F,
      "Using statement duplicated")));
   Attrs_.insert(WarningPair(UsingAdd,
      WarningAttrs(T, C, F,
      "Add using statement")));
   Attrs_.insert(WarningPair(UsingRemove,
      WarningAttrs(T, D, F,
      "Remove using statement")));
   Attrs_.insert(WarningPair(ForwardAdd,
      WarningAttrs(T, C, F,
      "Add forward declaration")));
   Attrs_.insert(WarningPair(ForwardRemove,
      WarningAttrs(T, D, F,
      "Remove forward declaration")));
   Attrs_.insert(WarningPair(ArgumentUnused,
      WarningAttrs(F, X, T,
      "Unused argument")));
   Attrs_.insert(WarningPair(ClassUnused,
      WarningAttrs(F, X, T,
      "Unused class")));
   Attrs_.insert(WarningPair(DataUnused,
      WarningAttrs(F, X, T,
      "Unused data")));
   Attrs_.insert(WarningPair(EnumUnused,
      WarningAttrs(T, D, T,
      "Unused enum")));
   Attrs_.insert(WarningPair(EnumeratorUnused,
      WarningAttrs(T, D, T,
      "Unused enumerator")));
   Attrs_.insert(WarningPair(FriendUnused,
      WarningAttrs(T, D, T,
      "Unused friend declaration")));
   Attrs_.insert(WarningPair(FunctionUnused,
      WarningAttrs(F, X, T,
      "Unused function")));
   Attrs_.insert(WarningPair(TypedefUnused,
      WarningAttrs(T, D, T,
      "Unused typedef")));
   Attrs_.insert(WarningPair(ForwardUnresolved,
      WarningAttrs(T, D, F,
      "No referent for forward declaration")));
   Attrs_.insert(WarningPair(FriendUnresolved,
      WarningAttrs(T, D, F,
      "No referent for friend declaration")));
   Attrs_.insert(WarningPair(FriendAsForward,
      WarningAttrs(F, X, F,
      "Indirect reference relies on friend, not forward, declaration")));
   Attrs_.insert(WarningPair(HidesInheritedName,
      WarningAttrs(F, X, F,
      "Member hides inherited name")));
   Attrs_.insert(WarningPair(ClassCouldBeNamespace,
      WarningAttrs(F, X, F,
      "Class could be namespace")));
   Attrs_.insert(WarningPair(ClassCouldBeStruct,
      WarningAttrs(T, R, F,
      "Class could be struct")));
   Attrs_.insert(WarningPair(StructCouldBeClass,
      WarningAttrs(T, R, F,
      "Struct could be class")));
   Attrs_.insert(WarningPair(RedundantAccessControl,
      WarningAttrs(T, D, F,
      "Redundant access control")));
   Attrs_.insert(WarningPair(ItemCouldBePrivate,
      WarningAttrs(F, X, F,
      "Member could be private")));
   Attrs_.insert(WarningPair(ItemCouldBeProtected,
      WarningAttrs(F, X, F,
      "Member could be protected")));
   Attrs_.insert(WarningPair(PointerTypedef,
      WarningAttrs(F, X, F,
      "Typedef of pointer type")));
   Attrs_.insert(WarningPair(AnonymousEnum,
      WarningAttrs(F, X, F,
      "Anonymous enum")));
   Attrs_.insert(WarningPair(DataUninitialized,
      WarningAttrs(F, X, F,
      "Global data initialization not found")));
   Attrs_.insert(WarningPair(DataInitOnly,
      WarningAttrs(F, X, F,
      "Data is init-only")));
   Attrs_.insert(WarningPair(DataWriteOnly,
      WarningAttrs(F, X, F,
      "Data is write-only")));
   Attrs_.insert(WarningPair(GlobalStaticData,
      WarningAttrs(F, X, F,
      "Global static data")));
   Attrs_.insert(WarningPair(DataNotPrivate,
      WarningAttrs(F, X, F,
      "Data is not private")));
   Attrs_.insert(WarningPair(DataCannotBeConst,
      WarningAttrs(F, X, F,
      "DATA CANNOT BE CONST")));
   Attrs_.insert(WarningPair(DataCannotBeConstPtr,
      WarningAttrs(F, X, F,
      "DATA CANNOT BE CONST POINTER")));
   Attrs_.insert(WarningPair(DataCouldBeConst,
      WarningAttrs(T, I, F,
      "Data could be const")));
   Attrs_.insert(WarningPair(DataCouldBeConstPtr,
      WarningAttrs(T, I, F,
      "Data could be const pointer")));
   Attrs_.insert(WarningPair(DataNeedNotBeMutable,
      WarningAttrs(T, E, F,
      "Data need not be mutable")));
   Attrs_.insert(WarningPair(DefaultPODConstructor,
      WarningAttrs(F, X, F,
      "Default constructor invoked: POD members not initialized")));
   Attrs_.insert(WarningPair(DefaultConstructor,
      WarningAttrs(T, C, F,
      "Default constructor invoked")));
   Attrs_.insert(WarningPair(DefaultCopyConstructor,
      WarningAttrs(T, C, F,
      "Default copy constructor invoked")));
   Attrs_.insert(WarningPair(DefaultCopyOperator,
      WarningAttrs(T, C, F,
      "Default copy (assignment) operator invoked")));
   Attrs_.insert(WarningPair(PublicConstructor,
      WarningAttrs(F, X, F,
      "Base class constructor is public")));
   Attrs_.insert(WarningPair(NonExplicitConstructor,
      WarningAttrs(T, I, F,
      "Single-argument constructor is not explicit")));
   Attrs_.insert(WarningPair(MemberInitMissing,
      WarningAttrs(F, X, F,
      "Member not included in member initialization list")));
   Attrs_.insert(WarningPair(MemberInitNotSorted,
      WarningAttrs(F, X, F,
      "Member not sorted in standard order in member initialization list")));
   Attrs_.insert(WarningPair(DefaultDestructor,
      WarningAttrs(T, C, F,
      "Default destructor invoked")));
   Attrs_.insert(WarningPair(VirtualDestructor,
      WarningAttrs(F, X, F,
      "Base class virtual destructor is not public")));
   Attrs_.insert(WarningPair(NonVirtualDestructor,
      WarningAttrs(F, X, F,
      "Base class non-virtual destructor is public")));
   Attrs_.insert(WarningPair(VirtualFunctionInvoked,
      WarningAttrs(F, X, F,
      "Virtual function in own class invoked by constructor or destructor")));
   Attrs_.insert(WarningPair(RuleOf3DtorNoCopyCtor,
      WarningAttrs(F, X, F,
      "Destructor defined, but not copy constructor")));
   Attrs_.insert(WarningPair(RuleOf3DtorNoCopyOper,
      WarningAttrs(F, X, F,
      "Destructor defined, but not copy operator")));
   Attrs_.insert(WarningPair(RuleOf3CopyCtorNoOper,
      WarningAttrs(T, C, F,
      "Copy constructor defined, but not copy operator")));
   Attrs_.insert(WarningPair(RuleOf3CopyOperNoCtor,
      WarningAttrs(T, C, F,
      "Copy operator defined, but not copy constructor")));
   Attrs_.insert(WarningPair(OperatorOverloaded,
      WarningAttrs(F, X, F,
      "Overloading operator && or ||")));
   Attrs_.insert(WarningPair(FunctionNotDefined,
      WarningAttrs(T, D, F,
      "Function not implemented")));
   Attrs_.insert(WarningPair(PureVirtualNotDefined,
      WarningAttrs(F, X, F,
      "Pure virtual function not implemented")));
   Attrs_.insert(WarningPair(VirtualAndPublic,
      WarningAttrs(F, X, F,
      "Virtual function is public")));
   Attrs_.insert(WarningPair(VirtualOverloading,
      WarningAttrs(F, X, F,
      "Virtual function is overloaded")));
   Attrs_.insert(WarningPair(FunctionNotOverridden,
      WarningAttrs(F, X, F,
      "Virtual function has no overrides")));
   Attrs_.insert(WarningPair(RemoveVirtualTag,
      WarningAttrs(T, E, F,
      "Remove virtual tag: function is an override or final")));
   Attrs_.insert(WarningPair(OverrideTagMissing,
      WarningAttrs(T, I, F,
      "Function should be tagged as override")));
   Attrs_.insert(WarningPair(VoidAsArgument,
      WarningAttrs(T, E, F,
      "\"(void)\" as function argument")));
   Attrs_.insert(WarningPair(AnonymousArgument,
      WarningAttrs(F, X, F,
      "Anonymous argument")));
   Attrs_.insert(WarningPair(AdjacentArgumentTypes,
      WarningAttrs(F, X, F,
      "Adjacent arguments have the same type")));
   Attrs_.insert(WarningPair(DefinitionRenamesArgument,
      WarningAttrs(T, R, F,
      "Definition renames argument in declaration")));
   Attrs_.insert(WarningPair(OverrideRenamesArgument,
      WarningAttrs(T, R, F,
      "Override renames argument in direct base class")));
   Attrs_.insert(WarningPair(VirtualDefaultArgument,
      WarningAttrs(F, X, F,
      "Virtual function defines default argument")));
   Attrs_.insert(WarningPair(ArgumentCannotBeConst,
      WarningAttrs(F, X, F,
      "ARGUMENT CANNOT BE CONST")));
   Attrs_.insert(WarningPair(ArgumentCouldBeConstRef,
      WarningAttrs(T, I, F,
      "Object could be passed by const reference")));
   Attrs_.insert(WarningPair(ArgumentCouldBeConst,
      WarningAttrs(T, I, F,
      "Argument could be const")));
   Attrs_.insert(WarningPair(FunctionCannotBeConst,
      WarningAttrs(F, X, F,
      "FUNCTION CANNOT BE CONST")));
   Attrs_.insert(WarningPair(FunctionCouldBeConst,
      WarningAttrs(T, I, F,
      "Function could be const")));
   Attrs_.insert(WarningPair(FunctionCouldBeStatic,
      WarningAttrs(T, I, F,
      "Function could be static")));
   Attrs_.insert(WarningPair(FunctionCouldBeFree,
      WarningAttrs(F, X, F,
      "Function could be free")));
   Attrs_.insert(WarningPair(StaticFunctionViaMember,
      WarningAttrs(F, X, F,
      "Static function invoked via operator \".\" or \"->\"")));
   Attrs_.insert(WarningPair(NonBooleanConditional,
      WarningAttrs(F, X, F,
      "Non-boolean in conditional expression")));
   Attrs_.insert(WarningPair(EnumTypesDiffer,
      WarningAttrs(F, X, F,
      "Arguments to binary operator have different enum types")));
   Attrs_.insert(WarningPair(UseOfTab,
      WarningAttrs(T, R, F,
      "Tab character in source code")));
   Attrs_.insert(WarningPair(Indentation,
      WarningAttrs(T, R, F,
      "Line indentation is not a multiple of the standard value")));
   Attrs_.insert(WarningPair(TrailingSpace,
      WarningAttrs(T, E, F,
      "Line contains trailing space")));
   Attrs_.insert(WarningPair(AdjacentSpaces,
      WarningAttrs(T, E, F,
      "Line contains adjacent spaces")));
   Attrs_.insert(WarningPair(AddBlankLine,
      WarningAttrs(T, C, F,
      "Insertion of blank line recommended")));
   Attrs_.insert(WarningPair(RemoveBlankLine,
      WarningAttrs(T, D, F,
      "Deletion of blank line recommended")));
   Attrs_.insert(WarningPair(LineLength,
      WarningAttrs(F, S, F,
      "Line length exceeds the standard maximum")));
   Attrs_.insert(WarningPair(FunctionNotSorted,
      WarningAttrs(F, X, F,
      "Function not sorted in standard order")));
   Attrs_.insert(WarningPair(HeadingNotStandard,
      WarningAttrs(F, X, F,
      "File heading is not standard")));
   Attrs_.insert(WarningPair(IncludeGuardMisnamed,
      WarningAttrs(T, R, F,
      "Name of #include guard is not standard")));
   Attrs_.insert(WarningPair(DebugFtNotInvoked,
      WarningAttrs(T, C, F,
      "Function does not invoke Debug::ft")));
   Attrs_.insert(WarningPair(DebugFtNotFirst,
      WarningAttrs(F, X, F,
      "Function does not invoke Debug::ft as first statement")));
   Attrs_.insert(WarningPair(DebugFtNameMismatch,
      WarningAttrs(T, R, F,
      "Function name passed to Debug::ft is not standard")));
   Attrs_.insert(WarningPair(DebugFtNameDuplicated,
      WarningAttrs(F, X, F,
      "Function name passed to Debug::ft is used by another function")));
   Attrs_.insert(WarningPair(DisplayNotOverridden,
      WarningAttrs(F, X, F,
      "Override of Base.Display not found")));
   Attrs_.insert(WarningPair(PatchNotOverridden,
      WarningAttrs(F, X, F,
      "Override of Object.Patch not found")));
   Attrs_.insert(WarningPair(FunctionCouldBeDefaulted,
      WarningAttrs(T, R, F,
      "Function could be defaulted")));
   Attrs_.insert(WarningPair(InitCouldUseConstructor,
      WarningAttrs(T, R, F,
      "Initialization uses assignment operator")));
   Attrs_.insert(WarningPair(CouldBeNoexcept,
      WarningAttrs(T, I, F,
      "Function could be tagged noexcept")));
   Attrs_.insert(WarningPair(ShouldNotBeNoexcept,
      WarningAttrs(T, I, F,
      "Function should not be tagged noexcept")));
   Attrs_.insert(WarningPair(UseOfSlashAsterisk,
      WarningAttrs(T, R, F,
      "C-style comment")));
   Attrs_.insert(WarningPair(RemoveLineBreak,
      WarningAttrs(T, J, F,
      "Line can merge with the next line and be under the length limit")));
   Attrs_.insert(WarningPair(CopyCtorConstructsBase,
      WarningAttrs(F, X, F,
      "Copy/move constructor does not invoke base copy/move constructor")));
   Attrs_.insert(WarningPair(ValueArgumentModified,
      WarningAttrs(F, X, F,
      "Argument passed by value is modified")));
   Attrs_.insert(WarningPair(ReturnsNonConstMember,
      WarningAttrs(F, X, F,
      "Function returns non-const reference or pointer to member data")));
   Attrs_.insert(WarningPair(Warning_N,
      WarningAttrs(F, X, F,
      ERROR_STR)));
}

//------------------------------------------------------------------------------

fn_name CodeWarning_Insert = "CodeWarning.Insert";

void CodeWarning::Insert(const CodeWarning& log)
{
   Debug::ft(CodeWarning_Insert);

   if(FindWarning(log) < 0) Warnings_.push_back(log);
}

//------------------------------------------------------------------------------

bool CodeWarning::IsSortedByFile
   (const CodeWarning& log1, const CodeWarning& log2)
{
   auto result = strCompare(log1.file->Path(), log2.file->Path());
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

bool CodeWarning::IsSortedByType
   (const CodeWarning& log1, const CodeWarning& log2)
{
   if(log1.warning < log2.warning) return true;
   if(log1.warning > log2.warning) return false;
   auto result = strCompare(log1.file->Path(), log2.file->Path());
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

bool CodeWarning::IsSortedToFix
   (const CodeWarning& log1, const CodeWarning& log2)
{
   auto result = strCompare(log1.file->Path(), log2.file->Path());
   if(result == -1) return true;
   if(result == 1) return false;
   if(Attrs_.at(log1.warning).order < Attrs_.at(log2.warning).order)
      return true;
   if(Attrs_.at(log1.warning).order > Attrs_.at(log2.warning).order)
      return false;
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

fn_name CodeWarning_LogsToFix = "CodeWarning.LogsToFix";

std::vector< CodeWarning* > CodeWarning::LogsToFix(std::string& expl)
{
   Debug::ft(CodeWarning_LogsToFix);

   std::vector< CodeWarning* > logs;
   CodeWarning* log = nullptr;

   if(!Attrs_.at(warning).fixable)
   {
      expl = "Fixing this type of warning is not supported.";
      return logs;
   }

   switch(warning)
   {
   case DefaultConstructor:
   case DefaultCopyConstructor:
   case DefaultCopyOperator:
   case DefaultDestructor:
      logs.push_back(this);
      log = FindRootLog(expl);
      if(log != nullptr) logs.push_back(log);
      break;

   case ArgumentCouldBeConstRef:
   case ArgumentCouldBeConst:
   case FunctionCouldBeConst:
   case FunctionCouldBeStatic:
   case CouldBeNoexcept:
   case ShouldNotBeNoexcept:
      if(static_cast< const Function* >(item)->IsVirtual())
      {
         expl = "Changing a virtual function's signature is not supported.";
         return logs;
      }
      //  [[fallthrough]]
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
   if(this->file != that.file) return false;
   if(this->line != that.line) return false;
   if(this->pos != that.pos) return false;
   if(this->warning != that.warning) return false;
   if(this->offset != that.offset) return false;
   return (this->info == that.info);
}

//------------------------------------------------------------------------------

bool CodeWarning::operator!=(const CodeWarning& that) const
{
   return !(*this == that);
}
}
