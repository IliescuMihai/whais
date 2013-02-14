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
#include <cstdio>
#include <cstring>

#include "utils/include/utf8.h"

#include "valtranslator.h"

static const D_UINT MAX_INTEGER_STR_SIZE = 64;

static D_INT64
read_real (const D_UINT8* pLocation,
           D_INT64*       pIntPart,
           D_INT64*       pFraPart,
           D_INT64*       pPrecision )
{
  D_INT64 size     = 0;
  bool    negative = false;

  if (*pLocation == '-')
    {
      pLocation++;
      if ((*pLocation < '0') || (*pLocation > '9'))
        return 0;

      size++;
      negative = true;
    }

  *pIntPart   = 0;
  *pFraPart   = 0;
  *pPrecision = 1;

  while (('0' <= *pLocation) && (*pLocation <= '9' ))
    {
      *pIntPart *= 10;
      *pIntPart += *pLocation - '0';

      pLocation++, size++;
    }

  if (*pLocation == '.')
    pLocation++, size++;

  while (('0' <= *pLocation) && (*pLocation <= '9' ))
    {
      *pFraPart   *= 10;
      *pFraPart   += *pLocation - '0';
      *pPrecision *= 10;

      pLocation++, size++;
    }

  if (negative)
    *pIntPart = -*pIntPart, *pFraPart = -*pFraPart;

  return size;
}

static D_INT64
read_integer (const D_UINT8* pLocation, D_UINT* const pCount)
{
  D_INT64 result     = 0;
  bool    isNegative = false;

  *pCount = 0;
  if (*pLocation == '-')
    {
      ++pLocation;
      if ((*pLocation < '0') || (*pLocation > '9'))
        return 0; // 0 should signal an error

      isNegative  = true;
      *pCount    += 1;
    }

  while (*pLocation >= '0' && *pLocation <= '9')
    {
      result *= 10;
      result += *pLocation - '0';

      ++pLocation;
      *pCount += 1;
    }

  if (isNegative)
    result *= -1;

  return result;
}

D_UINT
Utf8Translator::Read (const D_UINT8*       utf8Src,
                      const D_UINT         srcSize,
                      DBSBool* const       pValue)
{
  if (sizeof (*utf8Src) > srcSize)
    return 0;

  if (*utf8Src == 0)
    {
      *pValue = DBSBool ();
      return 1;
    }
  else if ( 2 * sizeof (D_UINT8) > srcSize)
    return 0;
  else
    {
      *pValue = DBSBool (*utf8Src == '1');
      if ((utf8Src[1] != 0)
          || ((*utf8Src != '1') && (*utf8Src != '0')))
        {
          return 0;
        }

      return 2;
    }

  return 0;
}

D_UINT
Utf8Translator::Read (const D_UINT8*       utf8Src,
                      const D_UINT         srcSize,
                      DBSChar* const       pValue)
{
  if (sizeof (*utf8Src) > srcSize)
    return 0;

  if (*utf8Src == 0)
    {
      *pValue = DBSChar ();
      return 1;
    }

  const D_UINT result = get_utf8_char_size (*utf8Src);
  if ((result == 0) || (result + 1 > srcSize))
    return 0;
  else
    {
      D_UINT32 ch;
      if ((decode_utf8_char (utf8Src, &ch) != result)
          || (ch == 0)
          || (utf8Src[result] != 0))
        {
          return 0;
        }

      *pValue = DBSChar (ch);
    }

  return result + 1;
}

D_UINT
Utf8Translator::Read (const D_UINT8*       utf8Src,
                      const D_UINT         srcSize,
                      DBSDate* const       pValue)
{
  if (sizeof (*utf8Src) > srcSize)
    return 0;

  if (*utf8Src == 0)
    {
      *pValue = DBSDate ();
      return 1;
    }

  D_UINT temp   = 0;
  D_UINT result = 0;

  D_INT64 year = read_integer (utf8Src, &temp);
  result += temp;
  if ((temp == 0) || (utf8Src[result++] != '/') || (result > srcSize))
    return 0;

  D_INT64 month = read_integer (utf8Src + result, &temp);
  result += temp;
  if ((temp == 0) || (utf8Src[result++] != '/') || (result > srcSize))
    return 0;

  D_INT64 day = read_integer (utf8Src + result, &temp);
  result += temp;
  if ((temp == 0) || (utf8Src[result++] != 0) || (result > srcSize))
    return 0;

  *pValue = DBSDate (year, month, day);

  return result;
}

D_UINT
Utf8Translator::Read (const D_UINT8*       utf8Src,
                      const D_UINT         srcSize,
                      DBSDateTime* const   pValue)
{
  if (sizeof (*utf8Src) > srcSize)
    return 0;

  if (*utf8Src == 0)
    {
      *pValue = DBSDateTime ();
      return 1;
    }

  D_UINT temp   = 0;
  D_UINT result = 0;

  D_INT64 year = read_integer (utf8Src, &temp);
  result += temp;
  if ((temp == 0) || (utf8Src[result++] != '/') || (result > srcSize))
    return 0;

  D_INT64 month = read_integer (utf8Src + result, &temp);
  result += temp;
  if ((temp == 0) || (utf8Src[result++] != '/') || (result > srcSize))
    return 0;

  D_INT64 day = read_integer (utf8Src + result, &temp);
  result += temp;
  if ((temp == 0) || (utf8Src[result++] != ' ') || (result > srcSize))
    return 0;

  D_INT64 hour = read_integer (utf8Src + result, &temp);
  result += temp;
  if ((temp == 0) || (utf8Src[result++] != ':') || (result > srcSize))
    return 0;

  D_INT64 min = read_integer (utf8Src + result, &temp);
  result += temp;
  if ((temp == 0) || (utf8Src[result++] != ':') || (result > srcSize))
    return 0;

  D_INT64 secs = read_integer (utf8Src + result, &temp);
  result += temp;
  if ((temp == 0) || (utf8Src[result++] != 0) || (result > srcSize))
    return 0;

  *pValue = DBSDateTime (year, month, day, hour, min, secs);

  return result;
}

D_UINT
Utf8Translator::Read (const D_UINT8*       utf8Src,
                      const D_UINT         srcSize,
                      DBSHiresTime* const  pValue)
{
  if (sizeof (*utf8Src) > srcSize)
    return 0;

  if (*utf8Src == 0)
    {
      *pValue = DBSHiresTime ();
      return 1;
    }

  D_UINT temp   = 0;
  D_UINT result = 0;

  D_INT64 year = read_integer (utf8Src, &temp);
  result += temp;
  if ((temp == 0) || (utf8Src[result++] != '/') || (result > srcSize))
    return 0;

  D_INT64 month = read_integer (utf8Src + result, &temp);
  result += temp;
  if ((temp == 0) || (utf8Src[result++] != '/') || (result > srcSize))
    return 0;

  D_INT64 day = read_integer (utf8Src + result, &temp);
  result += temp;
  if ((temp == 0) || (utf8Src[result++] != ' ') || (result > srcSize))
    return 0;

  D_INT64 hour = read_integer (utf8Src + result, &temp);
  result += temp;
  if ((temp == 0) || (utf8Src[result++] != ':') || (result > srcSize))
    return 0;

  D_INT64 min = read_integer (utf8Src + result, &temp);
  result += temp;
  if ((temp == 0) || (utf8Src[result++] != ':') || (result > srcSize))
    return 0;

  D_INT64 secs = read_integer (utf8Src + result, &temp);
  result += temp;
  if ((temp == 0) || (utf8Src[result++] != '.') || (result > srcSize))
    return 0;

  D_INT64 usecs = read_integer (utf8Src + result, &temp);
  result += temp;
  if ((temp == 0) || (utf8Src[result++] != 0) || (result > srcSize))
    return 0;


  *pValue = DBSHiresTime (year, month, day, hour, min, secs, usecs);

  return result;
}

D_UINT
Utf8Translator::Read (const D_UINT8*       utf8Src,
                      const D_UINT         srcSize,
                      DBSInt8* const       pValue)
{
  if (sizeof (*utf8Src) > srcSize)
    return 0;

  if (*utf8Src == 0)
    {
      *pValue = DBSInt8 ();
      return 1;
    }

  D_UINT  result = 0;
  D_INT64 value  = read_integer (utf8Src, &result);
  if ((result == 0)
      || (utf8Src[result++] != 0)
      || (result > srcSize))
    {
      return 0;
    }

  *pValue = DBSInt8 (value);

  return result;
}

D_UINT
Utf8Translator::Read (const D_UINT8*       utf8Src,
                      const D_UINT         srcSize,
                      DBSInt16* const      pValue)
{
  if (sizeof (*utf8Src) > srcSize)
    return 0;

  if (*utf8Src == 0)
    {
      *pValue = DBSInt16 ();
      return 1;
    }

  D_UINT  result = 0;
  D_INT64 value  = read_integer (utf8Src, &result);
  if ((result == 0)
      || (utf8Src[result++] != 0)
      || (result > srcSize))
    {
      return 0;
    }

  *pValue = DBSInt16 (value);

  return result;
}

D_UINT
Utf8Translator::Read (const D_UINT8*       utf8Src,
                      const D_UINT         srcSize,
                      DBSInt32* const      pValue)
{
  if (sizeof (*utf8Src) > srcSize)
    return 0;

  if (*utf8Src == 0)
    {
      *pValue = DBSInt32 ();
      return 1;
    }

  D_UINT  result = 0;
  D_INT64 value  = read_integer (utf8Src, &result);
  if ((result == 0)
      || (utf8Src[result++] != 0)
      || (result > srcSize))
    {
      return 0;
    }

  *pValue = DBSInt32 (value);

  return result;
}

D_UINT
Utf8Translator::Read (const D_UINT8*       utf8Src,
                      const D_UINT         srcSize,
                      DBSInt64* const      pValue)
{
  if (sizeof (*utf8Src) > srcSize)
    return 0;

  if (*utf8Src == 0)
    {
      *pValue = DBSInt64 ();
      return 1;
    }

  D_UINT  result = 0;
  D_INT64 value  = read_integer (utf8Src, &result);
  if ((result == 0)
      || (utf8Src[result++] != 0)
      || (result > srcSize))
    {
      return 0;
    }

  *pValue = DBSInt64 (value);

  return result;
}

D_UINT
Utf8Translator::Read (const D_UINT8*       utf8Src,
                      const D_UINT         srcSize,
                      DBSUInt8* const      pValue)
{
  if (sizeof (*utf8Src) > srcSize)
    return 0;

  if (*utf8Src == 0)
    {
      *pValue = DBSUInt8 ();
      return 1;
    }

  D_UINT  result = 0;
  D_INT64 value  = read_integer (utf8Src, &result);
  if ((result == 0)
      || (utf8Src[result++] != 0)
      || (result > srcSize)
      || (value < 0))
    {
      return 0;
    }
  *pValue = DBSUInt8 (value);

  return result;
}

D_UINT
Utf8Translator::Read (const D_UINT8*       utf8Src,
                      const D_UINT         srcSize,
                      DBSUInt16* const     pValue)
{
  if (sizeof (*utf8Src) > srcSize)
    return 0;

  if (*utf8Src == 0)
    {
      *pValue = DBSUInt16 ();
      return 1;
    }

  D_UINT  result = 0;
  D_INT64 value  = read_integer (utf8Src, &result);
  if ((result == 0)
      || (utf8Src[result++] != 0)
      || (result > srcSize)
      || (value < 0))
    {
      return 0;
    }
  *pValue = DBSUInt16 (value);

  return result;
}

D_UINT
Utf8Translator::Read (const D_UINT8*       utf8Src,
                      const D_UINT         srcSize,
                      DBSUInt32* const     pValue)
{
  if (sizeof (*utf8Src) > srcSize)
    return 0;

  if (*utf8Src == 0)
    {
      *pValue = DBSUInt32 ();
      return 1;
    }

  D_UINT  result = 0;
  D_INT64 value  = read_integer (utf8Src, &result);
  if ((result == 0)
      || (utf8Src[result++] != 0)
      || (result > srcSize)
      || (value < 0))
    {
      return 0;
    }

  *pValue = DBSUInt32 (value);

  return result;
}

D_UINT
Utf8Translator::Read (const D_UINT8*       utf8Src,
                      const D_UINT         srcSize,
                      DBSUInt64* const     pValue)
{
  if (sizeof (*utf8Src) > srcSize)
    return 0;

  if (*utf8Src == 0)
    {
      *pValue = DBSUInt64 ();
      return 1;
    }

  D_UINT  result = 0;
  D_INT64 value  = read_integer (utf8Src, &result);
  if ((result == 0)
      || (utf8Src[result++] != 0)
      || (result > srcSize)
      || (value < 0))
    {
      return 0;
    }

  *pValue = DBSUInt64 (value);

  return result;
}

D_UINT
Utf8Translator::Read (const D_UINT8*      utf8Src,
                      const D_UINT        srcSize,
                      DBSReal* const      pValue)
{
  if (sizeof (*utf8Src) > srcSize)
    return 0;

  if (*utf8Src == 0)
    {
      *pValue = DBSReal ();
      return 1;
    }

  D_INT64 intPart, fracPart, precision;
  D_UINT  result  = read_real (utf8Src, &intPart, &fracPart, &precision);
  if ((result == 0)
      || (utf8Src[result++] != 0)
      || (result > srcSize)
      || (precision % 10 != 0))
    {
      return 0;
    }

  *pValue = DBSReal (DBS_REAL_T (intPart, fracPart, precision));

  return result;
}

D_UINT
Utf8Translator::Read (const D_UINT8*      utf8Src,
                      const D_UINT        srcSize,
                      DBSRichReal* const  pValue)
{
  if (sizeof (*utf8Src) > srcSize)
    return 0;

  if (*utf8Src == 0)
    {
      *pValue = DBSRichReal ();
      return 1;
    }

  D_INT64 intPart, fracPart, precision;
  D_UINT  result  = read_real (utf8Src, &intPart, &fracPart, &precision);
  if ((result == 0)
      || (utf8Src[result++] != 0)
      || (result > srcSize)
      || (precision % 10 != 0))
    {
      return 0;
    }

  *pValue = DBSRichReal (DBS_RICHREAL_T (intPart, fracPart, precision));

  return result;
}

D_UINT
Utf8Translator::Write (D_UINT8* const      utf8Dest,
                       const D_UINT        maxSize,
                       const DBSBool&      value)
{
  if (maxSize == 0)
    return 0;

  if (value.IsNull ())
    {
      utf8Dest[0] = 0;
      return 1;
    }
  else if (maxSize < 2)
    return 0;
  else if (value.m_Value)
    utf8Dest[0] = '1';
  else
    utf8Dest[0] = '0';

  utf8Dest[1] = 0;
  return 2;
}

D_UINT
Utf8Translator::Write (D_UINT8* const      utf8Dest,
                       const D_UINT        maxSize,
                       const DBSChar&      value)
{
  if (maxSize == 0)
    return 0;

  if (value.IsNull ())
    {
      utf8Dest[0] = 0;
      return 1;
    }
  D_UINT  result = encode_utf8_char (value.m_Value, utf8Dest);
  if (result + 1 > maxSize)
    return 0;

  utf8Dest[result++] = 0;
  return result;
}

D_UINT
Utf8Translator::Write (D_UINT8* const   utf8Dest,
                       const D_UINT        maxSize,
                       const DBSDate&      value)
{
  if (maxSize == 0)
    return 0;

  if (value.IsNull ())
    {
      utf8Dest[0] = 0;
      return 1;
    }

  D_INT result = snprintf (_RC (D_CHAR*, utf8Dest),
                           maxSize,
                           "%d/%d/%d",
                           value.m_Year,
                           value.m_Month,
                           value.m_Day);
  if ((result < 0) || (_SC (D_UINT, result) >= maxSize))
    return 0;

  return result + 1;

}

D_UINT
Utf8Translator::Write (D_UINT8* const      utf8Dest,
                       const D_UINT        maxSize,
                       const DBSDateTime&  value)
{
  if (maxSize == 0)
    return 0;

  if (value.IsNull ())
    {
      utf8Dest[0] = 0;
      return 1;
    }

  D_INT result = snprintf (_RC (D_CHAR*, utf8Dest),
                           maxSize,
                           "%d/%d/%d %d:%d:%d",
                           value.m_Year,
                           value.m_Month,
                           value.m_Day,
                           value.m_Hour,
                           value.m_Minutes,
                           value.m_Seconds);
  if ((result < 0) || (_SC (D_UINT, result) >= maxSize))
    return 0;

  return result + 1;
}

D_UINT
Utf8Translator::Write (D_UINT8* const      utf8Dest,
                       const D_UINT        maxSize,
                       const DBSHiresTime& value)
{
  if (maxSize == 0)
    return 0;

  if (value.IsNull ())
    {
      utf8Dest[0] = 0;
      return 1;
    }

  D_INT result = snprintf (_RC (D_CHAR*, utf8Dest),
                           maxSize,
                           "%d/%d/%d %d:%d:%d.%d",
                           value.m_Year,
                           value.m_Month,
                           value.m_Day,
                           value.m_Hour,
                           value.m_Minutes,
                           value.m_Seconds,
                           value.m_Microsec);
  if ((result < 0) || (_SC (D_UINT, result) >= maxSize))
    return 0;

  return result + 1;
}

D_UINT
Utf8Translator::Write (D_UINT8* const      utf8Dest,
                       const D_UINT        maxSize,
                       const DBSInt8&      value)
{
  if (maxSize == 0)
    return 0;

  if (value.IsNull ())
    {
      utf8Dest[0] = 0;
      return 1;
    }

  D_INT result = snprintf (_RC (D_CHAR*, utf8Dest),
                           maxSize,
                           "%lld",
                           _SC (long long, value.m_Value));
  if ((result < 0) || (_SC (D_UINT, result) >= maxSize))
    return 0;

  return result + 1;
}

D_UINT
Utf8Translator::Write (D_UINT8* const      utf8Dest,
                       const D_UINT        maxSize,
                       const DBSInt16&     value)
{
  if (maxSize == 0)
    return 0;

  if (value.IsNull ())
    {
      utf8Dest[0] = 0;
      return 1;
    }

  D_INT result = snprintf (_RC (D_CHAR*, utf8Dest),
                           maxSize,
                           "%lld",
                           _SC (long long, value.m_Value));
  if ((result < 0) || (_SC (D_UINT, result) >= maxSize))
    return 0;

  return result + 1;
}

D_UINT
Utf8Translator::Write (D_UINT8* const      utf8Dest,
                       const D_UINT        maxSize,
                       const DBSInt32&     value)
{
  if (maxSize == 0)
    return 0;

  if (value.IsNull ())
    {
      utf8Dest[0] = 0;
      return 1;
    }

  D_INT result = snprintf (_RC (D_CHAR*, utf8Dest),
                           maxSize,
                           "%lld",
                           _SC (long long, value.m_Value));
  if ((result < 0) || (_SC (D_UINT, result) >= maxSize))
    return 0;

  return result + 1;
}

D_UINT
Utf8Translator::Write (D_UINT8* const      utf8Dest,
                       const D_UINT        maxSize,
                       const DBSInt64&     value)
{
  if (maxSize == 0)
    return 0;

  if (value.IsNull ())
    {
      utf8Dest[0] = 0;
      return 1;
    }

  D_INT result = snprintf (_RC (D_CHAR*, utf8Dest),
                           maxSize,
                           "%lld",
                           _SC (long long, value.m_Value));
  if ((result < 0) || (_SC (D_UINT, result) >= maxSize))
    return 0;

  return result + 1;
}


D_UINT
Utf8Translator::Write (D_UINT8* const      utf8Dest,
                       const D_UINT        maxSize,
                       const DBSUInt8&     value)
{
  if (maxSize == 0)
    return 0;

  if (value.IsNull ())
    {
      utf8Dest[0] = 0;
      return 1;
    }

  D_INT result = snprintf (_RC (D_CHAR*, utf8Dest),
                           maxSize,
                           "%llu",
                           _SC (unsigned long long, value.m_Value));
  if ((result < 0) || (_SC (D_UINT, result) >= maxSize))
    return 0;

  return result + 1;
}

D_UINT
Utf8Translator::Write (D_UINT8* const      utf8Dest,
                       const D_UINT        maxSize,
                       const DBSUInt16&    value)
{
  if (maxSize == 0)
    return 0;

  if (value.IsNull ())
    {
      utf8Dest[0] = 0;
      return 1;
    }

  D_INT result = snprintf (_RC (D_CHAR*, utf8Dest),
                           maxSize,
                           "%llu",
                           _SC (unsigned long long, value.m_Value));
  if ((result < 0) || (_SC (D_UINT, result) >= maxSize))
    return 0;

  return result + 1;
}

D_UINT
Utf8Translator::Write (D_UINT8* const      utf8Dest,
                       const D_UINT        maxSize,
                       const DBSUInt32&    value)
{
  if (maxSize == 0)
    return 0;

  if (value.IsNull ())
    {
      utf8Dest[0] = 0;
      return 1;
    }

  D_INT result = snprintf (_RC (D_CHAR*, utf8Dest),
                           maxSize,
                           "%llu",
                           _SC (unsigned long long, value.m_Value));
  if ((result < 0) || (_SC (D_UINT, result) >= maxSize))
    return 0;

  return result + 1;
}

D_UINT
Utf8Translator::Write (D_UINT8* const      utf8Dest,
                       const D_UINT        maxSize,
                       const DBSUInt64&    value)
{
  if (maxSize == 0)
    return 0;

  if (value.IsNull ())
    {
      utf8Dest[0] = 0;
      return 1;
    }

  D_INT result = snprintf (_RC (D_CHAR*, utf8Dest),
                           maxSize,
                           "%llu",
                           _SC (unsigned long long, value.m_Value));
  if ((result < 0) || (_SC (D_UINT, result) >= maxSize))
    return 0;

  return result + 1;
}

D_UINT
Utf8Translator::Write (D_UINT8* const      utf8Dest,
                       const D_UINT        maxSize,
                       const DBSReal&      value)
{
  D_CHAR tempBuffer[MAX_INTEGER_STR_SIZE];
   D_UINT result = 0;
   D_UINT size   = maxSize;

   if (maxSize <= 1)
     return 0;

   *utf8Dest = 0;
   if (value.IsNull ())
     return 1;

   D_INT64 intPart  = value.m_Value.Integer ();
   D_INT64 fracPart = value.m_Value.Fractional ();
   if ((intPart < 0) || (fracPart < 0))
     {
       strcat (_RC (D_CHAR*, utf8Dest),  "-");

       intPart = -intPart, fracPart = -fracPart;
       size--, result++;
     }

   if (fracPart > 0)
     {
       while ((fracPart % 10) == 0)
         fracPart /= 10;
     }

   result += snprintf (tempBuffer,
                       size,
                      "%lld.%lld",
                      _SC (long long int, intPart),
                      _SC (long long int, fracPart));

   strcat (_RC (D_CHAR*, utf8Dest), tempBuffer);
   assert (strlen (_RC (D_CHAR*, utf8Dest)) == result);

   if ((result < 0) || (_SC (D_UINT, result) >= maxSize))
     return 0;

   return result + 1;
}

D_UINT
Utf8Translator::Write (D_UINT8* const      utf8Dest,
                       const D_UINT        maxSize,
                       const DBSRichReal&  value)
{
  D_CHAR tempBuffer[MAX_INTEGER_STR_SIZE];
  D_UINT result = 0;
  D_UINT size   = maxSize;

  if (maxSize <= 1)
    return 0;

  *utf8Dest = 0;
  if (value.IsNull ())
    return 1;

  D_INT64 intPart  = value.m_Value.Integer ();
  D_INT64 fracPart = value.m_Value.Fractional ();
  if ((intPart < 0) || (fracPart < 0))
    {
      strcat (_RC (D_CHAR*, utf8Dest),  "-");

      intPart = -intPart, fracPart = -fracPart;
      size--, result++;
    }

  if (fracPart > 0)
    {
      while ((fracPart % 10) == 0)
        fracPart /= 10;
    }

  result += snprintf (tempBuffer,
                      size,
                     "%lld.%lld",
                     _SC (long long int, intPart),
                     _SC (long long int, fracPart));

  strcat (_RC (D_CHAR*, utf8Dest), tempBuffer);
  assert (strlen (_RC (D_CHAR*, utf8Dest)) == result);

  if ((result < 0) || (_SC (D_UINT, result) >= maxSize))
    return 0;

  return result + 1;
}
