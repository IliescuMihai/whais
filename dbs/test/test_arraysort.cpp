/*
 * test_arraysort.cpp
 *
 *  Created on: Jan 21, 2012
 *      Author: ipopa
 */


#include <assert.h>
#include <iostream>
#include <string.h>

#include "random.h"
#include "wthread.h"
#include "test/test_fmw.h"

#include "../include/dbs_mgr.h"
#include "../include/dbs_exception.h"
#include "../include/dbs_values.h"


const D_UINT _elemsCount = 1000000;

DBSDateTime
get_random_datetime ()
{
  D_INT16 year  = w_rnd () & 0xFFFF;
  D_UINT8 month = w_rnd () % 12 + 1;
  D_UINT8 day   = w_rnd () % 27 + 1;
  D_UINT8 hour  = w_rnd () % 24;
  D_UINT8 mins  = w_rnd () % 60;
  D_UINT8 secs  = w_rnd () % 60;

  return DBSDateTime (false, year, month, day, hour, mins, secs);
}

DBSHiresTime
get_random_hirestime ()
{
  D_INT16 year  = w_rnd () & 0xFFFF;
  D_UINT8 month = w_rnd () % 12 + 1;
  D_UINT8 day   = w_rnd () % 27 + 1;
  D_UINT8 hour  = w_rnd () % 24;
  D_UINT8 mins  = w_rnd () % 60;
  D_UINT8 secs  = w_rnd () % 60;
  D_UINT32  mic = w_rnd () % 1000000000;

  return DBSHiresTime (false, year, month, day, hour, mins, secs, mic);
}



DBSDate
get_random_date ()
{
  D_INT16 year  = w_rnd () & 0xFFFF;
  D_UINT8 month = w_rnd () % 12 + 1;
  D_UINT8 day   = w_rnd () % 27 + 1;

  return DBSDate (false, year, month, day);
}

void
test_array_with_dates (void *)
{
  std::cout << "Testing array sort with dates ...\n";
  DBSArray array (_SC (DBSDate*, NULL));

  for (D_UINT index = 0; index < _elemsCount; ++index)
    array.AddElement (get_random_date ());

  array.Sort ();

  if (array.GetElementsCount () != _elemsCount)
    throw 0;

  DBSDate lastValue (true);
  for (D_UINT index = 0; index < _elemsCount; ++index)
    {
      DBSDate currValue (true);
      array.GetElement (currValue, index);
      if (currValue < lastValue)
        throw 1;

      lastValue = currValue;
    }

  std::cout << "Testing array sort with dates ended!\n";
}

void
test_array_with_datetimes (void *)
{
  std::cout << "Testing array sort with datetimes ...\n";
  DBSArray array (_SC (DBSDateTime*, NULL));

  for (D_UINT index = 0; index < _elemsCount; ++index)
    array.AddElement (get_random_datetime ());

  array.Sort ();

  if (array.GetElementsCount () != _elemsCount)
    throw 0;

  DBSDateTime lastValue (true);
  for (D_UINT index = 0; index < _elemsCount; ++index)
    {
      DBSDateTime currValue (true);
      array.GetElement (currValue, index);
      if (currValue < lastValue)
        throw 1;

      lastValue = currValue;
    }

  std::cout << "Testing array sort with datetimes ended!\n";
}

void
test_array_with_hirestimes (void *)
{
  std::cout << "Testing array sort with hirestimes ...\n";
  DBSArray array (_SC (DBSHiresTime*, NULL));

  for (D_UINT index = 0; index < _elemsCount; ++index)
    array.AddElement (get_random_hirestime ());

  array.Sort ();

  if (array.GetElementsCount () != _elemsCount)
    throw 0;

  DBSHiresTime lastValue (true);
  for (D_UINT index = 0; index < _elemsCount; ++index)
    {
      DBSHiresTime currValue (true);
      array.GetElement (currValue, index);
      if (currValue < lastValue)
        throw 1;

      lastValue = currValue;
    }

  std::cout << "Testing array sort with hirestimes ended!\n";
}


void
test_array_with_dates_r (void *)
{
  std::cout << "Testing array reverse sort with dates ...\n";
  DBSArray array (_SC (DBSDate*, NULL));

  for (D_UINT index = 0; index < _elemsCount; ++index)
    array.AddElement (get_random_date ());

  array.Sort (true);

  if (array.GetElementsCount () != _elemsCount)
    throw 0;

  DBSDate lastValue (true);
  for (D_UINT index = _elemsCount; index-- > 0;)
    {
      DBSDate currValue (true);
      array.GetElement (currValue, index);
      if (currValue < lastValue)
        throw 1;

      lastValue = currValue;
    }

  std::cout << "Testing array reverse sort with dates ended!\n";

}

void
test_array_with_datetimes_r (void *)
{
  std::cout << "Testing array reverse sort with datetimes ...\n";
  DBSArray array (_SC (DBSDateTime*, NULL));

  for (D_UINT index = 0; index < _elemsCount; ++index)
    array.AddElement (get_random_datetime ());

  array.Sort (true);

  if (array.GetElementsCount () != _elemsCount)
    throw 0;

  DBSDateTime lastValue (true);
  for (D_UINT index = _elemsCount; index-- > 0 ;)
    {
      DBSDateTime currValue (true);
      array.GetElement (currValue, index);
      if (currValue < lastValue)
        throw 1;

      lastValue = currValue;
    }

  std::cout << "Testing array reverse sort with datetimes ended!\n";
}

void
test_array_with_hirestimes_r (void *)
{
  std::cout << "Testing array reverse sort with hirestimes ...\n";
  DBSArray array (_SC (DBSHiresTime*, NULL));

  for (D_UINT index = 0; index < _elemsCount; ++index)
    array.AddElement (get_random_hirestime ());

  array.Sort (true);

  if (array.GetElementsCount () != _elemsCount)
    throw 0;

  DBSHiresTime lastValue (true);
  for (D_UINT index = _elemsCount; index-- > 0;)
    {
      DBSHiresTime currValue (true);
      array.GetElement (currValue, index);
      if (currValue < lastValue)
        throw 1;

      lastValue = currValue;
    }

  std::cout << "Testing array reverse sort with hirestimes ended!\n";
}

int
main ()
{
  // VC++ allocates memory when the C++ runtime is initialized
  // We need not to test against it!
  D_UINT prealloc_mem = test_get_mem_used ();
  bool success = true;

  {
    std::string dir = ".";
    dir += whc_get_directory_delimiter ();

    DBSInit (dir.c_str (), dir.c_str ());
  }

  {
    WThread th1 (test_array_with_dates, NULL);
    WThread th2 (test_array_with_datetimes, NULL);
    WThread th3 (test_array_with_hirestimes, NULL);

    WThread th4 (test_array_with_dates_r, NULL);
    WThread th5 (test_array_with_datetimes_r, NULL);
    WThread th6 (test_array_with_hirestimes_r, NULL);

    th1.Join();
    th2.Join();
    th3.Join();
    th4.Join();
    th5.Join();
    th6.Join();
  }


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
  std::cout << "Current memory usage: " << mem_usage << " bytes." << std::endl;
  if (!success)
    {
      std::cout << "TEST RESULT: FAIL" << std::endl;
      return -1;
    }
  else
    std::cout << "TEST RESULT: PASS" << std::endl;

  return 0;
}

