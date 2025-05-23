//==============================================================================
//
//  BotTracer.h
//
//  Copyright (C) 2019-2022  Greg Utas
//
//  Diplomacy AI Client - Part of the DAIDE project (www.daide.org.uk).
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
#ifndef BOTTRACER_H_INCLUDED
#define BOTTRACER_H_INCLUDED

#include "Permanent.h"
#include "NbTypes.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace Diplomacy
{
class BotTracer : public Permanent
{
   friend class Singleton<BotTracer>;
public:
   //  Deleted to prohibit copying.
   //
   BotTracer(const BotTracer& that) = delete;

   //  Deleted to prohibit copy assignment.
   //
   BotTracer& operator=(const BotTracer& that) = delete;
private:
   //  Private because this is a singleton.
   //
   BotTracer();

   //  Private because this is a singleton.
   //
   ~BotTracer();
};
}
#endif
