/******************************************************************************
  WCMD - An utility to manage whisper database files.
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
******************************************************************************/

#include <assert.h>
#include <stdlib.h>
#include <string>

#include "wcmd_optglbs.h"

using namespace std;

const D_UINT64       MINIMUM_FILE_SIZE = 0x100000; //1 MB

static string        sWorkingDirectory (whc_get_current_directory ());
static string        sDBName;
static VERBOSE_LEVEL sVerbLevel   = VL_ERROR;
static D_UINT64      sMaxFileSize = 0x80000000; //default: 2GB
static I_DBSHandler* s_pDBSHnd    = NULL;

static string        sRemoteHost;
static string        sConnectPort;
static string        sPassword;

static D_INT         sUserId  = -1;

static const D_CHAR  DEFAULT_PORT[] = "1761";

static const D_UINT  DEFUALT_USER   = 1;

const string&
GetRemoteHostName ()
{
  return sRemoteHost;
}

void
SetRemoteHostName (const D_CHAR* pHostName)
{
  sRemoteHost = pHostName;
}

const string&
GetConnectionPort ()
{
  if (IsDatabaseRemote () && (sConnectPort.size () == 0))
    sConnectPort = DEFAULT_PORT;

  return sConnectPort;
}

void
SetConnectionPort (const D_CHAR* pPort)
{
  sConnectPort = pPort;
}

D_UINT
GetUserId ()
{
  if ((sUserId < 0 ) && IsDatabaseRemote ())
    sUserId = DEFUALT_USER;

  return sUserId;
}

void
SetUserId (const D_UINT userId)
{
  sUserId = (userId > 0) ? 1 : 0;
}

const string&
GetUserPassword ()
{
  return sPassword;
}

void
SetUserPassword (const D_CHAR* pPassword)
{
  sPassword = pPassword;
}

const string&
GetWorkingDirectory ()
{
  return sWorkingDirectory;
}

void
SetWorkingDirectory (const D_CHAR* pDirectory)
{
  sWorkingDirectory = pDirectory;
}

const string&
GetWorkingDB ()
{
  return sDBName;
}

void
SetWorkingDB (const D_CHAR* pDBName)
{
  sDBName = pDBName;
}

VERBOSE_LEVEL
GetVerbosityLevel ()
{
  return sVerbLevel;
}

void
SetVerbosityLevel (const D_UINT level)
{
  sVerbLevel = _SC (VERBOSE_LEVEL, MIN (level, VL_MAX));
}

bool
SetMaximumFileSize (std::string size)
{
  static const string digits = "0123456789";

  if (size.length () == 0)
    return false;

  D_UINT64 multiplier = 1;
  size_t   lastPos    = size.find_first_not_of (digits);

  if (lastPos != size.npos)
    {
      switch (size[lastPos])
        {
        case 'k':
        case 'K':
          multiplier     = 1024;
          size [lastPos] = 0;
          break;
        case 'm':
        case 'M':
          multiplier     = 1024 * 1024;
          size [lastPos] = 0;
          break;
        case 'g':
        case 'G':
          multiplier     = 1024 * 1024 * 1024;
          size [lastPos] = 0;
          break;
        default:
          return false;
        }
    }

  sMaxFileSize = atoi (size.c_str ()) * multiplier;

  if (sMaxFileSize == 0)
    return false;

  sMaxFileSize = MAX (sMaxFileSize, MINIMUM_FILE_SIZE);

  return true;
}

D_UINT64
GetMaximumFileSize ()
{
  return sMaxFileSize;
}

void
SetDbsHandler (I_DBSHandler& dbsHandler)
{
  s_pDBSHnd = &dbsHandler;
}

I_DBSHandler&
GetDBSHandler ()
{
  assert (s_pDBSHnd != NULL);
  return *s_pDBSHnd;
}

bool
IsDatabaseRemote ()
{
  if (GetRemoteHostName ().size () > 0)
    return true;

  return false;
}