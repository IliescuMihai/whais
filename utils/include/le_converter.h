/******************************************************************************
WHISPERC - A compiler for whisper programs
Copyright (C) 2009  Iulian Popa

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

#ifndef LE_CONVERTER_H_
#define LE_CONVERTER_H_

#include "utils_shl.h"

#ifdef __cplusplus
#undef INLINE
#define INLINE inline
#endif

static INLINE void
to_le_int16 (D_UINT8* pInOutValue)
{
  D_UINT8        temp;
  const D_UINT16 le_machine = 0x1;

  if (*((D_UINT8 *) & le_machine))
    return;     /* no need for conversion */

  temp           = pInOutValue[0];
  pInOutValue[0] = pInOutValue[1];
  pInOutValue[1] = temp;
}

static INLINE void
to_le_int32 (D_UINT8* pInOutValue)
{
  D_UINT8        temp;
  const D_UINT16 le_machine = 0x1;

  if (*((D_UINT8 *) & le_machine))
    return;     /* no need for conversion */

  temp           = pInOutValue[0];
  pInOutValue[0] = pInOutValue[3];
  pInOutValue[3] = temp;
  temp           = pInOutValue[1];
  pInOutValue[1] = pInOutValue[2];
  pInOutValue[2] = temp;
}

static INLINE void
to_le_int64 (D_UINT8* pInOutValue)
{
  D_UINT8        temp;
  const D_UINT16 le_machine = 0x1;


  if (*((D_UINT8 *) & le_machine))
    return;     /* no need for conversion */

  temp           = pInOutValue[0];
  pInOutValue[0] = pInOutValue[7];
  pInOutValue[7] = temp;
  temp           = pInOutValue[1];
  pInOutValue[1] = pInOutValue[6];
  pInOutValue[6] = temp;
  temp           = pInOutValue[2];
  pInOutValue[2] = pInOutValue[5];
  pInOutValue[5] = temp;
  temp           = pInOutValue[3];
  pInOutValue[3] = pInOutValue[4];
  pInOutValue[4] = temp;
}

static INLINE D_UINT16
from_le_int16 (const D_UINT8* pValue)
{
  D_UINT16 result;

  result = pValue[1];
  result <<= 8;
  result += pValue[0];
  return result;
}

static INLINE D_UINT32
from_le_int32 (const D_UINT8* pValue)
{
  D_UINT32 result;

  result = pValue[3];
  result <<= 8;
  result += pValue[2];
  result <<= 8;
  result += pValue[1];
  result <<= 8;
  result += pValue[0];

  return result;
}

static INLINE D_UINT64
from_le_int64 (const D_UINT8* pValue)
{
  D_UINT64 result;

  result = pValue[7];
  result <<= 8;
  result += pValue[6];
  result <<= 8;
  result += pValue[5];
  result <<= 8;
  result += pValue[4];
  result <<= 8;
  result += pValue[3];
  result <<= 8;
  result += pValue[2];
  result <<= 8;
  result += pValue[1];
  result <<= 8;
  result += pValue[0];

  return result;
}

#endif /* LE_CONVERTER_H_ */
