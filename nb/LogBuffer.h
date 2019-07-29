//==============================================================================
//
//  LogBuffer.h
//
//  Copyright (C) 2017  Greg Utas
//
//  This file is part of the Robust Services Core (RSC).
//
//  RSC is free software: you can redistribute it and/or modify it under the
//  terms of the GNU General Public License as published by the Free Software
//  Foundation, either version 3 of the License, or (at your option) any later
//  version.
//
//  RSC is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
//  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
//  details.
//
//  You should have received a copy of the GNU General Public License along
//  with RSC.  If not, see <http://www.gnu.org/licenses/>.
//
#ifndef LOGBUFFER_H_INCLUDED
#define LOGBUFFER_H_INCLUDED

#include "Permanent.h"
#include <cstddef>
#include <cstdint>
#include <string>
#include "SysMutex.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
class LogBuffer : public Permanent
{
//  Circular buffer for holding logs that are waiting to be written to a log
//  file.  A buffer is allocated during each restart and therefore provides a
//  record of the system's most recent incarnation.  If a buffer contains logs
//  that still have not been written to its log file, that buffer survives a
//  restart for debugging purposes and must be freed using a CLI command when
//  no longer required.
//
public:
   //  Creates a buffer of length KBS kilobytes.
   //
   explicit LogBuffer(size_t kbs);

   //  Releases the buffer.
   //
   ~LogBuffer();

   //  An entry in the buffer.
   //
   struct Entry
   {
      Entry* prev;           // previous entry in the buffer
      Entry* next;           // next entry in the buffer
      char log[UINT16_MAX];  // log's contents (null-terminated)
   };

   //  Adds LOG's contents to the buffer and releases LOG.  Returns
   //  the log's location in the buffer, or nullptr if the buffer
   //  is full.
   //
   Entry* Push(ostringstreamPtr& log);

   //  Provides access to the buffer's mutex so the buffer can
   //  be locked while using functions other than Insert (which
   //  handles locking itself).
   //
   SysMutex* GetLock() { return &lock_; }

   //  Returns the first (oldest) log in the buffer.  Returns
   //  nullptr if the buffer is empty.
   //
   const Entry* First() const;

   //  Updates CURR to the next log in the buffer.  Sets CURR
   //  to nullptr if there is no next log.
   //
   void Next(const Entry*& curr) const;

   //  Returns the last (newest) log in the buffer.  Returns
   //  nullptr if the buffer is empty.
   //
   const Entry* Last() const;

   //  Updates CURR to the previous log in the buffer.  Sets
   //  CURR to nullptr if there is no previous log.
   //
   static void Prev(const Entry*& curr);

   //  Erases the first log in the buffer.
   //
   void Pop();

   //  Returns the number of logs in the buffer.
   //
   size_t Size() const;

   //  Returns the number of discarded logs.
   //
   size_t Discards() const { return discards_; }

   //  Clears the number of discarded logs.
   //
   void ResetDiscards() { discards_ = 0; }

   //  Returns the file name for the logs.
   //
   const std::string& FileName() const { return fileName_; }

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
private:
   //  Sets NEXT as the next location for inserting a log.
   //
   void SetNext(Entry* next);

   //  Returns the location for inserting an entry of SIZE bytes.
   //
   Entry* InsertionPoint(size_t size);

   //  Updates the maximum space used in the buffer and returns WHERE.
   //
   Entry* UpdateMax(Entry* where);

   //  Critical section lock for the log buffer.
   //
   SysMutex lock_;

   //  File name for saving the logs.
   //
   std::string fileName_;

   //  The number of logs discarded because the buffer was full.
   //
   size_t discards_;

   //  The first log.
   //
   Entry* first_;

   //  The location for the next log (with prev_ to the last log).
   //
   Entry* next_;

   //  The maximum space used in the buffer.
   //
   size_t max_;

   //  The buffer's size.
   //
   size_t size_;

   //  The buffer.
   //
   char* buff_;
};
}
#endif
