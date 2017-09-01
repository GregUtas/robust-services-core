//==============================================================================
//
//  CxxNamed.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef CXXNAMED_H_INCLUDED
#define CXXNAMED_H_INCLUDED

#include "CxxToken.h"
#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <string>
#include "CodeTypes.h"
#include "Cxx.h"
#include "CxxFwd.h"
#include "CxxString.h"
#include "SysTypes.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace CodeTools
{
//  Where a C++ item was declared or defined.
//
struct CxxLocation
{
   friend class CxxNamed;
public:
   CxxLocation() : file(nullptr), pos(std::string::npos), internal(false) { }

   void SetLoc(CodeFile* f, size_t p) { file = f; pos = p; }

   size_t GetPos() const
   {
      if(pos == 0x7fffffff)
         return std::string::npos;
      else
         return pos;
   }

   CodeFile* file;     // the file where the item appeared
private:
   size_t pos : 31;    // the item's location (character offset) in that file
   bool internal : 1;  // set if the item appeared in internally generated code
                       // (e.g. in a template instance)
};

//------------------------------------------------------------------------------
//
//  Tags that can be applied to a type.
//
struct TypeTags
{
   bool const_ : 1;       // type is const
   bool constptr_ : 1;    // pointer is const
   TagCount arrays_ : 8;  // number of arrays
   TagCount ptrs_ : 8;    // number of pointers
   TagCount refs_ : 8;    // number of references

   //  Creates a nil instance.
   //
   TypeTags();

   //  Creates an instance using SPEC's attributes.
   //
   explicit TypeTags(const TypeSpec& spec);
};

//------------------------------------------------------------------------------
//
//  The base class for C++ entities that define a name.  Each of these knows
//  the file in which it was found.
//
class CxxNamed : public CxxToken
{
public:
   //  Virtual to allow subclassing.
   //
   virtual ~CxxNamed();

   //  Returns the file in which this item was found.
   //
   CodeFile* GetDeclFile() const { return decl_.file; }

   //  Returns the offset at which the item was found.  The item's file has a
   //  string member which contains the code, and the offset is an index into
   //  that string.
   //
   size_t GetDeclPos() const { return decl_.GetPos(); }

   //  Sets the file and offset at which this item was found.
   //
   virtual void SetDecl(CodeFile* file, size_t pos) { decl_.SetLoc(file, pos); }

   //  Indicates that the item appeared in internally generated code
   //  (which currently means within a template instance).
   //
   void SetInternal() { decl_.internal = true; }

   //  Returns true if the item appeared in internally generated code.
   //
   bool IsInternal() const { return decl_.internal; }

   //  Returns true if the item was declared in a function's code block
   //  or argument list.
   //
   virtual bool IsDeclaredInFunction() const { return false; }

   //  Returns true if the item is static.  Note that, for the purposes
   //  of this function:
   //  o only data and functions can be classified as non-static;
   //  o class membership for non-static data and functions must be
   //    checked separately, using if(item->GetClass() != nullptr).
   //
   virtual bool IsStatic() const { return true; }

   //  Returns the scope (namespace, class, or block) where the item
   //  was found.
   //
   virtual CxxScope* GetScope() const { return nullptr; }

   //  Sets the scope where the item was found.  The scope is mutable to
   //  support friend declarations and definitions, which sometimes need
   //  to correct their scope within functions that are logically const.
   //
   virtual void SetScope(CxxScope* scope) const { }

   //  Sets the access control that applies to the item.
   //
   virtual void SetAccess(Cxx::Access access) { }

   //  Sets the template parameters when the item declares a template.
   //  The default version invokes SetTemplateParms on GetQualName.
   //
   virtual void SetTemplateParms(TemplateParmsPtr& parms);

   //  Returns the template parameters associated with the item, if any.
   //  The default implementation invokes GetQualName and, if the result
   //  is not nullptr, asks it for its template parameters.
   //
   virtual const TemplateParms* GetTemplateParms() const;

   //  Returns true if the item is a class template or function template.
   //
   bool IsTemplate() const { return GetTemplateParms() != nullptr; }

   //  Returns the class template, if any, associated with a class.
   //
   virtual Class* GetTemplate() const { return nullptr; }

   //  Returns the qualified name as it appeared in the source code.  Includes
   //  prefixed scopes if SCOPES is set and template arguments if TEMPLATES is
   //  is set.  Consequently, QualifiedName(false, false) == *Name().
   //
   virtual std::string QualifiedName(bool scopes, bool templates)
      const { return *Name(); }

   //  Returns the item's fully qualified name.  Template arguments are omitted
   //  unless TEMPLATES is set.
   //
   virtual std::string ScopedName(bool templates) const;

   //  Returns the area (namespace or class) in which the item was declared.
   //  Returns (because of an override) the item itself if it is an area.
   //
   virtual CxxArea* GetArea() const;

   //  Returns the function associated with the item.  The interpretation of
   //  this varies by item type.
   //
   virtual Function* GetFunction() const { return nullptr; }

   //  Returns the access control that applies to the item.
   //
   virtual Cxx::Access GetAccess() const { return Cxx::Public; }

   //  Returns the identifier of the file in which the item was declared.
   //
   virtual id_t GetDeclFid() const;

   //  Returns true if the item was declared at file scope.
   //
   bool AtFileScope() const;

   //  Invoked before adding the item to the current scope (Context::Scope()).
   //  Returning false indicates that
   //  o for a C++ item, that it should be deleted, not added to the scope;
   //  o for a preprocessor directive, that the code that follows it should
   //    not be compiled.
   //
   //  NOTE: There is currently no provision for removing an item from a scope
   //  ====  after it has been added.  That is, there are no "undo" versions of
   //        functions such as CodeFile.InsertX and Class.AddX.  If the item is
   //        deleted later, its scope will be left with an invalid pointer.
   //
   virtual bool EnterScope() { return true; }

   //  Returns true if this item was a previous declaration of ITEM.  This is
   //  used after parsing data initializations and function definitions.
   //
   bool IsPreviousDeclOf(const CxxNamed* item) const;

   //  Returns true if the item is implemented.  Returns false for a function
   //  without an implementation or a class without an implemented function.
   //
   virtual bool IsImplemented() const { return true; }

   //  Finds what the item refers to.  The default version generates a log and
   //  returns false.
   //
   virtual bool FindReferent();

   //  Invoked when the item is found to be the referent of USER.
   //
   virtual void SetAsReferent(const CxxNamed* user) { }

   //  Invoked to instantiate a class template instance when it is declared as
   //  a member or named in executable code.  Returns false if an error occurs.
   //
   virtual bool Instantiate() { return false; }

   //  Returns true if the item is, or belongs to, a template instance.
   //
   virtual bool IsInTemplateInstance() const;

   //  Constructs an argument for the item when it is named directly, perhaps
   //  through an implicit "this".  OP is the operator that is on top of the
   //  operator stack (Cxx::NIL_OPERATOR if the stack is empty).  The result
   //  is pushed onto the argument stack.
   //
   virtual StackArg NameToArg(Cxx::Operator op);

   //  Constructs an argument for the item when it was accessed through VIA
   //  and OP (either "." or "->").  The result is pushed onto the argument
   //  stack.
   //
   virtual StackArg MemberToArg(StackArg& via, Cxx::Operator op);

   //  Invoked when the item is accessed.  Invokes ItemAccessed on the context
   //  function.
   //
   void Accessed() const;

   //  Logs WARNING at the position where this item is located.  OFFSET
   //  is specific to WARNING.
   //
   void Log(Warning warning, size_t offset = 0) const;

   //  Displays the item's referent in STREAM.  If FQ is set, the item's
   //  fully qualified name is displayed.
   //
   void DisplayReferent(std::ostream& stream, bool fq) const;

   //  Returns a string that identifies the item's source code location.
   //
   std::string strLocation() const;

   //  Displays NAME in STREAM.  If FQ is set, the fully qualified name is
   //  shown, else the qualified name (including any templates) is shown.
   //
   void strName(std::ostream& stream, bool fq, const QualName* name) const;

   //  Overridden to invoke GetClass on the item's scope (GetScope).
   //
   virtual Class* GetClass() const override;

   //  Overridden to use GetScope to find an item's namespace.
   //
   virtual Namespace* GetSpace() const override;

   //  Overridden to return the item's scoped name.
   //
   virtual std::string Trace() const override { return ScopedName(true); }
protected:
   //  Protected because this class is virtual.
   //
   CxxNamed();

   //  Copy constructor.
   //
   CxxNamed(const CxxNamed& that);

   //  Resolves the item's qualified name.  FILE, SCOPE, MASK, and VIEW are
   //  the same as the arguments for CxxSymbols::FindSymbol.
   //
   CxxNamed* ResolveName(const CodeFile* file, const CxxScope* scope,
      const Flags& mask, SymbolView* view) const;

   //  Resolves the item's qualified name within a function.
   //
   CxxNamed* ResolveLocal(SymbolView* view) const;

   //  Returns the item's direct type, which could be a typedef or forward.
   //  declaration.  To follow either of these to the final underlying type,
   //  use CxxToken.Root.
   //
   virtual CxxNamed* DirectType() const { return Referent(); }

   //  Invoked when ResolveName finds TYPE, a typedef.  If it returns false,
   //  ResolveName returns TYPE.  Otherwise, name resolution continues with
   //  TYPE's referent (its underlying type).
   //
   virtual bool ResolveTypedef(Typedef* type) const { return true; }

   //  Invoked when ResolveName finds ARGS, template arguments for CLS.  END
   //  is set if the arguments are attached to the last name in the qualified
   //  name.  To invoke EnsureInstance with the arguments, return true.  To
   //  skip the arguments, return false.
   //
   virtual bool ResolveTemplate
      (Class* cls, const TypeName* args, bool end) const { return true; }

   //  Invoked by overrides of RecordUsage.
   //
   void AddUsage() const;
private:
   //  Invoked when ResolveName finds DECL, a forward or friend declaration.
   //  If it returns false, ResolveName returns DECL.  Otherwise, name
   //  resolution continues with DECL's referent (the class or function to
   //  which it refers).  If this is still unknown, DECL is returned.
   //
   virtual bool ResolveForward(CxxScoped* decl) const { return false; }

   //  Invoked when ResolveName finds an item through a qualified name that
   //  includes a subclass name (derived::member instead of base::member).
   //
   virtual void SubclassAccess(Class* cls) const { }

   //  The location where the item was declared.
   //
   CxxLocation decl_;
};

//------------------------------------------------------------------------------
//
//  A member initialization.  This is one of the elements in a constructor's
//  initialization list.
//
class MemberInit : public CxxNamed
{
public:
   //  INIT is the expression that initializes NAME.  It is parsed as
   //  arguments for a function call in case it invokes a constructor.
   //
   MemberInit(std::string& name, TokenPtr& init);

   //  Not subclassed.
   //
   ~MemberInit() { CxxStats::Decr(CxxStats::MEMBER_INIT); }

   //  Returns the expression that initializes the member.
   //
   CxxToken* GetInit() const { return init_.get(); }

   //  Overridden to update SYMBOLS with the statement's type usage.
   //
   virtual void GetUsages
      (const CodeFile& file, CxxUsageSets& symbols) const override;

   //  Overridden to return the member's name.
   //
   virtual const std::string* Name() const override { return &name_; }

   //  Overridden to display the initialization statement.
   //
   virtual void Print(std::ostream& stream) const override;

   //  Overridden to shrink containers.
   //
   virtual void Shrink() override;

   //  Overridden to return the member's name.
   //
   virtual std::string Trace() const override { return name_; }
private:
   //  The name of the member being initialized.
   //
   std::string name_;

   //  The expression that initializes the member.
   //
   const TokenPtr init_;
};

//------------------------------------------------------------------------------
//
//  Used when an item can have a qualified name (that is, when its name
//  in the source code can contain a scope resolution operator).
//
class QualName : public CxxNamed
{
public:
   //  Creates a name that begins with TYPE.
   //
   explicit QualName(TypeNamePtr& type);

   //  Creates a name that begins with NAME.
   //
   explicit QualName(const std::string& name);

   //  Not subclassed.
   //
   ~QualName();

   //  Copy constructor.
   //
   QualName(const QualName& that);

   //  Specifies whether a scope resolution operator preceded the first name.
   //
   void SetGlobal(bool global) { global_ = global; }

   //  Returns true if a scope resolution operator preceded the first name.
   //
   bool IsGlobal() const { return global_; }

   //  Adds TYPE to the name.  In a qualified name, TYPE is preceded by a
   //  scope resolution operator.
   //
   void AddTypeName(TypeNamePtr& type);

   //  Adds NAME to the last name.  Inserts a space if SPACE is set.
   //
   void Append(const std::string& name, bool space = false);

   //  Invokes SetLocale on each name.
   //
   void SetLocale(Cxx::ItemType locale) const;

   //  Sets oper_ when an operator follows the final name.
   //
   void SetOperator(Cxx::Operator oper);

   //  Transfers the name's template parameters, if any, to ITEM.
   //
   void MoveTemplateParms(CxxScoped* item);

   //  Returns the number of names.
   //
   size_t Names_size() const;

   //  Returns the Nth name.
   //
   TypeName* Names_at(size_t idx) const;

   //  Returns the last name.
   //
   TypeName* Names_back() const;

   //  Returns the operator, if any, that follows the final name.
   //
   Cxx::Operator Operator() const { return oper_; }

   //  Sets the name's referent.  If the name appears in executable code, this
   //  is used by QualName.EnterBlock and Operation.PushMember.  It is also
   //  used by classes that contain a QualName member and that find a referent.
   //  Those classes do not contain executable code, so they can safely use the
   //  ref_ field because a QualName only uses it when it appears in executable
   //  code.
   //
   bool SetReferent(CxxNamed* ref) const;

   //  Returns the referent.  This is used in conjunction with SetReferent.  The
   //  class that contains a QualName instance cannot use the Referent function
   //  (overridden below) to access the referent because, if it is nullptr, the
   //  QualName will try to find it, starting with local variables.
   //
   CxxNamed* GetReferent() const { return ref_; }

   //  Records the typedef when ResolveTypedef returns true.
   //
   void SetTypedef(CxxNamed* type) const;

   //  Records the forward declaration when ResolveForward returns true.
   //
   void SetForward(CxxScoped* decl) const;

   //  Returns the forward declaration recorded by SetForward.
   //
   CxxScoped* GetForward() const;

   //  Invoked when the name accesses MEM via CLS.  Sets the referent to MEM
   //  and records CLS as the type through which it was accessed.
   //
   void MemberAccessed(Class* cls, CxxNamed* mem) const;

   //  Invoked by GetUsages to add, to directly used symbols, the class (if
   //  any) whose member was accessed by the name.
   //
   void GetClassUsage(const CodeFile& file, CxxUsageSets& symbols) const;

   //  Invokes MatchTemplate on each entry in names_, returing the least
   //  favorable result.
   //
   TypeMatch MatchTemplate(const QualName* that,
      stringVector& tmpltParms, stringVector& tmpltArgs, bool& argFound) const;

   //  Checks that the name is a valid constructor name ("...A::A").
   //
   bool CheckCtorDefn() const;

   //  Overridden to return type_ if it differs from ref_.
   //
   virtual CxxNamed* DirectType() const override;

   //  Overridden to find the referent and push it onto the argument stack.
   //
   virtual void EnterBlock() override;

   //  Overridden to return the item itself.
   //
   virtual QualName* GetQualName() const
      override { return const_cast< QualName* >(this); }

   //  Overridden to see if an entry in names_ specifies a template instance.
   //
   virtual TypeName* GetTemplateArgs() const override;

   //  Overridden to return the item's template parameters.
   //
   virtual const TemplateParms* GetTemplateParms() const
      override { return parms_.get(); }

   //  Overridden to update SYMBOLS with the name's type usage.
   //
   virtual void GetUsages
      (const CodeFile& file, CxxUsageSets& symbols) const override;

   //  Overridden to return the unqualified name (the name after the last
   //  scope resolution operator).
   //
   virtual const std::string* Name() const override;

   //  Overridden to display the name, including any template arguments.
   //
   virtual void Print(std::ostream& stream) const override;

   //  Overridden to return the qualified name.
   //
   virtual std::string QualifiedName
      (bool scopes, bool templates) const override;

   //  Overridden to return what the name refers to.
   //
   virtual CxxNamed* Referent() const override;

   //  Overridden to record and resolve the typedef.
   //
   virtual bool ResolveTypedef(Typedef* type) const override;

   //  Overridden to instantiate the template unless END is set.
   //
   virtual bool ResolveTemplate
      (Class* cls, const TypeName* args, bool end) const override;

   //  Sets the template parameters when the name declares a template.
   //
   virtual void SetTemplateParms(TemplateParmsPtr& parms) override;

   //  Overridden to shrink containers.
   //
   virtual void Shrink() override;

   //  Overridden to record access through a subclass name.
   //
   virtual void SubclassAccess(Class* cls) const override;

   //  Overridden to reveal that this is a qualified name.
   //
   virtual Cxx::ItemType Type() const override { return Cxx::QualName; }

   //  Overridden to return the referent's full root type.
   //
   virtual std::string TypeString(bool arg) const override;
private:
   //  Invoked when an error occurs in Referent().
   //
   CxxNamed* ReferentError(const std::string& item, debug32_t offset) const;

   union
   {
      TypeName* name_;            // an unqualified name
      TypeNamePtrVector* names_;  // the names in a qualified name
   };

   //  What the name refers to.
   //
   mutable CxxNamed* ref_;

   //  The class, if any, through which the name was accessed.
   //
   mutable Class* class_;

   //  The template parameters if the name declares a template.
   //
   TemplateParmsPtr parms_;

   //  The typedef, if any, resolved via ResolveTypedef.
   //
   mutable CxxNamed* type_;

   //  The forward declaration, if any, resolved via ResolveForward.
   //
   mutable CxxScoped* forw_;

   //  If and how ref_ was found via a using statement.
   //
   mutable UsingMode mode_: 8;

   //  The operator, if any, that follows the final name.
   //
   Cxx::Operator oper_ : 8;

   //  Set if the name begins with a scope resolution operator.
   //
   bool global_ : 8;

   //  Set if there are multiple names (using names_ instead of name_).
   //
   bool qualified_ : 8;
};

//------------------------------------------------------------------------------
//
//  One of the names in a qualified name.
//
class TypeName : public CxxNamed
{
public:
   //  NAME could be appearing alone or as part of a qualified name.
   //
   explicit TypeName(std::string& name);

   //  Not subclassed.
   //
   ~TypeName();

   //  Copy constructor.
   //
   TypeName(const TypeName& that);

   //  Adds a template argument (type specialization) to this name.
   //
   void AddTemplateArg(TypeSpecPtr& arg);

   //  Adds the string corresponding to OPER to the name.
   //
   void SetOperator(Cxx::Operator oper);

   //  Adds NAME to the last name.
   //
   void Append(const std::string& name);

   //  Invokes SetLocale on each template argument.
   //
   void SetLocale(Cxx::ItemType locale) const;

   //  Makes each template argument a template parameter.
   //
   void SetTemplateRole(TemplateRole role) const;

   //  If the type has template arguments, returns true if any of them are
   //  actually template parameters in SCOPE.
   //
   bool HasTemplateParmFor(const CxxScope* scope) const;

   //  Invoked when instantiating the template specified by the arguments.
   //  Invokes Instantiating on each template argument.
   //
   void Instantiating() const;

   //  Returns the template arguments.
   //
   const TypeSpecPtrVector* Args() const { return args_.get(); }

   //  Determines if THAT can instantiate a template for args_.
   //  o Returns Compatible if args_ is empty.
   //  o Returns Incompatible if THAT does not have the same number of
   //    template arguments as args_.
   //  o Invokes MatchTemplate on each entry in args_, returning the
   //    least favorable result.
   //
   TypeMatch MatchTemplate(const TypeName* that,
      stringVector& tmpltParms, stringVector& tmpltArgs, bool& argFound) const;

   //  Overridden to invoke FindReferent on each template argument.
   //
   virtual bool FindReferent() override;

   //  Overridden to return this item if it has template arguments.
   //
   virtual TypeName* GetTemplateArgs() const override;

   //  Overridden to update SYMBOLS with the name's type usage.
   //
   virtual void GetUsages
      (const CodeFile& file, CxxUsageSets& symbols) const override;

   //  Overridden to return the name.
   //
   virtual const std::string* Name() const override { return &name_; }

   //  Overridden to display the name.
   //
   virtual void Print(std::ostream& stream) const override;

   //  Overridden to return the name and, optionally, its template arguments.
   //
   virtual std::string QualifiedName
      (bool scopes, bool templates) const override;

   //  Overridden to shrink containers.
   //
   virtual void Shrink() override;

   //  Overridden to return the full type for template arguments.  Note that
   //  the name itself is omitted.
   //
   virtual std::string TypeString(bool arg) const override;
private:
   //  The name that appears in what could be a qualified name.
   //
   std::string name_;

   //  Any template arguments if the name is that of a template.
   //
   std::unique_ptr< TypeSpecPtrVector > args_;
};

//------------------------------------------------------------------------------
//
//  An item's underlying type.
//
class TypeSpec : public CxxNamed
{
public:
   //  Virtual to allow subclassing.
   //
   virtual ~TypeSpec() { }

   //  Sets the type of item to which the type belongs.
   //
   virtual void SetLocale(Cxx::ItemType locale);

   //  Returns the type of item in which the type appears.
   //
   Cxx::ItemType GetLocale() const { return locale_; }

   //  Returns the type's role, if any, in a template.
   //
   TemplateRole GetTemplateRole() const { return role_; }

   //  Returns the function signature for a function type.
   //
   virtual Function* GetFuncSpec() const { return nullptr; }

   //  Returns the level of compatibility when assigning THAT to this type.
   //  If IMPLICIT is set, the "explicit" keyword is ignored.  Generates a
   //  log if the types are Incompatible.
   //
   TypeMatch MustMatchWith(const StackArg& that, bool implicit = false) const;

   //  Creates and returns a copy of the type.  Serves as a copy constructor,
   //  which cannot be invoked directly on this class (because it is virtual).
   //
   virtual TypeSpec* Clone() const = 0;

   //  Adds a bounded array specification to the type.
   //
   virtual void AddArray(ArraySpecPtr& array) = 0;

   //  Sets the type's constness.
   //
   virtual void SetConst(bool readonly) = 0;

   //  Sets any pointer's constness.
   //
   virtual void SetConstPtr(bool constptr) = 0;

   //  Sets the level of pointer indirection to the type.
   //
   virtual void SetPtrs(TagCount ptrs) = 0;

   //  Sets the level of reference indirection to the type.
   //
   virtual void SetRefs(TagCount refs) = 0;

   //  Specifies the location of an unbounded array tag.
   //
   virtual void SetArrayPos(int8_t pos) = 0;

   //  Invoked when a pointer tag is detached from the type name.
   //
   virtual void SetPtrDetached(bool on) = 0;

   //  Invoked when a reference tag is detached from the type name.
   //
   virtual void SetRefDetached(bool on) = 0;

   //  Sets what the type refers to.  MODE indicates whether the item was
   //  made visible by an #include or using statement.
   //
   virtual void SetReferent(CxxNamed* ref, UsingMode mode) = 0;

   //  Returns the number of pointer tags attached to the type.  It follows
   //  the type to its root (a class or terminal), adding up pointer tags
   //  and, if ARRAYS is set, also arrays.
   //
   virtual TagCount Ptrs(bool arrays) const = 0;

   //  Returns the number of reference tags attached to the type.  It follows
   //  the type to its root (a class or terminal), stopping at the first type
   //  that has reference tag.
   //
   virtual TagCount Refs() const = 0;

   //  Returns the number of arrays attached to the type.  It follows the type
   //  to its root (a class or terminal), adding up the number of arrays.
   //
   virtual TagCount Arrays() const = 0;

   //  Returns true if the type has any bounded array specifications.
   //
   virtual bool HasArrayDefn() const = 0;

   //  Invoked when the type is entered into SCOPE.
   //
   virtual void EnteringScope(const CxxScope* scope) = 0;

   //  Invokes EnterBlock on the bounded array specifications.
   //
   virtual void EnterArrays() const = 0;

   //  Returns the type's attributes.
   //
   virtual TypeTags GetTags() const = 0;

   //  The same as TypeString, except it applies TAGS instead of the
   //  type's actual attributes.
   //
   virtual std::string TypeTagsString(const TypeTags& tags) const = 0;

   //  Displays the type's pointer, array, and reference tags.  Bounded
   //  arrays are omitted and must be displayed using DisplayArrays.
   //
   virtual void DisplayTags(std::ostream& stream) const = 0;

   //  Displays the bounded array specifications.
   //
   virtual void DisplayArrays(std::ostream& stream) const = 0;

   //  Adjusts the number of pointer tags when the type is assigned to an
   //  auto type.  For example, when auto a = *p; finds the type for P and
   //  assigns it to A, the level of pointer indirection is decremented.
   //
   virtual void AdjustPtrs(TagCount count) = 0;

   //  Eliminates references.  Used when the right-hand side of an auto
   //  type is a reference, but "auto" is used instead of "auto&".
   //
   virtual void RemoveRefs() = 0;

   //  Returns the number of pointers associated with this individual type.
   //  If ARRAYS is set, each [] counts as a pointer.
   //
   virtual TagCount PtrCount(bool arrays) const = 0;

   //  Returns the number of references associated with this individual type.
   //
   virtual TagCount RefCount() const = 0;

   //  Returns the number of arrays associated with this individual type.
   //
   virtual TagCount ArrayCount() const = 0;

   //  Returns true if the types of "this" and THAT match exactly, including
   //  all tags (constness, pointers, arrays, and references).
   //
   virtual bool MatchesExactly(const TypeSpec* that) const = 0;

   //  Constructs an argument based on the type's referent.
   //
   virtual StackArg ResultType() const = 0;

   //  Sets the type's role in a template.
   //
   virtual void SetTemplateRole(TemplateRole role) const { role_ = role; }

   //  Determines if THAT can instantiate a template for this type.
   //  o If this type is a template parameter in tmpltParms, argFound is set,
   //    and THAT is added to tmpltArgs as the corresponding template argument
   //    unless a different argument was found previously.
   //  o If this is not a template parameter, THAT must have the same type.
   //  Returns Incompatible if the above conditions are not satisifed.  Returns
   //  Compatible on an exact match, or something else on a partial match (if,
   //  for example, THAT is a pointer type and the template parameter is not).
   //
   virtual TypeMatch MatchTemplate(TypeSpec* that, stringVector& tmpltParms,
      stringVector& tmpltArgs, bool& argFound) const = 0;

   //  Determines how well THAT matches this type for the purpose of template
   //  instantiation.
   //
   virtual TypeMatch MatchTemplateArg(const TypeSpec* that) const = 0;

   //  During template instantiation, aligns thatArg's type with this type,
   //  which is that of a template parameter that might be a specialization.
   //
   virtual std::string AlignTemplateArg(const TypeSpec* thatArg) const = 0;

   //  Invoked when the type is a template argument that is about to be used
   //  to instantiate a template.  Finds the type's referent and, if it is a
   //  template, also instantiates it.
   //
   virtual void Instantiating() const = 0;

   //  Overridden to reveal that this is a type specification.
   //
   virtual Cxx::ItemType Type() const override { return Cxx::TypeSpec; }
protected:
   //  Protected because this class is virtual.
   //
   TypeSpec();

   //  Copy constructor.
   //
   TypeSpec(const TypeSpec& that);
private:
   //  The item type to which the type belongs.  The default is Cxx::Operation.
   //
   Cxx::ItemType locale_ : 8;

   //  The type's role in a template.
   //
   mutable TemplateRole role_ : 8;
};

//------------------------------------------------------------------------------
//
//  A name, possibily qualified, that is being used as a type.
//
class DataSpec : public TypeSpec
{
public:
   //  Creates a type for NAME.
   //
   explicit DataSpec(QualNamePtr& name);

   //  Creates a type for NAME.
   //
   explicit DataSpec(const char* name);

   //  Copy constructor.
   //
   //  NOTE: arrays_ is not copied.  This means that a type with a bounded
   //  ====  array cannot be used to automatically create an instance of a
   //        function template.
   //
   DataSpec(const DataSpec& that);

   //  Not subclassed.
   //
   ~DataSpec();
private:
   //  Returns true if the type was declared as auto, even if (unlike IsAuto)
   //  its underlying type has been determined.
   //
   bool IsAutoDecl() const;

   //  Returns true if the type did not have to be defined before it was used
   //  in this specification.  This means that the type is either built in
   //  (e.g. bool, int) or that it could be used after a forward declaration.
   //
   bool IsUsedInNameOnly() const;

   //  Resolves a template argument when parsing a template instance.  Returns
   //  false if this is not a template argument in a template instance.
   //
   bool ResolveTemplateArgument();

   //  Overridden to add a bounded array specification to the type.
   //
   virtual void AddArray(ArraySpecPtr& array) override;

   //  Overridden to adjust the number of pointer tags when the type is assigned
   //  to an auto type.
   //
   virtual void AdjustPtrs(TagCount count) override;

   //  Overridden to align thatArg's type with the this type, which is that of a
   //  template parameter that might be specialized.
   //
   virtual std::string AlignTemplateArg(const TypeSpec* thatArg) const override;

   //  Overridden to return the number of arrays associated with this type.
   //
   virtual TagCount ArrayCount() const override;

   //  Overridden to return the number of arrays attached to the type, following
   //  the type to its root.
   //
   virtual TagCount Arrays() const override;

   //  Overridden to set the type to assign to an "auto" variable.
   //
   virtual CxxToken* AutoType() const override { return (CxxToken*) this; }

   //  Overridden to find the type's referent and to log warnings associated
   //  with its specification.  Because it verifies that pointer and reference
   //  tags are attached to types, not names, the parser invokes it during the
   //  parse phase.  The reason is that a TypeSpec gets deleted when a data or
   //  function definition is merged into its original declaration, so the
   //  checks must occur before this happens.
   //
   virtual void Check() const override;

   //  Overridden to create and return a copy of the type.
   //
   virtual TypeSpec* Clone() const override;

   //  Overridden to return the class, if any, to which the type ultimately
   //  refers, provided that it is not a pointer or reference to that class.
   //
   virtual Class* DirectClass() const override;

   //  Overridden to invoke DirectType on name_.
   //
   virtual CxxNamed* DirectType() const override;

   //  Overridden to display the type's bounded array specifications.
   //
   virtual void DisplayArrays(std::ostream& stream) const override;

   //  Overridden to display the type's pointer, array, and reference tags.
   //
   virtual void DisplayTags(std::ostream& stream) const override;

   //  Overridden to invoke EnterBlock on any bounded array specifications.
   //
   virtual void EnterArrays() const override;

   //  Overridden to push the type's referent onto the argument stack.
   //
   virtual void EnterBlock() override;

   //  Overridden to see if the type is a template parameter in SCOPE and
   //  to invoke Check and EnterArrays.
   //
   virtual void EnteringScope(const CxxScope* scope) override;

   //  Overridden to find the type's referent, as well as the referent for
   //  each template argument used in the type.
   //
   virtual bool FindReferent() override;

   //  Overridden to return the type's attributes.
   //
   virtual TypeTags GetTags() const override;

   //  Overridden to return the numeric type.
   //
   virtual Numeric GetNumeric() const override;

   //  Overridden to return the type's qualified name.
   //
   virtual QualName* GetQualName() const override { return name_.get(); }

   //  Overridden to return the referent's scope, if known.
   //
   virtual CxxScope* GetScope() const override;

   //  Overridden to return the type itself.
   //
   virtual TypeSpec* GetTypeSpec() const override;

   //  Overridden to update SYMBOLS with the specification's type usage.
   //
   virtual void GetUsages
      (const CodeFile& file, CxxUsageSets& symbols) const override;

   //  Overridden to return true if the type has a bounded array specification.
   //
   virtual bool HasArrayDefn() const override { return (arrays_ == nullptr); }

   //  Overridden to instantiate the type's referent if it is a template.
   //
   virtual void Instantiating() const override;

   //  Overridden to return true if the type is "auto" and the referent has
   //  yet to be determined.
   //
   virtual bool IsAuto() const override;

   //  Overridden to return true if the type is const.
   //
   virtual bool IsConst() const override;

   //  Overridden to return true if the type is a const pointer.
   //
   virtual bool IsConstPtr() const override;

   //  Overridden to return true if the type has pointer or reference tags.
   //
   virtual bool IsIndirect() const override;

   //  Overridden to return true if the types of "this" and THAT match.
   //
   virtual bool MatchesExactly(const TypeSpec* that) const override;

   //  Overridden to determine if THAT can instantiate a template for this type.
   //
   virtual TypeMatch MatchTemplate(TypeSpec* that, stringVector& tmpltParms,
      stringVector& tmpltArgs, bool& argFound) const override;

   //  Overridden to determine how well THAT matches this type for the purpose
   //  of template instantiation.
   //
   virtual TypeMatch MatchTemplateArg(const TypeSpec* that) const override;

   //  Overridden to return the type's name.
   //
   virtual const std::string* Name() const override { return name_->Name(); }

   //  Overridden to display the type.
   //
   virtual void Print(std::ostream& stream) const override;

   //  Overridden to return the number of pointers associated with this type.
   //  Each array specification is counted as a pointer if ARRAYS is set.
   //
   virtual TagCount PtrCount(bool arrays) const override;

   //  Overridden to return the number of pointer tags attached to the type,
   //  following the type to its root.
   //
   virtual TagCount Ptrs(bool arrays) const override;

   //  Overridden to return the type's qualified name.
   //
   virtual std::string QualifiedName(bool scopes, bool templates) const
      override { return name_->QualifiedName(scopes, templates); }

   //  Overridden to return the number of references associated with this type.
   //
   virtual TagCount RefCount() const override { return refs_; }

   //  Overridden to return what the type refers to.
   //
   virtual CxxNamed* Referent() const override;

   //  Overridden to return the number of reference tags attached to the type,
   //  following the type to its root.
   //
   virtual TagCount Refs() const override;

   //  Overridden to eliminate reference tags from an auto type.
   //
   virtual void RemoveRefs() override;

   //  Overridden to return the forward declaration unless it has template
   //  arguments.
   //
   virtual bool ResolveForward(CxxScoped* decl) const override;

   //  Overridden to determine if EnsureInstance should be invoked.
   //
   virtual bool ResolveTemplate
      (Class* cls, const TypeName* args, bool end) const override;

   //  Overridden to return the typedef unless it has template arguments.
   //
   virtual bool ResolveTypedef(Typedef* type) const override;

   //  Overridden to construct an argument based on the type of referent and
   //  the level of pointer indirection to it.
   //
   virtual StackArg ResultType() const override;

   //  Overridden to return the underlying type.
   //
   virtual CxxToken* RootType() const override { return Referent(); }

   //  Overridden to set the location of an unbounded array tag.
   //
   virtual void SetArrayPos(int8_t pos) override { arrayPos_ = pos; }

   //  Overridden to set the type's constness.
   //
   virtual void SetConst(bool readonly) override { const_ = readonly; }

   //  Overridden to set a pointer type's constness.
   //
   virtual void SetConstPtr(bool constptr) override { constptr_ = constptr; }

   //  Overridden to propagate the locale to name_.
   //
   virtual void SetLocale(Cxx::ItemType locale) override;

   //  Overridden to record a pointer tag that is detached from the type name.
   //
   virtual void SetPtrDetached(bool on) override { ptrDet_ = on; }

   //  Overridden to set the level of pointer indirection to the type.
   //
   virtual void SetPtrs(TagCount ptrs) override { ptrs_ = ptrs; }

   //  Overridden to record a reference tag that is detached from the type name.
   //
   virtual void SetRefDetached(bool on) override { refDet_ = on; }

   //  Overridden to set what the type refers to.
   //
   virtual void SetReferent(CxxNamed* ref, UsingMode mode) override;

   //  Overridden to set the level of reference indirection to the type.
   //
   virtual void SetRefs(TagCount refs) override { refs_ = refs; }

   //  Overridden so that when ROLE is TemplateClass, its arguments are treated
   //  as parameters.
   //
   virtual void SetTemplateRole(TemplateRole role) const override;

   //  Overridden to shrink containers.
   //
   virtual void Shrink() override;

   //  Overridden to record access through a subclass name.
   //
   virtual void SubclassAccess(Class* cls) const override;

   //  Overridden to return the base class default (the scoped name) unless the
   //  type is "auto", in which case the referent (if known) is returned.
   //
   virtual std::string Trace() const override;

   //  Overridden to return the full root type.  Reference tags ('&') are
   //  omitted if ARG is set.
   //
   virtual std::string TypeString(bool arg) const override;

   //  Overridden to apply TAGS instead of the type's actual attributes.
   //
   virtual std::string TypeTagsString(const TypeTags& tags) const override;

   //  Overridden to support a temporary variable represented by a DataSpec.
   //
   virtual bool WasWritten(const StackArg* arg, bool passed)
      override { return false; }

   //  The qualified name for the type as it appeared in the source code.
   //
   QualNamePtr name_;

   //  The type's bounded array specifications (e.g. for int[10][10]).
   //
   std::unique_ptr< ArraySpecPtrVector > arrays_;

   //  The level of pointer indirection to the type.
   //
   TagCount ptrs_ : 8;

   //  The level of reference indirection to the type.
   //
   TagCount refs_ : 8;

   //  The position of any unbounded array tag ("[]"), else INT8_MAX.
   //
   int8_t arrayPos_ : 8;

   //  Set if the type is const.
   //
   bool const_ : 1;

   //  Set if the type is a const pointer.
   //
   bool constptr_ : 1;

   //  If and how ref_ was found via a using statement.
   //
   UsingMode mode_ : 2;

   //  Set if a pointer tag was detached.
   //
   bool ptrDet_ : 1;

   //  Set if a reference tag was detached.
   //
   bool refDet_ : 1;
};

//------------------------------------------------------------------------------
//
//  A template parameter appears within the angle brackets that follow the
//  "template" keyword.
//
class TemplateParm : public CxxNamed
{
public:
   //  Creates a parameter defined by NAME, TYPE, and PTRS (e.g. T/class/1
   //  for template <class T*...), which may specify an optional DEFAULT.
   //
   TemplateParm(std::string& name,
      Cxx::ClassTag tag, size_t ptrs, TypeNamePtr& default);

   //  Not subclassed.
   //
   ~TemplateParm() { CxxStats::Decr(CxxStats::TEMPLATE_PARM); }

   //  Copy constructor.
   //
   TemplateParm(const TemplateParm& that);

   //  Returns the parameter's default type.
   //
   const TypeName* Default() const { return default_.get(); }

   //  Overridden to return the parameter's name.
   //
   virtual const std::string* Name() const override { return &name_; }

   //  Overridden to display the parameter.
   //
   virtual void Print(std::ostream& stream) const override;

   //  Overridden to shrink the item's name.
   //
   virtual void Shrink() override;

   //  Overridden to return the parameter's name and pointers.
   //
   virtual std::string TypeString(bool arg) const override;
private:
   //  The parameter's name.
   //
   std::string name_;

   //  The parameter's type tag.
   //
   const Cxx::ClassTag tag_;

   //  The level of pointer indirection for the parameter.
   //
   const size_t ptrs_;

   //  The default type, if any, for the parameter.
   //
   TypeNamePtr default_;
};

//------------------------------------------------------------------------------
//
//  Parameters associated with a template declaration.  Even though this is
//  subclassed from CxxToken, it is put here to make TemplateParm's definition
//  visible, as the unique_ptrs in parms_ must be able to see its destructor.
//
class TemplateParms : public CxxToken
{
public:
   //  Creates a template declaration in which PARM is the first parameter
   //  (e.g. T/typename/1 for template <typename T*...).
   //
   explicit TemplateParms(TemplateParmPtr& parm);

   //  Not subclassed.
   //
   ~TemplateParms() { CxxStats::Decr(CxxStats::TEMPLATE_PARMS); }

   //  Copy constructor.
   //
   TemplateParms(const TemplateParms& that);

   //  Adds another parameter to the template.
   //
   void AddParm(TemplateParmPtr& parm);

   //  Returns the template's parameters.
   //
   const TemplateParmPtrVector* Parms() const { return &parms_; }

   //  Overridden to display the template's full specification.
   //
   virtual void Print(std::ostream& stream) const override;

   //  Overridden to shrink containers.
   //
   virtual void Shrink() override;

   //  Overridden to return the template's parameter names in angle brackets.
   //
   virtual std::string TypeString(bool arg) const override;
private:
   //  The template's parameters.
   //
   TemplateParmPtrVector parms_;
};
}
#endif
