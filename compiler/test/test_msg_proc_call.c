#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>

#include "../parser/parser.h"
#include "../semantics/vardecl.h"
#include "../semantics/opcodes.h"
#include "../semantics/procdecl.h"
#include "../semantics/wlog.h"

#include "test/test_fmw.h"

extern int yyparse (struct ParserState *);

D_UINT last_msg_code = 0xFF, last_msg_type = 0XFF;

static D_INT
get_buffer_line_from_pos (const char *buffer, D_UINT buff_pos)
{
  D_UINT count = 0;
  D_INT result = 1;

  if (buff_pos == IGNORE_BUFFER_POS)
    {
      return -1;
    }

  while (count < buff_pos)
    {
      if (buffer[count] == '\n')
	{
	  ++result;
	}
      else if (buffer[count] == 0)
	{
	  assert (0);
	}
      ++count;
    }
  return result;
}

static char *MSG_PREFIX[] = {
  "", "error ", "warning ", "error "
};

void
my_postman (POSTMAN_BAG bag,
	    D_UINT buff_pos,
	    D_UINT msg_id,
	    D_UINT msgType, const D_CHAR * msgFormat, va_list args)
{
  const char *buffer = (const char *) bag;
  D_INT buff_line = get_buffer_line_from_pos (buffer, buff_pos);

  if (msgType == MSG_EXTRA_EVENT)
    {
      msg_id = last_msg_code;
      msgType = last_msg_type;
    }
  printf (MSG_PREFIX[msgType]);
  printf ("%d : line %d: ", msg_id, buff_line);
  vprintf (msgFormat, args);
  printf ("\n");

  last_msg_code = msg_id;
  last_msg_type = msgType;
}

D_CHAR test_prog_1[] = ""
  "LET table_1 AS TABLE WITH ( f1 as DATE, f2 as INT16); \n"
  "LET table_2 AS TABLE WITH ( f1 as DATE); \n"
  " \n"
  "PROCEDURE Proc_1 ( row_arg AS ROW OF TABLE table_1) RETURN INT16 \n"
  "DO \n"
  "RETURN row_arg.f2; \n"
  "ENDPROC \n"
  " \n"
  "PROCEDURE Proc_1_2 () RETURN DATE \n"
  "DO \n"
  "LET one_row  AS ROW OF TABLE table_2; \n"
  "Proc_1 ( one_row ); \n" "RETURN one_row.f1; \n" "ENDPROC \n";

D_CHAR test_prog_2[] = ""
  "LET table_2 AS TABLE WITH ( f1 as DATE); \n"
  " \n"
  "PROCEDURE Proc_1_2 () RETURN DATE \n"
  "DO \n"
  "LET one_row  AS ROW OF TABLE table_2; \n"
  "Proc_1 ( one_row ); \n" "RETURN one_row.f1; \n" "ENDPROC \n";

D_CHAR test_prog_3[] = ""
  "LET table_1 AS TABLE WITH ( f1 as DATE, f2 as INT16); \n"
  " \n"
  "PROCEDURE Proc_1 ( row_arg AS ROW OF TABLE table_1) RETURN INT16 \n"
  "DO \n"
  "RETURN row_arg.f2; \n"
  "ENDPROC \n"
  " \n"
  "PROCEDURE Proc_1_2 () RETURN DATE \n"
  "DO \n"
  "LET one_row  AS ROW OF TABLE table_1; \n"
  "LET some_arg AS TEXT; \n"
  "Proc_1 ( one_row, some_arg ); \n" "RETURN one_row.f1; \n" "ENDPROC \n";

D_CHAR test_prog_4[] = ""
  "LET table_1 AS TABLE WITH ( f1 as DATE, f2 as INT16); \n"
  "LET table_2 AS TABLE WITH ( f1 as DATE); \n"
  " \n"
  "PROCEDURE Proc_1 ( row_arg AS ROW OF TABLE table_1) RETURN INT16 \n"
  "DO \n"
  "RETURN row_arg.f2; \n"
  "ENDPROC \n"
  " \n"
  "PROCEDURE Proc_1_2 () RETURN DATE \n"
  "DO \n"
  "LET one_row  AS ROW OF TABLE table_2; \n"
  "Proc_1 ( ); \n" "RETURN one_row.f1; \n" "ENDPROC \n";

D_CHAR test_prog_5[] = ""
  "PROCEDURE Proc_1 ( table_1 AS TABLE WITH ( f1 as DATE, f2 as INT16)) RETURN INT16 \n"
  "DO \n"
  "RETURN table_1[0].f2; \n"
  "ENDPROC \n"
  " \n"
  "PROCEDURE Proc_1_2 () RETURN DATE \n"
  "DO \n"
  "LET one_rec AS RECORD WITH (f1 AS DATE); \n"
  "Proc_1 ( one_rec); \n" "RETURN one_rec.f1; \n" "ENDPROC \n";

D_CHAR test_prog_6[] = ""
  "LET table_2 AS TABLE WITH ( f1 as DATE); \n"
  " \n"
  "PROCEDURE Proc_1 ( table_1 AS TABLE WITH ( f1 as DATE, f2 as INT16)) RETURN INT16 \n"
  "DO \n"
  "RETURN table_1[3].f2; \n"
  "ENDPROC \n"
  " \n"
  "PROCEDURE Proc_1_2 () RETURN DATE \n"
  "DO \n"
  "LET one_row  AS ROW OF TABLE table_2; \n"
  "Proc_1 ( one_row ); \n" "RETURN one_row.f1; \n" "ENDPROC \n";

D_CHAR test_prog_7[] = ""
  "PROCEDURE Proc_1 ( v1 as DATE, v2 as INT16) RETURN INT16 \n"
  "DO \n"
  "RETURN v2; \n"
  "ENDPROC \n"
  " \n"
  "PROCEDURE Proc_1_2 () RETURN DATE \n"
  "DO \n"
  "LET some_var as DATE;"
  "Proc_1 ( some_var, TRUE ); \n" "RETURN some_var; \n" "ENDPROC \n";

D_BOOL
test_for_error (const char *test_buffer, D_UINT err_expected, D_UINT err_type)
{
  WHC_HANDLER handler;
  D_BOOL test_result = TRUE;

  last_msg_code = 0xFF, last_msg_type = 0XFF;
  handler = whc_hnd_create (test_buffer,
			    strlen (test_buffer),
			    &my_postman, (WHC_MESSENGER_ARG) test_buffer);

  if (handler != NULL)
    {
      test_result = FALSE;
      whc_hnd_destroy (handler);
    }
  else
    {
      if ((last_msg_code != err_expected) || (last_msg_type != err_type))
	{
	  test_result = FALSE;
	}
    }

  if (test_get_mem_used () != 0)
    {
      printf ("Current memory usage: %u bytes! It should be 0.",
	      test_get_mem_used ());
      test_result = FALSE;
    }
  return test_result;
}

int
main ()
{
  D_BOOL test_result = TRUE;

  printf ("Testing for received error messages...\n");
  test_result = test_for_error (test_prog_1, MSG_NO_FIELD, MSG_ERROR_EVENT);
  test_result =
    (test_result == FALSE) ? FALSE : test_for_error (test_prog_2, MSG_NO_PROC,
						     MSG_ERROR_EVENT);
  test_result =
    (test_result == FALSE) ? FALSE : test_for_error (test_prog_3,
						     MSG_PROC_MORE_ARGS,
						     MSG_ERROR_EVENT);
  test_result =
    (test_result == FALSE) ? FALSE : test_for_error (test_prog_4,
						     MSG_PROC_LESS_ARGS,
						     MSG_ERROR_EVENT);
  test_result =
    (test_result == FALSE) ? FALSE : test_for_error (test_prog_5,
						     MSG_CONTAINER_NA,
						     MSG_ERROR_EVENT);
  test_result =
    (test_result == FALSE) ? FALSE : test_for_error (test_prog_6,
						     MSG_CONTAINER_NA,
						     MSG_ERROR_EVENT);
  test_result =
    (test_result == FALSE) ? FALSE : test_for_error (test_prog_7,
						     MSG_PROC_ARG_NA,
						     MSG_ERROR_EVENT);
  if (test_result == FALSE)
    {
      printf ("TEST RESULT: FAIL\n");
      return -1;
    }

  printf ("TEST RESULT: PASS\n");
  return 0;
}
