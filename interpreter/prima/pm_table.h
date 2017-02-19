/******************************************************************************
 PASTRA - A light database one file system and more.
 Copyright(C) 2008  Iulian Popa

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

#ifndef PM_TABLE_H_
#define PM_TABLE_H_

#include <assert.h>

#include "dbs/dbs_mgr.h"
#include "dbs/dbs_table.h"
#include "pm_general_table.h"



namespace whais {
namespace prima {



class TableReference
{
public:
  TableReference(IDBSHandler& dbs, ITable& table)
    : mDbsHnd(dbs),
      mTable(table),
      mRefCount(0)
  {
  }

  TableReference* const Spawn()
  {
    return new TableReference(mDbsHnd, mTable.Spawn());
  }

  void IncrementRefCount()
  {
    LockRAII<Lock> _lock(mLock);
    ++mRefCount;
  }

  void DecrementRefCount()
  {
    LockRAII<Lock> _lock(mLock);

    assert(mRefCount > 0);

    if (--mRefCount == 0)
      {
        _lock.Release();
        delete this;
      }
  }

  ITable& GetTable()
  {
    assert(mRefCount > 0);

    return mTable;
  }

  IDBSHandler& GetDBSHandler()
  {
    assert(mRefCount > 0);

    return mDbsHnd;
  }

private:
  ~TableReference()
  {
    if (&mTable != &GeneralTable::Instance())
      mDbsHnd.ReleaseTable(mTable);
  }

  IDBSHandler&     mDbsHnd;
  ITable&          mTable;
  uint64_t         mRefCount;
  Lock             mLock;
};

} //namespace whais
} //namespace prima

#endif // PM_TABLE_H_

