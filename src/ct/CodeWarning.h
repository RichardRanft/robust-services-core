//==============================================================================
//
//  CodeWarning.h
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
#ifndef CODEWARNING_H_INCLUDED
#define CODEWARNING_H_INCLUDED

#include <cstddef>
#include <iosfwd>
#include <map>
#include <string>
#include <utility>
#include <vector>
#include "CodeTypes.h"
#include "CxxFwd.h"
#include "CxxLocation.h"
#include "LibraryItem.h"
#include "LibraryTypes.h"
#include "SysTypes.h"

using NodeBase::word;

//------------------------------------------------------------------------------

namespace CodeTools
{
//  Attributes of a Warning.
//
struct WarningAttrs
{
   //  Set if the editor can fix the warning.
   //
   const bool fixable_;

   //  Set to prevent the warning from being generated.
   //
   const bool disabled_;

   //  A string that explains the warning.
   //
   NodeBase::fixed_string expl_;

   //  Constructs a warning with the specified attributes.
   //
   WarningAttrs(bool fixable, bool disabled, NodeBase::c_string expl);
};

//------------------------------------------------------------------------------
//
//  Inserts a string for WARNING into STREAM.
//
std::ostream& operator<<(std::ostream& stream, Warning warning);

//  Whether a warning has been fixed.
//
enum WarningStatus
{
   NotSupported,  // editor does not support fixing this warning
   Disabled,      // warning is not to be reported
   Revoked,       // warning was cancelled because of subsequent findings
   Deleted,       // code associated with the warning was deleted
   NotFixed,      // code not yet changed
   Pending,       // code changed but not written to file
   Fixed          // code changed and written to file
};

//------------------------------------------------------------------------------
//
//  Used to log a warning.
//
class CodeWarning
{
   friend class Editor;
public:
   //  Almost every member is supplied to the constructor.  If POS is
   //  string::npos, it means that the warning has no code to display.
   //
   CodeWarning(Warning warning, CodeFile* file, size_t pos,
      const CxxToken* item, word offset, const std::string& info);

   //  Not subclassed.
   //
   ~CodeWarning() = default;

   //  Copy constructor.  Needed to allow sorting.
   //
   CodeWarning(const CodeWarning& that) = default;

   //  Copy operator.  Needed to allow sorting.
   //
   CodeWarning& operator=(const CodeWarning& that) = default;

   //  Comparison operators.
   //
   bool operator==(const CodeWarning& that) const;
   bool operator!=(const CodeWarning& that) const;

   //  Initializes the Attrs map.
   //
   static void Initialize();

   //  Unless the warning is suppressed, adds it to the file where it occurred.
   //
   void Insert() const;

   //  Displays, in STREAM, the code warnings that were found in FILES during
   //  parsing and compilation.
   //
   static void GenerateReport(std::ostream* stream, const LibItemSet& files);

   //  Returns the explanation for warning W.
   //
   static NodeBase::c_string Expl(Warning w) { return Attrs_.at(w).expl_; }

   //  Returns true if LOG2 > LOG1 when sorting by file/line/reverse pos.
   //
   static bool IsSortedToFix(const CodeWarning& log1, const CodeWarning& log2);

   //  Updates the position of a warning when a file has been edited.  Has
   //  the same interface as CxxToken::UpdatePos.
   //
   void UpdatePos(EditorAction action,
      size_t begin, size_t count, size_t from = std::string::npos) const;

   //  Nullifies the warning if it is associated with ITEM.
   //
   void ItemDeleted(const CxxToken* item) const;

   //  Returns true if LOG should be retained when >check is re-executed.
   //
   bool Preserve() const;
private:
   //  Returns the file in which the warning appeared.
   //
   CodeFile* File() const { return loc_.GetFile(); }

   //  Returns the position at which the warning appeared.
   //
   size_t Pos() const { return loc_.GetPos(); }

   //  Returns the line number on which the warning appeared.
   //
   size_t Line() const;

   //  Searches FILE for a log that matches WARNING, ITEM, and OFFSET.
   //
   static const CodeWarning* FindWarning(const CodeFile* file,
      Warning warning, const CxxToken* item, NodeBase::word offset);

   //  Returns true if the warning should be suppressed.  Invoked when
   //  the warning is detected.
   //
   bool Suppress() const;

   //  Returns true if the warning should be revoked.  Invoked just
   //  before the warning is included by GenerateReport.
   //
   bool Revoke() const;

   //  Returns true if the warning has code to display.
   //
   bool HasCodeToDisplay() const;

   //  Returns true if .info should be displayed.
   //
   bool HasInfoToDisplay() const;

   //  Returns true if the warning is informational and cannot be fixed.
   //
   bool IsInformational() const;

   //  Returns true if the warning was fixed or the code associated with
   //  it was deleted.
   //
   bool WasResolved() const;

   //  Returns the warnings that need to be fixed to resolve this one.
   //  The warning itself is included in the result unless it does not
   //  need to be fixed.
   //
   std::vector<const CodeWarning*> LogsToFix(std::string& expl) const;

   //  Returns the other warning associated with a warning that involves
   //  fixing both a declaration and a definition.
   //
   const CodeWarning* FindMate(std::string& expl) const;

   //  Returns the name of the function that this warning wants added to
   //  a class.  Returns an empty string if LOG does not suggest this.
   //
   std::string GetNewFuncName() const;

   //  Returns true if LOG2 > LOG1 when sorting by file/warning/line.
   //
   static bool IsSortedByFile(const CodeWarning* log1, const CodeWarning* log2);

   //  Returns true if LOG2 > LOG1 when sorting by warning/file/line.
   //
   static bool IsSortedByType(const CodeWarning* log1, const CodeWarning* log2);

   //  For inserting elements into the attributes map.
   //
   typedef std::pair<Warning, WarningAttrs> WarningPair;

   //  A type for mapping warnings to their attributes.
   //
   typedef std::map<Warning, WarningAttrs> AttrsMap;

   //  The type of warning.
   //
   Warning warning_;

   //  Where the warning occurred.
   //
   mutable CxxLocation loc_;

   //  The C++ item associated with the warning.  It is provided as a const
   //  item, but its constness is removed so that the Editor can update it
   //  when modifying the code.
   //
   CxxToken* item_;

   //  Warning specific.  If > 0, displayed after line number (used, for
   //  example, to specify that a function's Nth argument is associated
   //  with the warning).  If < 0, denotes an informational warning that
   //  cannot be fixed but that is associated with one that can.
   //
   word offset_;

   //  Warning specific.
   //
   std::string info_;

   //  Set for a log generated during compilation.  It is then preserved
   //  if >check is re-executed, because it cannot be regenerated without
   //  recompiling.
   //
   bool preserve_;

   //  Whether a warning can be, or has been, fixed by the Editor.
   //
   mutable WarningStatus status_;

   //  Maps a warning to its attributes.
   //
   static AttrsMap Attrs_;
};
}
#endif
