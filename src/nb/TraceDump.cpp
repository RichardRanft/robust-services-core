//==============================================================================
//
//  TraceDump.cpp
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
#include "TraceDump.h"
#include <ostream>
#include "Debug.h"
#include "FunctionTrace.h"
#include "Singleton.h"
#include "TraceBuffer.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
fixed_string EndOfTrace = "END OF TRACE";

fixed_string Header = "mm:ss.ttt  Thr  Event  TotalTime   NetTime  Function\n"
                      "---------  ---  -----  ---------   -------  --------\n";
//                    |        9..  3..    5..        9..       8..<func>
//
//------------------------------------------------------------------------------

TraceRc TraceDump::Generate(ostream& stream, const string& opts)
{
   Debug::ft("TraceDump.Generate");

   FunctionTrace::Process(opts);

   auto buff = Singleton<TraceBuffer>::Instance();
   buff->DisplayStart(stream);

   stream << Header;

   //  Step through the trace buffer, displaying a trace record if the
   //  tool that created it is enabled.  This allows a single trace to
   //  be output several times, focusing on a different subset of the
   //  trace records each time.
   //
   buff->SetTool(ToolBuffer, true);
   buff->Lock();
   {
      TraceRecord* rec = nullptr;
      auto& mask = Flags().set();

      for(buff->Next(rec, mask); rec != nullptr; buff->Next(rec, mask))
      {
         if(buff->ToolIsOn(rec->Owner()))
         {
            if(rec->Display(stream, opts)) stream << CRLF;
         }
      }
   }
   buff->Unlock();
   buff->SetTool(ToolBuffer, false);

   stream << EndOfTrace << CRLF;
   return TraceOk;
}

//------------------------------------------------------------------------------

const string& TraceDump::Tab()
{
   static const string TabStr(TabWidth, SPACE);

   return TabStr;
}
}
