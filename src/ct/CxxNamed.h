//==============================================================================
//
//  CxxNamed.h
//
//  Copyright (C) 2013-2025  Greg Utas
//
//  This file is part of the Robust Services Core (RSC).
//
//  RSC is free software: you can redistribute it and/or modify it under the
//  terms of the Lesser GNU General Public License as published by the Free
//  Software Foundation, either version 3 of the License, or (at your option)
//  any later version.
//
//  RSC is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
//  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
//  details.
//
//  You should have received a copy of the Lesser GNU General Public License
//  along with RSC.  If not, see <http://www.gnu.org/licenses/>.
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
#include "CxxExecute.h"
#include "CxxFwd.h"
#include "CxxString.h"
#include "LibraryTypes.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace CodeTools
{
//  The base class for C++ entities that have a name.
//
class CxxNamed : public CxxToken
{
public:
   //  Virtual to allow subclassing.
   //
   virtual ~CxxNamed();

   //  Deleted to prohibit copy assignment.
   //
   CxxNamed& operator=(const CxxNamed& that) = delete;

   //  Returns true if the item was declared in a function's code block
   //  or argument list.
   //
   virtual bool IsDeclaredInFunction() const { return false; }

   //  Sets the template parameters when the item declares a template.
   //  The default version generates a log and must be overridden by an
   //  item that can declare a template.
   //
   virtual void SetTemplateParms(TemplateParmsPtr& parms);

   //  Returns the template parameters associated with the item, if any.
   //  Must be overridden by an item that can declare a template.
   //
   virtual TemplateParms* GetTemplateParms() const { return nullptr; }

   //  Returns true if the item declares template parameters.
   //
   bool IsTemplate() const { return GetTemplateParms() != nullptr; }

   //  Returns the qualified name as it appeared in the source code.  Includes
   //  prefixed scopes if SCOPES is set and template arguments if TEMPLATES is
   //  is set.  Consequently, QualifiedName(false, false) == Name().
   //
   virtual std::string QualifiedName(bool scopes, bool templates)
      const { return Name(); }

   //  Returns the item's fully qualified name.  Template arguments are omitted
   //  unless TEMPLATES is set.
   //
   virtual std::string ScopedName(bool templates) const;

   //  Updates NAMES with the item's fully qualified name(s), including
   //  template arguments if TEMPLATE is set.  Each name is prefixed by a
   //  scope resolution operator.
   //
   virtual void GetScopedNames(stringVector& names, bool templates) const;

   //  Returns the function associated with the item.  The interpretation of
   //  this varies by item type.
   //
   virtual Function* GetFunction() const { return nullptr; }

   //  Returns the file that *declared* the item.  Declaration is distinct
   //  from definition for extern data and functions, and often for static
   //  class data.  Such items appear twice, with one being the declaration
   //  and the other the definition.
   //
   virtual CodeFile* GetDeclFile() const { return GetFile(); }

   //  Returns the file that *defined* the item.  Returns nullptr if the
   //  item has no definition or if it was defined where it was declared.
   //
   virtual CodeFile* GetDefnFile() const { return nullptr; }

   //  Returns the file where the item is implemented: GetDefnFile unless it
   //  is nullptr, else GetDeclFile.
   //
   CodeFile* GetImplFile() const;

   //  Returns the file that declares the item if it is *not* the file that
   //  defines the item.  This can only occur for static class data, extern
   //  data, or a function.  Returns nullptr if the same file declares and
   //  defines the item.
   //
   CodeFile* GetDistinctDeclFile() const;

   //  Returns true if this item was a previous declaration of ITEM.  This is
   //  used after parsing data and function definitions.
   //
   bool IsPreviousDeclOf(const CxxNamed* item) const;

   //  Returns the item's direct type, which could be a typedef or forward
   //  declaration.  To follow either of these to the final underlying type,
   //  use CxxToken::Root.
   //
   virtual CxxScoped* DirectType() const { return Referent(); }

   //  Finds what the item refers to.  The default version generates a log.
   //
   virtual void FindReferent();

   //  Sets the name's referent to ITEM.  VIEW (if not nullptr) provides
   //  information about how the name was resolved.  The default version
   //  generates a log.
   //
   virtual void SetReferent(CxxScoped* item, const SymbolView* view) const;

   //  Invoked when the item is found to be the referent of USER.
   //
   virtual void SetAsReferent(const CxxNamed* user) { }

   //  Constructs an argument for the item when it is named directly, perhaps
   //  through an implicit "this".  OP is the operator that is on top of the
   //  operator stack (Cxx::NIL_OPERATOR if the stack is empty).  The result
   //  is pushed onto the argument stack.  NAME was used to access the item.
   //
   virtual StackArg NameToArg(Cxx::Operator op, TypeName* name);

   //  Constructs an argument for the item when it was accessed through VIA,
   //  NAME, and OP (either "." or "->" in VIA OP NAME).
   //
   virtual StackArg MemberToArg
      (StackArg& via, TypeName* name, Cxx::Operator op);

   //  Returns true if NAMES, used in SCOPE and FILE, could refer to a type
   //  and its template arguments.  INDEX is the current index into NAMES.
   //  The default version generates a log and returns false.
   //
   virtual bool NamesReferToArgs(const NameVector& names,
      const CxxScope* scope, CodeFile* file, size_t& index) const;

   //  In a default template argument, uses TMAP to replace a template
   //  parameter with its corresponding template argument.  The default
   //  version returns ERROR_STR.
   //
   virtual std::string ArgString(const TemplateParmToArgMap& tmap) const;

   //  Invoked when a type is a template argument that is about to be used
   //  to instantiate a template.  Finds the type's referent and, if it is a
   //  template, also instantiates it.  The default version generates a log.
   //
   virtual void Instantiating(CxxScopedVector& locals) const;

   //  Invoked on an item that was used directly (e.g. to invoke a function).
   //  If the item's type is a forward declaration, its actual class is added
   //  to SYMBOLS because that class must be #included by the file that used
   //  this item.  For example, say that a class declares a pointer member
   //  using a forward declaration.  If a client of that class invokes a
   //  function through that pointer (a direct usage), it must #include the
   //  definition of the pointer's class (or at least be certain that it will
   //  be #included transitively).  The default version invokes the function
   //  on GetTypeSpec().
   //
   virtual void GetDirectClasses(CxxUsageSets& symbols);

   //  Invoked to add template arguments that are direct (that is, that don't
   //  have a pointer) to SYMBOLS.  The definition of such arguments must be
   //  available when instantiation occurs.  The default version invokes the
   //  function on GetTypeSpec().
   //
   virtual void GetDirectTemplateArgs(CxxUsageSets& symbols) const;

   //  Returns true if ITEM is the referent of a template argument.  The
   //  default version generates a log and returns false.
   //
   virtual bool ItemIsTemplateArg(const CxxNamed* item) const;

   //  Returns the item's name as it should appear in the cross-reference.
   //  The default returns ScopedName(templates).
   //
   virtual std::string XrefName(bool templates) const;

   //  Displays the item's referent in STREAM.  If FQ is set, the item's
   //  fully qualified name is displayed.
   //
   void DisplayReferent(std::ostream& stream, bool fq) const;

   //  Returns a string that identifies the item and its type and location.
   //  Intended primarily for CLI commands.
   //
   std::string to_str() const;

   //  Displays NAME in STREAM.  If FQ is set, the fully qualified name is
   //  shown, else the qualified name (including any templates) is shown.
   //
   void strName(std::ostream& stream, bool fq, const QualName* name) const;

   //  Overridden to invoke GetArea on the item's scope (GetScope).
   //
   CxxArea* GetArea() const override;

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

   //  Resolves the item's qualified name.  FILE, SCOPE, MASK, and VIEW are
   //  the same as the arguments for CxxSymbols::FindSymbol.
   //
   CxxScoped* ResolveName(CodeFile* file, const CxxScope* scope,
      const NodeBase::Flags& mask, SymbolView& view) const;

   //  Invoked when ResolveName finds TYPE, a typedef.  If it returns false,
   //  ResolveName returns TYPE.  Otherwise, name resolution continues with
   //  TYPE's referent (its underlying type).  In a qualified name, N is the
   //  index of the name associated with the typedef.
   //
   virtual bool ResolveTypedef(Typedef* type, size_t n) const { return true; }

   //  Invoked when ResolveName finds TYPE, which has template arguments for
   //  CLS.  END is set if the arguments are attached to the last name in the
   //  qualified name.  To invoke EnsureInstance with the arguments, return
   //  true.  To skip the arguments, return false.
   //
   virtual bool ResolveTemplate
      (Class* cls, const TypeName* type, bool end) const { return true; }

   //  Changes oldName to newName.  Invoked by overrides of Rename.
   //
   virtual void RenameNonQual
      (std::string& oldName, const std::string& newName) const;

   //  Invoked when the item is accessed.  Invokes ItemAccessed on the context
   //  function.  If the item was accessed through the reference select (.) or
   //  pointer select (->) operator, VIA is the item that preceded the operator.
   //
   void Accessed(const StackArg* via) const;

   //  Invoked by overrides of RecordUsage to record that the file being
   //  compiled used this item.
   //
   void AddUsage();
private:
   //  Invoked when ResolveName finds DECL, a forward or friend declaration,
   //  when resolving the Nth name in a possibly qualified name.  If it
   //  returns false, ResolveName returns DECL.  Otherwise, name resolution
   //  continues with DECL's referent (the class or function to which it
   //  refers).  If this is still unknown, DECL is returned.
   //
   virtual bool ResolveForward
      (CxxScoped* decl, size_t n) const { return false; }
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

   //  Sets the qualified name in which the name appears.
   //
   void SetQualName(QualName* qname) { qname_ = qname; }

   //  In a qualified name, adds TYPE as the name that follows this one.
   //  A scope resolution operator separates the two names.
   //
   void PushBack(TypeNamePtr& type);

   //  Returns the next name in a qualified name.
   //
   TypeName* Next() const { return next_.get(); }

   //  Specifies whether a scope resolution operator precedes the name.
   //
   void SetScoped(bool scoped) { scoped_ = scoped; }

   //  Returns true if the name was preceded by a scope resolution operator.
   //
   bool IsScoped() const { return scoped_; }

   //  Adds a template argument (type specialization) to this name.
   //
   void AddTemplateArg(TemplateArgPtr& arg);

   //  Creates template arguments from template parameters.
   //
   void SetTemplateArgs(const TemplateParms* tparms);

   //  Adds the string corresponding to OPER to the name.
   //
   void SetOperator(Cxx::Operator oper);

   //  Returns the operator, if any, that follows the name.
   //
   Cxx::Operator Operator() const { return oper_; }

   //  Returns the minimum access control required to access the referent.
   //
   Cxx::Access MinControl() const { return control_; }

   //  Adds the suffix NAME to the name.  Adds a space first if SPACE is set.
   //
   void Append(const std::string& name, bool space);

   //  Invokes SetUserType on each template argument.
   //
   void SetUserType(TypeSpecUser user) const;

   //  Sets the role for each template argument.
   //
   void SetTemplateRole(TemplateRole role) const;

   //  If the type has template arguments, returns true if any of them are
   //  actually template parameters in SCOPE.
   //
   bool HasTemplateParmFor(const CxxScope* scope) const;

   //  Determines if THAT can instantiate a template for args_.
   //  o Returns Compatible if args_ is empty.
   //  o Returns Incompatible if THAT does not have the same number of
   //    template arguments as args_.
   //  o Invokes MatchTemplate on each entry in args_, returning the
   //    least favorable result.
   //
   TypeMatch MatchTemplate(const TypeName* that,
      stringVector& tmpltParms, stringVector& tmpltArgs, bool& argFound) const;

   //  Invoked when the name accesses MEM via CLS.  Sets the referent to MEM
   //  and records CLS as the type through which it was accessed.
   //
   void MemberAccessed(Class* cls, CxxScoped* mem) const;

   //  Invoked when the name was used directly.
   //
   void SetAsDirect() { direct_ = true; }

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

   //  Adds any template arguments to NAMES.
   //
   void GetNames(stringVector& names) const;

   //  Erases the name that follows this one, replacing it with NEXT.
   //
   void SetNext(TypeNamePtr& next);

   //  Overridden to return the name's template arguments.
   //
   const TemplateArgPtrVector* Args() const override;

   //  Overridden to forward to any template arguments.
   //
   std::string ArgString(const TemplateParmToArgMap& tmap) const override;

   //  Overridden to check template arguments.
   //
   void Check() const override;

   //  Overridden to support the deletion of a name.
   //
   void Delete() override;

   //  Overridden to return type_ if it exists, else ref_.
   //
   CxxScoped* DirectType() const override;

   //  Overridden to invoke FindReferent on each template argument.
   //
   void FindReferent() override;

   //  Overridden to invoke GetDirectClasses on DirectType() and on
   //  each template argument.
   //
   void GetDirectClasses(CxxUsageSets& symbols) override;

   //  Overridden to invoke GetDirectTemplateArgs on each template
   //  argument.
   //
   void GetDirectTemplateArgs(CxxUsageSets& symbols) const override;

   //  Overridden to return the type's qualified name.
   //
   QualName* GetQualName() const override { return qname_; }

   //  Overridden to return this item or type_ if it has template arguments.
   //
   TypeName* GetTemplatedName() const override;

   //  Overridden to update SYMBOLS with the name's type usage.
   //
   void GetUsages(const CodeFile& file, CxxUsageSets& symbols) override;

   //  Overridden to forward to each template argument.
   //
   void Instantiating(CxxScopedVector& locals) const override;

   //  Overridden to clear ref_ when it is deleted.
   //
   void ItemDeleted(const CxxScoped* item) const override;

   //  Overridden to return true if ITEM is the referent of a template argument.
   //
   bool ItemIsTemplateArg(const CxxNamed* item) const override;

   //  Overridden to return the name.
   //
   const std::string& Name() const override { return name_; }

   //  Overridden to use args_ if they exist, else to forward to type_ if
   //  it exists.
   //
   bool NamesReferToArgs(const NameVector& names, const CxxScope* scope,
      CodeFile* file, size_t& index) const override;

   //  Overridden to find the item located at POS.
   //
   CxxToken* PosToItem(size_t pos) const override;

   //  Overridden to display the name.
   //
   void Print
      (std::ostream& stream, const NodeBase::Flags& options) const override;

   //  Overridden to return the name and, optionally, its template arguments.
   //
   std::string QualifiedName(bool scopes, bool templates) const override;

   //  Overridden to return what the name refers to.
   //
   CxxScoped* Referent() const override { return ref_; }

   //  Overridden to support renaming the referent.
   //
   void Rename(const std::string& name) override;

   //  Overridden to record and resolve the typedef.
   //
   bool ResolveTypedef(Typedef* type, size_t n) const override;

   //  Overridden to record what the item refers to.
   //
   void SetReferent(CxxScoped* item, const SymbolView* view) const override;

   //  Overridden to reveal that this is part of a qualified name.
   //
   Cxx::ItemType Type() const override { return Cxx::TypeName; }

   //  Overridden to return the full type for template arguments.  Note that
   //  the name itself is omitted.
   //
   std::string TypeString(bool arg) const override;

   //  Overridden to update the name's location.
   //
   void UpdatePos(EditorAction action,
      size_t begin, size_t count, size_t from) const override;

   //  Overridden to invoke UpdateReference on the name's referent.
   //
   void UpdateXref(bool insert) override;

   //  Overridden to check the name and any template arguments.
   //
   bool VerifyReferents() const override;
private:
   //  Overridden so that a data item can be erased.
   //
   bool GetSpan(size_t& begin, size_t& left, size_t& end) const override;

   //  The name that appears in what could be a qualified name.
   //
   std::string name_;

   //  A pointer to the qualified name, if any, in which the name appears.
   //
   QualName* qname_;

   //  Any template arguments if the name is that of a template.
   //
   std::unique_ptr<TemplateArgPtrVector> args_;

   //  The next name in a qualified name.
   //
   TypeNamePtr next_;

   //  What the name refers to.
   //
   mutable CxxScoped* ref_;

   //  The class, if any, through which the name was accessed.
   //
   mutable Class* class_;

   //  The typedef, if any, resolved via ResolveTypedef.
   //
   mutable CxxScoped* type_;

   //  The forward declaration, if any, resolved via ResolveForward.
   //
   mutable CxxScoped* forw_;

   //  The operator, if any, that follows the name.
   //
   Cxx::Operator oper_ : 8;

   //  The minimum access control that was required to access the referent.
   //
   mutable Cxx::Access control_ : 4;

   //  Set if a scope resolution operator precedes the name.
   //  Initialized to false; must be set by SetScoped.
   //
   bool scoped_;

   //  Set if ref_ was made visible by a using statement.
   //
   mutable bool using_;

   //  Set if the name was used directly (e.g. to access a member, to
   //  invoke a function, or as the target of an assignment).
   //
   bool direct_;
};

//------------------------------------------------------------------------------
//
//  Used when an item can have a qualified name (that is, when its name
//  in the source code can contain a scope resolution operator).
//
class QualName : public CxxNamed
{
public:
   //  Creates a name that begins with NAME.
   //
   explicit QualName(TypeNamePtr& name);

   //  Creates a name that begins with NAME.
   //
   explicit QualName(const std::string& name);

   //  Not subclassed.
   //
   ~QualName();

   //  Copy constructor.
   //
   QualName(const QualName& that);

   //  Adds NAME to the name.  In a qualified name, NAME is preceded by a
   //  scope resolution operator.
   //
   void PushBack(TypeNamePtr& name);

   //  Records any elaborated type specifier that preceded the name.
   //
   void SetSpecifier(Cxx::Specifier specifier) { specifier_ = specifier; }

   //  Deletes NAME from the qualified name.  NEXT is the name after NAME.
   //
   void DeleteName(const TypeName* name, TypeNamePtr& next);

   //  Marks a data item's name used to initialize it at file scope.
   //
   void SetDataInit() { init_ = true; }

   //  Returns the first name.
   //
   TypeName* First() const { return first_.get(); }

   //  Returns the last name.
   //
   TypeName* Last() const;

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

   //  Invokes SetTemplateArgs on the last name.
   //
   void SetTemplateArgs(const TemplateParms* tparms) const;

   //  Sets the referent of the Nth name to ITEM.  VIEW provides information
   //  about how the name was resolved.  If name resolution failed, ITEM will
   //  be nullptr.  If whoever requested name resolution did not provide a
   //  SymbolView, VIEW will be nullptr.
   //
   void SetReferentN(size_t n, CxxScoped* item, const SymbolView* view) const;

   //  Returns the scopes, with any template arguments, that qualify the name.
   //  Returns an empty string if the name is unqualified.
   //
   std::string QualifyingScope() const;

   //  Returns the last name's referent.  This is used in conjunction with
   //  SetReferent.  A class that contains a QualName instance cannot use the
   //  Referent function (overridden below) to access the referent because,
   //  if it is nullptr, the QualName will try to find it, starting with local
   //  variables.
   //
   CxxScoped* GetReferent() const { return Last()->Referent(); }

   //  Returns the last name that was resolved by a forward declaration.
   //
   CxxScoped* GetForward() const;

   //  Invokes MatchTemplate on each name, returning the least favorable result.
   //
   TypeMatch MatchTemplate(const QualName* that,
      stringVector& tmpltParms, stringVector& tmpltArgs, bool& argFound) const;

   //  Adds the name and any template arguments to NAMES.
   //
   void GetNames(stringVector& names) const;

   //  Checks that the name is a valid constructor name ("...A::A").
   //
   bool CheckCtorDefn() const;

   //  Adds NAME, which refers to NS, as the scope for the name.
   //
   void AddPrefix(const std::string& name, Namespace* ns);

   //  Checks for a redundant scope name.
   //
   void CheckForRedundantScope() const;

   //  Overridden to return any template arguments associated with one of the
   //  names.
   //
   const TemplateArgPtrVector* Args() const override;

   //  Overridden to use TMAP to replace template parameters with template
   //  arguments.
   //
   std::string ArgString(const TemplateParmToArgMap& tmap) const override;

   //  Overridden to check each name and any template parameters.
   //
   void Check() const override;

   //  Overridden to propagate the context to each name.
   //
   void CopyContext(const CxxToken* that, bool internal) override;

   //  Overridden to forward to the last name.
   //
   CxxScoped* DirectType() const override { return Last()->DirectType(); }

   //  Overridden to find the referent and push it onto the argument stack.
   //
   void EnterBlock() override;

   //  Overridden to invoke GetDirectClasses on the last name.
   //
   void GetDirectClasses(CxxUsageSets& symbols) override;

   //  Overridden to invoke GetDirectTemplateArgs on each name.
   //
   void GetDirectTemplateArgs(CxxUsageSets& symbols) const override;

   //  Overridden to return the item itself.
   //
   QualName* GetQualName() const override
      { return const_cast<QualName*>(this); }

   //  Overridden to return the parser's enclosing scope when initializing data
   //  at file scope.  This prevents logging the RedundantScope warning on a
   //  qualified name.  The problem is that Context::Scope() is the data item
   //  itself when compiling its initialization statement, but the scope should
   //  be the namespace in which the initialization appears until compilation of
   //  the assignment operator's rvalue.
   //
   CxxScope* GetScope() const override;

   //  Overridden to return one of the names if it has template arguments.
   //
   TypeName* GetTemplatedName() const override;

   //  Overridden to update SYMBOLS with the name's type usage.
   //
   void GetUsages(const CodeFile& file, CxxUsageSets& symbols) override;

   //  Overridden to forward to each name.
   //
   void Instantiating(CxxScopedVector& locals) const override;

   //  Overridden to return true if ITEM is the referent of a template argument.
   //
   bool ItemIsTemplateArg(const CxxNamed* item) const override;

   //  Overridden to return the last name.
   //
   const std::string& Name() const override { return Last()->Name(); }

   //  Overridden to find the item located at POS.
   //
   CxxToken* PosToItem(size_t pos) const override;

   //  Overridden to display the name, including any template arguments.
   //
   void Print
      (std::ostream& stream, const NodeBase::Flags& options) const override;

   //  Overridden to return the qualified name.
   //
   std::string QualifiedName(bool scopes, bool templates) const override;

   //  Overridden to return the referent of the last name.
   //
   CxxScoped* Referent() const override;

   //  Overridden to support changing the last name.
   //
   void Rename(const std::string& name) override;

   //  Overridden to instantiate the template unless END is set.
   //
   bool ResolveTemplate
      (Class* cls, const TypeName* type, bool end) const override;

   //  Overridden to forward to the Nth name.
   //
   bool ResolveTypedef(Typedef* type, size_t n) const override;

   //  Overridden to also invoke SetContext on the first name.
   //
   void SetContext(size_t pos) override;

   //  Sets the last name's referent.  This is used by QualName::EnterBlock and
   //  Operation::PushMember when a name appears in executable code.  It is also
   //  used by classes that contain a QualName member and that find a referent.
   //  Those classes do not contain executable code, so they can safely use the
   //  last name's ref_ field because it is normally used only when it appears
   //  in executable code.
   //
   void SetReferent(CxxScoped* item, const SymbolView* view) const override;

   //  Overridden to reveal that this is a qualified name.
   //
   Cxx::ItemType Type() const override { return Cxx::QualName; }

   //  Overridden to return the referent's full root type.
   //
   std::string TypeString(bool arg) const override;

   //  Overridden to update the name's location.
   //
   void UpdatePos(EditorAction action,
      size_t begin, size_t count, size_t from) const override;

   //  Overridden to add the name's components to cross-references.
   //
   void UpdateXref(bool insert) override;

   //  Overridden to check each name.
   //
   bool VerifyReferents() const override;
private:
   //  Overridden so that a data item can be erased.
   //
   bool GetSpan(size_t& begin, size_t& left, size_t& end) const override;

   //  The first name in what might be a qualified name.
   //
   TypeNamePtr first_;

   //  Any elaborated type specifier that preceded the name.
   //
   Cxx::Specifier specifier_;

   //  Set if the name is used when initializing data at file scope.
   //
   bool init_;
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

   //  Not subclassed.
   //
   ~TypeTags() = default;

   //  Copy constructor.
   //
   TypeTags(const TypeTags& that) = default;

   //  Copy operator.
   //
   TypeTags& operator=(const TypeTags& that) = default;

   //  Sets the type's constness to READONLY.
   //
   void SetConst(bool readonly) const { const_ = readonly; }

   //  Sets the type's volatility to UNSTABLE.
   //
   void SetVolatile(bool unstable) const { volatile_ = unstable; }

   //  Returns true if the type is const.
   //
   bool IsConst() const { return const_; }

   //  Returns true if the type is volatile.
   //
   bool IsVolatile() const { return volatile_; }

   //  Returns true if the tags are compatible with an lvalue.
   //
   bool IsLvalue() const;

   //  Sets the Nth pointer (0<=n<=2) as const if READONLY is set, and as
   //  volatile if UNSTABLE is set.  Sets the number of pointers to N+1 if N
   //  is greater than the current number.  Returns false if N is out of range.
   //
   bool SetPointer(size_t n, bool readonly, bool unstable);

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

   //  Returns true if the Nth pointer (0<=n<=MAX_PTRS) is const.
   //
   bool IsConstPtr(size_t n) const;

   //  Sets the outermost (last) pointer to volatile.
   //
   void SetVolatilePtr() const;

   //  Returns 1 if the outermost pointer is volatile.  Returns 0 if the tag
   //  has no pointers.  Returns -1 if the outermost pointer is not volatile.
   //
   int IsVolatilePtr() const;

   //  Returns true if the Nth pointer (0<=n<=MAX_PTRS) is volatile.
   //
   bool IsVolatilePtr(size_t n) const;

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

   //  Modifies the tags by removing the number of pointers and arrays in
   //  THAT.  Returns true on success, and false if these tags have fewer
   //  pointers or arrays than THAT.
   //
   bool AlignTemplateTags(const TypeTags& that);

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

   //  Set if the type is volatile.
   //
   mutable bool volatile_ : 1;

   //  Set if the type is an unbounded array (e.g. table[]).
   //
   bool array_ : 1;

   //  The number of bounded arrays associated with the type.
   //
   TagCount arrays_ : 8;

   //  The number of pointers associated with the type.  This value can be
   //  *negative* for an auto type.  See the comment in TypeTags::TypeString.
   //
   TagCount ptrs_ : 8;

   //  The number of reference tags associated with the type.
   //
   TagCount refs_ : 8;

   //  Records whether a pointer is const.
   //
   mutable uint8_t constPtr_ : 8;

   //  Records whether a pointer is volatile.
   //
   mutable uint8_t volatilePtr_ : 8;
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

   //  Returns the type of item in which the type appears.
   //
   TypeSpecUser GetUserType() const { return user_; }

   //  Returns the type's role, if any, in a template.
   //
   TemplateRole GetTemplateRole() const { return role_; }

   //  Returns the function signature for a function type.
   //
   virtual Function* GetFuncSpec() const { return nullptr; }

   //  Returns the level of compatibility when assigning THAT to this type.
   //  Generates an error log if the types are Incompatible.
   //
   TypeMatch MustMatchWith(const StackArg& that) const;

   //  Provides access to the type's tags.
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

   //  Returns true if the types of "this" and THAT match exactly, including
   //  all tags (constness, pointers, arrays, and references).
   //
   virtual bool MatchesExactly(const TypeSpec* that) const = 0;

   //  Sets the type's role in a template.
   //
   virtual void SetTemplateRole(TemplateRole role) const { role_ = role; }

   //  Sets the type of item to which the type belongs.
   //
   virtual void SetUserType(TypeSpecUser user) const { user_ = user; }

   //  Determines if THAT can instantiate a template for this type.
   //  o If this type is a template parameter in tmpltParms, argFound is set,
   //    and THAT is added to tmpltArgs as the corresponding template argument
   //    unless a different argument was found previously.
   //  o If this is not a template parameter, THAT must have the same type.
   //  Returns Incompatible if the above conditions are not satisfied.  Returns
   //  Compatible on an exact match, or something else on a partial match (if,
   //  for example, THAT is a pointer type and the template parameter is not).
   //
   virtual TypeMatch MatchTemplate(const TypeSpec* that,
      stringVector& tmpltParms, stringVector& tmpltArgs,
      bool& argFound) const = 0;

   //  Determines how well THAT matches this type for the purpose of template
   //  instantiation.
   //
   virtual TypeMatch MatchTemplateArg(const TypeSpec* that) const = 0;

   //  During template instantiation, aligns thatArg's type with this type,
   //  which is that of a template parameter that might be a specialization.
   //
   virtual std::string AlignTemplateArg(const TypeSpec* thatArg) const = 0;

   //  Adds each scoped name in the type to NAMES.
   //
   virtual void GetNames(stringVector& names) const = 0;

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
   //  The item type to which the type belongs.  The default is TS_Unknown.
   //
   mutable TypeSpecUser user_ : 8;

   //  The type's role in a template.
   //
   mutable TemplateRole role_ : 8;
};

//------------------------------------------------------------------------------
//
//  A name, possibly qualified, that is being used as a type.
//
class DataSpec : public TypeSpec
{
public:
   //  Creates a type for NAME.
   //
   explicit DataSpec(QualNamePtr& name);

   //  Creates a type for NAME.
   //
   explicit DataSpec(NodeBase::c_string name);

   //  Not subclassed.
   //
   ~DataSpec();

   //  Copy constructor.
   //
   //  NOTE: arrays_ is not copied.  This means that a type with a bounded
   //  ====  array cannot be used to automatically create an instance of a
   //        function template.
   //
   DataSpec(const DataSpec& that);

   //  A DataSpec for a bool.
   //
   static const TypeSpecPtr Bool;

   //  A DataSpec for an int.
   //
   static const TypeSpecPtr Int;
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
   bool ResolveTemplateArg() const;

   //  Overridden to add a bounded array specification to the type.
   //
   void AddArray(ArraySpecPtr& array) override;

   //  Overridden to align thatArg's type with the this type, which is that of a
   //  template parameter that might be specialized.
   //
   std::string AlignTemplateArg(const TypeSpec* thatArg) const override;

   //  Overridden to return any template arguments associated with the type.
   //
   const TemplateArgPtrVector* Args() const override;

   //  Overridden to forward to the qualified name and to add its type tags.
   //
   std::string ArgString(const TemplateParmToArgMap& tmap) const override;

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

   //  Overridden to return a copy of the type.
   //
   TypeSpec* Clone() const override;

   //  Overridden to propagate the context to the type's qualified name.
   //
   void CopyContext(const CxxToken* that, bool internal) override;

   //  Overridden to return the class, if any, to which the type ultimately
   //  refers, provided that it is not a pointer or reference to that class.
   //
   Class* DirectClass() const override;

   //  Overridden to invoke DirectType on name_.
   //
   CxxScoped* DirectType() const override;

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

   //  Overridden to return the type's attributes.
   //
   TypeTags GetAllTags() const override;

   //  Overridden to invoke GetDirectClasses on its qualified name.
   //
   void GetDirectClasses(CxxUsageSets& symbols) override;

   //  If this is a direct template argument (i.e. one without pointer tags)
   //  but it was only made visible by a forward or friend declaration, its
   //  class definition is added to symbols.  If the type is to a template
   //  instance, its direct template arguments are added to SYMBOLS.  Finally,
   //  GetDirectTemplateArgs is also invoked on the type's qualified name.
   //
   void GetDirectTemplateArgs(CxxUsageSets& symbols) const override;

   //  Overridden add the type's names to NAMES.
   //
   void GetNames(stringVector& names) const override;

   //  Overridden to return the numeric type.
   //
   Numeric GetNumeric() const override;

   //  Overridden to return the type's qualified name.
   //
   QualName* GetQualName() const override { return name_.get(); }

   //  Overridden to return the type itself.
   //
   TypeSpec* GetTypeSpec() const override;

   //  Overridden to update SYMBOLS with the specification's type usage.
   //
   void GetUsages(const CodeFile& file, CxxUsageSets& symbols) override;

   //  Overridden to return true if the type has a bounded array specification.
   //
   bool HasArrayDefn() const override { return (arrays_ == nullptr); }

   //  Overridden to instantiate the type's referent if it is a template.
   //
   void Instantiating(CxxScopedVector& locals) const override;

   //  Overridden to return true if the type is "auto" and the referent has
   //  yet to be determined.
   //
   bool IsAuto() const override;

   //  Overridden to return true if the type is const.
   //
   bool IsConst() const override;

   //  Overridden to return true if the type's Nth pointer is const.
   //
   bool IsConstPtr(size_t n) const override;

   //  Overridden to return true if the type's outermost pointer is const.
   //
   bool IsConstPtr() const override;

   //  Overridden to return true if the type has pointer or reference tags,
   //  or if it is an array and ARRAYS is true.
   //
   bool IsIndirect(bool arrays) const override;

   //  Overridden to return false for an array of objects.
   //
   bool IsPOD() const override;

   //  Overridden to return true if the type is volatile.
   //
   bool IsVolatile() const override;

   //  Overridden to return true if the type's outermost pointer is volatile.
   //
   bool IsVolatilePtr() const override;

   //  Overridden to return true if the type's Nth pointer is volatile.
   //
   bool IsVolatilePtr(size_t n) const override;

   //  Overridden to return true if ITEM is the referent of a template argument.
   //
   bool ItemIsTemplateArg(const CxxNamed* item) const override;

   //  Overridden to return true if the types of "this" and THAT match.
   //
   bool MatchesExactly(const TypeSpec* that) const override;

   //  Overridden to determine if THAT can instantiate a template for this type.
   //
   TypeMatch MatchTemplate(const TypeSpec* that, stringVector& tmpltParms,
      stringVector& tmpltArgs, bool& argFound) const override;

   //  Overridden to determine how well THAT matches this type for the purpose
   //  of template instantiation.
   //
   TypeMatch MatchTemplateArg(const TypeSpec* that) const override;

   //  Overridden to return the type's name.
   //
   const std::string& Name() const override { return name_->Name(); }

   //  Overridden to determine if NAMES[INDEX] matches this type.
   //
   bool NamesReferToArgs(const NameVector& names, const CxxScope* scope,
      CodeFile* file, size_t& index) const override;

   //  Overridden to find the item located at POS.
   //
   CxxToken* PosToItem(size_t pos) const override;

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
   std::string QualifiedName(bool scopes, bool templates) const override
      { return name_->QualifiedName(scopes, templates); }

   //  Overridden to return what the type refers to.
   //
   CxxScoped* Referent() const override;

   //  Overridden to return the number of reference tags attached to the type,
   //  following the type to its root.
   //
   TagCount Refs() const override;

   //  Overridden to resolve the forward declaration only if it has template
   //  arguments.
   //
   bool ResolveForward(CxxScoped* decl, size_t n) const override;

   //  Overridden to determine if EnsureInstance should be invoked.
   //
   bool ResolveTemplate
      (Class* cls, const TypeName* type, bool end) const override;

   //  Overridden to resolve the typedef unless it has template arguments.
   //
   bool ResolveTypedef(Typedef* type, size_t n) const override;

   //  Overridden to construct an argument based on the type of referent and
   //  the level of pointer indirection to it.
   //
   StackArg ResultType() const override;

   //  Overridden to return the underlying type.
   //
   CxxToken* RootType() const override { return (CxxToken*) Referent(); }

   //  Overridden to reset the number of pointer tags when the type is assigned
   //  to an auto type.
   //
   void SetPtrs(TagCount count) override;

   //  Overridden to record what the item refers to.
   //
   void SetReferent(CxxScoped* item, const SymbolView* view) const override;

   //  Overridden so that when ROLE is TemplateClass, its arguments are treated
   //  as parameters.
   //
   void SetTemplateRole(TemplateRole role) const override;

   //  Overridden to propagate USER to name_.
   //
   void SetUserType(TypeSpecUser user) const override;

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

   //  Overridden to update the type's location.
   //
   void UpdatePos(EditorAction action,
      size_t begin, size_t count, size_t from) const override;

   //  Overridden to add the specification's components to cross-references.
   //
   void UpdateXref(bool insert) override;

   //  Overridden to check the specification's name.
   //
   bool VerifyReferents() const override;

   //  Overridden to support a temporary variable represented by a DataSpec.
   //
   bool WasWritten(const StackArg* arg, bool direct, bool indirect) override
      { return false; }

   //  The qualified name for the type as it appeared in the source code.
   //
   QualNamePtr name_;

   //  The type's bounded array specifications (e.g. for int[10][10]).
   //
   std::unique_ptr<ArraySpecPtrVector> arrays_;

   //  The type's tags.
   //
   TypeTags tags_;
};

//------------------------------------------------------------------------------
//
//  A template argument.  It can be a TypeSpec or an Expression, so it
//  usually forwards to whichever one of them it is wrapping.
//
class TemplateArg : public CxxNamed
{
public:
   //  Used when the argument is a TypeSpec.
   //
   explicit TemplateArg(TypeSpecPtr& type);

   //  Used when the argument is an Expression.
   //
   explicit TemplateArg(ExprPtr& expr);

   //  Not subclassed.
   //
   ~TemplateArg();

   //  Invoked when the argument was added as a template parameter default.
   //
   void SetImplicit() { implicit_ = true; }

   //  During template instantiation, aligns thatArg's type with this one,
   //  which is that of a template parameter that might be a specialization.
   //
   std::string AlignTemplateArg(const TemplateArg* thatArg) const;

   //  Invoked when the argument is entered into SCOPE.
   //
   void EnteringScope(const CxxScope* scope) const;

   //  Adds the argument to NAMES.
   //
   void GetNames(stringVector& names) const;

   //  Returns the argument's referent.
   //
   CxxScoped* GetReferent() const;

   //  Determines if THAT can instantiate a template for this type.
   //
   TypeMatch MatchTemplate(const TemplateArg* that, stringVector& tmpltParms,
      stringVector& tmpltArgs, bool& argFound) const;

   //  Determines how well THAT matches this argument for the purpose of
   //  template instantiation.
   //
   TypeMatch MatchTemplateArg(const TemplateArg* that) const;

   //  Sets the template argument's role.
   //
   void SetTemplateRole(TemplateRole role) const;

   //  Sets the template argument's user type.
   //
   void SetUserType(TypeSpecUser user) const;

   //  Each override forwards to the type or expression that serves as
   //  the template argument.
   //
   std::string ArgString(const TemplateParmToArgMap& tmap) const override;
   void Check() const override;
   CxxToken* Clone() const override;
   void CopyContext(const CxxToken* that, bool internal) override;
   void EnterBlock() override;
   void FindReferent() override;
   void GetDirectClasses(CxxUsageSets& symbols) override;
   void GetDirectTemplateArgs(CxxUsageSets& symbols) const override;
   TypeSpec* GetTypeSpec() const override { return spec_.get(); }
   void GetUsages(const CodeFile& file, CxxUsageSets& symbols) override;
   void Instantiating(CxxScopedVector& locals) const override;
   bool ItemIsTemplateArg(const CxxNamed* item) const override;
   const std::string& Name() const override;
   bool NamesReferToArgs(const NameVector& names, const CxxScope* scope,
      CodeFile* file, size_t& index) const override;
   CxxToken* PosToItem(size_t pos) const override;
   void Print
      (std::ostream& stream, const NodeBase::Flags& options) const override;
   std::string QualifiedName(bool scopes, bool templates) const override;
   StackArg ResultType() const override;
   std::string Trace() const override;
   std::string TypeString(bool arg) const override;
   void UpdatePos(EditorAction action,
      size_t begin, size_t count, size_t from) const override;
   void UpdateXref(bool insert) override;
   bool VerifyReferents() const override;
private:
   //  Used when the argument is a TypeSpec.
   //
   const TypeSpecPtr spec_;

   //  Used when the argument is an Expression.
   //
   const ExprPtr expr_;

   //  Set if the argument was provided by a template parameter default.
   //
   bool implicit_;
};
}
#endif
