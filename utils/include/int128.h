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
******************************************************************************/

#ifndef INT128_H_
#define INT128_H_

#include "whisper.h"

#ifndef D_INT128
class WE_I128
{
public:

  WE_I128 ()
  {
  }

  WE_I128 (const WE_I128& source)
  : m_Hi (source.m_Hi),
    m_Lo (source.m_Lo)
  {
  }

  WE_I128 (const D_INT source)
    : m_Hi (0),
      m_Lo (source)
    {
      if (source < 0)
        m_Hi = ~(_SC (D_UINT, 0));
    }

  WE_I128 (const D_UINT source)
    : m_Hi (0),
      m_Lo (source)
    {
    }

  WE_I128 (const D_INT64 source)
    : m_Hi (0),
      m_Lo (source)
    {
      if (source < 0)
        m_Hi = ~(_SC (D_UINT64, 0));
    }

  WE_I128 (const D_UINT64 source)
    : m_Hi (0),
      m_Lo (source)
    {
    }

  WE_I128
  operator- (const WE_I128& op) const
    {
      WE_I128 result (*this);

      if (op.m_Lo > result.m_Lo)
        result.m_Hi--;

      result.m_Lo -= op.m_Lo;
      result.m_Hi -= op.m_Hi;

      return result;
    }

  WE_I128
  operator+ (const WE_I128& op) const
    {
      WE_I128 result (*this);

      result.m_Hi += op.m_Hi;
      result.m_Lo += op.m_Lo;

      if (result.m_Lo < op.m_Lo)
        result.m_Hi++;

      return result;
    }

  WE_I128
  operator- () const
  {
    WE_I128 result (*this);

    result.m_Hi  = ~result.m_Hi;
    result.m_Lo  = ~result.m_Lo;
    result.m_Lo += 1;

    if (m_Lo == 0)
      result.m_Hi += 1;

    return result;
  }

  WE_I128
  operator* (const WE_I128& op) const
    {
      WE_I128 tthis (*this);
      WE_I128 top (op);
      bool    tneg = false;
      bool    oneg = false;

      if (*this < 0)
        {
          tneg  = true;
          tthis = -(*this);
        }

      if (op < 0)
        {
          oneg = true;
          top  = -top;
        }

      if ((top.m_Hi == 0) && (top.m_Lo <= 0xFFFFFFFF))
        tthis = tthis.multiply32 (top.m_Lo);
      else
        tthis = tthis.multiply (top);

      if (tneg ^ oneg)
        return -tthis;

      return tthis;
    }

  WE_I128
  operator/ (const WE_I128& op) const
    {
      WE_I128 quotient;
      WE_I128 reminder;

      WE_I128 tthis (*this);
      WE_I128 top (op);
      bool    tneg = false;
      bool    oneg = false;

      if (*this < 0)
        {
          tneg  = true;
          tthis = - (*this);
        }

      if (op < 0)
        {
          oneg = true;
          top  = -top;
        }

      if ((top.m_Hi == 0)
          && ((tthis.m_Hi == 0)
              || (tthis.m_Hi >= top.m_Lo)
              || (top.m_Lo <= 0xFFFFFFFF)))
        {
          tthis.devide64 (top.m_Lo, quotient, reminder);
        }
      else
        tthis.devide (top, quotient, reminder);

      if (tneg ^ oneg)
        quotient = -quotient;

      return quotient;
    }

  WE_I128
  operator% (const WE_I128& op) const
    {
      WE_I128 quotient;
      WE_I128 reminder;

      WE_I128 tthis (*this);
      WE_I128 top (op);
      bool    tneg = false;

      if (*this < 0)
        {
          tneg  = true;
          tthis = - (*this);
        }

      if (op < 0)
        top  = -top;

      if ((top.m_Hi == 0)
          && ((tthis.m_Hi == 0)
              || (tthis.m_Hi >= top.m_Lo)
              || (top.m_Lo <= 0xFFFFFFFF)))
        {
          tthis.devide64 (top.m_Lo, quotient, reminder);
        }
      else
        tthis.devide (top, quotient, reminder);

      if (tneg)
        reminder = -reminder;

      return reminder;
    }

  WE_I128
  operator| (const WE_I128& op) const
    {
      WE_I128 result = *this;

      result.m_Hi |= op.m_Hi;
      result.m_Lo |= op.m_Lo;

      return result;
    }

  WE_I128
  operator& (const WE_I128& op) const
    {
      WE_I128 result = *this;

      result.m_Hi &= op.m_Hi;
      result.m_Lo &= op.m_Lo;

      return result;
    }

  bool
  operator== (const WE_I128& op) const
  {
    return (m_Hi == op.m_Hi) && (m_Lo == op.m_Lo);
  }

  bool
  operator!= (const WE_I128& op) const
  {
    return ! (*this == op);
  }

  bool
  operator<  (const WE_I128& op) const
  {
    if ((_SC (D_INT64, m_Hi) < 0) && (_SC (D_INT64, op.m_Hi) >= 0))
      return true;
    if ((_SC (D_INT64, m_Hi) > 0) && (_SC (D_INT64, op.m_Hi) <= 0))
      return false;

    return ((m_Hi < op.m_Hi)
            || ((m_Hi == op.m_Hi) && ((m_Lo < op.m_Lo))));
  }

  bool
  operator<=  (const WE_I128& op) const
  {
    return (*this == op) || (*this < op);
  }

  bool
  operator>  (const WE_I128& op) const
  {
    return ! (*this <= op);
  }

  bool
  operator>=  (const WE_I128& op) const
  {
    return ! (*this < op);
  }

  const WE_I128&
  operator+= (const WE_I128& op)
  {
    return *this = *this + op;
  }

  const WE_I128&
  operator-= (const WE_I128& op)
  {
    return *this = *this - op;
  }

  const WE_I128&
  operator*= (const WE_I128& op)
  {
    return *this = *this * op;
  }

  const WE_I128&
  operator/= (const WE_I128& op)
  {
    return *this = *this / op;
  }

  const WE_I128&
  operator%= (const WE_I128& op)
  {
    return *this = *this % op;
  }

  const WE_I128&
  operator|= (const WE_I128& op)
  {
    return *this = *this | op;
  }

  const WE_I128&
  operator&= (const WE_I128& op)
  {
    return *this = *this & op;
  }

private:

  const WE_I128&
  lshift96 ()
  {
    m_Hi = m_Lo;
    m_Hi <<= 32;
    m_Lo = 0;

    return *this;
  }

  const WE_I128&
  lshift64 ()
  {
    m_Hi = m_Lo;
    m_Lo = 0;

    return *this;
  }

  const WE_I128&
  lshift32 ()
  {
    m_Hi <<= 32;
    m_Hi |= (m_Lo >> 32) & 0xFFFFFFFF;
    m_Lo <<= 32;

    return *this;
  }

  const WE_I128&
  lshift ()
  {
    m_Hi <<= 1;
    m_Hi |= (m_Lo >> 63) & 1;
    m_Lo <<= 1;

    return *this;
  }

  const WE_I128&
  rshift ()
  {
    const D_UINT64 one = 1;

    m_Lo = (m_Lo >> 1) & 0x7FFFFFFFFFFFFFFFull;
    m_Lo |= ((m_Hi & 1) << 63);

    const bool negative = _SC(D_INT64, m_Hi) < 0;
    m_Hi = (m_Hi >> 1) & 0x7FFFFFFFFFFFFFFFull;
    if (negative)
      m_Hi |= (one << 63);

    return *this;
  }

  WE_I128
  multiply32 (const D_UINT32 op) const
  {
    WE_I128 temp   = op * (m_Lo & 0xFFFFFFFF);
    WE_I128 result = temp;

    temp    = op * ((m_Lo >> 32) & 0xFFFFFFFF);
    result += temp.lshift32 ();

    temp    = op * (m_Hi & 0xFFFFFFFF);
    result += temp.lshift64();

    temp    = op * ((m_Hi >> 32) & 0xFFFFFFFF);
    result += temp.lshift96 ();

    return result;
  }

  WE_I128
  multiply (const WE_I128& op) const
  {
    const D_UINT32 tw0 = m_Lo & 0xFFFFFFFF;
    const D_UINT32 tw1 = (m_Lo >> 32) & 0xFFFFFFFF;
    const D_UINT32 tw2 = (m_Hi) & 0xFFFFFFFF;
    const D_UINT32 tw3 = (m_Hi >> 32) & 0xFFFFFFFF;

    const D_UINT64 opw0 = op.m_Lo & 0xFFFFFFFF;
    const D_UINT64 opw1 = (op.m_Lo >> 32) & 0xFFFFFFFF;
    const D_UINT64 opw2 = (op.m_Hi) & 0xFFFFFFFF;
    const D_UINT64 opw3 = (op.m_Hi >> 32) & 0xFFFFFFFF;


    WE_I128 temp   = opw0 * tw0;
    WE_I128 result = temp;

    temp    = opw0 * tw1;
    result += temp.lshift32 ();

    temp    = opw0 * tw2;
    result += temp.lshift64 ();

    temp   = opw0 * tw3;
    result += temp.lshift96 ();



    temp    = opw1 * tw0;
    result += temp.lshift32 ();

    temp    = opw1 * tw1;
    result += temp.lshift64 ();

    temp    = opw1 * tw2;
    result += temp.lshift96 ();



    temp    = opw2 * tw0;
    result += temp.lshift64 ();

    temp    = opw2 * tw1;
    result += temp.lshift96 ();



    temp   = opw3 * tw0;
    result += temp.lshift96 ();

    return result;
  }

  void
  devide64 (const D_UINT64 op, WE_I128& quotient, WE_I128& reminder) const
  {
    quotient.m_Hi = m_Hi / op;
    quotient.m_Lo = m_Lo / op;

    reminder.m_Hi = m_Hi % op;
    reminder.m_Lo = m_Lo % op;

    D_UINT64 sq = 0xFFFFFFFFFFFFFFFFul / op;
    D_UINT64 sr = (0xFFFFFFFFFFFFFFFFul % op) + 1;

    if (sr == op)
      ++sq, sr = 0;

    while (reminder.m_Hi > 0)
      {
        WE_I128 temp;

        temp      = reminder.m_Hi;
        temp     *= sq;
        quotient += temp;

        temp  = reminder.m_Hi;
        temp *= sr;

        temp += reminder.m_Lo;

        quotient.m_Hi += temp.m_Hi / op;
        quotient      += temp.m_Lo / op;

        reminder.m_Hi = temp.m_Hi % op;
        reminder.m_Lo = temp.m_Lo % op;
      }
  }

  void
  devide (const WE_I128& op, WE_I128& quotient, WE_I128& reminder) const
  {
    quotient = *this;
    reminder = 0;

    for (D_UINT i = 0; i < 128; ++i)
      {
        reminder.lshift ();
        reminder.m_Lo |= (quotient.m_Hi >> 63) & 1;
        quotient.lshift ();

        if (reminder >= op)
          {
            quotient.m_Lo |= 1;
            reminder -= op;
          }
      }
  }

  D_UINT64  m_Hi;
  D_UINT64  m_Lo;
};

WE_I128
operator+ (const D_INT64 op1, const WE_I128& op2)
{
  return WE_I128 (op2) + op1;
}

WE_I128
operator+ (const D_UINT64 op1, const WE_I128& op2)
{
  return WE_I128 (op2) + op1;
}

WE_I128
operator- (const D_INT64 op1, const WE_I128& op2)
{
  return WE_I128(op1) - op2;
}


WE_I128
operator- (const D_UINT64 op1, const WE_I128& op2)
{
  return WE_I128(op1) - op2;
}

WE_I128
operator* (const D_INT64 op1, const WE_I128& op2)
{
  return WE_I128 (op2) * op1;
}


WE_I128
operator* (const D_UINT64 op1, const WE_I128& op2)
{
  return WE_I128 (op2) * op1;
}

WE_I128
operator/ (const D_INT64 op1, const WE_I128& op2)
{
  return WE_I128(op1) / op2;
}

WE_I128
operator/ (const D_UINT64 op1, const WE_I128& op2)
{
  return WE_I128(op1) / op2;
}

WE_I128
operator% (const D_INT64 op1, const WE_I128& op2)
{
  return WE_I128(op1) % op2;
}

WE_I128
operator% (const D_UINT64 op1, const WE_I128& op2)
{
  return WE_I128(op1) % op2;
}

#else
typedef D_INT128 WE_I128;
#endif /* D_UINT128 */

#endif /* INT128_H_ */
