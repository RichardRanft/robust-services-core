//==============================================================================
//
//  UdpIpPort.h
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
#ifndef UDPIPPORT_H_INCLUDED
#define UDPIPPORT_H_INCLUDED

#include "IpPort.h"
#include "NwTypes.h"

//------------------------------------------------------------------------------

namespace NetworkBase
{
//  An IP port that supports a UDP-based protocol.
//
class UdpIpPort : public IpPort
{
public:
   //  See IpPort's constructor.
   //
   UdpIpPort(ipport_t port, const IpService* service);

   //  Not subclassed.
   //
   ~UdpIpPort();

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
private:
   //  Overridden to create a UdpIoThread for the port.
   //
   IoThread* CreateIoThread() override;
};
}
#endif
