//==============================================================================
//
//  PotsIncrement.cpp
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
#include "PotsIncrement.h"
#include "CliCommand.h"
#include <cstddef>
#include <sstream>
#include <string>
#include <vector>
#include "BcCause.h"
#include "CliTextParm.h"
#include "CliThread.h"
#include "Debug.h"
#include "Formatters.h"
#include "FunctionGuard.h"
#include "LocalAddress.h"
#include "MbPools.h"
#include "NbCliParms.h"
#include "PotsCircuit.h"
#include "PotsCliParms.h"
#include "PotsFeature.h"
#include "PotsFeatureProfile.h"
#include "PotsFeatureRegistry.h"
#include "PotsProfile.h"
#include "PotsProfileRegistry.h"
#include "PotsProtocol.h"
#include "SbCliParms.h"
#include "ServiceCodeRegistry.h"
#include "Singleton.h"
#include "Switch.h"
#include "SysTypes.h"
#include "ToneRegistry.h"
#include "Tones.h"

using namespace SessionBase;
using namespace MediaBase;
using namespace CallBase;

//------------------------------------------------------------------------------

namespace PotsBase
{
//  Parameters that support basic types.
//
fixed_string PortExpl = "Switch::PortId";

fixed_string PotsFeatureOptExpl = "PotsFeature::Id";

fixed_string ToneOptExpl = "Tone::Id (default=all)";

//------------------------------------------------------------------------------
//
//  The ACTIVATE command.
//
class ActivateCommand : public CliCommand
{
public:
   ActivateCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string ActivateStr = "activate";
fixed_string ActivateExpl = "Activates a feature assigned to a DN.";

ActivateCommand::ActivateCommand() : CliCommand(ActivateStr, ActivateExpl)
{
   BindParm(*new DnMandParm);
   BindParm(*Singleton<PotsFeatureRegistry>::Instance()->featuresActivate_);
}

word ActivateCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("ActivateCommand.ProcessCommand");

   word id1;
   id_t id2;

   if(!GetIntParm(id1, cli)) return -1;

   auto pro = Singleton<PotsProfileRegistry>::Instance()->Profile(id1);
   if(pro == nullptr) return cli.Report(-3, NotRegisteredExpl);

   if(!GetTextIndex(id2, cli)) return -1;

   auto ftr = pro->FindFeature(id2);
   if(ftr == nullptr) return cli.Report(-3, NotSubscribedExpl);
   if(!ftr->Activate(*pro, cli)) return -4;
   return cli.Report(0, SuccessExpl);
}

//------------------------------------------------------------------------------
//
//  The CODES command.
//
class CodesCommand : public CliCommand
{
public:
   CodesCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string CodesStr = "codes";
fixed_string CodesExpl = "Displays service codes.";

CodesCommand::CodesCommand() : CliCommand(CodesStr, CodesExpl) { }

word CodesCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("CodesCommand.ProcessCommand");

   if(!cli.EndOfInput()) return -1;

   Singleton<ServiceCodeRegistry>::Instance()->Output(*cli.obuf, 2, true);
   return 0;
}

//------------------------------------------------------------------------------
//
//  The DEACTIVATE command.
//
class DeactivateCommand : public CliCommand
{
public:
   DeactivateCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string DeactivateStr = "deactivate";
fixed_string DeactivateExpl = "Deactivates a feature assigned to a DN.";

DeactivateCommand::DeactivateCommand() :
   CliCommand(DeactivateStr, DeactivateExpl)
{
   auto reg = Singleton<PotsFeatureRegistry>::Instance();
   BindParm(*new DnMandParm);
   BindParm(*reg->featuresDeactivate_);
}

word DeactivateCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("DeactivateCommand.ProcessCommand");

   word id1;
   id_t id2;

   if(!GetIntParm(id1, cli)) return -1;

   auto pro = Singleton<PotsProfileRegistry>::Instance()->Profile(id1);
   if(pro == nullptr) return cli.Report(-3, NotRegisteredExpl);

   if(!GetTextIndex(id2, cli)) return -1;
   if(!cli.EndOfInput()) return -1;

   auto ftr = pro->FindFeature(id2);
   if(ftr == nullptr) return cli.Report(-3, NotSubscribedExpl);
   if(!ftr->Deactivate(*pro)) return -4;
   return cli.Report(0, SuccessExpl);
}

//------------------------------------------------------------------------------
//
//  The DEREGISTER command.
//
class DeregisterCommand : public CliCommand
{
public:
   DeregisterCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string DeregisterStr = "deregister";
fixed_string DeregisterExpl = "Deletes a DN.";

DeregisterCommand::DeregisterCommand() :
   CliCommand(DeregisterStr, DeregisterExpl)
{
   BindParm(*new DnMandParm);
}

word DeregisterCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("DeregisterCommand.ProcessCommand");

   word id1;

   if(!GetIntParm(id1, cli)) return -1;
   if(!cli.EndOfInput()) return -1;

   auto pro = Singleton<PotsProfileRegistry>::Instance()->Profile(id1);
   if(pro == nullptr) return cli.Report(-3, NotRegisteredExpl);
   if(!pro->Deregister()) return -4;
   return cli.Report(0, SuccessExpl);
}

//------------------------------------------------------------------------------
//
//  The DNS command.
//
class DnsCommand : public CliCommand
{
public:
   DnsCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string DnsStr = "dns";
fixed_string DnsExpl = "Displays the profile(s) in a range of DNs.";

DnsCommand::DnsCommand() : CliCommand(DnsStr, DnsExpl)
{
   BindParm(*new DnMandParm);
   BindParm(*new DnOptParm);
}

word DnsCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("DnsCommand.ProcessCommand");

   word first, last;
   bool one = false;

   if(!GetIntParm(first, cli)) return -1;

   switch(GetIntParmRc(last, cli))
   {
   case None:
      last = first;
      [[fallthrough]];
   case Ok:
      break;
   default:
      return -1;
   }

   if(!cli.EndOfInput()) return -1;

   auto reg = Singleton<PotsProfileRegistry>::Instance();

   for(auto p = reg->FirstProfile(first); p != nullptr; NO_OP)
   {
      auto dn = p->GetDN();

      if(word(dn) <= last)
      {
         one = true;

         if(first == last)
         {
            p->Output(*cli.obuf, 2, true);
            return 0;
         }

         *cli.obuf << spaces(2) << strIndex(dn) << CRLF;
         p->Output(*cli.obuf, 4, false);
         p = reg->NextProfile(*p);
      }
      else
      {
         p = nullptr;
      }
   }

   if(!one) return cli.Report(-2, NoDnsExpl);
   return 0;
}

//------------------------------------------------------------------------------
//
//  The FEATURES command.
//
class FeaturesCommand : public CliCommand
{
public:
   FeaturesCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string FeaturesStr = "features";
fixed_string FeaturesExpl = "Displays features that can be assigned to a DN.";

FeaturesCommand::FeaturesCommand() : CliCommand(FeaturesStr, FeaturesExpl)
{
   BindParm(*new CliIntParm(PotsFeatureOptExpl, 0, PotsFeature::MaxId, true));
   BindParm(*new DispBVParm);
}

word FeaturesCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("FeaturesCommand.ProcessCommand");

   word id;
   bool all;
   char disp = 'b';

   switch(GetIntParmRc(id, cli))
   {
   case None: all = true; break;
   case Ok: all = false; break;
   default: return -1;
   }

   if(GetCharParmRc(disp, cli) == Error) return -1;
   if(!cli.EndOfInput()) return -1;

   auto reg = Singleton<PotsFeatureRegistry>::Instance();

   if(all)
   {
      reg->Output(*cli.obuf, 2, disp == 'v');
   }
   else
   {
      auto ftr = reg->Feature(id);
      if(ftr == nullptr) return cli.Report(-2, NoFeatureExpl);
      ftr->Output(*cli.obuf, 4, disp == 'v');
   }

   return 0;
}

//------------------------------------------------------------------------------
//
//  The MEPS command.
//
class MepsCommand : public CliCommand
{
public:
   MepsCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string MepsStr = "meps";
fixed_string MepsExpl = "Counts or displays media endpoints.";

MepsCommand::MepsCommand() : CliCommand(MepsStr, MepsExpl)
{
   BindParm(*new FactoryIdOptParm);
   BindParm(*new DispCSVParm);
}

word MepsCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("MepsCommand.ProcessCommand");

   word fid;
   char disp;

   if(!GetIdDispS(*this, cli, fid, disp)) return -1;
   if(!cli.EndOfInput()) return -1;

   auto pool = Singleton<MediaEndptPool>::Instance();
   size_t count = 0;

   if(disp == 'c')
   {
      auto items = pool->GetUsed();
      for(auto i = items.cbegin(); i != items.cend(); ++i)
         if((*i)->Passes(fid)) ++count;
      if(count == 0) return cli.Report(0, NoMepsExpl);
      *cli.obuf << spaces(2) << count << CRLF;
   }
   else if(disp == 's')
   {
      count = pool->Summarize(*cli.obuf, fid);
   }
   else
   {
      const auto& opts = (disp == 'v' ? VerboseOpt : NoFlags);
      count = pool->DisplayUsed(*cli.obuf, spaces(2), opts, fid);
      if(count == 0) return cli.Report(0, NoMepsExpl);
   }

   return count;
}

//------------------------------------------------------------------------------
//
//  The REGISTER command.
//
class RegisterCommand : public CliCommand
{
public:
   RegisterCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string RegisterStr = "register";
fixed_string RegisterExpl = "Adds a new DN.";

RegisterCommand::RegisterCommand() : CliCommand(RegisterStr, RegisterExpl)
{
   BindParm(*new DnMandParm);
}

word RegisterCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("RegisterCommand.ProcessCommand");

   word id1;

   if(!GetIntParm(id1, cli)) return -1;
   if(!cli.EndOfInput()) return -1;

   auto pro = Singleton<PotsProfileRegistry>::Instance()->Profile(id1);
   if(pro != nullptr) return cli.Report(-3, AlreadyRegistered);

   FunctionGuard guard(Guard_MemUnprotect);
   pro = new PotsProfile(id1);
   if(pro == nullptr) return cli.Report(-7, AllocationError);
   return cli.Report(0, SuccessExpl);
}

//------------------------------------------------------------------------------
//
//  The RESET command.
//
class ResetCommand : public CliCommand
{
public:
   ResetCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string ResetStr = "reset";
fixed_string ResetExpl = "Resets a DN to its initial state.";

ResetCommand::ResetCommand() : CliCommand(ResetStr, ResetExpl)
{
   BindParm(*new DnMandParm);
}

word ResetCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("ResetCommand.ProcessCommand");

   word id1;

   if(!GetIntParm(id1, cli)) return -1;
   if(!cli.EndOfInput()) return -1;

   auto pro = Singleton<PotsProfileRegistry>::Instance()->Profile(id1);
   if(pro == nullptr) return cli.Report(-3, NotRegisteredExpl);

   auto cct = pro->GetCircuit();
   if(cct == nullptr) return cli.Report(-3, NoCircuitExpl);

   auto msg = new Pots_NU_Message(nullptr, 20);
   PotsHeaderInfo phi;
   CauseInfo cci;

   phi.signal = PotsSignal::Release;
   phi.port = cct->TsPort();
   cci.cause = Cause::ResetCircuit;
   msg->AddHeader(phi);
   msg->AddCause(cci);

   cct->Profile()->ClearObjAddr(LocalAddress());

   if(!msg->Send(Message::External)) return cli.Report(-6, SendFailure);
   return cli.Report(0, SuccessExpl);
}

//------------------------------------------------------------------------------
//
//  The SUBSCRIBE command.
//
class SubscribeCommand : public CliCommand
{
public:
   SubscribeCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string SubscribeStr = "subscribe";
fixed_string SubscribeExpl = "Assigns a feature to a DN.";

SubscribeCommand::SubscribeCommand() : CliCommand(SubscribeStr, SubscribeExpl)
{
   auto reg = Singleton<PotsFeatureRegistry>::Instance();
   BindParm(*new DnMandParm);
   BindParm(*reg->featuresSubscribe_);
}

word SubscribeCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("SubscribeCommand.ProcessCommand");

   word id1;
   id_t id2;

   if(!GetIntParm(id1, cli)) return -1;

   auto pro = Singleton<PotsProfileRegistry>::Instance()->Profile(id1);
   if(pro == nullptr) return cli.Report(-3, NotRegisteredExpl);

   if(!GetTextIndex(id2, cli)) return -1;

   if(pro->FindFeature(id2) != nullptr)
      return cli.Report(-3, AlreadySubscribed);
   if(!pro->Subscribe(id2, cli)) return -4;
   return cli.Report(0, SuccessExpl);
}

//------------------------------------------------------------------------------
//
//  The TONES command.
//
class TonesCommand : public CliCommand
{
public:
   TonesCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string TonesStr = "tones";
fixed_string TonesExpl = "Displays tones.";

TonesCommand::TonesCommand() : CliCommand(TonesStr, TonesExpl)
{
   BindParm(*new CliIntParm(ToneOptExpl, 0, Tone::MaxId, true));
   BindParm(*new DispBVParm);
}

word TonesCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("TonesCommand.ProcessCommand");

   word id;
   bool all;
   char disp = 'b';

   switch(GetIntParmRc(id, cli))
   {
   case None: all = true; break;
   case Ok: all = false; break;
   default: return -1;
   }

   if(GetCharParmRc(disp, cli) == Error) return -1;
   if(!cli.EndOfInput()) return -1;

   auto reg = Singleton<ToneRegistry>::Instance();

   if(all)
   {
      reg->Output(*cli.obuf, 2, disp == 'v');
   }
   else
   {
      auto tone = reg->GetTone(id);
      if(tone == nullptr) return cli.Report(0, NoToneExpl);
      tone->Output(*cli.obuf, 2, disp == 'v');
   }

   return 0;
}

//------------------------------------------------------------------------------
//
//  The TSPORTS command.
//
class TsPortsCommand : public CliCommand
{
public:
   TsPortsCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string TsPortsStr = "tsports";
fixed_string TsPortsExpl =
   "Displays the circuit(s) in a range of timeswitch ports.";

TsPortsCommand::TsPortsCommand() : CliCommand(TsPortsStr, TsPortsExpl)
{
   BindParm(*new CliIntParm(PortExpl, 0, Switch::MaxPortId));
   BindParm(*new CliIntParm(PortExpl, 0, Switch::MaxPortId, true));
}

word TsPortsCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("TsPortsCommand.ProcessCommand");

   word first, last;
   bool one = false;

   if(!GetIntParm(first, cli)) return -1;

   switch(GetIntParmRc(last, cli))
   {
   case None:
      last = first;
      [[fallthrough]];
   case Ok:
      break;
   default:
      return -1;
   }

   if(!cli.EndOfInput()) return -1;

   auto tsw = Singleton<Switch>::Instance();

   for(auto i = first; i <= last; ++i)
   {
      auto cct = tsw->GetCircuit(i);

      if(cct != nullptr)
      {
         one = true;
         *cli.obuf << spaces(2) << strIndex(i);

         if(first == last)
         {
            *cli.obuf << CRLF;
            cct->Output(*cli.obuf, 4, true);
         }
         else
         {
            *cli.obuf << "circuit : " << strObj(cct) << CRLF;
         }
      }
   }

   if(!one) return cli.Report(0, NoCircuitsExpl);
   return 0;
}

//------------------------------------------------------------------------------
//
//  The UNSUBSCRIBE command.
//
class UnsubscribeCommand : public CliCommand
{
public:
   UnsubscribeCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string UnsubscribeStr = "unsubscribe";
fixed_string UnsubscribeExpl = "Removes a feature from a DN.";

UnsubscribeCommand::UnsubscribeCommand() :
   CliCommand(UnsubscribeStr, UnsubscribeExpl)
{
   auto reg = Singleton<PotsFeatureRegistry>::Instance();
   BindParm(*new DnMandParm);
   BindParm(*reg->featuresUnsubscribe_);
}

word UnsubscribeCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("UnsubscribeCommand.ProcessCommand");

   word id1;
   id_t id2;

   if(!GetIntParm(id1, cli)) return -1;

   auto pro = Singleton<PotsProfileRegistry>::Instance()->Profile(id1);
   if(pro == nullptr) return cli.Report(-3, NotRegisteredExpl);

   if(!GetTextIndex(id2, cli)) return -1;
   if(!cli.EndOfInput()) return -1;

   if(!pro->Unsubscribe(id2)) return -4;
   return cli.Report(0, SuccessExpl);
}

//------------------------------------------------------------------------------
//
//  The POTS increment.
//
fixed_string PotsText = "pots";
fixed_string PotsExpl = "POTS Increment";

PotsIncrement::PotsIncrement() : CliIncrement(PotsText, PotsExpl)
{
   Debug::ft("PotsIncrement.ctor");

   BindCommand(*new DnsCommand);
   BindCommand(*new CodesCommand);
   BindCommand(*new FeaturesCommand);
   BindCommand(*new RegisterCommand);
   BindCommand(*new DeregisterCommand);
   BindCommand(*new SubscribeCommand);
   BindCommand(*new ActivateCommand);
   BindCommand(*new DeactivateCommand);
   BindCommand(*new UnsubscribeCommand);
   BindCommand(*new ResetCommand);
   BindCommand(*new TsPortsCommand);
   BindCommand(*new TonesCommand);
   BindCommand(*new MepsCommand);
}

//------------------------------------------------------------------------------

PotsIncrement::~PotsIncrement()
{
   Debug::ftnt("PotsIncrement.dtor");
}
}
