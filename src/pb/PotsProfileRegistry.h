//==============================================================================
//
//  PotsProfileRegistry.h
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
#ifndef POTSPROFILEREGISTRY_H_INCLUDED
#define POTSPROFILEREGISTRY_H_INCLUDED

#include "Protected.h"
#include "BcAddress.h"
#include "NbTypes.h"
#include "Registry.h"

namespace PotsBase
{
   class PotsProfile;
}

using namespace NodeBase;
using namespace CallBase;

//------------------------------------------------------------------------------

namespace PotsBase
{
//  Registry for POTS profiles.
//
class PotsProfileRegistry : public Protected
{
   friend class Singleton<PotsProfileRegistry>;
   friend class PotsProfile;
public:
   //  Deleted to prohibit copying.
   //
   PotsProfileRegistry(const PotsProfileRegistry& that) = delete;

   //  Deleted to prohibit copy assignment.
   //
   PotsProfileRegistry& operator=(const PotsProfileRegistry& that) = delete;

   //  Returns DN's profile.
   //
   PotsProfile* Profile(Address::DN dn) const;

   //  Returns DN's profile.  If DN is not registered, searches sequentially
   //  starting at DN and returns the profile of the next registered DN.
   //
   PotsProfile* FirstProfile(Address::DN dn) const;

   //  Returns the profile that follows PROFILE.
   //
   PotsProfile* NextProfile(const PotsProfile& profile) const;

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for restarts.
   //
   void Shutdown(RestartLevel level) override;

   //  Overridden for restarts.
   //
   void Startup(RestartLevel level) override;
private:
   //  Private because this is a singleton.
   //
   PotsProfileRegistry();

   //  Private because this is a singleton.
   //
   ~PotsProfileRegistry();

   //  Adds PROFILE to the registry.
   //
   bool BindProfile(PotsProfile& profile);

   //  Removes PROFILE from the registry.
   //
   void UnbindProfile(PotsProfile& profile);

   //  The registry of POTS profiles.
   //
   Registry<PotsProfile> profiles_;
};
}
#endif
