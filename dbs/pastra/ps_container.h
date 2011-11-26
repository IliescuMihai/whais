/******************************************************************************
 PASTRA - A light database one file system and more.
 Copyright (C) 2008  Iulian Popa

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
 *****************************************************************************/

#ifndef PS_CONTAINER_H_
#define PS_CONTAINER_H_

#include <vector>
#include <string>


#include "dbs_values.h"

#include "utils/include/wfile.h"
#include "utils/include/wsync.h"

namespace pastra
{

class DataContainerException:public WException
{
public:
  DataContainerException (const D_CHAR * message,
                          const D_CHAR * file,
                          D_UINT32 line,
                          D_UINT32 extra):WException (message, file, line,
                                                          extra)
  {
  }
  virtual ~ DataContainerException ()
  {
  }
  ;
};

class I_DataContainer
{
public:
  I_DataContainer ()
  {}
  virtual ~ I_DataContainer ()
  {}

  virtual void StoreData (D_UINT64 uPosition, D_UINT64 uLenght, const D_UINT8 * puDataSource) = 0;
  virtual void RetrieveData (D_UINT64 uPosition, D_UINT64 uLenght, D_UINT8 * puDataDestination) const = 0;
  virtual void ColapseContent (D_UINT64 uStartPosition, D_UINT64 uEndPosition) = 0;
  virtual D_UINT64 GetContainerSize () const = 0;
  virtual void MarkForRemoval () = 0;
};


class FileContainer:public I_DataContainer
{
public:
  FileContainer (const D_CHAR * pFileNameBase,
                 const D_UINT32 uMaxFileSize,
                 const D_UINT32 uUnitsCount);

  virtual ~ FileContainer ();

  // WIDataContainer virtual functions
  virtual void StoreData (D_UINT64 uPosition,
                          D_UINT64 uLenght,
                          const D_UINT8 * puDataSource);
  virtual void RetrieveData (D_UINT64 uPosition,
                             D_UINT64 uLenght,
                             D_UINT8 * puDataDestination) const;
  virtual void ColapseContent (D_UINT64 uStartPosition,
                               D_UINT64 uEndPosition);
  virtual D_UINT64 GetContainerSize () const;
  virtual void MarkForRemoval ();

protected:
  void ExtendContainer ();

protected:
  const D_UINT32 m_uMaxFileUnitSize;
  std::vector < WFile > m_FilesHandles;
  std::string m_FileNameBase;
  bool mIsMarked;
};

class FileTempContainer:public FileContainer
{
public:
  FileTempContainer (const D_CHAR * pFileNameBase,
                     const D_UINT32 uMaxFileSize);
  virtual ~ FileTempContainer ();
};

class TempContainer:public I_DataContainer
{
public:
  TempContainer (const D_CHAR * pTempDirectory, D_UINT uReservedMemory);
  virtual ~ TempContainer ();

  // WIDataContainer virtual functions
  virtual void StoreData (D_UINT64 uPosition,
                          D_UINT64 uLenght,
                          const D_UINT8 * puDataSource);
  virtual void RetrieveData (D_UINT64 uPosition,
                             D_UINT64 uLenght,
                             D_UINT8 * puDataDestination) const;
  virtual void ColapseContent (D_UINT64 uStartPosition,
                               D_UINT64 uEndPosition);
  virtual D_UINT64 GetContainerSize () const;
  virtual void MarkForRemoval ();

protected:
  void SwitchToFileContent ();

  std::auto_ptr < FileTempContainer > mContainer;
  std::auto_ptr < D_UINT8 > mMemory;
  std::string mTempDirectory;
  D_UINT muMemorySize;
  D_UINT muValidMemory;

  static D_UINT64 mTemporalsCount;
  static WSynchronizer mSync;
};

class WFileContainerException:public DataContainerException
{
public:
  explicit
  WFileContainerException (const D_CHAR * message,
                           const D_CHAR * file,
                           D_UINT32 line,
                           D_UINT32
                           extra):DataContainerException (message, file,
                                                              line, extra)
  {
  }
  virtual ~ WFileContainerException ()
  {
  }

  enum
  {
    INVALID_PARAMETERS = 1,
    CONTAINTER_INVALID,
    INVALID_ACCESS_POSITION,
    FILE_OS_IO_ERROR
  };
};
}

#endif /* PS_CONTAINER_H_ */