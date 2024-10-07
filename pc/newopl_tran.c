////////////////////////////////////////////////////////////////////////////////
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

FILE *ofp;

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
int token_is_function(char *token, char **tokstr);
void modify_expression_type(NOBJ_VARTYPE t);
void op_stack_display(void);
void op_stack_print(void);



////////////////////////////////////////////////////////////////////////////////
//
// Operator info
//
// Some operators have a mutable type, they are polymorphic. Some are not.
// The possible types for operators are listed here
//

#define MAX_OPERATOR_TYPES 3
#define IMMUTABLE_TYPE     1
#define   MUTABLE_TYPE     0

typedef struct _OP_INFO
{
  char *name;
  int   precedence;
  int   left_assoc;
  int   immutable;
  int   type[MAX_OPERATOR_TYPES];
  int   qcode;                     // Easily translatable qcodes
} OP_INFO;

OP_INFO  op_info[] =
  {
    { "+",   3, 0,   MUTABLE_TYPE, {NOBJ_VARTYPE_INT, NOBJ_VARTYPE_FLT, NOBJ_VARTYPE_STR} },
    { "-",   3, 0,   MUTABLE_TYPE, {NOBJ_VARTYPE_INT, NOBJ_VARTYPE_FLT, NOBJ_VARTYPE_STR} },
    { "*",   5, 1,   MUTABLE_TYPE, {NOBJ_VARTYPE_INT, NOBJ_VARTYPE_FLT, NOBJ_VARTYPE_STR} },
    { "/",   5, 1,   MUTABLE_TYPE, {NOBJ_VARTYPE_INT, NOBJ_VARTYPE_FLT, NOBJ_VARTYPE_STR} },
    { ">",   5, 1,   MUTABLE_TYPE, {NOBJ_VARTYPE_INT, NOBJ_VARTYPE_FLT, NOBJ_VARTYPE_STR} },
    { "AND", 5, 1,   MUTABLE_TYPE, {NOBJ_VARTYPE_INT, NOBJ_VARTYPE_FLT, NOBJ_VARTYPE_INT} },
    // (Handle bitwise on integer, logical on floats somewhere)
    //{ ",", 0, 0 }, /// Not used?
    
    // LZ only
    { "+%", 5, 1, IMMUTABLE_TYPE, {NOBJ_VARTYPE_FLT, NOBJ_VARTYPE_FLT, NOBJ_VARTYPE_FLT} },
    { "-%", 5, 1, IMMUTABLE_TYPE, {NOBJ_VARTYPE_FLT, NOBJ_VARTYPE_FLT, NOBJ_VARTYPE_FLT} },
  };

#define NUM_OPERATORS (sizeof(op_info)/sizeof(struct _OP_INFO))

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
int is_a_req_type(NOBJ_VARTYPE type, OP_INFO *op_info)
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
      if( !( isdigit(*token) || (*token == '.')))
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
  
  for(int i=0; i<len; i++)
    {
      if( !isdigit(*(token)) )
	{
	  all_digits = 0;
	}

      token++;
    }
  
  return(all_digits);
}

// Variable if it ends in $ or %
// Variable if not a function name
// Variables have to be only alpha or alpha followed by alphanum

int token_is_variable(char *token)
{
  int is_var = 0;
  char *tokptr;
  
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

  printf("\nA");
  
  for(int i=0; i<strlen(token)-1; i++)
    {
      if( !isalnum(token[i]) )
	{
	  return(0);
	}
    }

  printf("\nB");
  
  char last_char = token[strlen(token)-1];

  printf("\nC last:%c", last_char);

#if 0
  if( !isalnum(last_char) )
    {
      return(0);
    }
#endif
  printf("\nD");
    
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

struct _FN_INFO
{
  char *name;
  char *argtypes;
  char *resulttype;
}
  fn_info[] =
  {
    { "EOL",      "ii",       "f" },
    { "=",        "ii",       "f" },
    { "ABS",      "ii",       "f" },
    { "ACOS",     "ii",       "f" },
    { "ADDR",     "ii",       "f" },
    { "APPEND",   "ii",       "f" },
    { "ASC",      "ii",       "f" },
    { "ASIN",     "ii",       "f" },
    { "AT",       "ii",       "f" },
    { "ATAN",     "ii",       "f" },
    { "BACK",     "ii",       "f" },
    { "BEEP",     "ii",       "f" },
    { "BREAK",    "ii",       "f" },
    { "CHR$",     "ii",       "f" },
    { "CLOCK",    "ii",       "f" },
    { "CLOSE",    "ii",       "f" },
    { "CLS",      "ii",       "f" },
    { "CONTINUE", "ii",       "f" },
    { "COPY",     "ii",       "f" },
    { "COPYW",    "ii",       "f" },
    { "COS",      "ii",       "f" },
    { "COUNT",    "ii",       "f" },
    { "CREATE",   "ii",       "f" },
    { "CURSOR",   "ii",       "f" },
    { "DATIM$",   "ii",       "f" },
    { "DAY",      "ii",       "f" },
    { "DAYNAME$", "ii",       "f" },
    { "DAYS",     "ii",       "f" },
    { "DEG",      "ii",       "f" },
    { "DELETE",   "ii",       "f" },
    { "DELETEW",  "ii",       "f" },
    { "DIR$",     "ii",       "f" },
    { "DIRW$",    "ii",       "f" },
    { "DISP",     "ii",       "f" },
    { "DOW",      "ii",       "f" },
    { "EDIT",     "ii",       "f" },
    { "EOF",      "ii",       "f" },
    { "ERASE",    "ii",       "f" },
    { "ERR",      "ii",       "f" },
    { "ERR$",     "ii",       "f" },
    { "ESCAPE",   "ii",       "f" },
    { "EXIST",    "ii",       "f" },
    { "EXP",      "ii",       "f" },
    { "FIND",     "ii",       "f" },
    { "FINDW",    "ii",       "f" },
    { "FIRST",    "ii",       "f" },
    { "FIX$",     "ii",       "f" },
    { "FLT",      "ii",       "f" },
    { "FREE",     "ii",       "f" },
    { "GEN$",     "ii",       "f" },
    { "GET",      "ii",       "f" },
    { "GET$",     "ii",       "f" },
    { "GLOBAL",   "ii",       "f" },
    { "GOTO",     "ii",       "f" },
    { "HEX$",     "ii",       "f" },
    { "HOUR",     "ii",       "f" },
    { "IABS",     "ii",       "f" },
    { "INPUT",    "ii",       "f" },
    { "INT",      "ii",       "f" },
    { "INTF",     "ii",       "f" },
    { "KEY",      "ii",       "f" },
    { "KEY$",     "ii",       "f" },
    { "KSTAT",    "ii",       "f" },
    { "LAST",     "ii",       "f" },
    { "LEFT$",    "ii",       "f" },
    { "LEN",      "ii",       "f" },
    { "LN",       "ii",       "f" },
    { "LOC",      "ii",       "f" },
    { "LOCAL",    "ii",       "f" },
    { "LOG",      "ii",       "f" },
    { "LOWER$",   "ii",       "f" },
    { "LPRINT",   "ii",       "f" },
    { "MAX",      "ii",       "f" },
    { "MEAN",     "ii",       "f" },
    { "MENU",     "ii",       "f" },
    { "MENUN",    "ii",       "f" },
    { "MID$",     "ii",       "f" },
    { "MIN",      "ii",       "f" },
    { "MINUTE",   "ii",       "f" },
    { "MONTH",    "ii",       "f" },
    { "MONTH$",   "ii",       "f" },
    { "NEXT",     "ii",       "f" },
    { "NUM$",     "ii",       "f" },
    { "OFF",      "ii",       "f" },
    { "OPEN",     "ii",       "f" },
    { "ONERR",    "ii",       "f" },
    { "PAUSE",    "ii",       "f" },
    { "PEEKB",    "ii",       "f" },
    { "PEEKW",    "ii",       "f" },
    { "PI",       "ii",       "f" },
    { "POKEB",    "ii",       "f" },
    { "POKEW",    "ii",       "f" },
    { "POS",      "ii",       "f" },
    { "POSITION", "ii",       "f" },
    { "PRINT",    "ii",       "f" },
    { "RAD",      "ii",       "f" },
    { "RAISE",    "ii",       "f" },
    { "RANDOMIZE","ii",       "f" },
    { "RECSIZE",  "ii",       "f" },
    { "REM",      "ii",       "f" },
    { "RENAME",   "ii",       "f" },
    { "REPT$",    "ii",       "f" },
    { "RETURN",   "ii",       "f" },
    { "RIGHT$",   "ii",       "f" },
    { "RND",      "ii",       "f" },
    { "SCI$",     "ii",       "f" },
    { "SECOND",   "ii",       "f" },
    { "SIN",      "ii",       "f" },
    { "SPACE",    "ii",       "f" },
    { "SQR",      "ii",       "f" },
    { "STD",      "ii",       "f" },
    { "STOP",     "ii",       "f" },
    { "SUM",      "ii",       "f" },
    { "TAN",      "ii",       "f" },
    { "TRAP",     "ii",       "f" },
    { "UDG",      "ii",       "f" },
    { "UPDATE",   "ii",       "f" },
    { "UPPER$",   "ii",       "f" },
    { "USE",      "ii",       "f" },
    { "USR",      "ii",       ""  },
    { "USR$",     "ii",       "f" },
    { "VAL",      "ii",       "f" },
    { "VAR",      "ii",       "f" },
    { "VIEW",     "ii",       "f" },
    { "WEEK",     "ii",       "f" },
    { "YEAR",     "ii",       "f" },
  };


#define NUM_FUNCTIONS (sizeof(fn_info)/sizeof(struct _FN_INFO))

int token_is_function(char *token, char **tokstr)
{
  for(int i=0; i<NUM_FUNCTIONS; i++)
    {
      if( strcmp(token, fn_info[i].name) == 0 )
	{
	  *tokstr = &(fn_info[i].name[0]);
	  
	  printf("\n%s is function", token);
	  return(1);
	}
    }
  
  return(0);
}

// tokstr is a constant string that we use in the operator stack
// to minimise memory usage.

int token_is_operator(char *token, char **tokstr)
{
  for(int i=0; i<NUM_OPERATORS; i++)
    {
      if( strcmp(token, op_info[i].name) == 0 )
	{
	  *tokstr = &(op_info[i].name[0]);
	  
	  printf("\n'%s' is operator", token);
	  return(1);
	}
    }
  
  return(0);
}

int operator_precedence(char *token)
{
  for(int i=0; i<NUM_OPERATORS; i++)
    {
      if( strcmp(token, op_info[i].name) == 0 )
	{
	  printf("\n%s is operator", token);
	  return(op_info[i].precedence);
	}
    }
  
  return(0);
}

int operator_left_assoc(char *token)
{
  for(int i=0; i<NUM_OPERATORS; i++)
    {
      if( strcmp(token, op_info[i].name) == 0 )
	{
	  printf("\n%s is operator", token);
	  return(op_info[i].left_assoc);
	}
    }
  
  return(0);
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
//
//
// Fixed size tables for variables
//

typedef struct _VAR_INFO
{
  char *name;
  NOBJ_VARTYPE type;
  uint16_t offset;    // Offset from FP
} VAR_INFO;

VAR_INFO local_info[NOPL_MAX_LOCAL];
VAR_INFO global_info[NOPL_MAX_GLOBAL];

int local_info_index  = 0;
int global_info_index = 0;

void output_qcode(char *token)
{
  
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
  printf("\n%s:'%s'", __FUNCTION__, gns_ptr);
  
  // Skip leading spaces
  while( ((*gns_ptr) != '\0') && isspace(*gns_ptr) )
    {
      gns_ptr++;
    }

  printf("\n%s:'%s'", __FUNCTION__, gns_ptr);
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
	  printf("\n%s:gn:'%s'", __FUNCTION__, gn_str);
	  return(gn_str);
	  break;

	case '%':

	  *t = NOBJ_VARTYPE_INT;
	  gn_str[i] = '\0';
	  printf("\n%s:gn:'%s'", __FUNCTION__, gn_str);
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
  printf("\n%s:gn:'%s'", __FUNCTION__, gn_str);
  *t = NOBJ_VARTYPE_FLT;
  return(gn_str);
}

void output_qcode_variable(char *def)
{
  char vname[NOBJ_VARNAME_MAXLEN];
  NOBJ_VARTYPE type;
  
  printf("\n%s: %s", __FUNCTION__, def);

  if( strstr(def, "GLOBAL") != NULL )
    {
      // Get variable names
      init_get_name(def);

      if( get_name(vname, &type) )
	{
	  modify_expression_type(type);
	  type = expression_type;
	}
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

    case NOBJ_VARTYPE_UNKNOWN:
      c = 'U';
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
	  break;

	case NOBJ_VARTYPE_STR:
	  break;
	}
      break;
    }
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

OP_STACK_ENTRY type_check_stack[MAX_TYPE_CHECK_STACK];
int type_check_stack_ptr = 0;

void type_check_stack_push(OP_STACK_ENTRY entry)
{
  printf("\n Push:'%s'", entry.name);

  
  if( type_check_stack_ptr < MAX_TYPE_CHECK_STACK )
    {
      type_check_stack[type_check_stack_ptr++] = entry;
    }
  else
    {
      printf("\n%s: Operator stack full", __FUNCTION__);
      exit(-1);
    }
    type_check_stack_print();

}

// Copies data into string

OP_STACK_ENTRY type_check_stack_pop(void)
{
  OP_STACK_ENTRY o;
  
  if( type_check_stack_ptr == 0 )
    {
      printf("\n%s: Operator stack empty", __FUNCTION__);
      exit(-1);
    }
  
  type_check_stack_ptr --;

  o = type_check_stack[type_check_stack_ptr];
  
  printf("\nPop '%s'", o.name);
  type_check_stack_print();
  return(o);
}

void type_check_stack_display(void)
{
  char *s;
  
  fprintf(ofp, "\n\nType Check Stack (%d)\n", type_check_stack_ptr);

  for(int i=0; i<type_check_stack_ptr; i++)
    {
      s = type_check_stack[i].name;
      fprintf(ofp, "\n%03d: '%s' type:%d", i, s, type_check_stack[i].type);
    }
}

void type_check_stack_print(void)
{
  char *s;

  printf("\n------------------");
  printf("\nType Check Stack     (%d)\n", type_check_stack_ptr);
  
  for(int i=0; i<type_check_stack_ptr; i++)
    {
      s = type_check_stack[i].name;
      printf("\n%03d: '%s' type:%d", i, s, type_check_stack[i].type);
    }

  printf("\n------------------\n");
}

void type_check_stack_init(void)
{
  type_check_stack_ptr = 0;
}


////////////////////////////////////////////////////////////////////////////////

#define MAX_EXP_BUFFER   200

#define EXP_BUFF_ID_TKN        0x01
#define EXP_BUFF_ID_SUB_START  0x02
#define EXP_BUFF_ID_SUB_END    0x03
#define EXP_BUFF_ID_VARIABLE   0x04
#define EXP_BUFF_ID_INTEGER    0x05
#define EXP_BUFF_ID_FLT        0x06
#define EXP_BUFF_ID_STR        0x07
#define EXP_BUFF_ID_FUNCTION   0x08
#define EXP_BUFF_ID_OPERATOR   0x09

char *exp_buffer_id_str[] =
  {
    "EXP_BUFF_ID_???",
    "EXP_BUFF_ID_TKN",
    "EXP_BUFF_ID_SUB_START",
    "EXP_BUFF_ID_SUB_END",
    "EXP_BUFF_ID_VARIABLE",
    "EXP_BUFF_ID_INTEGER",
    "EXP_BUFF_ID_FLT",
    "EXP_BUFF_ID_STR",
    "EXP_BUFF_ID_FUNCTION",
    "EXP_BUFF_ID_OPERATOR",
  };

typedef struct _EXP_BUFFER_ENTRY
{
  char name[40];
  OP_STACK_ENTRY op;
  int buf_id;
} EXP_BUFFER_ENTRY;

EXP_BUFFER_ENTRY exp_buffer[MAX_EXP_BUFFER];
int exp_buffer_i = 0;

EXP_BUFFER_ENTRY exp_buffer2[MAX_EXP_BUFFER];
int exp_buffer2_i = 0;

//------------------------------------------------------------------------------
//

void clear_exp_buffer(void)
{
  exp_buffer_i = 0;
}

void add_exp_buffer_entry(OP_STACK_ENTRY op, int id)
{
  exp_buffer[exp_buffer_i].op = op;
  exp_buffer[exp_buffer_i].buf_id = id;
  strcpy(&(exp_buffer[exp_buffer_i].name[0]), op.name);
  exp_buffer_i++;
}

void add_exp_buffer2_entry(OP_STACK_ENTRY op, int id)
{
  exp_buffer2[exp_buffer2_i].op = op;
  exp_buffer2[exp_buffer2_i].buf_id = id;
  strcpy(&(exp_buffer2[exp_buffer2_i].name[0]), op.name);
  exp_buffer2_i++;
}

void dump_exp_buffer(void)
{
  char *idstr;
  
  fprintf(ofp, "\nExpression buffer");
  fprintf(ofp, "\n=================");
  
  for(int i=0; i<exp_buffer_i; i++)
    {
      OP_STACK_ENTRY token = exp_buffer[i].op;

      fprintf(ofp, "\n(%16s) %-24s %c %c %s", __FUNCTION__, exp_buffer_id_str[exp_buffer[i].buf_id], type_to_char(token.type), type_to_char(token.req_type), exp_buffer[i].name);
    }
  
    fprintf(ofp, "\n=================");
}

void dump_exp_buffer2(void)
{
  char *idstr;
  
  fprintf(ofp, "\nExpression buffer 2");
  fprintf(ofp, "\n===================");
  
  for(int i=0; i<exp_buffer2_i; i++)
    {
      OP_STACK_ENTRY token = exp_buffer2[i].op;

      fprintf(ofp, "\n(%16s) %-24s %c %c %s", __FUNCTION__, exp_buffer_id_str[exp_buffer2[i].buf_id], type_to_char(token.type), type_to_char(token.req_type), exp_buffer2[i].name);
    }
  
    fprintf(ofp, "\n=================");
}

////////////////////////////////////////////////////////////////////////////////
//
// Take the expression buffer and execute it for types
// Copies expression from one buffer to another, moving closer to QCode in
// the second buffer
//
////////////////////////////////////////////////////////////////////////////////

void typecheck_expression(void)
{
  EXP_BUFFER_ENTRY be;
  OP_INFO op_info;
  OP_STACK_ENTRY op1;
  OP_STACK_ENTRY op2;
  int copied;
  
  // We copy results over to a second buffer, this allows easy insertion of
  // needed extra codes

  exp_buffer2_i = 0;
  
  // We can check for an assignment and adjust the assignment token to
  // differentiate it from the equality token.

  // If first token is a variable
  if( exp_buffer[0].buf_id =  EXP_BUFF_ID_VARIABLE)
    {
      // and the last token is an '=', then this is an assignment
      if( strcmp(exp_buffer[exp_buffer_i-1].name, "=") == 0 )
	{
	  // Assignment, make token more specific
	  strcpy(exp_buffer[exp_buffer_i-1].name, ":=");
	}
    }
  
  type_check_stack_init();

#define REQ_TYPE (op_info.type[0])

  for(int i=0; i<exp_buffer_i; i++)
    {
      // Execute
      be = exp_buffer[i];
      copied = 0;

      fprintf(ofp, "\n BE:%s", be.name);
		  
      switch(be.buf_id)
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

	case EXP_BUFF_ID_VARIABLE:
	  type_check_stack_push(be.op);
	  break;

	case EXP_BUFF_ID_FLT:
	case EXP_BUFF_ID_INTEGER:
	case EXP_BUFF_ID_STR:
	  type_check_stack_push(be.op);
	  break;

	case EXP_BUFF_ID_FUNCTION:
	  break;

	  // Operators have to be typed correctly depending on their
	  // operands. Some of them are mutable (polymorphic) and we have to bind them to their
	  // type here.
	  // Some are immutable and cause errors if theior operators are not correct
	  
	case EXP_BUFF_ID_OPERATOR:
	  // Check that the operands are correct, i.e. all of them are the same and in
	  // the list of acceptable types
	  if( find_op_info(be.name, &op_info) )
	    {
	      fprintf(ofp, "\nFound operator %s", be.name);

	      // Single type only, all operands must be that type
	      op1 = type_check_stack_pop();
	      op2 = type_check_stack_pop();
	      
	      // Check all operands are of correct type.
	      if( op_info.immutable )
		{
		  
		  if( (op1.type ==  REQ_TYPE) && (op2.type == REQ_TYPE) )
		    {
		      // Types correct, copy operator over

		      // Push dummy result
		      OP_STACK_ENTRY res;
		      strcpy(res.name, "000");
		      res.type      = op1.type;
		      res.req_type  = op1.type;
		      type_check_stack_push(res);
					    
		    }
		  else
		    {
		      // Error
		      printf("\nType of %s or %s is not %c", op1.name, op2.name, type_to_char(REQ_TYPE));
		      exit(-1);
		    }
		}
	      else
		{
		  // Mutable type is dependent on the arguments, e.g.
		  //  A$ = "RTY"
		  // requires that a string equality is used, similarly
		  // INT and FLT need the correct operator.

		  // INT and FLT have an additional requirement where INT is used
		  // as long as possible, and also assignment can turn FLT into INT
		  // or INT into FLT
		  fprintf(ofp, "\n Mutable type %d %d", op1.type, op2.type);
		  
		  // Check types OK
		  if( is_a_req_type(op1.type, &op_info) && is_a_req_type(op1.type, &op_info))
		    {
		      // Types are both OK
		      // If they are the same then we will bind the operator type to that type
		      if( op1.type == op2.type )
			{
			  fprintf(ofp, "\n same type");
			  be.op.type = op1.type;
			  be.op.req_type = op1.req_type;
			}
		      else
			{
			  // Which type do we use?
			  // For now assume int and flt and promote to flt
			  be.op.type = NOBJ_VARTYPE_FLT;
			  be.op.req_type = NOBJ_VARTYPE_FLT;

			}

		      OP_STACK_ENTRY res;
		      strcpy(res.name, "000");
		      res.type      = op1.type;
		      res.req_type  = op1.type;
		      type_check_stack_push(res);

		    }
		}		
	    }
	  else
	    {
	      // Error, not found
	    }
	  break;
	    
	}
      
      // If entry not copied over, copy it
      if( !copied )
	{
	  exp_buffer2[exp_buffer2_i++] = be;
	  //add_exp_buffer2_entry(be.op, be.buf_id);
	}

      type_check_stack_display();
    }
  
}


////////////////////////////////////////////////////////////////////////////////



void output_float(OP_STACK_ENTRY token)
{
  printf("\nop float");
  fprintf(ofp, "\n(%16s) %c %c %s", __FUNCTION__, type_to_char(token.type), type_to_char(token.req_type), token.name);
  add_exp_buffer_entry(token, EXP_BUFF_ID_FLT);
}

void output_integer(OP_STACK_ENTRY token)
{
  printf("\nop integer");
  fprintf(ofp, "\n(%16s) %c %c %s", __FUNCTION__, type_to_char(token.type), type_to_char(token.req_type), token.name);
  add_exp_buffer_entry(token, EXP_BUFF_ID_INTEGER);
}

void output_operator(OP_STACK_ENTRY op)
{
  printf("\nop operator");
  fprintf(ofp, "\n(%16s) %c %c %s", __FUNCTION__, type_to_char(op.type), type_to_char(op.req_type), op.name);
  add_exp_buffer_entry(op, EXP_BUFF_ID_OPERATOR);
}

void output_variable(OP_STACK_ENTRY op)
{
  printf("\nop variable");
  fprintf(ofp, "\n(%16s) %c %c %s", __FUNCTION__, type_to_char(op.type), type_to_char(op.req_type), op.name);
  add_exp_buffer_entry(op, EXP_BUFF_ID_VARIABLE);
}

void output_string(OP_STACK_ENTRY op)
{
  printf("\nop string");
  fprintf(ofp, "\n(%16s) %c %c %s", __FUNCTION__, type_to_char(op.type), type_to_char(op.req_type), op.name); 
  add_exp_buffer_entry(op, EXP_BUFF_ID_STR);
}

// Markers used as comments, and hints
void output_marker(char *marker, ...)
{
  va_list valist;
  char line[80];
  
  va_start(valist, marker);

  vsprintf(line, marker, valist);
  va_end(valist);

  printf("\nop marker %s", line);
  fprintf(ofp, "\n(%16s) %s", __FUNCTION__, line);
}

void output_sub_start(void)
{
  OP_STACK_ENTRY op;
  
  printf("\nSub expression start");
  fprintf(ofp, "\n(%16s)", __FUNCTION__);

  strcpy(op.name,  "");
  op.type = NOBJ_VARTYPE_UNKNOWN;
  add_exp_buffer_entry(op, EXP_BUFF_ID_SUB_START);
}

void output_sub_end(void)
{
  OP_STACK_ENTRY op;
  
  printf("\nSub expression end");
  fprintf(ofp, "\n(%16s)", __FUNCTION__);

  strcpy(op.name, "");
  op.type = NOBJ_VARTYPE_UNKNOWN;
  add_exp_buffer_entry(op, EXP_BUFF_ID_SUB_END);
}

void output_expression_start(void)
{
  printf("\nExpression start");
  fprintf(ofp, "\n(%16s)", __FUNCTION__);

  dump_exp_buffer();
  typecheck_expression();
  dump_exp_buffer();
  dump_exp_buffer2();
    
  clear_exp_buffer();
}

////////////////////////////////////////////////////////////////////////////////

// Stack function in operator stack


OP_STACK_ENTRY op_stack[NOPL_MAX_OP_STACK+1];

int op_stack_ptr = 0;

void op_stack_push(OP_STACK_ENTRY entry)
{
  printf("\n Push:'%s'", entry.name);

  
  if( op_stack_ptr < NOPL_MAX_OP_STACK )
    {
      op_stack[op_stack_ptr++] = entry;
    }
  else
    {
      printf("\n%s: Operator stack full", __FUNCTION__);
      exit(-1);
    }
    op_stack_print();

}

// Copies data into string

OP_STACK_ENTRY op_stack_pop(void)
{
  OP_STACK_ENTRY o;
  
  if( op_stack_ptr == 0 )
    {
      printf("\n%s: Operator stack empty", __FUNCTION__);
      exit(-1);
    }
  
  op_stack_ptr --;

  o = op_stack[op_stack_ptr];
  printf("\nPop '%s'", o.name);
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

  printf("\n------------------");
  printf("\nOperator Stack     (%d)\n", op_stack_ptr);
  
  for(int i=0; i<op_stack_ptr; i++)
    {
      s = op_stack[i].name;
      printf("\n%03d: %s type:%d", i, s, op_stack[i].type);
    }

  printf("\n------------------\n");
}

// End of shunting algorithm, flush the stack

void op_stack_finalise(void)
{
  OP_STACK_ENTRY o;
  
  while( strlen(op_stack_top().name) != 0 )
    {
      o = op_stack_pop();
      output_operator(o);
    }
}

void init_output(void)
{
  ofp = fopen("output.txt", "w");
}

void uninit_output(void)
{
  op_stack_display();
  fclose(ofp);
}

////////////////////////////////////////////////////////////////////////////////
//
// Expression type stack
//
// Used when processing sub expressions as we need to have an expression type for
// all sub expressions but nbot lose the current one when the sub expression finishes
// processing.
//
//

#define MAX_EXP_TYPE_STACK  20

NOBJ_VARTYPE exp_type_stack[MAX_EXP_TYPE_STACK];
int exp_type_stack_ptr = 0;

void exp_type_push(NOBJ_VARTYPE t)
{
  if( exp_type_stack_ptr <= (MAX_EXP_TYPE_STACK - 1))
    {
      exp_type_stack[exp_type_stack_ptr++] = t;
    }
  else
    {
      printf("\nSub expression stack full");
      exit(-1);
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
      printf("\nExp stack empty on pop");
      exit(-1);
    }
}

////////////////////////////////////////////////////////////////////////////////

void process_token(char *token)
{
  char *tokptr;
  OP_STACK_ENTRY o1;
  OP_STACK_ENTRY o2;
  int opr1, opr2;
  
  printf("\n   T:'%s'", token);

  strcpy(o1.name, token);
  o1.type = expression_type;
  
  // Another token has arrived, process it using the shunting algorithm
  // First, check the stack for work to do

  o2 = op_stack_top();
  opr1 = operator_precedence(o1.name);
  opr2 = operator_precedence(o2.name);

    if( strcmp(o1.name, ",")==0 )
    {
      while( (strlen(op_stack_top().name) != 0) &&
	     strcmp(op_stack_top().name, "(") != 0 )
	{
	  printf("\nPop 2");
	  o2 = op_stack_pop();
	  output_operator(o2);
	}

      //output_marker("-------- Comma 2");
      output_sub_start();
      
      // Commas delimit sub expressions, reset the expression type.
      expression_type = NOBJ_VARTYPE_UNKNOWN;
      return;
    }

  if( strcmp(o1.name, "(")==0 )
    {
      OP_STACK_ENTRY o;

      //output_marker("--------- Sub 1");
      output_sub_start();
      
      strcpy(o.name, "(");
      o.type = NOBJ_VARTYPE_UNKNOWN;
      
      op_stack_push(o);

      // Sub expression, push the expression type and process the sub expression
      // as a new one
      exp_type_push(expression_type);
      expression_type = NOBJ_VARTYPE_UNKNOWN;
      
      return;
    }

  if( strcmp(o1.name, ")")==0 )
    {
      //output_marker("----------- Sub E 1");
	    
      while(  (strcmp(op_stack_top().name, "(") != 0) && (strlen(op_stack_top().name)!=0) )
	{
	  printf("\nPop 3");
	  o2 = op_stack_pop();
	  output_operator(o2);
	}

      if( strlen(op_stack_top().name)==0 )
	{
	  // Mismatched parentheses
	  printf("\nMismatched parentheses");
	}

      printf("\nPop 4");
      o2 = op_stack_pop();

      //output_marker("-------- Sub E 2");
      output_sub_end();
      
      if( strcmp(o2.name, "(") != 0 )
	{
	  printf("\n**** Should be left parenthesis");
	}

      if( token_is_function(op_stack_top().name, &tokptr) )
	{
	  printf("\nPop 5");
	  o2 = op_stack_pop();
	  output_operator(o2);
	}

      expression_type = exp_type_pop();

      output_sub_end();
      //output_marker("-------- Sub E 3");
      return;
    }

#define OP_PREC(OP) (operator_precedence(OP.name))
  
  if( token_is_operator(o1.name, &(tokptr)) )
    {
      printf("\nToken is operator o1 name:%s o2 name:%s", o1.name, o2.name);
      printf("\nopr1:%d opr2:%d", opr1, opr2);
      
      while( (strlen(op_stack_top().name) != 0) && (strcmp(op_stack_top().name, ")") != 0 ) &&
	     ( OP_PREC(op_stack_top()) > opr1) || ((opr1 == OP_PREC(op_stack_top()) && operator_left_assoc(o1.name)))
	       )
	{
	  printf("\nPop 1");
	  
	  o2 = op_stack_pop();
	  opr1 = operator_precedence(o1.name);
	  opr2 = operator_precedence(o2.name);

	  output_operator(o2);
	}

      printf("\nPush 1");
      
      strcpy(o1.name, tokptr);
      o1.type = NOBJ_VARTYPE_UNKNOWN;
      o1.req_type = expression_type;
      op_stack_push(o1);
      return;
    }
  
  if( token_is_float(o1.name) )
    {

      o1.type = NOBJ_VARTYPE_FLT;

      modify_expression_type(o1.type);

      o1.req_type = expression_type;
      
      output_float(o1);
      return;
    }

  if( token_is_integer(o1.name) )
    {
      o1.type = NOBJ_VARTYPE_INT;
      
      modify_expression_type(o1.type);
      o1.req_type = expression_type;
      output_integer(o1);
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
	  modify_expression_type(type);
	  o1.req_type = expression_type;
	  o1.type = expression_type;
	}
      else
	{
	  // Syntax error
	}
      
      // The type of the variable will affect the expression type
      
      output_variable(o1);
      return;
    }

  if( token_is_string(o1.name) )
    {
      output_string(o1);
      return;
    }
  
  if( token_is_function(o1.name, &tokptr) )
    {
      strcpy(o1.name, tokptr);
      o1.type = expression_type;
            o1.req_type = expression_type;
      op_stack_push(o1);
      return;
    }

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
//
//
////////////////////////////////////////////////////////////////////////////////

void process_expression(char *line)
{
  char token[NOPL_MAX_TOKEN+1];
  int i;
  int op_delimiter = 0;
  char cap_string[NOPL_MAX_TOKEN+1];
  int capture_string = 0;
  char ss[2];
  int end_cap_later = 0;

  fprintf(ofp, "\n==========================");
  fprintf(ofp, "\n%s", line);
  fprintf(ofp, "\n==========================");

  output_expression_start();
  
  // The type of an expression is initially unknown
  expression_type = NOBJ_VARTYPE_UNKNOWN;
  
  if( strstr(line, "GLOBAL") != NULL )
    {
      output_qcode_variable(line);
      return;
    }

  if( strstr(line, "LOCAL") != NULL )
    {
      output_qcode_variable(line);
      return;
    }
  
  // The line is tokenised and treated as an expression.
  // The expression is converted to RPN and that generates the
  // QCode.

  token[0] = '\0';

  while( *line != '\0' )
    {
      //printf("\n  F:'%s'", line);
      while( isspace(*line) )
	{
	  ss[1] = '\0';
	  ss[0] = *line;
	  if( capture_string )
	    {
	      strcat(cap_string, ss);
	    }

	  line++;
	}

      op_delimiter = 0;

      
      while((strlen(token) < NOPL_MAX_TOKEN) && !(is_delimiter(*line)) && (*line != '\0') )
	{

	  ss[1] = '\0';
	  ss[0] = *line;
	  strcat(token, ss);

	  if( capture_string )
	    {
	      strcat(cap_string, ss);
	    }
	  
	  line++;
	}

      if( capture_string )
	{
	  ss[0] = *line;
	  strcat(cap_string, ss);
	}
      
      if( (*line) == '\"' )
	{
	  if( capture_string )
	    {
	      // End of string
	      capture_string = 1;
	      end_cap_later = 1;
	      process_token(cap_string);
	    }
	  else
	    {
	      // Start of string
	      cap_string[0] = *line;
	      cap_string[1] = '\0';
	      capture_string = 1;
	    }
	}
      
      if( is_op_delimiter(*(line)) )
	{
	  ss[1] = '\0';
	  ss[0] = *(line);
	  
	  op_delimiter = 1;
	}

      line++;
      
      if( strlen(token) > 0 )
	{
	  if( !capture_string )
	    {
	      process_token(token);
	    }
	  token[0] = '\0';
	}

      if( op_delimiter )
	{
	  if( !capture_string )
	    {
	      process_token(ss);
	    }
	  token[0] = '\0';
	}

      if( end_cap_later )
	{
	  capture_string = 0;
	  end_cap_later = 0;
	}
    }

  // Now finalise the translation
  op_stack_finalise();
}

void process_line(char *line)
{
  // Process any expressions
  // Separate expressions are delimited by ':'
  process_expression(line);
}

void translate_file(FILE *fp, FILE *ofp)
{
  char line[MAX_NOPL_LINE+1];
  
  // Read lines from file and translate each line as a unit
  // Lines are separated by cr or ':'
  
  while(!feof(fp) )
    {
      if( fgets(line, MAX_NOPL_LINE, fp) == NULL )
	{
	  break;
	}

      //printf("\nL:'%s'", line);

      // Remove trailing newline
      while( isspace(line[strlen(line)-1]) )
	{
	  line[strlen(line)-1] = '\0';
	}

      //printf("\nL:'%s'", line);


      if( defline )
	{
	  // This is the definition of the procedure
	  printf("\nProcedure definition:'%s'", line);
	  defline = 0;
	}
      else
	{
	  // Process line
	  process_line(line);
	  //process_token("EOL");
	}
    }
}


int main(int argc, char *argv[])
{
  FILE *fp;
  FILE *ofp;

  init_output();
  
  // Open file and process on a line by line basis
  fp = fopen(argv[1], "r");

  if( fp == NULL )
    {
      printf("\nCould not open '%s'", argv[1]);
      exit(-1);
    }

  ofp = fopen("out.opl.tran", "w");
  
  translate_file(fp, ofp);

  fclose(fp);
  fclose(ofp);

  uninit_output();

  printf("\n");
  
}

