//==============================================================================
//
//  CxxNamed.cpp
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
#include "CxxNamed.h"
#include <cstring>
#include <set>
#include <sstream>
#include <utility>
#include "CodeFile.h"
#include "CxxArea.h"
#include "CxxRoot.h"
#include "CxxScope.h"
#include "CxxScoped.h"
#include "CxxSymbols.h"
#include "Debug.h"
#include "Editor.h"
#include "Formatters.h"
#include "Parser.h"
#include "Singleton.h"

using namespace NodeBase;
using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace CodeTools
{
//  PARMS are a template's parameters, and ARGS are the arguments in one of its
//  specializations or instantiations.  Updates TMAP to map each parameter to
//  its corresponding argument.  For example, given vector<T> and vector<TYPE>,
//  TMAP will contain an entry that maps T to TYPE.
//
static void BuildParameterMap(const TemplateParmPtrVector* parms,
   const TemplateArgPtrVector* args, TemplateParmToArgMap& tmap)
{
   Debug::ft("CodeTools.BuildParameterMap");

   std::pair<string, string> parmToArg;

   for(size_t i = 0; i < args->size(); ++i)
   {
      parmToArg.first = parms->at(i)->Name();

      std::ostringstream stream;
      Flags options(FQ_Mask | Code_Mask | NoAC_Mask | NoTP_Mask);
      args->at(i)->Print(stream, options);
      parmToArg.second = stream.str();

      tmap.insert(parmToArg);
   }
}

//------------------------------------------------------------------------------
//
//  Checks if REF (the name's referent) is a template argument.
//
static void CheckIfTemplateArgument(const CxxScoped* ref)
{
   Debug::ft("CodeTools.CheckIfTemplateArgument");

   //  If we are parsing a function in a template instance and this name's
   //  referent (REF) is a template argument, find the template's version
   //  of that function, indicating that its code uses a template argument.
   //
   if(!Context::GetParser()->ParsingTemplateInstance()) return;
   auto scope = Context::Scope();
   if(scope == nullptr) return;
   auto ifunc = scope->GetFunction();
   if(ifunc == nullptr) return;
   auto inst = ifunc->GetClass();
   if(inst == nullptr) return;
   if(!inst->IsInTemplateInstance()) return;
   auto args = inst->GetTemplatedName()->Args();

   for(auto a = args->cbegin(); a != args->cend(); ++a)
   {
      if((*a)->ReferentDefn() == ref)
      {
         auto tfunc = inst->FindTemplateAnalog(ifunc);
         if(tfunc != nullptr)
            static_cast<Function*>(tfunc)->SetTemplateParm();
      }
   }
}

//------------------------------------------------------------------------------

fn_name CodeTools_ReferentError = "CodeTools.ReferentError";

static CxxScoped* ReferentError(const string& item, debug64_t offset)
{
   Debug::ft(CodeTools_ReferentError);

   auto expl = "Failed to find referent for " + item;
   Context::SwLog(CodeTools_ReferentError, expl, offset);
   return nullptr;
}

//==============================================================================

CxxNamed::CxxNamed()
{
   Debug::ft("CxxNamed.ctor");
}

//------------------------------------------------------------------------------

CxxNamed::~CxxNamed()
{
   Debug::ftnt("CxxNamed.dtor");
}

//------------------------------------------------------------------------------

CxxNamed::CxxNamed(const CxxNamed& that) : CxxToken(that)
{
   Debug::ft("CxxNamed.ctor(copy)");
}

//------------------------------------------------------------------------------

void CxxNamed::Accessed(const StackArg* via) const
{
   Debug::ft("CxxNamed.Accessed");

   auto func = Context::Scope()->GetFunction();
   if(func == nullptr) return;
   func->ItemAccessed(this, via);
}

//------------------------------------------------------------------------------

void CxxNamed::AddUsage()
{
   Debug::ft("CxxNamed.AddUsage");

   if(!Context::ParsingSourceCode()) return;
   auto file = Context::File();
   if(file == nullptr) return;
   file->AddUsage(this);
}

//------------------------------------------------------------------------------

fn_name CxxNamed_ArgString = "CxxNamed.ArgString";

string CxxNamed::ArgString(const TemplateParmToArgMap& tmap) const
{
   Debug::ft(CxxNamed_ArgString);

   Context::SwLog(CxxNamed_ArgString, strOver(this), 0);
   return ERROR_STR;
}

//------------------------------------------------------------------------------

void CxxNamed::DisplayReferent(ostream& stream, bool fq) const
{
   auto ref = Referent();

   if(ref == nullptr)
   {
      stream << "null referent";
   }
   else
   {
      stream << ref->GetFile()->Name();
      if(!fq) stream << ": " << ref->Name();
   }
}

//------------------------------------------------------------------------------

fn_name CxxNamed_FindReferent = "CxxNamed.FindReferent";

void CxxNamed::FindReferent()
{
   Debug::ft(CxxNamed_FindReferent);

   Context::SwLog(CxxNamed_FindReferent, strOver(this), 0);
}

//------------------------------------------------------------------------------

CxxArea* CxxNamed::GetArea() const
{
   auto item = GetScope();
   if(item == nullptr) return nullptr;
   return item->GetArea();
}

//------------------------------------------------------------------------------

Class* CxxNamed::GetClass() const
{
   auto item = GetScope();
   if(item == nullptr) return nullptr;
   return item->GetClass();
}

//------------------------------------------------------------------------------

void CxxNamed::GetDirectClasses(CxxUsageSets& symbols)
{
   Debug::ft("CxxNamed.GetDirectClasses");

   auto spec = GetTypeSpec();
   if(spec == nullptr) return;
   spec->GetDirectClasses(symbols);
}

//------------------------------------------------------------------------------

void CxxNamed::GetDirectTemplateArgs(CxxUsageSets& symbols) const
{
   Debug::ft("CxxNamed.GetDirectTemplateArgs");

   auto spec = GetTypeSpec();
   if(spec == nullptr) return;
   spec->GetDirectTemplateArgs(symbols);
}

//------------------------------------------------------------------------------

CodeFile* CxxNamed::GetDistinctDeclFile() const
{
   auto defn = GetDefnFile();

   if(defn != nullptr)
   {
      auto decl = GetDeclFile();
      if(decl != defn) return decl;
   }

   return nullptr;
}

//------------------------------------------------------------------------------

CodeFile* CxxNamed::GetImplFile() const
{
   auto file = GetDefnFile();
   if(file != nullptr) return file;
   return GetDeclFile();
}

//------------------------------------------------------------------------------

void CxxNamed::GetScopedNames(stringVector& names, bool templates) const
{
   names.push_back(SCOPE_STR + ScopedName(templates));
}

//------------------------------------------------------------------------------

Namespace* CxxNamed::GetSpace() const
{
   auto item = GetScope();
   if(item == nullptr) return nullptr;
   return item->GetSpace();
}

//------------------------------------------------------------------------------

void CxxNamed::Instantiating(CxxScopedVector& locals) const
{
   Debug::SwLog("CxxNamed.Instantiating", strOver(this), 0);
}

//------------------------------------------------------------------------------

bool CxxNamed::IsPreviousDeclOf(const CxxNamed* item) const
{
   Debug::ft("CxxNamed.IsPreviousDeclOf");

   //  ITEM and "this" are already known to have the same name.  Check that
   //  they are distinct.
   //
   if((item == this) || (item == nullptr)) return false;

   //  ITEM and "this" must belong to the same class, or to no class.
   //
   auto thisClass = this->GetClass();
   auto itemClass = item->GetClass();
   if(thisClass != itemClass) return false;

   //  If ITEM is not a class member, "this" refers to it as long ITEM isn't
   //  static.  If ITEM *is* static, its linkage is internal, so "this" must
   //  be in the same file--a forward declaration.  ITEM's linkage is also
   //  internal if it is const and "this" is not extern--this only applies
   //  to data, as a free function cannot be const.
   //
   if(itemClass == nullptr)
   {
      if(item->IsStatic())
      {
         if(this->GetFile() != item->GetFile()) return false;
      }

      if(item->IsConst() && !this->IsExtern()) return false;
      return true;
   }

   //  For "this" and ITEM to refer to the same entity, the file that declares
   //  "this" must be in the transitive #include of ITEM.
   //
   auto file1 = this->GetFile();
   auto file2 = item->GetFile();
   const auto& affecters = file2->Affecters();
   auto iter = affecters.find(file1);
   return (iter != affecters.cend());
}

//------------------------------------------------------------------------------

fn_name CxxNamed_ItemIsTemplateArg = "CxxNamed.ItemIsTemplateArg";

bool CxxNamed::ItemIsTemplateArg(const CxxNamed* item) const
{
   Debug::ft(CxxNamed_ItemIsTemplateArg);

   Debug::SwLog(CxxNamed_ItemIsTemplateArg, strOver(this), 0);
   return false;
}

//------------------------------------------------------------------------------

fn_name CxxNamed_MemberToArg = "CxxNamed.MemberToArg";

StackArg CxxNamed::MemberToArg(StackArg& via, TypeName* name, Cxx::Operator op)
{
   Debug::ft(CxxNamed_MemberToArg);

   //  This should only be invoked on ClassData.
   //
   auto expl = "Unexpected member selection by " + via.item_->Name();
   Context::SwLog(CxxNamed_MemberToArg, expl, op);

   return NameToArg(op, nullptr);
}

//------------------------------------------------------------------------------

fn_name CxxNamed_NamesReferToArgs = "CxxNamed.NamesReferToArgs";

bool CxxNamed::NamesReferToArgs(const NameVector& names,
   const CxxScope* scope, CodeFile* file, size_t& index) const
{
   Debug::ft(CxxNamed_NamesReferToArgs);

   Debug::SwLog(CxxNamed_NamesReferToArgs, strOver(this), 0);
   return false;
}

//------------------------------------------------------------------------------

StackArg CxxNamed::NameToArg(Cxx::Operator op, TypeName* name)
{
   Debug::ft("CxxNamed.NameToArg");

   Accessed(nullptr);
   return StackArg(this, name);
}

//------------------------------------------------------------------------------

void CxxNamed::RenameNonQual(string& oldName, const string& newName) const
{
   Debug::ft("CxxNamed.RenameNonQual");

   if(oldName == newName) return;
   GetFile()->GetEditor().Rename(GetPos(), oldName, newName);
   oldName = newName;
}

//------------------------------------------------------------------------------

fn_name CxxNamed_ResolveName = "CxxNamed.ResolveName";

CxxScoped* CxxNamed::ResolveName(CodeFile* file,
   const CxxScope* scope, const Flags& mask, SymbolView& view) const
{
   Debug::ft(CxxNamed_ResolveName);

   CxxScoped* item;
   string name;
   Namespace* space;
   Class* cls;
   auto defts = view.defts_;
   auto func = GetFunction();
   auto qname = GetQualName();
   auto size = qname->Size();
   auto syms = Singleton<CxxSymbols>::Instance();
   auto selector = (size == 1 ? mask : SCOPE_REFS);
   size_t idx = (qname->IsGlobal() ? 0 : 1);

   if(idx == 0)
   {
      //  The name is prefixed by "::", so begin the search in the
      //  global namespace, starting with the first name.
      //
      view = DeclaredGlobally;
      item = Singleton<CxxRoot>::Instance()->GlobalNamespace();
   }
   else
   {
      //  Look for a terminal or local variable.
      //
      if(size == 1)
      {
         item = Context::FindLocal(Name(), view);

         if(item != nullptr)
         {
            qname->SetReferentN(0, item, &view);
            return item;
         }
      }

      //  Start with the first name in the qualified name.  Return if
      //  it refers to itself, which can occur for a friend declaration.
      //
      name = qname->At(0)->Name();
      item = syms->FindSymbol(file, scope, name, selector, view);
      qname->SetReferentN(0, item, &view);
      if(item == this) return item;

      if((size > 1) && !defts)
      {
         qname->CheckForRedundantScope();
      }
   }

   //  Continue with the name at IDX.
   //
   while(item != nullptr)
   {
      auto type = item->Type();

      switch(type)
      {
      case Cxx::Terminal:
      case Cxx::Function:
      case Cxx::Data:
      case Cxx::Enumerator:
      case Cxx::Macro:
         return item;

      case Cxx::Namespace:
         //
         //  If there is another name, resolve it within this namespace,
         //  else return the namespace itself.
         //
         if(idx >= size) return item;
         space = static_cast<Namespace*>(item);
         if(!name.empty()) name += SCOPE_STR;
         name += qname->At(idx)->Name();
         item = nullptr;
         if(++idx >= size)
         {
            selector = mask;
            if(func != nullptr)
            {
               view = DeclaredLocally;
               item = space->MatchFunc(func, false);
            }
         }
         if(item == nullptr)
         {
            view = NotAccessible;
            item = syms->FindSymbol(file, scope, name, selector, view, space);
            if(name.find(SCOPE_STR) != string::npos) view.using_ = false;
         }
         qname->SetReferentN(idx - 1, item, &view);
         if(item == nullptr) return nullptr;
         break;

      case Cxx::Class:
         cls = static_cast<Class*>(item);

         do
         {
            //  Before looking up the next name, see if this class has template
            //  arguments.  If so, create the template instance, and instantiate
            //  it if another name (one of its members) follows.  Don't apply
            //  template arguments, however, when parsing a template or template
            //  instance.
            //
            if(cls->IsInTemplateInstance()) break;
            auto tname = qname->At(idx - 1)->GetTemplatedName();
            if(tname == nullptr) break;
            if(tname->HasTemplateParmFor(scope)) break;
            if(!ResolveTemplate(cls, tname, (idx >= size))) break;
            cls = cls->EnsureInstance(tname);
            item = cls;
            qname->SetReferentN(idx - 1, item, &view);  // updated value
            if(item == nullptr) return nullptr;
            if(idx < size) cls->Instantiate();
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
            if(func != nullptr)
            {
               view = DeclaredLocally;
               item = cls->MatchFunc(func, true);
            }
         }
         if(item == nullptr)
         {
            view = NotAccessible;
            item = cls->FindMember(name, true, scope, &view);
         }
         qname->SetReferentN(idx - 1, item, &view);
         if(item == nullptr) return nullptr;
         if(item->GetClass() != cls) qname->At(idx - 1)->SubclassAccess(cls);
         break;

      case Cxx::Enum:
         //
         //  If there is another name, resolve it within this namespace,
         //  else return the enum itself.
         //
         if(idx >= size) return item;
         name = qname->At(idx)->Name();
         item = static_cast<Enum*>(item)->FindEnumerator(name);
         view = DeclaredLocally;
         qname->SetReferentN(idx, item, &view);
         return item;

      case Cxx::Typedef:
      {
         //  See if the item wants to resolve the typedef.  In case the
         //  typedef is that of a template, instantiate it if a template
         //  member is being named.
         //
         auto tdef = static_cast<Typedef*>(item);
         tdef->SetAsReferent(this);
         if(!ResolveTypedef(tdef, idx - 1)) return tdef;
         auto root = tdef->Root();
         if(root == nullptr) return tdef;
         item = static_cast<CxxScoped*>(root);
         qname->SetReferentN(idx - 1, item, &view);  // updated value
         if(idx < size) item->Instantiate();
         break;
      }

      case Cxx::Forward:
      case Cxx::Friend:
      {
         if(!ResolveForward(item, idx - 1)) return item;
         auto ref = item->Referent();
         if(ref == nullptr) return item;
         item = ref;
         qname->SetReferentN(idx - 1, item, &view);  // updated value
         break;
      }

      default:
         auto expl = "Invalid type found while resolving " + name;
         Context::SwLog(CxxNamed_ResolveName, expl, type);
         return nullptr;
      }
   }

   return item;
}

//------------------------------------------------------------------------------

string CxxNamed::ScopedName(bool templates) const
{
   //  If the item's scope is not yet known, return its qualified name.
   //  If its scope is known, prefix the enclosing scopes to the name
   //  unless the item is unnamed, as in an anonymous enum or union.
   //
   auto scope = GetScope();
   if(scope == nullptr) return QualifiedName(true, templates);
   auto qname = QualifiedName(false, templates);
   if(qname.empty()) return scope->ScopedName(templates);
   return Prefix(scope->ScopedName(templates)) + qname;
}

//------------------------------------------------------------------------------

void CxxNamed::SetReferent(CxxScoped* item, const SymbolView* view) const
{
   Debug::ft("CxxNamed.SetReferent");

   Context::SwLog(CxxNamed_FindReferent, strOver(this), 0);
}

//------------------------------------------------------------------------------

fn_name CxxNamed_SetTemplateParms = "CxxNamed.SetTemplateParms";

void CxxNamed::SetTemplateParms(TemplateParmsPtr& parms)
{
   Debug::ft(CxxNamed_SetTemplateParms);

   auto expl = "Template parameters not supported by " + Trace();
   Context::SwLog(CxxNamed_SetTemplateParms, expl, 0);
}

//------------------------------------------------------------------------------

void CxxNamed::strName(ostream& stream, bool fq, const QualName* name) const
{
   if(fq)
      stream << ScopedName(true);
   else
      name->Print(stream, NoFlags);
}

//------------------------------------------------------------------------------

string CxxNamed::to_str() const
{
   std::ostringstream stream;

   //  If this isn't the global namespace, remove any leading scope
   //  resolution operator.
   //
   auto name = ScopedName(true);
   if((name.rfind(SCOPE_STR, 0) == 0) && (name.size() > 2))
   {
      name.erase(0, 2);
   }

   stream << name << " @ " << strLocation();
   stream << " [" << strClass(this, false) << ']';
   return stream.str();
}

//------------------------------------------------------------------------------

string CxxNamed::XrefName(bool templates) const
{
   //  This is like ScopedName except that it invokes XrefName on scopes
   //  and separates names with a dot rather than a scope operator.
   //
   auto scope = GetScope();
   if(scope == nullptr) return QualifiedName(true, templates);
   auto xname = QualifiedName(false, templates);
   if(xname.empty()) return scope->XrefName(templates);
   return Prefix(scope->XrefName(templates), ".") + xname;
}

//==============================================================================

const TypeSpecPtr DataSpec::Bool = TypeSpecPtr(new DataSpec(BOOL_STR));
const TypeSpecPtr DataSpec::Int = TypeSpecPtr(new DataSpec(INT_STR));

//------------------------------------------------------------------------------

DataSpec::DataSpec(QualNamePtr& name) :
   name_(name.release()),
   arrays_(nullptr)
{
   Debug::ft("DataSpec.ctor");
}

//------------------------------------------------------------------------------

DataSpec::DataSpec(c_string name) : arrays_(nullptr)
{
   Debug::ft("DataSpec.ctor(string)");

   name_ = QualNamePtr(new QualName(name));
}

//------------------------------------------------------------------------------

DataSpec::~DataSpec()
{
   Debug::ftnt("DataSpec.dtor");
}

//------------------------------------------------------------------------------

DataSpec::DataSpec(const DataSpec& that) : TypeSpec(that),
   arrays_(nullptr),
   tags_(that.tags_)
{
   Debug::ft("DataSpec.ctor(copy)");

   SetInternal(true);
   name_.reset(new QualName(*that.name_.get()));
}

//------------------------------------------------------------------------------

void DataSpec::AddArray(ArraySpecPtr& array)
{
   Debug::ft("DataSpec.AddArray");

   if(arrays_ == nullptr) arrays_.reset(new ArraySpecPtrVector);
   arrays_->push_back(std::move(array));
   tags_.AddArray();
}

//------------------------------------------------------------------------------

fn_name DataSpec_AlignTemplateArg = "DataSpec.AlignTemplateArg";

string DataSpec::AlignTemplateArg(const TypeSpec* thatArg) const
{
   Debug::ft(DataSpec_AlignTemplateArg);

   //  If this is a template argument, remove any tags specified by this type
   //  from thatArg's type.  An example of this is unique_ptr<void*[]>, whose
   //  template argument is initially void**, but then gets adjusted to void*
   //  because the template parameter in unique_ptr<T[]> removes one pointer.
   //
   auto role = GetTemplateRole();
   if(role != TemplateArgument)
   {
      Context::SwLog(DataSpec_AlignTemplateArg, "unexpected role", role);
      return ERROR_STR;
   }

   auto thisTags = this->GetAllTags();
   if(thisTags.PtrCount(true) == 0)
   {
      return thatArg->TypeString(true);
   }

   auto thatTags = thatArg->GetAllTags();
   if(!thatTags.AlignTemplateTags(thisTags))
   {
      Context::SwLog(DataSpec_AlignTemplateArg, "tags alignment failed", 0);
      return ERROR_STR;
   }

   return thatArg->TypeTagsString(thatTags);
}

//------------------------------------------------------------------------------

const TemplateArgPtrVector* DataSpec::Args() const
{
   return name_->Args();
}

//------------------------------------------------------------------------------

string DataSpec::ArgString(const TemplateParmToArgMap& tmap) const
{
   std::ostringstream stream;

   if(tags_.IsConst()) stream << CONST_STR << SPACE;
   if(tags_.IsVolatile()) stream << VOLATILE_STR << SPACE;
   stream << name_->ArgString(tmap);
   tags_.Print(stream);

   return stream.str();
}

//------------------------------------------------------------------------------

TagCount DataSpec::Arrays() const
{
   Debug::ft("DataSpec.Arrays");

   TagCount count = 0;
   const TypeSpec* spec = this;

   while(spec != nullptr)
   {
      count += spec->Tags()->ArrayCount();
      auto ref = spec->Referent();
      if(ref == nullptr) break;
      spec = ref->GetTypeSpec();
   }

   return count;
}

//------------------------------------------------------------------------------

void DataSpec::Check() const
{
   name_->Check();

   if(arrays_ != nullptr)
   {
      for(auto a = arrays_->cbegin(); a != arrays_->cend(); ++a)
      {
         (*a)->Check();
      }
   }

   if(!IsInternal())
   {
      if(tags_.ptrDet_) Log(PtrTagDetached);
      if(tags_.refDet_) Log(RefTagDetached);
   }
}

//------------------------------------------------------------------------------

TypeSpec* DataSpec::Clone() const
{
   Debug::ft("DataSpec.Clone");

   return new DataSpec(*this);
}

//------------------------------------------------------------------------------

void DataSpec::CopyContext(const CxxToken* that, bool internal)
{
   Debug::ft("DataSpec.CopyContext");

   CxxNamed::CopyContext(that, internal);

   name_->CopyContext(that, internal);
}

//------------------------------------------------------------------------------

Class* DataSpec::DirectClass() const
{
   Debug::ft("DataSpec.DirectClass");

   auto root = Root();
   if(root->Type() != Cxx::Class) return nullptr;
   if(IsIndirect(false)) return nullptr;
   return static_cast<Class*>(root);
}

//------------------------------------------------------------------------------

CxxScoped* DataSpec::DirectType() const
{
   Debug::ft("DataSpec.DirectType");

   return name_->DirectType();
}

//------------------------------------------------------------------------------

void DataSpec::DisplayArrays(ostream& stream) const
{
   if(arrays_ != nullptr)
   {
      for(auto a = arrays_->cbegin(); a != arrays_->cend(); ++a)
      {
         (*a)->Print(stream, NoFlags);
      }
   }
}

//------------------------------------------------------------------------------

void DataSpec::DisplayTags(ostream& stream) const
{
   tags_.Print(stream);
}

//------------------------------------------------------------------------------

void DataSpec::EnterArrays() const
{
   Debug::ft("DataSpec.EnterArrays");

   if(arrays_ != nullptr)
   {
      for(auto a = arrays_->cbegin(); a != arrays_->cend(); ++a)
      {
         (*a)->EnterBlock();
      }
   }
}

//------------------------------------------------------------------------------

void DataSpec::EnterBlock()
{
   Debug::ft("DataSpec.EnterBlock");

   Context::SetPos(GetLoc());
   Context::PushArg(ResultType());
}

//------------------------------------------------------------------------------

void DataSpec::EnteringScope(const CxxScope* scope)
{
   Debug::ft("DataSpec.EnteringScope");

   Context::SetPos(GetLoc());

   if(scope->NameToTemplateParm(Name()) != nullptr)
   {
      SetTemplateRole(TemplateParameter);
   }

   EnterArrays();
   if(name_->GetReferent() == nullptr) FindReferent();
}

//------------------------------------------------------------------------------

fn_name DataSpec_FindReferent = "DataSpec.FindReferent";

void DataSpec::FindReferent()
{
   Debug::ft(DataSpec_FindReferent);

   //  Find referents for any template arguments used in the type's name.
   //  Bypass name_ itself; a QualName only finds its referent when used
   //  in executable code.
   //
   for(auto n = name_->First(); n != nullptr; n = n->Next())
   {
      n->FindReferent();
   }

   //  This should find a referent during parsing, when there is a context
   //  file.  If it isn't found then, it's pointless to look later.
   //
   auto file = Context::File();
   if(file == nullptr) return;
   auto scope = Context::Scope();
   if(scope == nullptr) return;

   if(ResolveTemplateArg()) return;

   SymbolView view;
   view.defts_ = (GetUserType() == TS_Definition);
   auto item = ResolveName(file, scope, TYPESPEC_REFS, view);

   if(item != nullptr)
   {
      SetReferent(item, &view);
      return;
   }

   //  The referent wasn't found.  If this is a template parameter (the "T"
   //  in "template<typename T>", for example) it never will be.
   //
   auto qname = QualifiedName(true, false);
   item = scope->NameToTemplateParm(qname);

   if(item != nullptr)
   {
      SetTemplateRole(TemplateParameter);
      view = DeclaredLocally;
      SetReferent(item, &view);
      return;
   }

   auto syms = Singleton<CxxSymbols>::Instance();

   switch(GetTemplateRole())
   {
   case TemplateArgument:
      //
      //  Here, NAME could be a constant instead of a type.  If not, it could
      //  be a template parameter used in a partial specialization.  In either
      //  case, report that the referent was found.
      //
      view = NotAccessible;
      item = syms->FindSymbol(file, scope, qname, VALUE_REFS, view);
      if(item != nullptr) SetReferent(item, &view);
      [[fallthrough]];
   case TemplateParameter:
   case TemplateClass:
      //
      //  When Operation::ExecuteOverload checks if a function overload applies,
      //  Function::MatchTemplate may create the DataSpec for a class template
      //  that defines an operator at file scope, like operator<< for a string.
      //  In this case, the class template may not even be visible in the scope
      //  where the possibility of the overload is being checked.
      //
      return;
   }

   //  When parsing a template instance, the arguments may not be visible,
   //  because the scope is the template instance itself.  For example, the
   //  type A is rarely visible in the scope std::unique_ptr<A>.
   //
   if(scope->IsInTemplateInstance()) return;

   //  The referent couldn't be found.
   //
   auto expl = "Failed to find referent for " + qname;
   Context::SwLog(DataSpec_FindReferent, expl, 0);
}

//------------------------------------------------------------------------------

TypeTags DataSpec::GetAllTags() const
{
   return TypeTags(*this);
}

//------------------------------------------------------------------------------

void DataSpec::GetDirectClasses(CxxUsageSets& symbols)
{
   Debug::ft("DataSpec.GetDirectClasses");

   name_->GetDirectClasses(symbols);
}

//------------------------------------------------------------------------------

void DataSpec::GetDirectTemplateArgs(CxxUsageSets& symbols) const
{
   Debug::ft("DataSpec.GetDirectTemplateArgs");

   auto ref = Referent();

   if(ref != nullptr)
   {
      auto args = ref->GetTemplatedName();

      if(args != nullptr)
      {
         args->GetDirectTemplateArgs(symbols);
      }

      if((GetTemplateRole() == TemplateArgument) && (Ptrs(true) == 0))
      {
         if(ref->IsForward())
         {
            ref->GetDirectClasses(symbols);
         }
      }
   }

   name_->GetDirectTemplateArgs(symbols);
}

//------------------------------------------------------------------------------

void DataSpec::GetNames(stringVector& names) const
{
   Debug::ft("DataSpec.GetNames");

   name_->GetNames(names);
}

//------------------------------------------------------------------------------

Numeric DataSpec::GetNumeric() const
{
   Debug::ft("DataSpec.GetNumeric");

   auto ptrs = Ptrs(true);
   if(ptrs > 0) return Numeric::Pointer;

   auto root = Root();
   if((root == nullptr) || (root == this)) return Numeric::Nil;
   return root->GetNumeric();
}

//------------------------------------------------------------------------------

TypeSpec* DataSpec::GetTypeSpec() const
{
   return const_cast<DataSpec*>(this);
}

//------------------------------------------------------------------------------

void DataSpec::GetUsages(const CodeFile& file, CxxUsageSets& symbols)
{
   //  Don't obtain usages for an auto type.  If the type ends up being
   //  used to invoke a function, for example, this usage will be noted.
   //  However, items in the expression that obtain the auto type will
   //  be noted as usages, and it must transitively #include that type.
   //
   if(IsAutoDecl()) return;

   //  Find usages for any template arguments used in the type's name.
   //  Bypass name_ itself, because it doesn't know if it is indirect.
   //
   for(auto n = name_->First(); n != nullptr; n = n->Next())
   {
      n->GetUsages(file, symbols);
   }

   //  Get the usages for any array specifications.
   //
   if(arrays_ != nullptr)
   {
      for(auto a = arrays_->cbegin(); a != arrays_->cend(); ++a)
      {
         (*a)->GetUsages(file, symbols);
      }
   }

   auto ref = DirectType();

   if(ref == nullptr)
   {
      //  The referent for this type was never found.  If this is actually
      //  a problem, a log should have been produced during compilation.
      //
      return;
   }

   //  Record how the item was used.
   //
   auto type = ref->Type();

   switch(type)
   {
   case Cxx::Terminal:
      break;

   case Cxx::Forward:
   case Cxx::Friend:
      symbols.AddForward(ref);
      break;

   case Cxx::Class:
   {
      auto tmplt = ref->GetTemplate();
      if(tmplt != nullptr) ref = tmplt;
      [[fallthrough]];
   }
   default:
      //  Although a .cpp can use a type indirectly, it is unusual.  In most
      //  cases, a pointer or reference type will be initialized, in which case
      //  it cannot be declared forward unless, for example, it is initialized
      //  to nullptr, passed as an argument, and not looked at again.  To make
      //  an accurate direct/indirect determination for a .cpp seems to involve
      //  more effort than is worthwhile.
      //
      if(file.IsHeader() && IsUsedInNameOnly())
         symbols.AddIndirect(ref);
      else
         symbols.AddDirect(ref);
   }
}

//------------------------------------------------------------------------------

fn_name DataSpec_Instantiating = "DataSpec.Instantiating";

void DataSpec::Instantiating(CxxScopedVector& locals) const
{
   Debug::ft(DataSpec_Instantiating);

   //  When instantiating a template, each of its arguments should have a
   //  referent.  Invoke Instantiate on each argument that is a class: if
   //  it's also a template, it must be instantiated so that our template
   //  instance can use it.
   //
   auto ref = Referent();

   if(ref != nullptr)
   {
      ref->Instantiate();

      if(ref->Type() == Cxx::TemplateParm)
      {
         //  To compile templates--not just template *instances*--we need to
         //  handle situations where one template uses another.  For example,
         //  TlvMessage::CopyParm<T> invokes TlvMessage::FindParm<T>.  In such
         //  a case, we "instantiate" the second template using a *template
         //  parameter* as a template argument.  So when FindParm<T> is about
         //  to be compiled, we will make T a local variable so that it can
         //  be resolved as a symbol.  Ideally Instantiate() would pass along
         //  our LOCALS parameter so that TemplateParm could override it and
         //  add itself to LOCALS, but this would be a bit messy, so it's done
         //  with this hack instead.
         //
         locals.push_back(ref);
      }

      name_->Instantiating(locals);
      return;
   }

   auto expl = "Failed to find referent for " + TypeString(false);
   Context::SwLog(DataSpec_Instantiating, expl, 0);
}

//------------------------------------------------------------------------------

bool DataSpec::IsAuto() const
{
   Debug::ft("DataSpec.IsAuto");

   //  A data item (FuncData) of type auto initially has the keyword "auto"
   //  as its referent.  This referent is overwritten when the data's actual
   //  type is determined.
   //
   return (Referent() == Singleton<CxxRoot>::Instance()->AutoTerm());
}

//------------------------------------------------------------------------------

bool DataSpec::IsAutoDecl() const
{
   return (Name() == AUTO_STR);
}

//------------------------------------------------------------------------------

bool DataSpec::IsConst() const
{
   Debug::ft("DataSpec.IsConst");

   if(IsAutoDecl()) return tags_.IsConst();
   if(tags_.IsConst()) return true;
   auto ref = Referent();
   if(ref == nullptr) return false;
   auto spec = ref->GetTypeSpec();
   if(spec == nullptr) return false;
   return spec->IsConst();
}

//------------------------------------------------------------------------------

bool DataSpec::IsConstPtr() const
{
   Debug::ft("DataSpec.IsConstPtr");

   auto cp = tags_.IsConstPtr();

   if(cp == 1) return true;
   if(cp == -1) return false;
   if(IsAutoDecl()) return false;

   //  We have no pointers, so see if our referent has any.
   //
   auto ref = Referent();
   if(ref == nullptr) return false;
   auto spec = ref->GetTypeSpec();
   if(spec == nullptr) return false;
   return spec->IsConstPtr();
}

//------------------------------------------------------------------------------

bool DataSpec::IsConstPtr(size_t n) const
{
   Debug::ft("DataSpec.IsConstPtr(size_t)");

   if(IsAutoDecl()) return tags_.IsConstPtr(n);
   if(tags_.IsConstPtr(n)) return true;
   auto ref = Referent();
   if(ref == nullptr) return false;
   auto spec = ref->GetTypeSpec();
   if(spec == nullptr) return false;
   return spec->IsConstPtr(n);
}

//------------------------------------------------------------------------------

bool DataSpec::IsIndirect(bool arrays) const
{
   Debug::ft("DataSpec.IsIndirect");

   return ((Refs() > 0) || (Ptrs(arrays) > 0));
}

//------------------------------------------------------------------------------

bool DataSpec::IsPOD() const
{
   //  An array of objects is not POD if its class has a constructor.
   //
   if(!TypeSpec::IsPOD()) return false;
   auto root = Root();
   if(root == nullptr) return true;
   if(root->Type() != Cxx::Class) return true;
   if(Ptrs(false) > 0) return true;
   return (static_cast<Class*>(root)->FindCtor(nullptr) == nullptr);
}

//------------------------------------------------------------------------------

bool DataSpec::IsUsedInNameOnly() const
{
   Debug::ft("DataSpec.IsUsedInNameOnly");

   //  This specification uses a type in name only (that is, it only needs to
   //  have been declared, but not defined) if one of the following is true:
   //  o The type has pointer or reference tags (but is not an array).
   //  o The type is used as a template argument--unless it is appearing in a
   //    template instance or code.
   //
   auto count = Ptrs(false);
   if(count > 0) return true;
   if(Refs() > 0) return true;

   auto role = GetTemplateRole();
   if(role != TemplateNone)
   {
      auto ref = name_->GetReferent();
      if((ref != nullptr) && ref->IsInTemplateInstance()) return false;
      return (role != TemplateArgument);
   }

   return false;
}

//------------------------------------------------------------------------------

bool DataSpec::IsVolatile() const
{
   Debug::ft("DataSpec.IsVolatile");

   if(IsAutoDecl()) return tags_.IsVolatile();
   if(tags_.IsVolatile()) return true;
   auto ref = Referent();
   if(ref == nullptr) return false;
   auto spec = ref->GetTypeSpec();
   if(spec == nullptr) return false;
   return spec->IsVolatile();
}

//------------------------------------------------------------------------------

bool DataSpec::IsVolatilePtr() const
{
   Debug::ft("DataSpec.IsVolatilePtr");

   auto vp = tags_.IsVolatilePtr();

   if(vp == 1) return true;
   if(vp == -1) return false;
   if(IsAutoDecl()) return false;

   //  We have no pointers, so see if our referent has any.
   //
   auto ref = Referent();
   if(ref == nullptr) return false;
   auto spec = ref->GetTypeSpec();
   if(spec == nullptr) return false;
   return spec->IsVolatilePtr();
}

//------------------------------------------------------------------------------

bool DataSpec::IsVolatilePtr(size_t n) const
{
   Debug::ft("DataSpec.IsVolatilePtr(size_t)");

   if(IsAutoDecl()) return tags_.IsVolatilePtr(n);
   if(tags_.IsVolatilePtr(n)) return true;
   auto ref = Referent();
   if(ref == nullptr) return false;
   auto spec = ref->GetTypeSpec();
   if(spec == nullptr) return false;
   return spec->IsVolatilePtr(n);
}

//------------------------------------------------------------------------------

bool DataSpec::ItemIsTemplateArg(const CxxNamed* item) const
{
   Debug::ft("DataSpec.ItemIsTemplateArg");

   return name_->ItemIsTemplateArg(item);
}

//------------------------------------------------------------------------------

bool DataSpec::MatchesExactly(const TypeSpec* that) const
{
   Debug::ft("DataSpec.MatchesExactly");

   auto type1 = this->TypeString(false);
   auto type2 = that->TypeString(false);
   return (type1 == type2);
}

//------------------------------------------------------------------------------

TypeMatch DataSpec::MatchTemplate(const TypeSpec* that,
   stringVector& tmpltParms, stringVector& tmpltArgs, bool& argFound) const
{
   Debug::ft("DataSpec.MatchTemplate");

   //  Do a depth-first traversal of this type and THAT.  For each node
   //  in this type, THAT must have a corresponding node.
   //
   if(that->Referent() == nullptr) return Incompatible;

   //  If this type is a template parameter, the node in THAT becomes its
   //  template argument.  If a template argument has already been found
   //  for the parameter, THAT must match it.
   //
   auto parm = QualifiedName(true, false);
   auto idx = FindIndex(tmpltParms, parm);
   auto match1 = Compatible;

   if(idx != string::npos)
   {
      //  If the template parameter specifies pointers, remove that number
      //  of pointers from the template argument to find the actual type.
      //
      TagCount thisPtrs = this->Ptrs(true);
      TagCount thatPtrs = that->Ptrs(true);
      if(thisPtrs > thatPtrs) return Incompatible;
      if(thisPtrs < thatPtrs) match1 = Convertible;

      argFound = true;
      auto thatType = that->TypeString(true);
      if(thisPtrs > 0) AdjustPtrs(thatType, -thisPtrs);

      //  If the type of the template parameter has already been set, assume
      //  that this type matches it.  Strictly comparing thatArg == thatType
      //  erroneously rejects, for example, std::min(int, unsigned int).
      //
      auto& thatArg = tmpltArgs.at(idx);
      if(thatArg.empty()) thatArg = thatType;
      return match1;
   }

   //  This type was not a template parameter.  THAT must match it.
   //
   if(this->Referent() == nullptr) return Incompatible;

   auto thisType = RemoveTemplates(this->TypeString(true));
   auto thatType = RemoveTemplates(that->TypeString(true));
   if((thisType != thatType) && (RemoveConsts(thisType) != thatType))
   {
      auto thisNum = this->GetNumeric();
      auto thatNum = that->GetNumeric();
      match1 = thisNum.CalcMatchWith(&thatNum);
      if(match1 == Incompatible) return Incompatible;
   }

   auto thisName = this->GetQualName();
   auto thatName = that->GetQualName();
   auto match2 = thisName->MatchTemplate
      (thatName, tmpltParms, tmpltArgs, argFound);
   return (match1 < match2 ? match1 : match2);
}

//------------------------------------------------------------------------------

TypeMatch DataSpec::MatchTemplateArg(const TypeSpec* that) const
{
   Debug::ft("DataSpec.MatchTemplateArg");

   //  If this is a template argument, match on the basis of tags,
   //  leaving room to prefer an exact match.
   //
   if(GetTemplateRole() == TemplateArgument)
   {
      auto thisTags = this->GetAllTags();
      auto thatTags = that->GetAllTags();
      return thisTags.MatchTemplateTags(thatTags);
   }

   //  This is not a template argument, so match on types.
   //
   if(MatchesExactly(that)) return Compatible;
   return Incompatible;
}

//------------------------------------------------------------------------------

bool DataSpec::NamesReferToArgs(const NameVector& names,
   const CxxScope* scope, CodeFile* file, size_t& index) const
{
   Debug::ft("DataSpec.NamesReferToArgs");

   //  See if NAME matches this type in constness and level of
   //  indirection while removing any "const" tags from NAME.
   //  Any pointer tags on NAME have already been removed.
   //
   if(index >= names.size()) return false;

   const auto& element = names.at(index);
   auto name = element.name;

   auto readonly = IsConst();
   auto pos = name.find("const ");

   if(readonly)
   {
      if(pos == string::npos) return false;
      name = name.erase(0, strlen("const "));
   }
   else
   {
      if(pos == 0) return false;
   }

   readonly = IsConstPtr();
   pos = name.find(" const");

   if(readonly)
   {
      if(pos == string::npos) return false;
      name = name.erase(pos, strlen(" const"));
   }
   else
   {
      if(pos == 0) return false;
   }

   if(element.ptrs != Ptrs(true)) return false;

   //  See if NAME refers to the same item as this type.  If this
   //  type refers to data (a constant), use its underlying type.
   //
   auto curr = name_->Referent();
   if(curr == nullptr) return false;

   switch(curr->Type())
   {
   case Cxx::Data:
      curr = curr->GetTypeSpec()->Referent();
      if(curr == nullptr) return false;
   }

   while(curr != nullptr)
   {
      //  Look for all symbols that match NAME.  There are cases in
      //  which NAME in FILE and SCOPE can see a class but not its
      //  forward declaration(s), and vice versa.
      //
      SymbolVector items;
      ViewVector views;

      auto syms = Singleton<CxxSymbols>::Instance();
      syms->FindSymbols(file, scope, name, TARG_REFS, items, views);

      if(!items.empty())
      {
         ++index;
         return true;
      }

      //  Keep looking while deeper underlying types exist.
      //
      auto next = curr->Referent();
      if(curr == next) break;
      curr = next;
   }

   return false;
}

//------------------------------------------------------------------------------

CxxToken* DataSpec::PosToItem(size_t pos) const
{
   auto item = TypeSpec::PosToItem(pos);
   if(item != nullptr) return item;

   item = name_->PosToItem(pos);
   if(item != nullptr) return item;

   if(arrays_ != nullptr)
   {
      for(auto a = arrays_->cbegin(); a != arrays_->cend(); ++a)
      {
         item = (*a)->PosToItem(pos);
         if(item != nullptr) return item;
      }
   }

   return nullptr;
}

//------------------------------------------------------------------------------

void DataSpec::Print(ostream& stream, const Flags& options) const
{
   if(tags_.IsConst()) stream << CONST_STR << SPACE;
   if(tags_.IsVolatile()) stream << VOLATILE_STR << SPACE;
   name_->Print(stream, options);
   tags_.Print(stream);

   if(IsAutoDecl())
   {
      stream << SPACE << COMMENT_BEGIN_STR << SPACE;
      stream << TypeString(true) << SPACE << COMMENT_END_STR;
   }
}

//------------------------------------------------------------------------------

fn_name DataSpec_Ptrs = "DataSpec.Ptrs";

TagCount DataSpec::Ptrs(bool arrays) const
{
   Debug::ft(DataSpec_Ptrs);

   TagCount count = 0;
   const TypeSpec* spec = this;

   while(spec != nullptr)
   {
      count += spec->Tags()->PtrCount(arrays);
      auto ref = spec->Referent();
      if(ref == nullptr) break;
      spec = ref->GetTypeSpec();
   }

   //  COUNT can be negative if this is invoked on an auto type with ARRAYS
   //  set to false.  Given
   //     auto& entry = table[index];
   //  ENTRY has a referent of TABLE, and ptrs_ = -1.  If arrays are then
   //  excluded from ENTRY's pointer count, the result will be -1 because
   //  TABLE's pointer count of 1 will not be included.
   //
   if(count < 0)
   {
      if(arrays || !IsAutoDecl())
      {
         auto expl = "Negative pointer count for " + Trace();
         Context::SwLog(DataSpec_Ptrs, expl, count);
      }
   }

   return count;
}

//------------------------------------------------------------------------------

CxxScoped* DataSpec::Referent() const
{
   auto ref = name_->GetReferent();
   if(ref != nullptr) return ref;

   const_cast<DataSpec*>(this)->FindReferent();
   return name_->GetReferent();
}

//------------------------------------------------------------------------------

fn_name DataSpec_Refs = "DataSpec.Refs";

TagCount DataSpec::Refs() const
{
   Debug::ft(DataSpec_Refs);

   //  An auto type can have a negative reference count that is eliminated
   //  once its type is determined.  Stop as soon as the count is positive;
   //  else an l-value reference (&) could become an rvalue reference (&&).
   //
   TagCount count = 0;
   const TypeSpec* spec = this;

   while(spec != nullptr)
   {
      count += spec->Tags()->RefCount();
      if(count > 0) return count;
      auto ref = spec->Referent();
      if(ref == nullptr) break;
      spec = ref->GetTypeSpec();
   }

   if(count >= 0) return count;

   auto expl = "Negative reference count for " + Trace();
   Context::SwLog(DataSpec_Refs, expl, count);
   return 0;
}

//------------------------------------------------------------------------------

bool DataSpec::ResolveForward(CxxScoped* decl, size_t n) const
{
   Debug::ft("DataSpec.ResolveForward");

   //  Stop at the forward declaration unless it's a template.  If it is,
   //  continue so that template arguments can be applied to its referent,
   //  provided that it has already been found.
   //
   if(decl->IsTemplate())
   {
      name_->At(n)->SetForward(decl);
      decl->SetAsReferent(this);
      return (decl->Referent() != nullptr);
   }

   return false;
}

//------------------------------------------------------------------------------

bool DataSpec::ResolveTemplate(Class* cls, const TypeName* type, bool end) const
{
   Debug::ft("DataSpec.ResolveTemplate");

   //  This returns false to avoid creating a template instance for a template
   //  type that was only created internally, during template matching.
   //
   if(GetTemplateRole() == TemplateClass) return false;

   auto args = type->Args();

   for(auto a = args->cbegin(); a != args->cend(); ++a)
   {
      auto ref = (*a)->GetReferent();

      if((ref != nullptr) && (ref->Type() != Cxx::TemplateParm))
      {
         return true;
      }
   }

   return false;
}

//------------------------------------------------------------------------------

bool DataSpec::ResolveTemplateArg() const
{
   Debug::ft("DataSpec.ResolveTemplateArg");

   if(GetTemplateRole() != TemplateArgument) return false;

   auto parser = Context::GetParser();
   auto item = parser->ResolveInstanceArgument(name_.get());
   if(item == nullptr) return false;

   SetReferent(item, nullptr);
   return true;
}

//------------------------------------------------------------------------------

bool DataSpec::ResolveTypedef(Typedef* type, size_t n) const
{
   Debug::ft("DataSpec.ResolveTypedef");

   //  Stop at the typedef unless it has template arguments.  If it does,
   //  delegate to name_, which will record it as a referent and resolve
   //  it to the template instance.
   //
   if(type->GetTemplatedName() == nullptr) return false;
   return name_->ResolveTypedef(type, n);
}

//------------------------------------------------------------------------------

fn_name DataSpec_ResultType = "DataSpec.ResultType";

StackArg DataSpec::ResultType() const
{
   Debug::ft(DataSpec_ResultType);

   auto ref = ReferentDefn();

   if(ref != nullptr)
   {
      StackArg arg(ref, tags_.PtrCount(true), tags_.IsLvalue(), false);
      arg.SetRefs(tags_.RefCount());
      if(tags_.IsConst()) arg.SetAsConst();
      if(tags_.IsConstPtr() == 1) arg.SetAsConstPtr();
      return arg;
   }

   if(GetTemplateRole() != TemplateClass)
   {
      auto expl = "Failed to find referent for " + QualifiedName(true, true);
      Context::SwLog(DataSpec_ResultType, expl, 0);
   }

   return NilStackArg;
}

//------------------------------------------------------------------------------

fn_name DataSpec_SetPtrs = "DataSpec.SetPtrs";

void DataSpec::SetPtrs(TagCount count)
{
   Debug::ft(DataSpec_SetPtrs);

   //  This should only be invoked on an auto type.  After resetting the
   //  count, invoke Ptrs to cause a log if the overall count is invalid.
   //
   if(!IsAutoDecl())
   {
      auto expl = "Resetting pointers on non-auto type " + Trace();
      Context::SwLog(DataSpec_SetPtrs, expl, 0);
      return;
   }

   tags_.SetPtrs(count);
   Ptrs(true);
}

//------------------------------------------------------------------------------

fn_name DataSpec_SetReferent = "DataSpec.SetReferent";

void DataSpec::SetReferent(CxxScoped* item, const SymbolView* view) const
{
   Debug::ft(DataSpec_SetReferent);

   //  If ITEM is an unresolved forward declaration for a template, our referent
   //  needs to be a template instance instantiated from that template.  This is
   //  not yet possible, so make sure that our referent is empty so that we will
   //  revisit it.
   //
   if(item == nullptr)
   {
      string expl = "Nil ITEM for " + TypeString(true);
      Context::SwLog(DataSpec_SetReferent, expl, 0);
      return;
   }

   if((item->IsTemplate() && item->Referent() == nullptr))
   {
      name_->SetReferent(nullptr, nullptr);
   }
   else
   {
      name_->SetReferent(item, view);
      if(GetTemplateRole() == TemplateArgument) item->WasRead();

      if(item->Type() != Cxx::Typedef)
      {
         //  SetAsReferent has already been invoked if our referent is a
         //  typedef, so don't invoke it again.
         //
         item->SetAsReferent(this);
      }
      else
      {
         //  If our referent is a pointer typedef, "const" and "volatile"
         //  apply to the pointer, not its target.
         //
         if(item->GetTypeSpec()->Ptrs(false) > 0)
         {
            if(tags_.IsConst())
            {
               tags_.SetConst(false);
               tags_.SetConstPtr();
            }

            if(tags_.IsVolatile())
            {
               tags_.SetVolatile(false);
               tags_.SetVolatilePtr();
            }
         }
      }
   }
}

//------------------------------------------------------------------------------

void DataSpec::SetTemplateRole(TemplateRole role) const
{
   Debug::ft("DataSpec.SetTemplateRole");

   TypeSpec::SetTemplateRole(role);

   if(role == TemplateClass)
   {
      for(auto n = name_->First(); n != nullptr; n = n->Next())
      {
         n->SetTemplateRole(TemplateParameter);
      }
   }
}

//------------------------------------------------------------------------------

void DataSpec::SetUserType(TypeSpecUser user) const
{
   Debug::ft("DataSpec.SetUserType");

   TypeSpec::SetUserType(user);

   for(auto n = name_->First(); n != nullptr; n = n->Next())
   {
      n->SetUserType(user);
   }
}

//------------------------------------------------------------------------------

string DataSpec::Trace() const
{
   auto result = TypeString(false);
   if(result != ERROR_STR) return result;
   return CxxNamed::Trace();
}

//------------------------------------------------------------------------------

string DataSpec::TypeString(bool arg) const
{
   string ts;

   //  Use the referent if it is known.  However, a template parameter has
   //  no referent, and a template argument could be an unresolved forward
   //  declaration.  In such cases, just use the full name.
   //
   //  Shameless hack.  If a static function returns a type defined in its
   //  class (e.g. an enum), code invoked from Function::AddThisArg arrives
   //  here when comparing function signatures.  This results in a spurious
   //  RedundantScope warning for the return type Class::Enum.  To suppress
   //  this, set our user type to TS_Definition, which is the value that it
   //  will soon take on when Function::EnterSignature is reached in that
   //  scenario.
   //
   auto hack = (GetUserType() == TS_Function);
   if(hack) SetUserType(TS_Definition);
   auto ref = Referent();
   if(hack) SetUserType(TS_Function);

   if(ref != nullptr)
   {
      ts = name_->TypeString(arg);
   }
   else
   {
      auto role = GetTemplateRole();
      if(role == TemplateNone)
      {
         Context::SwLog("DataSpec.TypeString", "unexpected role", role);
         return ERROR_STR;
      }

      ts = QualifiedName(true, true);
   }

   //  Remove any tags from TS and replace them with our own.
   //
   auto tags = GetAllTags();
   RemoveTags(ts);
   tags.TypeString(ts, arg);
   return ts;
}

//------------------------------------------------------------------------------

string DataSpec::TypeTagsString(const TypeTags& tags) const
{
   auto ts = name_->TypeString(true);
   tags.TypeString(ts, false);
   return ts;
}

//------------------------------------------------------------------------------

void DataSpec::UpdatePos
   (EditorAction action, size_t begin, size_t count, size_t from) const
{
   TypeSpec::UpdatePos(action, begin, count, from);
   name_->UpdatePos(action, begin, count, from);

   if(arrays_ != nullptr)
   {
      for(auto a = arrays_->cbegin(); a != arrays_->cend(); ++a)
      {
         (*a)->UpdatePos(action, begin, count, from);
      }
   }
}

//------------------------------------------------------------------------------

void DataSpec::UpdateXref(bool insert)
{
   if(IsAutoDecl()) return;

   name_->UpdateXref(insert);

   if(arrays_ != nullptr)
   {
      for(auto a = arrays_->cbegin(); a != arrays_->cend(); ++a)
      {
         (*a)->UpdateXref(insert);
      }
   }
}

//------------------------------------------------------------------------------

bool DataSpec::VerifyReferents() const
{
   return name_->VerifyReferents();
}

//==============================================================================

QualName::QualName(TypeNamePtr& name) :
   specifier_(Cxx::NilSpecifier),
   init_(false)
{
   Debug::ft("QualName.ctor(name)");

   first_ = std::move(name);
   first_->SetQualName(this);
}

//------------------------------------------------------------------------------

QualName::QualName(const string& name) :
   specifier_(Cxx::NilSpecifier),
   init_(false)
{
   Debug::ft("QualName.ctor(string)");

   auto copy = name;
   first_ = (TypeNamePtr(new TypeName(copy)));
   first_->SetQualName(this);
}

//------------------------------------------------------------------------------

QualName::~QualName()
{
   Debug::ftnt("QualName.dtor");
}

//------------------------------------------------------------------------------

QualName::QualName(const QualName& that) : CxxNamed(that),
   specifier_(that.specifier_),
   init_(that.init_)
{
   Debug::ft("QualName.ctor(copy)");

   for(auto n = that.First(); n != nullptr; n = n->Next())
   {
      TypeNamePtr name(new TypeName(*n));
      PushBack(name);
   }
}

//------------------------------------------------------------------------------

void QualName::AddPrefix(const string& name, Namespace* ns)
{
   Debug::ft("QualName.AddPrefix");

   auto scope = name;
   TypeNamePtr prefix(new TypeName(scope));

   first_->SetScoped(true);
   prefix->SetQualName(this);
   prefix->PushBack(first_);
   prefix->SetReferent(ns, nullptr);
   first_ = std::move(prefix);
}

//------------------------------------------------------------------------------

void QualName::Append(const string& name, bool space) const
{
   Debug::ft("QualName.Append");

   Last()->Append(name, space);
}

//------------------------------------------------------------------------------

const TemplateArgPtrVector* QualName::Args() const
{
   for(auto n = First(); n != nullptr; n = n->Next())
   {
      auto args = n->Args();
      if(args != nullptr) return args;
   }

   return nullptr;
}

//------------------------------------------------------------------------------

string QualName::ArgString(const TemplateParmToArgMap& tmap) const
{
   //  Get the fully qualified name without any template arguments.
   //  If it's a template parameter, replace it.
   //
   auto ref = Referent();

   if(ref != nullptr)
   {
      auto type = Last()->DirectType();

      if(type != ref)
      {
         return type->ArgString(tmap);
      }

      auto arg = ref->ScopedName(false);
      auto parm = tmap.find(arg);

      if(parm != tmap.cend())
      {
         arg = parm->second;
      }

      for(auto n = First(); n != nullptr; n = n->Next())
      {
         arg += n->ArgString(tmap);
      }

      return arg;
   }

   auto expl = "Failed to find referent for " + QualifiedName(true, true);
   Context::SwLog("QualName.ArgString", expl, 0);
   return ERROR_STR;
}

//------------------------------------------------------------------------------

TypeName* QualName::At(size_t n) const
{
   for(auto i = First(); i != nullptr; i = i->Next())
   {
      if(n == 0)
         return i;
      else
         --n;
   }

   return nullptr;
}

//------------------------------------------------------------------------------

void QualName::Check() const
{
   if(Name() == NULL_STR) Log(UseOfNull);

   for(auto n = First(); n != nullptr; n = n->Next())
   {
      n->Check();
   }
}

//------------------------------------------------------------------------------

bool QualName::CheckCtorDefn() const
{
   Debug::ft("QualName.CheckCtorDefn");

   auto size = Size();
   if(size <= 1) return false;
   return (At(size - 1)->Name() == At(size - 2)->Name());
}

//------------------------------------------------------------------------------

void QualName::CheckForRedundantScope() const
{
   Debug::ft("QualName.CheckForRedundantScope");

   //  This name is usually of the form FIRST::ITEM.  We want to see if FIRST
   //  can be removed, but this depends on the SCOPE where this name appears.
   //  So we start with the namespace or class that defines SCOPE (AREA) and
   //  proceed out through enclosing scopes, looking for one whose name
   //  matches FIRST.  If the matching AREA is INNER, FIRST is redundant
   //  (e.g. Class::ITEM used in one of Class's member functions).  If AREA
   //  is further out, then FIRST is redundant if INNER does not also declare
   //  an ITEM (e.g. Namespace::ITEM when no ambiguous Class::ITEM exists).
   //
   const auto& first = At(0)->Name();
   auto scope = GetScope();
   if(scope == nullptr) return;
   auto inner = scope->GetArea();
   auto type = inner->Type();
   auto cls = (type == Cxx::Class ? static_cast<Class*>(inner) : nullptr);

   for(CxxScope* area = inner; area != nullptr; area = area->GetScope())
   {
      if(area->Name() == first)
      {
         auto log = (area == inner);

         if(!log)
         {
            switch(type)
            {
            case Cxx::Namespace:
               log = (inner->FindItem(At(1)->Name()) == nullptr);
               break;
            case Cxx::Class:
               log = (cls->FindMember(At(1)->Name(), true) == nullptr);
               break;
            }
         }

         if(log)
         {
            Log(RedundantScope);
            return;
         }
      }
   }
}

//------------------------------------------------------------------------------

void QualName::CopyContext(const CxxToken* that, bool internal)
{
   Debug::ft("QualName.CopyContext");

   CxxNamed::CopyContext(that, internal);

   for(auto n = First(); n != nullptr; n = n->Next())
   {
      n->CopyContext(that, internal);
   }
}

//------------------------------------------------------------------------------

void QualName::DeleteName(const TypeName* name, TypeNamePtr& next)
{
   Debug::ft("QualName.DeleteName");

   if(first_.get() == name)
   {
      first_.release();
      first_ = std::move(next);
      if(first_ != nullptr) first_->SetScoped(false);
      return;
   }

   for(auto prev = first_.get(); prev != nullptr; prev = prev->Next())
   {
      if(prev->Next() == name)
      {
         prev->SetNext(next);
         return;
      }
   }
}

//------------------------------------------------------------------------------

void QualName::EnterBlock()
{
   Debug::ft("QualName.EnterBlock");

   Context::SetPos(GetLoc());

   //  If a "." or "->" operator is waiting for its second argument,
   //  push this name and return so that the operator can be executed.
   //
   auto op = Cxx::NIL_OPERATOR;
   auto top = Context::TopOp();

   if(top != nullptr)
   {
      op = top->Op();

      if((op == Cxx::REFERENCE_SELECT) || (op == Cxx::POINTER_SELECT))
      {
         Context::PushArg(StackArg(this, Last()));
         return;
      }
   }

   auto ref = Referent();
   if(ref != nullptr) Context::PushArg(ref->NameToArg(op, Last()));
}

//------------------------------------------------------------------------------

void QualName::GetDirectClasses(CxxUsageSets& symbols)
{
   Debug::ft("QualName.GetDirectClasses");

   Last()->GetDirectClasses(symbols);
}

//------------------------------------------------------------------------------

void QualName::GetDirectTemplateArgs(CxxUsageSets& symbols) const
{
   Debug::ft("QualName.GetDirectTemplateArgs");

   for(auto n = First(); n != nullptr; n = n->Next())
   {
      n->GetDirectTemplateArgs(symbols);
   }
}

//------------------------------------------------------------------------------

CxxScoped* QualName::GetForward() const
{
   Debug::ft("QualName.GetForward");

   CxxScoped* forw = nullptr;

   for(auto n = First(); n != nullptr; n = n->Next())
   {
      auto f = n->GetForward();
      if(f != nullptr) forw = f;
   }

   return forw;
}

//------------------------------------------------------------------------------

void QualName::GetNames(stringVector& names) const
{
   Debug::ft("QualName.GetNames");

   //  Add this name, without template arguments, to the list.
   //
   names.push_back(ScopedName(false));

   //  Include any template arguments attached to this name.
   //
   for(auto n = First(); n != nullptr; n = n->Next())
   {
      n->GetNames(names);
   }
}

//------------------------------------------------------------------------------

CxxScope* QualName::GetScope() const
{
   //  Currently, this is only invoked when CxxNamed::ResolveName is deciding
   //  whether to check if this name contains a redundant scope.  The current
   //  scope is usually the correct one, but we don't want to bother checking
   //  internally generated names, and we need to use the parser's enclosing
   //  scope when initializing data outside a function.
   //
   auto scope = Context::Scope();
   if(scope == nullptr) return nullptr;
   if(IsInternal()) return nullptr;
   if(!init_) return scope;
   return Context::OuterScope();
}

//------------------------------------------------------------------------------

bool QualName::GetSpan(size_t& begin, size_t& left, size_t& end) const
{
   Debug::ft("QualName.GetSpan");

   auto ref = Referent();
   if((ref == nullptr) || (ref->Type() != Cxx::Data)) return false;

   begin = GetPos();
   end = GetFile()->GetLexer().FindFirstOf(";", begin);
   return (end != string::npos);
}

//------------------------------------------------------------------------------

TypeName* QualName::GetTemplatedName() const
{
   for(auto n = First(); n != nullptr; n = n->Next())
   {
      auto spec = n->GetTemplatedName();
      if(spec != nullptr) return spec;
   }

   return nullptr;
}

//------------------------------------------------------------------------------

void QualName::GetUsages(const CodeFile& file, CxxUsageSets& symbols)
{
   //  Get the usages for each individual name.
   //
   for(auto n = First(); n != nullptr; n = n->Next())
   {
      n->GetUsages(file, symbols);
   }

   //  Add, as a direct usage, our referent or the typedef through which
   //  it was accessed.  Omit terminals and function arguments.
   //
   auto ref = DirectType();
   if(ref == nullptr) return;

   auto type = ref->Type();
   if((type == Cxx::Terminal) || (type == Cxx::Argument)) return;

   //  If the used item is in a template instance, find the corresponding
   //  item in the class template.
   //
   auto cls = ref->GetClass();

   if(cls != nullptr)
   {
      if(cls->IsInTemplateInstance())
      {
         ref = cls->FindTemplateAnalog(ref);
         if(ref == nullptr) return;
      }
   }

   //  If the item is a function, the referent could be an override, but
   //  only its original declaration needs to be accessible.
   //
   if(type == Cxx::Function)
   {
      auto func = static_cast<Function*>(ref);
      if(func->FuncRole() == FuncOther) ref = func->FindRootFunc();
   }

   symbols.AddDirect(ref);
}

//------------------------------------------------------------------------------

void QualName::Instantiating(CxxScopedVector& locals) const
{
   Debug::ft("QualName.Instantiating");

   //  This will add, to LOCALS, a template parameter that is nested within
   //  a template specialization.
   //
   for(auto n = First(); n != nullptr; n = n->Next())
   {
      n->Instantiating(locals);
   }
}

//------------------------------------------------------------------------------

bool QualName::ItemIsTemplateArg(const CxxNamed* item) const
{
   Debug::ft("QualName.ItemIsTemplateArg");

   for(auto n = First(); n != nullptr; n = n->Next())
   {
      if(n->ItemIsTemplateArg(item)) return true;
   }

   return false;
}

//------------------------------------------------------------------------------

TypeName* QualName::Last() const
{
   TypeName* prev = nullptr;

   for(auto curr = First(); curr != nullptr; curr = curr->Next())
   {
      prev = curr;
   }

   return prev;
}

//------------------------------------------------------------------------------

TypeMatch QualName::MatchTemplate(const QualName* that,
   stringVector& tmpltParms, stringVector& tmpltArgs, bool& argFound) const
{
   Debug::ft("QualName.MatchTemplate");

   auto match = Compatible;
   auto size = Size();
   auto n1 = this->First();
   auto n2 = that->First();

   for(size_t i = 0; i < size; ++i)
   {
      auto result = n1->MatchTemplate(n2, tmpltParms, tmpltArgs, argFound);
      if(result == Incompatible) return Incompatible;
      if(result < match) match = result;
      n1 = n1->Next();
      n2 = n2->Next();
   }

   return match;
}

//------------------------------------------------------------------------------

CxxToken* QualName::PosToItem(size_t pos) const
{
   auto item = CxxNamed::PosToItem(pos);
   if(item != nullptr) return item;

   for(auto n = First(); n != nullptr; n = n->Next())
   {
      item = n->PosToItem(pos);
      if(item != nullptr) return item;
   }

   return nullptr;
}

//------------------------------------------------------------------------------

void QualName::Print(ostream& stream, const Flags& options) const
{
   if(specifier_ != Cxx::NilSpecifier) stream << specifier_ << SPACE;

   for(auto n = First(); n != nullptr; n = n->Next())
   {
      n->Print(stream, options);
   }
}

//------------------------------------------------------------------------------

void QualName::PushBack(TypeNamePtr& name)
{
   Debug::ft("QualName.PushBack");

   name->SetQualName(this);

   if(first_ == nullptr)
      first_ = std::move(name);
   else
      Last()->PushBack(name);
}

//------------------------------------------------------------------------------

string QualName::QualifiedName(bool scopes, bool templates) const
{
   if(scopes)
   {
      //  Build the qualified name.
      //
      string qname;

      for(auto n = First(); n != nullptr; n = n->Next())
      {
         qname += n->QualifiedName(scopes, templates);
      }

      return qname;
   }

   //  Only the last name is wanted.
   //
   return Last()->QualifiedName(scopes, templates);
}

//------------------------------------------------------------------------------

string QualName::QualifyingScope() const
{
   Debug::ft("QualName.QualifyingScope");

   string qscope;

   for(auto n = First(); n->Next() != nullptr; n = n->Next())
   {
      qscope += n->QualifiedName(true, true);
   }

   return qscope;
}

//------------------------------------------------------------------------------

CxxScoped* QualName::Referent() const
{
   Debug::ft("QualName.Referent");

   //  This is invoked to find a referent in executable code.
   //
   auto ref = Last()->Referent();
   if(ref != nullptr) return ref;

   auto mask = CODE_REFS;
   SymbolView view;

   switch(specifier_)
   {
   case Cxx::ClassSpecifier:
   case Cxx::StructSpecifier:
   case Cxx::UnionSpecifier:
      mask = (CLASS_MASK | FORW_MASK | FRIEND_MASK);
      break;
   case Cxx::EnumSpecifier:
      mask = ENUM_MASK;
      break;
   }

   auto item = ResolveName(Context::File(), Context::Scope(), mask, view);
   if(item == nullptr) return ReferentError(QualifiedName(true, true), 0);

   //  Verify that the item has a referent in case it's a typedef or a
   //  forward declaration.
   //
   ref = item->Referent();
   if(ref == nullptr) return ReferentError(item->Trace(), item->Type());
   CheckIfTemplateArgument(ref);
   return ref;
}

//------------------------------------------------------------------------------

void QualName::Rename(const string& name)
{
   Debug::ft("QualName.Rename");

   Last()->Rename(name);
}

//------------------------------------------------------------------------------

bool QualName::ResolveTemplate(Class* cls, const TypeName* type, bool end) const
{
   Debug::ft("QualName.ResolveTemplate");

   //  If something within the template instance is being named,
   //  force its instantiation.
   //
   if(end) return true;
   auto inst = cls->EnsureInstance(type);
   if(inst == nullptr) return false;
   inst->Instantiate();
   return true;
}

//------------------------------------------------------------------------------

bool QualName::ResolveTypedef(Typedef* type, size_t n) const
{
   Debug::ft("QualName.ResolveTypedef");

   return At(n)->ResolveTypedef(type, n);
}

//------------------------------------------------------------------------------

void QualName::SetContext(size_t pos)
{
   Debug::ft("QualName.SetContext");

   CxxNamed::SetContext(pos);
   First()->SetContext(pos);
}

//------------------------------------------------------------------------------

void QualName::SetOperator(Cxx::Operator oper) const
{
   Debug::ft("QualName.SetOperator");

   Last()->SetOperator(oper);
}

//------------------------------------------------------------------------------

void QualName::SetReferent(CxxScoped* item, const SymbolView* view) const
{
   Debug::ft("QualName.SetReferent");

   Last()->SetReferent(item, view);
}

//------------------------------------------------------------------------------

void QualName::SetReferentN
   (size_t n, CxxScoped* item, const SymbolView* view) const
{
   Debug::ft("QualName.SetReferentN");

   At(n)->SetReferent(item, view);
}

//------------------------------------------------------------------------------

void QualName::SetTemplateArgs(const TemplateParms* tparms) const
{
   Debug::ft("QualName.SetTemplateArgs");

   Last()->SetTemplateArgs(tparms);
}

//------------------------------------------------------------------------------

size_t QualName::Size() const
{
   size_t s = 0;
   for(auto n = First(); n != nullptr; n = n->Next()) ++s;
   return s;
}

//------------------------------------------------------------------------------

string QualName::TypeString(bool arg) const
{
   auto ref = Referent();

   if(ref != nullptr)
   {
      auto type = Last()->DirectType();

      if(type != ref)
      {
         return type->TypeString(arg);
      }

      auto ts = ref->TypeString(arg);

      if(ref->IsTemplate())
      {
         ts += Last()->TypeString(arg);
      }

      return ts;
   }

   auto expl = "Failed to find referent for " + QualifiedName(true, true);
   Context::SwLog("QualName.TypeString", expl, 0);
   return ERROR_STR;
}

//------------------------------------------------------------------------------

void QualName::UpdatePos
   (EditorAction action, size_t begin, size_t count, size_t from) const
{
   CxxNamed::UpdatePos(action, begin, count, from);

   for(auto n = First(); n != nullptr; n = n->Next())
   {
      n->UpdatePos(action, begin, count, from);
   }
}

//------------------------------------------------------------------------------

void QualName::UpdateXref(bool insert)
{
   for(auto n = First(); n != nullptr; n = n->Next())
   {
      n->UpdateXref(insert);
   }
}

//------------------------------------------------------------------------------

bool QualName::VerifyReferents() const
{
   for(auto n = First(); n != nullptr; n = n->Next())
   {
      if(!n->VerifyReferents()) return false;
   }

   return true;
}

//==============================================================================

TemplateArg::TemplateArg(TypeSpecPtr& type) :
   spec_(std::move(type)),
   expr_(nullptr),
   implicit_(false)
{
   Debug::ft("TemplateArg.ctor(type)");

   spec_->SetTemplateRole(TemplateArgument);
}

//------------------------------------------------------------------------------

TemplateArg::TemplateArg(ExprPtr& expr) :
   spec_(nullptr),
   expr_(std::move(expr)),
   implicit_(false)
{
   Debug::ft("TemplateArg.ctor(expr)");
}

//------------------------------------------------------------------------------

TemplateArg::~TemplateArg()
{
   Debug::ftnt("TemplateArg.dtor");
}

//------------------------------------------------------------------------------

fn_name TemplateArg_AlignTemplateArg = "TemplateArg.AlignTemplateArg";

std::string TemplateArg::AlignTemplateArg(const TemplateArg* thatArg) const
{
   Debug::ft(TemplateArg_AlignTemplateArg);

   if((spec_ != nullptr) && (thatArg->spec_ != nullptr))
   {
      return spec_->AlignTemplateArg(thatArg->spec_.get());
   }

   Context::SwLog(TemplateArg_AlignTemplateArg, "needs implementation", 0);
   return ERROR_STR;
}

//------------------------------------------------------------------------------

string TemplateArg::ArgString(const TemplateParmToArgMap& tmap) const
{
   if(spec_ != nullptr)
   {
      return spec_->ArgString(tmap);
   }
   else if(expr_ != nullptr)
   {
      return TypeString(true);
   }

   Context::SwLog("TemplateArg.ArgString", "needs implementation", 0);
   return ERROR_STR;
}

//------------------------------------------------------------------------------

fn_name TemplateArg_Check = "TemplateArg.Check";

void TemplateArg::Check() const
{
   Debug::ftnt(TemplateArg_Check);

   if(implicit_) return;

   if(spec_ != nullptr)
   {
      spec_->Check();
   }
   else if(expr_ != nullptr)
   {
      expr_->Check();
   }
   else
   {
      Context::SwLog(TemplateArg_Check, "needs implementation", 0);
   }
}

//------------------------------------------------------------------------------

fn_name TemplateArg_Clone = "TemplateArg.Clone";

CxxToken* TemplateArg::Clone() const
{
   Debug::ft(TemplateArg_Clone);

   if(spec_ != nullptr)
   {
      TypeSpecPtr spec(static_cast<TypeSpec*>(spec_->Clone()));
      return new TemplateArg(spec);
   }
   else if(expr_ != nullptr)
   {
      ExprPtr expr(static_cast<Expression*>(expr_->Clone()));
      return new TemplateArg(expr);
   }

   Context::SwLog(TemplateArg_Clone, "needs implementation", 0);
   return nullptr;
}

//------------------------------------------------------------------------------

fn_name TemplateArg_CopyContext = "TemplateArg.CopyContext";

void TemplateArg::CopyContext(const CxxToken* that, bool internal)
{
   Debug::ft(TemplateArg_CopyContext);

   if(spec_ != nullptr)
   {
      spec_->CopyContext(that, internal);
   }
   else if(expr_ != nullptr)
   {
      expr_->CopyContext(that, internal);
   }
   else
   {
      Context::SwLog(TemplateArg_CopyContext, "needs implementation", 0);
   }
}

//------------------------------------------------------------------------------

fn_name TemplateArg_EnterBlock = "TemplateArg.EnterBlock";

void TemplateArg::EnterBlock()
{
   Debug::ft(TemplateArg_EnterBlock);

   if(spec_ != nullptr)
   {
      spec_->EnterBlock();
   }
   else if(expr_ != nullptr)
   {
      expr_->EnterBlock();
   }
   else
   {
      Context::SwLog(TemplateArg_EnterBlock, "needs implementation", 0);
   }
}

//------------------------------------------------------------------------------

void TemplateArg::EnteringScope(const CxxScope* scope) const
{
   Debug::ft("TemplateArg.EnteringScope");

   if(spec_ != nullptr)
   {
      spec_->EnteringScope(scope);
   }
}

//------------------------------------------------------------------------------

void TemplateArg::FindReferent()
{
   Debug::ft("TemplateArg.FindReferent");

   if(spec_ != nullptr)
   {
      spec_->FindReferent();
   }
}

//------------------------------------------------------------------------------

void TemplateArg::GetDirectClasses(CxxUsageSets& symbols)
{
   Debug::ft("TemplateArg.GetDirectClasses");

   if(implicit_) return;

   if(spec_ != nullptr)
   {
      spec_->GetDirectClasses(symbols);
   }
}

//------------------------------------------------------------------------------

void TemplateArg::GetDirectTemplateArgs(CxxUsageSets& symbols) const
{
   Debug::ft("TemplateArg.GetDirectTemplateArgs");

   if(implicit_) return;

   if(spec_ != nullptr)
   {
      spec_->GetDirectTemplateArgs(symbols);
   }
}

//------------------------------------------------------------------------------

void TemplateArg::GetNames(stringVector& names) const
{
   Debug::ft("TemplateArg.GetNames");

   if(implicit_) return;

   if(spec_ != nullptr)
   {
      spec_->GetNames(names);
   }
}

//------------------------------------------------------------------------------

CxxScoped* TemplateArg::GetReferent() const
{
   if(spec_ != nullptr)
   {
      return spec_->GetQualName()->GetReferent();
   }
   else if(expr_ != nullptr)
   {
      return expr_->Referent();
   }

   Context::SwLog("TemplateArg.GetReferent", "needs implementation", 0);
   return nullptr;
}

//------------------------------------------------------------------------------

void TemplateArg::GetUsages(const CodeFile& file, CxxUsageSets& symbols)
{
   if(implicit_) return;

   if(spec_ != nullptr)
   {
      spec_->GetUsages(file, symbols);
   }
   else if(expr_ != nullptr)
   {
      expr_->GetUsages(file, symbols);
   }
}

//------------------------------------------------------------------------------

void TemplateArg::Instantiating(CxxScopedVector& locals) const
{
   Debug::ft("TemplateArg.Instantiating");

   //  There is nothing to do for an expression, which must currently
   //  contain only a single Literal.
   //
   if(spec_ != nullptr)
   {
      spec_->Instantiating(locals);
   }
}

//------------------------------------------------------------------------------

bool TemplateArg::ItemIsTemplateArg(const CxxNamed* item) const
{
   Debug::ft("TemplateArg.ItemIsTemplateArg");

   if(spec_ != nullptr)
   {
      return spec_->ItemIsTemplateArg(item);
   }

   return false;
}

//------------------------------------------------------------------------------

fn_name TemplateArg_MatchTemplate = "TemplateArg.MatchTemplate";

TypeMatch TemplateArg::MatchTemplate(const TemplateArg* that,
   stringVector& tmpltParms, stringVector& tmpltArgs, bool& argFound) const
{
   Debug::ft(TemplateArg_MatchTemplate);

   if((spec_ != nullptr) && (that->spec_ != nullptr))
   {
      return spec_->MatchTemplate
         (that->spec_.get(), tmpltParms, tmpltArgs, argFound);
   }

   Context::SwLog(TemplateArg_MatchTemplate, "needs implementation", 0);
   return Incompatible;
}

//------------------------------------------------------------------------------

fn_name TemplateArg_MatchTemplateArg = "TemplateArg.MatchTemplateArg";

TypeMatch TemplateArg::MatchTemplateArg(const TemplateArg* that) const
{
   Debug::ft(TemplateArg_MatchTemplateArg);

   if((spec_ != nullptr) && (that->spec_ != nullptr))
   {
      return spec_->MatchTemplateArg(that->spec_.get());
   }

   Context::SwLog(TemplateArg_MatchTemplateArg, "needs implementation", 0);
   return Incompatible;
}

//------------------------------------------------------------------------------

const std::string& TemplateArg::Name() const
{
   if(spec_ != nullptr)
   {
      return spec_->Name();
   }
   else if(expr_ != nullptr)
   {
      return expr_->Name();
   }

   Context::SwLog("TemplateArg.Name", "needs implementation", 0);
   return CxxNamed::Name();
}

//------------------------------------------------------------------------------

fn_name TemplateArg_NamesReferToArgs = "TemplateArg.NamesReferToArgs";

bool TemplateArg::NamesReferToArgs(const NameVector& names,
   const CxxScope* scope, CodeFile* file, size_t& index) const
{
   Debug::ft(TemplateArg_NamesReferToArgs);

   if(spec_ != nullptr)
   {
      return spec_->NamesReferToArgs(names, scope, file, index);
   }
   else if(expr_ != nullptr)
   {
      if(index >= names.size()) return false;
      if(names.at(index).name != expr_->Name()) return false;
      ++index;
      return true;
   }

   Context::SwLog(TemplateArg_NamesReferToArgs, "needs implementation", 0);
   return false;
}

//------------------------------------------------------------------------------

CxxToken* TemplateArg::PosToItem(size_t pos) const
{
   if(implicit_) return nullptr;

   if(spec_ != nullptr)
   {
      return spec_->PosToItem(pos);
   }
   else if(expr_ != nullptr)
   {
      return expr_->PosToItem(pos);
   }

   Context::SwLog("TemplateArg.PosToItem", "needs implementation", 0);
   return nullptr;
}

//------------------------------------------------------------------------------

void TemplateArg::Print
   (std::ostream& stream, const NodeBase::Flags& options) const
{
   //  Omit implicit arguments unless generating code for internal compilation.
   //
   if(implicit_ && !options.test(DispCode)) return;

   if(spec_ != nullptr)
   {
      spec_->Print(stream, options);
   }
   else if(expr_ != nullptr)
   {
      expr_->Print(stream, options);
   }
   else
   {
      Context::SwLog("TemplateArg.Print", "needs implementation", 0);
   }
}

//------------------------------------------------------------------------------

string TemplateArg::QualifiedName(bool scopes, bool templates) const
{
   if(spec_ != nullptr)
   {
      return spec_->QualifiedName(scopes, templates);
   }
   else if(expr_ != nullptr)
   {
      return expr_->Name();
   }

   Context::SwLog("TemplateArg.QualifiedName", "needs implementation", 0);
   return ERROR_STR;
}

//------------------------------------------------------------------------------

fn_name TemplateArg_ResultType = "TemplateArg.ResultType";

StackArg TemplateArg::ResultType() const
{
   Debug::ft(TemplateArg_ResultType);

   if(spec_ != nullptr)
   {
      return spec_->ResultType();
   }
   else if(expr_ != nullptr)
   {
      return expr_->ResultType();
   }

   Context::SwLog(TemplateArg_ResultType, "needs implementation", 0);
   return NilStackArg;
}

//------------------------------------------------------------------------------

void TemplateArg::SetTemplateRole(TemplateRole role) const
{
   Debug::ft("TemplateArg.SetTemplateRole");

   if(spec_ != nullptr)
   {
      spec_->SetTemplateRole(role);
   }
}

//------------------------------------------------------------------------------

void TemplateArg::SetUserType(TypeSpecUser user) const
{
   Debug::ft("TemplateArg.SetUserType");

   if(spec_ != nullptr)
   {
      spec_->SetUserType(user);
   }
}

//------------------------------------------------------------------------------

std::string TemplateArg::Trace() const
{
   if(spec_ != nullptr)
   {
      return spec_->Trace();
   }
   else if(expr_ != nullptr)
   {
      return expr_->Trace();
   }

   Context::SwLog("TemplateArg.Trace", "needs implementation", 0);
   return ERROR_STR;
}

//------------------------------------------------------------------------------

std::string TemplateArg::TypeString(bool arg) const
{
   if(spec_ != nullptr)
   {
      return spec_->TypeString(arg);
   }
   else if(expr_ != nullptr)
   {
      std::ostringstream stream;
      Flags flags(NS_Mask | Code_Mask | NoTP_Mask);
      expr_->Print(stream, flags);
      return stream.str();
   }

   Context::SwLog("TemplateArg.TypeString", "needs implementation", 0);
   return ERROR_STR;
}

//------------------------------------------------------------------------------

void TemplateArg::UpdatePos(EditorAction action,
   size_t begin, size_t count, size_t from) const
{
   if(implicit_) return;

   CxxNamed::UpdatePos(action, begin, count, from);

   if(spec_ != nullptr)
   {
      spec_->UpdatePos(action, begin, count, from);
   }
   else if(expr_ != nullptr)
   {
      expr_->UpdatePos(action, begin, count, from);
   }
   else
   {
      Context::SwLog("TemplateArg.UpdatePos", "needs implementation", 0);
   }
}

//------------------------------------------------------------------------------

void TemplateArg::UpdateXref(bool insert)
{
   if(implicit_) return;

   if(spec_ != nullptr)
   {
      spec_->UpdateXref(insert);
   }
   else if(expr_ != nullptr)
   {
      expr_->UpdateXref(insert);
   }
}

//------------------------------------------------------------------------------

bool TemplateArg::VerifyReferents() const
{
   return (spec_ != nullptr ? spec_->VerifyReferents() : true);
}

//==============================================================================

TypeName::TypeName(string& name) :
   qname_(nullptr),
   args_(nullptr),
   ref_(nullptr),
   class_(nullptr),
   type_(nullptr),
   forw_(nullptr),
   oper_(Cxx::NIL_OPERATOR),
   control_(Cxx::Access_N),
   scoped_(false),
   using_(false),
   direct_(false)
{
   Debug::ft("TypeName.ctor");

   std::swap(name_, name);
}

//------------------------------------------------------------------------------

TypeName::~TypeName()
{
   Debug::ftnt("TypeName.dtor");

   UpdateXref(false);
}

//------------------------------------------------------------------------------

TypeName::TypeName(const TypeName& that) : CxxNamed(that),
   name_(that.name_),
   qname_(nullptr),
   ref_(that.ref_),
   class_(that.class_),
   type_(that.type_),
   forw_(that.forw_),
   oper_(that.oper_),
   control_(Cxx::Access_N),
   scoped_(that.scoped_),
   using_(that.using_),
   direct_(that.direct_)
{
   Debug::ft("TypeName.ctor(copy)");

   if(that.args_ != nullptr)
   {
      args_.reset(new TemplateArgPtrVector);

      for(auto a = that.args_->cbegin(); a != that.args_->cend(); ++a)
      {
         TemplateArgPtr arg(static_cast<TemplateArg*>((*a)->Clone()));
         arg->CopyContext(a->get(), true);
         args_->push_back(std::move(arg));
      }
   }
}

//------------------------------------------------------------------------------

void TypeName::AddTemplateArg(TemplateArgPtr& arg)
{
   Debug::ft("TypeName.AddTemplateArg");

   if(args_ == nullptr) args_.reset(new TemplateArgPtrVector);
   args_->push_back(std::move(arg));
}

//------------------------------------------------------------------------------

void TypeName::Append(const string& name, bool space)
{
   Debug::ft("TypeName.Append");

   if(space) name_ += SPACE;
   name_ += name;
}

//------------------------------------------------------------------------------

const TemplateArgPtrVector* TypeName::Args() const
{
   if(type_ != nullptr)
   {
      auto args = type_->Args();
      if(args != nullptr) return args;
   }

   return args_.get();
}

//------------------------------------------------------------------------------

string TypeName::ArgString(const TemplateParmToArgMap& tmap) const
{
   string args;

   if(args_ != nullptr)
   {
      args += '<';

      for(auto a = args_->cbegin(); a != args_->cend(); ++a)
      {
         args += (*a)->ArgString(tmap);
         if(*a != args_->back()) args += ',';
      }

      args += '>';
   }

   return args;
}

//------------------------------------------------------------------------------

void TypeName::Check() const
{
   if(args_ != nullptr)
   {
      for(auto a = args_->cbegin(); a != args_->cend(); ++a)
      {
         (*a)->Check();
      }
   }
}

//------------------------------------------------------------------------------

void TypeName::Delete()
{
   Debug::ft("TypeName.Delete");

   qname_->DeleteName(this, next_);
}

//------------------------------------------------------------------------------

CxxScoped* TypeName::DirectType() const
{
   return (type_ != nullptr ? type_ : ref_);
}

//------------------------------------------------------------------------------

void TypeName::FindReferent()
{
   Debug::ft("TypeName.FindReferent");

   if(args_ != nullptr)
   {
      for(auto a = args_->cbegin(); a != args_->cend(); ++a)
      {
         (*a)->FindReferent();
      }
   }
}

//------------------------------------------------------------------------------

void TypeName::GetDirectClasses(CxxUsageSets& symbols)
{
   Debug::ft("TypeName.GetDirectClasses");

   auto ref = DirectType();

   if(ref != nullptr) ref->GetDirectClasses(symbols);

   if(args_ != nullptr)
   {
      for(auto a = args_->cbegin(); a != args_->cend(); ++a)
      {
         (*a)->GetDirectClasses(symbols);
      }
   }
}

//------------------------------------------------------------------------------

void TypeName::GetDirectTemplateArgs(CxxUsageSets& symbols) const
{
   Debug::ft("TypeName.GetDirectTemplateArgs");

   if(args_ != nullptr)
   {
      for(auto a = args_->cbegin(); a != args_->cend(); ++a)
      {
         (*a)->GetDirectTemplateArgs(symbols);
      }
   }
}

//------------------------------------------------------------------------------

void TypeName::GetNames(stringVector& names) const
{
   Debug::ft("TypeName.GetNames");

   if(args_ != nullptr)
   {
      for(auto a = args_->cbegin(); a != args_->cend(); ++a)
      {
         (*a)->GetNames(names);
      }
   }
}

//------------------------------------------------------------------------------

bool TypeName::GetSpan(size_t& begin, size_t& left, size_t& end) const
{
   Debug::ft("TypeName.GetSpan");

   const auto& source = GetFile()->GetLexer().Source();

   begin = GetPos();

   if(next_ != nullptr)
   {
      end = source.find(SCOPE_STR, begin);
      if(end == string::npos) return false;
      end += 1;
   }
   else
   {
      end = source.find_first_not_of(ValidNextChars, begin);
      if(end == string::npos) return false;
      end -= 1;
   }

   return true;
}

//------------------------------------------------------------------------------

TypeName* TypeName::GetTemplatedName() const
{
   if(type_ != nullptr)
   {
      auto name = type_->GetTemplatedName();
      if(name != nullptr) return name;
   }

   if(args_ == nullptr) return nullptr;
   return const_cast<TypeName*>(this);
}

//------------------------------------------------------------------------------

void TypeName::GetUsages(const CodeFile& file, CxxUsageSets& symbols)
{
   if(direct_) GetDirectClasses(symbols);

   //  Currently, this does not report usages based on ref_ or type_.
   //  If it did,  DataSpec::GetUsages would need a way to suppress or
   //  bypass it, because a name doesn't know whether its ref_ or type_
   //  was used directly or indirectly.
   //
   if(args_ != nullptr)
   {
      for(auto a = args_->cbegin(); a != args_->cend(); ++a)
      {
         (*a)->GetUsages(file, symbols);
      }
   }

   auto cls = class_;

   if(cls != nullptr)
   {
      if(cls->IsInTemplateInstance()) cls = cls->GetClassTemplate();
      if(cls->GetFile() != &file) symbols.AddDirect(cls);
   }

   if(forw_ != nullptr) symbols.AddForward(forw_);
   if(using_) symbols.AddUser(this);
}

//------------------------------------------------------------------------------

bool TypeName::HasTemplateParmFor(const CxxScope* scope) const
{
   Debug::ft("TypeName.HasTemplateParmFor");

   if(args_ != nullptr)
   {
      for(auto a = args_->cbegin(); a != args_->cend(); ++a)
      {
         if(scope->NameToTemplateParm((*a)->Name()) != nullptr) return true;
      }
   }

   return false;
}

//------------------------------------------------------------------------------

void TypeName::Instantiating(CxxScopedVector& locals) const
{
   Debug::ft("TypeName.Instantiating");

   if(type_ != nullptr)
   {
      type_->Instantiating(locals);
   }

   if(args_ != nullptr)
   {
      for(auto a = args_->cbegin(); a != args_->cend(); ++a)
      {
         (*a)->Instantiating(locals);
      }
   }
}

//------------------------------------------------------------------------------

void TypeName::ItemDeleted(const CxxScoped* item) const
{
   if(ref_ == item)
   {
      ref_ = nullptr;
   }

   if(type_ == item)
   {
      type_ = nullptr;
   }
}

//------------------------------------------------------------------------------

fn_name TypeName_ItemIsTemplateArg = "TypeName.ItemIsTemplateArg";

bool TypeName::ItemIsTemplateArg(const CxxNamed* item) const
{
   Debug::ft(TypeName_ItemIsTemplateArg);

   if(item == nullptr)
   {
      Debug::SwLog(TypeName_ItemIsTemplateArg, "null item", 0);
      return false;
   }

   if(type_ != nullptr)
   {
      if(type_->ItemIsTemplateArg(item)) return true;
   }

   if(args_ != nullptr)
   {
      for(auto a = args_->cbegin(); a != args_->cend(); ++a)
      {
         if((*a)->ItemIsTemplateArg(item)) return true;
      }
   }

   if(ref_ != nullptr)
   {
      if(ref_ == item) return true;

      auto rname = ref_->ScopedName(true);
      auto iname = item->ScopedName(true);
      if(rname == iname) return true;

      auto spec = ref_->GetTypeSpec();
      if((spec != nullptr) && spec->ItemIsTemplateArg(item)) return true;

      if((ref_->Type() == Cxx::Enumerator) && (item->Type() == Cxx::Enum))
      {
         auto e = static_cast<const Enum*>(item);
         if(e->FindEnumerator(ref_->Name()) == ref_) return true;
      }
   }

   return false;
}

//------------------------------------------------------------------------------

fn_name TypeName_MatchTemplate = "TypeName.MatchTemplate";

TypeMatch TypeName::MatchTemplate(const TypeName* that,
   stringVector& tmpltParms, stringVector& tmpltArgs, bool& argFound) const
{
   Debug::ft(TypeName_MatchTemplate);

   if(type_ != nullptr)
   {
      Context::SwLog(TypeName_MatchTemplate, "forward to type_?", 0);
   }

   if(this->args_ == nullptr) return Compatible;
   auto thisSize = this->args_->size();
   if(thisSize == 0) return Compatible;
   if(thisSize != that->args_->size()) return Incompatible;

   auto match = Compatible;

   for(size_t i = 0; i < thisSize; ++i)
   {
      auto result = this->args_->at(i)->MatchTemplate
         (that->args_->at(i).get(), tmpltParms, tmpltArgs, argFound);
      if(result == Incompatible) return Incompatible;
      if(result < match) match = result;
   }

   return match;
}

//------------------------------------------------------------------------------

void TypeName::MemberAccessed(Class* cls, CxxScoped* mem) const
{
   Debug::ft("TypeName.MemberAccessed");

   ref_ = mem;
   class_ = cls;
}

//------------------------------------------------------------------------------

bool TypeName::NamesReferToArgs(const NameVector& names,
   const CxxScope* scope, CodeFile* file, size_t& index) const
{
   Debug::ft("TypeName.NamesReferToArgs");

   if(args_ != nullptr)
   {
      for(auto a = args_->cbegin(); a != args_->cend(); ++a)
      {
         if(!(*a)->NamesReferToArgs(names, scope, file, index)) return false;
      }
   }

   if(type_ != nullptr)
   {
      return type_->NamesReferToArgs(names, scope, file, index);
   }

   return true;
}

//------------------------------------------------------------------------------

CxxToken* TypeName::PosToItem(size_t pos) const
{
   auto item = CxxNamed::PosToItem(pos);
   if(item != nullptr) return item;

   if(args_ != nullptr)
   {
      for(auto a = args_->cbegin(); a != args_->cend(); ++a)
      {
         item = (*a)->PosToItem(pos);
         if(item != nullptr) return item;
      }
   }

   return nullptr;
}

//------------------------------------------------------------------------------

void TypeName::Print(ostream& stream, const Flags& options) const
{
   if(scoped_) stream << SCOPE_STR;
   stream << Name();

   if(args_ != nullptr)
   {
      stream << '<';

      for(auto a = args_->cbegin(); a != args_->cend(); ++a)
      {
         (*a)->Print(stream, options);
         if(*a != args_->back()) stream << ", ";
      }

      stream << '>';
   }
}

//------------------------------------------------------------------------------

void TypeName::PushBack(TypeNamePtr& type)
{
   next_ = std::move(type);
}

//------------------------------------------------------------------------------

string TypeName::QualifiedName(bool scopes, bool templates) const
{
   string qname = (scoped_ && scopes ? SCOPE_STR : EMPTY_STR);
   qname += name_;

   if(!templates || (args_ == nullptr)) return qname;

   qname += '<';

   for(auto a = args_->cbegin(); a != args_->cend(); ++a)
   {
      qname += (*a)->QualifiedName(scopes, templates);
      if(*a != args_->back()) qname += ',';
   }

   return qname + '>';
}

//------------------------------------------------------------------------------

void TypeName::Rename(const string& name)
{
   Debug::ft("TypeName.Rename");

   CxxNamed::RenameNonQual(name_, name);
}

//------------------------------------------------------------------------------

bool TypeName::ResolveTypedef(Typedef* type, size_t n) const
{
   Debug::ft("TypeName.ResolveTypedef");

   type_ = type;
   return true;
}

//------------------------------------------------------------------------------

void TypeName::SetForward(CxxScoped* decl) const
{
   Debug::ft("TypeName.SetForward");

   forw_ = decl;
}

//------------------------------------------------------------------------------

void TypeName::SetNext(TypeNamePtr& next)
{
   Debug::ft("TypeName.SetNext");

   next_.release();
   next_ = std::move(next);
}

//------------------------------------------------------------------------------

void TypeName::SetOperator(Cxx::Operator oper)
{
   Debug::ft("TypeName.SetOperator");

   switch(oper)
   {
   case Cxx::NIL_OPERATOR:
   case Cxx::CAST:
      //
      //  This either isn't an operator, or it's a conversion operator.  The
      //  name doesn't change in either case.  For the latter, the name is
      //  simply left as "operator", which will display as operator type()
      //  rather than operator() type().
      //
      break;

   case Cxx::LESS:
      if(oper_ == Cxx::LEFT_SHIFT)
      {
         //  This occurs when operator<< was really operator< followed by a
         //  template argument.  The name is currently operator<<, which will
         //  match operator< at position 0 in the default clause below, so it
         //  has to be fixed here.
         //
         name_.erase(name_.find('<'), 1);
      }
      [[fallthrough]];
   default:
      if(oper != Cxx::NIL_OPERATOR)
      {
         //  For a function template instance, the name already includes the
         //  operator.  The template arguments have also been appended to the
         //  name, so leave it alone.
         //
         auto name = CxxOp::OperatorToName(oper);

         if(name_.rfind(name, 0) != 0)
         {
            name_ = CxxOp::OperatorToName(oper);
         }
      }
   }

   oper_ = oper;
}

//------------------------------------------------------------------------------

fn_name TypeName_SetReferent = "TypeName.SetReferent";

void TypeName::SetReferent(CxxScoped* item, const SymbolView* view) const
{
   Debug::ft(TypeName_SetReferent);

   //  This can be invoked more than once when a class template name clears
   //  its referent, instead of leaving it as a forward declaration, so that
   //  the referent can later be set to a class template instance.  When this
   //  occurs, this function is also reinvoked on template arguments.  If an
   //  argument's name was already resolved, however, its using_ flag should
   //  not be set by a subsequent invocation.
   //
   if(view != nullptr)
   {
      control_ = view->control_;
      if(view->using_ && (ref_ == nullptr)) using_ = true;
   }

   //  Don't overwrite a function template with one of its instances.
   //
   if((ref_ != nullptr) && (ref_->Type() == Cxx::Function) &&
      (item != nullptr) && item->IsInTemplateInstance())
   {
      return;
   }

   ref_ = item;
   if(ref_ == nullptr) return;

   //  If the type has template arguments, add any that were defaulted.  This
   //  needs to refer to previous arguments.  For example, vector's signature
   //  is vector<T,A=allocator<T>>.  If given vector<unique_ptr<T>>, vector's
   //  "T" is unique_ptr<T>, so its allocator<T> is allocator<unique_ptr<T>>.
   //
   if(args_ == nullptr) return;
   auto tparms = ref_->GetTemplateParms();
   if(tparms == nullptr) return;

   auto parms = tparms->Parms();
   auto argSize = args_->size();
   auto parmSize = parms->size();
   if(argSize == parmSize) return;

   TemplateParmToArgMap map;
   BuildParameterMap(parms, args_.get(), map);

   for(auto i = argSize; i < parmSize; ++i)
   {
      auto def = parms->at(i)->Default();

      if(def == nullptr)
      {
         Context::SwLog(TypeName_SetReferent, "default argument expected", i);
         return;
      }

      auto defStr = def->ArgString(map);
      TemplateArgPtr defArg;
      auto scope = Context::Scope();
      ParserPtr parser(new Parser(scope));
      parser->ParseTemplateArg(defStr, defArg);
      defArg->CopyContext(def, true);
      defArg->SetImplicit();
      args_->push_back(std::move(defArg));
   }
}

//------------------------------------------------------------------------------

void TypeName::SetTemplateArgs(const TemplateParms* tparms)
{
   Debug::ft("TypeName.SetTemplateArgs");

   auto parms = tparms->Parms();

   for(auto p = parms->cbegin(); p != parms->cend(); ++p)
   {
      TypeSpecPtr spec(new DataSpec((*p)->Name().c_str()));
      spec->CopyContext(this, true);
      TemplateArgPtr arg(new TemplateArg(spec));
      AddTemplateArg(arg);
   }
}

//------------------------------------------------------------------------------

void TypeName::SetTemplateRole(TemplateRole role) const
{
   Debug::ft("TypeName.SetTemplateRole");

   if(args_ != nullptr)
   {
      for(auto a = args_->cbegin(); a != args_->cend(); ++a)
      {
         (*a)->SetTemplateRole(role);
      }
   }
}

//------------------------------------------------------------------------------

void TypeName::SetUserType(TypeSpecUser user) const
{
   Debug::ft("TypeName.SetUserType");

   if(args_ != nullptr)
   {
      for(auto a = args_->cbegin(); a != args_->cend(); ++a)
      {
         (*a)->SetUserType(user);
      }
   }
}

//------------------------------------------------------------------------------

void TypeName::SubclassAccess(Class* cls) const
{
   Debug::ft("TypeName.SubclassAccess");

   class_ = cls;
}

//------------------------------------------------------------------------------

string TypeName::TypeString(bool arg) const
{
   if(args_ == nullptr) return EMPTY_STR;

   string ts = "<";

   for(auto a = args_->cbegin(); a != args_->cend(); ++a)
   {
      ts += (*a)->TypeString(false);
      if(*a != args_->back()) ts += ',';
   }

   return ts + '>';
}

//------------------------------------------------------------------------------

void TypeName::UpdatePos
   (EditorAction action, size_t begin, size_t count, size_t from) const
{
   CxxNamed::UpdatePos(action, begin, count, from);

   if(args_ != nullptr)
   {
      for(auto a = args_->cbegin(); a != args_->cend(); ++a)
      {
         (*a)->UpdatePos(action, begin, count, from);
      }
   }
}

//------------------------------------------------------------------------------

void TypeName::UpdateXref(bool insert)
{
   if(ref_ != nullptr)
   {
      ref_->UpdateReference(this, insert);

      //  If the referent is in a template instance, also record a reference
      //  to the analogous item in the template.  A template class instance
      //  (e.g. basic_string) is often accessed through a typedef ("string"),
      //  so make sure the reference is recorded against the correct item.
      //
      if(ref_->IsInternal())
      {
         auto item = ref_->FindTemplateAnalog(ref_);

         if(item != nullptr)
         {
            if(item->Name() == name_)
               item->UpdateReference(this, insert);
            else if((type_ != nullptr) && (type_->Name() == name_))
               type_->UpdateReference(this, insert);
         }
      }
   }
   else
   {
      //  Record this unresolved item in case it is one that a template
      //  needs to have resolved by a template instance.
      //
      if(!IsInternal() && insert) Context::PushXrefItem(this);
   }

   if(args_ != nullptr)
   {
      for(auto a = args_->cbegin(); a != args_->cend(); ++a)
      {
         (*a)->UpdateXref(insert);
      }
   }

   if(type_ != nullptr)
   {
      type_->UpdateReference(this, insert);
   }
}

//------------------------------------------------------------------------------

bool TypeName::VerifyReferents() const
{
   if(ref_ == nullptr)
      return false;

   if(args_ != nullptr)
   {
      for(auto a = args_->cbegin(); a != args_->cend(); ++a)
      {
         if(!(*a)->VerifyReferents()) return false;
      }
   }

   return true;
}

//==============================================================================

TypeSpec::TypeSpec() :
   user_(TS_Unspecified),
   role_(TemplateNone)
{
   Debug::ft("TypeSpec.ctor");
}

//------------------------------------------------------------------------------

TypeSpec::TypeSpec(const TypeSpec& that) : CxxNamed(that),
   user_(that.user_),
   role_(that.role_)
{
   Debug::ft("TypeSpec.ctor(copy)");
}

//------------------------------------------------------------------------------

fn_name TypeSpec_PureVirtualFunction = "TypeSpec.PureVirtualFunction";

//------------------------------------------------------------------------------

void TypeSpec::AddArray(ArraySpecPtr& array)
{
   Debug::SwLog(TypeSpec_PureVirtualFunction, "AddArray", 0);
}

//------------------------------------------------------------------------------

string TypeSpec::AlignTemplateArg(const TypeSpec* thatArg) const
{
   Debug::SwLog(TypeSpec_PureVirtualFunction, "AlignTemplateArg", 0);
   return ERROR_STR;
}

//------------------------------------------------------------------------------

TagCount TypeSpec::Arrays() const
{
   Debug::SwLog(TypeSpec_PureVirtualFunction, "Arrays", 0);
   return 0;
}

//------------------------------------------------------------------------------

void TypeSpec::DisplayArrays(ostream& stream) const
{
   Debug::SwLog(TypeSpec_PureVirtualFunction, "DisplayArrays", 0);
}

//------------------------------------------------------------------------------

void TypeSpec::DisplayTags(ostream& stream) const
{
   Debug::SwLog(TypeSpec_PureVirtualFunction, "DisplayTags", 0);
}

//------------------------------------------------------------------------------

void TypeSpec::EnterArrays() const
{
   Debug::SwLog(TypeSpec_PureVirtualFunction, "EnterArrays", 0);
}

//------------------------------------------------------------------------------

void TypeSpec::EnteringScope(const CxxScope* scope)
{
   Debug::SwLog(TypeSpec_PureVirtualFunction, "EnteringScope", 0);
}

//------------------------------------------------------------------------------

TypeTags TypeSpec::GetAllTags() const
{
   TypeTags tags;

   Debug::SwLog(TypeSpec_PureVirtualFunction, "GetAllTags", 0);
   return tags;
}

//------------------------------------------------------------------------------

void TypeSpec::GetNames(stringVector& names) const
{
   Debug::SwLog(TypeSpec_PureVirtualFunction, "GetNames", 0);
}

//------------------------------------------------------------------------------

bool TypeSpec::HasArrayDefn() const
{
   Debug::SwLog(TypeSpec_PureVirtualFunction, "HasArrayDefn", 0);
   return false;
}

//------------------------------------------------------------------------------

bool TypeSpec::MatchesExactly(const TypeSpec* that) const
{
   Debug::SwLog(TypeSpec_PureVirtualFunction, "MatchesExactly", 0);
   return false;
}

//------------------------------------------------------------------------------

TypeMatch TypeSpec::MatchTemplate(const TypeSpec* that,
   stringVector& tmpltParms, stringVector& tmpltArgs, bool& argFound) const
{
   Debug::SwLog(TypeSpec_PureVirtualFunction, "MatchTemplate", 0);
   return Incompatible;
}

//------------------------------------------------------------------------------

TypeMatch TypeSpec::MatchTemplateArg(const TypeSpec* that) const
{
   Debug::SwLog(TypeSpec_PureVirtualFunction, "MatchTemplateArg", 0);
   return Incompatible;
}

//------------------------------------------------------------------------------

fn_name TypeSpec_MustMatchWith = "TypeSpec.MustMatchWith";

TypeMatch TypeSpec::MustMatchWith(const StackArg& that) const
{
   Debug::ft(TypeSpec_MustMatchWith);

   auto thisType = this->TypeString(true);
   auto thatType = that.TypeString(true);
   auto match = ResultType().CalcMatchWith(that, thisType, thatType);

   if(match == Incompatible)
   {
      auto expl = thisType + " is incompatible with " + thatType;
      Context::SwLog(TypeSpec_MustMatchWith, expl, 0);
   }
   else if((match == Abridgeable) || (match == Promotable))
   {
      if((this->Name() == BOOL_STR) || that.IsBool())
      {
         Context::Log(BoolMixedWithNumeric);
         that.item_->Log(BoolMixedWithNumeric, that.item_, -1);
      }
   }

   return match;
}

//------------------------------------------------------------------------------

TagCount TypeSpec::Ptrs(bool arrays) const
{
   Debug::SwLog(TypeSpec_PureVirtualFunction, "Ptrs", 0);
   return 0;
}

//------------------------------------------------------------------------------

TagCount TypeSpec::Refs() const
{
   Debug::SwLog(TypeSpec_PureVirtualFunction, "Refs", 0);
   return 0;
}

//------------------------------------------------------------------------------

void TypeSpec::SetPtrs(TagCount count)
{
   Debug::SwLog(TypeSpec_PureVirtualFunction, "SetPtrs", 0);
}

//------------------------------------------------------------------------------

const TypeTags* TypeSpec::Tags() const
{
   Debug::SwLog(TypeSpec_PureVirtualFunction, "Tags", 0);
   return nullptr;
}

//------------------------------------------------------------------------------

TypeTags* TypeSpec::Tags()
{
   Debug::SwLog(TypeSpec_PureVirtualFunction, "Tags", 1);
   return nullptr;
}

//------------------------------------------------------------------------------

string TypeSpec::TypeTagsString(const TypeTags& tags) const
{
   Debug::SwLog(TypeSpec_PureVirtualFunction, "TypeTagsString", 0);
   return ERROR_STR;
}

//==============================================================================

TypeTags::TypeTags() :
   ptrDet_(false),
   refDet_(false),
   const_(false),
   volatile_(false),
   array_(false),
   arrays_(0),
   ptrs_(0),
   refs_(0),
   constPtr_(0),
   volatilePtr_(0)
{
   Debug::ft("TypeTags.ctor");
}

//------------------------------------------------------------------------------

TypeTags::TypeTags(const TypeSpec& spec) :
   ptrDet_(false),
   refDet_(false),
   const_(spec.IsConst()),
   volatile_(spec.IsVolatile()),
   array_(spec.Tags()->IsUnboundedArray()),
   arrays_(spec.Arrays()),
   ptrs_(spec.Ptrs(false)),
   refs_(spec.Refs()),
   constPtr_(0),
   volatilePtr_(0)
{
   Debug::ft("TypeTags.ctor(TypeSpec)");

   for(auto i = 0; i < ptrs_; ++i)
   {
      SetPointer(i, spec.IsConstPtr(i), spec.IsVolatilePtr(i));
   }
}

//------------------------------------------------------------------------------

bool TypeTags::AlignTemplateTags(const TypeTags& that)
{
   Debug::ft("TypeTags.AlignTemplateTags");

   if(this->ptrs_ < that.ptrs_) return false;
   if(this->arrays_ < that.arrays_) return false;
   this->ptrs_ -= that.ptrs_;
   this->arrays_ -= that.arrays_;
   return true;
}

//------------------------------------------------------------------------------

TagCount TypeTags::ArrayCount() const
{
   Debug::ft("TypeTags.ArrayCount");

   auto count = arrays_;
   if(array_) ++count;
   return count;
}

//------------------------------------------------------------------------------

int TypeTags::IsConstPtr() const
{
   if(ptrs_ <= 0) return 0;
   auto mask = 1 << (ptrs_ - 1);
   return (((constPtr_ & mask) != 0) ? 1 : -1);
}

//------------------------------------------------------------------------------

bool TypeTags::IsConstPtr(size_t n) const
{
   if(TagCount(n) >= ptrs_) return false;
   auto mask = 1 << n;
   return ((constPtr_ & mask) != 0);
}

//------------------------------------------------------------------------------

bool TypeTags::IsLvalue() const
{
   if(refs_ != 1) return false;
   auto count = ptrs_ + arrays_;
   if(array_) ++count;
   return (count == 0);
}

//------------------------------------------------------------------------------

int TypeTags::IsVolatilePtr() const
{
   if(ptrs_ <= 0) return 0;
   auto mask = 1 << (ptrs_ - 1);
   return (((volatilePtr_ & mask) != 0) ? 1 : -1);
}

//------------------------------------------------------------------------------

bool TypeTags::IsVolatilePtr(size_t n) const
{
   if(TagCount(n) >= ptrs_) return false;
   auto mask = 1 << n;
   return ((volatilePtr_ & mask) != 0);
}

//------------------------------------------------------------------------------

TypeMatch TypeTags::MatchTemplateTags(const TypeTags& that) const
{
   Debug::ft("TypeTags.MatchTemplateTags");

   if(this->ptrs_ > that.ptrs_) return Incompatible;
   if(this->arrays_ > that.arrays_) return Incompatible;
   if(this->ptrs_ < that.ptrs_) return Convertible;
   if(this->arrays_ < that.arrays_) return Convertible;
   return Compatible;
}

//------------------------------------------------------------------------------

void TypeTags::Print(ostream& stream) const
{
   //  This is used to display code, so arrays_ is ignored because
   //  array specifications will follow the name of the data item.
   //
   if(array_) stream << ARRAY_STR;

   for(auto i = 0; i < ptrs_; ++i)
   {
      stream << '*';
      if(IsConstPtr(i)) stream << " const";
   }

   if(refs_ > 0) stream << string(refs_, '&');
}

//------------------------------------------------------------------------------

TagCount TypeTags::PtrCount(bool arrays) const
{
   if(!arrays) return ptrs_;
   auto count = ptrs_ + arrays_;
   if(array_) ++count;
   return count;
}

//------------------------------------------------------------------------------

fn_name TypeTags_SetConstPtr = "TypeTags.SetConstPtr";

void TypeTags::SetConstPtr() const
{
   Debug::ft(TypeTags_SetConstPtr);

   if(ptrs_ > 0)
      constPtr_ |= (1 << (ptrs_ - 1));
   else
      Context::SwLog(TypeTags_SetConstPtr, "Item has no pointer tags", 0);
}

//------------------------------------------------------------------------------

bool TypeTags::SetPointer(size_t n, bool readonly, bool unstable)
{
   Debug::ft("TypeTags.SetPointer");

   //  Note that a "const" or "volatile" attribute cannot be cleared once set.
   //
   if(n < Cxx::MAX_PTRS)
   {
      auto mask = 1 << n;
      if(TagCount(n) >= ptrs_) ptrs_ = n + 1;
      if(readonly) constPtr_ |= mask;
      if(unstable) volatilePtr_ |= mask;
      return true;
   }

   return false;
}

//------------------------------------------------------------------------------

void TypeTags::SetPtrs(TagCount count)
{
   Debug::ft("TypeTags.SetPtrs");

   ptrs_ = count;
}

//------------------------------------------------------------------------------

fn_name TypeTags_SetVolatilePtr = "TypeTags.SetVolatilePtr";

void TypeTags::SetVolatilePtr() const
{
   Debug::ft(TypeTags_SetVolatilePtr);

   if(ptrs_ > 0)
      volatilePtr_ |= (1 << (ptrs_ - 1));
   else
      Context::SwLog(TypeTags_SetVolatilePtr, "No pointer tags", 0);
}

//------------------------------------------------------------------------------

void TypeTags::TypeString(std::string& name, bool arg) const
{
   //c "volatile" is omitted because it is not supported in type matching.
   //
   if(const_) name = "const " + name;

   for(auto i = 0; i < ptrs_; ++i)
   {
      name.push_back('*');
      if(IsConstPtr(i)) name += " const";
   }

   if(arg)
   {
      //  For an auto type, ptrs_ can be negative:
      //     auto& entry = table[index];
      //  ENTRY initially has ptrs_ = 0.  StackArg::WasIndexed, invoked on
      //  TABLE, decrements its ptrs_ from 0 to -1.  The result is ENTRY's
      //  referent, so ENTRY has ptrs_ = -1 and arrays_ = 1 (from TABLE's
      //  DataSpec).  These must cancel each other out so that ENTRY doesn't
      //  masquerade as either an array or a pointer.
      //
      auto count = (ptrs_ < 0 ? ptrs_ + arrays_ : arrays_);
      if(count > 0) name += string(count, '*');
   }
   else
   {
      for(auto i = 0; i < arrays_; ++i) name += ARRAY_STR;
   }

   if(!arg && (refs_ > 0)) name += string(refs_, '&');
}
}
