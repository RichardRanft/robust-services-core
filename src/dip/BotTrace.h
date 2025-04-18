//==============================================================================
//
//  BotTrace.h
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
#ifndef BOTTRACE_H_INCLUDED
#define BOTTRACE_H_INCLUDED

#include "TimedRecord.h"

namespace Diplomacy
{
   struct DipHeader;
   class DipIpBuffer;
}

using namespace NodeBase;

//------------------------------------------------------------------------------
//
//  Trace records for SessionBase objects.
//
namespace Diplomacy
{
//  Records an entire incoming or outgoing message.
//
class BotTrace : public TimedRecord
{
public:
   //  Deleted to prohibit copying.
   //
   BotTrace(const BotTrace& that) = delete;

   //  Deleted to prohibit copy assignment.
   //
   BotTrace& operator=(const BotTrace& that) = delete;

   //  Types of buffer trace records.
   //
   static const Id IcMsg = 1;  // incoming message
   static const Id OgMsg = 2;  // outgoing message

   //  Creates an SbIpBuffer trace for BUFF, travelling in the direction
   //  specified by RID.
   //
   BotTrace(Id rid, const DipIpBuffer& buff);

   //  Releases buff_.  Not subclassed.
   //
   ~BotTrace();

   //  Returns the message's header.
   //
   DipHeader* Header() const;

   //  Overridden to display the trace record.
   //
   bool Display(std::ostream& stream, const std::string& opts) override;
private:
   //  Overridden to claim buff_.
   //
   void ClaimBlocks() override;

   //  Overridden to return a string for displaying this type of record.
   //
   c_string EventString() const override;

   //  Overridden to nullify the record if buff_ will vanish.
   //
   void Shutdown(RestartLevel level) override;

   //  A clone of the buffer being captured.
   //
   DipIpBuffer* buff_;

   //  Set if the buffer caused a trap.
   //
   bool corrupt_;
};
}
#endif
