//==============================================================================
//
//  IpServiceRegistry.cpp
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
#include "IpServiceRegistry.h"
#include <iomanip>
#include <ostream>
#include "Debug.h"
#include "Formatters.h"
#include "IpService.h"
#include "NwTypes.h"
#include "SysTypes.h"

using namespace NodeBase;
using std::ostream;
using std::setw;
using std::string;

//------------------------------------------------------------------------------

namespace NetworkBase
{
IpServiceRegistry::IpServiceRegistry()
{
   Debug::ft("IpServiceRegistry.ctor");

   services_.Init(IpService::MaxId, IpService::CellDiff(), MemImmutable);
}

//------------------------------------------------------------------------------

fn_name IpServiceRegistry_dtor = "IpServiceRegistry.dtor";

IpServiceRegistry::~IpServiceRegistry()
{
   Debug::ftnt(IpServiceRegistry_dtor);

   Debug::SwLog(IpServiceRegistry_dtor, UnexpectedInvocation, 0);
}

//------------------------------------------------------------------------------

bool IpServiceRegistry::BindService(IpService& service)
{
   Debug::ft("IpServiceRegistry.BindService");

   return services_.Insert(service);
}

//------------------------------------------------------------------------------

void IpServiceRegistry::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Immutable::Display(stream, prefix, options);

   stream << prefix << "services [id_t]" << CRLF;
   services_.Display(stream, prefix + spaces(2), options);
}

//------------------------------------------------------------------------------

std::vector<IpService*> IpServiceRegistry::GetServices(const string& name) const
{
   std::vector<IpService*> services;

   for(auto s = services_.First(); s != nullptr; services_.Next(s))
   {
      if(s->Name() == name) services.push_back(s);
   }

   return services;
}

//------------------------------------------------------------------------------

void IpServiceRegistry::Patch(sel_t selector, void* arguments)
{
   Immutable::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

void IpServiceRegistry::Shutdown(RestartLevel level)
{
   Debug::ft("IpServiceRegistry.Shutdown");

   for(auto s = services_.First(); s != nullptr; services_.Next(s))
   {
      s->Shutdown(level);
   }
}

//------------------------------------------------------------------------------

void IpServiceRegistry::Startup(RestartLevel level)
{
   Debug::ft("IpServiceRegistry.Startup");

   for(auto s = services_.First(); s != nullptr; services_.Next(s))
   {
      s->Startup(level);
   }
}

//------------------------------------------------------------------------------

fixed_string ServiceHeader = " Id   Port  Protocol      Faction  Enabled  Name";
//                           |  3      7        10.          12        9..<name>

size_t IpServiceRegistry::Summarize(ostream& stream, uint32_t selector) const
{
   stream << ServiceHeader << CRLF;

   for(auto s = services_.First(); s != nullptr; services_.Next(s))
   {
      stream << setw(3) << s->Sid();
      stream << setw(7) << s->Port();
      stream << setw(10) << s->Protocol();
      stream << SPACE << setw(12) << s->GetFaction();
      stream << setw(9) << s->Enabled();
      stream << spaces(2) << s->Name() << CRLF;
   }

   return services_.Size();
}

//------------------------------------------------------------------------------

void IpServiceRegistry::UnbindService(IpService& service)
{
   Debug::ftnt("IpServiceRegistry.UnbindService");

   services_.Erase(service);
}
}
