//==============================================================================
//
//  CxxArea.h
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
#ifndef CXXAREA_H_INCLUDED
#define CXXAREA_H_INCLUDED

#include "CxxScope.h"
#include <cstddef>
#include <iosfwd>
#include <string>
#include "CodeTypes.h"
#include "Cxx.h"
#include "CxxFwd.h"
#include "CxxNamed.h"
#include "CxxScoped.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace CodeTools
{
//  A namespace or class.
//
class CxxArea : public CxxScope
{
public:
   //  Virtual to allow subclassing.
   //
   virtual ~CxxArea();

   //  Adds USE as a using statement in the area's scope.
   //
   bool AddUsing(UsingPtr& use);

   //  Adds a class to the area.
   //
   bool AddClass(ClassPtr& cls);

   //  Adds a data member to the area.
   //
   bool AddData(DataPtr& data);

   //  Adds an enumeration to the area.
   //
   bool AddEnum(EnumPtr& decl);

   //  Adds a forward declaration to the area.
   //
   bool AddForw(ForwardPtr& forw);

   //  Adds a function or operator to the area.
   //
   bool AddFunc(FunctionPtr& func);

   //  Adds a typedef to the area.
   //
   bool AddType(TypedefPtr& type);

   //  Adds inline assembly code to the area.
   //
   bool AddAsm(AsmPtr& code);

   //  Adds a static_assert to the area.
   //
   bool AddStaticAssert(StaticAssertPtr& assert);

   //  Returns the area's classes.
   //
   const ClassPtrVector* Classes() const { return &classes_; }

   //  Returns the area's data members.
   //
   const DataPtrVector* Datas() const { return &data_; }

   //  Returns the area's enumerations.
   //
   const EnumPtrVector* Enums() const { return &enums_; }

   //  Returns the area's forward declarations.
   //
   const ForwardPtrVector* Forws() const { return &forws_; }

   //  Returns the area's functions.
   //
   const FunctionPtrVector* Funcs() const { return &funcs_; }

   //  Returns the area's operators.
   //
   const FunctionPtrVector* Opers() const { return &opers_; }

   //  Returns funcs_ or opers_, based on whether NAME begins with "operator".
   //
   const FunctionPtrVector* FuncVector(const std::string& name) const;

   //  Returns the area's typedefs.
   //
   const TypedefPtrVector* Types() const { return &types_; }

   //  Returns the area's assembly code.
   //
   const AsmPtrVector* Assembly() const { return &assembly_; }

   //  Returns the area's assertions.
   //
   const StaticAssertPtrVector* Asserts() const { return &asserts_; }

   //  Returns the class identified by NAME and declared in this area.
   //
   Class* FindClass(const std::string& name) const;

   //  Returns the data member identified by NAME and declared in this area.
   //
   Data* FindData(const std::string& name) const;

   //  Returns the typedef identified by NAME and declared in this area.
   //
   Typedef* FindType(const std::string& name) const;

   //  Returns the function in this area that matches NAME.  If there is more
   //  than one function with NAME, the first one is returned unless ARGS are
   //  supplied, in which case the function must match those arguments.  If
   //  BASE is set, the search continues up the namespace or class hierarchy
   //  if no matching function is found in this area.  If SCOPE is provided,
   //  VIEW is updated with the function's accessibility to SCOPE.
   //
   virtual Function* FindFunc(const std::string& name, StackArgVector* args,
      bool base, const CxxScope* scope, SymbolView* view) const;

   //  Returns the function or operator that matches CURR's name and signature.
   //  If it is not found, the search continues up the class hierarchy if BASE
   //  is set and the area is a class.
   //
   virtual Function* MatchFunc(const Function* curr, bool base) const;

   //  Overridden to add the area's components to cross-references.
   //
   void AddToXref() const override;

   //  Overridden to log warnings associated with the area's declarations.
   //
   void Check() const override;

   //  Overridden to return the area itself.
   //
   CxxArea* GetArea() const
      override { return const_cast< CxxArea* >(this); }

   //  Overridden to shrink containers.
   //
   void Shrink() override;
protected:
   //  Protected because this class is virtual.
   //
   CxxArea();

   //  Returns the area's using statements.
   //
   const UsingPtrVector* Usings() const { return &usings_; }

   //  The same as Datas(), but provides non-const access.
   //
   DataPtrVector* Datas() { return &data_; }

   //  Returns the enum identified by NAME and declared in this area.
   //
   Enum* FindEnum(const std::string& name) const;

   //  Returns the enumerator identified by NAME and declared in this area.
   //
   Enumerator* FindEnumerator(const std::string& name) const;

   //  Returns the first item in this area that matches NAME.
   //
   virtual CxxScoped* FindItem(const std::string& name) const;
private:
   //  Defined so that a class can register ITEM in the order in which it was
   //  declared.
   //
   virtual void AddItem(const CxxNamed* item) { }

   //  Invoked to add CLS to the area if CLS is an anonymous union.  Returns
   //  true if the union's members were promoted to the union's outer scope.
   //
   virtual bool AddAnonymousUnion(const ClassPtr& cls) { return false; }

   //  Invoked by FindFunc.  Updates VIEW with MATCH and returns FUNC.
   //
   static Function* FoundFunc
      (Function* func, SymbolView* view, TypeMatch match);

   //  The area's using statements.
   //
   UsingPtrVector usings_;

   //  The area's classes.
   //
   ClassPtrVector classes_;

   //  The area's data members.
   //
   DataPtrVector data_;

   //  The area's enumerations.
   //
   EnumPtrVector enums_;

   //  The area's forward declarations.
   //
   ForwardPtrVector forws_;

   //  The area's functions.
   //
   FunctionPtrVector funcs_;

   //  The area's operators.
   //
   FunctionPtrVector opers_;

   //  The area's typedefs.
   //
   TypedefPtrVector types_;

   //  The area's assembly code.
   //
   AsmPtrVector assembly_;

   //  The area's static assertions.
   //
   StaticAssertPtrVector asserts_;

   //  The area's definitions (of previously declared data or functions).
   //
   ScopePtrVector defns_;
};

//------------------------------------------------------------------------------
//
//  Types used when checking that a constructor initializes all necessary data.
//
struct DataInitAttrs
{
   const Data* const member;  // a data member
   const bool initNeeded;     // set if it needs to be explicitly initialized
   size_t initOrder;          // the order in which it was initialized

   DataInitAttrs(const Data* m, bool n, size_t o) :
      member(m), initNeeded(n), initOrder(o) { }
};

typedef std::vector< DataInitAttrs > DataInitVector;

//------------------------------------------------------------------------------
//
//  A class, struct, or union.
//
class Class : public CxxArea
{
public:
   //  Creates a class with NAME and TAG.
   //
   Class(QualNamePtr& name, Cxx::ClassTag tag);

   //  Virtual to allow subclassing.
   //
   virtual ~Class();

   //  Sets the class's alignment.
   //
   void SetAlignment(AlignAsPtr& align);

   //  Adds BASE as the class's base class.
   //
   void AddBase(BaseDeclPtr& base);

   //  Adds DIR as a preprocessor directive within the class.
   //
   bool AddDirective(DirectivePtr& dir);

   //  Adds DECL as a friend of the class.
   //
   bool AddFriend(FriendPtr& decl);

   //  Adds CLS as a direct subclass of the class.
   //
   bool AddSubclass(Class* cls);

   //  Finds a class template instance based on TYPE.  If it does not exist,
   //  it is created.  This class is the class template for the instance.
   //
   virtual ClassInst* EnsureInstance(const TypeName* type);

   //  Sets the current level of access control when parsing the class.
   //
   bool SetCurrAccess(Cxx::Access access);

   //  Returns the class's direct base class.
   //
   virtual Class* BaseClass()
      const { return (base_ == nullptr ? nullptr : base_->GetClass()); }

   //  Returns the base class declaration.
   //
   virtual BaseDecl* GetBaseDecl() const { return base_.get(); }

   //  Returns the class's outer class.  Returns nullptr if the class is
   //  not an inner class.
   //
   Class* OuterClass() const;

   //  Returns the class's direct subclasses.
   //
   const ClassVector* Subclasses() const { return &subs_; }

   //  Returns the class's friends.
   //
   const FriendPtrVector* Friends() const { return &friends_; }

   //  Returns the class template, if any, associated with the class.
   //
   virtual Class* GetClassTemplate() const;

   //  Returns a class template's instantiations.
   //
   const ClassInstPtrVector* Instances() const { return &tmplts_; }

   //  Returns the distance, up the class hierarchy, to CLS.  Returns 0
   //  if this class *is* CLS.  Returns NOT_A_SUBCLASS if this class is
   //  not derived from CLS.
   //
   Distance ClassDistance(const Class* cls) const;

   //  Returns true if this class subclasses from CLS.  Returns false if
   //  this class is not derived from CLS or actually *is* CLS.
   //
   virtual bool DerivesFrom(const Class* cls) const;

   //  Returns true if this class subclasses from NAME.  Returns false if
   //  this class is not derived from NAME or actually *is* NAME.  This is
   //  a simple function not intended for general use, given that names are
   //  contextual.  For example, it was initially used when parsing a base
   //  class constructor call, to see if what followed the ':' was the base
   //  class name.  However, NameRefersToItem was eventually used instead,
   //  given that it can handle different names that refer to the same item.
   //
   bool DerivesFrom(const std::string& name) const;

   //  Returns the item in this class that matches NAME.  If BASE is set, the
   //  search continues up the class hierarchy when NAME is not defined in this
   //  class.  If SCOPE is provided, updates VIEW with the item's accessibility
   //  to SCOPE.
   //
   CxxScoped* FindMember(const std::string& name, bool base,
      const CxxScope* scope = nullptr, SymbolView* view = nullptr) const;

   //  Returns true if the class has a POD member.
   //
   bool HasPODMember() const;

   //  Returns the constructor that accepts ARGS.  If ARGS is nullptr, returns
   //  the default constructor if it exists, else the first constructor found.
   //  Otherwise, looks for a constructor that matches ARGS.  Returns nullptr
   //  if a constructor is not found, in which case the class has an implicit,
   //  default constructor if IsDefaultConstructible returns true.  Updates
   //  VIEW with the constructor's accessibility to SCOPE if SCOPE is provided.
   //
   Function* FindCtor(StackArgVector* args,
      const CxxScope* scope = nullptr, SymbolView* view = nullptr);

   //  Returns all constructors.  Returns a nullptr entry if none are found.
   //
   std::vector< Function* > FindCtors() const;

   //  Returns the destructor.  Returns nullptr if the class doesn't define one,
   //  in which case it has a default, public destructor.  If SCOPE is provided,
   //  VIEW is updated with the destructor's accessibility to SCOPE.
   //
   Function* FindDtor
      (const CxxScope* scope = nullptr, SymbolView* view = nullptr) const;

   //  Returns the function that provides ROLE.  If not found, the search
   //  continues up the class hierarchy if BASE is set.  Not supported for
   //  FuncOther and PureCtor, since there can be multiple such functions.
   //
   Function* FindFuncByRole(FunctionRole role, bool base) const;

   //  Determines where and how a function is implemented.
   //
   FunctionDefinition GetFuncDefinition(const Function* func) const;

   //  Returns the friend declaration for SCOPE if one exists.
   //
   Friend* FindFriend(const CxxScope* scope) const;

   //  Returns true if this class has a non-explicit constructor that can
   //  be invoked with THAT, whose type is thatType.
   //
   bool CanConstructFrom
      (const StackArg& that, const std::string& thatType) const;

   //  Returns the item known by NAME within this class, even if inherited.
   //  If BASE is not nullptr, the search stops once BASE is reached, and
   //  BASE is not included in the search.
   //
   CxxScoped* FindName(const std::string& name, const Class* base) const;

   //  Returns the type of class.
   //
   Cxx::ClassTag GetClassTag() const { return tag_; }

   //  Updates CODE with the code for the template instance INST, returning the
   //  location where parsing should begin.  Returns string::npos on an error.
   //
   size_t CreateCode(const ClassInst* inst, NodeBase::stringPtr& code) const;

   //  Updates IDX to FUNC's index within its vector and return true.  Returns
   //  FALSE if FUNC was not found.
   //
   bool GetFuncIndex(const Function* func, size_t& idx) const;

   //  Returns true if the class has a default (zero-argument) constructor.
   //
   bool IsDefaultConstructible();

   //  Returns data members and their initialization requirements in MEMBERS.
   //
   void GetMemberInitAttrs(DataInitVector& members) const;

   //  Invokes EnterBlock on any template parameters.
   //
   void EnterParms() const;

   //  Invokes ExitBlock on any template parameters.
   //
   void ExitParms() const;

   //  Invoked when an instance of the class is block-copied by assignment
   //  to ARG.  This means that none of its data members can be const.
   //
   void BlockCopied(const StackArg* arg);

   //  Displays the class's subclasses, recursively.
   //
   void DisplayHierarchy(std::ostream& stream, const std::string& prefix) const;

   //  Overridden to invoke AccessibilityOf on the class rather than on its
   //  scope (usually a namespace), which is what would occur in the default
   //  version.  This is done to see if SCOPE is a subclass of the class.
   //
   void AccessibilityTo(const CxxScope* scope, SymbolView* view) const override;

   //  Overridden to determine how ITEM, which is declared in this class, is
   //  visible to SCOPE.
   //
   void AccessibilityOf(const CxxScope* scope,
      const CxxScoped* item, SymbolView* view) const override;

   //  Overridden to promote CLS's members to their outer scope if CLS is
   //  an anonymous union.
   //
   bool AddAnonymousUnion(const ClassPtr& cls) override;

   //  Overridden to update imSet with files that declare or define any of
   //  the class's members.
   //
   void AddFiles(SetOfIds& imSet) const override;

   //  Overridden to add the class's components to cross-references.
   //
   void AddToXref() const override;

   //  Overridden to set the type for an "auto" variable.
   //
   CxxToken* AutoType() const override { return (CxxToken*) this; }

   //  Overridden to log warnings associated with the class's declarations.
   //
   void Check() const override;

   //  Overridden generate a log if the class is unused.  Also generates
   //  other types of logs related to a class.
   //
   bool CheckIfUnused(Warning warning) const override;

   //  Overridden to return the outer class.
   //
   Class* Declarer() const override { return OuterClass(); }

   //  Overridden to return the class itself.
   //
   Class* DirectClass() const override { return GetClass(); }

   //  Overridden to display the class and its members.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;

   //  Overridden to add the class to the current scope.
   //
   bool EnterScope() override;

   //  Overridden to support searching up the class and namespace hierarchy
   //  if a function matching the criteria is not found in this class.
   //
   Function* FindFunc(const std::string& name, StackArgVector* args,
      bool base, const CxxScope* scope, SymbolView* view) const override;

   //  Overridden to return the class.
   //
   Class* GetClass() const
      override { return const_cast< Class* >(this); }

   //  Overridden to return the types for which the class has conversion
   //  operators.
   //
   void GetConvertibleTypes(StackArgVector& types) override;

   //  Overridden to add the class to SYMBOLS.  The purpose of this function is
   //  to find a class that was resolved by a forward or friend declaration but
   //  whose definition should be #included.  If the class is already #included,
   //  it must add itself as a direct usage to ensure that >trim won't suggest
   //  removing the #include for its definition when it isn't used directly in
   //  any other situation.
   //
   void GetDirectClasses(CxxUsageSets& symbols) const override;

   //  Returns the current access control level when parsing the class.
   //
   Cxx::Access GetCurrAccess() const override;

   //  Overridden to return the class's qualified name.
   //
   QualName* GetQualName() const override { return name_.get(); }

   //  Overridden to return the offset of the left brace.
   //
   size_t GetRange(size_t& begin, size_t& end) const override;

   //  Overridden to return the class if it is a class template.
   //
   CxxScope* GetTemplate() const override;

   //  Overridden to support class templates.
   //
   const TemplateParms* GetTemplateParms() const
      override { return parms_.get(); }

   //  Overridden to update SYMBOLS with the type usage of each of the
   //  class's components.
   //
   void GetUsages(const CodeFile& file, CxxUsageSets& symbols) const override;

   //  Overridden to look at using statements that are local to the class.
   //
   Using* GetUsingFor(const std::string& fqName, size_t prefix,
      const CxxNamed* item, const CxxScope* scope) const override;

   //  Overridden to look for an implemented function.
   //
   bool IsImplemented() const override;

   //  Overridden to support BASE.
   //
   Function* MatchFunc(const Function* curr, bool base) const override;

   //  Overridden to return the class's name.
   //
   const std::string* Name() const override { return name_->Name(); }

   //  Overridden to create an argument when the class is used to access a
   //  constructor.
   //
   StackArg NameToArg(Cxx::Operator op, TypeName* name) override;

   //  Overridden to record usage of the class.
   //
   void RecordUsage() const override { AddUsage(); }

   //  Overridden to support class templates.
   //
   void SetTemplateParms(TemplateParmsPtr& parms) override;

   //  Overridden to shrink containers.
   //
   void Shrink() override;

   //  Overridden to reveal that this is a class.
   //
   Cxx::ItemType Type() const override { return Cxx::Class; }

   //  Overridden to return the class's fully qualified name plus any template
   //  specification.
   //
   std::string TypeString(bool arg) const override;

   //  Overridden to support, for example, passing a "this" argument or writing
   //  to a class object in an array.
   //
   bool WasWritten(const StackArg* arg, bool passed)
      override { return false; }

   //  Overridden to append template arguments to a template specialization.
   //
   std::string XrefName(bool templates) const override;
protected:
   //  Displays the first line of the declaration (the name and base class).
   //
   void DisplayBase(std::ostream& stream, const NodeBase::Flags& options) const;
private:
   //  Overridden to register ITEM in the order in which it was declared.
   //
   void AddItem(const CxxNamed* item) override;

   //  Determines if MEMBER is accessible to SCOPE, updating VIEW with details
   //  on its visibility.
   //
   static bool MemberIsAccessibleTo
      (const CxxScoped* member, const CxxScope* scope, SymbolView* view);

   //  Determines how well the class (which is a class template or a
   //  specialization of one) matches TYPE.
   //
   TypeMatch MatchTemplate(const TypeName& type) const;

   //  Creates a class template instance based on NAME and TYPE.
   //
   ClassInst* CreateInstance(const std::string& name, const TypeName* type);

   //  Invoked when an error occurs in CreateCode.  NAME is the template
   //  whose code could not be found.
   //
   static size_t CreateCodeError
      (const std::string& name, NodeBase::debug64_t offset);

   //  Class attributes and the types of items that it defines.
   //
   enum Attributes
   {
      IsBase,
      IsConstructed,
      HasInstantiations,
      HasPublicInnerClass,
      HasNonPublicInnerClass,
      HasPublicMemberFunction,
      HasNonPublicMemberFunction,
      HasPublicStaticFunction,
      HasNonPublicStaticFunction,
      HasPublicMemberData,
      HasNonPublicMemberData,
      HasPublicStaticData,
      HasNonPublicStaticData,
      HasEnum,
      HasTypedef,
      Attribute_N
   };

   //  The attributes of a class, used to analyze if it is unused, or should
   //  be a namespace or struct, or a struct should be a class.
   //
   typedef std::bitset< Attribute_N > UsageAttributes;

   //  Returns a class's attributes.  Only sets flags in UsageAttributes for
   //  items that are actually used.
   //
   UsageAttributes GetUsageAttrs() const;

   //  Checks that the class follows the Rule of Three and its variants.
   //
   void CheckRuleOfThree() const;

   //  Checks that the class's functions use the "override" tag when applicable.
   //
   void CheckOverrides() const;

   //  The class's name.
   //
   const QualNamePtr name_;

   //  The class's alignment.
   //
   AlignAsPtr alignas_;

   //  The template parameters for a class template.
   //
   TemplateParmsPtr parms_;

   //  The type of class.
   //
   const Cxx::ClassTag tag_ : 8;

   //  The current access control level when parsing the class.
   //
   Cxx::Access currAccess_ : 8;

   //  Set if the class was block-copied.
   //
   bool copied_ : 8;

   //  The class's base class declaration.
   //
   //  NOTE: To access the base class, use BaseClass(), even within this class,
   //  ====  because the field is nullptr for a template instance, even if its
   //        class template has a base class.   Similarly, use GetBaseDecl() to
   //        access the base class declaration.
   //
   BaseDeclPtr base_;

   //  The class's friends.
   //
   FriendPtrVector friends_;

   //  The class's preprocessor directives.
   //
   DirectivePtrVector dirs_;

   //  The class's items in the order in which they appeared.
   //
   CxxNamedVector items_;

   //  The class's template instantiations.
   //
   ClassInstPtrVector tmplts_;

   //  The class's direct subclasses.
   //
   ClassVector subs_;

   //  The source code if this is a class template.
   //
   mutable NodeBase::stringPtr code_;
};

//------------------------------------------------------------------------------
//
//  An instance of a class template.
//
class ClassInst : public Class
{
public:
   //  Creates a class template instance from class TMPLT, whose template
   //  parameters were specified within SPEC.  NAME is its unqualified name.
   //
   ClassInst(QualNamePtr& name, Class* tmplt, const TypeName* spec);

   //  Not subclassed.
   //
   ~ClassInst();

   //  Returns the instance's source code.
   //
   const std::string& GetCode() const { return *code_; }

   //  Returns the instance item that corresponds to ITEM in the class template.
   //
   CxxScoped* FindInstanceAnalog(const CxxNamed* item) const;

   //  Overridden to not add any symbols to the cross-reference.  The class
   //  template adds them itself because each template instance is the same
   //  (apart from its template arguments, which we would want to exclude).
   //
   void AddToXref() const override { }

   //  Overridden to return the class template's base class.
   //
   Class* BaseClass() const override { return tmplt_->BaseClass(); }

   //  Overridden to check only the first class template instance.
   //
   void Check() const override;

   //  Overridden to check if CLS if of the form T<args2>, where this class
   //  is of the form T<args1>.  If so, return true if args2 are compatible
   //  with args1.  If CLS is not a class template instance of T, returns the
   //  result of the superclass version.
   //
   bool DerivesFrom(const Class* cls) const override;

   //  Overridden to display the name, the number of references to it, and
   //  its interface.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;

   //  Overridden to return the class template instance when it already exists
   //  and is found as a referent by another user.
   //
   ClassInst* EnsureInstance(const TypeName* type)
      override { return this; }

   //  Overridden to return the template item that corresponds to ITEM.
   //
   CxxScoped* FindTemplateAnalog(const CxxNamed* item) const override;

   //  Overridden to return the template's base class declaration.
   //
   BaseDecl* GetBaseDecl() const
      override { return tmplt_->GetBaseDecl(); }

   //  Overridden to return the instance's class template.
   //
   Class* GetClassTemplate() const override { return tmplt_; }

   //  Overridden to not add the class template instance to SYMBOLS, given
   //  that each of its components (class template and template arguments)
   //  is added individually.
   //
   void GetDirectClasses(CxxUsageSets& symbols) const override { }

   //  Overridden to return the instance's class template.
   //
   CxxScope* GetTemplate() const override { return tmplt_; }

   //  Overridden to return the instance's template arguments.
   //
   TypeName* GetTemplateArgs() const override { return tspec_.get(); }

   //  Overridden to obtain usages for the class template.
   //
   void GetUsages(const CodeFile& file, CxxUsageSets& symbols) const override;

   //  Overridden to instantiate the class template instance.
   //
   void Instantiate() override;

   //  Overridden to return this class template instance.
   //
   CxxScope* GetTemplateInstance() const override { return (CxxScope*) this; }

   //  Overridden for when NAME refers to a class template instance.
   //
   bool NameRefersToItem(const std::string& name, const CxxScope* scope,
      const CodeFile* file, SymbolView* view) const override;

   //  Overridden to record usage of the instance's template.
   //
   void RecordUsage() const override { tmplt_->RecordUsage(); }

   //  Overridden to count references.
   //
   void SetAsReferent(const CxxNamed* user) override { ++refs_; }

   //  Overridden to shrink containers.
   //
   void Shrink() override;

   //  Overridden to return a string containing a fully qualified class name
   //  and fully qualified template arguments.
   //
   std::string TypeString(bool arg) const override;
private:
   //  The class template of which this is an instance.
   //
   Class* const tmplt_;

   //  The instance's name (with template arguments).  This is saved because
   //  its actual name is a single string containing the arguments.  The code
   //  supports such names rather than generating an artificial but legal C++
   //  identifier.
   //
   TypeNamePtr tspec_;

   //  The instance's source code.
   //
   NodeBase::stringPtr code_;

   //  The number of references to the instance.
   //
   size_t refs_ : 16;

   //  Set when Instantiate is invoked.
   //
   bool created_;

   //  Set if the instance was successfully compiled.
   //
   bool compiled_;
};

//------------------------------------------------------------------------------
//
//  A namespace.
//
class Namespace : public CxxArea
{
public:
   //  Creates a namespace with NAME, which is nested inside SPACE.
   //
   Namespace(const std::string& name, Namespace* space);

   //  Not subclassed.
   //
   ~Namespace();

   //  Finds the namespace known by NAME.  If it doesn't exist, it is created
   //  as a nested namespace within this one.
   //
   Namespace* EnsureNamespace(const std::string& name);

   //  Returns the namespace's outer namespace.
   //
   Namespace* OuterSpace()
      const { return static_cast< Namespace* >(GetScope()); }

   //  Returns the namespace identified by NAME that is a direct subscope of
   //  this namespace.
   //
   Namespace* FindNamespace(const std::string& name) const;

   //  Overridden to determine how ITEM, which is declared in this namespace,
   //  is accessible to SCOPE.
   //
   void AccessibilityOf(const CxxScope* scope,
      const CxxScoped* item, SymbolView* view) const override;

   //  Overridden to log warnings associated with the namespace's declarations.
   //
   void Check() const override;

   //  Overridden to display the namespace and its declarations.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;

   //  Overridden to support searching up the namespace hierarchy if a function
   //  matching the criteria is not found in this namespace.
   //
   Function* FindFunc(const std::string& name, StackArgVector* args,
      bool base, const CxxScope* scope, SymbolView* view) const override;

   //  Overridden to also look for an inner namespace.
   //
   CxxScoped* FindItem(const std::string& name) const override;

   //  Overridden to indicate that an enclosing scope cannot be a class.
   //
   Class* GetClass() const override { return nullptr; }

   //  All items at file scope are public.
   //
   Cxx::Access GetCurrAccess() const override { return Cxx::Public; }

   //  Overridden to return the namespace.
   //
   Namespace* GetSpace() const
      override { return const_cast< Namespace* >(this); }

   //  Overridden to indicate that we are not in a template instance.
   //
   CxxScope* GetTemplateInstance() const override { return nullptr; }

   //  Overridden to return the namespace's name.
   //
   const std::string* Name() const override { return &name_; }

   //  Overridden to preserve the location where the namespace first occurred.
   //
   void SetLoc(CodeFile* file, size_t pos) override;

   //  Overridden to handle the global namespace.
   //
   std::string ScopedName(bool templates) const override;

   //  Overridden to shrink containers.
   //
   void Shrink() override;

   //  Overridden to reveal that this is a namespace.
   //
   Cxx::ItemType Type() const override { return Cxx::Namespace; }

   //  Overridden to return the namespace's fully qualified name.
   //
   std::string TypeString(bool arg) const override;
private:
   //  The namespace's name.
   //
   std::string name_;

   //  Set if >check was run on the namespace.
   //
   mutable bool checked_;

   //  The namespaces nested inside this one.
   //
   NamespacePtrVector spaces_;
};
}
#endif
