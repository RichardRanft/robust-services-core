//==============================================================================
//
//  CxxScoped.cpp
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
#include "CxxScoped.h"
#include <bitset>
#include <cstdint>
#include <sstream>
#include <utility>
#include "CodeFile.h"
#include "CxxArea.h"
#include "CxxExecute.h"
#include "CxxRoot.h"
#include "CxxScope.h"
#include "CxxString.h"
#include "CxxSymbols.h"
#include "CxxVector.h"
#include "Debug.h"
#include "Formatters.h"
#include "Lexer.h"
#include "Singleton.h"

using namespace NodeBase;
using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace CodeTools
{
Argument::Argument(string& name, TypeSpecPtr& spec) :
   spec_(spec.release()),
   reads_(0),
   writes_(0),
   nonconst_(false),
   modified_(false)
{
   Debug::ft("Argument.ctor");

   std::swap(name_, name);
}

//------------------------------------------------------------------------------

Argument::~Argument()
{
   Debug::ft("Argument.dtor");
}

//------------------------------------------------------------------------------

void Argument::Check() const
{
   Debug::ft("Argument.Check");

   spec_->Check();
   if(default_ != nullptr) default_->Check();
   if(name_.empty() && IsUnused()) LogToFunc(AnonymousArgument);
   if(modified_ && spec_->Refs() == 0) LogToFunc(ValueArgumentModified);
}

//------------------------------------------------------------------------------

void Argument::CheckVoid() const
{
   Debug::ft("Argument.CheckVoid");

   if(name_.empty())
   {
      if((spec_->Name() == VOID_STR) && (spec_->Ptrs(false) == 0))
      {
         //  Deleting the empty argument "(void)" makes it much easier to
         //  compare function signatures and match arguments to functions.
         //
         Log(VoidAsArgument);
         auto func = static_cast<Function*>(GetScope());
         func->DeleteVoidArg();
      }
   }
}

//------------------------------------------------------------------------------

void Argument::Delete()
{
   Debug::ft("Argument.Delete");

   auto func = GetFunction();
   if(func != nullptr) func->DeleteArg(this);
}

//------------------------------------------------------------------------------

void Argument::EnterBlock()
{
   Debug::ft("Argument.EnterBlock");

   Context::SetPos(GetLoc());
   if(!name_.empty()) Context::InsertLocal(this);
}

//------------------------------------------------------------------------------

bool Argument::EnterScope()
{
   Debug::ft("Argument.EnterScope");

   Context::SetPos(GetLoc());
   spec_->EnteringScope(GetScope());

   if(default_ != nullptr)
   {
      default_->EnterBlock();
      auto result = Context::PopArg(true);
      spec_->MustMatchWith(result);
   }

   CheckVoid();
   return true;
}

//------------------------------------------------------------------------------

void Argument::ExitBlock() const
{
   Debug::ft("Argument.ExitBlock");

   if(name_.empty()) return;
   Context::EraseLocal(this);
}

//------------------------------------------------------------------------------

bool Argument::GetSpan(size_t& begin, size_t& left, size_t& end) const
{
   Debug::ft("Argument.GetSpan");

   begin = spec_->GetPos();

   const auto& lexer = GetFile()->GetLexer();
   auto prev = lexer.RfindFirstOf(begin, ",(");
   auto next = lexer.FindFirstOf(",)", begin);

   if(lexer.At(next) == ',')
   {
      begin = prev + 1;
      end = next;
   }
   else if(lexer.At(prev) == ',')
   {
      begin = prev;
      end = next - 1;
   }
   else
   {
      begin = prev + 1;
      end = next - 1;
   }

   return true;
}

//------------------------------------------------------------------------------

void Argument::GetUsages(const CodeFile& file, CxxUsageSets& symbols)
{
   spec_->GetUsages(file, symbols);
   if(default_ != nullptr) default_->GetUsages(file, symbols);
}

//------------------------------------------------------------------------------

bool Argument::IsDummy() const
{
   Debug::ft("Argument.IsDummy");

   const auto& type = GetTypeSpec()->Name();

   if(type == "nothrow_t") return true;

   if(type == "int")
   {
      const auto& fname = static_cast<Function*>(GetScope())->Name();
      return ((fname == "operator++") || (fname == "operator--"));
   }

   return false;
}

//------------------------------------------------------------------------------

Class* Argument::IsThisCandidate() const
{
   Debug::ft("Argument.IsThisCandidate");

   auto ref = spec_->Referent();
   if(ref == nullptr) return nullptr;
   if(ref->Type() != Cxx::Class) return nullptr;
   auto cls = static_cast<Class*>(ref);
   if(cls->GetFile()->IsSubsFile()) return nullptr;
   if(cls->IsInternal()) return nullptr;
   if(IsConst()) return nullptr;
   if(spec_->Ptrs(true) + spec_->Refs() != 1) return nullptr;
   return cls;
}

//------------------------------------------------------------------------------

bool Argument::IsUnused() const
{
   Debug::ft("Argument.IsUnused");

   if((reads_ > 0) || (writes_ > 0)) return false;
   return (IsDummy() ? false : true);
}

//------------------------------------------------------------------------------

void Argument::LogToFunc(Warning warning) const
{
   Debug::ft("Argument.LogToFunc");

   auto func = static_cast<Function*>(GetScope());
   auto offset = func->FindArg(this, true);
   if(offset == SIZE_MAX) offset = 0;
   Log(warning, func, offset);
}

//------------------------------------------------------------------------------

CxxToken* Argument::PosToItem(size_t pos) const
{
   auto item = CxxScoped::PosToItem(pos);
   if(item != nullptr) return item;

   item = spec_->PosToItem(pos);
   if(item != nullptr) return item;

   return (default_ != nullptr ? default_->PosToItem(pos) : nullptr);
}

//------------------------------------------------------------------------------

void Argument::Print(ostream& stream, const Flags& options) const
{
   spec_->Print(stream, options);

   if(spec_->GetFuncSpec() == nullptr)
   {
      if(!name_.empty()) stream << SPACE << name_;
   }

   spec_->DisplayArrays(stream);

   if(default_ != nullptr)
   {
      stream << " = ";
      default_->Print(stream, options);
   }

   if(!options.test(DispStats)) return;
   if((reads_ == 0) && (writes_ == 0)) return;
   stream << SPACE << COMMENT_BEGIN_STR;
   stream << SPACE << "r=" << reads_;
   if(writes_ > 0) stream << SPACE << "w=" << writes_;
   stream << SPACE << COMMENT_END_STR;
}

//------------------------------------------------------------------------------

void Argument::Rename(const string& name)
{
   Debug::ft("Argument.Rename");

   CxxScoped::RenameNonQual(name_, name);
}

//------------------------------------------------------------------------------

bool Argument::SetNonConst()
{
   Debug::ft("Argument.SetNonConst");

   if(!nonconst_)
   {
      nonconst_ = true;
      auto item = static_cast<Argument*>(FindTemplateAnalog(this));
      if(item != nullptr) item->nonconst_ = true;
   }

   return !IsConst();
}

//------------------------------------------------------------------------------

string Argument::TypeString(bool arg) const
{
   return spec_->TypeString(arg);
}

//------------------------------------------------------------------------------

void Argument::UpdatePos
   (EditorAction action, size_t begin, size_t count, size_t from) const
{
   CxxScoped::UpdatePos(action, begin, count, from);
   spec_->UpdatePos(action, begin, count, from);
   if(default_ != nullptr) default_->UpdatePos(action, begin, count, from);
}

//------------------------------------------------------------------------------

void Argument::UpdateXref(bool insert)
{
   spec_->UpdateXref(insert);
   if(default_ != nullptr) default_->UpdateXref(insert);
}

//------------------------------------------------------------------------------

bool Argument::WasRead()
{
   Debug::ft("Argument.WasRead");

   ++reads_;
   auto item = static_cast<Argument*>(FindTemplateAnalog(this));
   if(item != nullptr) ++item->reads_;
   return true;
}

//------------------------------------------------------------------------------

bool Argument::WasWritten(const StackArg* arg, bool direct, bool indirect)
{
   Debug::ft("Argument.WasWritten");

   ++writes_;
   auto item = static_cast<Argument*>(FindTemplateAnalog(this));
   if(item != nullptr) ++item->writes_;

   if((arg == nullptr) || (arg->Ptrs(true) == 0))
   {
      nonconst_ = true;
      if(item != nullptr) item->nonconst_ = true;
   }

   if(direct && (name_ != THIS_STR) && (arg != nullptr) &&
      !arg->UsedIndirectly())
   {
      modified_ = true;
      if(item != nullptr) item->modified_ = true;
   }

   return true;
}

//==============================================================================

BaseDecl::BaseDecl(QualNamePtr& name, Cxx::Access access) :
   name_(name.release()),
   using_(false)
{
   Debug::ft("BaseDecl.ctor");

   SetAccess(access);
}

//------------------------------------------------------------------------------

BaseDecl::~BaseDecl()
{
   Debug::ft("BaseDecl.dtor");

   GetClass()->EraseSubclass(static_cast<Class*>(GetScope()));
}

//------------------------------------------------------------------------------

void BaseDecl::Check() const
{
   name_->Check();
}

//------------------------------------------------------------------------------

void BaseDecl::DisplayDecl(ostream& stream, bool fq) const
{
   stream << " : " << GetAccess() << SPACE;
   strName(stream, fq, name_.get());
}

//------------------------------------------------------------------------------

bool BaseDecl::EnterScope()
{
   Debug::ft("BaseDecl.EnterScope");

   //  If the base class cannot be found, return false so that this
   //  object will be deleted.  Otherwise, record our new subclass.
   //
   Context::SetPos(GetLoc());
   FindReferent();
   if(Referent() == nullptr) return false;
   GetClass()->AddSubclass(static_cast<Class*>(Context::Scope()));
   return true;
}

//------------------------------------------------------------------------------

fn_name BaseDecl_FindReferent = "BaseDecl.FindReferent";

void BaseDecl::FindReferent()
{
   Debug::ft(BaseDecl_FindReferent);

   //  Find the class to which this base class declaration refers.
   //
   SymbolView view;
   auto item = ResolveName(GetFile(), GetScope(), CLASS_MASK, view);

   if(item != nullptr)
   {
      using_ = view.using_;
      item->SetAsReferent(this);
      return;
   }

   //  The base class wasn't found.
   //
   auto log = "Unknown base class: " + Name() + " [" + strLocation() + ']';
   Debug::SwLog(BaseDecl_FindReferent, log, 0, false);
}

//------------------------------------------------------------------------------

Class* BaseDecl::GetClass() const
{
   return static_cast<Class*>(name_->GetReferent());
}

//------------------------------------------------------------------------------

bool BaseDecl::GetSpan(size_t& begin, size_t& left, size_t& end) const
{
   Debug::ft("BaseDecl.GetSpan");

   begin = GetPos();

   const auto& lexer = GetFile()->GetLexer();
   begin = lexer.RfindFirstOf(begin, ":");
   end = lexer.FindFirstOf("{", begin);
   end = lexer.RfindNonBlank(end - 1);
   return true;
}

//------------------------------------------------------------------------------

void BaseDecl::GetUsages(const CodeFile& file, CxxUsageSets& symbols)
{
   //  Our class was used as a base class.  Its name cannot include template
   //  arguments, because subclassing a template instance is not supported.
   //
   symbols.AddBase(GetClass());
   if(using_) symbols.AddUser(this);
}

//------------------------------------------------------------------------------

CxxToken* BaseDecl::PosToItem(size_t pos) const
{
   auto item = CxxScoped::PosToItem(pos);
   if(item != nullptr) return item;

   return name_->PosToItem(pos);
}

//------------------------------------------------------------------------------

CxxScoped* BaseDecl::Referent() const
{
   Debug::ft("BaseDecl.Referent");

   return name_->GetReferent();
}

//------------------------------------------------------------------------------

string BaseDecl::ScopedName(bool templates) const
{
   return Referent()->ScopedName(templates);
}

//------------------------------------------------------------------------------

void BaseDecl::SetAccess(Cxx::Access access)
{
   Debug::ft("BaseDecl.SetAccess");

   //  This is invoked twice: first by our constructor, and then by
   //  Parser::SetContext.  We want to preserve the value set by our
   //  constructor, so if the current value isn't Cxx::Public (the
   //  default), it has already been set and should be preserved.
   //
   if(GetAccess() != Cxx::Public) return;
   CxxScoped::SetAccess(access);
}

//------------------------------------------------------------------------------

string BaseDecl::TypeString(bool arg) const
{
   return GetClass()->TypeString(arg);
}

//------------------------------------------------------------------------------

void BaseDecl::UpdatePos
   (EditorAction action, size_t begin, size_t count, size_t from) const
{
   CxxScoped::UpdatePos(action, begin, count, from);
   name_->UpdatePos(action, begin, count, from);
}

//------------------------------------------------------------------------------

void BaseDecl::UpdateXref(bool insert)
{
   name_->UpdateXref(insert);
}

//==============================================================================

CxxScoped::CxxScoped() :
   scope_(nullptr),
   access_(Cxx::Public),
   public_(false),
   protected_(false)
{
   Debug::ft("CxxScoped.ctor");
}

//------------------------------------------------------------------------------

CxxScoped::~CxxScoped()
{
   Debug::ftnt("CxxScoped.dtor");

   for(auto r = xref_.cbegin(); r != xref_.cend(); ++r)
   {
      (*r)->ItemDeleted(this);
   }
}

//------------------------------------------------------------------------------

void CxxScoped::AccessibilityTo(const CxxScope* scope, SymbolView& view) const
{
   Debug::ft("CxxScoped.AccessibilityTo");

   return GetScope()->AccessibilityOf(scope, this, view);
}

//------------------------------------------------------------------------------

void CxxScoped::AddFiles(LibItemSet& imSet) const
{
   auto decl = GetDeclFile();
   auto defn = GetDefnFile();
   if(decl != nullptr) imSet.insert(decl);
   if(defn != nullptr) imSet.insert(defn);
}

//------------------------------------------------------------------------------

Cxx::Access CxxScoped::BroadestAccessUsed() const
{
   Debug::ft("CxxScoped.BroadestAccessUsed");

   if(GetClass() == nullptr) return Cxx::Public;
   if(public_) return Cxx::Public;
   if(protected_) return Cxx::Protected;
   return Cxx::Private;
}

//------------------------------------------------------------------------------

void CxxScoped::CheckAccessControl() const
{
   Debug::ft("CxxScoped.CheckAccessControl");

   if(SkipAccessControlCheck()) return;

   auto used = BroadestAccessUsed();

   switch(used)
   {
   case Cxx::Private:
      if(GetAccess() > used) Log(ItemCouldBePrivate);
      break;
   case Cxx::Protected:
      if(GetAccess() > used) Log(ItemCouldBeProtected);
   }
}

//------------------------------------------------------------------------------

void CxxScoped::CheckIfHiding() const
{
   Debug::ft("CxxScoped.CheckIfHiding");

   auto item = FindInheritedName();
   if((item == nullptr) || (item->GetAccess() == Cxx::Private)) return;
   Log(HidesInheritedName);
}

//------------------------------------------------------------------------------

bool CxxScoped::CheckIfUnused(Warning warning) const
{
   Debug::ft("CxxScoped.CheckIfUnused");

   if(!IsUnused()) return false;
   Log(warning);
   return true;
}

//------------------------------------------------------------------------------

bool CxxScoped::Contains(const CxxToken* item) const
{
   Debug::ft("CxxScoped.Contains");

   if(item == nullptr) return false;

   auto file1 = this->GetFile();
   auto file2 = item->GetFile();
   if(file1 != file2) return false;

   size_t begin, end;
   GetSpan2(begin, end);

   auto pos = item->GetPos();
   return ((pos >= begin) && (pos <= end));
}

//------------------------------------------------------------------------------

void CxxScoped::CopyContext(const CxxToken* that, bool internal)
{
   Debug::ft("CxxScoped.CopyContext");

   CxxNamed::CopyContext(that, internal);

   SetScope(that->GetScope());
   SetAccess(that->GetAccess());
}

//------------------------------------------------------------------------------

void CxxScoped::DisplayFiles(ostream& stream) const
{
   auto decl = GetDeclFile();
   auto defn = GetDefnFile();

   if(IsAtFileScope())
   {
      if(decl != nullptr)
      {
         stream << decl->Name();

         if((defn != nullptr) && (defn != decl))
         {
            stream << " & " << defn->Name();
         }
      }
   }
   else if((defn != nullptr) && (defn != decl))
   {
      stream << defn->Name();
   }
}

//------------------------------------------------------------------------------

Accessibility CxxScoped::FileScopeAccessiblity() const
{
   Debug::ft("CxxScoped.FileScopeAccessiblity");

   if(IsInTemplateInstance()) return Unrestricted;
   if(GetFile()->IsCpp()) return Restricted;
   return Unrestricted;
}

//------------------------------------------------------------------------------

CxxScoped* CxxScoped::FindInheritedName() const
{
   Debug::ft("CxxScoped.FindInheritedName");

   auto cls = GetClass();
   if(cls == nullptr) return nullptr;
   auto base = cls->BaseClass();
   if(base == nullptr) return nullptr;
   return base->FindName(Name(), nullptr);
}

//------------------------------------------------------------------------------

CxxScoped* CxxScoped::FindNthItem(const std::string& name, size_t& n) const
{
   Debug::ft("CxxScoped.FindNthItem");

   if(n == 0) return nullptr;
   if(name == Name()) --n;
   if(n == 0) return const_cast<CxxScoped*>(this);
   return nullptr;
}

//------------------------------------------------------------------------------

bool CxxScoped::GetBracedSpan(size_t& begin, size_t& left, size_t& end) const
{
   Debug::ft("CxxScoped.GetBracedSpan");

   const auto& lexer = GetFile()->GetLexer();
   begin = GetPos();
   if(begin == string::npos) return false;
   left = lexer.FindFirstOf("{", begin);
   if(left == string::npos) return false;
   end = lexer.FindClosing('{', '}', left + 1);
   return (end != string::npos);
}

//------------------------------------------------------------------------------

CxxTokenVector CxxScoped::GetNonLocalRefs() const
{
   Debug::ft("CxxScoped.GetNonLocalRefs");

   CxxTokenVector items;
   auto mate = GetMate();

   for(auto r = xref_.cbegin(); r != xref_.cend(); ++r)
   {
      if(this->Contains(*r)) continue;
      if((mate != nullptr) && mate->Contains(*r)) continue;
      items.push_back(*r);
   }

   return items;
}

//------------------------------------------------------------------------------

bool CxxScoped::GetTypeSpan(size_t& begin, size_t& end) const
{
   Debug::ft("CxxScoped.GetTypeSpan");

   auto spec = GetTypeSpec();
   if((spec == nullptr) || spec->IsInternal())
      begin = GetPos();
   else
      begin = spec->GetPos();
   if(begin == string::npos) return false;

   //  Functions and data can have leading keywords (e.g. static), so back up
   //  to the end of the previous item and then step forward to the next parse
   //  position.  The colon will find the end of an access control; a scope
   //  resolution operator shouldn't be encountered here because we're already
   //  at the start of the TypeSpec, which can only be preceded by keywords.
   //
   const auto& lexer = GetFile()->GetLexer();
   auto prev = lexer.RfindFirstOf(begin - 1, ";{}:");
   if(prev != string::npos) begin = lexer.NextPos(prev + 1);

   end = lexer.FindFirstOf(";", begin);
   return (end != string::npos);
}

//------------------------------------------------------------------------------

bool CxxScoped::IsAtFileScope() const
{
   Debug::ft("CxxScoped.IsAtFileScope");

   auto scope = GetScope();
   if(scope == nullptr) return false;
   return (scope->Type() == Cxx::Namespace);
}

//------------------------------------------------------------------------------

bool CxxScoped::IsAuto() const
{
   auto spec = GetTypeSpec();
   if(spec == nullptr) return false;
   return spec->IsAuto();
}

//------------------------------------------------------------------------------

bool CxxScoped::IsClassMember() const
{
   if(Type() != Cxx::Class)
   {
      auto cls = GetClass();
      if(cls == nullptr) return false;
      return (cls->GetClassTag() == Cxx::ClassType);
   }

   return false;
}

//------------------------------------------------------------------------------

bool CxxScoped::IsConst() const
{
   auto spec = GetTypeSpec();
   if(spec == nullptr) return false;
   return spec->IsConst();
}

//------------------------------------------------------------------------------

bool CxxScoped::IsConstPtr() const
{
   auto spec = GetTypeSpec();
   if(spec == nullptr) return false;
   return spec->IsConstPtr();
}

//------------------------------------------------------------------------------

bool CxxScoped::IsConstPtr(size_t n) const
{
   auto spec = GetTypeSpec();
   if(spec == nullptr) return false;
   return spec->IsConstPtr(n);
}

//------------------------------------------------------------------------------

bool CxxScoped::IsDeclaredInFunction() const
{
   auto type = scope_->Type();
   return ((type == Cxx::Block) || (type == Cxx::Function));
}

//------------------------------------------------------------------------------

bool CxxScoped::IsDefinedIn(const CxxArea* area) const
{
   for(auto s = GetScope(); s != nullptr; s = s->GetScope())
   {
      if(s == area) return true;
      if(s->Type() == Cxx::Namespace) return false;
   }

   return false;
}

//------------------------------------------------------------------------------

bool CxxScoped::IsIndirect(bool arrays) const
{
   auto spec = GetTypeSpec();
   if(spec == nullptr) return false;
   return spec->IsIndirect(arrays);
}

//------------------------------------------------------------------------------

bool CxxScoped::IsSubscopeOf(const string& fqSuper) const
{
   Debug::ft("CxxScoped.IsSubscopeOf");

   auto fqSub = ScopedName(false);
   return (CompareScopes(fqSub, fqSuper, false) != string::npos);
}

//------------------------------------------------------------------------------

bool CxxScoped::IsSuperscopeOf(const string& fqSub, bool tmplt) const
{
   Debug::ft("CxxScoped.IsSuperscopeOf");

   auto fqSuper = ScopedName(tmplt);
   return (CompareScopes(fqSub, fqSuper, tmplt) != string::npos);
}

//------------------------------------------------------------------------------

bool CxxScoped::LocateItem(const CxxToken* item, size_t& n) const
{
   Debug::ft("CxxScoped.LocateItem");

   if(item == this)
   {
      ++n;
      return true;
   }

   if(item->Name() == Name()) ++n;
   return false;
}

//------------------------------------------------------------------------------

fn_name CxxScoped_NameRefersToItem = "CxxScoped.NameRefersToItem";

bool CxxScoped::NameRefersToItem(const string& name,
   const CxxScope* scope, CodeFile* file, SymbolView& view) const
{
   Debug::ft(CxxScoped_NameRefersToItem);

   auto itemType = Type();
   auto itemFile = GetFile();

   if(itemFile == nullptr)
   {
      auto expl = "No file for item: " + Name();
      Context::SwLog(CxxScoped_NameRefersToItem, expl, itemType);
      return false;
   }

   //  The file that declares this item must affect (that is, be in the
   //  transitive #include of) FILE.  The check can fail when looking up a
   //  namespace, which is arbitrarily assigned to the first file where it
   //  appears, even though it can appear in many others.
   //
   auto iter = file->Affecters().find(itemFile);
   auto affected = (iter != file->Affecters().cend());
   if(!affected && (itemType != Cxx::Namespace)) return false;

   //  See how SCOPE can access this item: this information is provided in
   //  VIEW.  Set checkUsing if a using statement will be needed for ITEM
   //  if it is in another namespace.
   //
   auto checkUsing = true;
   AccessibilityTo(scope, view);

   switch(view.accessibility_)
   {
   case Inaccessible:
      return false;

   case Restricted:
      if(file != itemFile) return false;
      break;

   case Inherited:
   case Declared:
      checkUsing = false;
   }

   //  NAME must partially match this item's fully qualified name.
   //
   stringVector fqNames;
   GetScopedNames(fqNames, false);

   for(auto fqn = fqNames.begin(); fqn != fqNames.end(); ++fqn)
   {
      auto pos = NameCouldReferTo(*fqn, name);
      if(pos == string::npos) continue;

      switch(pos)
      {
      case 0:
      case 2:
         //
         //  NAME completely matches this item's fully qualified name,
         //  with the possible exception of a leading scope resolution
         //  operator.
         //
         return true;
      case 1:
      case 3:
         //
         //  These shouldn't occur, because fqName has a "::" prefix.
         //
         Debug::SwLog(CxxScoped_NameRefersToItem, *fqn, pos);
         return false;
      }

      //  NAME is a partial match for this item.  Report a match if SCOPE
      //  is this item's declarer or one of its subclasses.
      //
      if(!checkUsing) return true;

      //  Report a match if SCOPE is already in this item's scope.
      //
      fqn->erase(0, 2);
      auto prefix = fqn->substr(0, pos - 4);
      if(scope->IsSubscopeOf(prefix)) return true;

      //  Report a match if SCOPE's class derives from this item's class.
      //
      auto itemClass = Declarer();
      if(itemClass != nullptr)
      {
         auto usingClass = scope->GetClass();
         if((usingClass != nullptr) && usingClass->DerivesFrom(itemClass))
            return true;
      }

      //  Look for a using statement that matches at least the PREFIX
      //  of fqName.  That is, if fqName is "a::b::c::d" and PREFIX is
      //  "a::b", the using statement must be for "a::b", "a::b::c",
      //  or "a::b::c::d".
      //
      if(file->FindUsingFor(*fqn, pos - 4, this, scope) != nullptr)
      {
         view.using_ = true;
         return true;
      }
   }

   return false;
}

//------------------------------------------------------------------------------

fn_name CxxScoped_RecordAccess = "CxxScoped.RecordAccess";

void CxxScoped::RecordAccess(Cxx::Access access) const
{
   Debug::ft(CxxScoped_RecordAccess);

   if(access == Cxx::Access_N) return;

   if(access > access_)
   {
      auto expl = "Member should be inaccessible: " + ScopedName(true);
      Context::SwLog(CxxScoped_RecordAccess, expl, access);
   }

   if(access == Cxx::Public)
   {
      if(public_) return;
      public_ = true;
      RecordTemplateAccess(Cxx::Public);
   }
   else if(access == Cxx::Protected)
   {
      if(protected_) return;
      protected_ = true;
      RecordTemplateAccess(Cxx::Protected);
   }
}

//------------------------------------------------------------------------------

void CxxScoped::RecordTemplateAccess(Cxx::Access access) const
{
   Debug::ft("CxxScoped.RecordTemplateAccess");

   auto item = FindTemplateAnalog(this);
   if(item != nullptr) item->RecordAccess(access);
}

//------------------------------------------------------------------------------

void CxxScoped::RenameNonQual(string& oldName, const string& newName) const
{
   Debug::ft("CxxScoped.RenameNonQual");

   if(oldName == newName) return;

   for(auto r = xref_.begin(); r != xref_.end(); ++r)
   {
      (*r)->Rename(newName);
   }

   CxxNamed::RenameNonQual(oldName, newName);
}

//------------------------------------------------------------------------------

void CxxScoped::RenameQual(QualName& oldName, const string& newName) const
{
   Debug::ft("CxxScoped.RenameQual");

   if(oldName.Name() == newName) return;

   for(auto r = xref_.begin(); r != xref_.end(); ++r)
   {
      (*r)->Rename(newName);
   }

   oldName.Rename(newName);
}

//------------------------------------------------------------------------------

void CxxScoped::SetContext(size_t pos)
{
   Debug::ft("CxxScoped.SetContext");

   CxxNamed::SetContext(pos);

   //  If the item has already set its scope, don't overwrite it.
   //
   auto scope = GetScope();

   if(scope == nullptr)
   {
      scope = Context::Scope();
      SetScope(scope);
   }

   SetAccess(scope->GetCurrAccess());
}

//------------------------------------------------------------------------------

bool CxxScoped::SkipAccessControlCheck() const
{
   Debug::ft("CxxScoped.SkipAccessControlCheck");

   if(!IsClassMember()) return true;
   if(IsInternal()) return true;
   if(IsUnused()) return true;
   return false;
}

//------------------------------------------------------------------------------

void CxxScoped::UpdateReference(CxxToken* item, bool insert) const
{
   auto file = item->GetFile();
   if(file == nullptr) return;
   if(file->IsSubsFile()) return;
   if(insert && (item->GetPos() == string::npos)) return;

   //  If this item is not internal (i.e. appears in original source code),
   //  don't track references from an internal item (e.g. one that appears
   //  in a template instance).
   //
   if(item->IsInternal() && !this->IsInternal()) return;

   if(Context::GetXrefUpdater() == InstanceFunction)
   {
      //  A function in a template instance only adds, to the cross-reference,
      //  items that were unresolved in its template.  These items are usually
      //  functions invoked via a template parameter, and so the instance will
      //  often invoke an override in a derived class.  This should be aliased
      //  back to the base class declaration of the function.
      //
      const auto& name = item->Name();
      if(name.empty()) return;

      auto prev = Context::FindXrefItem(name);
      if(prev == nullptr) return;

      auto ref = item->Referent();

      if(ref->IsInTemplateInstance())
      {
         ref = ref->FindTemplateAnalog(ref);
      }

      if(ref->Type() == Cxx::Function)
      {
         ref = static_cast<const Function*>(ref)->FindRootFunc();
      }

      //  A template instance can't be edited, so an erasure shouldn't occur
      //  here.  But just in case...
      //
      if(insert)
         ref->Xref()->insert(prev);
      else
         ref->Xref()->erase(prev);
      return;
   }

   if(insert)
      xref_.insert(item);
   else
      xref_.erase(item);
}

//------------------------------------------------------------------------------

CxxTokenVector CxxScoped::XrefItems() const
{
   CxxTokenVector refs;

   for(auto r = xref_.cbegin(); r != xref_.cend(); ++r)
   {
      if(!(*r)->IsInternal()) refs.push_back(*r);
   }

   return refs;
}

//==============================================================================

Enum::Enum(string& name) : refs_(0)
{
   Debug::ft("Enum.ctor");

   std::swap(name_, name);
   if(!name_.empty()) Singleton<CxxSymbols>::Instance()->InsertEnum(this);
}

//------------------------------------------------------------------------------

Enum::~Enum()
{
   Debug::ftnt("Enum.dtor");

   GetFile()->EraseEnum(this);
   if(!name_.empty()) Singleton<CxxSymbols>::Extant()->EraseEnum(this);
}

//------------------------------------------------------------------------------

void Enum::AddEnumerator(string& name, ExprPtr& init, size_t pos)
{
   Debug::ft("Enum.AddEnumerator");

   EnumeratorPtr etor(new Enumerator(name, init, this));
   etor->SetScope(GetScope());
   etor->SetLoc(GetFile(), pos);
   etor->SetAccess(GetAccess());
   etors_.push_back(std::move(etor));
}

//------------------------------------------------------------------------------

void Enum::AddType(TypeSpecPtr& type)
{
   Debug::ft("Enum.AddType");

   spec_ = std::move(type);
}

//------------------------------------------------------------------------------

void Enum::Check() const
{
   Debug::ft("Enum.Check");

   if(alignas_ != nullptr) alignas_->Check();
   if(spec_ != nullptr) spec_->Check();

   for(auto e = etors_.cbegin(); e != etors_.cend(); ++e)
   {
      (*e)->Check();
   }

   if(name_.empty()) Log(AnonymousEnum);
   CheckIfUnused(EnumUnused);
   CheckIfHiding();
   CheckAccessControl();
}

//------------------------------------------------------------------------------

void Enum::CheckAccessControl() const
{
   Debug::ft("Enum.CheckAccessControl");

   if(SkipAccessControlCheck()) return;

   //  Whether the access control can be further restricted depends on
   //  each of the enumerators as well as the enumeration type itself.
   //
   auto ctrl = GetAccess();
   auto max = BroadestAccessUsed();
   if(max >= ctrl) return;

   for(auto e = etors_.cbegin(); e != etors_.cend(); ++e)
   {
      auto used = (*e)->BroadestAccessUsed();
      if(used >= ctrl) return;
      if(used > max) max = used;
   }

   switch(max)
   {
   case Cxx::Private:
      Log(ItemCouldBePrivate);
      return;
   case Cxx::Protected:
      Log(ItemCouldBeProtected);
      return;
   }
}

//------------------------------------------------------------------------------

void Enum::Delete()
{
   Debug::ftnt("Enum.Delete");

   GetArea()->DeleteEnum(this);
}

//------------------------------------------------------------------------------

void Enum::DeleteEnumerator(const Enumerator* etor)
{
   Debug::ft("Enum.DeleteEnumerator");

   DeleteItemPtr(etors_, etor);
}

//------------------------------------------------------------------------------

void Enum::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   auto anon = name_.empty();
   auto fq = options.test(DispFQ);
   stream << prefix;
   if(GetScope()->Type() == Cxx::Class) stream << GetAccess() << ": ";
   stream << ENUM_STR;

   if(alignas_ != nullptr)
   {
      stream << SPACE;
      alignas_->Print(stream, options);
   }

   if(!anon) stream << SPACE << (fq ? ScopedName(true) : name_);

   if(spec_ != nullptr)
   {
      stream << " : ";
      spec_->Print(stream, options);
   }

   if(!options.test(DispCode))
   {
      std::ostringstream buff;
      buff << " // ";
      if(!anon && options.test(DispStats)) buff << "r=" << refs_ << SPACE;
      if(!fq) DisplayFiles(buff);
      auto str = buff.str();
      if(str.size() > 4) stream << str;
   }

   auto opts = options;
   stream << CRLF << prefix << '{' << CRLF;

   auto lead = prefix + spaces(IndentSize());

   for(auto e = etors_.cbegin(); e != etors_.cend(); ++e)
   {
      if(*e == etors_.back()) opts.set(DispLast);
      (*e)->Display(stream, lead, opts);
      stream << CRLF;
   }

   stream << prefix << "};" << CRLF;
}

//------------------------------------------------------------------------------

void Enum::EnterBlock()
{
   Debug::ft("Enum.EnterBlock");

   Context::SetPos(GetLoc());

   if(alignas_ != nullptr) alignas_->EnterBlock();
   if(spec_ != nullptr) spec_->EnteringScope(GetScope());

   for(auto e = etors_.cbegin(); e != etors_.cend(); ++e)
   {
      (*e)->EnterBlock();
   }
}

//------------------------------------------------------------------------------

bool Enum::EnterScope()
{
   Debug::ft("Enum.EnterScope");

   Context::SetPos(GetLoc());
   if(IsAtFileScope()) GetFile()->InsertEnum(this);
   EnterBlock();
   return true;
}

//------------------------------------------------------------------------------

void Enum::ExitBlock() const
{
   Debug::ft("Enum.ExitBlock");

   for(auto e = etors_.cbegin(); e != etors_.cend(); ++e)
   {
      (*e)->ExitBlock();
   }

   Singleton<CxxSymbols>::Instance()->EraseEnum(this);
}

//------------------------------------------------------------------------------

Enumerator* Enum::FindEnumerator(const string& name) const
{
   Debug::ft("Enum.FindEnumerator");

   for(auto e = etors_.cbegin(); e != etors_.cend(); ++e)
   {
      if((*e)->Name() == name) return e->get();
   }

   return nullptr;
}

//------------------------------------------------------------------------------

void Enum::GetDecls(CxxNamedSet& items)
{
   items.insert(this);

   for(auto e = etors_.cbegin(); e != etors_.cend(); ++e)
   {
      (*e)->GetDecls(items);
   }
}

//------------------------------------------------------------------------------

bool Enum::GetSpan(size_t& begin, size_t& left, size_t& end) const
{
   Debug::ft("Enum.GetSpan");

   return GetBracedSpan(begin, left, end);
}

//------------------------------------------------------------------------------

TypeSpec* Enum::GetTypeSpec() const
{
   return (spec_ != nullptr ? spec_.get() : DataSpec::Int.get());
}

//------------------------------------------------------------------------------

void Enum::GetUsages(const CodeFile& file, CxxUsageSets& symbols)
{
   if(alignas_ != nullptr) alignas_->GetUsages(file, symbols);
   if(spec_ != nullptr) spec_->GetUsages(file, symbols);

   for(auto e = etors_.cbegin(); e != etors_.cend(); ++e)
   {
      (*e)->GetUsages(file, symbols);
   }
}

//------------------------------------------------------------------------------

bool Enum::IsUnused() const
{
   Debug::ft("Enum.IsUnused");

   if(refs_ > 0) return false;

   for(auto e = etors_.cbegin(); e != etors_.cend(); ++e)
   {
      if(!(*e)->IsUnused()) return false;
   }

   return true;
}

//------------------------------------------------------------------------------

CxxToken* Enum::PosToItem(size_t pos) const
{
   auto item = CxxScoped::PosToItem(pos);
   if(item != nullptr) return item;

   for(auto e = etors_.cbegin(); e != etors_.cend(); ++e)
   {
      item = (*e)->PosToItem(pos);
      if(item != nullptr) return item;
   }

   return nullptr;
}

//------------------------------------------------------------------------------

void Enum::Rename(const string& name)
{
   Debug::ft("Enum.Rename");

   CxxScoped::RenameNonQual(name_, name);
}

//------------------------------------------------------------------------------

void Enum::SetAlignment(AlignAsPtr& align)
{
   Debug::ft("Enum.SetAlignment");

   alignas_ = std::move(align);
}

//------------------------------------------------------------------------------

void Enum::SetAsReferent(const CxxNamed* user)
{
   Debug::ft("Enum.SetAsReferent");

   ++refs_;
   auto item = static_cast<Enum*>(FindTemplateAnalog(this));
   if(item != nullptr) ++item->refs_;
}

//------------------------------------------------------------------------------

string Enum::TypeString(bool arg) const
{
   return Prefix(GetScope()->TypeString(arg)) + name_;
}

//------------------------------------------------------------------------------

void Enum::UpdatePos
   (EditorAction action, size_t begin, size_t count, size_t from) const
{
   CxxScoped::UpdatePos(action, begin, count, from);

   for(auto e = etors_.cbegin(); e != etors_.cend(); ++e)
   {
      (*e)->UpdatePos(action, begin, count, from);
   }
}

//------------------------------------------------------------------------------

void Enum::UpdateXref(bool insert)
{
   if(alignas_ != nullptr) alignas_->UpdateXref(insert);
   if(spec_ != nullptr) spec_->UpdateXref(insert);

   for(auto e = etors_.cbegin(); e != etors_.cend(); ++e)
   {
      (*e)->UpdateXref(insert);
   }
}

//==============================================================================

Enumerator::Enumerator(string& name, ExprPtr& init, Enum* decl) :
   init_(init.release()),
   enum_(decl),
   refs_(0)
{
   Debug::ft("Enumerator.ctor");

   std::swap(name_, name);
   Singleton<CxxSymbols>::Instance()->InsertEtor(this);
}

//------------------------------------------------------------------------------

Enumerator::~Enumerator()
{
   Debug::ftnt("Enumerator.dtor");

   Singleton<CxxSymbols>::Extant()->EraseEtor(this);
}

//------------------------------------------------------------------------------

void Enumerator::Check() const
{
   Debug::ft("Enumerator.Check");

   if(init_ != nullptr) init_->Check();

   CheckIfUnused(EnumeratorUnused);
   CheckIfHiding();
}

//------------------------------------------------------------------------------

bool Enumerator::CheckIfUnused(Warning warning) const
{
   Debug::ft("Enumerator.CheckIfUnused");

   if(!IsUnused()) return false;
   if(enum_->IsUnused()) return false;
   Log(warning);
   return true;
}

//------------------------------------------------------------------------------

void Enumerator::Delete()
{
   Debug::ftnt("Enumerator.Delete");

   enum_->DeleteEnumerator(this);
}

//------------------------------------------------------------------------------

void Enumerator::Display
   (ostream& stream, const string& prefix, const Flags& options) const
{
   stream << prefix << Name();

   if(init_ != nullptr)
   {
      stream << " = ";
      init_->Print(stream, options);
   }

   if(!options.test(DispLast)) stream << ',';
   if(options.test(DispStats)) stream << " // r=" << refs_;
}

//------------------------------------------------------------------------------

fn_name Enumerator_EnterBlock = "Enumerator.EnterBlock";

void Enumerator::EnterBlock()
{
   Debug::ft(Enumerator_EnterBlock);

   Context::SetPos(GetLoc());

   if(init_ != nullptr)
   {
      //  When an enumerator's definition is compiled, the scope is the class
      //  itself.  Overwrite the class's access control with the enumerator's
      //  so a protected or private enumerator can use its class's non-public
      //  members without a log occurring when Class::AccessibilityOf invokes
      //  ScopeVisibility.
      //
      auto access = Context::SetAccess(GetAccess());
      init_->EnterBlock();
      Context::SetAccess(access);

      auto result = Context::PopArg(true);
      auto numeric = result.NumericType();

      if((numeric.Type() != Numeric::INT) && (numeric.Type() != Numeric::ENUM))
      {
         auto expl = "Non-numeric value for enumerator";
         Context::SwLog(Enumerator_EnterBlock, expl, numeric.Type());
      }
   }
}

//------------------------------------------------------------------------------

bool Enumerator::EnterScope()
{
   Debug::ft("Enumerator.EnterScope");

   EnterBlock();
   return true;
}

//------------------------------------------------------------------------------

void Enumerator::ExitBlock() const
{
   Debug::ft("Enumerator.ExitBlock");

   Singleton<CxxSymbols>::Instance()->EraseEtor(this);
}

//------------------------------------------------------------------------------

void Enumerator::GetDecls(CxxNamedSet& items)
{
   items.insert(this);
}

//------------------------------------------------------------------------------

void Enumerator::GetScopedNames(stringVector& names, bool templates) const
{
   Debug::ft("Enumerator.GetScopedNames");

   //  The superclass version provides the enumerator's fully qualified name,
   //  which includes that of its enum.  Then, unless the enum is anonymous,
   //  delete the enum's name from the fully qualified name, and provide that
   //  as an alternative.
   //
   CxxScoped::GetScopedNames(names, templates);
   auto prev = enum_->Name();
   if(prev.empty()) return;
   prev += SCOPE_STR;
   auto name = names.front();
   auto pos = name.rfind(prev);
   name.erase(pos, prev.size());
   names.push_back(name);
}

//------------------------------------------------------------------------------

bool Enumerator::GetSpan(size_t& begin, size_t& left, size_t& end) const
{
   Debug::ft("Enumerator.GetSpan");

   begin = GetPos();

   const auto& lexer = GetFile()->GetLexer();
   auto prev = lexer.RfindFirstOf(begin, ",{");
   auto next = lexer.FindFirstOf(",}", begin);

   if(lexer.At(next) == ',')
   {
      end = next;
   }
   else if(lexer.At(prev) == ',')
   {
      begin = prev;
      end = lexer.RfindNonBlank(next - 1);
   }
   else
   {
      begin = prev + 1;
      end = next - 1;
   }

   return true;
}

//------------------------------------------------------------------------------

void Enumerator::GetUsages(const CodeFile& file, CxxUsageSets& symbols)
{
   if(init_ != nullptr) init_->GetUsages(file, symbols);
}

//------------------------------------------------------------------------------

CxxToken* Enumerator::PosToItem(size_t pos) const
{
   auto item = CxxScoped::PosToItem(pos);
   if(item != nullptr) return item;

   return (init_ != nullptr ? init_->PosToItem(pos) : nullptr);
}

//------------------------------------------------------------------------------

void Enumerator::RecordAccess(Cxx::Access access) const
{
   Debug::ft("Enumerator.RecordAccess");

   CxxScoped::RecordAccess(access);
   enum_->RecordAccess(access);
}

//------------------------------------------------------------------------------

void Enumerator::Rename(const string& name)
{
   Debug::ft("Enumerator.Rename");

   CxxScoped::RenameNonQual(name_, name);
}

//------------------------------------------------------------------------------

string Enumerator::ScopedName(bool templates) const
{
   return Prefix(enum_->ScopedName(templates)) + Name();
}

//------------------------------------------------------------------------------

void Enumerator::SetAsReferent(const CxxNamed* user)
{
   Debug::ft("Enumerator.SetAsReferent");

   ++refs_;
   auto item = static_cast<Enumerator*>(FindTemplateAnalog(this));
   if(item != nullptr) ++item->refs_;
}

//------------------------------------------------------------------------------

string Enumerator::TypeString(bool arg) const
{
   auto ts = enum_->TypeString(arg);
   if(!arg) ts += SCOPE_STR + Name();
   return ts;
}

//------------------------------------------------------------------------------

void Enumerator::UpdatePos
   (EditorAction action, size_t begin, size_t count, size_t from) const
{
   CxxScoped::UpdatePos(action, begin, count, from);
   if(init_ != nullptr) init_->UpdatePos(action, begin, count, from);
}

//------------------------------------------------------------------------------

void Enumerator::UpdateXref(bool insert)
{
   if(init_ != nullptr) init_->UpdateXref(insert);
}

//------------------------------------------------------------------------------

bool Enumerator::WasRead()
{
   Debug::ft("Enumerator.WasRead");

   ++refs_;
   auto item = static_cast<Enumerator*>(FindTemplateAnalog(this));
   if(item != nullptr) ++item->refs_;
   return true;
}

//------------------------------------------------------------------------------

string Enumerator::XrefName(bool templates) const
{
   return Prefix(enum_->XrefName(templates), ".") + Name();
}

//==============================================================================

Forward::Forward(QualNamePtr& name, Cxx::ClassTag tag) :
   tag_(tag),
   name_(name.release()),
   users_(0)
{
   Debug::ft("Forward.ctor");

   Singleton<CxxSymbols>::Instance()->InsertForw(this);
}

//------------------------------------------------------------------------------

Forward::~Forward()
{
   Debug::ftnt("Forward.dtor");

   GetFile()->EraseForw(this);
   Singleton<CxxSymbols>::Extant()->EraseForw(this);
}

//------------------------------------------------------------------------------

CxxToken* Forward::AutoType() const
{
   auto ref = Referent();
   if(ref != nullptr) return ref;
   return (CxxToken*) this;
}

//------------------------------------------------------------------------------

void Forward::Check() const
{
   Debug::ft("Forward.Check");

   name_->Check();
   if(parms_ != nullptr) parms_->Check();

   if(Referent() == nullptr)
   {
      Log(ForwardUnresolved);
      return;
   }
}

//------------------------------------------------------------------------------

void Forward::Delete()
{
   Debug::ftnt("Forward.Delete");

   GetArea()->DeleteForw(this);
}

//------------------------------------------------------------------------------

void Forward::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   auto fq = options.test(DispFQ);
   stream << prefix;

   if(!options.test(DispNoTP))
   {
      if(parms_ != nullptr) parms_->Print(stream, options);
   }

   stream << tag_ << SPACE;
   strName(stream, fq, name_.get());
   stream << ';';

   if(!options.test(DispCode))
   {
      stream << " // ";
      if(options.test(DispStats)) stream << "u=" << users_ << SPACE;
      DisplayReferent(stream, fq);
   }
   stream << CRLF;
}

//------------------------------------------------------------------------------

void Forward::EnterBlock()
{
   Debug::ft("Forward.EnterBlock");

   Context::PushArg(StackArg(Referent(), 0, false, false));
}

//------------------------------------------------------------------------------

bool Forward::EnterScope()
{
   Debug::ft("Forward.EnterScope");

   Context::SetPos(GetLoc());
   if(IsAtFileScope()) GetFile()->InsertForw(this);
   if(parms_ != nullptr) parms_->EnterScope();
   return true;
}

//------------------------------------------------------------------------------

void Forward::GetDecls(CxxNamedSet& items)
{
   items.insert(this);
}

//------------------------------------------------------------------------------

void Forward::GetDirectClasses(CxxUsageSets& symbols)
{
   Debug::ft("Forward.GetDirectClasses");

   auto ref = Referent();
   if(ref != nullptr) symbols.AddDirect(ref);
}

//------------------------------------------------------------------------------

bool Forward::GetSpan(size_t& begin, size_t& left, size_t& end) const
{
   Debug::ft("Forward.GetSpan");

   return GetSemiSpan(begin, end);
}

//------------------------------------------------------------------------------

CxxToken* Forward::PosToItem(size_t pos) const
{
   auto item = CxxScoped::PosToItem(pos);
   if(item != nullptr) return item;

   item = name_->PosToItem(pos);
   if(item != nullptr) return item;

   return (parms_ != nullptr ? parms_->PosToItem(pos) : nullptr);
}

//------------------------------------------------------------------------------

CxxScoped* Forward::Referent() const
{
   Debug::ft("Forward.Referent");

   auto ref = name_->GetReferent();
   if(ref != nullptr) return ref;

   auto name = QualifiedName(false, true);
   ref = GetArea()->FindClass(name);
   name_->SetReferent(ref, nullptr);
   return ref;
}

//------------------------------------------------------------------------------

void Forward::Rename(const string& name)
{
   Debug::ft("Forward.Rename");

   CxxScoped::RenameQual(*name_, name);
}

//------------------------------------------------------------------------------

string Forward::ScopedName(bool templates) const
{
   auto ref = Referent();
   if(ref != nullptr) return ref->ScopedName(templates);
   return CxxNamed::ScopedName(templates);
}

//------------------------------------------------------------------------------

void Forward::SetAsReferent(const CxxNamed* user)
{
   Debug::ft("Forward.SetAsReferent");

   ++users_;
   auto item = static_cast<Forward*>(FindTemplateAnalog(this));
   if(item != nullptr) ++item->users_;
}

//------------------------------------------------------------------------------

void Forward::SetTemplateParms(TemplateParmsPtr& parms)
{
   Debug::ft("Forward.SetTemplateParms");

   parms_ = std::move(parms);
}

//------------------------------------------------------------------------------

string Forward::TypeString(bool arg) const
{
   auto ref = Referent();
   if(ref != nullptr) return ref->TypeString(arg);
   return Prefix(GetScope()->TypeString(arg)) + Name();
}

//------------------------------------------------------------------------------

void Forward::UpdatePos
   (EditorAction action, size_t begin, size_t count, size_t from) const
{
   CxxScoped::UpdatePos(action, begin, count, from);
   name_->UpdatePos(action, begin, count, from);
   if(parms_ != nullptr) parms_->UpdatePos(action, begin, count, from);
}

//------------------------------------------------------------------------------

void Forward::UpdateXref(bool insert)
{
   if(Referent() == nullptr) return;
   name_->UpdateXref(insert);
}

//==============================================================================
//
//  Used to prevent FindReferent from nesting too deeply.
//
static size_t Depth_ = 0;

//------------------------------------------------------------------------------

Friend::Friend() :
   inline_(nullptr),
   grantor_(nullptr),
   tag_(Cxx::Typename),
   using_(false),
   searching_(false),
   searched_(false),
   users_(0)
{
   Debug::ft("Friend.ctor");
}

//------------------------------------------------------------------------------

Friend::~Friend()
{
   Debug::ftnt("Friend.dtor");

   Singleton<CxxSymbols>::Extant()->EraseFriend(this);
}

//------------------------------------------------------------------------------

CxxToken* Friend::AutoType() const
{
   auto ref = Referent();
   if(ref != nullptr) return ref;
   return (CxxToken*) this;
}

//------------------------------------------------------------------------------

void Friend::Check() const
{
   Debug::ft("Friend.Check");

   name_->Check();
   if(parms_ != nullptr) parms_->Check();

   //  Log an unknown friend.
   //
   auto ref = GetReferent();

   if(ref == nullptr)
   {
      Log(FriendUnresolved);
      return;
   }

   //  Log an unused friend declaration (that is, one that did not access an
   //  item that would otherwise have been inaccessible) unless the grantor
   //  is also unused or the friend is an instance of an external template.
   //
   if(users_ == 0)
   {
      if(grantor_->CheckIfUnused(ClassUnused)) return;

      auto inst = ref->GetTemplateInstance();

      if(inst != nullptr)
      {
         if(inst->GetTemplate()->GetFile()->IsSubsFile()) return;
      }

      Log(FriendUnused);
   }
}

//------------------------------------------------------------------------------

void Friend::Delete()
{
   Debug::ftnt("Friend.Delete");

   static_cast<Class*>(grantor_)->DeleteFriend(this);
}

//------------------------------------------------------------------------------

void Friend::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   auto fq = options.test(DispFQ);
   stream << prefix;

   if(!options.test(DispNoTP))
   {
      if(parms_ != nullptr) parms_->Print(stream, options);
   }

   stream << FRIEND_STR << SPACE;

   auto func = GetFunction();

   if(func == nullptr)
   {
      if(tag_ != Cxx::Typename) stream << tag_ << SPACE;
      strName(stream, fq, name_.get());
   }
   else
   {
      func->DisplayDecl(stream, options);
   }

   stream << ';';

   if(!options.test(DispCode))
   {
      stream << " // ";
      if(options.test(DispStats)) stream << "u=" << users_ << SPACE;
      DisplayReferent(stream, fq);

      auto forw = name_->GetForward();
      if(forw != nullptr) stream << " via " << forw->GetFile()->Name();
   }

   stream << CRLF;
}

//------------------------------------------------------------------------------

bool Friend::EnterScope()
{
   Debug::ft("Friend.EnterScope");

   //  A friend declaration can also act as a forward declaration, so add it
   //  to the symbol table.  This was not done in the constructor because the
   //  friend's name was not yet known.  Look for what the friend refers to.
   //
   Context::SetPos(GetLoc());
   Singleton<CxxSymbols>::Instance()->InsertFriend(this);
   if(parms_ != nullptr) parms_->EnterScope();
   FindReferent();
   return true;
}

//------------------------------------------------------------------------------

fn_name Friend_FindForward = "Friend.FindForward";

CxxScoped* Friend::FindForward() const
{
   Debug::ft(Friend_FindForward);

   //  This is similar to ResolveName, except that the name's scope must be
   //  known.  On the other hand, its definition need not be visible in the
   //  scope where it appeared.
   //
   CxxScoped* item = GetScope();
   auto func = GetFunction();
   auto qname = GetQualName();
   auto size = qname->Size();
   auto name = qname->First()->Name();
   size_t idx = (item->Name() == name ? 1 : 0);
   Namespace* space;
   Class* cls;

   while(item != nullptr)
   {
      auto type = item->Type();

      switch(type)
      {
      case Cxx::Function:
         return item;

      case Cxx::Namespace:
         //
         //  If there is another name, resolve it within this namespace, else
         //  return the namespace itself.
         //
         if(idx >= size) return item;
         space = static_cast<Namespace*>(item);
         name = qname->At(idx)->Name();
         item = nullptr;
         if(++idx >= size)
         {
            if(func != nullptr) item = space->MatchFunc(func, false);
         }
         if(item == nullptr) item = space->FindItem(name);
         qname->SetReferentN(idx - 1, item, nullptr);
         if(item == nullptr) return nullptr;
         break;

      case Cxx::Class:
         cls = static_cast<Class*>(item);

         do
         {
            //  If this class was found through name resolution, see if it has
            //  template arguments before looking up the next name: if it does,
            //  create (but do not instantiate) the template instance.
            //
            if(idx == 0) break;
            if(cls->IsInTemplateInstance()) break;
            auto tname = qname->At(idx - 1)->GetTemplatedName();
            if(tname == nullptr) break;
            if(!ResolveTemplate(cls, tname, (idx >= size))) break;
            cls = cls->EnsureInstance(tname);
            item = cls;
            qname->SetReferentN(idx - 1, item, nullptr);  // updated value
            if(item == nullptr) return nullptr;
         }
         while(false);

         //  Resolve the next name within CLS.  This is similar to the above,
         //  when TYPE is a namespace.
         //
         if(idx >= size) return item;
         name = qname->At(idx)->Name();
         item = nullptr;
         if(++idx >= size)
         {
            if(func != nullptr) item = cls->MatchFunc(func, true);
         }
         if(item == nullptr) item = cls->FindMember(name, true);
         qname->SetReferentN(idx - 1, item, nullptr);
         if(item == nullptr) return nullptr;
         break;

      case Cxx::Typedef:
      {
         //  See if the item wants to resolve the typedef.
         //
         auto tdef = static_cast<Typedef*>(item);
         tdef->SetAsReferent(this);
         if(!ResolveTypedef(tdef, idx - 1)) return tdef;
         auto root = tdef->Root();
         if(root == nullptr) return tdef;
         item = static_cast<CxxScoped*>(root);
         qname->SetReferentN(idx - 1, item, nullptr);  // updated value
         break;
      }

      default:
         auto expl = name + " is an invalid friend";
         Context::SwLog(Friend_FindForward, expl, type);
         return nullptr;
      }
   }

   return item;
}

//------------------------------------------------------------------------------

void Friend::FindReferent()
{
   Debug::ft("Friend.FindReferent");

   //  The following prevents a stack overflow.  The declaration itself can be
   //  found as a candidate when ResolveName is invoked.  To find what it refers
   //  to, it is asked for its scoped name, which looks for its referent, which
   //  causes this function to be reinvoked.  A depth limit of 2 is also
   //  enforced over all friend declarations.  This allows another friend
   //  declaration to find its referent and provide its resolution to this one.
   //  However, it prevents futile nesting in which declarations ask each other
   //  for a referent that is still only a forward declaration.
   //    Even if the referent is not found, SetReferent(nullptr) must be invoked
   //  to reset searching_ and Depth_, which reenables this function.
   //
   if(searching_ || (Depth_ > 1)) return;
   searching_ = true;
   ++Depth_;

   SymbolView view = DeclaredGlobally;
   const auto& mask =
      (GetFunction() != nullptr ? FRIEND_FUNCS : FRIEND_CLASSES);
   CxxScoped* ref = nullptr;

   if(!searched_)
   {
      //  This is the initial search for the friend's referent, so the scope
      //  is the class where the friend declaration appeared.  Search for the
      //  friend from that scope, but update the scope to the namespace where
      //  the class is defined, because that will be the friend's scope if it
      //  has not yet been declared.
      //
      searched_ = true;
      grantor_ = GetScope();
      SetScope(grantor_->GetSpace());
      ref = ResolveName(GetFile(), grantor_, mask, view);
      if(ref != nullptr) using_ = view.using_;
   }

   //  Keep searching for the friend if the initial search failed or previous
   //  searches have only returned an unresolved forward declaration.
   //
   auto forw = name_->GetForward();
   if((ref == nullptr) || (ref == this) || (ref == forw)) ref = FindForward();
   SetReferent(ref, nullptr);
}

//------------------------------------------------------------------------------

void Friend::GetDecls(CxxNamedSet& items)
{
   items.insert(this);
}

//------------------------------------------------------------------------------

void Friend::GetDirectClasses(CxxUsageSets& symbols)
{
   Debug::ft("Friend.GetDirectClasses");

   auto ref = Referent();
   if(ref != nullptr) symbols.AddDirect(ref);
}

//------------------------------------------------------------------------------

Function* Friend::GetFunction() const
{
   if(func_ != nullptr) return func_.get();
   if(inline_ != nullptr) return inline_;
   return nullptr;
}

//------------------------------------------------------------------------------

QualName* Friend::GetQualName() const
{
   auto func = GetFunction();
   if(func != nullptr) return func->GetQualName();
   return name_.get();
}

//------------------------------------------------------------------------------

CxxScoped* Friend::GetReferent() const
{
   return GetQualName()->GetReferent();
}

//------------------------------------------------------------------------------

bool Friend::GetSpan(size_t& begin, size_t& left, size_t& end) const
{
   Debug::ft("Friend.GetSpan");

   return GetSemiSpan(begin, end);
}

//------------------------------------------------------------------------------

void Friend::GetUsages(const CodeFile& file, CxxUsageSets& symbols)
{
   auto ref = Referent();
   if(ref == nullptr) return;

   //  If the friend is a class template or class template instance, it must
   //  be visible as a forward declaration, although the friend declaration
   //  itself could have doubled as that forward declaration.
   //
   auto tmplt = ((ref->IsTemplate()) || (ref->GetTemplatedName() != nullptr));
   auto forw = name_->GetForward();
   auto type = (forw != nullptr ? forw->Type() : ref->Type());

   switch(type)
   {
   case Cxx::Forward:
   case Cxx::Friend:
      if(tmplt) symbols.AddForward(forw);
      break;

   case Cxx::Class:
   {
      //  o The outer class for an inner one must be directly visible.
      //  o The class template for a class template instance should
      //    (must, if in another namespace) be declared forward.
      //
      auto outer = ref->Declarer();

      if(outer != nullptr)
      {
         if(outer->GetTemplate() != nullptr)
            outer = outer->GetClassTemplate();
         symbols.AddDirect(outer);
      }
      else if(ref->IsTemplate())
      {
         symbols.AddIndirect(ref);
      }
      else if(ref->GetTemplatedName() != nullptr)
      {
         symbols.AddIndirect(ref->GetTemplate());
      }
      break;
   }

   case Cxx::Function:
      //
      //  o An inline friend has no visibility requirements.
      //  o A function's class must be directly visible.
      //  o A function outside a class should be declared forward.
      //
      if(inline_ != nullptr) break;

      if(ref->GetClass() != nullptr)
         symbols.AddDirect(ref->GetClass());
      else
         symbols.AddIndirect(ref);
      break;
   }

   //  Indicate whether our referent was made visible by a using statement.
   //
   if(using_) symbols.AddUser(this);
}

//------------------------------------------------------------------------------

void Friend::IncrUsers()
{
   Debug::ft("Friend.IncrUsers");

   ++users_;
   auto item = static_cast<Friend*>(grantor_->FindTemplateAnalog(this));
   if(item != nullptr) ++item->users_;
}

//------------------------------------------------------------------------------

const string& Friend::Name() const
{
   auto func = GetFunction();
   if(func != nullptr) return func->Name();
   return name_->Name();
}

//------------------------------------------------------------------------------

CxxToken* Friend::PosToItem(size_t pos) const
{
   auto item = CxxScoped::PosToItem(pos);
   if(item != nullptr) return item;

   if(name_ != nullptr) item = name_->PosToItem(pos);
   if(item != nullptr) return item;

   if(parms_ != nullptr) item = parms_->PosToItem(pos);
   if(item != nullptr) return item;

   return (func_ != nullptr ? func_->PosToItem(pos) : nullptr);
}

//------------------------------------------------------------------------------

string Friend::QualifiedName(bool scopes, bool templates) const
{
   auto func = GetFunction();
   if(func != nullptr) return func->QualifiedName(scopes, templates);
   return name_->QualifiedName(scopes, templates);
}

//------------------------------------------------------------------------------

CxxScoped* Friend::Referent() const
{
   Debug::ft("Friend.Referent");

   auto ref = GetReferent();
   if(ref != nullptr) return ref;
   const_cast<Friend*>(this)->FindReferent();
   return GetReferent();
}

//------------------------------------------------------------------------------

void Friend::Rename(const string& name)
{
   Debug::ft("Friend.Rename");

   CxxScoped::RenameQual(*name_, name);
}

//------------------------------------------------------------------------------

bool Friend::ResolveForward(CxxScoped* decl, size_t n) const
{
   Debug::ft("Friend.ResolveForward");

   //  A forward declaration for the friend was found.  Unless it is
   //  the friend declaration itself, save it, along with its scope,
   //  and continue resolving the name.
   //
   if(decl == this) return false;
   name_->At(n)->SetForward(decl);
   decl->SetAsReferent(this);
   const_cast<Friend*>(this)->SetScope(decl->GetSpace());
   return true;
}

//------------------------------------------------------------------------------

bool Friend::ResolveTemplate(Class* cls, const TypeName* type, bool end) const
{
   Debug::ft("Friend.ResolveTemplate");

   auto scope = cls->GetScope();
   const_cast<Friend*>(this)->SetScope(scope);

   //  Class::AccessbilityTo invokes Class::FindFriend to determine if a friend
   //  declaration did anything useful, even if it wasn't needed to access a
   //  member.  If a friend is a class template instance, this can cause this
   //  code to be invoked to find the friend's referent.  If the referent for
   //  a template parameter is unknown, it would cause Class::EnsureInstance
   //  to create a class instance name with unqualified template parameters,
   //  and so we prevent this.
   //
   return type->VerifyReferents();
}

//------------------------------------------------------------------------------

string Friend::ScopedName(bool templates) const
{
   auto ref = Referent();
   if(ref != nullptr) return ref->ScopedName(templates);
   return CxxNamed::ScopedName(templates);
}

//------------------------------------------------------------------------------

void Friend::SetAsReferent(const CxxNamed* user)
{
   Debug::ft("Friend.SetAsReferent");

   //  Don't log this for another friend or forward declaration.
   //
   if(user->IsForward()) return;

   //  Provide a string that specifies the forward declaration that
   //  is equivalent to the friend declaration.
   //
   std::ostringstream name;
   if(parms_ != nullptr) parms_->Print(name, NoFlags);
   name << tag_ << SPACE;
   name << ScopedName(true);
   user->Log(FriendAsForward, nullptr, 0, name.str());
}

//------------------------------------------------------------------------------

void Friend::SetFunc(FunctionPtr& func)
{
   Debug::ft("Friend.SetFunc");

   func->CloseScope();

   if(func->GetBracePos() != string::npos)
   {
      //  This is a friend definition (an inline friend function).  Such
      //  a function belongs to the same scope that defined the class in
      //  which the friend appeared.
      //
      auto cls = func->GetClass();
      auto scope = cls->GetScope();
      SetScope(scope);
      inline_ = func.get();
      inline_->SetTemplateParms(parms_);
      inline_->SetFriend();
      static_cast<CxxArea*>(scope)->AddFunc(func);
      GetQualName()->SetReferent(inline_, nullptr);
      inline_->SetAsReferent(this);
   }
   else
   {
      func_ = std::move(func);
   }
}

//------------------------------------------------------------------------------

void Friend::SetName(QualNamePtr& name)
{
   Debug::ft("Friend.SetName");

   name_ = std::move(name);
}

//------------------------------------------------------------------------------

fn_name Friend_SetReferent = "Friend.SetReferent";

void Friend::SetReferent(CxxScoped* item, const SymbolView* view) const
{
   Debug::ft(Friend_SetReferent);

   searching_ = false;
   --Depth_;

   if(item == nullptr) return;

   auto type = item->Type();

   switch(type)
   {
   case Cxx::Class:
   case Cxx::Function:
      break;
   default:
      auto expl = item->ScopedName(true) + " is an invalid friend";
      Context::SwLog(Friend_SetReferent, expl, type);
      return;
   }

   item->SetAsReferent(this);
   GetQualName()->SetReferent(item, view);
}

//------------------------------------------------------------------------------

void Friend::SetTemplateParms(TemplateParmsPtr& parms)
{
   Debug::ft("Friend.SetTemplateParms");

   parms_ = std::move(parms);
}

//------------------------------------------------------------------------------

string Friend::TypeString(bool arg) const
{
   auto ref = Referent();
   if(ref != nullptr) return ref->TypeString(arg);

   auto func = GetFunction();
   if(func != nullptr) return func->TypeString(arg);
   return Prefix(GetScope()->TypeString(arg)) + QualifiedName(false, true);
}

//------------------------------------------------------------------------------

void Friend::UpdatePos
   (EditorAction action, size_t begin, size_t count, size_t from) const
{
   CxxScoped::UpdatePos(action, begin, count, from);
   if(name_ != nullptr) name_->UpdatePos(action, begin, count, from);
   if(parms_ != nullptr) parms_->UpdatePos(action, begin, count, from);
   if(func_ != nullptr) func_->UpdatePos(action, begin, count, from);
}

//------------------------------------------------------------------------------

void Friend::UpdateXref(bool insert)
{
   if(Referent() == nullptr) return;
   name_->UpdateXref(insert);
}

//==============================================================================

MemberInit::MemberInit(const Function* ctor, string& name, TokenPtr& init) :
   ctor_(ctor),
   ref_(nullptr),
   init_(init.release())
{
   Debug::ft("MemberInit.ctor");

   std::swap(name_, name);
}

//------------------------------------------------------------------------------

MemberInit::~MemberInit()
{
   Debug::ft("MemberInit.dtor");

   if(ref_ != nullptr) ref_->UpdateReference(this, false);
}

//------------------------------------------------------------------------------

void MemberInit::Check() const
{
   init_->Check();
}

//------------------------------------------------------------------------------

void MemberInit::Delete()
{
   Debug::ft("MemberInit.Delete");

   auto func = static_cast<Function*>(GetScope());
   if(func != nullptr) func->DeleteMemberInit(this);
}

//------------------------------------------------------------------------------

fn_name MemberInit_EnterBlock = "MemberInit.EnterBlock";

void MemberInit::EnterBlock()
{
   Debug::ft(MemberInit_EnterBlock);

   Context::SetPos(GetLoc());

   if(Referent() != nullptr)
   {
      ref_->SetMemInit(this);
   }
   else
   {
      string expl("Failed to find member ");
      expl += ctor_->GetClass()->Name() + SCOPE_STR + name_;
      Context::SwLog(MemberInit_EnterBlock, expl, 0);
   }
}

//------------------------------------------------------------------------------

bool MemberInit::GetSpan(size_t& begin, size_t& left, size_t& end) const
{
   Debug::ft("MemberInit.GetSpan");

   begin = GetPos();

   const auto& lexer = GetFile()->GetLexer();
   auto prev = lexer.RfindFirstOf(begin, ",:");
   auto next = lexer.FindFirstOf(",{", begin);

   if(lexer.At(next) == ',')
   {
      end = next;
   }
   else
   {
      begin = prev;
      end = lexer.RfindNonBlank(next - 1);
   }

   return true;
}

//------------------------------------------------------------------------------

void MemberInit::GetUsages(const CodeFile& file, CxxUsageSets& symbols)
{
   init_->GetUsages(file, symbols);
}

//------------------------------------------------------------------------------

void MemberInit::ItemDeleted(const CxxScoped* item) const
{
   if(ref_ == item)
   {
      ref_ = nullptr;
   }
}

//------------------------------------------------------------------------------

CxxToken* MemberInit::PosToItem(size_t pos) const
{
   auto item = CxxScoped::PosToItem(pos);
   if(item != nullptr) return item;

   return init_->PosToItem(pos);
}

//------------------------------------------------------------------------------

void MemberInit::Print(ostream& stream, const Flags& options) const
{
   stream << name_;
   init_->Print(stream, options);
}

//------------------------------------------------------------------------------

CxxScoped* MemberInit::Referent() const
{
   if(ref_ != nullptr) return ref_;

   auto cls = ctor_->GetClass();
   ref_ = static_cast<ClassData*>(cls->FindData(name_));
   return ref_;
}

//------------------------------------------------------------------------------

void MemberInit::Rename(const string& name)
{
   Debug::ft("MemberInit.Rename");

   CxxScoped::RenameNonQual(name_, name);
}

//------------------------------------------------------------------------------

void MemberInit::UpdatePos
   (EditorAction action, size_t begin, size_t count, size_t from) const
{
   CxxScoped::UpdatePos(action, begin, count, from);
   init_->UpdatePos(action, begin, count, from);
}

//------------------------------------------------------------------------------

void MemberInit::UpdateXref(bool insert)
{
   if(ref_ != nullptr) ref_->UpdateReference(this, insert);
   init_->UpdateXref(insert);
}

//==============================================================================

TemplateParm::TemplateParm(string& name, Cxx::ClassTag tag,
   QualNamePtr& type, size_t ptrs, TemplateArgPtr& preset) :
   tag_(tag),
   type_(std::move(type)),
   ptrs_(ptrs),
   default_(std::move(preset))
{
   Debug::ft("TemplateParm.ctor");

   std::swap(name_, name);
}

//------------------------------------------------------------------------------

TemplateParm::~TemplateParm()
{
   Debug::ftnt("TemplateParm.dtor");
}

//------------------------------------------------------------------------------

CxxToken* TemplateParm::AutoType() const
{
   if(default_ != nullptr)
   {
      auto ref = default_->Referent();
      if(ref != nullptr) return ref;
   }

   return (CxxToken*) this;
}

//------------------------------------------------------------------------------

void TemplateParm::Check() const
{
   Debug::ft("TemplateParm.Check");

   if(type_ != nullptr) type_->Check();
   if(default_ != nullptr) default_->Check();
}

//------------------------------------------------------------------------------

void TemplateParm::EnterBlock()
{
   Debug::ft("TemplateParm.EnterBlock");

   Context::SetPos(GetLoc());
   Context::InsertLocal(this);
}

//------------------------------------------------------------------------------

bool TemplateParm::EnterScope()
{
   Debug::ft("TemplateParm.EnterScope");

   Context::SetPos(GetLoc());
   Context::InsertLocal(this);
   if(default_ != nullptr) default_->EnteringScope(Context::Scope());
   return true;
}

//------------------------------------------------------------------------------

void TemplateParm::ExitBlock() const
{
   Debug::ft("TemplateParm.ExitBlock");

   Context::EraseLocal(this);
}

//------------------------------------------------------------------------------

void TemplateParm::GetUsages(const CodeFile& file, CxxUsageSets& symbols)
{
   if(type_ != nullptr) type_->GetUsages(file, symbols);
   if(default_ != nullptr) default_->GetUsages(file, symbols);
}

//------------------------------------------------------------------------------

CxxToken* TemplateParm::PosToItem(size_t pos) const
{
   auto item = CxxScoped::PosToItem(pos);
   if(item != nullptr) return item;

   if(type_ != nullptr) item = type_->PosToItem(pos);
   if(item != nullptr) return item;

   return (default_ != nullptr ? default_->PosToItem(pos) : nullptr);
}

//------------------------------------------------------------------------------

void TemplateParm::Print(ostream& stream, const Flags& options) const
{
   if(tag_ != Cxx::ClassTag_N)
      stream << tag_;
   else
      type_->Print(stream, options);

   stream << SPACE << Name();
   if(ptrs_ > 0) stream << string(ptrs_, '*');

   if(default_ != nullptr)
   {
      stream << " = ";
      default_->Print(stream, options);
   }
}

//------------------------------------------------------------------------------

CxxScoped* TemplateParm::Referent() const
{
   if(default_ != nullptr)
   {
      auto ref = default_->Referent();
      if(ref != nullptr) return ref;
   }

   return (CxxScoped*) this;
}

//------------------------------------------------------------------------------

CxxToken* TemplateParm::RootType() const
{
   if(default_ != nullptr)
   {
      auto ref = default_->Referent();
      if(ref != nullptr) return ref;
   }

   return (CxxToken*) this;
}

//------------------------------------------------------------------------------

string TemplateParm::TypeString(bool arg) const
{
   if(tag_ != Cxx::ClassTag_N)
   {
      auto ts = Name();
      if(ptrs_ > 0) ts += string(ptrs_, '*');
      return ts;
   }

   return type_->TypeString(arg);
}

//------------------------------------------------------------------------------

void TemplateParm::UpdatePos
   (EditorAction action, size_t begin, size_t count, size_t from) const
{
   CxxScoped::UpdatePos(action, begin, count, from);
   if(type_ != nullptr) type_->UpdatePos(action, begin, count, from);
   if(default_ != nullptr) default_->UpdatePos(action, begin, count, from);
}

//------------------------------------------------------------------------------

void TemplateParm::UpdateXref(bool insert)
{
   if(type_ != nullptr) type_->UpdateXref(insert);
   if(default_ != nullptr) default_->UpdateXref(insert);
}

//==============================================================================

TemplateParms::TemplateParms(TemplateParmPtr& parm)
{
   Debug::ft("TemplateParms.ctor");

   parms_.push_back(std::move(parm));
}

//------------------------------------------------------------------------------

TemplateParms::~TemplateParms()
{
   Debug::ftnt("TemplateParms.dtor");
}

//------------------------------------------------------------------------------

void TemplateParms::AddParm(TemplateParmPtr& parm)
{
   Debug::ft("TemplateParms.AddParm");

   parms_.push_back(std::move(parm));
}

//------------------------------------------------------------------------------

void TemplateParms::Check() const
{
   for(auto p = parms_.cbegin(); p != parms_.cend(); ++p)
   {
      (*p)->Check();
   }
}

//------------------------------------------------------------------------------

void TemplateParms::EnterBlock()
{
   Debug::ft("TemplateParms.EnterBlock");

   for(auto p = parms_.cbegin(); p != parms_.cend(); ++p)
   {
      (*p)->EnterBlock();
   }
}

//------------------------------------------------------------------------------

bool TemplateParms::EnterScope()
{
   Debug::ft("TemplateParms.EnterScope");

   for(auto p = parms_.cbegin(); p != parms_.cend(); ++p)
   {
      (*p)->EnterScope();
   }

   //  Each template parameter added itself as a local so that it could
   //  be resolved if used to specify a default value for a subsequent
   //  template parameter.  Thus this hack to erase those locals...
   //
   ExitBlock();
   return true;
}

//------------------------------------------------------------------------------

void TemplateParms::ExitBlock() const
{
   Debug::ft("TemplateParms.ExitBlock");

   for(auto p = parms_.cbegin(); p != parms_.cend(); ++p)
   {
      (*p)->ExitBlock();
   }
}

//------------------------------------------------------------------------------

void TemplateParms::GetUsages(const CodeFile& file, CxxUsageSets& symbols)
{
   for(auto p = parms_.cbegin(); p != parms_.cend(); ++p)
   {
      (*p)->GetUsages(file, symbols);
   }
}

//------------------------------------------------------------------------------

CxxToken* TemplateParms::PosToItem(size_t pos) const
{
   auto item = CxxToken::PosToItem(pos);
   if(item != nullptr) return item;

   for(auto p = parms_.cbegin(); p != parms_.cend(); ++p)
   {
      item = (*p)->PosToItem(pos);
      if(item != nullptr) return item;
   }

   return nullptr;
}

//------------------------------------------------------------------------------

void TemplateParms::Print(ostream& stream, const Flags& options) const
{
   stream << TEMPLATE_STR << '<';

   for(auto p = parms_.cbegin(); p != parms_.cend(); ++p)
   {
      (*p)->Print(stream, options);
      if(*p != parms_.back()) stream << ", ";
   }

   stream << "> ";
}

//------------------------------------------------------------------------------

string TemplateParms::TypeString(bool arg) const
{
   string ts = "<";

   for(auto p = parms_.cbegin(); p != parms_.cend(); ++p)
   {
      ts += (*p)->TypeString(false);
      if(*p != parms_.back()) ts += ',';
   }

   return ts + '>';
}

//------------------------------------------------------------------------------

void TemplateParms::UpdatePos
   (EditorAction action, size_t begin, size_t count, size_t from) const
{
   CxxToken::UpdatePos(action, begin, count, from);

   for(auto p = parms_.cbegin(); p != parms_.cend(); ++p)
   {
      (*p)->UpdatePos(action, begin, count, from);
   }
}

//------------------------------------------------------------------------------

void TemplateParms::UpdateXref(bool insert)
{
   for(auto p = parms_.cbegin(); p != parms_.cend(); ++p)
   {
      (*p)->UpdateXref(insert);
   }
}

//==============================================================================

Terminal::Terminal(const string& name, const string& type) :
   name_(name),
   type_(type.empty() ? name : type),
   attrs_(Numeric::Nil)
{
   Debug::ft("Terminal.ctor");

   SetScope(Singleton<CxxRoot>::Instance()->GlobalNamespace());
   Singleton<CxxSymbols>::Instance()->InsertTerm(this);
}

//------------------------------------------------------------------------------

Terminal::~Terminal()
{
   Debug::ftnt("Terminal.dtor");

   Singleton<CxxSymbols>::Extant()->EraseTerm(this);
}

//------------------------------------------------------------------------------

void Terminal::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   stream << prefix << "terminal " << Name();
   stream << ';';

   if(!options.test(DispFQ))
   {
      stream << " // ";
      DisplayFiles(stream);
   }

   stream << CRLF;
}

//------------------------------------------------------------------------------

void Terminal::EnterBlock()
{
   Debug::ft("Terminal.EnterBlock");

   Context::PushArg(StackArg(this, 0, false, false));
}

//------------------------------------------------------------------------------

bool Terminal::IsAuto() const
{
   Debug::ft("Terminal.IsAuto");

   return (Name() == AUTO_STR);
}

//------------------------------------------------------------------------------

bool Terminal::NameRefersToItem(const string& name,
   const CxxScope* scope, CodeFile* file, SymbolView& view) const
{
   Debug::ft("Terminal.NameRefersToItem");

   view = DeclaredGlobally;
   return true;
}

//==============================================================================

Typedef::Typedef(string& name, TypeSpecPtr& spec) :
   spec_(spec.release()),
   using_(false),
   refs_(0)
{
   Debug::ft("Typedef.ctor");

   std::swap(name_, name);
   Singleton<CxxSymbols>::Instance()->InsertType(this);
}

//------------------------------------------------------------------------------

Typedef::~Typedef()
{
   Debug::ftnt("Typedef.dtor");

   GetFile()->EraseType(this);
   Singleton<CxxSymbols>::Extant()->EraseType(this);
}

//------------------------------------------------------------------------------

const TemplateArgPtrVector* Typedef::Args() const
{
   return spec_->Args();
}

//------------------------------------------------------------------------------

string Typedef::ArgString(const TemplateParmToArgMap& tmap) const
{
   return spec_->ArgString(tmap);
}

//------------------------------------------------------------------------------

void Typedef::Check() const
{
   Debug::ft("Typedef.Check");

   spec_->Check();
   if(alignas_ != nullptr) alignas_->Check();

   CheckIfUnused(TypedefUnused);
   CheckIfHiding();
   CheckAccessControl();
   CheckPointerType();
}

//------------------------------------------------------------------------------

void Typedef::CheckPointerType() const
{
   Debug::ft("Typedef.CheckPointerType");

   if(spec_->Ptrs(false) > 0) Log(PointerTypedef);
}

//------------------------------------------------------------------------------

void Typedef::Delete()
{
   Debug::ftnt("Typedef.Delete");

   GetArea()->DeleteType(this);
}

//------------------------------------------------------------------------------

void Typedef::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   if(IsDeclaredInFunction())
   {
      stream << prefix;
      Print(stream, options);
      stream << CRLF;
      return;
   }

   auto fq = options.test(DispFQ);
   stream << prefix;
   if(GetScope()->Type() == Cxx::Class) stream << GetAccess() << ": ";

   if(using_)
   {
      stream << USING_STR << SPACE;
      stream << (fq ? ScopedName(true) : Name()) << SPACE;

      if(alignas_ != nullptr)
      {
         alignas_->Print(stream, options);
         stream << SPACE;
      }

      stream << "= ";
      spec_->Print(stream, options);
      spec_->DisplayArrays(stream);
   }
   else
   {
      stream << TYPEDEF_STR << SPACE;
      spec_->Print(stream, options);

      if(spec_->GetFuncSpec() == nullptr)
      {
         stream << SPACE << (fq ? ScopedName(true) : Name());
      }

      spec_->DisplayArrays(stream);

      if(alignas_ != nullptr)
      {
         stream << SPACE;
         alignas_->Print(stream, options);
      }
   }

   stream << ';';

   if(!options.test(DispCode))
   {
      std::ostringstream buff;
      buff << " // ";
      if(options.test(DispStats)) buff << "r=" << refs_ << SPACE;
      if(!fq) DisplayFiles(buff);
      auto str = buff.str();
      if(str.size() > 4) stream << str;
   }

   stream << CRLF;
}

//------------------------------------------------------------------------------

void Typedef::EnterBlock()
{
   Debug::ft("Typedef.EnterBlock");

   Context::SetPos(GetLoc());
   spec_->EnteringScope(GetScope());
   if(alignas_ != nullptr) alignas_->EnterBlock();
   refs_ = 0;
}

//------------------------------------------------------------------------------

bool Typedef::EnterScope()
{
   Debug::ft("Typedef.EnterScope");

   //  When a typedef in a class is compiled, the scope is the class itself.
   //  Overwrite the class's access control with the typedef's so a protected
   //  or private typedef can use its class's non-public members without a log
   //  occurring when Class::AccessibilityOf invokes ScopeVisibility.
   //
   Context::SetPos(GetLoc());
   Context::Enter(this);
   if(IsAtFileScope()) GetFile()->InsertType(this);

   auto access = Context::SetAccess(GetAccess());
   spec_->EnteringScope(GetScope());
   Context::SetAccess(access);

   refs_ = 0;
   return true;
}

//------------------------------------------------------------------------------

void Typedef::ExitBlock() const
{
   Debug::ft("Typedef.ExitBlock");

   Singleton<CxxSymbols>::Instance()->EraseType(this);
}

//------------------------------------------------------------------------------

void Typedef::GetDecls(CxxNamedSet& items)
{
   items.insert(this);
}

//------------------------------------------------------------------------------

bool Typedef::GetSpan(size_t& begin, size_t& left, size_t& end) const
{
   Debug::ft("Typedef.GetSpan");

   return GetSemiSpan(begin, end);
}

//------------------------------------------------------------------------------

TypeName* Typedef::GetTemplatedName() const
{
   return spec_->GetTemplatedName();
}

//------------------------------------------------------------------------------

void Typedef::GetUsages(const CodeFile& file, CxxUsageSets& symbols)
{
   spec_->GetUsages(file, symbols);
   if(alignas_ != nullptr) alignas_->GetUsages(file, symbols);
}

//------------------------------------------------------------------------------

void Typedef::Instantiating(CxxScopedVector& locals) const
{
   spec_->Instantiating(locals);
}

//------------------------------------------------------------------------------

bool Typedef::ItemIsTemplateArg(const CxxNamed* item) const
{
   Debug::ft("Typedef.ItemIsTemplateArg");

   return spec_->ItemIsTemplateArg(item);
}

//------------------------------------------------------------------------------

bool Typedef::NamesReferToArgs(const NameVector& names,
   const CxxScope* scope, CodeFile* file, size_t& index) const
{
   Debug::ft("Typedef.NamesReferToArgs");

   return spec_->NamesReferToArgs(names, scope, file, index);
}

//------------------------------------------------------------------------------

CxxToken* Typedef::PosToItem(size_t pos) const
{
   auto item = CxxScoped::PosToItem(pos);
   if(item != nullptr) return item;

   return spec_->PosToItem(pos);
}

//------------------------------------------------------------------------------

void Typedef::Print(ostream& stream, const Flags& options) const
{
   if(using_)
   {
      stream << USING_STR << SPACE << Name() << " = ";
      spec_->Print(stream, options);
   }
   else
   {
      stream << TYPEDEF_STR << SPACE;
      spec_->Print(stream, options);
      if(spec_->GetFuncSpec() == nullptr) stream << SPACE << Name();
   }

   spec_->DisplayArrays(stream);
   stream << ';';
}

//------------------------------------------------------------------------------

CxxScoped* Typedef::Referent() const
{
   Debug::ft("Typedef.Referent");

   return spec_->Referent();
}

//------------------------------------------------------------------------------

void Typedef::Rename(const string& name)
{
   Debug::ft("Typedef.Rename");

   CxxScoped::RenameNonQual(name_, name);
}

//------------------------------------------------------------------------------

void Typedef::SetAlignment(AlignAsPtr& align)
{
   Debug::ft("Typedef.SetAlignment");

   alignas_ = std::move(align);
}

//------------------------------------------------------------------------------

void Typedef::SetAsReferent(const CxxNamed* user)
{
   Debug::ft("Typedef.SetAsReferent");

   ++refs_;
   auto item = static_cast<Typedef*>(FindTemplateAnalog(this));
   if(item != nullptr) ++item->refs_;
}

//------------------------------------------------------------------------------

string Typedef::TypeString(bool arg) const
{
   return spec_->TypeString(arg);
}

//------------------------------------------------------------------------------

void Typedef::UpdatePos
   (EditorAction action, size_t begin, size_t count, size_t from) const
{
   CxxScoped::UpdatePos(action, begin, count, from);
   spec_->UpdatePos(action, begin, count, from);
}

//------------------------------------------------------------------------------

void Typedef::UpdateXref(bool insert)
{
   spec_->UpdateXref(insert);
   if(alignas_ != nullptr) alignas_->UpdateXref(insert);
}

//------------------------------------------------------------------------------

bool Typedef::VerifyReferents() const
{
   return spec_->VerifyReferents();
}

//==============================================================================

Using::Using(QualNamePtr& name, bool space, bool added) :
   name_(name.release()),
   users_(0),
   added_(added),
   remove_(false),
   space_(space)
{
   Debug::ft("Using.ctor");
}

//------------------------------------------------------------------------------

Using::~Using()
{
   Debug::ft("Using.dtor");

   GetFile()->EraseUsing(this);
}

//------------------------------------------------------------------------------

void Using::Check() const
{
   Debug::ft("Using.Check");

   if(added_) return;

   name_->Check();

   //  A using statement should be avoided in a header except to import
   //  items from a base class.
   //
   if(GetFile()->IsHeader() && (GetScope()->Type() != Cxx::Class))
   {
      Log(UsingInHeader);
   }
}

//------------------------------------------------------------------------------

void Using::Delete()
{
   Debug::ft("Using.Delete");

   GetArea()->DeleteUsing(this);
}

//------------------------------------------------------------------------------

void Using::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   if(added_) return;

   auto fq = options.test(DispFQ);
   stream << prefix << USING_STR << SPACE;
   if(space_) stream << NAMESPACE_STR << SPACE;
   strName(stream, fq, name_.get());
   stream << ';';

   if(!options.test(DispCode))
   {
      stream << " // ";
      if(options.test(DispStats)) stream << "u=" << users_ << SPACE;
      DisplayReferent(stream, fq);
   }

   stream << CRLF;
}

//------------------------------------------------------------------------------

void Using::EnterBlock()
{
   Debug::ft("Using.EnterBlock");

   Context::SetPos(GetLoc());
   Block::AddUsing(this);
   FindReferent();
}

//------------------------------------------------------------------------------

bool Using::EnterScope()
{
   Debug::ft("Using.EnterScope");

   Context::SetPos(GetLoc());
   if(IsAtFileScope()) GetFile()->InsertUsing(this);
   FindReferent();
   return true;
}

//------------------------------------------------------------------------------

void Using::ExitBlock() const
{
   Debug::ft("Using.ExitBlock");

   Block::RemoveUsing(this);
}

//------------------------------------------------------------------------------

fn_name Using_FindReferent = "Using.FindReferent";

void Using::FindReferent()
{
   Debug::ft(Using_FindReferent);

   //  If the symbol table doesn't know what this using statement refers to,
   //  log it.  Template arguments are not supported in a using statement.
   //
   if(Referent() != nullptr) return;

   auto qname = QualifiedName(true, false);
   auto log = "Unknown using: " + qname + " [" + strLocation() + ']';
   Debug::SwLog(Using_FindReferent, log, 0, false);
}

//------------------------------------------------------------------------------

bool Using::GetSpan(size_t& begin, size_t& left, size_t& end) const
{
   Debug::ft("Using.GetSpan");

   return GetSemiSpan(begin, end);
}

//------------------------------------------------------------------------------

bool Using::IsUsingFor
   (const string& fqName, size_t prefix, const CxxScope* scope) const
{
   Debug::ft("Using.IsUsingFor");

   auto ref = Referent();
   if(ref == nullptr) return false;

   //  See if the using statement's referent is a superscope of fqName.
   //
   auto fqSuper = ref->ScopedName(false);
   auto pos = CompareScopes(fqName, fqSuper, false);

   if((pos != string::npos) && (pos >= prefix))
   {
      //  This can be invoked when >check or >trim adds a using statement.
      //  In that case, the using statement was not part of the original
      //  source, so don't claim that it has users.
      //
      if(Context::ParsingSourceCode())
      {
         ++users_;
         auto item = static_cast<Using*>(FindTemplateAnalog(this));
         if(item != nullptr) ++item->users_;
      }
      return true;
   }

   return false;
}

//------------------------------------------------------------------------------

CxxToken* Using::PosToItem(size_t pos) const
{
   auto item = CxxScoped::PosToItem(pos);
   if(item != nullptr) return item;

   return name_->PosToItem(pos);
}

//------------------------------------------------------------------------------

CxxScoped* Using::Referent() const
{
   Debug::ft("Using.Referent");

   auto ref = name_->GetReferent();
   if(ref != nullptr) return ref;

   SymbolView view;
   return ResolveName(GetFile(), GetScope(), USING_REFS, view);
}

//------------------------------------------------------------------------------

fn_name Using_ScopedName = "Using.ScopedName";

string Using::ScopedName(bool templates) const
{
   auto ref = Referent();
   if(ref != nullptr) return ref->ScopedName(templates);

   auto expl = "using " + QualifiedName(true, false) + ": symbol not found";
   Context::SwLog(Using_ScopedName, expl, 0);
   return ERROR_STR;
}

//------------------------------------------------------------------------------

void Using::SetScope(CxxScope* scope)
{
   Debug::ft("Using.SetScope");

   //  If a using statement appears in a class, the class is not part
   //  of what it refers to, so step out to the class's namespace.
   //
   if(scope->Type() == Cxx::Class) scope = scope->GetSpace();
   CxxScoped::SetScope(scope);
}

//------------------------------------------------------------------------------

void Using::UpdatePos
   (EditorAction action, size_t begin, size_t count, size_t from) const
{
   CxxScoped::UpdatePos(action, begin, count, from);
   name_->UpdatePos(action, begin, count, from);
}

//------------------------------------------------------------------------------

void Using::UpdateXref(bool insert)
{
   name_->UpdateXref(insert);
}
}
