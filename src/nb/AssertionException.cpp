//==============================================================================
//
//  AssertionException.cpp
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
#include "AssertionException.h"
#include <ostream>
#include <string>
#include "Debug.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
AssertionException::AssertionException(debug64_t errval) : Exception(true),
   errval_(errval)
{
   Debug::ft("AssertionException.ctor");
}

//------------------------------------------------------------------------------

AssertionException::~AssertionException()
{
   Debug::ftnt("AssertionException.dtor");
}

//------------------------------------------------------------------------------

void AssertionException::Display(ostream& stream, const string& prefix) const
{
   Exception::Display(stream, prefix);

   stream << prefix << "errval : " << errval_ << CRLF;
}

//------------------------------------------------------------------------------

fixed_string AssertionExceptionExpl = "Assertion Failed";

const char* AssertionException::what() const noexcept
{
   return AssertionExceptionExpl;
}
}
