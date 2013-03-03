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

#include <assert.h>
#include <string.h>

#include "pm_procedures.h"
#include "pm_interpreter.h"
#include "pm_typemanager.h"

using namespace std;
using namespace prima;

D_UINT32
ProcedureManager::AddProcedure (const D_UINT8*      pName,
                                const D_UINT        nameLength,
                                const D_UINT32      localsCount,
                                const D_UINT32      argsCount,
                                const D_UINT32      syncCount,
                                vector<StackValue>& localValues,
                                const D_UINT32*     pTypesOffset,
                                const D_UINT8*      pCode,
                                const D_UINT32      codeSize,
                                Unit&               unit)
{
  assert (GetProcedure (pName, nameLength) == INVALID_ENTRY);
  assert (localsCount > 0);
  assert (argsCount < localsCount);

  ProcedureEntry entry;

  entry.m_LocalsCount = localsCount;
  entry.m_ArgsCount   = argsCount;
  entry.m_SyncCount   = syncCount;
  entry.m_SyncIndex   = m_SyncStmts.size ();
  entry.m_LocalsIndex = m_LocalsValues.size ();
  entry.m_IdIndex     = m_Identifiers.size ();
  entry.m_TypeOff     = m_LocalsTypes.size ();
  entry.m_CodeIndex   = m_Definitions.size ();
  entry.m_CodeSize    = codeSize;
  entry.m_pUnit       = &unit;

  const D_UINT32 result = m_ProcsEntrys.size ();

  m_SyncStmts.insert (m_SyncStmts.end(), syncCount, false);
  m_LocalsValues.insert (m_LocalsValues.end (),
                         &localValues[0],
                         &localValues[0] + localsCount);
  m_Identifiers.insert (m_Identifiers.end (), pName, pName + nameLength);
  m_Identifiers.push_back (0);
  m_Definitions.insert (m_Definitions.end (), pCode, pCode + codeSize);
  m_LocalsTypes.insert (m_LocalsTypes.end (),
                        pTypesOffset,
                        pTypesOffset + localsCount);
  m_ProcsEntrys.push_back (entry);

  return result;
}

D_UINT32
ProcedureManager::GetProcedure (const D_UINT8* pName,
                                const D_UINT   nameLength) const
{
  for (D_UINT32 index = 0; index < m_ProcsEntrys.size (); ++index)
    {
      if (strncmp (_RC (const D_CHAR*, pName),
                   _RC (const D_CHAR*,
                        &m_Identifiers[m_ProcsEntrys[index].m_IdIndex]),
                   nameLength) == 0)
        return index;
    }

  return INVALID_ENTRY;
}

const D_UINT8*
ProcedureManager::Name (const D_UINT procEntry) const
{
  if (procEntry >= m_ProcsEntrys.size ())
    throw InterException (NULL, _EXTRA (InterException::INVALID_PROC_REQ));

  return &m_Identifiers[m_ProcsEntrys[procEntry].m_IdIndex];
}

Unit&
ProcedureManager::GetUnit (const D_UINT procEntry) const
{
  const D_UINT32 procedure = procEntry & ~GLOBAL_ID;
  assert (procedure < m_ProcsEntrys.size ());

  if (procedure >= m_ProcsEntrys.size ())
    throw InterException (NULL, _EXTRA (InterException::INVALID_PROC_REQ));

  return *m_ProcsEntrys[procedure].m_pUnit;
}

D_UINT32
ProcedureManager::LocalsCount (const D_UINT procEntry) const
{
  const D_UINT32 procedure = procEntry & ~GLOBAL_ID;
  assert (procedure < m_ProcsEntrys.size ());

  if (procedure >= m_ProcsEntrys.size ())
    throw InterException (NULL, _EXTRA (InterException::INVALID_PROC_REQ));

  return m_ProcsEntrys[procedure].m_LocalsCount;
}

D_UINT32
ProcedureManager::ArgsCount (const D_UINT procEntry) const
{
  const D_UINT32 procedure = procEntry & ~GLOBAL_ID;

  if (procedure >= m_ProcsEntrys.size ())
    throw InterException (NULL, _EXTRA (InterException::INVALID_PROC_REQ));

  return m_ProcsEntrys[procedure].m_ArgsCount;
}

const StackValue&
ProcedureManager::LocalValue (const D_UINT   procEntry,
                              const D_UINT32 local) const
{
  const D_UINT32 procedure = procEntry & ~GLOBAL_ID;

  if (procedure >= m_ProcsEntrys.size ())
    throw InterException (NULL, _EXTRA (InterException::INVALID_PROC_REQ));

  const ProcedureEntry& entry = m_ProcsEntrys[procedure];

  if (local >= entry.m_LocalsCount)
    throw InterException (NULL, _EXTRA (InterException::INVALID_LOCAL_REQ));

  return m_LocalsValues[entry.m_LocalsIndex + local];
}

const D_UINT8*
ProcedureManager::LocalTI (const D_UINT  procEntry,
                          const D_UINT32 local) const
{
  const D_UINT32 procedure = procEntry & ~GLOBAL_ID;
  assert (procedure < m_ProcsEntrys.size ());

  if (procedure >= m_ProcsEntrys.size ())
    throw InterException (NULL, _EXTRA (InterException::INVALID_PROC_REQ));

  const ProcedureEntry& entry = m_ProcsEntrys[procedure];

  assert (local < entry.m_LocalsCount);

  if (local >= entry.m_LocalsCount)
    throw InterException (NULL, _EXTRA (InterException::INVALID_LOCAL_REQ));

  const TypeManager& typeMgr = m_NameSpace.GetTypeManager ();
  return typeMgr.GetType (m_LocalsTypes[entry.m_TypeOff + local]);
}

const D_UINT8*
ProcedureManager::Code (const D_UINT procEntry, D_UINT64* pOutCodeSize) const
{
  const D_UINT32 procedure = procEntry & ~GLOBAL_ID;
  assert (procedure < m_ProcsEntrys.size ());

  if (procedure >= m_ProcsEntrys.size ())
    throw InterException (NULL, _EXTRA (InterException::INVALID_PROC_REQ));

  const ProcedureEntry& entry = m_ProcsEntrys[procedure];

  if (pOutCodeSize != NULL)
    *pOutCodeSize = entry.m_CodeSize;

  return &m_Definitions[entry.m_CodeIndex];
}

void
ProcedureManager::AquireSync (const D_UINT procEntry, const D_UINT32 sync)
{
  const D_UINT32 procedure = procEntry & ~GLOBAL_ID;
  assert (procedure < m_ProcsEntrys.size ());

  if (procedure >= m_ProcsEntrys.size ())
    throw InterException (NULL, _EXTRA (InterException::INVALID_PROC_REQ));

  const ProcedureEntry& entry = m_ProcsEntrys[procedure];

  assert (sync < entry.m_SyncCount);

  if (sync >= entry.m_SyncCount)
    throw InterException (NULL, _EXTRA (InterException::INVALID_SYNC_REQ));

  do
    {
      WSynchronizerRAII holder (m_Sync);
      const bool aquired = m_SyncStmts[entry.m_SyncIndex + sync];
      if (aquired)
        {
          //Some one has taken this prior! Prepare to try again!
          holder.Leave ();
          wh_yield ();
        }
      else
        {
          m_SyncStmts[entry.m_SyncIndex + sync] = true;
          break;
        }
    }
  while (true);
}

void
ProcedureManager::ReleaseSync (const D_UINT procEntry, const D_UINT32 sync)
{
  const D_UINT32 procedure = procEntry & ~GLOBAL_ID;
  assert (procedure < m_ProcsEntrys.size ());

  if (procedure >= m_ProcsEntrys.size ())
    throw InterException (NULL, _EXTRA (InterException::INVALID_PROC_REQ));

  const ProcedureEntry& entry = m_ProcsEntrys[procedure];

  assert (sync < entry.m_SyncCount);

  if (sync >= entry.m_SyncCount)
    throw InterException (NULL, _EXTRA (InterException::INVALID_SYNC_REQ));

  assert (m_SyncStmts[entry.m_SyncIndex + sync] == true);
  m_SyncStmts[entry.m_SyncIndex + sync] = false;
}

