//==============================================================================
//
//  CxxStatement.cpp
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
#include "CxxStatement.h"
#include <sstream>
#include "CodeFile.h"
#include "CodeTypes.h"
#include "CxxArea.h"
#include "CxxExecute.h"
#include "CxxNamed.h"
#include "Debug.h"
#include "Lexer.h"
#include "SysTypes.h"

using namespace NodeBase;
using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace CodeTools
{
Break::Break(size_t pos) : CxxStatement(pos)
{
   Debug::ft("Break.ctor");
}

//------------------------------------------------------------------------------

void Break::Print(ostream& stream, const Flags& options) const
{
   stream << BREAK_STR << ';';
}

//==============================================================================

Case::Case(ExprPtr& expression, size_t pos) : CxxStatement(pos),
   expr_(expression.release())
{
   Debug::ft("Case.ctor");
}

//------------------------------------------------------------------------------

void Case::Check() const
{
   expr_->Check();
}

//------------------------------------------------------------------------------

void Case::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   auto lead = prefix.substr(0, prefix.size() - IndentSize());
   stream << lead << CASE_STR << SPACE;
   expr_->Print(stream, options);
   stream << ':' << CRLF;
}

//------------------------------------------------------------------------------

void Case::EnterBlock()
{
   Debug::ft("Case.EnterBlock");

   CxxStatement::EnterBlock();

   expr_->EnterBlock();
   auto result = Context::PopArg(true);
   DataSpec::Int->MustMatchWith(result);
}

//------------------------------------------------------------------------------

bool Case::GetSpan(size_t& begin, size_t& left, size_t& end) const
{
   Debug::ft("Case.GetSpan");

   return GetColonSpan(begin, end);
}

//------------------------------------------------------------------------------

void Case::GetUsages(const CodeFile& file, CxxUsageSets& symbols)
{
   expr_->GetUsages(file, symbols);
}

//------------------------------------------------------------------------------

CxxToken* Case::PosToItem(size_t pos) const
{
   auto item = CxxStatement::PosToItem(pos);
   if(item != nullptr) return item;

   return expr_->PosToItem(pos);
}

//------------------------------------------------------------------------------

void Case::UpdatePos
   (EditorAction action, size_t begin, size_t count, size_t from) const
{
   CxxStatement::UpdatePos(action, begin, count, from);
   expr_->UpdatePos(action, begin, count, from);
}

//------------------------------------------------------------------------------

void Case::UpdateXref(bool insert)
{
   expr_->UpdateXref(insert);
}

//==============================================================================

Catch::Catch(size_t pos) : CxxStatement(pos)
{
   Debug::ft("Catch.ctor");
}

//------------------------------------------------------------------------------

void Catch::Check() const
{
   if(arg_ != nullptr) arg_->Check();
   handler_->Check();
}

//------------------------------------------------------------------------------

void Catch::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   stream << prefix << CATCH_STR << '(';

   if(arg_ != nullptr)
      arg_->Print(stream, options);
   else
      stream << ELLIPSES_STR;

   stream << ')';

   auto opts = options;
   opts.set(DispLF);
   handler_->Display(stream, prefix, opts);
}

//------------------------------------------------------------------------------

void Catch::EnterBlock()
{
   Debug::ft("Catch.EnterBlock");

   CxxStatement::EnterBlock();

   if(arg_ != nullptr)
   {
      arg_->EnterScope();
      arg_->EnterBlock();
   }

   handler_->EnterBlock();
}

//------------------------------------------------------------------------------

void Catch::ExitBlock() const
{
   Debug::ft("Catch.ExitBlock");

   if(arg_ != nullptr) arg_->ExitBlock();
}

//------------------------------------------------------------------------------

CxxScoped* Catch::FindNthItem(const std::string& name, size_t& n) const
{
   Debug::ft("Catch.FindNthItem");

   return handler_->FindNthItem(name, n);
}

//------------------------------------------------------------------------------

bool Catch::GetSpan(size_t& begin, size_t& left, size_t& end) const
{
   Debug::ft("Catch.GetSpan");

   return GetParSpan(begin, left, end);
}

//------------------------------------------------------------------------------

void Catch::GetUsages(const CodeFile& file, CxxUsageSets& symbols)
{
   if(arg_ != nullptr) arg_->GetUsages(file, symbols);
   handler_->GetUsages(file, symbols);
}

//------------------------------------------------------------------------------

bool Catch::LocateItem(const CxxToken* item, size_t& n) const
{
   Debug::ft("Catch.LocateItem");

   return handler_->LocateItem(item, n);
}

//------------------------------------------------------------------------------

CxxToken* Catch::PosToItem(size_t pos) const
{
   auto item = CxxStatement::PosToItem(pos);
   if(item != nullptr) return item;

   if(arg_ != nullptr) item = arg_->PosToItem(pos);
   if(item != nullptr) return item;

   return handler_->PosToItem(pos);
}

//------------------------------------------------------------------------------

void Catch::UpdatePos
   (EditorAction action, size_t begin, size_t count, size_t from) const
{
   CxxStatement::UpdatePos(action, begin, count, from);
   if(arg_ != nullptr) arg_->UpdatePos(action, begin, count, from);
   handler_->UpdatePos(action, begin, count, from);
}

//------------------------------------------------------------------------------

void Catch::UpdateXref(bool insert)
{
   if(arg_ != nullptr) arg_->UpdateXref(insert);
   handler_->UpdateXref(insert);
}

//==============================================================================

Condition::Condition(size_t pos) : CxxStatement(pos)
{
   Debug::ft("Condition.ctor");
}

//------------------------------------------------------------------------------

void Condition::Check() const
{
   if(condition_ != nullptr) condition_->Check();
}

//------------------------------------------------------------------------------

void Condition::EnterBlock()
{
   Debug::ft("Condition.EnterBlock");

   CxxStatement::EnterBlock();

   if(condition_ != nullptr)
   {
      //  The result of the conditional expression should be a boolean.
      //
      condition_->EnterBlock();
      auto result = Context::PopArg(true);
      result.CheckIfBool();
   }
}

//------------------------------------------------------------------------------

void Condition::GetUsages(const CodeFile& file, CxxUsageSets& symbols)
{
   if(condition_ != nullptr) condition_->GetUsages(file, symbols);
}

//------------------------------------------------------------------------------

CxxToken* Condition::PosToItem(size_t pos) const
{
   auto item = CxxStatement::PosToItem(pos);
   if(item != nullptr) return item;

   return (condition_ != nullptr ? condition_->PosToItem(pos) : nullptr);
}

//------------------------------------------------------------------------------

void Condition::Print(ostream& stream, const Flags& options) const
{
   Show(stream);
}

//------------------------------------------------------------------------------

bool Condition::Show(ostream& stream) const
{
   if(condition_ == nullptr) return false;
   condition_->Print(stream, NoFlags);
   return true;
}

//------------------------------------------------------------------------------

void Condition::UpdatePos
   (EditorAction action, size_t begin, size_t count, size_t from) const
{
   CxxStatement::UpdatePos(action, begin, count, from);
   if(condition_ != nullptr) condition_->UpdatePos(action, begin, count, from);
}

//------------------------------------------------------------------------------

void Condition::UpdateXref(bool insert)
{
   if(condition_ != nullptr) condition_->UpdateXref(insert);
}

//==============================================================================

Continue::Continue(size_t pos) : CxxStatement(pos)
{
   Debug::ft("Continue.ctor");
}

//------------------------------------------------------------------------------

void Continue::Print(ostream& stream, const Flags& options) const
{
   stream << CONTINUE_STR << ';';
}

//==============================================================================

CxxStatement::CxxStatement(size_t pos) :
   scope_(nullptr)
{
   Debug::ft("CxxStatement.ctor");

   SetContext(pos);
}

//------------------------------------------------------------------------------

void CxxStatement::Delete()
{
   Debug::ft("CxxStatement.Delete");

   static_cast<Block*>(GetScope())->DeleteItem(this);
}

//------------------------------------------------------------------------------

void CxxStatement::EnterBlock()
{
   Context::SetPos(GetPos());
}

//------------------------------------------------------------------------------

bool CxxStatement::GetColonSpan(size_t& begin, size_t& end) const
{
   Debug::ft("CxxStatement.GetColonSpan");

   //  Note that FindFirstOf skips scope resolution operators, which could
   //  be necessary if this is a case label.
   //
   begin = GetPos();
   end = GetFile()->GetLexer().FindFirstOf(":", begin);
   return (end != string::npos);
}

//------------------------------------------------------------------------------

bool CxxStatement::GetParSpan(size_t& begin, size_t& left, size_t& end) const
{
   Debug::ft("CxxStatement.GetParSpan");

   const auto& lexer = GetFile()->GetLexer();
   begin = GetPos();
   auto lpar = lexer.FindFirstOf("(", begin);
   if(lpar == string::npos) return false;
   auto rpar = lexer.FindClosing('(', ')', lpar + 1);
   if(rpar == string::npos) return false;
   return GetSeqSpan(rpar, left, end);
}

//------------------------------------------------------------------------------

bool CxxStatement::GetSeqSpan(size_t begin, size_t& left, size_t& end) const
{
   Debug::ft("CxxStatement.GetSeqSpan");

   const auto& lexer = GetFile()->GetLexer();
   left = lexer.FindFirstOf(";{", begin);
   if(left == string::npos) return false;

   if(lexer.At(left) == ';')
   {
      end = left;
      return true;
   }

   end = lexer.FindClosing('{', '}', left + 1);
   return (end != string::npos);
}

//------------------------------------------------------------------------------

bool CxxStatement::GetSpan(size_t& begin, size_t& left, size_t& end) const
{
   Debug::ft("CxxStatement.GetSpan");

   return GetSemiSpan(begin, end);
}

//==============================================================================

Do::Do(size_t pos) : Condition(pos)
{
   Debug::ft("Do.ctor");
}

//------------------------------------------------------------------------------

void Do::Check() const
{
   loop_->Check();
   Condition::Check();
}

//------------------------------------------------------------------------------

void Do::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   stream << prefix << DO_STR;

   auto lf = loop_->CrlfOver(Block::Unbraced);

   if(!lf)
   {
      loop_->Print(stream, options);
      stream << SPACE;
   }
   else
   {
      auto opts = options;
      opts.set(DispLF);
      loop_->Display(stream, prefix, opts);
      stream << prefix;
   }

   stream << WHILE_STR << '(';
   Condition::Print(stream, options);
   stream << ");" << CRLF;
}

//------------------------------------------------------------------------------

void Do::EnterBlock()
{
   Debug::ft("Do.EnterBlock");

   loop_->EnterBlock();
   Condition::EnterBlock();
}

//------------------------------------------------------------------------------

CxxScoped* Do::FindNthItem(const std::string& name, size_t& n) const
{
   Debug::ft("Do.FindNthItem");

   return loop_->FindNthItem(name, n);
}

//------------------------------------------------------------------------------

bool Do::GetSpan(size_t& begin, size_t& left, size_t& end) const
{
   Debug::ft("Do.GetSpan");

   const auto& lexer = GetFile()->GetLexer();
   begin = GetPos();
   if(!GetSeqSpan(begin, left, end)) return false;
   end = lexer.FindFirstOf(";", end);
   return (end != string::npos);
}

//------------------------------------------------------------------------------

void Do::GetUsages(const CodeFile& file, CxxUsageSets& symbols)
{
   loop_->GetUsages(file, symbols);
   Condition::GetUsages(file, symbols);
}

//------------------------------------------------------------------------------

bool Do::InLine() const
{
   return !loop_->CrlfOver(Block::Unbraced);
}

//------------------------------------------------------------------------------

bool Do::LocateItem(const CxxToken* item, size_t& n) const
{
   Debug::ft("Do.LocateItem");

   return loop_->LocateItem(item, n);
}

//------------------------------------------------------------------------------

CxxToken* Do::PosToItem(size_t pos) const
{
   auto item = Condition::PosToItem(pos);
   if(item != nullptr) return item;

   return loop_->PosToItem(pos);
}

//------------------------------------------------------------------------------

void Do::Print(ostream& stream, const Flags& options) const
{
   stream << SPACE << DO_STR;
   loop_->Print(stream, options);
   stream << SPACE << WHILE_STR << '(';
   Condition::Print(stream, options);
   stream << ");";
}

//------------------------------------------------------------------------------

void Do::UpdatePos
   (EditorAction action, size_t begin, size_t count, size_t from) const
{
   Condition::UpdatePos(action, begin, count, from);
   loop_->UpdatePos(action, begin, count, from);
}

//------------------------------------------------------------------------------

void Do::UpdateXref(bool insert)
{
   loop_->UpdateXref(insert);
   Condition::UpdateXref(insert);
}

//==============================================================================

Expr::Expr(ExprPtr& expression, size_t pos) : CxxStatement(pos),
   expr_(expression.release())
{
   Debug::ft("Expr.ctor");
}

//------------------------------------------------------------------------------

void Expr::Check() const
{
   expr_->Check();
}

//------------------------------------------------------------------------------

void Expr::EnterBlock()
{
   Debug::ft("Expr.EnterBlock");

   CxxStatement::EnterBlock();

   expr_->EnterBlock();
}

//------------------------------------------------------------------------------

void Expr::GetUsages(const CodeFile& file, CxxUsageSets& symbols)
{
   expr_->GetUsages(file, symbols);
}

//------------------------------------------------------------------------------

CxxToken* Expr::PosToItem(size_t pos) const
{
   auto item = CxxStatement::PosToItem(pos);
   if(item != nullptr) return item;

   return expr_->PosToItem(pos);
}

//------------------------------------------------------------------------------

void Expr::Print(ostream& stream, const Flags& options) const
{
   expr_->Print(stream, options);
   stream << ';';
}

//------------------------------------------------------------------------------

void Expr::UpdatePos
   (EditorAction action, size_t begin, size_t count, size_t from) const
{
   CxxStatement::UpdatePos(action, begin, count, from);
   expr_->UpdatePos(action, begin, count, from);
}

//------------------------------------------------------------------------------

void Expr::UpdateXref(bool insert)
{
   expr_->UpdateXref(insert);
}

//==============================================================================

For::For(size_t pos) : Condition(pos)
{
   Debug::ft("For.ctor");
}

//------------------------------------------------------------------------------

void For::Check() const
{
   if(initial_ != nullptr) initial_->Check();
   Condition::Check();
   if(subsequent_ != nullptr) subsequent_->Check();
   loop_->Check();
}

//------------------------------------------------------------------------------

void For::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   auto data = false;
   string info;
   auto stats = options.test(DispStats);

   stream << prefix << FOR_STR << '(';

   //  If initial_ declares a loop variable, as is usually the case, it will
   //  end in a // comment showing the variable's init/read/write counts.  To
   //  make the output parsable, save the comment in INFO and append it later.
   //
   if(initial_ != nullptr)
   {
      std::ostringstream buff;
      initial_->Print(buff, options);
      auto init = buff.str();
      auto pos = init.find(COMMENT_STR);
      if(pos != string::npos)
      {
         info = init.substr(pos - 1);
         init.erase(pos - 1);
      }
      stream << init;
      data = (initial_->Type() == Cxx::Data);
   }
   else
   {
      stream << SPACE;
   }

   if(!data) stream << ';';
   stream << SPACE;

   if(!Condition::Show(stream)) stream << SPACE;
   stream << "; ";
   if(subsequent_ != nullptr) subsequent_->Print(stream, options);
   stream << ')';

   if(options.test(DispNoLF))
   {
      loop_->Print(stream, options);
      if(stats) stream << info;
   }
   else
   {
      auto lf = loop_->CrlfOver(Block::Unbraced);

      if(lf)
      {
         auto opts = options;
         opts.set(DispLF);
         if(stats) stream << info;
         loop_->Display(stream, prefix, opts);
      }
      else
      {
         loop_->Print(stream, options);
         if(stats) stream << info;
         stream << CRLF;
      }
   }
}

//------------------------------------------------------------------------------

void For::EnterBlock()
{
   Debug::ft("For.EnterBlock");

   if(initial_ != nullptr)
   {
      initial_->EnterBlock();
      Context::Clear(4);
   }

   Condition::EnterBlock();

   if(subsequent_ != nullptr)
   {
      subsequent_->EnterBlock();
      Context::PopArg(true);
   }

   loop_->EnterBlock();
}

//------------------------------------------------------------------------------

void For::ExitBlock() const
{
   Debug::ft("For.ExitBlock");

   if(initial_ != nullptr) initial_->ExitBlock();
}

//------------------------------------------------------------------------------

CxxScoped* For::FindNthItem(const std::string& name, size_t& n) const
{
   Debug::ft("For.FindNthItem");

   auto item = initial_->FindNthItem(name, n);
   if(item != nullptr) return item;
   item = subsequent_->FindNthItem(name, n);
   if(item != nullptr) return item;
   return loop_->FindNthItem(name, n);
}

//------------------------------------------------------------------------------

bool For::GetSpan(size_t& begin, size_t& left, size_t& end) const
{
   Debug::ft("For.GetSpan");

   return GetParSpan(begin, left, end);
}

//------------------------------------------------------------------------------

void For::GetUsages(const CodeFile& file, CxxUsageSets& symbols)
{
   if(initial_ != nullptr) initial_->GetUsages(file, symbols);
   Condition::GetUsages(file, symbols);
   if(subsequent_ != nullptr) subsequent_->GetUsages(file, symbols);
   loop_->GetUsages(file, symbols);
}

//------------------------------------------------------------------------------

bool For::InLine() const
{
   return !loop_->CrlfOver(Block::Unbraced);
}

//------------------------------------------------------------------------------

bool For::LocateItem(const CxxToken* item, size_t& n) const
{
   Debug::ft("For.LocateItem");

   if(initial_->LocateItem(item, n)) return true;
   if(subsequent_->LocateItem(item, n)) return true;
   return loop_->LocateItem(item, n);
}

//------------------------------------------------------------------------------

CxxToken* For::PosToItem(size_t pos) const
{
   auto item = Condition::PosToItem(pos);
   if(item != nullptr) return item;

   if(initial_ != nullptr) item = initial_->PosToItem(pos);
   if(item != nullptr) return item;

   if(subsequent_ != nullptr) item = subsequent_->PosToItem(pos);
   if(item != nullptr) return item;

   return loop_->PosToItem(pos);
}

//------------------------------------------------------------------------------

void For::Print(ostream& stream, const Flags& options) const
{
   Display(stream, EMPTY_STR, Flags(LF_Mask));
}

//------------------------------------------------------------------------------

void For::UpdatePos
   (EditorAction action, size_t begin, size_t count, size_t from) const
{
   Condition::UpdatePos(action, begin, count, from);
   if(initial_ != nullptr) initial_->UpdatePos(action, begin, count, from);
   if(subsequent_ != nullptr)
      subsequent_->UpdatePos(action, begin, count, from);
   loop_->UpdatePos(action, begin, count, from);
}

//------------------------------------------------------------------------------

void For::UpdateXref(bool insert)
{
   if(initial_ != nullptr) initial_->UpdateXref(insert);
   Condition::UpdateXref(insert);
   if(subsequent_ != nullptr) subsequent_->UpdateXref(insert);
   loop_->UpdateXref(insert);
}

//==============================================================================

Goto::Goto(string& label, size_t pos) : CxxStatement(pos)
{
   Debug::ft("Goto.ctor");

   std::swap(label_, label);
}

//------------------------------------------------------------------------------

void Goto::EnterBlock()
{
   Debug::ft("Goto.EnterBlock");

   CxxStatement::EnterBlock();

   //  A full compiler would verify the label here, but
   //  we don't bother to do anything with labels.
}

//------------------------------------------------------------------------------

void Goto::Print(std::ostream& stream, const NodeBase::Flags& options) const
{
   stream << GOTO_STR << SPACE << label_ << ';';
}

//==============================================================================

If::If(size_t pos) : Condition(pos),
   elseif_(false)
{
   Debug::ft("If.ctor");
}

//------------------------------------------------------------------------------

void If::Check() const
{
   Condition::Check();
   then_->Check();
   if(else_ != nullptr) else_->Check();
}

//------------------------------------------------------------------------------

void If::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   if(elseif_)
      stream << SPACE;
   else
      stream << prefix;

   stream << IF_STR << '(';
   Condition::Print(stream, options);
   stream << ')';

   auto lf =
      (else_ != nullptr) || (elseif_) || (then_->CrlfOver(Block::Unbraced));

   if(!lf)
   {
      then_->Print(stream, options);
      stream << CRLF;
      return;
   }

   auto opts = options;
   opts.set(DispLF);
   then_->Display(stream, prefix, opts);
   if(else_ == nullptr) return;
   stream << prefix << ELSE_STR;
   else_->Display(stream, prefix, opts);
}

//------------------------------------------------------------------------------

void If::EnterBlock()
{
   Debug::ft("If.EnterBlock");

   Condition::EnterBlock();
   then_->EnterBlock();
   if(else_ != nullptr) else_->EnterBlock();
}

//------------------------------------------------------------------------------

CxxScoped* If::FindNthItem(const std::string& name, size_t& n) const
{
   Debug::ft("If.FindNthItem");

   auto item = then_->FindNthItem(name, n);
   if(item != nullptr) return item;
   if(else_ == nullptr) return nullptr;
   return else_->FindNthItem(name, n);
}

//------------------------------------------------------------------------------

bool If::GetSpan(size_t& begin, size_t& left, size_t& end) const
{
   Debug::ft("If.GetSpan");

   if(!GetParSpan(begin, left, end)) return false;
   if(else_ == nullptr) return true;
   return GetSeqSpan(end + 1, left, end);
}

//------------------------------------------------------------------------------

void If::GetUsages(const CodeFile& file, CxxUsageSets& symbols)
{
   Condition::GetUsages(file, symbols);
   then_->GetUsages(file, symbols);
   if(else_ != nullptr) else_->GetUsages(file, symbols);
}

//------------------------------------------------------------------------------

bool If::InLine() const
{
   if(else_ != nullptr) return false;
   if(elseif_) return false;
   if(then_->CrlfOver(Block::Unbraced)) return false;
   return true;
}

//------------------------------------------------------------------------------

bool If::LocateItem(const CxxToken* item, size_t& n) const
{
   Debug::ft("If.LocateItem");

   if(then_->LocateItem(item, n)) return true;
   if(else_ == nullptr) return false;
   return else_->LocateItem(item, n);
}

//------------------------------------------------------------------------------

CxxToken* If::PosToItem(size_t pos) const
{
   auto item = Condition::PosToItem(pos);
   if(item != nullptr) return item;

   item = then_->PosToItem(pos);
   if(item != nullptr) return item;

   return (else_ != nullptr ? else_->PosToItem(pos) : nullptr);
}

//------------------------------------------------------------------------------

void If::Print(ostream& stream, const Flags& options) const
{
   stream << IF_STR << '(';
   Condition::Print(stream, options);
   stream << ')';
   then_->Print(stream, options);
   if(else_ == nullptr) return;

   //  We want multiple lines when an "else" clause exists.  Somehow this
   //  didn't happen, but output the else clause anyway.
   //
   stream << " <@ " << ELSE_STR;
   else_->Print(stream, options);
}

//------------------------------------------------------------------------------

void If::UpdatePos
   (EditorAction action, size_t begin, size_t count, size_t from) const
{
   Condition::UpdatePos(action, begin, count, from);
   then_->UpdatePos(action, begin, count, from);
   if(else_ != nullptr) else_->UpdatePos(action, begin, count, from);
}

//------------------------------------------------------------------------------

void If::UpdateXref(bool insert)
{
   Condition::UpdateXref(insert);
   then_->UpdateXref(insert);
   if(else_ != nullptr) else_->UpdateXref(insert);
}

//==============================================================================

Label::Label(string& name, size_t pos) : CxxStatement(pos)
{
   Debug::ft("Label.ctor");

   std::swap(name_, name);
}

//------------------------------------------------------------------------------

void Label::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   auto lead = prefix.substr(0, prefix.size() - IndentSize());
   stream << lead << name_ << ':' << CRLF;
}

//------------------------------------------------------------------------------

void Label::EnterBlock()
{
   Debug::ft("Label.EnterBlock");

   CxxStatement::EnterBlock();

   //  A full compiler would add the label to a symbol table
   //  here, but we don't bother to do anything with labels.
}

//------------------------------------------------------------------------------

void Label::ExitBlock() const
{
   Debug::ft("Label.ExitBlock");

   //  A full compiler would remove the label from a symbol table
   //  here, but we don't bother to do anything with labels.
}

//------------------------------------------------------------------------------

bool Label::GetSpan(size_t& begin, size_t& left, size_t& end) const
{
   Debug::ft("Label.GetSpan");

   return GetColonSpan(begin, end);
}

//==============================================================================

NoOp::NoOp(size_t pos, bool fallthrough) : CxxStatement(pos),
   fallthrough_(fallthrough)
{
   Debug::ft("NoOp.ctor");
}

//------------------------------------------------------------------------------

void NoOp::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   stream << prefix;
   if(fallthrough_) stream << FALLTHROUGH_STR;
   stream << ';' << CRLF;
}

//------------------------------------------------------------------------------

void NoOp::Print(ostream& stream, const Flags& options) const
{
   stream << ';';
}

//==============================================================================

Return::Return(size_t pos) : CxxStatement(pos)
{
   Debug::ft("Return.ctor");
}

//------------------------------------------------------------------------------

void Return::Check() const
{
   if(expr_ != nullptr) expr_->Check();
}

//------------------------------------------------------------------------------

void Return::EnterBlock()
{
   Debug::ft("Return.EnterBlock");

   CxxStatement::EnterBlock();

   if(expr_ != nullptr)
   {
      //  Verify that the result is compatible with what the function is
      //  supposed to return, and then check for an implicit invocation of
      //  a copy constructor.
      //
      expr_->EnterBlock();
      auto result = Context::PopArg(true);
      auto spec = Context::Scope()->GetFunction()->GetTypeSpec();
      spec->MustMatchWith(result);
      auto tags = spec->Tags();
      result.AssignedTo(StackArg(spec, 0, tags->IsLvalue(), false), Returned);

      if(result.item_ != nullptr)
      {
         if((result.item_->Type() == Cxx::Class) && !result.IsIndirect())
         {
            static_cast<Class*>(result.item_)->InvokeCopyCtor();
         }
      }
   }
}

//------------------------------------------------------------------------------

void Return::GetUsages(const CodeFile& file, CxxUsageSets& symbols)
{
   if(expr_ != nullptr) expr_->GetUsages(file, symbols);
}

//------------------------------------------------------------------------------

CxxToken* Return::PosToItem(size_t pos) const
{
   auto item = CxxStatement::PosToItem(pos);
   if(item != nullptr) return item;

   return (expr_ != nullptr ? expr_->PosToItem(pos) : nullptr);
}

//------------------------------------------------------------------------------

void Return::Print(ostream& stream, const Flags& options) const
{
   stream << RETURN_STR;

   if(expr_ != nullptr)
   {
      stream << SPACE;
      expr_->Print(stream, options);
   }

   stream << ';';
}

//------------------------------------------------------------------------------

void Return::UpdatePos
   (EditorAction action, size_t begin, size_t count, size_t from) const
{
   CxxStatement::UpdatePos(action, begin, count, from);
   if(expr_ != nullptr) expr_->UpdatePos(action, begin, count, from);
}

//------------------------------------------------------------------------------

void Return::UpdateXref(bool insert)
{
   if(expr_ != nullptr) expr_->UpdateXref(insert);
}

//==============================================================================

Switch::Switch(size_t pos) : CxxStatement(pos)
{
   Debug::ft("Switch.ctor[>ct]");
}

//------------------------------------------------------------------------------

void Switch::Check() const
{
   expr_->Check();
   cases_->Check();

   GetFile()->GetLexer().CheckSwitch(*this);
}

//------------------------------------------------------------------------------

void Switch::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   stream << prefix << SWITCH_STR << '(';
   expr_->Print(stream, options);
   stream << ')';

   auto opts = options;
   opts.set(DispLF);
   cases_->Display(stream, prefix, opts);
}

//------------------------------------------------------------------------------

void Switch::EnterBlock()
{
   Debug::ft("Switch.EnterBlock");

   CxxStatement::EnterBlock();

   expr_->EnterBlock();
   auto result = Context::PopArg(true);
   DataSpec::Int->MustMatchWith(result);
   cases_->EnterBlock();
}

//------------------------------------------------------------------------------

CxxScoped* Switch::FindNthItem(const std::string& name, size_t& n) const
{
   Debug::ft("Switch.FindNthItem");

   return cases_->FindNthItem(name, n);
}

//------------------------------------------------------------------------------

bool Switch::GetSpan(size_t& begin, size_t& left, size_t& end) const
{
   Debug::ft("Switch.GetSpan");

   return GetParSpan(begin, left, end);
}

//------------------------------------------------------------------------------

void Switch::GetUsages(const CodeFile& file, CxxUsageSets& symbols)
{
   expr_->GetUsages(file, symbols);
   cases_->GetUsages(file, symbols);
}

//------------------------------------------------------------------------------

bool Switch::LocateItem(const CxxToken* item, size_t& n) const
{
   Debug::ft("Switch.LocateItem");

   return cases_->LocateItem(item, n);
}

//------------------------------------------------------------------------------

CxxToken* Switch::PosToItem(size_t pos) const
{
   auto item = CxxStatement::PosToItem(pos);
   if(item != nullptr) return item;

   item = expr_->PosToItem(pos);
   if(item != nullptr) return item;

   return cases_->PosToItem(pos);
}

//------------------------------------------------------------------------------

void Switch::UpdatePos
   (EditorAction action, size_t begin, size_t count, size_t from) const
{
   CxxStatement::UpdatePos(action, begin, count, from);
   expr_->UpdatePos(action, begin, count, from);
   cases_->UpdatePos(action, begin, count, from);
}

//------------------------------------------------------------------------------

void Switch::UpdateXref(bool insert)
{
   expr_->UpdateXref(insert);
   cases_->UpdateXref(insert);
}

//==============================================================================

Try::Try(size_t pos) : CxxStatement(pos)
{
   Debug::ft("Try.ctor");
}

//------------------------------------------------------------------------------

void Try::Check() const
{
   try_->Check();

   for(auto c = catches_.cbegin(); c != catches_.cend(); ++c)
   {
      (*c)->Check();
   }
}

//------------------------------------------------------------------------------

void Try::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   auto opts = options;
   opts.set(DispLF);
   stream << prefix << TRY_STR;
   try_->Display(stream, prefix, opts);

   for(auto c = catches_.cbegin(); c != catches_.cend(); ++c)
   {
      (*c)->Display(stream, prefix, opts);
   }
}

//------------------------------------------------------------------------------

void Try::EnterBlock()
{
   Debug::ft("Try.EnterBlock");

   CxxStatement::EnterBlock();

   try_->EnterBlock();

   for(auto c = catches_.cbegin(); c != catches_.cend(); ++c)
   {
      (*c)->EnterBlock();
   }
}

//------------------------------------------------------------------------------

void Try::ExitBlock() const
{
   Debug::ft("Try.ExitBlock");

   for(auto c = catches_.crbegin(); c != catches_.crend(); ++c)
   {
      (*c)->ExitBlock();
   }
}

//------------------------------------------------------------------------------

CxxScoped* Try::FindNthItem(const std::string& name, size_t& n) const
{
   Debug::ft("Try.FindNthItem");

   auto item = try_->FindNthItem(name, n);
   if(item != nullptr) return item;

   for(auto c = catches_.cbegin(); c != catches_.cend(); ++c)
   {
      item = (*c)->FindNthItem(name, n);
      if(item != nullptr) return item;
   }

   return nullptr;
}

//------------------------------------------------------------------------------

bool Try::GetSpan(size_t& begin, size_t& left, size_t& end) const
{
   Debug::ft("Try.GetSpan");

   begin = GetPos();
   return GetSeqSpan(begin, left, end);
}

//------------------------------------------------------------------------------

void Try::GetUsages(const CodeFile& file, CxxUsageSets& symbols)
{
   try_->GetUsages(file, symbols);

   for(auto c = catches_.cbegin(); c != catches_.cend(); ++c)
   {
      (*c)->GetUsages(file, symbols);
   }
}

//------------------------------------------------------------------------------

bool Try::LocateItem(const CxxToken* item, size_t& n) const
{
   Debug::ft("Try.LocateItem");

   if(try_->LocateItem(item, n)) return true;

   for(auto c = catches_.cbegin(); c != catches_.cend(); ++c)
   {
      if((*c)->LocateItem(item, n)) return true;
   }

   return false;
}

//------------------------------------------------------------------------------

CxxToken* Try::PosToItem(size_t pos) const
{
   auto item = CxxStatement::PosToItem(pos);
   if(item != nullptr) return item;

   item = try_->PosToItem(pos);
   if(item != nullptr) return item;

   for(auto c = catches_.cbegin(); c != catches_.cend(); ++c)
   {
      item = (*c)->PosToItem(pos);
      if(item != nullptr) return item;
   }

   return nullptr;
}

//------------------------------------------------------------------------------

void Try::UpdatePos
   (EditorAction action, size_t begin, size_t count, size_t from) const
{
   CxxStatement::UpdatePos(action, begin, count, from);
   try_->UpdatePos(action, begin, count, from);

   for(auto c = catches_.cbegin(); c != catches_.cend(); ++c)
   {
      (*c)->UpdatePos(action, begin, count, from);
   }
}

//------------------------------------------------------------------------------

void Try::UpdateXref(bool insert)
{
   try_->UpdateXref(insert);

   for(auto c = catches_.cbegin(); c != catches_.cend(); ++c)
   {
      (*c)->UpdateXref(insert);
   }
}

//==============================================================================

While::While(size_t pos) : Condition(pos)
{
   Debug::ft("While.ctor");
}

//------------------------------------------------------------------------------

void While::Check() const
{
   Condition::Check();
   loop_->Check();
}

//------------------------------------------------------------------------------

void While::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   stream << prefix << WHILE_STR << '(';
   Condition::Print(stream, options);
   stream << ')';

   auto opts = options;
   opts.set(DispLF, loop_->CrlfOver(Block::Unbraced));
   loop_->Display(stream, prefix, opts);
}

//------------------------------------------------------------------------------

void While::EnterBlock()
{
   Debug::ft("While.EnterBlock");

   Condition::EnterBlock();
   loop_->EnterBlock();
}

//------------------------------------------------------------------------------

CxxScoped* While::FindNthItem(const std::string& name, size_t& n) const
{
   Debug::ft("While.FindNthItem");

   return loop_->FindNthItem(name, n);
}

//------------------------------------------------------------------------------

bool While::GetSpan(size_t& begin, size_t& left, size_t& end) const
{
   Debug::ft("While.GetSpan");

   return GetParSpan(begin, left, end);
}

//------------------------------------------------------------------------------

void While::GetUsages(const CodeFile& file, CxxUsageSets& symbols)
{
   Condition::GetUsages(file, symbols);
   loop_->GetUsages(file, symbols);
}

//------------------------------------------------------------------------------

bool While::InLine() const
{
   return !loop_->CrlfOver(Block::Unbraced);
}

//------------------------------------------------------------------------------

bool While::LocateItem(const CxxToken* item, size_t& n) const
{
   Debug::ft("While.LocateItem");

   return loop_->LocateItem(item, n);
}

//------------------------------------------------------------------------------

CxxToken* While::PosToItem(size_t pos) const
{
   auto item = Condition::PosToItem(pos);
   if(item != nullptr) return item;

   return loop_->PosToItem(pos);
}

//------------------------------------------------------------------------------

void While::Print(ostream& stream, const Flags& options) const
{
   stream << SPACE << WHILE_STR << '(';
   Condition::Print(stream, options);
   stream << ')';
   loop_->Print(stream, options);
}

//------------------------------------------------------------------------------

void While::UpdatePos
   (EditorAction action, size_t begin, size_t count, size_t from) const
{
   Condition::UpdatePos(action, begin, count, from);
   loop_->UpdatePos(action, begin, count, from);
}

//------------------------------------------------------------------------------

void While::UpdateXref(bool insert)
{
   Condition::UpdateXref(insert);
   loop_->UpdateXref(insert);
}
}
