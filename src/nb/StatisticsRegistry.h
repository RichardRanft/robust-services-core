//==============================================================================
//
//  StatisticsRegistry.h
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
#ifndef STATISTICSREGISTRY_H_INCLUDED
#define STATISTICSREGISTRY_H_INCLUDED

#include "Dynamic.h"
#include <iosfwd>
#include "NbTypes.h"
#include "Registry.h"
#include "SteadyTime.h"
#include "SystemTime.h"
#include "SysTypes.h"

namespace NodeBase
{
   class Statistic;
}

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Global registry for statistics.
//
class StatisticsRegistry : public Dynamic
{
   friend class Singleton<StatisticsRegistry>;
   friend class Statistic;
   friend class StatisticsGroup;
public:
   //  Deleted to prohibit copying.
   //
   StatisticsRegistry(const StatisticsRegistry& that) = delete;

   //  Deleted to prohibit copy assignment.
   //
   StatisticsRegistry& operator=(const StatisticsRegistry& that) = delete;

   //  Returns the group registered against GID.
   //
   StatisticsGroup* GetGroup(id_t gid) const;

   //  Displays all statistics in STREAM.
   //
   void DisplayStats(std::ostream& stream, const Flags& options) const;

   //  Returns the clock time when the current interval started.
   //
   static const SystemTime::Point& StartTime() { return StartTime_; }

   //  Returns the tick time when the current interval started.
   //
   static const SteadyTime::Point& StartPoint() { return StartPoint_; }

   //  Invoked at regular intervals to start a new measurement period.  If
   //  FIRST is true, previous values in Statistics::total_ are discarded.
   //
   void StartInterval(bool first);

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;

   //  Overridden for restarts.
   //
   void Startup(RestartLevel level) override;
private:
   //  Private because this is a singleton.
   //
   StatisticsRegistry();

   //  Private because this is a singleton.
   //
   ~StatisticsRegistry();

   //  Adds STAT, which is explained by EXPL, to the registry.
   //
   bool BindStat(Statistic& stat);

   //  Removes STAT from the registry.
   //
   void UnbindStat(Statistic& stat);

   //  Adds GROUP to the registry.
   //
   bool BindGroup(StatisticsGroup& group);

   //  Removes GROUP from the registry.
   //
   void UnbindGroup(StatisticsGroup& group);

   //  The global registry of statistics.
   //
   Registry<Statistic> stats_;

   //  The global registry of statistics groups.
   //
   Registry<StatisticsGroup> groups_;

   //  The time when the most recent statistics interval began.
   //
   static SystemTime::Point StartTime_;

   //  The point when the most recent statistics interval began.
   //
   static SteadyTime::Point StartPoint_;
};
}
#endif
