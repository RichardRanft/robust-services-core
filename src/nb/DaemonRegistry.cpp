//==============================================================================
//
//  DaemonRegistry.cpp
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
#include "DaemonRegistry.h"
#include <iomanip>
#include <ios>
#include <ostream>
#include <string>
#include "Alarm.h"
#include "Daemon.h"
#include "Debug.h"
#include "Formatters.h"
#include "Thread.h"

using std::ostream;
using std::setw;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
DaemonRegistry::DaemonRegistry()
{
   Debug::ft("DaemonRegistry.ctor");

   daemons_.Init(Thread::MaxId, Daemon::CellDiff(), MemPermanent);
}

//------------------------------------------------------------------------------

fn_name DaemonRegistry_dtor = "DaemonRegistry.dtor";

DaemonRegistry::~DaemonRegistry()
{
   Debug::ftnt(DaemonRegistry_dtor);

   Debug::SwLog(DaemonRegistry_dtor, UnexpectedInvocation, 0);
}

//------------------------------------------------------------------------------

fn_name DaemonRegistry_BindDaemon = "DaemonRegistry.BindDaemon";

bool DaemonRegistry::BindDaemon(Daemon& daemon)
{
   Debug::ft(DaemonRegistry_BindDaemon);

   if(FindDaemon(daemon.Name().c_str()) != nullptr)
   {
      Debug::SwLog(DaemonRegistry_BindDaemon, daemon.Name(), 0);
      return false;
   }

   return daemons_.Insert(daemon);
}

//------------------------------------------------------------------------------

void DaemonRegistry::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Permanent::Display(stream, prefix, options);

   stream << prefix << "daemons [id_t]" << CRLF;
   daemons_.Display(stream, prefix + spaces(2), options);
}

//------------------------------------------------------------------------------

fn_name DaemonRegistry_FindDaemon = "DaemonRegistry.FindDaemon";

Daemon* DaemonRegistry::FindDaemon(c_string name) const
{
   Debug::ft(DaemonRegistry_FindDaemon);

   if(name == nullptr)
   {
      Debug::SwLog(DaemonRegistry_FindDaemon, "null pointer", 0);
      return nullptr;
   }

   for(auto d = daemons_.First(); d != nullptr; daemons_.Next(d))
   {
      if(d->Name() == name) return d;
   }

   return nullptr;
}

//------------------------------------------------------------------------------

void DaemonRegistry::Patch(sel_t selector, void* arguments)
{
   Permanent::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

void DaemonRegistry::Shutdown(RestartLevel level)
{
   Debug::ft("DaemonRegistry.Shutdown");

   for(auto d = daemons_.First(); d != nullptr; daemons_.Next(d))
   {
      d->Shutdown(level);
   }
}

//------------------------------------------------------------------------------

void DaemonRegistry::Startup(RestartLevel level)
{
   Debug::ft("DaemonRegistry.Startup");

   for(auto d = daemons_.First(); d != nullptr; daemons_.Next(d))
   {
      d->Startup(level);
   }
}

//------------------------------------------------------------------------------

fixed_string DaemonHeader = "Id  Name       Alarm     AlarmId  Level";
//                          | 2..10        .10              7      7

size_t DaemonRegistry::Summarize(ostream& stream, uint32_t selector) const
{
   stream << DaemonHeader << CRLF;

   for(auto d = daemons_.First(); d != nullptr; daemons_.Next(d))
   {
      stream << setw(2) << d->Did();
      stream << spaces(2) << std::left << setw(10) << d->Name();

      auto alarm = d->GetAlarm();
      if(alarm != nullptr)
      {
         stream << SPACE << setw(10) << alarm->Name();
         stream << std::right << setw(7) << alarm->Aid();
         stream << setw(7) << AlarmStatusSymbol(alarm->Status());
      }
      else
      {
         stream << SPACE << setw(10) << "none" << std::right;
      }

      stream << CRLF;
   }

   return daemons_.Size();
}

//------------------------------------------------------------------------------

void DaemonRegistry::UnbindDaemon(Daemon& daemon)
{
   Debug::ftnt("DaemonRegistry.UnbindDaemon");

   daemons_.Erase(daemon);
}
}
