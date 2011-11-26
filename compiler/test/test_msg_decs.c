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
#include "../include/whisperc/whisperc.h"

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

  printf (MSG_PREFIX[msgType]);
  printf ("%d : line %d: ", msg_id, buff_line);
  vprintf (msgFormat, args);
  printf ("\n");

  last_msg_code = msg_id;
  last_msg_type = msgType;
}

D_CHAR test_prog_1[] = ""
  "PROCEDURE Proc_1 (v2 AS HIRESTIME) RETURN HIRESTIME\n "
  "DO\n " "RETURN --v2;\n " "ENDPROC\n ";

D_CHAR test_prog_2[] = ""
  "PROCEDURE Proc_1 (v2 AS RICHREAL) RETURN RICHREAL\n "
  "DO\n " "RETURN --v2;\n " "ENDPROC\n ";

D_CHAR test_prog_3[] = ""
  "PROCEDURE Proc_2 (v2 AS INT16) RETURN INT16\n "
  "DO\n " "RETURN --(v2 + 1);\n " "ENDPROC\n ";

D_CHAR test_prog_4[] = ""
  "PROCEDURE Proc_3 (v2 AS INT16) RETURN INT16\n "
  "DO\n " "RETURN --(2 + 1);\n " "ENDPROC\n " "\n ";

D_CHAR test_prog_5[] = ""
  "PROCEDURE Proc_4 (v2 AS INT32) RETURN INT32\n "
  "DO\n "
  "LET v3 AS INT8;\n " "v3 = 1;\n " "RETURN --(v2 + v3);\n " "ENDPROC\n ";

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
  test_result = test_for_error (test_prog_1, MSG_DEC_NA, MSG_ERROR_EVENT);
  test_result =
    (test_result == FALSE) ? FALSE : test_for_error (test_prog_2, MSG_DEC_NA,
						     MSG_ERROR_EVENT);
  test_result =
    (test_result == FALSE) ? FALSE : test_for_error (test_prog_3, MSG_DEC_ELV,
						     MSG_ERROR_EVENT);
  test_result =
    (test_result == FALSE) ? FALSE : test_for_error (test_prog_4, MSG_DEC_ELV,
						     MSG_ERROR_EVENT);
  test_result =
    (test_result == FALSE) ? FALSE : test_for_error (test_prog_5, MSG_DEC_ELV,
						     MSG_ERROR_EVENT);
  if (test_result == FALSE)
    {
      printf ("TEST RESULT: FAIL\n");
      return -1;
    }

  printf ("TEST RESULT: PASS\n");
  return 0;
}
