//==============================================================================
//
//  MbPools.h
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
#ifndef MBPOOLS_H_INCLUDED
#define MBPOOLS_H_INCLUDED

#include "ObjectPool.h"
#include "NbTypes.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace MediaBase
{
//  Pool for MediaEndpt objects.
//
class MediaEndptPool : public ObjectPool
{
public:
   //  Overridden to display MEPs selected by the FactoryId of each one's PSM.
   //
   size_t Summarize(std::ostream& stream, uint32_t selector) const override;
private:
   friend class Singleton<MediaEndptPool>;

   //  Private because this is a singleton.
   //
   MediaEndptPool();

   //  Private because this is a singleton.
   //
   ~MediaEndptPool();
};
}
#endif
