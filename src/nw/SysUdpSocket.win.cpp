//==============================================================================
//
//  SysUdpSocket.win.cpp
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
#ifdef OS_WIN

#include "SysUdpSocket.h"
#include <winerror.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include "Debug.h"
#include "IpPortRegistry.h"
#include "NwLogs.h"
#include "Restart.h"
#include "SysIpL3Addr.h"
#include "UdpIpService.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace NetworkBase
{
size_t SysUdpSocket::MaxUdpSize_ = 0;

//------------------------------------------------------------------------------

SysUdpSocket::SysUdpSocket(ipport_t port,
   const UdpIpService* service, AllocRc& rc) : SysSocket(port, service, rc)
{
   Debug::ft("SysUdpSocket.ctor");

   //  If the maximum UDP message size has not been set, set it now.
   //
   if((MaxUdpSize_ == 0) && (rc == AllocOk))
   {
      size_t max;
      int maxsize = sizeof(max);

      if(getsockopt(Socket(), SOL_SOCKET, SO_MAX_MSG_SIZE,
         (char*) &max, &maxsize) == SOCKET_ERROR)
      {
         OutputLog(NetworkSocketError,
            "getsockopt/MAX_MSG_SIZE", WSAGetLastError());
         rc = GetOptionError;
         return;
      }

      MaxUdpSize_ = (max < MaxMsgSize ? max : MaxMsgSize);
   }
}

//------------------------------------------------------------------------------

fn_name SysUdpSocket_RecvFrom = "SysUdpSocket.RecvFrom";

word SysUdpSocket::RecvFrom(byte_t* buff, size_t size, SysIpL3Addr& remAddr)
{
   Debug::ft(SysUdpSocket_RecvFrom);

   if(buff == nullptr)
   {
      Debug::SwLog(SysUdpSocket_RecvFrom, "invalid buffer", 0);
      return -1;
   }

   if((buff == nullptr) || (size == 0))
   {
      Debug::SwLog(SysUdpSocket_RecvFrom, "invalid size", size);
      return -1;
   }

   sockaddr_in ipv4peer;
   sockaddr_in6 ipv6peer;
   sockaddr* peer = nullptr;
   int peersize = 0;

   auto ipv6 = IpPortRegistry::UseIPv6();

   if(ipv6)
   {
      peer = (sockaddr*) &ipv6peer;
      peersize = sizeof(ipv6peer);
   }
   else
   {
      peer = (sockaddr*) &ipv4peer;
      peersize = sizeof(ipv4peer);
   }

   auto rcvd = recvfrom(Socket(),
      reinterpret_cast<char*>(buff), size, 0, peer, &peersize);

   if(rcvd == SOCKET_ERROR)
   {
      auto error = WSAGetLastError();

      switch(error)
      {
      case WSAEWOULDBLOCK:
         //
         //  There is nothing on the socket, but it hasn't yet been made
         //  blocking.
         //
         break;
      case WSAEINTR:
         //
         //  Don't log this during a restart, when an I/O thread's socket
         //  is released so that the thread can exit.
         //
         if(Restart::GetLevel() >= RestartCold)
         {
            break;
         }
         [[fallthrough]];
      default:
         OutputLog(NetworkSocketError, "recvfrom", error);
      }

      return -1;
   }

   NetworkIsUp();

   if(ipv6)
      remAddr.NetworkToHost(ipv6peer.sin6_addr.s6_words, ipv6peer.sin6_port);
   else
      remAddr.NetworkToHost(ipv4peer.sin_addr.s_addr, ipv4peer.sin_port);
   return rcvd;
}

//------------------------------------------------------------------------------

fn_name SysUdpSocket_SendTo = "SysUdpSocket.SendTo";

word SysUdpSocket::SendTo
   (const byte_t* data, size_t size, const SysIpL3Addr& remAddr)
{
   Debug::ft(SysUdpSocket_SendTo);

   if(data == nullptr)
   {
      Debug::SwLog(SysUdpSocket_SendTo, "invalid data", 0);
      return -2;
   }

   if((data == nullptr) || (size == 0))
   {
      Debug::SwLog(SysUdpSocket_SendTo, "invalid size", size);
      return -2;
   }

   if(!SetBlocking(false)) return GetError();

   sockaddr_in ipv4peer;
   sockaddr_in6 ipv6peer;
   sockaddr* peer = nullptr;
   int peersize = 0;

   auto ipv6 = IpPortRegistry::UseIPv6();

   if(ipv6)
   {
      ipv6peer.sin6_family = AF_INET6;
      remAddr.HostToNetwork(ipv6peer.sin6_addr.s6_words, ipv6peer.sin6_port);
      ipv6peer.sin6_flowinfo = 0;
      ipv6peer.sin6_scope_id = 0;
      peer = (sockaddr*) &ipv6peer;
      peersize = sizeof(ipv6peer);
   }
   else
   {
      ipv4peer.sin_family = AF_INET;
      remAddr.HostToNetwork
         ((IPv4Addr&) ipv4peer.sin_addr.s_addr, ipv4peer.sin_port);
      peer = (sockaddr*) &ipv4peer;
      peersize = sizeof(ipv4peer);
   }

   auto sent = sendto(Socket(),
      reinterpret_cast<const char*>(data), size, 0, peer, peersize);

   if(sent == SOCKET_ERROR)
   {
      return SetError(WSAGetLastError());
   }

   NetworkIsUp();
   return sent;
}
}
#endif
