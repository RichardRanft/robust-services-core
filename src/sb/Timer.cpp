//==============================================================================
//
//  Timer.cpp
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
#include "Timer.h"
#include <ostream>
#include <string>
#include "Algorithms.h"
#include "Context.h"
#include "Debug.h"
#include "Formatters.h"
#include "Parameter.h"
#include "ProtocolSM.h"
#include "Q1Way.h"
#include "Q2Way.h"
#include "SbAppIds.h"
#include "SbPools.h"
#include "SbTrace.h"
#include "Signal.h"
#include "Singleton.h"
#include "SteadyTime.h"
#include "SysTypes.h"
#include "TimerProtocol.h"
#include "TimerRegistry.h"
#include "TlvMessage.h"
#include "TlvParameter.h"
#include "ToolTypes.h"
#include "TraceBuffer.h"

using namespace NodeBase;
using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace SessionBase
{
Timer::Timer
   (ProtocolSM& psm, Base& owner, TimerId tid, uint32_t secs, bool repeat) :
   psm_(&psm),
   owner_(&owner),
   tid_(tid),
   repeat_(repeat),
   qid_(NilQId),
   secs_(secs),
   remaining_(secs)
{
   Debug::ft("Timer.ctor");

   auto reg = Singleton<TimerRegistry>::Instance();
   qid_ = reg->CalcQId(secs_);
   reg->timerq_[qid_].Henq(*this);
   psm_->timerq_.Henq(*this);

   //  Record the timer's creation if this context is traced.
   //
   TransTrace* trans = nullptr;

   if(Context::RunningContextTraced(trans))
   {
      auto warp = SteadyTime::Now();
      auto buff = Singleton<TraceBuffer>::Instance();

      if(buff->ToolIsOn(ContextTracer))
      {
         auto rec = new TimerTrace(TimerTrace::Creation, *this);
         buff->Insert(rec);
      }

      if(trans != nullptr) trans->ResumeTime(warp);
   }
}

//------------------------------------------------------------------------------

Timer::~Timer()
{
   Debug::ftnt("Timer.dtor");

   //  Record the timer's deletion if this context is traced.  If the timer
   //  is being deleted because it expired and the TimerRegistry is sending
   //  a timeout message, there will be no running context and the deletion
   //  will not be recorded.
   //
   TransTrace* trans = nullptr;

   if(Context::RunningContextTraced(trans))
   {
      auto warp = SteadyTime::Now();
      auto buff = Singleton<TraceBuffer>::Extant();

      if(buff->ToolIsOn(ContextTracer))
      {
         auto rec = new TimerTrace(TimerTrace::Deletion, *this);
         buff->Insert(rec);
      }

      if(trans != nullptr) trans->ResumeTime(warp);
   }

   Deregister();
   Exqueue();
}

//------------------------------------------------------------------------------

void Timer::Cleanup()
{
   Debug::ft("Timer.Cleanup");

   Deregister();
   Pooled::Cleanup();
}

//------------------------------------------------------------------------------

void Timer::Deregister()
{
   Debug::ftnt("Timer.Deregister");

   if(qid_ == NilQId) return;

   //  Remove the timer from the timer registry and clear our queue
   //  identifier so that we no longer think we're on a queue.
   //
   Singleton<TimerRegistry>::Extant()->timerq_[qid_].Exq(*this);
   qid_ = NilQId;
}

//------------------------------------------------------------------------------

void Timer::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Pooled::Display(stream, prefix, options);

   stream << prefix << "psm       : " << psm_ << CRLF;
   stream << prefix << "owner     : " << owner_ << CRLF;
   stream << prefix << "tid       : " << int(tid_) << CRLF;
   stream << prefix << "repeat    : " << repeat_ << CRLF;
   stream << prefix << "qid       : " << qid_ << CRLF;
   stream << prefix << "link      : " << CRLF;
   link_.Display(stream, prefix + spaces(2));
   stream << prefix << "secs      : " << secs_ << CRLF;
   stream << prefix << "remaining : " << remaining_ << CRLF;
}

//------------------------------------------------------------------------------

fn_name Timer_Exqueue = "Timer.Exqueue";

void Timer::Exqueue()
{
   Debug::ft(Timer_Exqueue);

   if(psm_ == nullptr) return;

   if(!psm_->timerq_.Exq(*this))
   {
      Debug::SwLog(Timer_Exqueue, "Exq failed", Tid());
   }

   psm_ = nullptr;
}

//------------------------------------------------------------------------------

ptrdiff_t Timer::LinkDiff()
{
   uintptr_t local;
   auto fake = reinterpret_cast<const Timer*>(&local);
   return ptrdiff(&fake->link_, fake);
}

//------------------------------------------------------------------------------

void* Timer::operator new(size_t size)
{
   Debug::ft("Timer.operator new");

   return Singleton<TimerPool>::Instance()->DeqBlock(size);
}

//------------------------------------------------------------------------------

bool Timer::Passes(uint32_t selector) const
{
   return ((selector == 0) || (Psm()->GetFactory() == selector));
}

//------------------------------------------------------------------------------

void Timer::Patch(sel_t selector, void* arguments)
{
   Pooled::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

void Timer::Restart()
{
   Debug::ft("Timer.Restart");

   auto reg = Singleton<TimerRegistry>::Instance();

   if(qid_ < MaxQId)
   {
      //  Move this timer to the queue that will be reached in secs_
      //  seconds.
      //
      auto secs = (secs_ == 0 ? 1 : secs_);
      auto nextq = reg->CalcQId(secs);
      Deregister();

      qid_ = nextq;
      reg->timerq_[qid_].Henq(*this);
   }
   else
   {
      //  This timer is MaxQId seconds or more, so it never moves.
      //  Just reset its countdown value.
      //
      remaining_ = secs_;
   }
}

//------------------------------------------------------------------------------

fn_name Timer_SendTimeout = "Timer.SendTimeout";

void Timer::SendTimeout()
{
   Debug::ft(Timer_SendTimeout);

   //  If this is a repetitive timer, move it to its next queue.
   //  If it isn't repetitive, remove it from its current queue.
   //  This clears the psm_ field, so save it.
   //
   auto psm = psm_;

   if(repeat_)
      Restart();
   else
      Exqueue();

   //  Queue a timeout message on the PSM and inject it.
   //
   auto size = sizeof(TlvParmHeader) + sizeof(TimeoutInfo);
   auto msg = new TlvMessage(psm, size);

   msg->SetProtocol(TimerProtocolId);
   msg->SetSignal(Signal::Timeout);
   msg->SetPriority(PROGRESS);

   TimeoutInfo info;
   info.owner = owner_;
   info.tid = tid_;

   msg->AddType(info, Parameter::Timeout);

   if(!msg->SendToSelf())
   {
      Debug::SwLog(Timer_SendTimeout, "send failed", psm->GetFactory());
   }

   //  If the timer isn't repetitive, delete it.
   //
   if(!repeat_) delete this;
}
}
