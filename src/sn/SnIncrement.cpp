//==============================================================================
//
//  SnIncrement.cpp
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
#include "SnIncrement.h"
#include "CliCommand.h"
#include <sstream>
#include "CliIntParm.h"
#include "CliThread.h"
#include "Debug.h"
#include "NbCliParms.h"
#include "PotsCliParms.h"
#include "PotsTreatmentRegistry.h"
#include "PotsTreatments.h"
#include "Singleton.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace PotsBase
{
//  The TREATMENTS command.
//
fixed_string TreatmentQIdOptExpl = "PotsTreatmentRegistry::QId (default=all)";

class TreatmentsCommand : public CliCommand
{
public:
   TreatmentsCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string TreatmentsStr = "treatments";
fixed_string TreatmentsExpl = "Displays treatments.";

TreatmentsCommand::TreatmentsCommand() :
   CliCommand(TreatmentsStr, TreatmentsExpl)
{
   BindParm(*new CliIntParm
      (TreatmentQIdOptExpl, 0, PotsTreatmentQueue::MaxQId, true));
   BindParm(*new DispBVParm);
}

word TreatmentsCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("TreatmentsCommand.ProcessCommand");

   word qid;
   bool all;
   char disp = 'b';

   switch(GetIntParmRc(qid, cli))
   {
   case None: all = true; break;
   case Ok: all = false; break;
   default: return -1;
   }

   if(GetCharParmRc(disp, cli) == Error) return -1;
   if(!cli.EndOfInput()) return -1;

   auto reg = Singleton<PotsTreatmentRegistry>::Instance();

   if(all)
   {
      reg->Output(*cli.obuf, 2, disp == 'v');
   }
   else
   {
      auto tq = reg->TreatmentQ(qid);
      if(tq == nullptr) return cli.Report(0, NoTreatmentExpl);
      tq->Output(*cli.obuf, 2, disp == 'v');
   }

   return 0;
}

//------------------------------------------------------------------------------
//
//  The Service Node increment.
//
fixed_string SnText = "sn";
fixed_string SnExpl = "Service Node Increment";

SnIncrement::SnIncrement() : CliIncrement(SnText, SnExpl)
{
   Debug::ft("SnIncrement.ctor");

   BindCommand(*new TreatmentsCommand);
}

//------------------------------------------------------------------------------

SnIncrement::~SnIncrement()
{
   Debug::ftnt("SnIncrement.dtor");
}
}
