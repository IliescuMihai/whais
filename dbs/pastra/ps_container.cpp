/******************************************************************************
WHAIS - An advanced database system
Copyright(C) 2014-2018  Iulian Popa

Address: Str Olimp nr. 6
         Pantelimon Ilfov,
         Romania
Phone:   +40721939650
e-mail:  popaiulian@gmail.com

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/


#include <assert.h>
#include <string.h>
#include <memory>

#include "dbs/dbs_mgr.h"
#include "ps_container.h"


using namespace std;


namespace whais {
namespace pastra {


static inline void
safe_memcpy(uint8_t* to, uint8_t* from, uint64_t count)
{
  while (count-- > 0)
    *to++ = *from++;
}


void
append_int_to_str(uint64_t number, string& inoutStr)
{
  const uint_t bitsCount = sizeof(number) * 8;

  char buffer[bitsCount];
  char *conv = &buffer[bitsCount - 1];

  *conv = 0;
  do
  {
    conv--;
    *conv = _SC(char, ('0' + (number % 10)));
    number /= 10;
  } while (number != 0);

  inoutStr += conv;
}



DataContainerException::DataContainerException(const uint32_t      code,
                                               const char*         file,
                                               const uint32_t      line,
                                               const char* const   fmtMsg,
                                               ...)
  : Exception(code, file, line)
{
  if (fmtMsg != nullptr)
  {
    va_list vl;

    va_start(vl, fmtMsg);
    this->Message(fmtMsg, vl);
    va_end(vl);
  }
}



WFileContainerException::WFileContainerException(const uint32_t   code,
                                                 const char      *file,
                                                 uint32_t         line,
                                                 const char      *fmtMsg,
                                                 ...)
  : DataContainerException(code, file, line, nullptr)
{
  if (fmtMsg != nullptr)
  {
    va_list vl;

    va_start(vl, fmtMsg);
    this->Message(fmtMsg, vl);
    va_end(vl);
  }
}


Exception*
WFileContainerException::Clone() const
{
  return new WFileContainerException(*this);
}


EXCEPTION_TYPE
WFileContainerException::Type() const
{
  return FILE_CONTAINER_EXCEPTION;
}


const char*
WFileContainerException::Description() const
{
  switch(Code())
  {
  case INVALID_PARAMETERS:
    return "Container operation failed due to invalid parameters.";

  case CONTAINTER_INVALID:
    return "File container inconsistency detected.";

  case INVALID_ACCESS_POSITION:
    return "File container accessed outside bounds.";

  case FILE_OS_IO_ERROR:
    return "File container request failed due to internal file IO error.";
  }

  assert(false);
  return "Unknown file container exception.";
}



FileContainer::FileContainer(const char*       baseName,
                             const uint64_t    maxFileSize,
                             const uint64_t    unitsCount,
                             const bool        truncate)
  : mMaxFileUnitSize(maxFileSize),
    mFilesHandles(),
    mFileNamePrefix(baseName),
    mToRemove(false),
    mIgnoreExistingData(truncate)
{
  uint_t openMode;

  if (unitsCount > 0)
  {
    assert(truncate == false);

    openMode = WH_FILEOPEN_EXISTING;
  }
  else if (mIgnoreExistingData)
    openMode = WH_FILECREATE | WH_FILETRUNC;

  else
    openMode = WH_FILECREATE_NEW;

  openMode |= WH_FILERDWR;
  for (uint_t unit = 0; unit < unitsCount; ++unit)
  {
    const string baseName = mFileNamePrefix + (unit ? to_string(unit) : "");

    mFilesHandles.push_back(File(baseName.c_str(), openMode));
  }

  assert(mFilesHandles.size() == unitsCount);

  // Check for structural consistency
  for (uint_t unit = 0; unit < unitsCount; ++unit)
  {
    File& file = mFilesHandles[unit];

    if (file.Size() != maxFileSize
        && (unit != (unitsCount - 1) || file.Size() > maxFileSize))
    {
      throw WFileContainerException(_EXTRA(WFileContainerException::CONTAINTER_INVALID),
                                    "Inconsistent container (base name: '%s', unit %d(of %d),"
                                      " fs: %d(max %d)!",
                                    mFileNamePrefix.c_str(),
                                    unit,
                                    unitsCount,
                                    file.Size(),
                                    maxFileSize);
    }
  }
}

FileContainer::~FileContainer()
{
  if (mToRemove)
    Colapse(0, Size() );
}

void
FileContainer::Write(uint64_t to, uint64_t size, const uint8_t* buffer)
{
  const uint_t unitsCount = mFilesHandles.size();
  uint64_t unitIndex = to / mMaxFileUnitSize;
  uint64_t unitPosition = to % mMaxFileUnitSize;

  if (unitIndex > unitsCount)
  {
    throw WFileContainerException(_EXTRA(WFileContainerException::INVALID_ACCESS_POSITION),
                                  "Could not access file container offset %d, "
                                  "unit %d(%d * %lu)!",
                                  to,
                                  unitIndex,
                                  unitsCount,
                                  _SC(long, mMaxFileUnitSize));
  }
  else if (unitIndex == unitsCount)
  {
    if (unitPosition != 0)
    {
      throw WFileContainerException(_EXTRA(WFileContainerException::INVALID_ACCESS_POSITION),
                                    "Could not access file container offset %d, "
                                      "unit %d(of %d * %lu).",
                                     to,
                                     unitIndex,
                                     unitsCount,
                                     _SC(long, mMaxFileUnitSize));
    }
    else
      ExtendContainer();
  }

  uint64_t actualSize = size;

  if ((actualSize + unitPosition) > mMaxFileUnitSize)
    actualSize = mMaxFileUnitSize - unitPosition;

  assert(actualSize <= size);

  File& file = mFilesHandles[unitIndex];

  if (file.Size() < unitPosition)
  {
    throw WFileContainerException(_EXTRA(WFileContainerException::INVALID_ACCESS_POSITION),
                                  "Unit position %lu(%lu).",
                                  _SC(long, unitPosition),
                                  _SC(long, file.Size()));
  }

  file.Seek(unitPosition, WH_SEEK_BEGIN);
  file.Write(buffer, actualSize);

  //Write the rest
  if (actualSize < size)
    Write(to + actualSize, size - actualSize, buffer + actualSize);
}


void
FileContainer::Read(uint64_t from, uint64_t size, uint8_t* buffer)
{
  if (size == 0)
    return ;

  const uint_t unitsCount = mFilesHandles.size();
  uint64_t unitIndex = from / mMaxFileUnitSize;
  uint64_t unitPosition = from % mMaxFileUnitSize;

  if ((unitIndex > unitsCount) || (from + size > Size()))
  {
    throw WFileContainerException(_EXTRA(WFileContainerException::INVALID_ACCESS_POSITION),
                                  "Failed to read %lu bytes from %lu( of %lu), "
                                    "unit %d( of %d).",
                                    _SC(long, size),
                                    _SC(long, from),
                                    _SC(long, Size()),
                                    unitIndex,
                                    unitsCount);
  }

  File& file = mFilesHandles[unitIndex];

  uint64_t actualSize = size;

  if (actualSize + unitPosition > file.Size())
    actualSize = file.Size() - unitPosition;

  file.Seek(unitPosition, WH_SEEK_BEGIN);
  file.Read(buffer, actualSize);

  //Read the rest
  if (actualSize < size)
    Read(from + actualSize, size - actualSize, buffer + actualSize);

}


void
FileContainer::Colapse(uint64_t from, uint64_t to)
{
  const uint64_t intervalSize = to - from;
  const uint64_t containerSize = Size();

  if ((to < from) || (containerSize < to))
  {
    throw WFileContainerException(_EXTRA(WFileContainerException::INVALID_PARAMETERS),
                                  "Failed to collapse from %lu to %lu(of %lu).",
                                  _SC(long, from),
                                  _SC(long, to),
                                  _SC(long, containerSize));
  }
  else if (intervalSize == 0)
    return;

  while (to < containerSize)
  {
    uint8_t buffer[1024];
    uint64_t stepSize = sizeof buffer;

    if (stepSize + to > containerSize)
      stepSize = containerSize - to;

    Read(to, stepSize, buffer);
    Write(from, stepSize, buffer);

    to += stepSize, from += stepSize;
  }

  //Delete the remaining content.
  const uint64_t newSize = containerSize - intervalSize;
  int lastUnit = newSize / mMaxFileUnitSize;
  const int lastUnitSize = newSize % mMaxFileUnitSize;

  if (newSize == 0)
    --lastUnit;

  else
    mFilesHandles[lastUnit].Size(lastUnitSize);

  for (int unit = mFilesHandles.size() - 1; unit > lastUnit; --unit)
  {
    mFilesHandles[unit].Close();

    const string baseName = mFileNamePrefix + (unit ? to_string(unit) : "");
    if ( ! whf_remove(baseName.c_str()))
    {
      throw WFileContainerException(_EXTRA(WFileContainerException::FILE_OS_IO_ERROR),
                                    "Failed to remove file '%s'.",
                                    baseName.c_str());
    }
    mFilesHandles.pop_back();
  }
}

uint64_t
FileContainer::Size() const
{
  if (mFilesHandles.size() == 0)
    return 0;

  const File& lastUnitFile = mFilesHandles[mFilesHandles.size() - 1];

  return  (mFilesHandles.size() - 1) * mMaxFileUnitSize + lastUnitFile.Size();
}


void
FileContainer::MarkForRemoval()
{
  mToRemove = true;
}


void
FileContainer::Flush()
{
  for (auto& f : mFilesHandles)
    f.Sync();
}


void
FileContainer::Fix(const char* const   baseFile,
                   const uint64_t      maxFileSize,
                   const uint64_t      newContainerSize)
{
  uint64_t size = 0;
  uint_t fileCount = 0;

  while (true)
  {
    const string fileName = string(baseFile) + (fileCount ? to_string (fileCount) : "");

    fileCount++;

    if (size >= newContainerSize)
    {
      //Remove all trailing container files until you found a missing one
      if ( ! whf_remove(fileName.c_str()))
        break;
    }
    else
    {
      File unitFile(fileName.c_str(), WH_FILECREATE | WH_FILEWRITE);
      if (newContainerSize - size >= maxFileSize)
      {
        unitFile.Size(maxFileSize);
        size += maxFileSize;
      }
      else
      {
        unitFile.Size(newContainerSize - size);
        size = newContainerSize;
      }
    }
  }
}

void
FileContainer::ExtendContainer()
{
  const uint_t count = mFilesHandles.size();
  const string baseName = mFileNamePrefix + (count ? to_string(count) : "");

  const uint_t openMode = mIgnoreExistingData
                            ? WH_FILECREATE | WH_FILETRUNC | WH_FILERDWR
                            : WH_FILECREATE_NEW | WH_FILERDWR;
  mFilesHandles.push_back(File(baseName.c_str(), openMode));
}



TemporalFileContainer::TemporalFileContainer(const char* baseName, const uint32_t maxFileSize)
  : FileContainer(baseName, maxFileSize, 0, true)
{
  MarkForRemoval();
}



TemporalContainer::TemporalContainer(const uint_t reservedMemory)
  : mCache_1(unique_array_make(uint8_t, reservedMemory / 2)),
    mCacheStartPos_1(0),
    mCacheEndPos_1(0),
    mCacheStartPos_2(0),
    mCacheEndPos_2(0),
    mCacheSize(reservedMemory / 2),
    mDirtyCache_1(false),
    mDirtyCache_2(false),
    mCache1LastUsed(true)
{
}


void
TemporalContainer::Write(uint64_t to, uint64_t size, const uint8_t* buffer)
{
  if (to > Size())
  {
    throw WFileContainerException(_EXTRA(WFileContainerException::INVALID_ACCESS_POSITION),
                                  "Failed to write %lu bytes at %lu(of %lu).",
                                  _SC(long, size),
                                  _SC(long, to),
                                  _SC(long, Size()));
  }

  while (size > 0)
  {
    assert((mCacheStartPos_1 % mCacheSize) == 0);
    assert((mCacheStartPos_2 % mCacheSize) == 0);

    if (mCacheStartPos_1 <= to && to < (mCacheStartPos_1 + mCacheSize))
    {
      const uint_t toWrite = MIN(size, mCacheStartPos_1 + mCacheSize - to);

      memcpy(mCache_1.get() + (to - mCacheStartPos_1), buffer, toWrite);

      if (to + toWrite > mCacheEndPos_1)
        mCacheEndPos_1 = to + toWrite;

      to += toWrite, buffer += toWrite, size -= toWrite;
      mDirtyCache_1 = true;
    }
    else if (mCache_2.get() != nullptr
            && (mCacheStartPos_2 <= to && to < (mCacheStartPos_2 + mCacheSize)))
    {
      assert(mCacheStartPos_1 != mCacheStartPos_2);
      assert((mCacheEndPos_1 != mCacheEndPos_2) || (mCacheEndPos_2 == mCacheStartPos_2));

      const uint_t toWrite = MIN(size, mCacheStartPos_2 + mCacheSize - to);

      memcpy(mCache_2.get() + (to - mCacheStartPos_2), buffer, toWrite);

      if (to + toWrite > mCacheEndPos_2)
        mCacheEndPos_2 = to + toWrite;

      to += toWrite, buffer += toWrite, size -= toWrite;
      mDirtyCache_2 = true;
    }
    else
      FillCache(to);
  }
}

void
TemporalContainer::Read(uint64_t from, uint64_t size, uint8_t* buffer)
{
  if (from + size > Size())
  {
    throw WFileContainerException(_EXTRA(WFileContainerException::INVALID_ACCESS_POSITION),
                                  "Failed to read %lu bytes from %lu(of %lu).",
                                  _SC(long, size),
                                  _SC(long, from),
                                  _SC(long, Size()));
  }

  while (size > 0)
  {
    assert((mCacheStartPos_1 % mCacheSize) == 0);
    assert((mCacheStartPos_2 % mCacheSize) == 0);

    if (mCacheStartPos_1 <= from && from < mCacheEndPos_1)
    {
      const uint_t toRead = MIN(size, mCacheEndPos_1 - from);

      memcpy(buffer, mCache_1.get() + (from - mCacheStartPos_1), toRead);

      from += toRead, buffer += toRead, size -= toRead;
    }
    else if (mCache_2.get() != nullptr
             && (mCacheStartPos_2 <= from && from < mCacheEndPos_2))
    {
      const uint_t toRead = MIN(size, mCacheEndPos_2 - from);

      memcpy(buffer, mCache_2.get() + (from - mCacheStartPos_2), toRead);

      from += toRead, buffer += toRead, size -= toRead;
    }

    else
      FillCache(from);
  }
}


void
TemporalContainer::Colapse(uint64_t from, uint64_t to)
{
  const uint64_t containerSize = Size();

  if ((to < from) || (containerSize < to))
    {
      throw WFileContainerException(_EXTRA(WFileContainerException::INVALID_PARAMETERS),
                                    "Failed to collapse temporal container from %lu to %lu(%lu).",
                                     _SC(long, from),
                                     _SC(long, to),
                                     _SC(long, containerSize));
    }

  if (mFileContainer.get() == nullptr
      && mCache_2.get() != nullptr)
  {
    assert(mCacheStartPos_1 == 0);
    assert(mCacheEndPos_1 == mCacheSize);
    assert(mCacheStartPos_2 == mCacheSize);
    assert(mCacheEndPos_2 <= 2 * mCacheSize);
    assert(containerSize == mCacheEndPos_2);

    uint8_t stepBuffer[128];
    uint64_t tempFrom = from, tempTo = to;

    while (tempTo < containerSize)
    {
      uint_t stepSize = sizeof(stepBuffer);

      if (tempTo + stepSize > containerSize)
        stepSize = containerSize - tempTo;

      Read(tempTo, stepSize, stepBuffer);
      Write(tempFrom, stepSize, stepBuffer);

      tempTo += stepSize, tempFrom += stepSize;
    }

    assert(to <= mCacheEndPos_2);

    mCacheEndPos_2 -= to - from;
    if (mCacheEndPos_2 <= mCacheSize)
    {
      mCacheEndPos_1 = mCacheEndPos_2;

      mCacheStartPos_2 = mCacheEndPos_2 = 0;
      mCache_2.reset(nullptr);
      mCache1LastUsed = true;
    }
  }
  else if (mFileContainer.get() != nullptr)
  {
    assert(mCache_2.get() != nullptr);

    if (mDirtyCache_1)
    {
      mFileContainer->Write(mCacheStartPos_1, mCacheEndPos_1 - mCacheStartPos_1, mCache_1.get());
      mDirtyCache_1 = false;
    }

    if (mDirtyCache_2)
    {
      mFileContainer->Write(mCacheStartPos_2, mCacheEndPos_2 - mCacheStartPos_2, mCache_2.get());
      mDirtyCache_2 = false;
    }

    mFileContainer->Colapse(from, to);
  }
  else
  {
    assert(containerSize == mCacheEndPos_1);
    assert((containerSize == 0) || mDirtyCache_1);

    uint8_t* const cache_     = mCache_1.get();
    const uint_t   remainSize = mCacheEndPos_1 - to;

    safe_memcpy(cache_ + from, cache_ + to, remainSize);
    mCacheEndPos_1 -= (to - from);
  }

  if (mFileContainer.get() != nullptr
      && mFileContainer->Size() <= (2 * mCacheSize))
  {
    if (mFileContainer->Size() > mCacheSize)
    {
      assert(mCache_2.get() != nullptr);

      mCacheStartPos_2 = mCacheSize;
      mCacheEndPos_2 = mFileContainer->Size();

      mFileContainer->Read(mCacheStartPos_2, mCacheEndPos_2 - mCacheStartPos_2, mCache_2.get());
      mDirtyCache_2 = false;
    }
    else if (mCache_2.get() != nullptr)
    {
      mCacheStartPos_2 = mCacheEndPos_2 = 0;
      mDirtyCache_2 = false;
      mCache1LastUsed = true;

      mCache_2.reset(nullptr);
    }

    mCacheStartPos_1 = 0;
    mCacheEndPos_1 = MIN(mFileContainer->Size(), mCacheSize);

    mFileContainer->Read(mCacheStartPos_1, mCacheEndPos_1 - mCacheStartPos_1, mCache_1.get());
    mDirtyCache_1 = false;

    mFileContainer.reset(nullptr);
  }
  else if (mFileContainer.get() != nullptr)
  {
    /* Refill both cache buffers. */
    assert(mCache_2.get() != nullptr);

    assert(mDirtyCache_1 == false);
    assert(mDirtyCache_2 == false);

    mCacheStartPos_1 = 0;
    mCacheEndPos_1 = mCacheSize;

    mFileContainer->Read(mCacheStartPos_1, mCacheEndPos_1 - mCacheStartPos_1, mCache_1.get());

    from -= from % mCacheSize;
    if (from == 0)
      from = mCacheSize;

    mCacheStartPos_2 = from;
    mCacheEndPos_2 = MIN(mFileContainer->Size(), from + mCacheSize);

    mFileContainer->Read(mCacheStartPos_2, mCacheEndPos_2 - mCacheStartPos_2, mCache_2.get());

    mCache1LastUsed = false;
  }
}

void
TemporalContainer::MarkForRemoval()
{
  return ; //This will be deleted automatically. Nothing to do here!
}

void
TemporalContainer::Flush()
{
  if (mFileContainer.get() != nullptr)
    mFileContainer->Flush();
}

uint64_t
TemporalContainer::Size() const
{
  assert((mCacheStartPos_1 % mCacheSize) == 0);
  assert((mCacheStartPos_2 % mCacheSize) == 0);

  assert(mCacheStartPos_1 <= mCacheEndPos_1);
  assert(mCacheStartPos_2 <= mCacheEndPos_2);

  if (mFileContainer.get() != nullptr)
  {
    assert(mCache_2.get() != nullptr);
    assert(mCacheStartPos_1 != mCacheStartPos_2);
    assert((mCacheEndPos_1 != mCacheEndPos_2)
            || (mCacheEndPos_2 == mCacheStartPos_2));

    const uint64_t temp = max(mCacheEndPos_1, mCacheEndPos_2);
    return max(temp, mFileContainer->Size());
  }
  else if (mCache_2.get() != nullptr)
  {
    assert(mCacheStartPos_1  == 0);
    assert(mCacheEndPos_1    == mCacheSize);
    assert(mCacheStartPos_2  == mCacheSize);
    assert(mCacheEndPos_1    <= 2 * mCacheSize);
  }
  else
  {
    assert(mCacheStartPos_1 == 0);
    assert(mCacheEndPos_1 <= mCacheSize);
  }

  return max(mCacheEndPos_1, mCacheEndPos_2);
}


void
TemporalContainer::FillCache(uint64_t position)
{
  position -= (position % mCacheSize);

  assert((mCacheStartPos_1 % mCacheSize) == 0);
  assert((mCacheStartPos_2 % mCacheSize) == 0);

  if (mCacheStartPos_1 == position)
    return;

  else if (mCache_2.get() != nullptr && mCacheStartPos_2 == position)
    return;

  else if (mCache_2.get() == nullptr && position == mCacheSize)
  {
    assert(mFileContainer.get() == nullptr);

    mCache_2.reset(new uint8_t[mCacheSize]);
    mCacheStartPos_2 = mCacheEndPos_2 = mCacheSize;
    mDirtyCache_2 = false;

    mCache1LastUsed = false;

    return;
  }

  if (mFileContainer.get() == nullptr)
  {
    const uint64_t currentId = wh_atomic_fetch_inc64(_RC(int64_t*, &smTemporalsCount));

    assert(mCacheStartPos_1 == 0);
    assert(mCacheEndPos_1 == mCacheSize);
    assert(mCacheStartPos_2 == mCacheSize);
    assert(mCacheEndPos_2 == 2 * mCacheSize);
    assert(position == 2 * mCacheSize);

    const DBSSettings& settings = DBSGetSeettings();

    const string baseName = settings.mTempDir + "wtemp" + to_string(currentId) + ".tmp";

    mFileContainer.reset(new TemporalFileContainer(baseName.c_str(), settings.mMaxFileSize));

    mFileContainer->Write(0, mCacheSize, mCache_1.get());
    mFileContainer->Write(mCacheSize, mCacheSize, mCache_2.get());

    mDirtyCache_1 = mDirtyCache_2 = false;

    mCacheStartPos_1 = mCacheEndPos_1 = position;

    mCache1LastUsed = true;
    return;
  }
  else if (mCache1LastUsed)
  {
    if (mDirtyCache_2)
    {
      mFileContainer->Write(mCacheStartPos_2, mCacheEndPos_2 - mCacheStartPos_2, mCache_2.get());
      mDirtyCache_2 = false;
    }

    if (position >= mFileContainer->Size())
    {
      if (mDirtyCache_1)
      {
        mFileContainer->Write(mCacheStartPos_1, mCacheEndPos_1 - mCacheStartPos_1, mCache_1.get());
        mDirtyCache_1 = false;
      }
    }

    const uint_t toRead = MIN(mCacheSize, mFileContainer->Size() - position);

    mFileContainer->Read(position, toRead, mCache_2.get());

    mCacheStartPos_2 = position;
    mCacheEndPos_2 = mCacheStartPos_2 + toRead;
    mCache1LastUsed = false;
  }
  else
  {
    if (mDirtyCache_1)
    {
      mFileContainer->Write(mCacheStartPos_1, mCacheEndPos_1 - mCacheStartPos_1, mCache_1.get());
      mDirtyCache_1 = false;
    }

    if (position >= mFileContainer->Size())
    {
      if (mDirtyCache_2)
      {
        mFileContainer->Write(mCacheStartPos_2, mCacheEndPos_2 - mCacheStartPos_2, mCache_2.get());
        mDirtyCache_2 = false;
      }
    }

    const uint_t toRead = MIN(mCacheSize, mFileContainer->Size() - position);

    mFileContainer->Read(position, toRead, mCache_1.get());

    mCacheStartPos_1 = position;
    mCacheEndPos_1 = mCacheStartPos_1 + toRead;
    mCache1LastUsed = true;
  }
}


uint64_t  TemporalContainer::smTemporalsCount = 1;


} //namespace pastra
} //namespace whais
