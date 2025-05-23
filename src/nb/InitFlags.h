//==============================================================================
//
//  InitFlags.h
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
#ifndef INITFLAGS_H_INCLUDED
#define INITFLAGS_H_INCLUDED

//------------------------------------------------------------------------------

namespace NodeBase
{
//  This supports debugging during initialization.  To change the value
//  of a flag, change the implementation of the static member function
//  that returns its value.
//
namespace InitFlags
{
   //  Return true to suspend RootThread indefinitely after booting.
   //  Useful to prevent it from constantly reappearing when debugging
   //  context switching.
   //
   bool SuspendRoot();  // default=false

   //  Return true to allow breakpoints during initialization.  If this
   //  returns false and a breakpoint is hit, RootThread will time out
   //  and generate logs or try to restart initialization.
   //
   bool AllowBreak();  // default=true (false if FIELD_LOAD #defined)

   //  Return true to test the initialization timeout by delaying
   //  indefinitely during initialization.
   //
   bool CauseTimeout();  // default=false

   //  Return true to trace initialization.  This traces the initial
   //  boot, not a restart.  If set, ModuleRegistry::Startup creates
   //  a Deferred work item that stops tracing soon after the system
   //  has booted.
   //
   bool TraceInit();  // default=true (false if FIELD_LOAD #defined)
}
}
#endif
