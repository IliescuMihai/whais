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

#include <assert.h>
#include <string.h>

#include "wlog.h"
#include "statement.h"
#include "vardecl.h"

YYSTYPE
add_idlist (YYSTYPE list, YYSTYPE id)
{
  struct SemId temp;

  assert (id->val_type == VAL_ID);
  assert ((list == NULL) || (list->val_type == VAL_ID_LIST));

  /* transform the id into in an id list's node */
  memcpy (&temp, &(id->val.u_id), sizeof (temp));
  id->val_type = VAL_ID_LIST;
  memcpy (&(id->val.u_idlist.id), &temp, sizeof (temp));

  /*link the list */
  id->val.u_idlist.next = list;

  return id;
}

YYSTYPE
create_type_spec (struct ParserState * state, D_UINT16 type)
{
  struct SemValue *result = get_sem_value (state);
  if (result != NULL)
    {
      result->val_type = VAL_TYPE_SPEC;
      result->val.u_tspec.type = type;
      result->val.u_tspec.extra = NULL;
    }
  else
    {
      w_log_msg (state, IGNORE_BUFFER_POS, MSG_NO_MEM);

    }
  return result;
}

static D_BOOL
process_row_decl (struct ParserState *state,
		  struct DeclaredVar *var, struct SemValue *id_val)
{
  struct Statement *const stmt = state->current_stmt;
  struct SemId *id = (id_val == NULL) ? NULL : (&(id_val->val.u_id));
  D_BOOL result = TRUE;

  assert (id_val == NULL || id_val->val_type == VAL_ID);

  if (id == NULL)
    {
      var->extra = NULL;
    }
  else
    {
      D_CHAR tname[128];
      struct DeclaredVar *table =
	stmt_find_declaration (stmt, id->text, id->length, TRUE);
      if (table == NULL)
	{
	  copy_text_truncate (tname, id->text, sizeof tname, id->length);
	  result = FALSE;
	  w_log_msg (state, state->buffer_pos, MSG_VAR_NFOUND, tname);
	  state->err_sem = TRUE;
	}
      else if (table->type != T_TABLE_MASK)
	{
	  copy_text_truncate (tname, id->text, sizeof tname, id->length);
	  result = FALSE;
	  w_log_msg (state, state->buffer_pos, MSG_NOT_TABLE, tname);
	  state->err_sem = TRUE;
	}
      else
	{
	  var->extra = table;
	}

      /* mark this for reuse */
      id_val->val_type = VAL_REUSE;
    }

  return result;
}

D_BOOL
process_container_decls (struct ParserState * state, struct DeclaredVar * var,
			 void *extra)
{
  D_BOOL result = TRUE;
  switch (var->type)
    {
    case T_ROW_MASK:
      {
	struct SemValue *const ex_row = (struct SemValue *) extra;
	result = process_row_decl (state, var, ex_row);
      }
      break;
    case T_TABLE_MASK:
    case T_RECORD_MASK:
      var->extra = (struct DeclaredVar *) extra;
      break;
    default:
      assert (FALSE);		/* we should not be here */
      w_log_msg (state, IGNORE_BUFFER_POS, MSG_INT_ERR);

      return FALSE;
    }

  return result;
}

struct DeclaredVar *
install_declaration (struct ParserState *state,
		     YYSTYPE sem_var,
		     YYSTYPE sem_type, D_BOOL parameter, D_BOOL unique)
{
  struct DeclaredVar *decl = NULL;
  struct DeclaredVar *result = NULL;
  struct SemId *const id = &(sem_var->val.u_id);
  struct Statement *const stmt = state->current_stmt;

  assert (sem_var->val_type == VAL_ID);
  assert (sem_type->val_type == VAL_TYPE_SPEC);

  if (unique)
    {

      assert ((sem_type->val.u_tspec.type & T_FIELD_MASK) == 0);
      decl = stmt_find_declaration (stmt, id->text, id->length, FALSE);

    }
  else
    {

      assert ((sem_type->val.u_tspec.type & T_FIELD_MASK) != 0);
    }

  if (decl != NULL)
    {
      /* already declared */
      D_CHAR text[128];
      copy_text_truncate (text, decl->label, sizeof text, decl->l_label);
      w_log_msg (state, state->buffer_pos, MSG_VAR_DEFINED, text);

    }
  else
    {
      struct DeclaredVar var;
      var.label = id->text;
      var.l_label = id->length;
      var.type = sem_type->val.u_tspec.type;
      var.extra = NULL;
      var.offset = 0;

      if ((var.type & T_CONTAINER_MASK) &&
	  (process_container_decls (state, &var, sem_type->val.u_tspec.extra)
	   == FALSE))
	{
	  result = NULL;	/* something went wrong along the way */
	}
      else if ((result = stmt_add_declaration (stmt, &var, parameter)) ==
	       NULL)
	{
	  /* no more memory */
	  w_log_msg (state, IGNORE_BUFFER_POS, MSG_NO_MEM);
	  state->err_sem = TRUE;
	}
      else if (((var.type & (T_TABLE_MASK | T_RECORD_MASK)) != 0))
	{
	  /* fix the extra field */
	  struct DeclaredVar *it = result->extra;
	  if (it == NULL)
	    {
	      result->extra = result;
	    }
	  else
	    {
	      while (it->extra && ((it->extra->type & T_FIELD_MASK) != 0))
		{
		  it = it->extra;
		}
	      it->extra = result;
	    }

	}
    }

  if (result && state->extern_decl)
    {
      assert (state->current_stmt == &state->global_stmt);
      result->var_id |= EXTERN_DECLARED;
    }

  return result;
}

YYSTYPE
install_list_declrs (struct ParserState * state, YYSTYPE sem_vars,
		     YYSTYPE sem_type)
{
  struct SemIdList *it;
  YYSTYPE result = NULL;

  assert (sem_vars->val_type == VAL_ID_LIST);
  assert (sem_type->val_type == VAL_TYPE_SPEC);

  it = &sem_vars->val.u_idlist;
  /* we do not need this anymore */
  sem_vars->val_type = VAL_REUSE;

  while (it != NULL)
    {
      struct SemValue id;
      id.val_type = VAL_ID;
      id.val.u_id = it->id;

      if ((result = (YYSTYPE) install_declaration (state, &id,
						   sem_type, FALSE, TRUE))
	  == NULL)
	{
	  break;		/* some error has been encountered */
	}

      /* next in list */
      if (it->next != NULL)
	{
	  assert (it->next->val_type == VAL_ID_LIST);

	  /* mark this as free for reuse */
	  it->next->val_type = VAL_REUSE;

	  it = &(it->next->val.u_idlist);
	}
      else
	{
	  it = NULL;
	}
    }

  /* mark this as free for reuse */
  sem_type->val_type = VAL_REUSE;
  return result;
}

YYSTYPE
install_field_declaration (struct ParserState * state,
			   YYSTYPE sem_var,
			   YYSTYPE sem_type, struct DeclaredVar * const extra)
{
  struct DeclaredVar *result = NULL;
  struct DeclaredVar *it = extra;
  struct SemId *id = &sem_var->val.u_id;

  assert (sem_type->val_type == VAL_TYPE_SPEC);
  assert ((sem_type->val.u_tspec.type & T_FIELD_MASK) != 0);
  assert (sem_var->val_type == VAL_ID);

  /* check for fields with the same name */
  while (it != NULL)
    {
      assert ((it->type & T_FIELD_MASK) != 0);
      if ((it->l_label == id->length) &&
	  (strncmp (it->label, id->text, it->l_label) == 0))
	{
	  D_CHAR tname[128];
	  copy_text_truncate (tname, id->text, sizeof tname, id->length);
	  w_log_msg (state, state->buffer_pos, MSG_SAME_FIELD, tname);
	  state->err_sem = TRUE;
	  return NULL;
	}
      /* next one */
      it = it->extra;
    }
  result = install_declaration (state, sem_var, sem_type, FALSE, FALSE);

  result->extra = extra;
  it = extra;
  while (it != NULL)
    {
      /* Insert this alphabetically to make sure we avoid equivalent fields declarations. */
      if (strncmp (it->label, result->label, it->l_label) >= 0)
        {
          struct DeclaredVar* pExtra = it->extra;

          it->extra = result;
          result->extra = pExtra;

          result = extra;
          break;
        }
      it = it->extra;
    }


  /* mark for reuse */
  sem_var->val_type = VAL_REUSE;
  sem_type->val_type = VAL_REUSE;

  return (YYSTYPE) result;
}
