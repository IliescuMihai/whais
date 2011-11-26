/*
 * test_dbsmgr.cpp
 *
 *  Created on: Jul 11, 2011
 *      Author: iupo
 */

#include <assert.h>
#include <iostream>
#include <string.h>
#include <vector>

#include "test/test_fmw.h"

#include "../include/dbs_mgr.h"
#include "../include/dbs_exception.h"

#include "../pastra/ps_table.h"
#include "../pastra/ps_valintep.h"

using namespace pastra;

static D_INT
get_next_alignment (D_INT size)
{
  D_INT result = 1;

  assert (size > 0);

  size &= 0x7;
  size |= 0x8;

  while ((size & 1) == 0)
    {
      result <<= 1;
      size >>= 1;
    }

  return result;
}

bool
operator!= (const DBSFieldDescriptor & field_1,
            const DBSFieldDescriptor & field_2)
{
  return (field_1.mFieldType != field_2.mFieldType) ||
         (field_1.isArray != field_2.isArray) ||
         (strcmp (field_1.mpFieldName, field_2.mpFieldName) != 0);
}

bool
test_for_no_args (I_DBSHandler &rDbs)
{
  bool result = false;

  std::cout << "Test for invalid argumetns ... ";

  try
  {
    rDbs.AddTable ("test_dummy", NULL, 10);
  }
  catch (DBSException &e)
  {
    if (e.GetExtra() == DBSException::INVALID_PARAMETERS)
      result = true;
  }

  try
  {
    DBSFieldDescriptor temp;
    temp.mpFieldName = "dummy";
    temp.mFieldType = T_BOOL;
    temp.isArray = false;

    if (result)
      rDbs.AddTable ("test_dummy", &temp, 0);
    result = false;
  }
  catch (DBSException &e)
  {
    if (e.GetExtra() == DBSException::INVALID_PARAMETERS)
      result = true;
    else
      result = false;
  }

  std::cout << ( result ? "OK" : "FALSE") << std::endl;
  return result;
}

bool
test_for_invalid_fields (I_DBSHandler &rDbs)
{
  bool result = false;
  DBSFieldDescriptor temp;

  std::cout << "Test for invalid fields ... ";

  temp.mFieldType = _SC (DBS_FIELD_TYPE, 78);
  temp.isArray = false;
  temp.mpFieldName = "good_name";

  try
  {
    rDbs.AddTable("test_dummy", &temp, 1);
  }
  catch (DBSException &e)
  {
    if (e.GetExtra () == DBSException::FIELD_TYPE_INVALID)
      result = true;
  }

  temp.mpFieldName = "1bad_name?";
  temp.mFieldType = T_TEXT;

  try
  {
    if( result )
      rDbs.AddTable("test_dummy", &temp, 1);
    result = false;
  }
  catch (DBSException &e)
  {
    if (e.GetExtra () == DBSException::FIELD_NAME_INVALID)
      result = true;
    else
      result = false;
  }

  try
  {
    DBSFieldDescriptor more_temps[3];

    temp.mpFieldName = "field_1";
    more_temps[0] = temp;
    temp.mpFieldName = "field_2";
    more_temps[1] = temp;
    temp.mpFieldName = "field_1";
    more_temps[2] = temp;

    if( result )
      rDbs.AddTable("test_dummy", more_temps, 3);
    result = false;
  }
  catch (DBSException &e)
  {
    if (e.GetExtra () == DBSException::FIELD_NAME_DUPLICATED)
      result = true;
    else
      result = false;
  }


  std::cout << ( result ? "OK" : "FALSE") << std::endl;
  return result;
}

bool
test_for_one_field (I_DBSHandler &rDbs)
{
  bool result = true;

  std::cout << "Test with one field ... ";

  DBSFieldDescriptor temp;
  temp.mpFieldName = "dummy";
  temp.mFieldType = T_INT16;
  temp.isArray = false;

  rDbs.AddTable ("t_test_tab", &temp, 1);
  I_DBSTable &table = rDbs.RetrievePersistentTable ("t_test_tab");

  D_UINT rowSize = (_RC (pastra::I_PSTable &, table)).GetRowSize ();


  //Check if we added the byte to keep the null bit.
  if (_SC (D_INT, rowSize + 1) <= pastra::PSValInterp::GetSize(T_INT16, false))
    result = false;
  else if (get_next_alignment (rowSize) <
      pastra::PSValInterp::GetAlignment (T_INT16, false))
    result = false;

  rDbs.ReleaseTable (table);
  rDbs.DeleteTable ("t_test_tab");

  std::cout << ( result ? "OK" : "FALSE") << std::endl;
  return result;
}

struct StorageInterval
  {
    StorageInterval (D_UINT32 begin_byte, D_UINT32 end_byte) :
        mBegin (begin_byte),
        mEnd (end_byte)
    {}

    D_UINT32 mBegin;
    D_UINT32 mEnd;

  };

bool
test_for_fields (I_DBSHandler &rDbs,
                 const DBSFieldDescriptor *pDesc,
                 const D_UINT32 fieldsCount)
{
  bool result = true;

  std::cout << "Test with fields with count " << fieldsCount << " ... ";

  rDbs.AddTable ("t_test_tab", pDesc, fieldsCount);
  I_DBSTable &table = rDbs.RetrievePersistentTable ("t_test_tab");

  std::vector <D_UINT32 > nullPositions;
  std::vector <StorageInterval> storage;

  if (table.GetFieldsCount () != fieldsCount)
    result = false;

  for (D_UINT32 fieldIndex = 0;
      result && (fieldIndex < fieldsCount);
      ++fieldIndex)
    {
      PSFieldDescriptor descr = _SC(I_PSTable &, table).GetFieldDescriptorInternal (fieldIndex);

      if (descr.mNullBitIndex >= _SC(I_PSTable &, table).GetRowSize () * 8)
        {
          result = false;
          break;
        }

      for (D_UINT index = 0; index < nullPositions.size (); ++index)
        {
          if (descr.mNullBitIndex == nullPositions[index])
            {
              result = false;
              break;
            }
        }

      if (descr.mNullBitIndex)
        nullPositions.push_back (descr.mNullBitIndex);

      if (! result)
        break;

      D_UINT elem_start = descr.mStoreIndex;
      D_UINT elem_end = elem_start +
          PSValInterp::GetSize (_SC (DBS_FIELD_TYPE, descr.mTypeDesc & PS_TABLE_FIELD_TYPE_MASK),
                                descr.mTypeDesc & PS_TABLE_ARRAY_MASK);

      for (D_UINT index = 0; index < storage.size (); ++index)
        {
          if ( ((storage[index].mBegin >= elem_start) &&
              (elem_start <= storage[index].mEnd))
              || ((storage[index].mBegin >= elem_end) &&
                  (elem_end <= storage[index].mEnd))
              || ((storage[index].mBegin >= descr.mNullBitIndex) &&
                (descr.mNullBitIndex <= storage[index].mEnd) && descr.mNullBitIndex) )
            {
              result = false;
              break;
            }
        }
      storage.push_back (StorageInterval (elem_start, elem_end - 1));

      if (result && (elem_start > 0))
        {
          if ( get_next_alignment (elem_start) >
            PSValInterp::GetSize (_SC (DBS_FIELD_TYPE,
                                   descr.mTypeDesc & PS_TABLE_FIELD_TYPE_MASK),
                                   descr.mTypeDesc & PS_TABLE_ARRAY_MASK))
            result = false;
            break;
        }

      if (result)
        {
          DBSFieldDescriptor desc = table.GetFieldDescriptor(fieldIndex);
          D_UINT array_index = desc.mpFieldName[0] - 'a';
          if (desc != pDesc[array_index])
            {
              result = false;
              break;
            }
        }

    }

  rDbs.ReleaseTable (table);
  rDbs.DeleteTable ("t_test_tab");

  std::cout << ( result ? "OK" : "FALSE") << std::endl;
  return result;
}


struct DBSFieldDescriptor int_descs[] = {
    {"a", T_UINT8, false},
    {"b", T_UINT16, false},
    {"c", T_UINT32, false},
    {"d", T_UINT64, false},
    {"e", T_INT64, false},
    {"f", T_INT32, false},
    {"g", T_INT16, false},
    {"h", T_INT8, false}
};

struct DBSFieldDescriptor non_int_descs[] = {
    {"a", T_DATE, false},
    {"b", T_DATETIME, false},
    {"c", T_HIRESTIME, false},
    {"d", T_REAL, false},
    {"e", T_RICHREAL, false},
    {"f", T_BOOL, false},
    {"g", T_TEXT, false},
};

struct DBSFieldDescriptor alt_descs[] = {
    {"a", T_UINT8, false},
    {"b", T_UINT16, false},
    {"c", T_UINT32, false},
    {"d", T_UINT64, false},
    {"e", T_INT64, true},
    {"f", T_INT32, false},
    {"g", T_INT16, true},
    {"h", T_INT8, true},
    {"i", T_DATE, false},
    {"j", T_DATETIME, true},
    {"k", T_HIRESTIME, false},
    {"l", T_REAL, false},
    {"m", T_RICHREAL, true},
    {"n", T_BOOL, false},
    {"o", T_TEXT, false},
    {"p", T_TEXT, false}
};

const D_CHAR db_name[] = "t_baza_date_1";
int
main ()
{
  // VC++ allocates memory when the C++ runtime is initialised
  // We need not to test against it!
  std::cout << "Print a message to not confuse the memory tracker: " << (D_UINT) 0x3456 << "\n";
  D_UINT prealloc_mem = test_get_mem_used ();
  bool success = true;

  {
    std::string dir = ".";
    dir += whc_get_directory_delimiter ();

    DBSInit (dir.c_str (), dir.c_str ());
  }

  DBSCreateDatabase (db_name);
  I_DBSHandler & handler = DBSRetrieveDatabase (db_name);

  success = test_for_no_args (handler);
  success = success && test_for_invalid_fields (handler);
  success = success && test_for_one_field (handler);
  success = success && test_for_fields (handler, int_descs,
                                        sizeof (int_descs) / sizeof (int_descs[0]));
  success = success && test_for_fields (handler,
                                        non_int_descs, sizeof (non_int_descs) / sizeof (non_int_descs[0]));
  success = success && test_for_fields (handler,
                                        alt_descs, sizeof (alt_descs) / sizeof (alt_descs[0]));

  DBSReleaseDatabase (handler);
  DBSRemoveDatabase (db_name);
  DBSShoutdown ();


  D_UINT mem_usage = test_get_mem_used () - prealloc_mem;

  if (mem_usage)
    {
      success = false;
      test_print_unfree_mem();
    }

  std::cout << "Memory peak (no prealloc): " <<
            test_get_mem_peak () - prealloc_mem << " bytes." << std::endl;
  std::cout << "Preallocated mem: " << prealloc_mem << " bytes." << std::endl;
  std::cout << "Current memory usage: " << mem_usage << " bytes." << std::
            endl;
  if (!success)
    {
      std::cout << "TEST RESULT: FAIL" << std::endl;
      return -1;
    }
  else
    std::cout << "TEST RESULT: PASS" << std::endl;

  return 0;
}
