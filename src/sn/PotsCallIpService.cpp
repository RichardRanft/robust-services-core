//==============================================================================
//
//  PotsCallIpService.cpp
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
#include "PotsSessions.h"
#include "CliText.h"
#include "Debug.h"

//------------------------------------------------------------------------------

namespace PotsBase
{
PotsCallIpService::PotsCallIpService()
{
   Debug::ft("PotsCallIpService.ctor");
}

//------------------------------------------------------------------------------

PotsCallIpService::~PotsCallIpService()
{
   Debug::ftnt("PotsCallIpService.dtor");
}

//------------------------------------------------------------------------------

InputHandler* PotsCallIpService::CreateHandler(IpPort* port) const
{
   Debug::ft("PotsCallIpService.CreateHandler");

   return new PotsCallHandler(port);
}

//------------------------------------------------------------------------------

fixed_string PotsCallServiceStr = "POTS Call/UDP";
fixed_string PotsCallServiceExpl = "POTS Call Protocol";

CliText* PotsCallIpService::CreateText() const
{
   Debug::ft("PotsCallIpService.CreateText");

   return new CliText(PotsCallServiceStr, PotsCallServiceExpl);
}
}
