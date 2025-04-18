//==============================================================================
//
//  AlarmRegistry.h
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
#ifndef ALARMREGISTRY_H_INCLUDED
#define ALARMREGISTRY_H_INCLUDED

#include "Immutable.h"
#include <string>
#include "NbTypes.h"
#include "Registry.h"

namespace NodeBase
{
   class Alarm;
}

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Global registry for alarms.
//
class AlarmRegistry : public Immutable
{
   friend class Singleton<AlarmRegistry>;
   friend class Alarm;
public:
   //  Deleted to prohibit copying.
   //
   AlarmRegistry(const AlarmRegistry& that) = delete;

   //  Deleted to prohibit copy assignment.
   //
   AlarmRegistry& operator=(const AlarmRegistry& that) = delete;

   //  Returns the alarm associated with NAME.
   //
   Alarm* Find(const std::string& name) const;

   //  Returns the registry.
   //
   const Registry<Alarm>& Alarms() const { return alarms_; }

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;

   //  Overridden for restarts.
   //
   void Shutdown(RestartLevel level) override;

   //  Overridden for restarts.
   //
   void Startup(RestartLevel level) override;

   //  Overridden to display each alarm.
   //
   size_t Summarize(std::ostream& stream, uint32_t selector) const override;
private:
   //  Private because this is a singleton.
   //
   AlarmRegistry();

   //  Private because this is a singleton.
   //
   ~AlarmRegistry();

   //  Registers GROUP.
   //
   bool BindAlarm(Alarm& alarm);

   //  Removes ALARM from the registry.
   //
   void UnbindAlarm(Alarm& alarm);

   //  The registry of alarms.
   //
   Registry<Alarm> alarms_;
};
}
#endif
