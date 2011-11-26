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
/*
 * parser.h - Declares the types required to manage the semantics objects.
 */

#ifndef PARSER_H
#define PARSER_H

#include "whisper.h"
#include "whisperc/whisperc.h"
#include "utils/include/array.h"
#include "strstore.h"
#include "../semantics/statement.h"

enum SEMVALUE_TYPE
{
  VAL_ERROR = 0,		/* reserved for error situations */
  VAL_ID,

  VAL_C_CHAR,
  VAL_C_INT,
  VAL_C_REAL,
  VAL_C_TEXT,
  VAL_C_TIME,
  VAL_C_BOOL,

  VAL_ID_LIST,
  VAL_TYPE_SPEC,
  VAL_PRCDCL_LIST,
  VAL_PRC_ARG_LINK,

  VAL_EXP_LINK,

  VAL_NULL,
  VAL_REUSE,			/* marked for reuse */
  VAL_UNKNOWN
};

/* represents an identifier */
struct SemId
{
  const char *text;		/* points at the name of identifier */
  D_UINT length;		/* the name's length of the identifier */
};

/* represents a number */
struct SemCInt
{
  D_UINTMAX value;
  D_BOOL is_signed;
};

/*represents a real number */
struct SemCReal
{
  D_INT64 int_part;
  D_UINT64 frac_part;
};

/* represents a string, a text delimited by \" */
struct SemCText
{
  D_CHAR *text;
  D_UINT length;		/* the length of the text including the null terminator */
};

/* represent a character, delimited by \' */
struct SemCChar
{
  D_CHAR value;
};

/* represents a date and time value */
struct SemCTime
{
  D_UINT32 usec;		/* microseconds */
  D_INT16 year;
  D_UINT8 month;
  D_UINT8 day;
  D_UINT8 hour;
  D_UINT8 min;
  D_UINT8 sec;
};

struct SemCBool
{
  D_BOOL value;
};

struct SemIdList
{
  struct SemValue *next;
  struct SemId id;
};

struct SemTypeSpec
{
  void *extra;			/* extra info for container types */
  D_UINT16 type;		/* contains the type specification */
};

struct SemProcParamList
{
  struct SemValue *next;	/* next in list */

  struct SemId id;		/* the id of this parameter */
  struct SemTypeSpec type;	/*type of this parameter */
};

struct SemExpression
{
  struct SemValue *first_op;
  struct SemValue *second_op;

  D_UINT16 op;
};

struct SemProcArgumentsList
{
  struct SemValue *expr;	/* holds the expression tree */
  struct SemValue *next;	/* next argument in list */
};

struct SemValue
{
  /*    D_UINT   buffer_pos; */
  enum SEMVALUE_TYPE val_type;	/* the type of the value */
  union
  {
    struct SemId u_id;

    struct SemCInt u_int;
    struct SemCReal u_real;
    struct SemCText u_text;
    struct SemCChar u_char;
    struct SemCTime u_time;
    struct SemCBool u_bool;

    struct SemIdList u_idlist;
    struct SemTypeSpec u_tspec;
    struct SemProcParamList u_prdcl;
    struct SemProcArgumentsList u_args;

    struct SemExpression u_exp;
  } val;
};

struct ParserState
{
  WHC_MESSENGER_ARG context;
  WHC_MESSENGER msg_callback;

  const D_CHAR *buffer;
  D_UINT buffer_pos;		/* Use this offset to get the next token */
  D_UINT buffer_len;		/* how big the buffer is */
  StringStoreHnd strs;		/* String container to hold constant strings */
  struct UArray vals;		/* Array to store the semantics values parsed */
  struct Statement global_stmt;	/* the global statement */
  struct Statement *current_stmt;
  D_BOOL err_sem;		/* set to true to abort parsing. */
  D_BOOL extern_decl;		/* set to true if the declaration is external */

};

struct SemValue *
get_sem_value (struct ParserState *state);

struct SemValue *
get_bool_sem_value (struct ParserState *state, D_BOOL value);

INLINE static void
recycle_sem_value (struct SemValue *value)
{
  value->val_type = VAL_REUSE;
}

#endif /* PARSER_H */
