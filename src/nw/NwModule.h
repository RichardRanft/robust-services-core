//==============================================================================
//
//  NwModule.h
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
#ifndef NWMODULE_H_INCLUDED
#define NWMODULE_H_INCLUDED

#include "Module.h"
#include "NbTypes.h"

//------------------------------------------------------------------------------

namespace NetworkBase
{
//  Module for initializing NetworkBase.
//
class NwModule : public NodeBase::Module
{
   friend class NodeBase::Singleton<NwModule>;
public:
   //  Overridden to enable modules that this one requires.
   //
   void Enable() override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
private:
   //  Private because this is a singleton.
   //
   NwModule();

   //  Private because this is a singleton.
   //
   ~NwModule();

   //  Overridden for restarts.
   //
   void Shutdown(NodeBase::RestartLevel level) override;

   //  Overridden for restarts.
   //
   void Startup(NodeBase::RestartLevel level) override;
};
}
#endif
