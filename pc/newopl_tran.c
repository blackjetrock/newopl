///////////////////////////////////////////////////////////////////////////////
//
// NewOPL Translater
//
// Translates NewOPL to byte code.
//
// Processes files: reads file, translates it and writes bytecode file.
//
////////////////////////////////////////////////////////////////////////////////

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include "nopl.h"
#include "newopl.h"
#include "nopl_obj.h"

#include "parser.h"

FILE *icfp;
FILE *ofp;
FILE *chkfp;
FILE *trfp;

// Reads next composite line into buffer

char current_expression[200];
int first_token = 1;

OP_STACK_ENTRY op_stack[NOPL_MAX_OP_STACK+1];

int op_stack_ptr = 0;


NOBJ_VARTYPE exp_type_stack[MAX_EXP_TYPE_STACK];
int exp_type_stack_ptr = 0;

#define SAVE_I     1
#define NO_SAVE_I  0

////////////////////////////////////////////////////////////////////////////////

// Per-expression
// Indices start at 1, 0 is 'no p'
int node_id_index = 1;

EXP_BUFFER_ENTRY exp_buffer[MAX_EXP_BUFFER];
int exp_buffer_i = 0;

EXP_BUFFER_ENTRY exp_buffer2[MAX_EXP_BUFFER];
int exp_buffer2_i = 0;

////////////////////////////////////////////////////////////////////////////////
//
// Translate a file
//
// Lines are one of the following formats:
//
// All lines are treated as expressions.
//
// E.g.
//
// PRINT A%
//
// PRINT is a function.
//
// A% = 23
//
// '=' is a function (that maps to qcode)
//
// A% = SIN(45)
//
// SIN is a function. As (45) is an expression
//
// SIN 45 is also vaid (but not in original OPL
//
//
// Tokens are strings, delimited by spaces and also:
//
// (),:
//
//
// 1 when line defines the procedure (i.e. the first line)
//
////////////////////////////////////////////////////////////////////////////////

int token_is_operator(char *token, char **tokstr);

void modify_expression_type(NOBJ_VARTYPE t);
void op_stack_display(void);
void op_stack_print(void);
char type_to_char(NOBJ_VARTYPE t);
char access_to_char(NOPL_OP_ACCESS a);
NOBJ_VARTYPE char_to_type(char ch);
void dump_exp_buffer(FILE *fp, int bufnum);

////////////////////////////////////////////////////////////////////////////////
//
// Markers used as comments, and hints
//
////////////////////////////////////////////////////////////////////////////////

//#define dbprintf(fmt,...) dbpf(__FUNCTION__, fmt, ...)
#define MAX_INDENT 30

int indent_level = 0;
char indent_str[MAX_INDENT+1];

void indent_none(void)
{
  indent_level = 0;
}

void indent_more(void)
{
  if( indent_level <(MAX_INDENT-1) )
    {
      indent_level++;
    }
  fprintf(ofp, "\n");
}

#define MAX_DBPF_LINE 160

void dbpf(const char *caller, char *fmt, ...)
{
  va_list valist;
  char line[MAX_DBPF_LINE];
  
  va_start(valist, fmt);

  vsnprintf(line,  MAX_DBPF_LINE, fmt, valist);
  va_end(valist);
  
  indent_str[0] = '\0';
  
  for(int i=0; i<indent_level; i++)
    {
      strcat(indent_str, " ");
    }
  
  fprintf(ofp, "\n%s(%s) %s", indent_str, caller, line);
  fflush(ofp);
  
    if( (strstr(line, "ret0") != NULL) || (strstr(line, "ret1") != NULL) )
    {

      if( indent_level != 0 )
	{
	  indent_level--;
	}
    }
}

int find_op_info(char *name, OP_INFO *op)
{
  for(int i=0; i<NUM_OPERATORS; i++)
    {
      if( strcmp(op_info[i].name, name) == 0 )
	{
	  *op = op_info[i];
	  return(1);
	}
    }
  
  return(0);
}

// Is type one of those in the list?
int is_a_valid_type(NOBJ_VARTYPE type, OP_INFO *op_info)
{
  for(int i=0; i<MAX_OPERATOR_TYPES; i++)
    {
      if( op_info->type[i] == type )
	{
	  // It is in the list
	  return(1);
	}
    }

  // Not in list
  return(0);
}

////////////////////////////////////////////////////////////////////////////////
//
// Expression type is reset for each line and also for sub-lines separated by colons
//
// This is a global to avoid passing it down to every function in the translate call stack.
// If translating is ever to be a parallel process then that will have to change.
//

NOBJ_VARTYPE expression_type = NOBJ_VARTYPE_UNKNOWN;


////////////////////////////////////////////////////////////////////////////////


int defline = 1;

int token_is_float(char *token)
{
  int all_digits = 1;
  int decimal_present = 0;
  int retval;
  int len = strlen(token);
  
  for(int i=0; i < len; i++)
    {
      if( !( isdigit(*token) || (*token == '.') || (*token == '-') ))
	{
	  all_digits = 0;
	}

      if( *token == '.' )
	{
	  decimal_present = 1;
	}
      
      token++;
    }

  retval = all_digits && decimal_present;

  return(retval);
}

int token_is_integer(char *token)
{
  int all_digits = 1;
  int len = strlen(token);

  dbprintf(" tok:'%s'", token);

  if( *token == '-' )
    {
      token++;
      len--;
    }
  
  for(int i=0; i<len; i++)
    {
      if( !isdigit(*(token)) )
	{
	  all_digits = 0;
	}

      token++;
    }

  dbprintf(" tok:ret%d", all_digits);
  return(all_digits);
}

////////////////////////////////////////////////////////////////////////////////

// Variable if it ends in $ or %
// Variable if not a function name
// Variables have to be only alpha or alpha followed by alphanum

int token_is_variable(char *token)
{
  int is_var = 0;
  char *tokptr;

  fprintf(ofp, "\n%s: tok:'%s'", __FUNCTION__, token);
  
  if( token_is_operator(token, &tokptr) )
    {
      return(0);
    }

  if( token_is_function(token, &tokptr) )
    {
      return(0);
    }

  // First char has to be alpha

  if( !isalpha(*token) )
    {
      return(0);
    }

  for(int i=0; i<strlen(token)-1; i++)
    {
      if( ! (isalnum(token[i]) || token[i] == '.') )
	{
	  return(0);
	}
    }

  char last_char = token[strlen(token)-1];

  switch(last_char)
    {
    case '$':
    case '%':
      return(1);
      break;
    }

  return(1);
}

int token_is_string(char *token)
{
  return( *token == '"' );
}


////////////////////////////////////////////////////////////////////////////////
//
// tokstr is a constant string that we use in the operator stack
// to minimise memory usage.
//
////////////////////////////////////////////////////////////////////////////////

int token_is_operator(char *token, char **tokstr)
{
  for(int i=0; i<NUM_OPERATORS; i++)
    {
      if( strcmp(token, op_info[i].name) == 0 )
	{
	  *tokstr = &(op_info[i].name[0]);
	  
	  fprintf(ofp, "\n'%s' is operator", token);
	  return(1);
	}
    }
  
  return(0);
}

////////////////////////////////////////////////////////////////////////////////
//
// returns the precedence of the token. This comes from the operator info table for operators,
// functions are set the precedence of 0
//

int operator_precedence(char *token)
{
  for(int i=0; i<NUM_OPERATORS; i++)
    {
      if( strcmp(token, op_info[i].name) == 0 )
	{
	  dbprintf("\n%s is operator", token);
	  return(op_info[i].precedence);
	}
    }
  
  return(100);
}

int operator_left_assoc(char *token)
{
  for(int i=0; i<NUM_OPERATORS; i++)
    {
      if( strcmp(token, op_info[i].name) == 0 )
	{
	  dbprintf("\n%s is operator", token);
	  return(op_info[i].left_assoc);
	}
    }
  
  return(0);
}

//------------------------------------------------------------------------------

// Turn operator into unary version if possible

void operator_can_be_unary(OP_STACK_ENTRY *op)
{
  for(int i=0; i<NUM_OPERATORS; i++)
    {
      if( strcmp(op->name, op_info[i].name) == 0 )
	{
	  // Convert to unary if it can be
	  if( op_info[i].can_be_unary )
	    {
	      op->buf_id = EXP_BUFF_ID_OPERATOR_UNARY;
	      strcpy(op->name, op_info[i].unaryop);
	      dbprintf("Operator converted to unary. Now '%s'", op->name);
	      return;
	    }
	}
    }
  
  return;
}

////////////////////////////////////////////////////////////////////////////////
//
// Table with simple BUFF_ID to QCode mappings
//

typedef struct _SIMPLE_QC_MAP
{
  int            buf_id;
  char           *name;     // Name of OP
  NOBJ_VARTYPE   optype;    // Type of OP
  NOPL_VAR_CLASS class;
  NOBJ_VARTYPE   type;      // Type of variable
  NOPL_OP_ACCESS access;
  int            qcode;
} SIMPLE_QC_MAP;

// A value that will never match
#define __  200


SIMPLE_QC_MAP qc_map[] =
  {
    {EXP_BUFF_ID_VARIABLE, "",    __, NOPL_VAR_CLASS_EXTERNAL,  NOBJ_VARTYPE_INT,    NOPL_OP_ACCESS_READ, QI_INT_SIM_IND},
    {EXP_BUFF_ID_VARIABLE, "",    __, NOPL_VAR_CLASS_EXTERNAL,  NOBJ_VARTYPE_FLT,    NOPL_OP_ACCESS_READ, QI_NUM_SIM_IND},
    {EXP_BUFF_ID_VARIABLE, "",    __, NOPL_VAR_CLASS_EXTERNAL,  NOBJ_VARTYPE_STR,    NOPL_OP_ACCESS_READ, QI_STR_SIM_IND},
    {EXP_BUFF_ID_VARIABLE, "",    __, NOPL_VAR_CLASS_EXTERNAL,  NOBJ_VARTYPE_INTARY, NOPL_OP_ACCESS_READ, QI_INT_ARR_IND},
    {EXP_BUFF_ID_VARIABLE, "",    __, NOPL_VAR_CLASS_EXTERNAL,  NOBJ_VARTYPE_FLTARY, NOPL_OP_ACCESS_READ, QI_NUM_ARR_IND},
    {EXP_BUFF_ID_VARIABLE, "",    __, NOPL_VAR_CLASS_EXTERNAL,  NOBJ_VARTYPE_STRARY, NOPL_OP_ACCESS_READ, QI_STR_ARR_IND},
    {EXP_BUFF_ID_VARIABLE, "",    __, NOPL_VAR_CLASS_PARAMETER, NOBJ_VARTYPE_INT,    NOPL_OP_ACCESS_READ, QI_INT_SIM_IND},
    {EXP_BUFF_ID_VARIABLE, "",    __, NOPL_VAR_CLASS_PARAMETER, NOBJ_VARTYPE_FLT,    NOPL_OP_ACCESS_READ, QI_NUM_SIM_IND},
    {EXP_BUFF_ID_VARIABLE, "",    __, NOPL_VAR_CLASS_PARAMETER, NOBJ_VARTYPE_STR,    NOPL_OP_ACCESS_READ, QI_STR_SIM_IND},
    {EXP_BUFF_ID_VARIABLE, "",    __, NOPL_VAR_CLASS_PARAMETER, NOBJ_VARTYPE_INTARY, NOPL_OP_ACCESS_READ, QI_INT_ARR_IND},
    {EXP_BUFF_ID_VARIABLE, "",    __, NOPL_VAR_CLASS_PARAMETER, NOBJ_VARTYPE_FLTARY, NOPL_OP_ACCESS_READ, QI_NUM_ARR_IND},
    {EXP_BUFF_ID_VARIABLE, "",    __, NOPL_VAR_CLASS_PARAMETER, NOBJ_VARTYPE_STRARY, NOPL_OP_ACCESS_READ, QI_STR_ARR_IND},
    {EXP_BUFF_ID_VARIABLE, "",    __, NOPL_VAR_CLASS_GLOBAL,    NOBJ_VARTYPE_INT,       NOPL_OP_ACCESS_READ, QI_INT_SIM_FP},
    {EXP_BUFF_ID_VARIABLE, "",    __, NOPL_VAR_CLASS_GLOBAL,    NOBJ_VARTYPE_FLT,       NOPL_OP_ACCESS_READ, QI_NUM_SIM_FP},
    {EXP_BUFF_ID_VARIABLE, "",    __, NOPL_VAR_CLASS_GLOBAL,    NOBJ_VARTYPE_STR,       NOPL_OP_ACCESS_READ, QI_STR_SIM_FP},
    {EXP_BUFF_ID_VARIABLE, "",    __, NOPL_VAR_CLASS_GLOBAL,    NOBJ_VARTYPE_INTARY,    NOPL_OP_ACCESS_READ, QI_INT_ARR_FP},
    {EXP_BUFF_ID_VARIABLE, "",    __, NOPL_VAR_CLASS_GLOBAL,    NOBJ_VARTYPE_FLTARY,    NOPL_OP_ACCESS_READ, QI_NUM_ARR_FP},
    {EXP_BUFF_ID_VARIABLE, "",    __, NOPL_VAR_CLASS_GLOBAL,    NOBJ_VARTYPE_STRARY,    NOPL_OP_ACCESS_READ, QI_STR_ARR_FP},
    {EXP_BUFF_ID_VARIABLE, "",    __, NOPL_VAR_CLASS_LOCAL,     NOBJ_VARTYPE_INT,        NOPL_OP_ACCESS_READ, QI_INT_SIM_FP},
    {EXP_BUFF_ID_VARIABLE, "",    __, NOPL_VAR_CLASS_LOCAL,     NOBJ_VARTYPE_FLT,        NOPL_OP_ACCESS_READ, QI_NUM_SIM_FP},
    {EXP_BUFF_ID_VARIABLE, "",    __, NOPL_VAR_CLASS_LOCAL,     NOBJ_VARTYPE_STR,        NOPL_OP_ACCESS_READ, QI_STR_SIM_FP},
    {EXP_BUFF_ID_VARIABLE, "",    __, NOPL_VAR_CLASS_LOCAL,     NOBJ_VARTYPE_INTARY,     NOPL_OP_ACCESS_READ, QI_INT_ARR_FP},
    {EXP_BUFF_ID_VARIABLE, "",    __, NOPL_VAR_CLASS_LOCAL,     NOBJ_VARTYPE_FLTARY,     NOPL_OP_ACCESS_READ, QI_NUM_ARR_FP},
    {EXP_BUFF_ID_VARIABLE, "",    __, NOPL_VAR_CLASS_LOCAL,     NOBJ_VARTYPE_STRARY,     NOPL_OP_ACCESS_READ, QI_STR_ARR_FP},
    {EXP_BUFF_ID_VARIABLE, "",    __, NOPL_VAR_CLASS_EXTERNAL,  NOBJ_VARTYPE_INT,    NOPL_OP_ACCESS_WRITE, QI_LS_INT_SIM_IND},
    {EXP_BUFF_ID_VARIABLE, "",    __, NOPL_VAR_CLASS_EXTERNAL,  NOBJ_VARTYPE_FLT,    NOPL_OP_ACCESS_WRITE, QI_LS_NUM_SIM_IND},
    {EXP_BUFF_ID_VARIABLE, "",    __, NOPL_VAR_CLASS_EXTERNAL,  NOBJ_VARTYPE_STR,    NOPL_OP_ACCESS_WRITE, QI_LS_STR_SIM_IND},
    {EXP_BUFF_ID_VARIABLE, "",    __, NOPL_VAR_CLASS_EXTERNAL,  NOBJ_VARTYPE_INTARY, NOPL_OP_ACCESS_WRITE, QI_LS_INT_ARR_IND},
    {EXP_BUFF_ID_VARIABLE, "",    __, NOPL_VAR_CLASS_EXTERNAL,  NOBJ_VARTYPE_FLTARY, NOPL_OP_ACCESS_WRITE, QI_LS_NUM_ARR_IND},
    {EXP_BUFF_ID_VARIABLE, "",    __, NOPL_VAR_CLASS_EXTERNAL,  NOBJ_VARTYPE_STRARY, NOPL_OP_ACCESS_WRITE, QI_LS_STR_ARR_IND},
    {EXP_BUFF_ID_VARIABLE, "",    __, NOPL_VAR_CLASS_PARAMETER, NOBJ_VARTYPE_INT,    NOPL_OP_ACCESS_WRITE, QI_LS_INT_SIM_IND},
    {EXP_BUFF_ID_VARIABLE, "",    __, NOPL_VAR_CLASS_PARAMETER, NOBJ_VARTYPE_FLT,    NOPL_OP_ACCESS_WRITE, QI_LS_NUM_SIM_IND},
    {EXP_BUFF_ID_VARIABLE, "",    __, NOPL_VAR_CLASS_PARAMETER, NOBJ_VARTYPE_STR,    NOPL_OP_ACCESS_WRITE, QI_LS_STR_SIM_IND},
    {EXP_BUFF_ID_VARIABLE, "",    __, NOPL_VAR_CLASS_PARAMETER, NOBJ_VARTYPE_INTARY, NOPL_OP_ACCESS_WRITE, QI_LS_INT_ARR_IND},
    {EXP_BUFF_ID_VARIABLE, "",    __, NOPL_VAR_CLASS_PARAMETER, NOBJ_VARTYPE_FLTARY, NOPL_OP_ACCESS_WRITE, QI_LS_NUM_ARR_IND},
    {EXP_BUFF_ID_VARIABLE, "",    __, NOPL_VAR_CLASS_PARAMETER, NOBJ_VARTYPE_STRARY, NOPL_OP_ACCESS_WRITE, QI_LS_STR_ARR_IND},
    {EXP_BUFF_ID_VARIABLE, "",    __, NOPL_VAR_CLASS_GLOBAL,    NOBJ_VARTYPE_INT,       NOPL_OP_ACCESS_WRITE, QI_LS_INT_SIM_FP},
    {EXP_BUFF_ID_VARIABLE, "",    __, NOPL_VAR_CLASS_GLOBAL,    NOBJ_VARTYPE_FLT,       NOPL_OP_ACCESS_WRITE, QI_LS_NUM_SIM_FP},
    {EXP_BUFF_ID_VARIABLE, "",    __, NOPL_VAR_CLASS_GLOBAL,    NOBJ_VARTYPE_STR,       NOPL_OP_ACCESS_WRITE, QI_LS_STR_SIM_FP},
    {EXP_BUFF_ID_VARIABLE, "",    __, NOPL_VAR_CLASS_GLOBAL,    NOBJ_VARTYPE_INTARY,    NOPL_OP_ACCESS_WRITE, QI_LS_INT_ARR_FP},
    {EXP_BUFF_ID_VARIABLE, "",    __, NOPL_VAR_CLASS_GLOBAL,    NOBJ_VARTYPE_FLTARY,    NOPL_OP_ACCESS_WRITE, QI_LS_NUM_ARR_FP},
    {EXP_BUFF_ID_VARIABLE, "",    __, NOPL_VAR_CLASS_GLOBAL,    NOBJ_VARTYPE_STRARY,    NOPL_OP_ACCESS_WRITE, QI_LS_STR_ARR_FP},
    {EXP_BUFF_ID_VARIABLE, "",    __, NOPL_VAR_CLASS_LOCAL,     NOBJ_VARTYPE_INT,        NOPL_OP_ACCESS_WRITE, QI_LS_INT_SIM_FP},
    {EXP_BUFF_ID_VARIABLE, "",    __, NOPL_VAR_CLASS_LOCAL,     NOBJ_VARTYPE_FLT,        NOPL_OP_ACCESS_WRITE, QI_LS_NUM_SIM_FP},
    {EXP_BUFF_ID_VARIABLE, "",    __, NOPL_VAR_CLASS_LOCAL,     NOBJ_VARTYPE_STR,        NOPL_OP_ACCESS_WRITE, QI_LS_STR_SIM_FP},
    {EXP_BUFF_ID_VARIABLE, "",    __, NOPL_VAR_CLASS_LOCAL,     NOBJ_VARTYPE_INTARY,     NOPL_OP_ACCESS_WRITE, QI_LS_INT_ARR_FP},
    {EXP_BUFF_ID_VARIABLE, "",    __, NOPL_VAR_CLASS_LOCAL,     NOBJ_VARTYPE_FLTARY,     NOPL_OP_ACCESS_WRITE, QI_LS_NUM_ARR_FP},
    {EXP_BUFF_ID_VARIABLE, "",    __, NOPL_VAR_CLASS_LOCAL,     NOBJ_VARTYPE_STRARY,     NOPL_OP_ACCESS_WRITE, QI_LS_STR_ARR_FP},
    {EXP_BUFF_ID_AUTOCON,  "",    NOBJ_VARTYPE_INT,     __,                      __,     __,               QCO_NUM_TO_INT},
    {EXP_BUFF_ID_AUTOCON,  "",    NOBJ_VARTYPE_FLT,     __,                      __,     __,               QCO_INT_TO_NUM},
    {EXP_BUFF_ID_OPERATOR, ":=",  NOBJ_VARTYPE_INT,     __,                   __,        __,               QCO_ASS_INT},
    {EXP_BUFF_ID_OPERATOR, ":=",  NOBJ_VARTYPE_FLT,     __,                   __,        __,               QCO_ASS_NUM},
    {EXP_BUFF_ID_OPERATOR, ":=",  NOBJ_VARTYPE_STR,     __,                   __,        __,               QCO_ASS_STR},
    {EXP_BUFF_ID_OPERATOR, "=",   NOBJ_VARTYPE_INT,     __,                   __,        __,               QCO_EQ_INT},
    {EXP_BUFF_ID_OPERATOR, "=",   NOBJ_VARTYPE_FLT,     __,                   __,        __,               QCO_EQ_NUM},
    {EXP_BUFF_ID_OPERATOR, "=",   NOBJ_VARTYPE_STR,     __,                   __,        __,               QCO_EQ_STR},
    {EXP_BUFF_ID_FUNCTION, "AT",                __,     __,                   __,        __,               QCO_AT},
    {EXP_BUFF_ID_FUNCTION, "PAUSE",             __,     __,                   __,        __,               QCO_PAUSE},
    {EXP_BUFF_ID_FUNCTION, "KEY",               __,     __,                   __,        __,               RTF_KEY},
    {EXP_BUFF_ID_FUNCTION, "IABS", NOBJ_VARTYPE_INT,    __,                   __,        __,               RTF_IABS},
    {EXP_BUFF_ID_FUNCTION, "ABS", NOBJ_VARTYPE_FLT,     __,                   __,        __,               RTF_ABS},
    {EXP_BUFF_ID_OPERATOR, "<",   NOBJ_VARTYPE_INT,     __,                   __,        __,               QCO_LT_INT},
    {EXP_BUFF_ID_OPERATOR, "<",   NOBJ_VARTYPE_FLT,     __,                   __,        __,               QCO_LT_NUM},
    {EXP_BUFF_ID_OPERATOR, "<",   NOBJ_VARTYPE_STR,     __,                   __,        __,               QCO_LT_STR},
    {EXP_BUFF_ID_OPERATOR, "<=",  NOBJ_VARTYPE_INT,     __,                   __,        __,               QCO_LTE_INT},
    {EXP_BUFF_ID_OPERATOR, "<=",  NOBJ_VARTYPE_FLT,     __,                   __,        __,               QCO_LTE_NUM},
    {EXP_BUFF_ID_OPERATOR, "<=",  NOBJ_VARTYPE_STR,     __,                   __,        __,               QCO_LTE_STR},
    {EXP_BUFF_ID_OPERATOR, ">",   NOBJ_VARTYPE_INT,     __,                   __,        __,               QCO_GT_INT},
    {EXP_BUFF_ID_OPERATOR, ">",   NOBJ_VARTYPE_FLT,     __,                   __,        __,               QCO_GT_NUM},
    {EXP_BUFF_ID_OPERATOR, ">",   NOBJ_VARTYPE_STR,     __,                   __,        __,               QCO_GT_STR},
    {EXP_BUFF_ID_OPERATOR, ">=",  NOBJ_VARTYPE_INT,     __,                   __,        __,               QCO_GTE_INT},
    {EXP_BUFF_ID_OPERATOR, ">=",  NOBJ_VARTYPE_FLT,     __,                   __,        __,               QCO_GTE_NUM},
    {EXP_BUFF_ID_OPERATOR, ">=",  NOBJ_VARTYPE_STR,     __,                   __,        __,               QCO_GTE_STR},
    {EXP_BUFF_ID_OPERATOR, "<>",  NOBJ_VARTYPE_INT,     __,                   __,        __,               QCO_NE_INT},
    {EXP_BUFF_ID_OPERATOR, "<>",  NOBJ_VARTYPE_FLT,     __,                   __,        __,               QCO_NE_NUM},
    {EXP_BUFF_ID_OPERATOR, "<>",  NOBJ_VARTYPE_STR,     __,                   __,        __,               QCO_NE_STR},
    {EXP_BUFF_ID_OPERATOR, "=",   NOBJ_VARTYPE_INT,     __,                   __,        __,               QCO_EQ_INT},
    {EXP_BUFF_ID_OPERATOR, "=",   NOBJ_VARTYPE_FLT,     __,                   __,        __,               QCO_EQ_NUM},
    {EXP_BUFF_ID_OPERATOR, "=",   NOBJ_VARTYPE_STR,     __,                   __,        __,               QCO_EQ_STR},
    {EXP_BUFF_ID_OPERATOR, "+",   NOBJ_VARTYPE_INT,     __,                   __,        __,               QCO_ADD_INT},
    {EXP_BUFF_ID_OPERATOR, "+",   NOBJ_VARTYPE_FLT,     __,                   __,        __,               QCO_ADD_NUM},
    {EXP_BUFF_ID_OPERATOR, "+",   NOBJ_VARTYPE_STR,     __,                   __,        __,               QCO_ADD_STR},
    {EXP_BUFF_ID_OPERATOR, "-",   NOBJ_VARTYPE_INT,     __,                   __,        __,               QCO_SUB_INT},
    {EXP_BUFF_ID_OPERATOR, "-",   NOBJ_VARTYPE_FLT,     __,                   __,        __,               QCO_SUB_NUM},
    {EXP_BUFF_ID_OPERATOR, "*",   NOBJ_VARTYPE_INT,     __,                   __,        __,               QCO_MUL_INT},
    {EXP_BUFF_ID_OPERATOR, "*",   NOBJ_VARTYPE_FLT,     __,                   __,        __,               QCO_MUL_NUM},
    {EXP_BUFF_ID_OPERATOR, "/",   NOBJ_VARTYPE_INT,     __,                   __,        __,               QCO_DIV_INT},
    {EXP_BUFF_ID_OPERATOR, "/",   NOBJ_VARTYPE_FLT,     __,                   __,        __,               QCO_DIV_NUM},
    {EXP_BUFF_ID_OPERATOR, "**",  NOBJ_VARTYPE_INT,     __,                   __,        __,               QCO_POW_INT},
    {EXP_BUFF_ID_OPERATOR, "**",  NOBJ_VARTYPE_FLT,     __,                   __,        __,               QCO_POW_NUM},
    {EXP_BUFF_ID_OPERATOR, "UMIN",NOBJ_VARTYPE_INT,     __,                   __,        __,               QCO_UMIN_INT},
    {EXP_BUFF_ID_OPERATOR, "UMIN",NOBJ_VARTYPE_FLT,     __,                   __,        __,               QCO_UMIN_NUM},
    {EXP_BUFF_ID_OPERATOR, "NOT", NOBJ_VARTYPE_INT,     __,                   __,        __,               QCO_NOT_INT},
    {EXP_BUFF_ID_OPERATOR, "NOT", NOBJ_VARTYPE_FLT,     __,                   __,        __,               QCO_NOT_NUM},
    {EXP_BUFF_ID_OPERATOR, "AND", NOBJ_VARTYPE_INT,     __,                   __,        __,               QCO_AND_INT},
    {EXP_BUFF_ID_OPERATOR, "AND", NOBJ_VARTYPE_FLT,     __,                   __,        __,               QCO_AND_NUM},
    {EXP_BUFF_ID_OPERATOR, "OR",  NOBJ_VARTYPE_INT,     __,                   __,        __,               QCO_OR_INT},
    {EXP_BUFF_ID_OPERATOR, "OR",  NOBJ_VARTYPE_FLT,     __,                   __,        __,               QCO_OR_NUM},


  };

#define NUM_SIMPLE_QC_MAP (sizeof(qc_map)/sizeof(SIMPLE_QC_MAP))

int add_simple_qcode(int idx, OP_STACK_ENTRY *op, NOBJ_VAR_INFO *vi)
{
  int op_type = NOBJ_VARTYPE_UNKNOWN;
  
  for(int i=0; i<NUM_SIMPLE_QC_MAP; i++)
    {
      if( op->buf_id == qc_map[i].buf_id )
	{
	  // Some operators should have their types forced to the qcode_type as that is based on their inputs.
	  // the '.type' field is based on the output and is used for typechecking.
	  if( op->qcode_type != NOBJ_VARTYPE_UNKNOWN )
	    {
	      op_type = op->qcode_type;
	    }
	  else
	    {
	      op_type = op->type;
	    }
	  
	  // See if other values match
	  if( ((qc_map[i].class  == vi->class)         || (qc_map[i].class == __))   &&
	      ((qc_map[i].type   == vi->type)          || (qc_map[i].type == __)) &&
	      ((qc_map[i].optype == op_type)           || (qc_map[i].optype == __)) &&
	      ((qc_map[i].access == op->access)        || (qc_map[i].access == __)) &&
	      (((strcmp(qc_map[i].name, op->name) == 0) && (strlen(qc_map[i].name) != 0)) || (strlen(qc_map[i].name) == 0))
	      )
	    {
	      // We have a match and a qcode
	      return(set_qcode_header_byte_at(idx, 1, qc_map[i].qcode));
	    }
	}
    }
  return(idx);
}

////////////////////////////////////////////////////////////////////////////////
//
// Conditional branch offset fixups

typedef struct _COND_FIXUP_ENTRY
{
  int idx;
  int buf_id;       // The type of point in the qcode that this is
  int level;
} COND_FIXUP_ENTRY;

#define MAX_COND_FIXUP 100

COND_FIXUP_ENTRY cond_fixup[MAX_COND_FIXUP];
int cond_fixup_i = 0;

void add_cond_fixup(int idx, int buf_id, int level)
{
  if( cond_fixup_i < MAX_COND_FIXUP-1 )
    {
      cond_fixup[cond_fixup_i].idx    = idx;
      cond_fixup[cond_fixup_i].buf_id = buf_id;
      cond_fixup[cond_fixup_i].level  = level;
      cond_fixup_i++;
    }
  else
    {
      internal_error("Too many conditionals");
    }
}

//------------------------------------------------------------------------------
//
// Find an index given a level and buf_id

int find_idx(int buf_id, int level)
{
  for(int i=0; i<cond_fixup_i; i++)
    {
      if( (buf_id == cond_fixup[i].buf_id) && (level == cond_fixup[i].level) )
	{
	  return(cond_fixup[i].idx);
	}
    }
  return(0);
}

//------------------------------------------------------------------------------
//
// Fix up conditional branches
//

void do_cond_fixup(void)
{
  for(int i=0; i<cond_fixup_i; i++)
    {
      switch(cond_fixup[i].buf_id)
	{
	case EXP_BUFF_ID_UNTIL:
	  // Find matching DO and get idx
	  int do_idx = find_idx(EXP_BUFF_ID_DO, cond_fixup[i].level);

	  // Calculate offset
	  int branch_offset = (do_idx - cond_fixup[i].idx);
	  int until_offset = branch_offset;
	  
	  // Fill in the offset
	  set_qcode_header_byte_at(cond_fixup[i].idx+0, 1, (until_offset) >> 8);
	  set_qcode_header_byte_at(cond_fixup[i].idx+1, 1, (until_offset) & 0xFF);
	  break;
	}
    }
}


////////////////////////////////////////////////////////////////////////////////
//
// QCode output stream
//
//
//
// The text based tokens from the output stream are converted here to the final
// qcodes
//
// Most codes are translated directly
// Variable offsets have to be inserted instea dof variable names.
// Tables of variables are maintained
//
// Fixed size tables for variables
//
// This is called after each line is processed, That generates enough intermediate
// code to generate QCode for one line.
//
// NOTE:
// As we want to reduce the amount of RAM needed to generate Qcode, we want to generate
// code for a line at a time so we don't have to hold all the intermediate code for
// the entire PROC in memory at once. One problem is that we need to genrate the output
// (OB3) header before the QCode can be generated. We need variable offsets and types
// before the qcode is created. This is fine for the locals and globals and parameters as
// they are always declared at the start of the PROC. Externals aren't, however, we find
// them as we translate the PROC lines. That means we can't generate the qcode until we
// have translated and found al the externals. To get around this the translation is done twice.
// The first pass is to collect the externals, although error will also be found then. The entire
// variable table is built and kept for the second pass.
// The second pass then generates the header and qcode, as it has all the information it needs.
// There's a pass number that tells the code what it needs to do

typedef struct _VAR_INFO
{
  char name[NOBJ_VARNAME_MAXLEN];
  int is_array;
  int is_integer;
  int is_float;
  int is_string;
  int max_array;
  int max_string;
  NOBJ_VARTYPE type;
  uint16_t offset;    // Offset from FP
} VAR_INFO;

VAR_INFO local_info[NOPL_MAX_LOCAL];
VAR_INFO global_info[NOPL_MAX_GLOBAL];

int local_info_index  = 0;
int global_info_index = 0;

void output_qcode_for_line(void)
{
  // Do nothing on first pass
  if( pass_number == 1 )
    {
      return;
    }
  
  dump_exp_buffer(icfp, 2);

  //------------------------------------------------------------------------------
  // We are now able to append qcode to the qcode output
  // Run through the exp_buffer and convert the tokens into QCode...
  //------------------------------------------------------------------------------
  
  dbprintf("================================================================================");
  dbprintf("Generating QCode     Buf2_i:%d qcode_idx:%04X", exp_buffer2_i, qcode_idx);
  dbprintf("================================================================================");

  // Things need to be detected so qcode can be generated earlier than the normal token generation
  // would have created them.
  int elseif_present = 0;
  
  for(int i=0; i<exp_buffer2_i; i++)
    {
      if(  exp_buffer2[i].op.buf_id == EXP_BUFF_ID_ELSEIF )
	{
	  elseif_present = 1;
	}
    }
  
  for(int i=0; i<exp_buffer2_i; i++)
    {
      EXP_BUFFER_ENTRY token = exp_buffer2[i];
#define tokop token.op
      
      dbprintf("QC: i:%d", i);
      
      if( (exp_buffer2[i].op.buf_id < 0) || (exp_buffer2[i].op.buf_id > EXP_BUFF_ID_MAX) )
	{
	  dbprintf("N%d buf_id invalid", token.node_id);
	}
      
      switch(token.op.buf_id)
	{

	case EXP_BUFF_ID_META:
	  // On pass 2 when we see the PROCDEF we generate the qcode header,
	  // each line then generates qcodes after that
	  dbprintf("QC:META %s", tokop.name);
	  
	  if( pass_number == 2 )
	    {
	      if( strcmp(exp_buffer[i].name, "PROCDEF")==0 )
		{
		  dbprintf("QC:Building QCode header");
		  qcode_idx = 0;
		  build_qcode_header();
		  fprintf(icfp, "Header built qcode_idx:%04X", qcode_idx);
		  //		  exit(0);
		}

	      // Do not process line fuirther
	      return;
	    }
	  break;

	  // If there's a return keyword then there would have been a return value that should have been stacked.
	  // The qcode for this situation is  QCO_RETURN
	  // If there's no RETURN then a code that stacks a zero/null has to be added once the translating ends.
	case EXP_BUFF_ID_RETURN:
	  procedure_has_return = 1;
	  
	  if( token.op.access == NOPL_OP_ACCESS_EXP )
	    {
	      qcode_idx = set_qcode_header_byte_at(qcode_idx, 1,  QCO_RETURN);
	    }
	  else
	    {
	      switch(procedure_type)
		{
		case NOBJ_VARTYPE_INT:
		  qcode_idx = set_qcode_header_byte_at(qcode_idx, 1,  QCO_RETURN_NOUGHT);
		  break;

		case NOBJ_VARTYPE_FLT:
		  qcode_idx = set_qcode_header_byte_at(qcode_idx, 1,  QCO_RETURN_ZERO);
		  break;

		case NOBJ_VARTYPE_STR:
		  qcode_idx = set_qcode_header_byte_at(qcode_idx, 1,  QCO_RETURN_NULL);
		  break;
		}
	    }
	  break;
	  
	case EXP_BUFF_ID_VARIABLE:
	  // Find the info about this variable
	  NOBJ_VAR_INFO *vi;

	  vi = find_var_info(tokop.name);

	  qcode_idx = add_simple_qcode(qcode_idx, &(token.op), vi);
	  qcode_idx = set_qcode_header_byte_at(qcode_idx, 1, (vi->offset) >> 8);
	  qcode_idx = set_qcode_header_byte_at(qcode_idx, 1, (vi->offset) & 0xFF);
	  break;
	  
	case EXP_BUFF_ID_INTEGER:
	  qcode_idx = set_qcode_header_byte_at(qcode_idx, 1, QI_INT_CON);

	  // Convert integer and add to qcode
	  int intval;

	  sscanf(tokop.name, "%d", &intval);
	  
	  qcode_idx = set_qcode_header_byte_at(qcode_idx, 1, (intval) >> 8);
	  qcode_idx = set_qcode_header_byte_at(qcode_idx, 1, (intval) & 0xFF);
	  break;
	  
	case EXP_BUFF_ID_PRINT_NEWLINE:
	  dbprintf("QC:PRINT");
	  qcode_idx = set_qcode_header_byte_at(qcode_idx, 1, QCO_PRINT_CR);
	  break;

	case EXP_BUFF_ID_PRINT_SPACE:
	  dbprintf("QC:PRINT");
	  qcode_idx = set_qcode_header_byte_at(qcode_idx, 1, QCO_PRINT_SP);
	  break;
	  
	case EXP_BUFF_ID_PRINT:
	  dbprintf("QC:PRINT");
	  switch(token.op.type)
	    {
	    case NOBJ_VARTYPE_INT:
	      qcode_idx = set_qcode_header_byte_at(qcode_idx, 1, QCO_PRINT_INT);
	      break;

	    case NOBJ_VARTYPE_FLT:
	      qcode_idx = set_qcode_header_byte_at(qcode_idx, 1, QCO_PRINT_NUM);
	      break;

	    case NOBJ_VARTYPE_STR:
	      qcode_idx = set_qcode_header_byte_at(qcode_idx, 1, QCO_PRINT_STR);
	      break;
	    }
	  break;

	case EXP_BUFF_ID_UNTIL:
	case EXP_BUFF_ID_IF:
	  // We put zero in as a dummy jump offset and add it to the conditionals fixup table
	  qcode_idx = set_qcode_header_byte_at(qcode_idx, 1, QCO_BRA_FALSE);
	  add_cond_fixup(qcode_idx, token.op.buf_id, token.op.level);
	  qcode_idx = set_qcode_header_byte_at(qcode_idx, 1, 0x00);
	  qcode_idx = set_qcode_header_byte_at(qcode_idx, 1, 0x00);
	  
	  break;

	case EXP_BUFF_ID_DO:
	  // No Qcode, we just create a point where UNTIL can branch back to
	  add_cond_fixup(qcode_idx, token.op.buf_id, token.op.level);
	  break;
	  
	case EXP_BUFF_ID_STR:
	  // String literal
	  dbprintf("\nQC:String Literal");
	  
	  qcode_idx = set_qcode_header_byte_at(qcode_idx, 1, QI_STR_CON);
	  qcode_idx = set_qcode_header_byte_at(qcode_idx, 1, strlen(exp_buffer[i].name)-2);
	  
	  for(int j=1; j<strlen(exp_buffer2[i].name)-1; j++)
	    {
	      qcode_idx = set_qcode_header_byte_at(qcode_idx, 1, exp_buffer[i].name[j]);
	    }
	  break;

	default:
	  // Check the simple mapping table
	  qcode_idx = add_simple_qcode(qcode_idx, &(token.op), vi);
	  break;
	}
    }

  qcode_len = qcode_idx - qcode_start_idx;
}

void output_qcode_suffix(void)
{
  switch(procedure_type)
    {
    case NOBJ_VARTYPE_INT:
      qcode_idx = set_qcode_header_byte_at(qcode_idx, 1,  QCO_RETURN_NOUGHT);
      break;
      
    case NOBJ_VARTYPE_FLT:
      qcode_idx = set_qcode_header_byte_at(qcode_idx, 1,  QCO_RETURN_ZERO);
      break;
      
    case NOBJ_VARTYPE_STR:
      qcode_idx = set_qcode_header_byte_at(qcode_idx, 1,  QCO_RETURN_NULL);
      break;
    }

  qcode_len = qcode_idx - qcode_start_idx;
}


////////////////////////////////////////////////////////////////////////////////
//
// Variable name parsing
//
// The type of the variable is determined.
//

char *gns_ptr;
char gn_str[NOBJ_VARNAME_MAXLEN];

void init_get_name(char *s)
{
  gns_ptr = s;
  dbprintf("\n%s:'%s'", __FUNCTION__, gns_ptr);
  
  // Skip leading spaces
  while( ((*gns_ptr) != '\0') && isspace(*gns_ptr) )
    {
      gns_ptr++;
    }

  dbprintf("\n%s:'%s'", __FUNCTION__, gns_ptr);
}


char *get_name(char *n, NOBJ_VARTYPE *t)
{
  int i = 0;
  
  while (*gns_ptr != '\0')
    {
      gn_str[i++] = *gns_ptr;
      switch(*gns_ptr)
	{
	case '$':
	  *t = NOBJ_VARTYPE_STR;
	  gn_str[i] = '\0';
	  dbprintf("\n%s:gn:'%s'", __FUNCTION__, gn_str);
	  return(gn_str);
	  break;

	case '%':

	  *t = NOBJ_VARTYPE_INT;
	  gn_str[i] = '\0';
	  dbprintf("\n%s:gn:'%s'", __FUNCTION__, gn_str);
	  return(gn_str);
	  break;
	  
	case ',':
	  break;

	default:
	  
	  break;
	}

      gns_ptr++;
    }

  gn_str[i] = '\0';
  dbprintf("\n%s:gn:'%s'", __FUNCTION__, gn_str);
  *t = NOBJ_VARTYPE_FLT;
  return(gn_str);
}

void output_qcode_variable(char *def)
{
  char vname[NOBJ_VARNAME_MAXLEN];
  NOBJ_VARTYPE type;
  
  dbprintf("\n%s: %s", __FUNCTION__, def);

  if( strstr(def, "GLOBAL") != NULL )
    {
#if 0
      // Get variable names
      init_get_name(def);

      if( get_name(vname, &type) )
	{
	  modify_expression_type(type);
	  type = expression_type;
	}
#endif
    }

  if( strstr(def, "LOCAL") != NULL )
    {
    }

  
}

////////////////////////////////////////////////////////////////////////////////

char type_to_char(NOBJ_VARTYPE t)
{
  char c;
  
  switch(t)
    {
    case NOBJ_VARTYPE_INT:
      c = 'i';
      break;

    case NOBJ_VARTYPE_FLT:
      c = 'f';
      break;

    case NOBJ_VARTYPE_STR:
      c = 's';
      break;

    case NOBJ_VARTYPE_INTARY:
      c = 'I';
      break;

    case NOBJ_VARTYPE_FLTARY:
      c = 'F';
      break;

    case NOBJ_VARTYPE_STRARY:
      c = 'S';
      break;

    case NOBJ_VARTYPE_VAR_ADDR:
      c = 'V';
      break;

    case NOBJ_VARTYPE_UNKNOWN:
      c = 'U';
      break;

    case NOBJ_VARTYPE_VOID:
      c = 'v';
      break;
      
    default:
      c = '?';
      break;
    }
  
  return(c);
}

//------------------------------------------------------------------------------

char access_to_char(NOPL_OP_ACCESS a)
{
  char c;
  
  switch(a)
    {
    case NOPL_OP_ACCESS_UNKNOWN:
      c = 'U';
      break;

    case NOPL_OP_ACCESS_READ:
      c = 'R';
      break;

    case NOPL_OP_ACCESS_EXP:
      c = 'X';
      break;

    case NOPL_OP_ACCESS_NO_EXP:
      c = 'x';
      break;
      
    default:
      c = '?';
      break;
    }
  
  return(c);
}


//------------------------------------------------------------------------------
//
// A new token has appeared in an expression. Modify the expression based on
// this new type.
//
//

void modify_expression_type(NOBJ_VARTYPE t)
{
  fprintf(ofp, "\n%s:Inittype:%c", __FUNCTION__, type_to_char(expression_type));
  switch(expression_type)
    {
    case NOBJ_VARTYPE_UNKNOWN:
      expression_type = t;
      break;

    case NOBJ_VARTYPE_INT:
      switch(t)
	{
	case NOBJ_VARTYPE_INT:
	  break;
	  
	case NOBJ_VARTYPE_FLT:
	  // Move to float type
	  expression_type = t;
	  break;

	case NOBJ_VARTYPE_STR:
	  // Syntax error
	  break;
	}
      break;

    case NOBJ_VARTYPE_FLT:
      switch(t)
	{
	case NOBJ_VARTYPE_INT:
	  // Type conversion
	  break;
	  
	case NOBJ_VARTYPE_FLT:
	  break;

	case NOBJ_VARTYPE_STR:
	  // Syntax error
	  break;
	}
      break;

    case NOBJ_VARTYPE_STR:
      switch(t)
	{
	case NOBJ_VARTYPE_INT:
	case NOBJ_VARTYPE_FLT:
	  // Syntax error
	  //expression_type = t;
	  break;

	case NOBJ_VARTYPE_STR:
	  expression_type = t;
	  break;
	}
      break;
    }
  
  fprintf(ofp, " Intype:%c Outtype:%c", type_to_char(t), type_to_char(expression_type));
}

////////////////////////////////////////////////////////////////////////////////
//
// Output expression processing
//
// Each expression is placed in this buffer and processed to create the
// corresponding QCode.
//
// The buffer holds RPN codes with functions and operands in text form. This is
// executed as RPN on a stack, but with no function. This is done to check the
// typing of the functions and operands, and also as a general way to insert the
// automatic type conversion codes.
//
// The reason that this is done is that the operands for an operator may be
// arbitrarily far from the operator.
//
// For instance:
//
//     B = 2 * (3 * ( 4 + (1 + 1)))
//
//  is the following in RPN:
//
//
//    B
//    2      <=== First operand
//    3
//    4
//    1
//    1
//    +
//    +
//    *
//    *      <==== operator
//    =
//
// From this you can see that the first operand of the * operator is
// many entries in the stack away from the operator. Working out if an auto
// conversion code should be inserted when that operand is emitted to the qcode
// stream requires a look-ahead of many stack entries. Executing the code as RPN allows the
// positions of auto conversion codes to be determined in a general way.
//
// 
////////////////////////////////////////////////////////////////////////////////
//
//
// Typechecking execution stack
//

void type_check_stack_print(void);

#define MAX_TYPE_CHECK_STACK  40

EXP_BUFFER_ENTRY type_check_stack[MAX_TYPE_CHECK_STACK];
int type_check_stack_ptr = 0;

void type_check_stack_push(EXP_BUFFER_ENTRY entry)
{
  fprintf(ofp, "\n%s: '%s'", __FUNCTION__, entry.name);
  
  if( type_check_stack_ptr < MAX_TYPE_CHECK_STACK )
    {
      type_check_stack[type_check_stack_ptr++] = entry;
    }
  else
    {
      fprintf(ofp, "\n%s: Operator stack full", __FUNCTION__);
      typecheck_error("Stack full");
      n_stack_errors++;
    }
  type_check_stack_print();

}

// Copies data into string

EXP_BUFFER_ENTRY type_check_stack_pop(void)
{
  EXP_BUFFER_ENTRY o;
  
  if( type_check_stack_ptr == 0 )
    {
      fprintf(ofp, "\n%s: Operator stack empty", __FUNCTION__);
      typecheck_error("Stack empty");
      n_stack_errors++;
      return(o);
    }
  
  type_check_stack_ptr --;

  o = type_check_stack[type_check_stack_ptr];
  
  fprintf(ofp, "\n%s: '%s'", __FUNCTION__, o.name);
  type_check_stack_print();
  return(o);
}

void type_check_stack_display(void)
{
  char *s;
  NOBJ_VARTYPE type;
  
  dbprintf("Type Check Stack (%d)", type_check_stack_ptr);

  for(int i=0; i<type_check_stack_ptr; i++)
    {
      s = type_check_stack[i].name;
      type = type_check_stack[i].op.type;
      
      dbprintf("%03d: '%s' type:%c (%d)", i, s, type_to_char(type), type);
    }
}

void type_check_stack_print(void)
{
  char *s;

  dbprintf("\n------------------");
  dbprintf("\nType Check Stack     (%d)\n", type_check_stack_ptr);
  
  for(int i=0; i<type_check_stack_ptr; i++)
    {
      s = type_check_stack[i].name;
      dbprintf("\n%03d: '%s' type:%d", i, s, type_check_stack[i].op.type);
    }

  dbprintf("\n------------------\n");
}

void type_check_stack_init(void)
{
  type_check_stack_ptr = 0;
}



//------------------------------------------------------------------------------
//

void clear_exp_buffer(void)
{
  exp_buffer_i = 0;
}

void add_exp_buffer_entry(OP_STACK_ENTRY op, int id)
{
  exp_buffer[exp_buffer_i].op = op;
  exp_buffer[exp_buffer_i].op.buf_id = id;
  strcpy(&(exp_buffer[exp_buffer_i].name[0]), op.name);
  exp_buffer_i++;
}

void add_exp_buffer2_entry(OP_STACK_ENTRY op, int id)
{
  exp_buffer2[exp_buffer2_i].op = op;
  exp_buffer2[exp_buffer2_i].op.buf_id = id;
  strcpy(&(exp_buffer2[exp_buffer2_i].name[0]), op.name);
  exp_buffer2_i++;
}

////////////////////////////////////////////////////////////////////////////////

void dump_exp_buffer(FILE *fp, int bufnum)
{
  char *idstr;
  int exp_buff_len = 0;
  char levstr[10];
  
  switch(bufnum)
    {
    case 1:
      exp_buff_len = exp_buffer_i;
      break;
      
    case 2:
      exp_buff_len = exp_buffer2_i;
      break;
    }

  if( exp_buff_len == 0 )
    {
      return;
    }

  for(int i=0; i<exp_buff_len; i++)
    {
      EXP_BUFFER_ENTRY token;

	switch(bufnum)
	  {
	  case 1:
	    token = exp_buffer[i];
	    break;

	  case 2:
	    token = exp_buffer2[i];
	    break;
	  }

	if( token.op.level == 0 )
	  {
	    sprintf(levstr, "  %5s", "");
	  }
	else
	  {
	    sprintf(levstr, "L:%-5d", token.op.level);
	  }

	
	fprintf(fp, "\n(%16s) N%d %c %-30s %s ty:%c rq:%c qcty:%c '%s' npar:%d nidx:%d",
		__FUNCTION__,
		token.node_id,
		access_to_char(token.op.access),
		exp_buffer_id_str[token.op.buf_id],
		levstr,
		type_to_char(token.op.type),
		type_to_char(token.op.req_type),
		type_to_char(token.op.qcode_type),
		token.name,
		token.op.num_parameters,
		token.op.vi.num_indices);
      
      fprintf(fp, "  %d:", token.p_idx);

      for(int pi=0; pi<token.p_idx; pi++)
	{
	  fprintf(fp, " %d", token.p[pi]);
	}

      fprintf(fp, "  nb %d:(", token.op.num_bytes);
      
      for(int i=0; (i<token.op.num_bytes) & (i < NOPL_MAX_SUFFIX_BYTES); i++)
	{
	  fprintf(fp, " %02X", token.op.bytes[i]);
	}
      fprintf(fp, ")");
      
    }

  fprintf(fp, "\n");
}

////////////////////////////////////////////////////////////////////////////////
//
// Expression tree
//
// Once we have an RPN version of the expression we then build a tree. This
// allows the auto type conversion to be done in a general way by traversing
// the tree looking for type conflict and inserting auto conversion nodes.
//
//
////////////////////////////////////////////////////////////////////////////////

// Insert a buffer2 entry into the list, after the entry with the given
// node_id.

int insert_buf2_entry_after_node_id(int node_id, EXP_BUFFER_ENTRY e)
{
  int j;

  
  dump_exp_buffer(ofp, 2);

  // Find the entry with the given node_id
  fprintf(ofp, "\n Insert after %d exp_buffer2_i:%d", node_id, exp_buffer2_i);
  
  for(int i= 0; i<exp_buffer2_i; i++)
    {
      if( exp_buffer2[i].node_id == node_id )
	{
	  fprintf(ofp, "\n   Found at i:%d", i);
	  
	  // Found the entry, move all after it on by one entry
	  for(j=exp_buffer2_i; j>=i+2; j--)
	    {
	      exp_buffer2[j] = exp_buffer2[j-1];
	      fprintf(ofp, "\n   Copied %d to %d:", j-1, j);
	    }
	  
	  exp_buffer2[i+1] = e;
	  exp_buffer2_i++;

	  dump_exp_buffer(ofp, 2);
	  return(1);	  
	}
    }
  
  return(0);
}

////////////////////////////////////////////////////////////////////////////////
//
// Return a type that can be reached by as few type conversions as possible.
//
////////////////////////////////////////////////////////////////////////////////


NOBJ_VARTYPE type_with_least_conversion_from(NOBJ_VARTYPE t1, NOBJ_VARTYPE t2)
{
  NOBJ_VARTYPE ret = t1;

  if( (t1 == NOBJ_VARTYPE_INT) && (t2 == NOBJ_VARTYPE_INT) )
    {
      ret = NOBJ_VARTYPE_INT;
    }

  if( (t1 == NOBJ_VARTYPE_FLT) && (t2 == NOBJ_VARTYPE_FLT) )
    {
      ret = NOBJ_VARTYPE_FLT;
    }

  if( (t1 == NOBJ_VARTYPE_FLT) && (t2 == NOBJ_VARTYPE_INT) )
    {
      ret = NOBJ_VARTYPE_FLT;
    }

  if( (t1 == NOBJ_VARTYPE_INT) && (t2 == NOBJ_VARTYPE_FLT) )
    {
      ret = NOBJ_VARTYPE_FLT;
    }

  fprintf(ofp, "\n%s: %c %c => %c",  __FUNCTION__, type_to_char(t1), type_to_char(t2), type_to_char(ret));
  return(ret);
}

////////////////////////////////////////////////////////////////////////////////
//
// Convert exp_buffer into an infix expression
//
////////////////////////////////////////////////////////////////////////////////
//
// This is useful for checking the translator.
//
////////////////////////////////////////////////////////////////////////////////

#define MAX_INFIX_STACK 500
#define MAX_INFIX_STR   400

char infix_stack[MAX_INFIX_STACK][MAX_INFIX_STR];
int infix_stack_ptr = 0;
char result[MAX_INFIX_STR*20];

void infix_stack_push(char *entry)
{
  fprintf(ofp, "\n%s: '%s'", __FUNCTION__, entry);
  
  if( infix_stack_ptr < MAX_INFIX_STACK )
    {
      strcpy(infix_stack[infix_stack_ptr++], entry);
    }
  else
    {
      fprintf(ofp, "\n%s: Operator stack full", __FUNCTION__);
      typecheck_error("Operator stack full");
      return;
    }
}

// Copies data into string

void infix_stack_pop(char *entry)
{
  EXP_BUFFER_ENTRY o;
  
  if( infix_stack_ptr == 0 )
    {
      fprintf(ofp, "\n%s: Operator stack empty", __FUNCTION__);
      typecheck_error("Operator stack empty");
      return;
    }
  
  infix_stack_ptr --;

  strcpy(entry, infix_stack[infix_stack_ptr]);
  
  fprintf(ofp, "\n%s: '%s'", __FUNCTION__, entry);
}

////////////////////////////////////////////////////////////////////////////////

char *infix_from_rpn(void)
{
  EXP_BUFFER_ENTRY be;
  char op1[MAX_INFIX_STR], op2[MAX_INFIX_STR];
  char newstr[MAX_INFIX_STR*20+5];

  char newstr2[MAX_INFIX_STR*20+5];
  int numarg;
  
  infix_stack_ptr = 0;
  
  for(int i=0; i<exp_buffer2_i; i++)
    {
      
      be = exp_buffer2[i];

      dbprintf("(%s)", be.name);
      
      switch(be.op.buf_id)
	{
	case EXP_BUFF_ID_VAR_ADDR_NAME:
	  fprintf(ofp, "\nVar ADDR Name: %s", be.name);
	  infix_stack_push(be.name);
	  break;
	  
	case EXP_BUFF_ID_VARIABLE:
	  fprintf(ofp, "\nVar: %s %s NumIdx:%d", be.name,
		  type_to_str(be.op.vi.type),
		  be.op.vi.num_indices);

	  if( var_type_is_array(be.op.vi.type) )
	    {
	      // Pop the number of array indices the variable has off the stack
	      // and build the variable name plus indices.
	      strcpy(newstr2, be.name);
	      strcat(newstr2, "(");

	      switch(be.op.vi.num_indices)
		{
		case 1:
		  infix_stack_pop(op1);
		  strcat(newstr2, op1);
		  break;

		case 2:
		  infix_stack_pop(op1);
		  infix_stack_pop(op2);
		  strcat(newstr2, op2);
		  strcat(newstr2, ",");
		  strcat(newstr2, op1);
		  break;
		}
	      
	      strcat(newstr2, ")");
	      infix_stack_push(newstr2);
	    }
	  else
	    {
	      // Non array variables are just pushed on the stack
	      infix_stack_push(be.name);
	    }
	  break;

	  // Just put the name in the outout
	case EXP_BUFF_ID_UNTIL:
	case EXP_BUFF_ID_ELSEIF:
	case EXP_BUFF_ID_WHILE:
	case EXP_BUFF_ID_IF:
	  infix_stack_pop(op1);
	  fprintf(ofp, "\n%s", be.name);
	  snprintf(newstr2, MAX_INFIX_STR, "%s %s", be.name, op1);
	  infix_stack_push(newstr2);
	  break;

	case EXP_BUFF_ID_DO:
	case EXP_BUFF_ID_TRAP:
	case EXP_BUFF_ID_ELSE:
	case EXP_BUFF_ID_ENDIF:
	case EXP_BUFF_ID_ENDWH:
	  dbprintf("%s", be.name);
	  snprintf(newstr2, MAX_INFIX_STR, "%s", be.name);
	  infix_stack_push(newstr2);
	  dbprintf("endif done");
	  break;
	  
	case EXP_BUFF_ID_PRINT:
	case EXP_BUFF_ID_LPRINT:
	  
	  fprintf(ofp, "\n%s", be.name);

	  // Pop what we will print
	  infix_stack_pop(op1);

	  snprintf(newstr2, MAX_INFIX_STR, "%s(%s)", be.name, op1);
	  infix_stack_push(newstr2);

	  break;

	case EXP_BUFF_ID_PRINT_SPACE:
	  infix_stack_push("< > ");
	  break;

	case EXP_BUFF_ID_PRINT_NEWLINE:
	  infix_stack_push("<nl>");
	  break;

	case EXP_BUFF_ID_LPRINT_SPACE:
	  infix_stack_push("L< > ");
	  break;

	case EXP_BUFF_ID_LPRINT_NEWLINE:
	  infix_stack_push("L<nl>");
	  break;
	  
	case EXP_BUFF_ID_INTEGER:
	case EXP_BUFF_ID_FLT:
	  infix_stack_push(be.name);
	  break;
	  
	case EXP_BUFF_ID_STR:
	  sprintf(newstr2, "%s", be.name);
	  infix_stack_push(newstr2);
	  break;

	case EXP_BUFF_ID_COMMAND:
	case EXP_BUFF_ID_FUNCTION:
	  // See how many arguments to pop
	  numarg = function_num_args(be.name);

	  strcpy(newstr, "");
	  
	  for(int i=0; i<numarg; i++)
	    {
	      infix_stack_pop(op1);

	      sprintf(newstr2, "%s %s", op1, newstr);
	      strcpy(newstr, newstr2);

	      //	      strcat(newstr, ",");
	    }
	  
	  snprintf(newstr2, MAX_INFIX_STR, "%s(%s)", be.name, newstr);
	  infix_stack_push(newstr2);
	  break;
	  
	case EXP_BUFF_ID_OPERATOR:
	  infix_stack_pop(op1);
	  infix_stack_pop(op2);
	  sprintf(newstr, "(%s %s %s)", op2, be.name, op1);
	  infix_stack_push(newstr);
	  break;

	case EXP_BUFF_ID_OPERATOR_UNARY:
	  infix_stack_pop(op1);
	  sprintf(newstr, "(%s %s)", be.name, op1);
	  infix_stack_push(newstr);
	  break;
	  
	case EXP_BUFF_ID_AUTOCON:
	  break;

	case EXP_BUFF_ID_PROC_CALL:
	  // Pop the procname and then the parameters
	  // Push the proc call

	  sprintf(newstr, "%s", be.name);
	  
	  for(int i=0; i<be.op.num_parameters; i++)
	    {
	      infix_stack_pop(op1);

	      strcat(newstr, "(");
	      strcat(newstr, op1);
	      if( i != be.op.num_parameters-1 )
		{
		  strcat(newstr, ", ");
		}

	      strcat(newstr, ")");
	    }
	  
	  infix_stack_push(newstr);
	  break;
	}
    }

  fprintf(ofp, "\nDone\n");
  
  // There may not be a result if there was just a command
  result[0] = '\0';
  
  if( infix_stack_ptr != 0 )
    {
      infix_stack_pop(result);

      sprintf(newstr2, "%s", result);
      strcpy(newstr, newstr2);

      fprintf(ofp, "\nInfix stack result %s", newstr);

    }
  else
    {
      fprintf(ofp, "\nInfix stack empty");
    }

  // If the first character of the result is an open bracket then remove the brackets
  // that surround the result, they are redundant.
  char *res = result;
  
  if( *result == '(' )
    {
      res = result+1;
      result[strlen(result)-1] = '\0';
    }
  
  fprintf(chkfp, "\n----------------------------------------infix-----------------------------------\n");
  fprintf(chkfp, "\n%s\n", res);

  fprintf(trfp, "\n%s", res);
  
  dbprintf("exit  '%s'", res);  
  return(result+1);
}


////////////////////////////////////////////////////////////////////////////////
//
// Take the expression buffer and execute it for types
// Copies expression from one buffer to another, moving closer to QCode in
// the second buffer
//
// Tree-like information about the nodes in the RPN is built up, this allows
// auto conversion tokens to be inserted higher up the tree when needed.
// Auto conversion is only inserted to convert inputs to the required type, it
// isn't used on the output or result of an operator or function. This is to
// avoid double application of conversion tokens.
//
////////////////////////////////////////////////////////////////////////////////

void typecheck_expression(void)
{
  EXP_BUFFER_ENTRY be;
  EXP_BUFFER_ENTRY autocon;
  OP_INFO          op_info;
  EXP_BUFFER_ENTRY op1;
  EXP_BUFFER_ENTRY op2;
  NOBJ_VARTYPE     op1_type, op2_type;
  NOBJ_VARTYPE     op1_reqtype, op2_reqtype;
  NOBJ_VARTYPE     ret_type;
  int              copied;

  dbprintf("");
  
  // Initialise
  init_op_stack_entry(&(autocon.op));
  init_op_stack_entry(&(op1.op));
  init_op_stack_entry(&(op2.op));

  // We copy results over to a second buffer, this allows easy insertion of
  // needed extra codes

  exp_buffer2_i = 0;
  
  // We can check for an assignment and adjust the assignment token to
  // differentiate it from the equality token.

  type_check_stack_init();

  for(int i=0; i<exp_buffer_i; i++)
    {
      // Execute
      be = exp_buffer[i];
      copied = 0;

      // Give every entry a node id
      be.node_id = node_id_index++;
      
      fprintf(ofp, "\n BE:%s", be.name);
		  
      switch(be.op.buf_id)
	{
	  // Not used
	case EXP_BUFF_ID_TKN:
	  break;

	  // No type, marker
	case EXP_BUFF_ID_SUB_START:
	  break;

	  // No type, marker
	case EXP_BUFF_ID_SUB_END:
	  break;

	case EXP_BUFF_ID_META:
	  break;

	case EXP_BUFF_ID_VARIABLE:
	  be.p_idx = 0;
	  type_check_stack_push(be);
	  break;

	case EXP_BUFF_ID_VAR_ADDR_NAME:
	  be.p_idx = 0;
	  type_check_stack_push(be);
	  break;

	case EXP_BUFF_ID_IF:
	case EXP_BUFF_ID_ENDIF:
	case EXP_BUFF_ID_WHILE:
	case EXP_BUFF_ID_ENDWH:
	case EXP_BUFF_ID_FLT:
	case EXP_BUFF_ID_INTEGER:
	case EXP_BUFF_ID_STR:
	  be.p_idx = 0;
	  type_check_stack_push(be);
	  break;

	case EXP_BUFF_ID_FUNCTION:
	  // Functions also require certain types, for instance USR reuires
	  // all integers. Any floats in the arguments require conversion codes.
	  fprintf(ofp, "\nFN: %d args", function_num_args(be.name));
	  
	  // Set up the function return value
	  ret_type = function_return_type(be.name);

	  fprintf(ofp, "\nret_type;%d %c", ret_type, type_to_char(ret_type));
	  fprintf(ofp, "\n%s:Ret type of %s : %c", __FUNCTION__, be.name, type_to_char(ret_type));
	  
	  // Now insert auto convert nodes if required

	  autocon.op.buf_id = EXP_BUFF_ID_AUTOCON;

	  autocon.p_idx = 0;
	  //	  autocon.p[0] = op1.node_id;
	  //autocon.p[1] = op2.node_id;
	  autocon.op.type      = ret_type;
	  autocon.op.req_type  = ret_type;

	  // Now check that all arguments have the correct type or
	  // can with an auto type conversion
	  for(int i=function_num_args(be.name)-1; i>=0; i--)
	    {
	      // Pop an argument off and check it
	      op1 = type_check_stack_pop();

	      fprintf(ofp, "\nFN ARG %d r%c %s %d(%c)", i,
		      type_to_char(function_arg_type_n(be.name, i)),
		      op1.name,
		      op1.op.type,
		      type_to_char(op1.op.type));
	      
	      if( op1.op.type == function_arg_type_n(be.name, i))
		{
		  fprintf(ofp, "  Arg ok");
		  // All OK
		}
	      else
		{
		  fprintf(ofp, "  Arg not OK");

		  sprintf(autocon.name, "autocon %c->%c", type_to_char(op1.op.type), type_to_char(function_arg_type_n(be.name, i)));
		  // Can we use an auto conversion?
		  if( (op1.op.type == NOBJ_VARTYPE_INT) && (function_arg_type_n(be.name, i) == NOBJ_VARTYPE_FLT))
		    {
		      autocon.node_id = node_id_index++;
		      insert_buf2_entry_after_node_id(op1.node_id, autocon);
		    }

		  if( (op1.op.type == NOBJ_VARTYPE_FLT) && (function_arg_type_n(be.name, i) == NOBJ_VARTYPE_INT))
		    {
		      autocon.node_id = node_id_index++;
		      insert_buf2_entry_after_node_id(op1.node_id, autocon);
		    }
		}
	    }
	  
	  // Push dummy result
	  EXP_BUFFER_ENTRY res;
	  res.node_id = be.node_id;          // Result id is that of the operator
	  res.p_idx = function_num_args(be.name);
	  res.p[0] = op1.node_id;
	  res.p[1] = op2.node_id;
	  strcpy(res.name, "000");
	  res.op.type      = ret_type;
	  res.op.req_type  = ret_type;
	  type_check_stack_push(res);
	  
	  // The return type opf the function is known
	  be.op.type = ret_type;
	  be.op.req_type = ret_type;
	  
	  break;

	  //------------------------------------------------------------------------------
	  //
	  // Operators have to be typed correctly depending on their
	  // operands. Some of them are mutable (polymorphic) and we have to bind them to their
	  // type here.
	  // Some are immutable and cause errors if theior operators are not correct
	  // Some have a fixed output type (>= for example, but still have mutable inputs)
	  //
	  
	case EXP_BUFF_ID_OPERATOR:
	  // Check that the operands are correct, i.e. all of them are the same and in
	  // the list of acceptable types
	  fprintf(ofp, "\nBUFF_ID_OPERATOR");
	  
	  if( find_op_info(be.name, &op_info) )
	    {
	      fprintf(ofp, "\nFound operator %s", be.name);

	      // We only handle binary operators here
	      // Pop arguments off stack, this is an analogue of execution of the operator
	      
	      op1 = type_check_stack_pop();
	      op2 = type_check_stack_pop();

	      op1_type = op1.op.type;
	      op2_type = op2.op.type;
	      op1_reqtype = op1.op.req_type;
	      op2_reqtype - op2.op.req_type;
	      dbprintf("op1 type:%c op2 type:%c", type_to_char(op1.op.type), type_to_char(op2.op.type));
	      
	      // Get the node ids of the argumenmts so we can find them if we need to
	      // adjust them.
	      
	      be.p_idx = 2;
	      be.p[0] = op1.node_id;
	      be.p[1] = op2.node_id;
	      
	      // Check all operands are of correct type.
	      if( op_info.immutable )
		{
		  // Immutable types for this operator so we don't do any
		  // auto conversion here. Just check that the correct type
		  // is present, if not, it's an error
		  
		  if( (op1.op.type ==  op_info.type[0]) )
		    {
		      // Types correct, push a dummy result so we have a correct execution stack

		      // Push dummy result
		      EXP_BUFFER_ENTRY res;
		      res.node_id = be.node_id;          // Result id is that of the operator
		      res.p_idx = 2;
		      res.p[0] = op1.node_id;
		      res.p[1] = op2.node_id;
		      strcpy(res.name, "000");

		      // Now set up output type
		      if( op_info.output_type == NOBJ_VARTYPE_UNKNOWN )
			{
			  res.op.type      = op1.op.type;
			  res.op.req_type  = op1.op.type;
			}
		      else
			{
			  res.op.type      = op_info.output_type;
			  res.op.req_type  = op_info.output_type;
			}
		      
		      type_check_stack_push(res);
					    
		    }
		  else
		    {
		      // Error
		      fprintf(ofp, "\nType of %s or %s is not %c", op1.name, op2.name, type_to_char(op_info.type[0]));
		      internal_error("Type of %s or %s is not %c", op1.name, op2.name, type_to_char(op_info.type[0]));
		      //		      exit(-1);
		    }
		}
	      else
		{
		  // Mutable type is dependent on the arguments, e.g.
		  //  A$ = "RTY"
		  // requires that a string equality is used, similarly
		  // INT and FLT need the correctly typed operator.
		  //
		  // INT and FLT have an additional requirement where INT is used
		  // as long as possible, and also assignment can turn FLT into INT
		  // or INT into FLT
		  
		  fprintf(ofp, "\n Mutable type %c %c", type_to_char(op1.op.type), type_to_char(op2.op.type));
		  
		  // Check input types are valid for this operator
		  if( is_a_valid_type(op1.op.type, &op_info) && is_a_valid_type(op2.op.type, &op_info))
		    {
		      // We have types here. We need to insert auto type conversion qcodes here
		      // if needed.
		      //
		      // Int -> float if float required
		      // Float -> int if int required
		      // Expressions start as integer and turn into float if a float is found.
		      //

		      // If the operator type is unknown then we use the types of the arguments
		      // Unknown types arise when brackets are used.
		      if( be.op.type == NOBJ_VARTYPE_UNKNOWN)
			{
			  be.op.type = type_with_least_conversion_from(op1.op.type, op2.op.type);
			}

		      if( be.op.req_type == NOBJ_VARTYPE_UNKNOWN)
			{
			  be.op.req_type = type_with_least_conversion_from(op1.op.type, op2.op.type);
			}

		      if( be.op.type == NOBJ_VARTYPE_INT)
			{
			  be.op.type = type_with_least_conversion_from(op1.op.type, op2.op.type);
			}

		      if( be.op.req_type == NOBJ_VARTYPE_INT)
			{
			  be.op.req_type = type_with_least_conversion_from(op1.op.type, op2.op.type);
			}
		      
		      // Types are both OK
		      // If they are the same then we will bind the operator type to that type
		      // as long as they are both the required type, if not then if types aren't
		      // INT or FLT then it's an error
		      // INT or FLT can be auto converted to the required type
		      
		      if( op1.op.type == op2.op.type )
			{
			  fprintf(ofp, "\n Same type");
			  if( op1.op.type == be.op.req_type )
			    {
			      // The input types of the operands are the same as the required type, all ok
			      be.op.type = op1.op.type;
			      be.op.req_type = op1.op.req_type;

			      // Now set up output type
			      if( op_info.output_type == NOBJ_VARTYPE_UNKNOWN )
				{
				  // Do not force
				}
			      else
				{
				  dbprintf("(A) Forced type to %c", type_to_char(op1.op.type));
				  be.op.type      = op_info.output_type;
				  be.op.req_type  = op_info.output_type;
				  be.op.qcode_type = op1.op.type;
				}
			    }
			  else
			    {
			      // The input types of the argument aren't the required type, we may be able to
			      // auto convert.
			      switch(op1.op.type)
				{
				case NOBJ_VARTYPE_INT:
				case NOBJ_VARTYPE_FLT:
				  // We need to auto convert both operands. We don't change the operator type
				  // to match as the operator has a required type. This is probably quite unusual.
				  
				  // Push dummy result
				  
				  sprintf(autocon.name, "autocon %c->%c", type_to_char(op1.op.type), type_to_char(be.op.req_type));
				  autocon.op.buf_id = EXP_BUFF_ID_AUTOCON;
				  autocon.node_id = node_id_index++;   //Dummy result carries the operator node id as that is the tree node
				  autocon.p_idx = 2;
				  autocon.p[0] = op1.node_id;
				  autocon.p[1] = op2.node_id;
				  autocon.op.type      = be.op.type;
				  autocon.op.req_type  = be.op.type;
				  
				  //exp_buffer2[exp_buffer2_i++] = be;
				  // Now set up output type
				  if( op_info.output_type == NOBJ_VARTYPE_UNKNOWN )
				    {
				      // Do not force
				    }
				  else
				    {
				      dbprintf("(B) Forced type to %c", type_to_char(be.op.type));
				      be.op.qcode_type = be.op.type;
				    }

				  // Insert entry
				  insert_buf2_entry_after_node_id(op1.node_id, autocon);
				  insert_buf2_entry_after_node_id(op2.node_id, autocon);
				  
				  break;
				  
				default:
				  // No auto conversion is available, so this is an error
				  fprintf(ofp, "\nType is not the required type and no auto conversion available,");
				  fprintf(ofp, "\n Node N%d", be.node_id);
				  //exit(-1);
				  syntax_error("Type is not the required type and no auto conversion available,");
				  break;
				}
			    }
			}
		      else
			{
			  fprintf(ofp, "\n Autoconversion");
			  fprintf(ofp, "\n --------------");
			  fprintf(ofp, "\n Op1: type:%d req type:%d", op1.op.type, op1.op.req_type);
			  fprintf(ofp, "\n Op2: type:%d req type:%d", op2.op.type, op2.op.req_type);
			  fprintf(ofp, "\n BE:  type:%d req type:%d",  be.op.type,  be.op.req_type);
		 
			  // We insert auto conversion nodes to force the type of the arguments to match the
			  // operator type. For INT and FLT we can force the operator to FLT if required
			  // Do that before inserting auto conversion nodes.
			  // Special treatment for assignment operator
			  if( op_info.assignment )
			    {
			      // Operator type follows the second operand, which is the variable we
			      // are assigning to

			      // Now set up output type
			      if( op_info.output_type == NOBJ_VARTYPE_UNKNOWN )
				{
				  // Do not force
				}
			      else
				{
				  dbprintf("(C) Forced type to %c", type_to_char(op2.op.type));
				  be.op.type       = op_info.output_type;
				  be.op.req_type   = op_info.output_type;
				  be.op.qcode_type = op2.op.type;
				}
#if 0
			      be.op.type = op2.op.type;
			      be.op.req_type = op2.op.req_type;
#endif
			    }
			  else
			    {
			      // Must be INT/FLT or FLT/INT
			      if( (op1.op.type == NOBJ_VARTYPE_FLT) || (op2.op.type == NOBJ_VARTYPE_FLT) )
				{
				  // Force operator to FLT
				  be.op.type = NOBJ_VARTYPE_FLT;
				  be.op.req_type = NOBJ_VARTYPE_FLT;

				}
			    }

			  // If type of operator is unknown, 
			  
			  // Now insert auto convert nodes if required
			  sprintf(autocon.name, "autocon %c->%c", type_to_char(op1.op.type), type_to_char(be.op.req_type));
			  autocon.op.buf_id = EXP_BUFF_ID_AUTOCON;
			  autocon.node_id = node_id_index++;   //Dummy result carries the operator node id as that is the tree node
			  autocon.p_idx = 2;
			  autocon.p[0] = op1.node_id;
			  autocon.p[1] = op2.node_id;
			  autocon.op.type      = be.op.type;
			  autocon.op.req_type  = be.op.type;

			  if( (op1.op.type != be.op.req_type) )
			    {
			      sprintf(autocon.name, "autocon %c->%c", type_to_char(op1.op.type), type_to_char(be.op.req_type));
			      insert_buf2_entry_after_node_id(op1.node_id, autocon);
			    }

			  if( (op2.op.type != be.op.req_type) )
			    {
			      sprintf(autocon.name, "autocon %c->%c", type_to_char(op2.op.type), type_to_char(be.op.req_type));
			      insert_buf2_entry_after_node_id(op2.node_id, autocon);
			    }
			  
			  // Now set up output type
			  if( op_info.output_type == NOBJ_VARTYPE_UNKNOWN )
			    {
			      // Do not force
			    }
			  else
			    {
			      dbprintf("(D) Forced type to %c", type_to_char(be.op.req_type));
			      be.op.type      = op_info.output_type;
			      be.op.req_type  = op_info.output_type;
			      be.op.qcode_type = autocon.op.req_type;
			    }
			}
		      
		      EXP_BUFFER_ENTRY res;
		      strcpy(res.name, "000");
		      res.node_id = be.node_id;   //Dummy result carries the operator node id as that is the tree node
		      res.p_idx = 2;
		      res.p[0] = op1.node_id;
		      res.p[1] = op2.node_id;
		      res.op.type      = be.op.type;
		      res.op.req_type  = be.op.type;
		      type_check_stack_push(res);

		    }
		  else
		    {
		      // unknown required types exist, this probably shoudn't happen is a syntax error
		      dbprintf("Syntax error at node N%d", be.node_id);
		      dbprintf("Unknown required type");
		      type_check_stack_display();

		      dump_exp_buffer(ofp, 1);
		      internal_error("Syntax error at node N%d", be.node_id);
		      //exit(-1);
		    }
		}		
	    }
	  else
	    {
	      // Error, not found
	    }
	  break;

	default:
	  fprintf(ofp, "\ndefault buf_id");
	  break;
	}
      
      // If entry not copied over, copy it
      if( !copied )
	{
	  exp_buffer2[exp_buffer2_i++] = be;
	  //add_exp_buffer2_entry(be.op, be.op.buf_id);
	}

      type_check_stack_display();
    }
  
}

////////////////////////////////////////////////////////////////////////////////
//
// Processes the RPN tree
//
// The RPN is 'executed' but no function is actually performed. The execution
// is done so a tree can be built up with the nodes of the RPN (with types
// associated with the nodes). This allows auto type conversion nodes to be
// added in the tree where required.
//
////////////////////////////////////////////////////////////////////////////////

void expression_tree_process(char *expr)
{
  node_id_index = 1;
  
  dump_exp_buffer(ofp, 1);
  typecheck_expression();
  dump_exp_buffer(ofp, 1);
  dump_exp_buffer(ofp, 2);
  //  dump_exp_buffer(icfp, 2);
}

////////////////////////////////////////////////////////////////////////////////

// String display of type stack

char tss[40];

char *type_stack_str(void)
{
  char tmps[20];
  
  sprintf(tss, "[%c,(", type_to_char(expression_type));
  
  for(int i=0; i<exp_type_stack_ptr; i++)
    {
      sprintf(tmps, "%c ", type_to_char(exp_type_stack[i]));
      strcat(tss, tmps);
    }
  strcat(tss, ")]");
  return(tss);
}

////////////////////////////////////////////////////////////////////////////////

void init_op_stack_entry(OP_STACK_ENTRY *op)
{
  op->buf_id = EXP_BUFF_ID_NONE;
  op->name[0]        = '\0';
  op->num_bytes      = 0;
  op->level          = 0;
  op->num_parameters = 0;
  op->access         = NOPL_OP_ACCESS_READ;  // Default to reading things
  op->qcode_type     = NOBJ_VARTYPE_UNKNOWN; // Ignored if UNKNOWN, only some operators use this
  
  for(int i=0; i<NOPL_MAX_SUFFIX_BYTES; i++)
    {
      op->bytes[i] = 0xCC;
    }
}

void output_float(OP_STACK_ENTRY token)
{
  fprintf(ofp, "\n(%16s) %s %c %c %s", __FUNCTION__, type_stack_str(), type_to_char(token.type), type_to_char(token.req_type), token.name);
  add_exp_buffer_entry(token, EXP_BUFF_ID_FLT);
}

void output_integer(OP_STACK_ENTRY token)
{
  fprintf(ofp, "\n(%16s) %s %c %c %s", __FUNCTION__, type_stack_str(), type_to_char(token.type), type_to_char(token.req_type), token.name);
  add_exp_buffer_entry(token, EXP_BUFF_ID_INTEGER);
}

// Operators can be unary
void output_operator(OP_STACK_ENTRY op)
{
  char *tokptr;

  op.type = expression_type;
  op.req_type = expression_type;

  dbprintf("%s %c %c %s", type_stack_str(), type_to_char(op.type), type_to_char(op.req_type), op.name);
  add_exp_buffer_entry(op, op.buf_id);
}

void output_function(OP_STACK_ENTRY op)
{
  fprintf(ofp, "\n(%16s) %s %c %c %s", __FUNCTION__, type_stack_str(), type_to_char(op.type), type_to_char(op.req_type), op.name);
  add_exp_buffer_entry(op, EXP_BUFF_ID_FUNCTION);
}

void output_variable(OP_STACK_ENTRY op)
{
  expression_type = op.type;
  
  fprintf(ofp, "\n(%16s) %s %c %c %s", __FUNCTION__, type_stack_str(), type_to_char(op.type), type_to_char(op.req_type), op.name);
  add_exp_buffer_entry(op, EXP_BUFF_ID_VARIABLE);
}

void output_var_addr_name(OP_STACK_ENTRY op)
{
  fprintf(ofp, "\n(%16s) %s %c %c %s", __FUNCTION__, type_stack_str(), type_to_char(op.type), type_to_char(op.req_type), op.name);
  add_exp_buffer_entry(op, EXP_BUFF_ID_VAR_ADDR_NAME);
}

void output_string(OP_STACK_ENTRY op)
{
  fprintf(ofp, "\n(%16s) %s %c %c %s", __FUNCTION__, type_stack_str(), type_to_char(op.type), type_to_char(op.req_type), op.name);
  // Always a string type
  op.req_type = NOBJ_VARTYPE_STR;
  add_exp_buffer_entry(op, EXP_BUFF_ID_STR);
}

void output_return(OP_STACK_ENTRY op)
{
  op.type = expression_type;
  op.req_type = expression_type;
  
  fprintf(ofp, "\n(%16s) %s %c %c %s", __FUNCTION__, type_stack_str(), type_to_char(op.type), type_to_char(op.req_type), op.name); 
  add_exp_buffer_entry(op, EXP_BUFF_ID_RETURN);
}

void output_print(OP_STACK_ENTRY op)
{
  op.type = expression_type;
  op.req_type = expression_type;

  fprintf(ofp, "\n(%16s) %s %c %c %s", __FUNCTION__, type_stack_str(), type_to_char(op.type), type_to_char(op.req_type), op.name); 
  add_exp_buffer_entry(op, op.buf_id);
}

void output_proc_call(OP_STACK_ENTRY op)
{
  fprintf(ofp, "\n(%16s) %s %c %c %s", __FUNCTION__, type_stack_str(), type_to_char(op.type), type_to_char(op.req_type), op.name); 
  add_exp_buffer_entry(op, EXP_BUFF_ID_PROC_CALL);
}

void output_if(OP_STACK_ENTRY op)
{
  fprintf(ofp, "\n(%16s) %s %c %c %s", __FUNCTION__, type_stack_str(), type_to_char(op.type), type_to_char(op.req_type), op.name); 
  add_exp_buffer_entry(op, EXP_BUFF_ID_IF);
}

void output_generic(OP_STACK_ENTRY op, char *name, int buf_id)
{
  char line[20];
  
  strcpy(op.name, name);
  op.buf_id = buf_id;
  op.type = expression_type;
  op.req_type = expression_type;
  
  dbprintf("%s %c %c %s exp_type:%c", type_stack_str(), type_to_char(op.type), type_to_char(op.req_type), op.name, type_to_char(expression_type) ); 
  add_exp_buffer_entry(op, buf_id);
}

void output_endif(OP_STACK_ENTRY op)
{
  printf("\nop if");
  fprintf(ofp, "\n(%16s) %s %c %c %s", __FUNCTION__, type_stack_str(), type_to_char(op.type), type_to_char(op.req_type), op.name); 
  add_exp_buffer_entry(op, EXP_BUFF_ID_ENDIF);
}

// Markers used as comments, and hints
void output_marker(char *marker, ...)
{
  va_list valist;
  char line[80];
  
  va_start(valist, marker);

  vsprintf(line, marker, valist);
  va_end(valist);

  fprintf(ofp, "\n(%16s) %s", __FUNCTION__, line);
}

void output_sub_start(void)
{
  OP_STACK_ENTRY op;
  init_op_stack_entry(&op);
  
  fprintf(ofp, "\n(%16s)", __FUNCTION__);

  strcpy(op.name,  "");
  op.type = NOBJ_VARTYPE_UNKNOWN;
  add_exp_buffer_entry(op, EXP_BUFF_ID_SUB_START);
}

void output_sub_end(void)
{
  OP_STACK_ENTRY op;

  init_op_stack_entry(&op);
    
  fprintf(ofp, "\n(%16s)", __FUNCTION__);

  strcpy(op.name, "");
  op.type = NOBJ_VARTYPE_UNKNOWN;
  add_exp_buffer_entry(op, EXP_BUFF_ID_SUB_END);
}

void output_expression_start(char *expr)
{
  strcpy(current_expression, expr);
  
  if( strlen(expr) > 0 )
    {
      fprintf(ofp, "\n%s", expr);
      fprintf(ofp, "\n========================================================");

      fprintf(ofp, "\n(%16s)", __FUNCTION__);
      
      // We have a new expression, process the previous one which will be in the
      // buffer
      
      //  expression_tree_process(expr);
      
    }

  // Clear operator stack
  op_stack_ptr = 0;
  
  // Clear the buffer ready for the new expression that has just come in
  clear_exp_buffer();

  first_token = 1;
  expression_type = NOBJ_VARTYPE_UNKNOWN;
	
}

////////////////////////////////////////////////////////////////////////////////
//
// Stack function in operator stack
//
////////////////////////////////////////////////////////////////////////////////


void op_stack_push(OP_STACK_ENTRY entry)
{
  fprintf(ofp,"\n Push:'%s'", entry.name);

  
  if( op_stack_ptr < NOPL_MAX_OP_STACK )
    {
      op_stack[op_stack_ptr++] = entry;
    }
  else
    {
      fprintf(ofp, "\n%s: Operator stack full", __FUNCTION__);
      internal_error("Operator stack full");
      //exit(-1);
    }
  op_stack_print();

}

// Copies data into string

OP_STACK_ENTRY op_stack_pop(void)
{
  OP_STACK_ENTRY o;
  
  if( op_stack_ptr == 0 )
    {
      fprintf(ofp, "\n%s: Operator stack empty", __FUNCTION__);
      typecheck_error("Operatior stack empty");
      return(o);
    }
  
  op_stack_ptr --;

  o = op_stack[op_stack_ptr];
  dbprintf("Pop '%s' type:%c ", o.name, type_to_char(o.type));
  op_stack_print();
  return(o);
}

// Return a pointer to the top entry of the stack, empty string if empty
OP_STACK_ENTRY op_stack_top(void)
{
  if( op_stack_ptr == 0 )
    {
      OP_STACK_ENTRY o;
      
      strcpy(o.name, "");
      o.type = NOBJ_VARTYPE_UNKNOWN;
      
      return(o);
    }

  return( op_stack[op_stack_ptr-1] );
}

void op_stack_display(void)
{
  char *s;
  
  fprintf(ofp, "\n\nOperator Stack\n");
  
  for(int i=0; i<op_stack_ptr-1; i++)
    {
      s = op_stack[i].name;
      fprintf(ofp, "\n%03d: %s type:%d", i, s, op_stack[i].type);
    }
}

void op_stack_print(void)
{
  char *s;

  dbprintf("------------------");
  dbprintf("Operator Stack     (%d)\n", op_stack_ptr);
  
  for(int i=0; i<op_stack_ptr; i++)
    {
      s = op_stack[i].name;
      dbprintf("%03d: %s type:%c id:%s",
	       i,
	       s,
	       type_to_char(op_stack[i].type),
	       exp_buffer_id_str[op_stack[i].buf_id]);
    }

  dbprintf("------------------\n");
}

////////////////////////////////////////////////////////////////////////////////
//
// End of shunting algorithm, flush the stack
// Processing the RPN as a tree (adds auto conversion) is done after we flush
// the stack (shunting operator stack)
//
////////////////////////////////////////////////////////////////////////////////

void op_stack_finalise(void)
{
  OP_STACK_ENTRY o;

  dbprintf("Finalise stack");

  while( strlen(op_stack_top().name) != 0 )
    {
      o = op_stack_pop();

      dbprintf("Popped:%s rqt:%c", o.name, type_to_char(o.req_type));
      if( o.req_type == NOBJ_VARTYPE_UNKNOWN )
	{
	  o.req_type = expression_type;
	  o.type = expression_type;
	}

      // PRINT command sneed to match the expression type
      if( o.buf_id == EXP_BUFF_ID_PRINT )
	{
	  // Force the type
	  o.req_type = expression_type;
	  o.type = expression_type;
	}
      output_operator(o);
    }

}

////////////////////////////////////////////////////////////////////////////////

void process_expression_types(void)
{
  char *infix;

  dbprintf("\n%s:", __FUNCTION__);

  // TODO needed?
  if( strlen(current_expression) > 0,1 )
    {
      // Process the RPN as a tree
      expression_tree_process(current_expression);

      dbprintf("\n==INFIX==\n",0);
      dbprintf("==%s==", infix = infix_from_rpn());
      dbprintf("\n\n",0);
      
      // Generate the QCode from the tree output, but only on pass 2
      if( pass_number == 2 )
	{
	  output_qcode_for_line();
	}
    }
}

void init_output(void)
{
  ofp   = fopen("output.txt", "w");
  chkfp = fopen("check.txt", "w");
  trfp  = fopen("translated.opl", "w");
  icfp  = fopen("intcode.txt", "w");
}

void uninit_output(void)
{
  op_stack_display();
  fclose(ofp);
  fclose(icfp);
}

////////////////////////////////////////////////////////////////////////////////
//
// Expression type stack
//
// Used when processing sub expressions as we need to have an expression type for
// all sub expressions but not lose the current one when the sub expression finishes
// processing. The current expression type is saved by pushing it onto this stack
//
//
////////////////////////////////////////////////////////////////////////////////


void exp_type_push(NOBJ_VARTYPE t)
{
  if( exp_type_stack_ptr <= (MAX_EXP_TYPE_STACK - 1))
    {
      exp_type_stack[exp_type_stack_ptr++] = t;
    }
  else
    {
      fprintf(ofp, "\nSub expression stack full");
      typecheck_error("Sub expression stack full");
      return;
    }
}

NOBJ_VARTYPE exp_type_pop(void)
{
  if( exp_type_stack_ptr > 0 )
    {
      return(exp_type_stack[--exp_type_stack_ptr]);
    }
  else
    {
      fprintf(ofp, "\nExp stack empty on pop");
      typecheck_error("Expression stack empty on pop");
      return(NOBJ_VARTYPE_UNKNOWN);
    }
}

////////////////////////////////////////////////////////////////////////////////
//
// Process another token using the shunting algorithm.
//
// This converts the infix expression (as in OPL) to an RPN expression.
// All OPL statements are treated as expressions. This works well with the
// QCode assignment operator, and OPL functions and commands (functions
// with no return value)
//
////////////////////////////////////////////////////////////////////////////////

// For unary operators (we only have '-')
// The next token is unary (if it can be) if it is at the start of th einput, or follows an operator
// or follows a left parenthesis.

int unary_next = 0;

#define CAN_BE_UNARY (first_token || unary_next)

void process_token(OP_STACK_ENTRY *token)
{
  char *tokptr;
  OP_STACK_ENTRY o1;
  OP_STACK_ENTRY o2;
  int opr1, opr2;
  
  dbprintf("   Frst:%d T:'%s' exptype:%c bufid:'%s'",
	  first_token, token->name,
	  type_to_char(expression_type),
	  exp_buffer_id_str[token->buf_id]);

  o1 = *token;
  //strcpy(o1.name, token);
  o1.type = expression_type;
  o1.buf_id = token->buf_id;
  
  // Another token has arrived, process it using the shunting algorithm
  // First, check the stack for work to do

  o2 = op_stack_top();
  opr1 = operator_precedence(o1.name);
  opr2 = operator_precedence(o2.name);

  if( strcmp(o1.name, "(")==0 )
    {
      OP_STACK_ENTRY o;

      init_op_stack_entry(&o);
	
      //output_marker("--------- Sub 1");
      output_sub_start();
      
      strcpy(o.name, "(");
      o.type = NOBJ_VARTYPE_UNKNOWN;
      
      op_stack_push(o);

      // Sub expression, push (save) the expression type and process the sub expression
      // as a new one
      exp_type_push(expression_type);
      expression_type = NOBJ_VARTYPE_UNKNOWN;
      first_token = 0;
      unary_next = 1;
      return;
    }

  if( strcmp(o1.name, ")")==0 )
    {
      //output_marker("----------- Sub E 1");
	    
      while(  (strcmp(op_stack_top().name, "(") != 0) && (strlen(op_stack_top().name)!=0) )
	{
	  dbprintf("\nPop 3");
	  o2 = op_stack_pop();
	  output_operator(o2);
	}

      if( strlen(op_stack_top().name)==0 )
	{
	  // Mismatched parentheses
	  dbprintf("\nMismatched parentheses");
	  return;
	}

      // Pop the open bracket off the operator stack
      fprintf(ofp, "\nPop 4");
      o2 = op_stack_pop();
      
      //output_marker("-------- Sub E 2");
      output_sub_end();
      
      if( strcmp(o2.name, "(") != 0 )
	{
	  dbprintf("\n**** Should be left parenthesis");
	  return;
	}
      
      expression_type = exp_type_pop();

      output_sub_end();
      first_token = 0;
      unary_next = 0;
      return;
    }

  switch( o1.buf_id )
    {

    case EXP_BUFF_ID_PRINT:
    case EXP_BUFF_ID_PRINT_SPACE:
    case EXP_BUFF_ID_PRINT_NEWLINE:
    case EXP_BUFF_ID_LPRINT:
    case EXP_BUFF_ID_LPRINT_SPACE:
    case EXP_BUFF_ID_LPRINT_NEWLINE:
      fprintf(ofp, "\nBuff id print");
      
      NOBJ_VARTYPE vt;
      
      // The type of the function is known, use that, not the expression type
      // which is more of a hint.
      //strcpy(o1.name, tokptr);
      
      fprintf(ofp, "\n%s: '%s' t=>%c", __FUNCTION__, o1.name, type_to_char(vt));

      o1.type = NOBJ_VARTYPE_UNKNOWN;
      o1.req_type = NOBJ_VARTYPE_UNKNOWN;
      op_stack_push(o1);
      first_token = 0;
      unary_next = 0;
      return;
      break;

    
    case EXP_BUFF_ID_RETURN:
      fprintf(ofp, "\nBuff id return");

      // RETURN needs to change depending on the type of the expression we are to return.
      // The type of that expression must also match that of the procedure we are translating
      o1.req_type = expression_type;
      output_return(o1);
      unary_next = 0;
      return;
      break;
      
    case EXP_BUFF_ID_PROC_CALL:
      fprintf(ofp, "\nBuff id proc call");
      
      // Parser supplies type
      o1.req_type = expression_type;
      output_proc_call(o1);
      unary_next = 0;
      return;
      break;

    case EXP_BUFF_ID_WHILE:
    case EXP_BUFF_ID_UNTIL:
    case EXP_BUFF_ID_IF:
    case EXP_BUFF_ID_ENDIF:
    case EXP_BUFF_ID_ENDWH:
    case EXP_BUFF_ID_TRAP:
    case EXP_BUFF_ID_META:
      dbprintf("Buff id %s", o1.name);
      
      // Parser supplies type
      o1.req_type = expression_type;
      output_generic(o1, o1.name, o1.buf_id);
      unary_next = 0;
      return;
      break;

    case EXP_BUFF_ID_VAR_ADDR_NAME:
      fprintf(ofp, "\nBuff id var addr name");
      
      // Parser supplies type
      o1.req_type = NOBJ_VARTYPE_VAR_ADDR;
      o1.type     = NOBJ_VARTYPE_VAR_ADDR;
      output_var_addr_name(o1);
      unary_next = 0;
      return;
      break;
    }

#define OP_PREC(OP) (operator_precedence(OP.name))
  
  if( token_is_operator(o1.name, &(tokptr)) )
    {
      dbprintf("\nToken is operator o1 name:%s o2 name:%s", o1.name, o2.name);
      dbprintf("\nopr1:%d opr2:%d", opr1, opr2);
      
      // Turn token into unary version if we can
      if( CAN_BE_UNARY )
	{
	  operator_can_be_unary(&o1);
	  dbprintf("Operator turned into unary version");
	  opr1 = operator_precedence(o1.name);

	}
      
      unary_next = 1;
      
      while( (strlen(op_stack_top().name) != 0) && (strcmp(op_stack_top().name, "(") != 0 ) &&
	     ( OP_PREC(op_stack_top()) > opr1) || ((opr1 == OP_PREC(op_stack_top()) && operator_left_assoc(o1.name)))
	     )
	{
	  fprintf(ofp, "\nPop 1");
	  
	  o2 = op_stack_pop();
	  opr1 = operator_precedence(o1.name);
	  opr2 = operator_precedence(o2.name);

	  output_operator(o2);
	}

      dbprintf("Push %s", exp_buffer_id_str[o1.buf_id]);
      
      //strcpy(o1.name, tokptr);

      o1.type = expression_type;
      o1.req_type = expression_type;
      op_stack_push(o1);
      first_token = 0;
      return;
    }

  if( token_is_float(o1.name) )
    {

      o1.type = NOBJ_VARTYPE_FLT;

      modify_expression_type(o1.type);

      o1.req_type = expression_type;
      
      output_float(o1);
      first_token = 0;
      unary_next = 0;
      return;
    }

  if( token_is_integer(o1.name) )
    {
      o1.type = NOBJ_VARTYPE_INT;
      
      modify_expression_type(o1.type);
      o1.req_type = expression_type;
      output_integer(o1);
      first_token = 0;
      unary_next = 0;
      return;
    }

    if( token_is_function(o1.name, &tokptr) )
    {
      NOBJ_VARTYPE vt;
     
      // The type of the function is known, use that, not the expression type
      // which is more of a hint.
      strcpy(o1.name, tokptr);
      vt = function_return_type(o1.name);
      
      fprintf(ofp, "\n%s: '%s' t=>%c", __FUNCTION__, o1.name, type_to_char(vt));
      
      o1.type = vt;
      o1.req_type = vt;
      op_stack_push(o1);
      first_token = 0;
      unary_next = 0;
      return;
    }

  if( token_is_variable(o1.name) )
    {
      NOBJ_VARTYPE type, new_type;

      // Get variable names
      init_get_name(o1.name);

      if( get_name(o1.name, &type) )
	{
	  // Valid variable name
	  // Perform type checking. We need to be sure that this is a
	  // valid type (e.g. A% = B$ is invalid) and also the expression type
	  // may need to change (e.g.  A% + 10 * B needs to be type FLT for the * and a
	  // type conversion token needs to be inserted.

	  // If the first token is a variable then we don't want to update the expression type
	  // as this is an assignment and we want the assignment to become a float, for instance
	  // only based on the calculation not the assignment variable type. If we didn't then
	  // expressionms like:
	  //
	  // A= 10*20
	  //
	  // would be calculated as floats, which isn't what the original does. The type is therefore set
	  // to int if it's a float or int
	  //
	  //
	  // Arrays need to have their indices calculated with expressions. An array dereference
	  // operator is inserted to ensure that the index expressions are bound to the variables
	  // correctly through the shunting algorithm.
	  
	  if( first_token )
	    {
	      // If a float variable then we start with INT as a type. Auto conversion will
	      // handle the int/float type issues.
	      if (type == NOBJ_VARTYPE_FLT)
		{
		  modify_expression_type(NOBJ_VARTYPE_INT);
		}
	      else
		{
		  modify_expression_type(type);
		}
	      
	      o1.req_type = type;
	      o1.type = type;
	    }
	  else
	    {
	      fprintf(ofp, "\n%s:type:%c req_type:%c", __FUNCTION__, type_to_char(o1.type), type_to_char(o1.req_type));
	      modify_expression_type(type);
	      o1.req_type = expression_type;
	      o1.type = expression_type;
	    }
	}
      else
	{
	  // Syntax error
	}

      // The type of the variable will affect the expression type
      
      output_variable(o1);
      first_token = 0;
      unary_next = 0;

      
      return;
    }

  if( token_is_string(o1.name) )
    {
      o1.type = NOBJ_VARTYPE_STR;
      output_string(o1);
      modify_expression_type(o1.type);
      first_token = 0;
      unary_next = 0;
      return;
    }
  
  first_token = 0;
  unary_next = 0;

  dbprintf("**Unknown token **      '%s'", o1.name);
}

int is_op_delimiter(char ch)
{
  switch(ch)
    {
    case ':':
    case ';':
    case ',':
    case '(':
    case ')':
    case '"':
    case '+':
    case'@':
    case '-':
    case '*':
    case '/':
    case '>':
    case '<':
      //    case '$':
    case '=':
      return(1);
      break;
    }

  return 0;
  
}

int is_delimiter(char ch)
{
  switch(ch)
    {
    case ' ':
    case '\n':
    case '\r':
    case ':':
    case ';':
    case ',':
    case '@':
    case '+':
    case '-':
    case '*':
    case '/':
    case '(':
    case ')':
    case '>':
    case '<':
      //case '$':
    case '=':
    case '"':
      return(1);
      break;
    }

  return 0;
  
}

////////////////////////////////////////////////////////////////////////////////
//
// Scan and check for command
// 
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////

void finalise_expression(void)
{
  dbprintf("Finalise expression Buf i:%d buf2 i:%d", exp_buffer_i, exp_buffer2_i);

  // Now finalise the translation
  op_stack_finalise();
  process_expression_types();
  
  dbprintf("Finalise expression done.");
}

#if 0
void dummy(void)
{
  int num_commas;
  
  // Assignment can be done using an expression
  scan_expression(&num_commas, HEED_COMMA);
  finalise_expression();
  process_expression_types);
}
#endif

////////////////////////////////////////////////////////////////////////////////
//
//
//
//
////////////////////////////////////////////////////////////////////////////////

int n_lines_ok    = 0;
int n_lines_bad   = 0;
int n_lines_blank = 0;
int n_stack_errors = 0;

////////////////////////////////////////////////////////////////////////////////
//
// Translates a file
//
////////////////////////////////////////////////////////////////////////////////

void translate_file(FILE *fp, FILE *ofp)
{
  char line[MAX_NOPL_LINE+1];
  int idx;
  
  // Initialise the line supplier
  initialise_line_supplier(fp);

  idx = cline_i;
  
  // Now translate the file
  pull_next_line();
  
  if( scan_procdef() )
    {
      n_lines_ok++;
      dbprintf("\ncline scanned OK");
      //finalise_expression();
    }
  else
    {
      n_lines_bad++;
      dbprintf("\ncline failed scan");
    }

  pull_next_line();
  
  idx = cline_i;  

  while( check_declare(&idx) )
    {
      //output_expression_start(cline);
      if( scan_declare() )
	{
	  // All OK
	}
      else
	{
	  syntax_error("Bad declaration");
	}
      
      pull_next_line();
      idx = cline_i;  
    }

  indent_none();

  LEVEL_INFO levels;

  levels.if_level = 0;

  while( 1 )
    {
      if( !scan_line(levels) )
	{
	  break;
	}

      dbprintf("********************************************************************************");
      dbprintf("********************************************************************************");
      dbprintf("Scan line ok");
      
      n_lines_ok++;
      
      idx = cline_i;
      
      if ( check_literal(&idx," :") )
	{
	  dbprintf("Dropping colon");
	  fprintf(chkfp, "  dropping colon");
	  cline_i = idx;
	  //scan_literal(" :");
	}

      indent_none();
    }

  finalise_expression();
  
  
  // Done

  dbprintf("Done");
  dbprintf("");
}

////////////////////////////////////////////////////////////////////////////////

void dump_vars(FILE *fp)
{

  fprintf(fp, "\nVariables");
  fprintf(fp, "\n");

  for(int i=0; i<num_var_info; i++)
    {
      fprintf(fp, "\n%4d:  ", i);
      fprint_var_info(fp, &(var_info[i]));
    }
  fprintf(fp, "\n");
}


////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////////


int main(int argc, char *argv[])
{
  FILE *fp;
  FILE *vfp;
  
  init_output();

  //ofp = fopen("out.opl.tran", "w");

  parser_check();

  // Perform two passes of translation and qcode generation
  for(pass_number = 1; pass_number<=2; pass_number++)
    {
      // Open file and process on a line by line basis
      fp = fopen(argv[1], "r");
      
      if( fp == NULL )
	{
	  fprintf(ofp, "\nCould not open '%s'", argv[1]);
	  printf("\nCould not open '%s'", argv[1]);
	  exit(-1);
	}

      translate_file(fp, ofp);
      fclose(fp);
    }

  output_qcode_suffix();

  // Fill in the conditional offsets
  do_cond_fixup();
  
  // Fill in the qcode length field
  set_qcode_header_byte_at(size_of_qcode_idx,   1, qcode_len >> 8);
  set_qcode_header_byte_at(size_of_qcode_idx+1, 1, qcode_len &  0xFF);

  dump_exp_buffer(ofp, 2);
  dump_qcode_data();

  fclose(chkfp);
  fclose(trfp);

  vfp = fopen("vars.txt", "w");
  dump_vars(vfp);
  fclose(vfp); 

  dbprintf("\n");
  dbprintf("\n %d lines scanned OK",       n_lines_ok);
  dbprintf("\n %d lines scanned failed",   n_lines_bad);
  dbprintf("\n %d lines blank",            n_lines_blank);
  dbprintf("\n %d variables",              num_var_info);
  dbprintf("\n");

  printf("\n %d lines scanned Ok",       n_lines_ok);
  printf("  %d lines scanned failed",   n_lines_bad);
  printf("  %d variables",              num_var_info);
  printf("  %d lines blank\n",            n_lines_blank);

  uninit_output();  
}

