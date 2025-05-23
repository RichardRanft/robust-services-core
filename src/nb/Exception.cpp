//==============================================================================
//
//  Exception.cpp
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
#include "Exception.h"
#include <ios>
#include <utility>
#include "Debug.h"
#include "Duration.h"
#include "SysStackTrace.h"
#include "Thread.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
Exception::Exception(bool stack) : stack_(nullptr)
{
   //  Reenable Debug functions before tracing this function.
   //
   Thread::ResetDebugFlags();
   Debug::ft("Exception.ctor");  //@

   //  Capturing a stack trace takes time, so give the thread an extra
   //  20 msecs.
   //
   Thread::ExtendTime(msecs_t(20));

   if(stack)
   {
      stack_.reset(new std::ostringstream);
      *stack_ << std::boolalpha << std::nouppercase;
      SysStackTrace::Display(*stack_);
   }
}

//------------------------------------------------------------------------------

Exception::~Exception()
{
   Debug::ftnt("Exception.dtor");
}

//------------------------------------------------------------------------------

Exception::Exception(const Exception& that) :
   exception(that),
   stack_(std::move(that.stack_))
{
   Debug::ft("Exception.ctor(copy)");
}

//------------------------------------------------------------------------------

Exception::Exception(Exception&& that) :
   exception(that),
   stack_(std::move(that.stack_))
{
   Debug::ft("Exception.ctor(move)");
}

//------------------------------------------------------------------------------

void Exception::Display(ostream& stream, const string& prefix) const
{
   //  There is nothing to display; the stack is provided separately.
}

//------------------------------------------------------------------------------

fixed_string ExceptionExpl = "Unspecified Exception";

const char* Exception::what() const noexcept
{
   //  Subclasses are supposed to override this.
   //
   return ExceptionExpl;
}
}
