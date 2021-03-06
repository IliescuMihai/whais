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
#include <string>
#include <iostream>
#include <vector>
#include <cstdio>

#include "whais.h"

#include "utils/endianness.h"
#include "client/whais_connector.h"

#include "wcmd_onlinecmds.h"
#include "wcmd_cmdsmgr.h"
#include "wcmd_optglbs.h"
#include "wcmd_execcmd.h"

using namespace std;


const char*
wcmd_translate_status(const uint32_t cs)
{
  if (cs > WCS_OS_ERR_BASE)
    {
      /* This is safe, coz this function is not supposed to be executed
       * in a multithread environment. a*/
      static char statusStr [64];
      sprintf(statusStr, "OS internal error: %u.", cs - WCS_OS_ERR_BASE);

      return statusStr;
    }

  switch(cs)
  {
  case WCS_OK:
    return "No error returned.";

  case WCS_INVALID_ARGS:
    return "Invalid arguments.";

  case WCS_OP_NOTSUPP:
    return "Operation not supported";

  case WCS_OP_NOTPERMITED:
    return "Operation not permitted.";

  case WCS_DROPPED:
    return "Connection dropped by peer.";

  case WCS_PROTOCOL_NOTSUPP:
    return "No suitable protocol to communicate with the server.";

  case WCS_ENCTYPE_NOTSUPP:
    return "Could not agree on a supported encryption type.";

  case WCS_UNEXPECTED_FRAME:
    return "Unexpected communication frame received.";

  case WCS_INVALID_FRAME:
    return "A communication frame with invalid content received.";

  case WCS_COMM_OUT_OF_SYNC:
    return "Communication with peer is out of sync.";

  case WCS_LARGE_ARGS:
    return "Size of the request arguments is big.";

  case WCS_LARGE_RESPONSE:
    return "Size of the request's response is too big.";

  case WCS_CONNECTION_TIMEOUT:
    return "Peer is taking to long time to respond.";

  case WCS_SERVER_BUSY:
    return "The server rejected our connection request because is too busy.";

  case WCS_INCOMPLETE_CMD:
    return "The requested command could not be handled."
           " The previous one should be completed first.";

  case WCS_INVALID_ARRAY_OFF:
    return "An invalid array index was used.";

  case WCS_INVALID_TEXT_OFF:
    return "An invalid text index was used.";

  case WCS_INVALID_ROW:
    return "An invalid row index was used.";

  case WCS_INVALID_FIELD:
    return "An invalid table field was used.";

  case WCS_TYPE_MISMATCH:
    return "The request command cannot be completed"
           " due to an unexpected type of a value.";

  case WCS_PROC_NOTFOUND:
    return "Procedure not found.";

  case WCS_PROC_RUNTIME_ERR:
    return "Runtime error during procedure call.";

  case WCS_GENERAL_ERR:
    return "Unexpected internal error.";
  }

  assert(false);

  return "Unknown error encountered!";
}



static const char globalShowDesc[]    = "List context database's "
                                          "global variables.";
static const char globalShowDescExt[] =
  "Show the global variables installed in the database context.\n"
  "If a name is provided it limits the listing to only those variables.\n"
  "Usage:\n"
  "  global [variable_name] ... ";

static bool
cmdGlobalList(const string& cmdLine, ENTRY_CMD_CONTEXT context)
{
  size_t              linePos   = 0;
  string              token     = CmdLineNextToken(cmdLine, linePos);
  string              globals;
  WH_CONNECTION       conHdl    = nullptr;
  uint_t              glbsCount = 0;
  uint_t              glbsSel   = 0;
  const VERBOSE_LEVEL level     = GetVerbosityLevel();
  vector<string>      subStrs;

  assert(token == "global");

  uint32_t cs = WConnect(GetRemoteHostName().c_str(),
                          GetConnectionPort().c_str(),
                          GetWorkingDB().c_str(),
                          GetUserPassword().c_str(),
                          GetUserId(),
                          DEFAULT_FRAME_SIZE,
                          &conHdl);
  if (cs != WCS_OK)
    {
      if (level >= VL_DEBUG)
        cout << "Failed to connect: " << wcmd_translate_status(cs) << endl;

      cout << wcmd_translate_status(cs) << endl;
      return false;
    }

  while (linePos <= cmdLine.length())
    {
      token = CmdLineNextToken(cmdLine, linePos);
      if (token.length() == 0)
        break;

      subStrs.push_back(token);
    }

  cs = WStartGlobalsList(conHdl, &glbsCount);
  if (level >= VL_DEBUG)
    {
      if (cs == WCS_OK)
        cout << "Got " << glbsCount << " globals.\n";

      else
        cout << "Listing globals variables has failed\n";
    }

  uint_t glbIt = glbsCount;
  while ((cs == WCS_OK)
          && (glbIt-- > 0))
    {
      const char* glbName = nullptr;
      cs = WFetchGlobal(conHdl, &glbName);
      if ((cs != WCS_OK) && (level < VL_DEBUG))
        cout << "Fetching global value name has failed.\n";

      assert(glbName != nullptr);

      if (subStrs.size() > 0)
        {
          for (size_t i = 0; i < subStrs.size(); ++i)
            {
              if (strstr(glbName, subStrs[i].c_str()) != nullptr)
                {
                  globals += ' ';
                  globals += glbName;
                  ++glbsSel;

                  break;
                }
            }
        }
      else
        {
          globals += ' ';
          globals += glbName;
          ++glbsSel;
        }
    }

  if (cs != WCS_OK)
    goto cmdGlobalList_exit;

  linePos = 0;
  do
    {
      unsigned int rawType = 0;
      token   = CmdLineNextToken(globals, linePos);
      if (token.length() == 0)
	    break;

      cs = WDescribeGlobal(conHdl, token.c_str(), &rawType);
      if (cs != WCS_OK)
        {
          if (level <= VL_DEBUG)
            {
              cout << "Failed to fetch type information for '"
                   << token << "' global variable.\n";
            }
          break;
        }

      cout << token << ' ';
      if (rawType & WHC_TYPE_TABLE_MASK)
        {
          assert((rawType & WHC_TYPE_FIELD_MASK) == 0);

          uint_t fieldsCount;

          cs = WValueFieldsCount(conHdl, &fieldsCount);
          if (cs != WCS_OK)
            break;

          if (fieldsCount == 0)
            {
              cout << "TABLE\n";
              continue;
            }

          cout << "TABLE(";
          for (uint_t field = 0; field < fieldsCount; field++)
            {
              const char* fieldName;

              cs = WValueFetchField(conHdl, &fieldName, &rawType);
              if (cs != WCS_OK)
                break;

              if (field > 0)
                cout << ", ";

              cout << fieldName << " " << wcmd_decode_typeinfo(rawType);
            }
          cout << ")\n";
        }
      else if (rawType & WHC_TYPE_FIELD_MASK)
        {
          rawType &= ~WHC_TYPE_FIELD_MASK;

          cout << wcmd_decode_typeinfo(rawType) << " FIELD\n";
        }
      else
        cout << wcmd_decode_typeinfo(rawType) << endl;
    }
  while ((linePos < globals.length())
         && (cs == WCS_OK));

  if ((cs == WCS_OK)
      && (level >= VL_INFO))
    {
      cout << "Listed " << glbsSel << '(' << glbsCount << ") globals.\n";
    }


cmdGlobalList_exit:
  WClose(conHdl);

  if (cs != WCS_OK)
    cout << wcmd_translate_status(cs) << endl;

  return(cs == WCS_OK) ? true : false;
}



static const char procShowDesc[]    = "List context database's procedures.";
static const char procShowDescExt[] =
  "Show the procedures installed in the database context.\n"
  "If a name is provided it limits the listing to only those procedures\n"
  "Usage:\n"
  "  procedure [procedure_name] ... ";


static bool
cmdProcList(const string& cmdLine, ENTRY_CMD_CONTEXT context)
{
  size_t              linePos     = 0;
  string              token       = CmdLineNextToken(cmdLine, linePos);
  string              procedures;
  WH_CONNECTION       conHdl      = nullptr;
  uint_t              procsCount  = 0;
  uint_t              procsSelect = 0;
  const VERBOSE_LEVEL level       = GetVerbosityLevel();
  vector<string>      subStrs;

  uint32_t cs = WConnect(GetRemoteHostName().c_str(),
                          GetConnectionPort().c_str(),
                          GetWorkingDB().c_str(),
                          GetUserPassword().c_str(),
                          GetUserId(),
                          DEFAULT_FRAME_SIZE,
                          &conHdl);

  assert(token == "procedure");

  if (cs != WCS_OK)
    {
      if (level >= VL_DEBUG)
        cout << "Failed to connect: ";

      cout << wcmd_translate_status(cs) << endl;

      return false;
    }

  while (linePos <= cmdLine.length())
    {
      token = CmdLineNextToken(cmdLine, linePos);
      if (token.length() == 0)
        break;

      subStrs.push_back(token);
    }

  cs = WStartProceduresList(conHdl, &procsCount);
  if (level >= VL_DEBUG)
    {
      if (cs == WCS_OK)
        cout << "Got " << procsCount << " procedures.\n";

      else
        cout << "Listing procedures has failed\n";
    }

  uint_t procIt = procsCount;
  while ((cs == WCS_OK)
          && (procIt-- > 0))
    {
      const char* procName = nullptr;
      cs = WFetchProcedure(conHdl, &procName);
      if ((cs != WCS_OK) && (level < VL_DEBUG))
        cout << "Fetching procedure name has failed.\n";

      assert(procName != nullptr);

      if (subStrs.size() > 0)
        {
          for (size_t i = 0; i < subStrs.size(); ++i)
            {
              if (strstr(procName, subStrs[i].c_str()) != nullptr)
                {
                  procedures += ' ';
                  procedures += procName;
                  ++procsSelect;

                  break;
                }
            }
        }
      else
        {
          procedures += ' ';
          procedures += procName;
          ++procsSelect;
        }
    }

  if (cs != WCS_OK)
    goto cmdProcList_exit;

  linePos = 0;
  do
    {
      uint_t procsParametersCount;

      token   = CmdLineNextToken(procedures, linePos);
      if (token.empty())
        break;

      cs = WProcParamsCount(conHdl, token.c_str(), &procsParametersCount);

      if (cs != WCS_OK)
        {
          if (level <= VL_DEBUG)
            {
              cout << "Failed to get the number of arguments for procedure '"
                   << token << "'.\n";
            }
          break;
        }

      cout << token << " (";

      //Start from the first parameter and let the return type at then end
      //after the enclosing ')'.
      uint_t param = 1;
      do
        {
          param %= procsParametersCount;
          if (param == 0)
            cout << ") ";

          unsigned int paramType;

          cs = WProcParamType(conHdl, token.c_str(), param, &paramType);
          if (cs != WCS_OK)
            {
              if (level <= VL_DEBUG)
                {
                  cout << "Failed to fetch type information for '"
                       << token << "' procedure.\n";
                }
              break;
            }

          if (param > 1)
            cout << ", ";

          if (paramType & WHC_TYPE_TABLE_MASK)
            {
              assert((paramType & WHC_TYPE_FIELD_MASK) == 0);

              uint_t fieldsCount;

              cs = WProcParamFieldCount(conHdl,
                                         token.c_str(),
                                         param,
                                         &fieldsCount);
              if (cs != WCS_OK)
                break;

              if (fieldsCount == 0)
                {
                  cout << "TABLE";
                  continue;
                }

              cout << "TABLE(";
              for (uint_t field = 0; field < fieldsCount; field++)
                {
                  const char* fieldName;

                  cs = WProcParamField(conHdl,
                                        token.c_str(),
                                        param,
                                        field,
                                        &fieldName,
                                        &paramType);
                  if (cs != WCS_OK)
                    break;

                  if (field > 0)
                    cout << ", ";

                  cout << fieldName << " " << wcmd_decode_typeinfo(paramType);
                }
              cout << ')';
            }
          else if (paramType & WHC_TYPE_FIELD_MASK)
            {
              paramType &= ~WHC_TYPE_FIELD_MASK;

              if (paramType == WHC_TYPE_NOTSET)
                cout << "FIELD";

              else
                cout << wcmd_decode_typeinfo(paramType) << " FIELD";
            }
          else
            cout << wcmd_decode_typeinfo(paramType);
        }
      while (param++ > 0);

      cout << endl;
    }
  while ((linePos < procedures.length())
         && (cs == WCS_OK));

  if ((cs == WCS_OK)
      && (level >= VL_INFO))
    {
      cout << "Listed " << procsSelect << '(' << procsCount
           << ") procedures.\n";
    }

cmdProcList_exit:
  WClose(conHdl);

  if (cs != WCS_OK)
    cout << wcmd_translate_status(cs) << endl;

  return(cs == WCS_OK) ? true : false;
}



static const char pingShowDesc[]    = "Ping the database sever. ";
static const char pingShowDescExt[] =
  "Ping the database server to check if it is up.\n"
  "Usage:\n"
  "  ping";


static bool
cmdPing(const string& cmdLine, ENTRY_CMD_CONTEXT context)
{
  WH_CONNECTION conHdl = nullptr;
  WTICKS        ticks  = wh_msec_ticks();
  uint32_t      cs     = WConnect(GetRemoteHostName().c_str(),
                                   GetConnectionPort().c_str(),
                                   GetWorkingDB().c_str(),
                                   GetUserPassword().c_str(),
                                   GetUserId(),
                                   DEFAULT_FRAME_SIZE,
                                   &conHdl);
  if (cs != WCS_OK)
    goto cmd_ping_exit;

  cs = WPingServer(conHdl);

cmd_ping_exit:

  WClose(conHdl);

  ticks = wh_msec_ticks() - ticks;
  if (cs != WCS_OK)
    {
      cout << wcmd_translate_status(cs) << endl;
      return false;
    }
  else
    {
      cout << "Ping time: " << ticks / 1000 << '.';
      cout.width(3); cout.fill('0');
      cout << right << ticks % 1000<< "s.\n";
    }

  return true;
}


static const char greetShowDesc[]    = "Greet the database sever. ";
static const char greetShowDescExt[] =
  "Greet the database server to get a description of it.\n"
  "Usage:\n"
  "  greet";


static bool
cmdGreet(const string& cmdLine, ENTRY_CMD_CONTEXT context)
{
  const char*   servAns = nullptr;
  WH_CONNECTION conHdl  = nullptr;
  uint32_t      cs      = WConnect(GetRemoteHostName().c_str(),
                                    GetConnectionPort().c_str(),
                                    GetWorkingDB().c_str(),
                                    GetUserPassword().c_str(),
                                    GetUserId(),
                                    DEFAULT_FRAME_SIZE,
                                    &conHdl);
  if (cs != WCS_OK)
    goto cmd_greet_exit;

  cs = WGreetServer(conHdl, &servAns);

cmd_greet_exit:
  if (cs != WCS_OK)
    {
      WClose(conHdl);
      cout << wcmd_translate_status(cs) << endl;

      return false;
    }
  else
    {
      assert(servAns != nullptr);

      if (strlen(servAns) == 0)
        cout << "No answer from server.\n";

      else
        cout << "Server says:\n" << servAns << endl;

      WClose(conHdl);
    }

  return true;
}



static const char execShowDesc[]    = "Execute a procedure. ";
static const char execShowDescExt[] =
  "Execute a procedure on the remote server using the "
  "specified parameters.\n"
  "Base values parameter specifiers:\n"
  "  B   -- denotes a value of boolean type.\n"
  "  C   -- denotes a value of character type.\n"
  "  D   -- denotes a value of date type.\n"
  "  H   -- denotes a value of time type.\n"
  "  M   -- denotes a value of high resolution time type.\n"
  "  I8  -- denotes a value of 8 bits integer type.\n"
  "  I16 -- denotes a value of 16 bits integer type.\n"
  "  I32 -- denotes a value of 32 bits integer type.\n"
  "  I64 -- denotes a value of 64 bits integer type.\n"
  "  U8  -- denotes a value of 8 bits unsigned integer type.\n"
  "  U16 -- denotes a value of 16 bits unsigned integer type.\n"
  "  U32 -- denotes a value of 32 bits unsigned integer type.\n"
  "  U64 -- denotes a value of 64 bits unsigned integer type.\n"
  "  R   -- denotes a value of real type.\n"
  "  RR  -- denotes a value of real type.\n"
  "  T   -- denotes a value of text type.\n"
  "\n"
  "  For null values, use the type specifier followed empty string ''.\n"
  "\n"
  "Array values parameter specifiers:\n"
  "  Array values are specified using the base value type specifier followed\n"
  "  by a pair of {} holding the actual values inside. Like in the following\n"
  "  examples:\n"
  "\n"
  "  B{'1' '0'} -- an array holding two boolean values, true and false\n"
  "  D{'1970/1/1' '1999/12/31' } -- this array holds two well known dates.\n"
  "  H{'1970/1/1 0:0:0' '1999/12/31 23:59:59' } -- datetime variants\n"
  "  M{'1970/1/1 0:0:0.0' '1999/12/31 23:59:59.99999' } -- hirestime variants\n"
  "  RR{'1.0' '-1.0' } -- holding two rich real values.\n"
  "  I8{} -- and a null array holding 8 bit integers.\n"
  "\n"
  "Table values parameter specifiers:\n"
  "  Table values holds their rows inside of a() pair. Rows holds their\n"
  "  field values inside of []. A field value is specfied using the field's\n"
  "  followed by a dot and then by the value specifier as it is done in the\n"
  "  case of base and arrays values. Example:\n"
  "  (\n"
  "    [born.d'1970/1/1' name.t'Jhon Best' sav.U32{}]\n"
  "    [born.d'2030/1/1' name.t'HAL 500' sav.u32{'8' '32' '64'}]\n"
  "    []\n"
  "    [born.d'2100/1/1' name.t'HAL 1000' sav.u32{'7' '31' '63' '42'}]\n"
  "  )\n"
  "\n"
  "Usage:\n"
  "  exec proc_name\n"
  "  exec proc_name i8'231' t'Text \\'example\\'')\n";

void
AddOnlineTableCommands()
{

  CmdEntry entry;

  entry.mShowStatus   = true;
  entry.mName         = "global";
  entry.mDesc         = globalShowDesc;
  entry.mExtendedDesc = globalShowDescExt;
  entry.mCmd          = cmdGlobalList;

  RegisterCommand(entry);

  entry.mShowStatus   = true;
  entry.mName         = "procedure";
  entry.mDesc         = procShowDesc;
  entry.mExtendedDesc = procShowDescExt;
  entry.mCmd          = cmdProcList;

  RegisterCommand(entry);

  entry.mShowStatus   = true;
  entry.mName         = "ping";
  entry.mDesc         = pingShowDesc;
  entry.mExtendedDesc = pingShowDescExt;
  entry.mCmd          = cmdPing;

  RegisterCommand(entry);

  entry.mShowStatus   = true;
  entry.mName         = "greet";
  entry.mDesc         = greetShowDesc;
  entry.mExtendedDesc = greetShowDescExt;
  entry.mCmd          = cmdGreet;

  RegisterCommand(entry);

  entry.mShowStatus   = true;
  entry.mName         = "exec";
  entry.mDesc         = execShowDesc;
  entry.mExtendedDesc = execShowDescExt;
  entry.mCmd          = cmdExec;

  RegisterCommand(entry);
}

