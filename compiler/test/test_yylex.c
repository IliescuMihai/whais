#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <math.h>

#define YYSTYPE struct SemValue*
#include "../parser/whisper.tab.h"
#include "../parser/parser.h"
#include "../parser/strstore.h"

extern int yylex (YYSTYPE * lvalp, struct ParserState *state);

const char tokens[] =
  " ArRaY   aS #comment\n  BOOL \t brEak CHARACTER  Continue DATE DATETIME   DO"
  " elSE  \t \n \n ELSEIF  END ENDPROC \n ENDsync\t ExTerN false FOREACH HIrESTIME   IF IN"
  " InT8\n\n\t\tINT16 \n  \t\t\tINT32 INT64 LET OF NULL real RECORD RETURN"
  "\t\t \nRICHREAL      ROW PROCEDURE \nSynC\n\tTABLE \n TEXT THEN tRUE "
  " UNTIL  \n###bau#bau\n#bau\n#hello\n\n#bau again\n   UNSIGNED  wHIle  WITH ";
const int tokens_values[] =
  { ARRAY, AS, BOOL, BREAK, CHARACTER, CONTINUE, DATE, DATETIME, DO,
  ELSE, ELSEIF, END, ENDPROC, ENDSYNC, EXTERN, W_FALSE, FOREACH, HIRESTIME,
    IF, IN,
  INT8,
  INT16, INT32, INT64, LET, OF, WHISPER_NULL, REAL, RECORD, RETURN, RICHREAL,
  ROW, PROCEDURE, SYNC, TABLE, TEXT, THEN, W_TRUE, UNTIL, UNSIGNED, WHILE,
  WITH,
};

static int
test_tokens (void)
{
  int result = 0;
  int count = 0;
  struct ParserState state = { 0, };
  YYSTYPE lvalp;

  state.buffer = tokens;
  state.buffer_len = strlen (tokens);

  printf ("Testing keywords...");
  while ((result = yylex (&lvalp, &state)) != 0)
    {
      if (tokens_values[count++] != result ||
	  get_array_count (&(state.vals)) != 0)
	{
	  printf ("FAIL\n");
	  return -1;
	}
    }
  if ((count != (WITH - ARRAY + 1)) ||
      (count != sizeof (tokens_values) / sizeof (tokens_values[0])) ||
      (state.buffer_pos != sizeof (tokens)))
    {
      printf ("FAIL\n");
      return -2;
    }

  printf ("PASS\n");
  return 0;
}

static char buff_ids[] =
  "\n\t\tmyId id2341 \t __20id\n _48my_id\t\t __0myid_\n\t\noid__\tSupreme_Id";
static struct SemId ids_vals[] = {
  {"myId", 4},
  {"id2341", 6},
  {"__20id", 6},
  {"_48my_id", 8},
  {"__0myid_", 8},
  {"oid__", 5},
  {"Supreme_Id", 10},
  {0, 0}
};

static int
test_buff_ids (void)
{
  int result = 0;
  int count = 0;
  struct ParserState state = { 0, };
  YYSTYPE lvalp;

  state.buffer = buff_ids;
  init_array (&state.vals, sizeof (struct SemValue));
  printf ("Testing identifiers...");
  while ((result = yylex (&lvalp, &state)) != 0)
    {
      if ((result != IDENTIFIER) ||
	  (lvalp->val_type != VAL_ID) ||
	  (lvalp->val.u_id.length != ids_vals[count].length) ||
	  (strncmp (lvalp->val.u_id.text, ids_vals[count].text,
		    ids_vals[count].length) != 0))
	{
	  printf ("FAIL\n");
	  return -1;
	}
      count++;
    }

  if (count != get_array_count (&(state.vals)))
    {
      printf ("FAIL\n");
      return -2;
    }

  printf ("PASS\n");
  return 0;
}

static char buff_integers[] = "18 1 3  0x83 0x4a -200 0XD8 -0x001";
static struct SemCInt int_vals[] = {
  {18}, {1}, {3}, {0x83}, {0x4a}, {-200}, {0XD8}, {-1}
};

static int
test_buff_integers (void)
{
  int result = 0;
  int count = 0;
  struct ParserState state = { 0, };
  YYSTYPE lvalp;

  state.buffer = buff_integers;
  init_array (&state.vals, sizeof (struct SemValue));
  printf ("Testing integers...");
  while ((result = yylex (&lvalp, &state)) != 0)
    {
      if ((result != WHISPER_INTEGER) ||
	  (lvalp->val_type != VAL_C_INT) ||
	  (lvalp->val.u_int.value != int_vals[count].value))
	{
	  printf ("FAIL\n");
	  return -1;
	}
      count++;
    }

  if (count != get_array_count (&(state.vals)))
    {
      printf ("FAIL\n");
      return -2;
    }

  printf ("PASS\n");
  return 0;
}

static char buff_reals[] = " 25.001 0023.41 0.0134 878.0 -2.1 -0.10 91.305";
static struct SemCReal real_vals[] = {
  {25, 0x0010000000000000ull},
  {23, 0x4100000000000000ull},
  {0, 0x0134000000000000ull},
  {878, 0x0000000000000000ull},
  {-2, 0x1000000000000000ull},
  {0, 0x1000000000000000ull},
  {91, 0x3050000000000000ull},
};

static int
test_buff_reals (void)
{
  int result = 0;
  int count = 0;
  struct ParserState state = { 0, };
  YYSTYPE lvalp;

  state.buffer = buff_reals;
  init_array (&state.vals, sizeof (struct SemValue));
  printf ("Testing reals...");
  while ((result = yylex (&lvalp, &state)) != 0)
    {
      if ((result != WHISPER_REAL) ||
	  (lvalp->val_type != VAL_C_REAL) ||
	  (lvalp->val.u_real.int_part != real_vals[count].int_part) ||
	  (lvalp->val.u_real.frac_part != real_vals[count].frac_part))
	{
	  printf ("FAIL\n");
	  return -1;
	}
      count++;
    }

  if (count != get_array_count (&(state.vals)))
    {
      printf ("FAIL\n");
      return -2;
    }

  printf ("PASS\n");
  return 0;
}

static char buff_chars[] =
  " \'\\n\' \'\\r\' \'\\\\\' \'\\f\' \'\\t\' \'\\v\' \'\\b\'"
  " \'\\a\' \'\\\'\' \'\\\"\' \'d\' \'F\' \'\\90\' \'\\0x6\'";
static struct SemCChar char_vals[] = {
  {'\n'}, {'\r'}, {'\\'}, {'\f'}, {'\t'}, {'\v'}, {'\b'},
  {'\a'}, {'\''}, {'\"'}, {'d'}, {'F'}, {'\x5a'}, {'\x6'}
};

static int
test_buff_chars (void)
{
  int result = 0;
  int count = 0;
  struct ParserState state = { 0, };
  YYSTYPE lvalp;

  state.buffer = buff_chars;
  init_array (&state.vals, sizeof (struct SemValue));
  printf ("Testing chars...");
  while ((result = yylex (&lvalp, &state)) != 0)
    {
      if ((result != WHISPER_CHARACTER) ||
	  (lvalp->val_type != VAL_C_CHAR) ||
	  (lvalp->val.u_char.value != char_vals[count].value))
	{
	  printf ("FAIL\n");
	  return -1;
	}
      count++;
    }

  if (count != get_array_count (&(state.vals)))
    {
      printf ("FAIL\n");
      return -2;
    }

  printf ("PASS\n");
  return 0;
}

static char buff_dates[] =
  " \'1998/12/31 04:35:12.9097\' \'-300/09/7\' \'0900/09/10 7:3\'";
static struct SemCTime date_vals[] = {
  {9097, 1998, 12, 31, 4, 35, 12},
  {0, -300, 9, 7,},
  {0, 900, 9, 10, 7, 3,}
};

static int
test_buff_dates (void)
{
  int result = 0;
  int count = 0;
  struct ParserState state = { 0, };
  YYSTYPE lvalp;

  state.buffer = buff_dates;
  init_array (&state.vals, sizeof (struct SemValue));
  printf ("Testing dates...");
  while ((result = yylex (&lvalp, &state)) != 0)
    {
      if ((result != WHISPER_TIME) ||
	  (lvalp->val_type != VAL_C_TIME) ||
	  memcmp (&lvalp->val.u_time,
		  &date_vals[count], sizeof date_vals[0]) != 0)
	{
	  printf ("FAIL\n");
	  return -1;
	}
      count++;
    }

  if (count != get_array_count (&(state.vals)))
    {
      printf ("FAIL\n");
      return -2;
    }

  printf ("PASS\n");
  return 0;
}

static char buff_strs[] = " "
  "\"The first string test in the world \\0x1.\\tThere is nothing you can do!\\n. Booo!\" "
  " "
  "\"Please don't leave \\125 me here! You should work\\rthere is noting to stop you...\"  "
  " " "\"\\\"Please enter your user name and password: \" " " "
  "\"\\0x35\\t\\n\\r What are you doing here? Are you ok?\"" " "
  "\"The incredible long string test. This string should be very very big. The idea is to force "
  "a string test to use more memory than it can normally handle. First let's do a fake zero "
  " because we need to test some weird condition \\0 . Now I thing this string is long enough! \\\\ "
  " La revedere. \"" " " "\"1\"" " " "\"12\"" " " "\"\"";

static char *strs_vals[] = {
  "The first string test in the world \x1.\tThere is nothing you can do!\n. Booo!",
  "Please don't leave \x7d me here! You should work\rthere is noting to stop you...",
  "\"Please enter your user name and password: ",
  "\x35\t\n\r What are you doing here? Are you ok?",
  "The incredible long string test. This string should be very very big. The idea is to force "
    "a string test to use more memory than it can normally handle. First let's do a fake zero "
    " because we need to test some weird condition \x0 . Now I thing this string is long enough! \\ "
    " La revedere. ",
  "1",
  "12",
  ""
};

static int
test_buff_strs (void)
{
  int result = 0;
  int count = 0;
  struct ParserState state = { 0, };
  YYSTYPE lvalp;

  state.buffer = buff_strs;
  init_array (&state.vals, sizeof (struct SemValue));
  state.strs = create_string_store ();
  printf ("Testing string...");
  while ((result = yylex (&lvalp, &state)) != 0)
    {
      if ((result != WHISPER_TEXT) ||
	  (lvalp->val_type != VAL_C_TEXT) ||
	  memcmp (lvalp->val.u_text.text, strs_vals[count],
		  lvalp->val.u_text.length) != 0)
	{
	  printf ("FAIL\n");
	  return -1;
	}
      count++;
    }

  if (count != get_array_count (&(state.vals)))
    {
      printf ("FAIL\n");
      return -2;
    }

  printf ("PASS\n");
  return 0;
}

int
main (void)
{
  D_BOOL test_result = TRUE;
  if ((test_tokens () < 0))
    {
      test_result = FALSE;
    }

  if ((test_buff_ids () < 0))
    {
      test_result = FALSE;
    }

  if ((test_buff_integers () < 0))
    {
      test_result = FALSE;
    }

  if ((test_buff_reals () < 0))
    {
      test_result = FALSE;
    }

  if ((test_buff_chars () < 0))
    {
      test_result = FALSE;
    }

  if ((test_buff_dates () < 0))
    {
      test_result = FALSE;
    }

  if ((test_buff_strs () < 0))
    {
      test_result = FALSE;
    }

  if (test_result == FALSE)
    {
      printf ("TEST RESULT: FAIL\n");
      return -1;
    }

  printf ("TEST RESULT: PASS\n");
  return 0;
}