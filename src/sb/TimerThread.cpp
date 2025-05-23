//==============================================================================
//
//  TimerThread.cpp
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
#include "TimerThread.h"
#include <ratio>
#include "Debug.h"
#include "Duration.h"
#include "SbDaemons.h"
#include "SbTracer.h"
#include "Singleton.h"
#include "TimerRegistry.h"
#include "ToolTypes.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace SessionBase
{
TimerThread::TimerThread() :
   Thread(PayloadFaction, Singleton<TimerDaemon>::Instance())
{
   Debug::ft("TimerThread.ctor");

   SetInitialized();
}

//------------------------------------------------------------------------------

TimerThread::~TimerThread()
{
   Debug::ftnt("TimerThread.dtor");
}

//------------------------------------------------------------------------------

c_string TimerThread::AbbrName() const
{
   return TimerDaemonName;
}

//------------------------------------------------------------------------------

TraceStatus TimerThread::CalcStatus(bool dynamic) const
{
   //  When determining whether to trace a timer thread, this function takes
   //  the ">include/exclude/clear timers" commands into account.  Settings
   //  have the following precedence:
   //  1. The setting for this thread.
   //  2. The setting for timer work.
   //  3. The setting for this thread's faction.
   //  4. The ">include all on/off" command.
   //  The latter two are handled by invoking the Thread class.
   //
   auto status = GetStatus();
   if(status != TraceDefault) return status;
   status = Singleton<SbTracer>::Instance()->TimersStatus();
   if(status != TraceDefault) return status;
   return Thread::CalcStatus(dynamic);
}

//------------------------------------------------------------------------------

void TimerThread::Destroy()
{
   Debug::ft("TimerThread.Destroy");

   Singleton<TimerThread>::Destroy();
}

//------------------------------------------------------------------------------

void TimerThread::Enter()
{
   Debug::ft("TimerThread.Enter");

   //  Every second, tell our registry to process the next timer queue.
   //
   auto reg = Singleton<TimerRegistry>::Instance();
   msecs_t sleep(ONE_SEC);

   while(true)
   {
      Pause(sleep);
      reg->ProcessWork();

      //  Sleep for one second, minus the amount of time that we just ran.
      //
      auto runTime = CurrTimeRunning();
      sleep = (runTime > ONE_SEC ? TIMEOUT_IMMED : ONE_SEC - runTime);
   }
}

//------------------------------------------------------------------------------

void TimerThread::Patch(sel_t selector, void* arguments)
{
   Thread::Patch(selector, arguments);
}
}
