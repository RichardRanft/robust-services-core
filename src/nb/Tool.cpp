//==============================================================================
//
//  Tool.cpp
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
#include "Tool.h"
#include <cctype>
#include <cstdint>
#include <ostream>
#include "Algorithms.h"
#include "Debug.h"
#include "Element.h"
#include "Singleton.h"
#include "ToolRegistry.h"
#include "TraceBuffer.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
Tool::Tool(FlagId tid, char abbr, bool safe) :
   abbr_(abbr),
   safe_(safe)
{
   Debug::ft("Tool.ctor");

   tid_.SetId(tid);
   Singleton<ToolRegistry>::Instance()->BindTool(*this);
}

//------------------------------------------------------------------------------

fn_name Tool_dtor = "Tool.dtor";

Tool::~Tool()
{
   Debug::ftnt(Tool_dtor);

   Debug::SwLog(Tool_dtor, UnexpectedInvocation, 0);
   Singleton<ToolRegistry>::Extant()->UnbindTool(*this);
}

//------------------------------------------------------------------------------

ptrdiff_t Tool::CellDiff()
{
   uintptr_t local;
   auto fake = reinterpret_cast<const Tool*>(&local);
   return ptrdiff(&fake->tid_, fake);
}

//------------------------------------------------------------------------------

void Tool::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Immutable::Display(stream, prefix, options);

   stream << prefix << "tid  : " << Tid() << CRLF;
   if(isprint(abbr_))
   {
      stream << prefix << "abbr : " << abbr_ << CRLF;
   }
   stream << prefix << "safe : " << safe_ << CRLF;
}

//------------------------------------------------------------------------------

fn_name Tool_Expl = "Tool.Expl";

c_string Tool::Expl() const
{
   Debug::SwLog(Tool_Expl, strOver(this), 0);
   return EMPTY_STR;
}

//------------------------------------------------------------------------------

bool Tool::IsSafe() const
{
   Debug::ft("Tool.IsSafe");

   if(safe_) return true;
   return Element::RunningInLab();
}

//------------------------------------------------------------------------------

fn_name Tool_Name = "Tool.Name";

c_string Tool::Name() const
{
   Debug::SwLog(Tool_Name, strOver(this), 0);
   return EMPTY_STR;
}

//------------------------------------------------------------------------------

void Tool::Patch(sel_t selector, void* arguments)
{
   Immutable::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fixed_string ToolOn = "ON";
fixed_string ToolOff = "off";

string Tool::Status() const
{
   auto buff = Singleton<TraceBuffer>::Instance();
   return (buff->ToolIsOn(Tid()) ? ToolOn : ToolOff);
}
}
