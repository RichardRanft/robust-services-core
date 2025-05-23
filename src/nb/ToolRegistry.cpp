//==============================================================================
//
//  ToolRegistry.cpp
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
#include "ToolRegistry.h"
#include <cctype>
#include <cstddef>
#include <iomanip>
#include <ios>
#include <ostream>
#include "Debug.h"
#include "Formatters.h"
#include "Tool.h"

using std::ostream;
using std::setw;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
//> The maximum number of tools that can register.
//
constexpr size_t MaxTools = 20;

//------------------------------------------------------------------------------

ToolRegistry::ToolRegistry()
{
   Debug::ft("ToolRegistry.ctor");

   tools_.Init(MaxTools, Tool::CellDiff(), MemImmutable);
}

//------------------------------------------------------------------------------

fn_name ToolRegistry_dtor = "ToolRegistry.dtor";

ToolRegistry::~ToolRegistry()
{
   Debug::ftnt(ToolRegistry_dtor);

   Debug::SwLog(ToolRegistry_dtor, UnexpectedInvocation, 0);
}

//------------------------------------------------------------------------------

fn_name ToolRegistry_BindTool = "ToolRegistry.BindTool";

bool ToolRegistry::BindTool(Tool& tool)
{
   Debug::ft(ToolRegistry_BindTool);

   //  Check that TOOL's CLI character is not already in use.
   //
   auto c = tool.CliChar();

   if(isprint(c))
   {
      for(auto t = tools_.First(); t != nullptr; tools_.Next(t))
      {
         if(t->CliChar() == c)
         {
            Debug::SwLog(ToolRegistry_BindTool, strClass(this), int(c));
            return false;
         }
      }
   }

   return tools_.Insert(tool);
}

//------------------------------------------------------------------------------

void ToolRegistry::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Immutable::Display(stream, prefix, options);

   stream << prefix << "tools : " << CRLF;
   tools_.Display(stream, prefix + spaces(2), options);
}

//------------------------------------------------------------------------------

Tool* ToolRegistry::FindTool(char abbr) const
{
   Debug::ft("ToolRegistry.FindTool");

   if(!isprint(abbr)) return nullptr;

   for(auto t = tools_.First(); t != nullptr; tools_.Next(t))
   {
      if(t->CliChar() == abbr) return t;
   }

   return nullptr;
}

//------------------------------------------------------------------------------

Tool* ToolRegistry::GetTool(FlagId id) const
{
   return tools_.At(id);
}

//------------------------------------------------------------------------------

string ToolRegistry::ListToolChars() const
{
   Debug::ft("ToolRegistry.ListToolChars");

   string tools;

   for(auto t = tools_.First(); t != nullptr; tools_.Next(t))
   {
      tools.push_back(t->CliChar());
   }

   return tools;
}

//------------------------------------------------------------------------------

void ToolRegistry::Patch(sel_t selector, void* arguments)
{
   Immutable::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fixed_string ToolHeader = "Id  Char  Name             Safe  Explanation";
//                        | 2     6..15             .    5..<expl>

size_t ToolRegistry::Summarize(ostream& stream, uint32_t selector) const
{
   //  Display the available tools.  If a tool's CLI character is not
   //  printable, it is not supported through the CLI.  If a tool is
   //  not field-safe, only display it in the lab.
   //
   stream << ToolHeader << CRLF;

   for(auto t = tools_.First(); t != nullptr; tools_.Next(t))
   {
      auto c = t->CliChar();
      if(!isprint(c)) continue;

      stream << setw(2) << t->Tid();
      stream << setw(6) << c;
      stream << spaces(2) << std::left << setw(15) << t->Name();
      stream << SPACE << std::right << setw(5) << t->IsSafe();
      stream << spaces(2) << t->Expl() << CRLF;
   }

   return tools_.Size();
}

//------------------------------------------------------------------------------

void ToolRegistry::UnbindTool(Tool& tool)
{
   Debug::ftnt("ToolRegistry.UnbindTool");

   tools_.Erase(tool);
}
}
