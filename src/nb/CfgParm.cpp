//==============================================================================
//
//  CfgParm.cpp
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
#include "CfgParm.h"
#include <cstdint>
#include <sstream>
#include "Algorithms.h"
#include "CfgParmRegistry.h"
#include "CfgTuple.h"
#include "Debug.h"
#include "Formatters.h"
#include "FunctionGuard.h"
#include "Log.h"
#include "NbLogs.h"
#include "Singleton.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
CfgParm::CfgParm(c_string key, c_string def, c_string expl) :
   tuple_(nullptr),
   default_(def),
   expl_(expl),
   level_(RestartNone)
{
   Debug::ft("CfgParm.ctor");

   Debug::Assert(key != nullptr);
   Debug::Assert(default_ != nullptr);
   Debug::Assert(expl_ != nullptr);

   auto reg = Singleton<CfgParmRegistry>::Instance();

   tuple_ = reg->FindTuple(key);

   if(tuple_ == nullptr)
   {
      tuple_ = new CfgTuple(key, default_);
      reg->BindTuple(*tuple_);
   }
}

//------------------------------------------------------------------------------

fn_name CfgParm_dtor = "CfgParm.dtor";

CfgParm::~CfgParm()
{
   Debug::ftnt(CfgParm_dtor);

   Debug::SwLog(CfgParm_dtor, UnexpectedInvocation, 0);
   Singleton<CfgParmRegistry>::Extant()->UnbindParm(*this);
}

//------------------------------------------------------------------------------

void CfgParm::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Protected::Display(stream, prefix, options);

   stream << prefix << "tuple : ";

   if(tuple_ != nullptr)
   {
      stream << CRLF;
      tuple_->Display(stream, prefix + spaces(2), options);
   }
   else
   {
      stream << "undefined" << CRLF;
   }

   stream << prefix << "default : " << default_ << CRLF;
   stream << prefix << "expl    : " << expl_ << CRLF;
   stream << prefix << "level   : " << level_ << CRLF;
   stream << prefix << "link    : " << link_.to_str() << CRLF;
}

//------------------------------------------------------------------------------

fn_name CfgParm_GetCurr = "CfgParm.GetCurr";

string CfgParm::GetCurr() const
{
   Debug::ft(CfgParm_GetCurr);

   Debug::SwLog(CfgParm_GetCurr, strOver(this), 0);
   return EMPTY_STR;
}

//------------------------------------------------------------------------------

string CfgParm::GetInput() const
{
   Debug::ft("CfgParm.GetInput");

   return GetCurr();
}

//------------------------------------------------------------------------------

c_string CfgParm::Key() const
{
   return tuple_->Key();
}

//------------------------------------------------------------------------------

ptrdiff_t CfgParm::LinkDiff()
{
   uintptr_t local;
   auto fake = reinterpret_cast<const CfgParm*>(&local);
   return ptrdiff(&fake->link_, fake);
}

//------------------------------------------------------------------------------

void CfgParm::Patch(sel_t selector, void* arguments)
{
   Protected::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

void CfgParm::SetCurr()
{
   Debug::ft("CfgParm.SetCurr");

   FunctionGuard guard(Guard_MemUnprotect);
   auto input = GetInput();
   tuple_->SetInput(input.c_str());
   level_ = RestartNone;
}

//------------------------------------------------------------------------------

bool CfgParm::SetFromTuple()
{
   Debug::ft("CfgParm.SetFromTuple");

   auto input = tuple_->Input();

   if(SetNext(input))
   {
      SetCurr();
      return true;
   }

   auto log = Log::Create(ConfigLogGroup, ConfigValueInvalid);

   if(log != nullptr)
   {
      *log << Log::Tab << "errval=" << input << " key=" << Key();
      Log::Submit(log);
   }

   SetNext(default_);
   SetCurr();
   return false;
}

//------------------------------------------------------------------------------

fn_name CfgParm_SetNext = "CfgParm.SetNext";

bool CfgParm::SetNext(c_string input)
{
   Debug::ft(CfgParm_SetNext);

   Debug::SwLog(CfgParm_SetNext, strOver(this), 0);
   return false;
}

//------------------------------------------------------------------------------

bool CfgParm::SetValue(c_string input, RestartLevel& level)
{
   Debug::ft("CfgParm.SetValue");

   FunctionGuard guard(Guard_MemUnprotect);
   if(!SetNext(input)) return false;
   level_ = RestartRequired();
   level = level_;
   if(level_ == RestartNone) SetCurr();
   return true;
}
}
