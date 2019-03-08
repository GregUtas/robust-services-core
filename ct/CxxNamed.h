//==============================================================================
//
//  CxxNamed.h
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
#ifndef CXXNAMED_H_INCLUDED
#define CXXNAMED_H_INCLUDED

#include "CxxToken.h"
#include <cstddef>
#include <iosfwd>
#include <string>
#include "CodeTypes.h"
#include "Cxx.h"
#include "CxxFwd.h"
#include "CxxString.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace CodeTools
{
//  Where a C++ item was declared or defined.
//
class CxxLocation
{
   friend class CxxNamed;
public:
   //  Returns the file in which the item is located.  A template
   //  instance belongs to the file that caused its instantiation.
   //  An item added by the Editor belongs to the file to which it
   //  was added.
   //
   CodeFile* GetFile() const { return file_; }

   //  Returns the start of the item's position within its file, which
   //  is an index into a string that contains the file's contents.
   //  For a template instance, this is an offset into its internally
   //  generated code.  For an item added by the Editor, string::npos
   //  is returned.
   //
   size_t GetPos() const;

   //  Returns true for an internally generated item, such as the code
   //  for a template instance.
   //
   bool IsInternal() const { return internal_; }

   //  A position that indicates that the item was not found in the
   //  original source code.  This is used when the Editor creates
   //  an item, for example.
   //
   static const size_t NOT_IN_SOURCE = 0x7fffffff;
private:
   //  Initializes fields to default values.  Private because this class
   //  only appears as a private member of the friend class CxxNamed.
   //
   CxxLocation();

   //  Copy constructor.
   //
   CxxLocation(const CxxLocation& that) = default;

   //  Copy operator.
   //
   CxxLocation& operator=(const CxxLocation& that) = default;

   //  Records the item's location in source code.
   //
   void SetLoc(CodeFile* file, size_t pos);

   //  Marks the item as internally generated.
   //
   void SetInternal() { internal_ = true; }

   //  The file in which the item appeared.
   //
   CodeFile* file_;

   //  The item's location in FILE.  The file has a string member which
   //  contains the code, and this is an index into that string.
   //
   size_t pos_ : 31;

   //  Set if the item appeared in internally generated code, which currently
   //  means in a template instance.
   //
   bool internal_ : 1;
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

   //  Sets the file and offset at which this item was found.
   //
   virtual void SetLoc(CodeFile* file, size_t pos);

   //  Sets the context in which this item was found:
   //  o Invokes SetScope(Context::Scope()) unless the item already has a scope
   //  o invokes SetAccess(item's scope->GetCurrAccess())
   //  o invokes SetLoc(Context::File(), pos)
   //  o invokes SetInternal() if Context::ParsingTemplateInstance() is true
   //
   void SetContext(size_t pos);

   //  Sets the item's context based on THAT.  Used when an item is created
   //  internally (e.g. a "this" argument).
   //
   virtual void CopyContext(const CxxNamed* that);

   //  Returns the item's location information.
   //
   const CxxLocation& GetLoc() const { return loc_; }

   //  Returns the file in which this item was found.
   //
   CodeFile* GetFile() const { return loc_.GetFile(); }

   //  Returns the offset at which the item was found.
   //
   size_t GetPos() const { return loc_.GetPos(); }

   //  Sets BEGIN and END to where the item begins and ends, and returns
   //  the location of its opening left brace (if applicable).  The default
   //  sets BEGIN and END to string::npos and also returns string::npos.
   //
   virtual size_t GetRange(size_t& begin, size_t& end) const;

   //  Returns the scope (namespace, class, or block) where the item is
   //  declared.
   //
   virtual CxxScope* GetScope() const { return nullptr; }

   //  Returns true if the item is static.  Note that, for the purposes
   //  of this function:
   //  o only data and functions can be classified as non-static;
   //  o class membership for non-static data and functions must be
   //    checked separately, using if(item->GetClass() != nullptr).
   //
   virtual bool IsStatic() const { return true; }

   //  Returns true if the item was declared in a function's code block
   //  or argument list.
   //
   virtual bool IsDeclaredInFunction() const { return false; }

   //  Returns true if the item appeared in internally generated code.
   //
   bool IsInternal() const { return loc_.IsInternal(); }

   //  Sets the template parameters when the item declares a template.
   //  The default version generates a log and must be overridden by an
   //  item that can declare a template.
   //
   virtual void SetTemplateParms(TemplateParmsPtr& parms);

   //  Returns the template parameters associated with the item, if any.
   //  Must be overridden by an item that can declare a template.
   //
   virtual const TemplateParms* GetTemplateParms() const { return nullptr; }

   //  Returns true if the item declares template parameters.
   //
   bool IsTemplate() const { return GetTemplateParms() != nullptr; }

   //  Returns the template, if any, associated with a class or function.
   //
   virtual CxxScope* GetTemplate() const { return nullptr; }

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

   //  Updates NAMES with the item's fully qualified name(s).  Each name omits
   //  template arguments but prefixes a scope resolution operator.
   //
   virtual void GetScopedNames(stringVector& names) const;

   //  Returns true if this item is a superscope of fqSub.  TMPLT is set if
   //  a template should be considered a superscope of one of its instances.
   //  This version returns false because the superscope's fully qualified name
   //  is required but is only available in classes derived from CxxScoped.
   //
   virtual bool IsSuperscopeOf(const std::string& fqSub, bool tmplt) const
      { return false; }

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

   //  Returns the file that *declared* the item.  Declaration is distinct
   //  from definition for extern data and functions, and often for static
   //  class data.  Such items appear twice, with one being the declaration
   //  and the other the definition.
   //
   virtual CodeFile* GetDeclFile() const { return GetFile(); }

   //  Returns the identifier of the file in which the item was declared.
   //
   virtual NodeBase::id_t GetDeclFid() const;

   //  Returns the file that *defined* the item.  Returns nullptr if the
   //  item has no definition or if it was defined where it was declared.
   //
   virtual CodeFile* GetDefnFile() const { return nullptr; }

   //  Returns true if the item was declared at file scope.
   //
   bool AtFileScope() const;

   //  Returns the item's mate.  Returns nullptr unless the item is declared
   //  and defined separately, in which case it returns the other instance.
   //
   virtual CxxNamed* GetMate() const { return nullptr; }

   //  Invoked before adding the item to the current scope (Context::Scope()).
   //  Returning false indicates that
   //  o for a C++ item, that it is a definition of a previous declaration;
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

   //  Returns the item's direct type, which could be a typedef or forward
   //  declaration.  To follow either of these to the final underlying type,
   //  use CxxToken.Root.
   //
   virtual CxxNamed* DirectType() const { return Referent(); }

   //  Finds what the item refers to.  The default version generates a log.
   //
   virtual void FindReferent();

   //  Sets the name's referent to ITEM.  VIEW (if not nullptr) provides
   //  information about how the name was resolved.  The default version
   //  generates a log.
   //
   virtual void SetReferent(CxxNamed* item, const SymbolView* view) const;

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

   //  Logs WARNING at the position where this item is located.  ITEM
   //  and OFFSET are specific to WARNING, and HIDE is set to prevent
   //  the warning from being displayed.  If ITEM is nullptr, "this" is
   //  included in the log.
   //
   void Log(Warning warning, const CxxNamed* item = nullptr,
      NodeBase::word offset = 0, bool hide = false) const;

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
   Class* GetClass() const override;

   //  Overridden to use GetScope to find an item's namespace.
   //
   Namespace* GetSpace() const override;

   //  Overridden to return the item's scoped name.
   //
   std::string Trace() const override { return ScopedName(true); }
protected:
   //  Protected because this class is virtual.
   //
   CxxNamed();

   //  Copy constructor.
   //
   CxxNamed(const CxxNamed& that);

   //  Sets the scope where the item was found.  For classes derived from
   //  CxxScoped, this is usually the scope where the item is declared.
   //
   virtual void SetScope(CxxScope* scope) { }

   //  Sets the access control that applies to the item.
   //
   virtual void SetAccess(Cxx::Access access) { }

   //  Resolves the item's qualified name.  FILE, SCOPE, MASK, and VIEW are
   //  the same as the arguments for CxxSymbols::FindSymbol.
   //
   CxxNamed* ResolveName(const CodeFile* file, const CxxScope* scope,
      const NodeBase::Flags& mask, SymbolView* view) const;

   //  Invoked when ResolveName finds TYPE, a typedef.  If it returns false,
   //  ResolveName returns TYPE.  Otherwise, name resolution continues with
   //  TYPE's referent (its underlying type).  In a qualified name, N is the
   //  index of the name associated with the typedef.
   //
   virtual bool ResolveTypedef(Typedef* type, size_t n) const { return true; }

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
   //  Indicates that the item appeared in internally generated code.
   //
   void SetInternal() { loc_.SetInternal(); }

   //  Invoked when ResolveName finds DECL, a forward or friend declaration,
   //  when resolving the Nth name in a possibly qualified name.  If it
   //  returns false, ResolveName returns DECL.  Otherwise, name resolution
   //  continues with DECL's referent (the class or function to which it
   //  refers).  If this is still unknown, DECL is returned.
   //
   virtual bool ResolveForward
      (CxxScoped* decl, size_t n) const { return false; }

   //  Deleted to prohibit copy assignment.
   //
   CxxNamed& operator=(const CxxNamed& that) = delete;

   //  The location where the item appeared.
   //
   CxxLocation loc_;
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
   void GetUsages(const CodeFile& file, CxxUsageSets& symbols) const override;

   //  Overridden to return the member's name.
   //
   const std::string* Name() const override { return &name_; }

   //  Overridden to display the initialization statement.
   //
   void Print
      (std::ostream& stream, const NodeBase::Flags& options) const override;

   //  Overridden to shrink containers.
   //
   void Shrink() override;

   //  Overridden to return the member's name.
   //
   std::string Trace() const override { return name_; }
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

   //  In a qualified name, adds TYPE as the name that follows this one.
   //  A scope resolution operator separates the two names.
   //
   void PushBack(TypeNamePtr& type);

   //  Returns the next name in a qualified name.
   //
   TypeName* Next() const { return next_.get(); }

   //  Invoked if a scope resolution operator preceded the name.
   //
   void SetScoped() { scoped_ = true; }

   //  Returns true if the name was preceded by a scope resolution operator.
   //
   bool IsScoped() const { return scoped_; }

   //  Adds a template argument (type specialization) to this name.
   //
   void AddTemplateArg(TypeSpecPtr& arg);

   //  Returns the template arguments.
   //
   const TypeSpecPtrVector* Args() const { return args_.get(); }

   //  Adds the string corresponding to OPER to the name.
   //
   void SetOperator(Cxx::Operator oper);

   //  Returns the operator, if any, that follows the name.
   //
   Cxx::Operator Operator() const { return oper_; }

   //  Adds the suffix NAME to the name.  Adds a space first if SPACE is set.
   //
   void Append(const std::string& name, bool space);

   //  Invokes SetUserType on each template argument.
   //
   void SetUserType(Cxx::ItemType user) const;

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

   //  Determines if THAT can instantiate a template for args_.
   //  o Returns Compatible if args_ is empty.
   //  o Returns Incompatible if THAT does not have the same number of
   //    template arguments as args_.
   //  o Invokes MatchTemplate on each entry in args_, returning the
   //    least favorable result.
   //
   TypeMatch MatchTemplate(const TypeName* that,
      stringVector& tmpltParms, stringVector& tmpltArgs, bool& argFound) const;

   //  Returns true if ITEM is the referent of a template argument.
   //
   bool ItemIsTemplateArg(const CxxNamed* item) const;

   //  Invoked when the name accesses MEM via CLS.  Sets the referent to MEM
   //  and records CLS as the type through which it was accessed.
   //
   void MemberAccessed(Class* cls, CxxNamed* mem) const;

   //  Invoked when the name is that of a member which a qualified name
   //  accessed via a subclass (derived::member instead of base::member).
   //
   void SubclassAccess(Class* cls) const;

   //  Records the forward declaration when ResolveForward returns true.
   //
   void SetForward(CxxScoped* decl) const;

   //  Returns the forward declaration recorded by SetForward.
   //
   CxxScoped* GetForward() const { return forw_; }

   //  Overridden to check template arguments.
   //
   void Check() const override;

   //  Overridden to return type_ if it exists, else ref_.
   //
   CxxNamed* DirectType() const override;

   //  Overridden to invoke FindReferent on each template argument.
   //
   void FindReferent() override;

   //  Overridden to return this item if it has template arguments.
   //
   TypeName* GetTemplateArgs() const override;

   //  Overridden to update SYMBOLS with the name's type usage.
   //
   void GetUsages(const CodeFile& file, CxxUsageSets& symbols) const override;

   //  Overridden to return the name.
   //
   const std::string* Name() const override { return &name_; }

   //  Overridden to display the name.
   //
   void Print
      (std::ostream& stream, const NodeBase::Flags& options) const override;

   //  Overridden to return the name and, optionally, its template arguments.
   //
   std::string QualifiedName(bool scopes, bool templates) const override;

   //  Overridden to return what the name refers to.
   //
   CxxNamed* Referent() const override { return ref_; }

   //  Overridden to record and resolve the typedef.
   //
   bool ResolveTypedef(Typedef* type, size_t n) const override;

   //  Overridden to record what the item refers to.
   //
   void SetReferent(CxxNamed* item, const SymbolView* view) const override;

   //  Overridden to shrink containers.
   //
   void Shrink() override;

   //  Overridden to return the full type for template arguments.  Note that
   //  the name itself is omitted.
   //
   std::string TypeString(bool arg) const override;
private:
   //  Deleted to prohibit copy assignment.
   //
   TypeName& operator=(const TypeName& that) = delete;

   //  The name that appears in what could be a qualified name.
   //
   std::string name_;

   //  Any template arguments if the name is that of a template.
   //
   std::unique_ptr< TypeSpecPtrVector > args_;

   //  The next name in a qualified name.
   //
   TypeNamePtr next_;

   //  What the name refers to.
   //
   mutable CxxNamed* ref_;

   //  The class, if any, through which the name was accessed.
   //
   mutable Class* class_;

   //  The typedef, if any, resolved via ResolveTypedef.
   //
   mutable CxxNamed* type_;

   //  The forward declaration, if any, resolved via ResolveForward.
   //
   mutable CxxScoped* forw_;

   //  The operator, if any, that follows the name.
   //
   Cxx::Operator oper_ : 8;

   //  Set if a scope resolution operator precedes the name.
   //  Initialized to false; must be set by SetScoped.
   //
   bool scoped_ : 1;

   //  Set if ref_ was made visible by a using statement.
   //
   mutable bool using_ : 1;
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

   //  Adds TYPE to the name.  In a qualified name, TYPE is preceded by a
   //  scope resolution operator.
   //
   void PushBack(TypeNamePtr& type);

   //  Returns the first name.
   //
   TypeName* First() const { return first_.get(); }

   //  Returns the Nth name.
   //
   TypeName* At(size_t n) const;

   //  Returns the number of names.
   //
   size_t Size() const;

   //  Returns true if a scope resolution operator preceded the first name.
   //
   bool IsGlobal() const { return First()->IsScoped(); }

   //  Adds the suffix NAME to the last name.  Adds a space first if SPACE
   //  is set.
   //
   void Append(const std::string& name, bool space) const;

   //  Invoked when an operator follows the last name, which is "operator".
   //
   void SetOperator(Cxx::Operator oper) const;

   //  Returns the operator, if any, that follows the last name.
   //
   Cxx::Operator Operator() const { return Last()->Operator(); }

   //  Invokes SetUserType on each name.
   //
   void SetUserType(Cxx::ItemType user) const;

   //  Sets the referent of the Nth name to ITEM.  VIEW provides information
   //  about how the name was resolved.  If name resolution failed, ITEM will
   //  be nullptr.  If whoever requested name resolution did not provide a
   //  SymbolView, VIEW will be nullptr.
   //
   void SetReferentN(size_t n, CxxNamed* item, const SymbolView* view) const;

   //  Returns the last name's referent.  This is used in conjunction with
   //  SetReferent.  A class that contains a QualName instance cannot use the
   //  Referent function (overridden below) to access the referent because,
   //  if it is nullptr, the QualName will try to find it, starting with local
   //  variables.
   //
   CxxNamed* GetReferent() const { return Last()->Referent(); }

   //  Returns the last name that was resolved by a forward declaration.
   //
   CxxScoped* GetForward() const;

   //  Invoked when the name accesses MEM via CLS.  Forwards to the first name
   //  that has no referent and that matches MEM's name.
   //
   void MemberAccessed(Class* cls, CxxNamed* mem) const;

   //  Invokes MatchTemplate on each name, returning the least favorable result.
   //
   TypeMatch MatchTemplate(const QualName* that,
      stringVector& tmpltParms, stringVector& tmpltArgs, bool& argFound) const;

   //  Returns true if ITEM is the referent of a template argument.
   //
   bool ItemIsTemplateArg(const CxxNamed* item) const;

   //  Checks that the name is a valid constructor name ("...A::A").
   //
   bool CheckCtorDefn() const;

   //  Overridden to check each name and any template parameters.
   //
   void Check() const override;

   //  Overridden to propagate the context to each name.
   //
   void CopyContext(const CxxNamed* that) override;

   //  Overridden to forward to the last name.
   //
   CxxNamed* DirectType() const
      override { return Last()->DirectType(); }

   //  Overridden to find the referent and push it onto the argument stack.
   //
   void EnterBlock() override;

   //  Overridden to return the item itself.
   //
   QualName* GetQualName() const
      override { return const_cast< QualName* >(this); }

   //  Overridden to see if one of the names specifies a template instance.
   //
   TypeName* GetTemplateArgs() const override;

   //  Overridden to update SYMBOLS with the name's type usage.
   //
   void GetUsages(const CodeFile& file, CxxUsageSets& symbols) const override;

   //  Overridden to return the last name.
   //
   const std::string* Name() const override { return Last()->Name(); }

   //  Overridden to display the name, including any template arguments.
   //
   void Print
      (std::ostream& stream, const NodeBase::Flags& options) const override;

   //  Overridden to return the qualified name.
   //
   std::string QualifiedName(bool scopes, bool templates) const override;

   //  Overridden to return the referent of the last name.
   //
   CxxNamed* Referent() const override;

   //  Overridden to forward to the Nth name.
   //
   bool ResolveTypedef(Typedef* type, size_t n) const override;

   //  Overridden to instantiate the template unless END is set.
   //
   bool ResolveTemplate
      (Class* cls, const TypeName* args, bool end) const override;

   //  Sets the last name's referent.  This is used by QualName.EnterBlock and
   //  Operation.PushMember when a name appears in executable code.  It is also
   //  used by classes that contain a QualName member and that find a referent.
   //  Those classes do not contain executable code, so they can safely use the
   //  last name's ref_ field because it is normally used only when it appears
   //  in executable code.
   //
   void SetReferent(CxxNamed* item, const SymbolView* view) const override;

   //  Overridden to shrink containers.
   //
   void Shrink() override;

   //  Overridden to reveal that this is a qualified name.
   //
   Cxx::ItemType Type() const override { return Cxx::QualName; }

   //  Overridden to return the referent's full root type.
   //
   std::string TypeString(bool arg) const override;
private:
   //  Deleted to prohibit copy assignment.
   //
   QualName& operator=(const QualName& that) = delete;

   //  Returns the last name.
   //
   TypeName* Last() const;

   //  Resolves the item's qualified name within a function.
   //
   CxxNamed* ResolveLocal(SymbolView* view) const;

   //  The first name in what might be a qualified name.
   //
   TypeNamePtr first_;
};

//------------------------------------------------------------------------------
//
//  Tags that can be applied to a type.
//
class TypeTags
{
public:
   //  Creates a default instance.
   //
   TypeTags();

   //  Creates an instance using SPEC's attributes.
   //
   explicit TypeTags(const TypeSpec& spec);

   //  Copy constructor.
   //
   TypeTags(const TypeTags& that) = default;

   //  Copy operator.
   //
   TypeTags& operator=(const TypeTags& that) = default;

   //  Sets the type's constness to READONLY.
   //
   void SetConst(bool readonly) const { const_ = readonly; }

   //  Returns true if the type is const.
   //
   bool IsConst() const { return const_; }

   //  Sets the Nth pointer (0<=n<=2) as const if READONLY is set.  Also
   //  updates the number of pointers if N is greater than the current
   //  number.  Returns false if N is out of range.
   //
   bool SetPointer(size_t n, bool readonly);

   //  Resets the number of pointers to COUNT.
   //
   void SetPtrs(TagCount count);

   //  Returns the number of pointers attached to this type.  If ARRAYS
   //  is set, each [] also counts as a pointer.
   //
   TagCount PtrCount(bool arrays) const;

   //  Sets the outermost (last) pointer as const.
   //
   void SetConstPtr() const;

   //  Returns 1 if the outermost pointer is const.  Returns 0 if the tag
   //  has no pointers.  Returns -1 if the outermost pointer is not const.
   //
   int IsConstPtr() const;

   //  Returns true if the Nth pointer (0<=n<=2) is const.
   //
   bool IsConstPtr(size_t n) const;

   //  Specifies that the type is followed by an unbounded array tag.
   //
   void SetUnboundedArray() { array_ = true; }

   //  Returns true if the type has an unbounded array tag.
   //
   bool IsUnboundedArray() const { return array_; }

   //  Adds a bounded array to the type.
   //
   void AddArray() { ++arrays_; }

   //  Returns the number of arrays associated with the type.
   //
   TagCount ArrayCount() const;

   //  Sets the level of reference indirection to the type.
   //
   void SetRefs(TagCount refs) { refs_ = refs; }

   //  Returns the level of reference indirection to the type.
   //
   TagCount RefCount() const { return refs_; }

   //  If THAT has as many or more pointers and arrays as these tags do,
   //  modifies THAT by removing the number of pointers and arrays in
   //  these tags and returns true.  Returns false if these tags have
   //  more pointers or arrays than THAT.
   //
   bool AlignTemplateTag(TypeTags& that) const;

   //  Returns Compatible if these tags have the same number of pointers
   //  and arrays as THAT.  Returns Convertible if these tags have less
   //  pointers and less arrays than THAT.  Returns Incompatible if THIS
   //  has more pointers or more arrays than THAT.
   //
   TypeMatch MatchTemplateTags(const TypeTags& that) const;

   //  Displays the tags in STREAM.  Any leading "const" is not displayed
   //  because it is output before the type name, and before this function
   //  is invoked.
   //
   void Print(std::ostream& stream) const;

   //  Adds the tags to NAME.  Prefixes any leading "const".  Omits reference
   //  tags ('&') if ARG is set.
   //
   void TypeString(std::string& name, bool arg) const;

   //  Set if a pointer tag was detached from the type.
   //
   bool ptrDet_: 1;

   //  Set if the reference tag was detached from the type.
   //
   bool refDet_: 1;
private:
   //  Set if the type is const.
   //
   mutable bool const_ : 1;

   //  Set if the type is an unbounded array (e.g. table[]).
   //
   bool array_ : 1;

   //  The number of bounded arrays associated with the type.
   //
   TagCount arrays_ : 8;

   //  The number of pointers associated with the type.  NOTE: This value can
   //  be *negative* for an auto type.  See the comment in TypeTags.TypeString.
   //
   TagCount ptrs_ : 8;

   //  The number of reference tags associated with the type.
   //
   TagCount refs_ : 8;

   //  Set if pointer[n] is const.
   //
   mutable bool constptr_[Cxx::MAX_PTRS];
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
   virtual ~TypeSpec() = default;

   //  Sets the type of item to which the type belongs.
   //
   virtual void SetUserType(Cxx::ItemType user);

   //  Returns the type of item in which the type appears.
   //
   Cxx::ItemType GetUserType() const { return user_; }

   //  Returns the type's role, if any, in a template.
   //
   TemplateRole GetTemplateRole() const { return role_; }

   //  Returns the function signature for a function type.
   //
   virtual Function* GetFuncSpec() const { return nullptr; }

   //  Returns the level of compatibility when assigning THAT to this type.
   //  Generates a log if the types are Incompatible.
   //
   TypeMatch MustMatchWith(const StackArg& that) const;

   //  Creates and returns a copy of the type.  Serves as a copy constructor,
   //  which cannot be invoked directly on this class (because it is virtual).
   //
   virtual TypeSpec* Clone() const = 0;

   //  Provides acccess to the type's tags.
   //
   virtual TypeTags* Tags() = 0;
   virtual const TypeTags* Tags() const = 0;

   //  Adds a bounded array specification to the type.
   //
   virtual void AddArray(ArraySpecPtr& array) = 0;

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

   //  Returns the type's cumulative tags by following it to its root
   //  definition (e.g. through typedefs).
   //
   virtual TypeTags GetAllTags() const = 0;

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

   //  Resets the number of pointer tags when the type is assigned to an
   //  auto type.  For example, in auto a = *p; P's type is assigned to
   //  A, but with a pointer count of -1.
   //
   virtual void SetPtrs(TagCount count) = 0;

   //  Eliminates references.  Used when the right-hand side of an auto
   //  type is a reference, but "auto" is used instead of "auto&".
   //
   virtual void RemoveRefs() = 0;

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

   //  Returns true if ITEM is the referent of a template argument.
   //
   virtual bool ItemIsTemplateArg(const CxxNamed* item) const = 0;

   //  Invoked when the type is a template argument that is about to be used
   //  to instantiate a template.  Finds the type's referent and, if it is a
   //  template, also instantiates it.
   //
   virtual void Instantiating() const = 0;

   //  Overridden to reveal that this is a type specification.
   //
   Cxx::ItemType Type() const override { return Cxx::TypeSpec; }
protected:
   //  Protected because this class is virtual.
   //
   TypeSpec();

   //  Copy constructor.
   //
   TypeSpec(const TypeSpec& that);
private:
   //  Deleted to prohibit copy assignment.
   //
   TypeSpec& operator=(const TypeSpec& that) = delete;

   //  The item type to which the type belongs.  The default is Cxx::Operation.
   //
   Cxx::ItemType user_ : 8;

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

   //  A DataSpec for a bool.
   //
   static const TypeSpecPtr Bool;

   //  A DataSpec for an int.
   //
   static const TypeSpecPtr Int;
private:
   //  Deleted to prohibit copy assignment.
   //
   DataSpec& operator=(const DataSpec& that) = delete;

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
   bool ResolveTemplateArg() const;

   //  Overridden to add a bounded array specification to the type.
   //
   void AddArray(ArraySpecPtr& array) override;

   //  Overridden to align thatArg's type with the this type, which is that of a
   //  template parameter that might be specialized.
   //
   std::string AlignTemplateArg(const TypeSpec* thatArg) const override;

   //  Overridden to return the number of arrays attached to the type, following
   //  the type to its root.
   //
   TagCount Arrays() const override;

   //  Overridden to set the type to assign to an "auto" variable.
   //
   CxxToken* AutoType() const override { return (CxxToken*) this; }

   //  Overridden to verify that pointer and reference tags are attached to
   //  types, not names.
   //
   void Check() const override;

   //  Overridden to create and return a copy of the type.
   //
   TypeSpec* Clone() const override;

   //  Overridden to propagate the context to the type's qualified name.
   //
   void CopyContext(const CxxNamed* that) override;

   //  Overridden to return the class, if any, to which the type ultimately
   //  refers, provided that it is not a pointer or reference to that class.
   //
   Class* DirectClass() const override;

   //  Overridden to invoke DirectType on name_.
   //
   CxxNamed* DirectType() const override;

   //  Overridden to display the type's bounded array specifications.
   //
   void DisplayArrays(std::ostream& stream) const override;

   //  Overridden to display the type's pointer, array, and reference tags.
   //
   void DisplayTags(std::ostream& stream) const override;

   //  Overridden to invoke EnterBlock on any bounded array specifications.
   //
   void EnterArrays() const override;

   //  Overridden to push the type's referent onto the argument stack.
   //
   void EnterBlock() override;

   //  Overridden to see if the type is a template parameter in SCOPE and
   //  to invoke Check and EnterArrays.
   //
   void EnteringScope(const CxxScope* scope) override;

   //  Overridden to find the type's referent, as well as the referent for
   //  each template argument used in the type.
   //
   void FindReferent() override;

   //  Overridden to return the numeric type.
   //
   Numeric GetNumeric() const override;

   //  Overridden to return the type's qualified name.
   //
   QualName* GetQualName() const override { return name_.get(); }

   //  Overridden to return the type's attributes.
   //
   TypeTags GetAllTags() const override;

   //  Overridden to return the type itself.
   //
   TypeSpec* GetTypeSpec() const override;

   //  Overridden to update SYMBOLS with the specification's type usage.
   //
   void GetUsages(const CodeFile& file, CxxUsageSets& symbols) const override;

   //  Overridden to return true if the type has a bounded array specification.
   //
   bool HasArrayDefn() const override { return (arrays_ == nullptr); }

   //  Overridden to instantiate the type's referent if it is a template.
   //
   void Instantiating() const override;

   //  Overridden to return true if the type is "auto" and the referent has
   //  yet to be determined.
   //
   bool IsAuto() const override;

   //  Overridden to return true if ITEM is the referent of a template argument.
   //
   bool ItemIsTemplateArg(const CxxNamed* item) const override;

   //  Overridden to return true if the type is const.
   //
   bool IsConst() const override;

   //  Overridden to return true if the type's outermost pointer is const.
   //
   bool IsConstPtr() const override;

   //  Overridden to return true if the type's Nth pointer is const.
   //
   bool IsConstPtr(size_t n) const override;

   //  Overridden to return true if the type has pointer or reference tags.
   //
   bool IsIndirect() const override;

   //  Overridden to return true if the types of "this" and THAT match.
   //
   bool MatchesExactly(const TypeSpec* that) const override;

   //  Overridden to determine if THAT can instantiate a template for this type.
   //
   TypeMatch MatchTemplate(TypeSpec* that, stringVector& tmpltParms,
      stringVector& tmpltArgs, bool& argFound) const override;

   //  Overridden to determine how well THAT matches this type for the purpose
   //  of template instantiation.
   //
   TypeMatch MatchTemplateArg(const TypeSpec* that) const override;

   //  Overridden to return the type's name.
   //
   const std::string* Name() const override { return name_->Name(); }

   //  Overridden to display the type.
   //
   void Print
      (std::ostream& stream, const NodeBase::Flags& options) const override;

   //  Overridden to return the number of pointer tags attached to the type,
   //  following the type to its root.
   //
   TagCount Ptrs(bool arrays) const override;

   //  Overridden to return the type's qualified name.
   //
   std::string QualifiedName(bool scopes, bool templates) const
      override { return name_->QualifiedName(scopes, templates); }

   //  Overridden to return what the type refers to.
   //
   CxxNamed* Referent() const override;

   //  Overridden to return the number of reference tags attached to the type,
   //  following the type to its root.
   //
   TagCount Refs() const override;

   //  Overridden to eliminate reference tags from an auto type.
   //
   void RemoveRefs() override;

   //  Overridden to resolve the forward declaration only if it has template
   //  arguments.
   //
   bool ResolveForward(CxxScoped* decl, size_t n) const override;

   //  Overridden to determine if EnsureInstance should be invoked.
   //
   bool ResolveTemplate
      (Class* cls, const TypeName* args, bool end) const override;

   //  Overridden to resolve the typedef unless it has template arguments.
   //
   bool ResolveTypedef(Typedef* type, size_t n) const override;

   //  Overridden to construct an argument based on the type of referent and
   //  the level of pointer indirection to it.
   //
   StackArg ResultType() const override;

   //  Overridden to return the underlying type.
   //
   CxxToken* RootType() const override { return Referent(); }

   //  Overridden to reset the number of pointer tags when the type is assigned
   //  to an auto type.
   //
   void SetPtrs(TagCount count) override;

   //  Overridden to record what the item refers to.
   //
   void SetReferent(CxxNamed* item, const SymbolView* view) const override;

   //  Overridden so that when ROLE is TemplateClass, its arguments are treated
   //  as parameters.
   //
   void SetTemplateRole(TemplateRole role) const override;

   //  Overridden to propagate USER to name_.
   //
   void SetUserType(Cxx::ItemType user) override;

   //  Overridden to shrink containers.
   //
   void Shrink() override;

   //  Overridden to provide access to the type's tags.
   //
   TypeTags* Tags() override { return &tags_; }
   const TypeTags* Tags() const override { return &tags_; }

   //  Overridden to return the base class default (the scoped name) unless the
   //  type is "auto", in which case the referent (if known) is returned.
   //
   std::string Trace() const override;

   //  Overridden to return the full root type.  Reference tags ('&') are
   //  omitted if ARG is set.
   //
   std::string TypeString(bool arg) const override;

   //  Overridden to apply TAGS instead of the type's actual attributes.
   //
   std::string TypeTagsString(const TypeTags& tags) const override;

   //  Overridden to support a temporary variable represented by a DataSpec.
   //
   bool WasWritten(const StackArg* arg, bool passed)
      override { return false; }

   //  The qualified name for the type as it appeared in the source code.
   //
   QualNamePtr name_;

   //  The type's bounded array specifications (e.g. for int[10][10]).
   //
   std::unique_ptr< ArraySpecPtrVector > arrays_;

   //  The type's tags.
   //
   TypeTags tags_;
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
   //  for template <class T*...), which may specify an optional default.
   //
   TemplateParm(std::string& name,
      Cxx::ClassTag tag, size_t ptrs, QualNamePtr& preset);

   //  Not subclassed.
   //
   ~TemplateParm() { CxxStats::Decr(CxxStats::TEMPLATE_PARM); }

   //  Returns the parameter's default type.
   //
   const QualName* Default() const { return default_.get(); }

   //  Overridden to check the default type.
   //
   void Check() const override;

   //  Overridden to return the parameter's name.
   //
   const std::string* Name() const override { return &name_; }

   //  Overridden to display the parameter.
   //
   void Print
      (std::ostream& stream, const NodeBase::Flags& options) const override;

   //  Overridden to shrink the item's name.
   //
   void Shrink() override;

   //  Overridden to return the parameter's name and pointers.
   //
   std::string TypeString(bool arg) const override;
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

   //  The parameter's default value, if any.
   //
   QualNamePtr default_;
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

   //  Adds another parameter to the template.
   //
   void AddParm(TemplateParmPtr& parm);

   //  Returns the template's parameters.
   //
   const TemplateParmPtrVector* Parms() const { return &parms_; }

   //  Overridden to check each parameter.
   //
   void Check() const override;

   //  Overridden to display the template's full specification.
   //
   void Print
      (std::ostream& stream, const NodeBase::Flags& options) const override;

   //  Overridden to shrink containers.
   //
   void Shrink() override;

   //  Overridden to return the template's parameter names in angle brackets.
   //
   std::string TypeString(bool arg) const override;
private:
   //  The template's parameters.
   //
   TemplateParmPtrVector parms_;
};
}
#endif
