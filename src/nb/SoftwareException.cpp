//==============================================================================
//
//  SoftwareException.cpp
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
#include "SoftwareException.h"
#include <ostream>
#include "Debug.h"
#include "Formatters.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
SoftwareException::SoftwareException(debug64_t errval, debug64_t offset) :
   Exception(true),
   errval_(errval),
   errstr_(EMPTY_STR),
   offset_(offset)
{
   Debug::ft("SoftwareException.ctor");
}

//------------------------------------------------------------------------------

SoftwareException::SoftwareException(const string& errstr, debug64_t offset) :
   Exception(true),
   errval_(0),
   errstr_(errstr),
   offset_(offset)
{
   Debug::ft("SoftwareException.ctor(string)");
}

//------------------------------------------------------------------------------

SoftwareException::~SoftwareException()
{
   Debug::ftnt("SoftwareException.dtor");
}

//------------------------------------------------------------------------------

void SoftwareException::Display(ostream& stream, const string& prefix) const
{
   if(errstr_.empty())
      stream << prefix << "errval : " << strHex(errval_) << CRLF;
   else
      stream << prefix << "errstr : " << errstr_ << CRLF;

   stream << prefix << "offset : " << strHex(offset_) << CRLF;
}

//------------------------------------------------------------------------------

fixed_string SoftwareExceptionExpl = "Software Error";

const char* SoftwareException::what() const noexcept
{
   return SoftwareExceptionExpl;
}
}
