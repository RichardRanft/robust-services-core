//==============================================================================
//
//  CodeFileSet.h
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
#ifndef CODEFILESET_H_INCLUDED
#define CODEFILESET_H_INCLUDED

#include "CodeSet.h"
#include <string>
#include <vector>
#include "LibraryItem.h"
#include "LibraryTypes.h"

//------------------------------------------------------------------------------

namespace CodeTools
{
//  A collection of code files.
//
class CodeFileSet : public CodeSet
{
public:
   //  Identifies SET with NAME.
   //
   CodeFileSet(const std::string& name, const LibItemSet* items);

   //  Override the operators supported by a set of code files.
   //
   LibrarySet* AffectedBy() const override;
   LibrarySet* Affecters() const override;
   LibrarySet* CommonAffecters() const override;
   LibrarySet* DeclaredBy() const override;
   LibrarySet* Directories() const override;
   LibrarySet* FileName(const LibrarySet* that) const override;
   LibrarySet* Files() const override;
   LibrarySet* FileType(const LibrarySet* that) const override;
   LibrarySet* FoundIn(const LibrarySet* that) const override;
   LibrarySet* Implements() const override;
   LibrarySet* MatchString(const LibrarySet* that) const override;
   LibrarySet* NeededBy() const override;
   LibrarySet* Needers() const override;
   LibrarySet* ReferencedBy() const override;
   LibrarySet* UsedBy(bool self) const override;
   LibrarySet* Users(bool self) const override;

   //  Returns the files in alphabetical order.
   //
   std::vector<CodeFile*> SortInAlphaOrder() const;

   //  Checks the code files in the set.
   //
   NodeBase::word Check(NodeBase::CliThread& cli,
      std::ostream* stream, std::string& expl) const override;

   //  Updates RESULT with the number of lines of code in the set.
   //
   NodeBase::word Countlines(std::string& result) const override;

   //  Fixes warnings detected by >check.
   //
   NodeBase::word Fix(NodeBase::CliThread& cli,
      FixOptions& opts, std::string& expl) const override;

   //  Formats the code files in the set.
   //
   NodeBase::word Format(std::string& expl) const override;

   //  Returns the type of set.
   //
   LibSetType GetType() const override { return FILE_SET; }

   //  Displays the count of line types in the set's files.
   //
   NodeBase::word LineTypes(NodeBase::CliThread& cli,
      std::ostream* stream, std::string& expl) const override;

   //  Parses the code files in the set.
   //
   NodeBase::word Parse
      (std::string& expl, const std::string& opts) const override;

   //  Displays, in STREAM, lines from the code files that match PATTERN.
   //
   NodeBase::word Scan(std::ostream& stream,
      const std::string& pattern, std::string& expl) const override;

   //  Displays the build order in STREAM.
   //
   NodeBase::word Sort(std::ostream& stream, std::string& expl) const override;

   //  Returns the build order of the set.
   //
   BuildOrder SortInBuildOrder() const override;

   //  Returns a string for each file in the set.
   //
   void to_str(stringVector& strings, bool verbose) const override;
private:
   //  Private to restrict deletion.  Not subclassed.
   //
   ~CodeFileSet();

   //  Overridden to create a set of files.
   //
   LibrarySet* Create
      (const std::string& name, const LibItemSet* items) const override;
};
}
#endif
