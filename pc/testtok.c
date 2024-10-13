#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <inttypes.h>
#include "nopl.h"
#include "newopl.h"
#include "nopl_obj.h"

FILE *fp;

// Tokenise OPL
// one argument: an OPL file

// reads next composite line into buffer
char cline[MAX_NOPL_LINE];
int cline_i = 0;
FILE *ofp;
FILE *chkfp;

char current_expression[200];
int first_token = 1;

#define MAX_EXP_TYPE_STACK  20

NOBJ_VARTYPE exp_type_stack[MAX_EXP_TYPE_STACK];
int exp_type_stack_ptr = 0;

#define SAVE_I     1
#define NO_SAVE_I  0

////////////////////////////////////////////////////////////////////////////////
//
// Expression type is reset for each line and also for sub-lines separated by colons
//
// This is a global to avoid passing it down to every function in the translate call stack.
// If translating is ever to be a parallel process then that will have to change.

NOBJ_VARTYPE expression_type = NOBJ_VARTYPE_UNKNOWN;

char type_to_char(NOBJ_VARTYPE t);
int check_expression(void);

////////////////////////////////////////////////////////////////////////////////

#define MAX_EXP_BUFFER   200

enum
  {
   EXP_BUFF_ID_TKN = 1,
   EXP_BUFF_ID_SUB_START,
   EXP_BUFF_ID_SUB_END,
   EXP_BUFF_ID_VARIABLE,
   EXP_BUFF_ID_INTEGER,
   EXP_BUFF_ID_FLT,
   EXP_BUFF_ID_STR,
   EXP_BUFF_ID_FUNCTION,
   EXP_BUFF_ID_OPERATOR,
   EXP_BUFF_ID_AUTOCON,
   EXP_BUFF_ID_COMMAND,
   EXP_BUFF_ID_MAX,
  };

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
   "EXP_BUFF_ID_AUTOCON",
   "EXP_BUFF_ID_COMMAND",
   "EXP_BUFF_ID_MAX",
  };


// Per-expression
// Indices start at 1, 0 is 'no p'
int node_id_index = 1;

EXP_BUFFER_ENTRY exp_buffer[MAX_EXP_BUFFER];
int exp_buffer_i = 0;

EXP_BUFFER_ENTRY exp_buffer2[MAX_EXP_BUFFER];
int exp_buffer2_i = 0;

////////////////////////////////////////////////////////////////////////////////

struct _FN_INFO
{
  char *name;
  int command;         // 1 if command, 0 if function
  char *argtypes;
  char *resulttype;
  uint8_t qcode;
}
  fn_info[] =
    {
     { "EOL",      0, "ii",       "f", 0x00 },
     //{ "=",      0,   "ii",       "f", 0x00 },
     { "ABS",      0, "f",       "f", 0x00 },
     { "ACOS",     0, "f",       "f", 0x00 },
     { "ADDR",     0, "ii",       "f", 0x00 },
     { "APPEND",   1, "ii",       "f", 0x00 },
     { "ASC",      0, "i",        "s", 0x00 },
     { "ASIN",     0, "f",       "f", 0x00 },
     { "AT",       1, "ii",       "f", 0x4C },
     { "ATAN",     0, "f",       "f", 0x00 },
     { "BACK",     1, "ii",       "f", 0x00 },
     { "BEEP",     1, "ii",       "f", 0x00 },
     { "BREAK",    1, "ii",       "f", 0x00 },
     { "CHR$",     0, "s",        "i", 0x00 },
     { "CLOCK",    0, "ii",       "f", 0x00 },
     { "CLOSE",    1, "ii",       "f", 0x00 },
     { "CLS",      1, "ii",       "f", 0x00 },
     { "CONTINUE", 1, "ii",       "f", 0x00 },
     { "COPY",     1, "ii",       "f", 0x00 },
     { "COPYW",    1, "ii",       "f", 0x00 },
     { "COS",      0, "f",        "f", 0x00 },
     { "COUNT",    0, "ii",       "f", 0x00 },
     { "CREATE",   1, "ii",       "f", 0x00 },
     { "CURSOR",   1, "ii",       "f", 0x00 },
     { "DATIM$",   0, "ii",       "f", 0x00 },
     { "DAY",      0, "",         "i", 0x00 },
     { "DAYNAME$", 0, "ii",       "f", 0x00 },
     { "DAYS",     0, "ii",       "f", 0x00 },
     { "DEG",      0, "ii",       "f", 0x00 },
     { "DELETE",   1, "ii",       "f", 0x00 },
     { "DELETEW",  1, "ii",       "f", 0x00 },
     { "DIR$",     0, "ii",       "f", 0x00 },
     { "DIRW$",    0, "ii",       "f", 0x00 },
     { "DISP",     0, "ii",       "f", 0x00 },
     { "DOW",      0, "ii",       "f", 0x00 },
     { "EDIT",     1, "ii",       "f", 0x00 },
     { "EOF",      0, "ii",       "f", 0x00 },
     { "ERASE",    1, "ii",       "f", 0x00 },
     { "ERR",      0, "ii",       "f", 0x00 },
     { "ERR$",     0, "ii",       "f", 0x00 },
     { "ESCAPE",   1, "ii",       "f", 0x00 },
     { "EXIST",    0, "ii",       "f", 0x00 },
     { "EXP",      0, "f",        "f", 0x00 },
     { "FIND",     0, "ii",       "f", 0x00 },
     { "FINDW",    0, "ii",       "f", 0x00 },
     { "FIRST",    1, "ii",       "f", 0x00 },
     { "FIX$",     0, "ii",       "f", 0x00 },
     { "FLT",      0, "i",        "f", 0x00 },
     { "FREE",     0, "ii",       "f", 0x00 },
     { "GEN$",     0, "ii",       "f", 0x00 },
     { "GET",      0, "ii",       "f", 0x00 },
     { "GET$",     0, "ii",       "f", 0x00 },
     { "GLOBAL",   1, "ii",       "f", 0x00 },
     { "GOTO",     1, "ii",       "f", 0x00 },
     { "HEX$",     0, "ii",       "f", 0x00 },
     { "HOUR",     0, "",         "i", 0x00 },
     { "IABS",     0, "i",        "i", 0x00 },
     { "INPUT",    1, "ii",       "f", 0x00 },
     { "INT",      0, "f",        "i", 0x00 },
     { "INTF",     0, "ii",       "f", 0x00 },
     { "KEY",      0, "ii",       "f", 0x00 },
     { "KEY$",     0, "ii",       "f", 0x00 },
     { "KSTAT",    1, "ii",       "f", 0x00 },
     { "LAST",     1, "ii",       "f", 0x00 },
     { "LEFT$",    0, "ii",       "f", 0x00 },
     { "LEN",      0, "ii",       "f", 0x00 },
     { "LN",       0, "f",         "f", 0x00 },
     { "LOC",      0, "ii",       "f", 0x00 },
     { "LOCAL",    1, "ii",       "f", 0x00 },
     { "LOG",      0, "ii",       "f", 0x00 },
     { "LOWER$",   0, "ii",       "f", 0x00 },
     { "LPRINT",   1, "ii",       "f", 0x00 },
     { "MAX",      0, "ii",        "f", 0x00 },
     { "MEAN",     0, "ii",       "f", 0x00 },
     { "MENU",     0, "ii",       "f", 0x00 },
     { "MENUN",    0, "ii",       "f", 0x00 },
     { "MID$",     0, "ii",       "f", 0x00 },
     { "MIN",      0, "ii",       "f", 0x00 },
     { "MINUTE",   0, "",         "i", 0x00 },
     { "MONTH",    0, "",         "i", 0x00 },
     { "MONTH$",   0, "ii",       "f", 0x00 },
     { "NEXT",     0, "ii",       "f", 0x00 },
     { "NUM$",     0, "ii",       "f", 0x00 },
     { "OFF",      1, "ii",       "f", 0x00 },
     { "OPEN",     1, "ii",       "f", 0x00 },
     { "ONERR",    1, "ii",       "f", 0x00 },
     { "PAUSE",    1, "ii",       "f", 0x00 },
     { "PEEKB",    0, "ii",       "f", 0x00 },
     { "PEEKW",    0, "ii",       "f", 0x00 },
     { "PI",       0, "ii",       "f", 0x00 },
     { "POKEB",    1, "ii",       "f", 0x00 },
     { "POKEW",    1, "ii",       "f", 0x00 },
     { "POS",      0, "ii",       "f", 0x00 },
     { "POSITION", 1, "ii",       "f", 0x00 },
     { "PRINT",    1, "i",        "v", 0x00 },
     { "RAD",      0, "ii",       "f", 0x00 },
     { "RAISE",    1, "ii",       "f", 0x00 },
     { "RANDOMIZE",1, "ii",       "f", 0x00 },
     { "RECSIZE",  0, "ii",       "f", 0x00 },
     { "REM",      1, "ii",       "f", 0x00 },
     { "RENAME",   1, "ii",       "f", 0x00 },
     { "REPT$",    0, "ii",       "f", 0x00 },
     { "RETURN",   1, "ii",       "f", 0x00 },
     { "RIGHT$",   0, "ii",       "f", 0x00 },
     { "RND",      0, "ii",       "f", 0x00 },
     { "SCI$",     0, "ii",       "f", 0x00 },
     { "SECOND",   0, "",         "i", 0x00 },
     { "SIN",      0, "f",        "f", 0x00 },
     { "SPACE",    0, "ii",       "f", 0x00 },
     { "SQR",      0, "f",         "f", 0x00 },
     { "STD",      0, "ii",        "f", 0x00 },
     { "STOP",     1, "ii",        "f", 0x00 },
     { "SUM",      0, "ii",        "f", 0x00 },
     { "TAN",      0, "f",         "f", 0x00 },
     { "TRAP",     1, "ii",        "f", 0x00 },
     { "UDG",      1, "iiiiiiiii", "v", 0x00 },
     { "UPDATE",   1, "ii",        "f", 0x00 },
     { "UPPER$",   0, "ii",        "f", 0x00 },
     { "USE",      1, "ii",        "f", 0x00 },
     { "USR",      0, "ii",        "v", 0x00 },
     { "USR$",     0, "ii",        "f", 0x00 },
     { "VAL",      0, "ii",        "f", 0x00 },
     { "VAR",      0, "ii",        "f", 0x00 },
     { "VIEW",     0, "ii",        "f", 0x00 },
     { "WEEK",     0, "ii",        "f", 0x00 },
     { "YEAR",     0, "ii",        "f", 0x00 },
    };


#define NUM_FUNCTIONS (sizeof(fn_info)/sizeof(struct _FN_INFO))

int token_is_function(char *token, char **tokstr)
{
  for(int i=0; i<NUM_FUNCTIONS; i++)
    {
      if( strcmp(token, fn_info[i].name) == 0 )
	{
	  *tokstr = &(fn_info[i].name[0]);
	  
	  fprintf(ofp,"\n%s is function", token);
	  return(1);
	}
    }
  return(0);
}

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
  char          *name;
  int           precedence;
  int           left_assoc;
  int           immutable;                // Is the operator type mutable?
  int           assignment;               // Special code for assignment
  NOBJ_VARTYPE  type[MAX_OPERATOR_TYPES];
  int           qcode;                     // Easily translatable qcodes
} OP_INFO;

OP_INFO  op_info[] =
  {
    // Array dereference internal operator
   { "@",    9, 0,   MUTABLE_TYPE, 1, {NOBJ_VARTYPE_INT, NOBJ_VARTYPE_FLT, NOBJ_VARTYPE_STR} },
   { "=",    1, 0,   MUTABLE_TYPE, 1, {NOBJ_VARTYPE_INT, NOBJ_VARTYPE_FLT, NOBJ_VARTYPE_STR} },
   { ":=",   1, 0,   MUTABLE_TYPE, 1, {NOBJ_VARTYPE_INT, NOBJ_VARTYPE_FLT, NOBJ_VARTYPE_STR} },
   { "+",    3, 0,   MUTABLE_TYPE, 0, {NOBJ_VARTYPE_INT, NOBJ_VARTYPE_FLT, NOBJ_VARTYPE_STR} },
   { "-",    3, 0,   MUTABLE_TYPE, 0, {NOBJ_VARTYPE_INT, NOBJ_VARTYPE_FLT, NOBJ_VARTYPE_STR} },
   { "*",    5, 1,   MUTABLE_TYPE, 0, {NOBJ_VARTYPE_INT, NOBJ_VARTYPE_FLT, NOBJ_VARTYPE_INT} },
   { "/",    5, 1,   MUTABLE_TYPE, 0, {NOBJ_VARTYPE_INT, NOBJ_VARTYPE_FLT, NOBJ_VARTYPE_INT} },
   { ">",    5, 1,   MUTABLE_TYPE, 0, {NOBJ_VARTYPE_INT, NOBJ_VARTYPE_FLT, NOBJ_VARTYPE_STR} },
   { "AND",  5, 1,   MUTABLE_TYPE, 0, {NOBJ_VARTYPE_INT, NOBJ_VARTYPE_FLT, NOBJ_VARTYPE_INT} },
   // (Handle bitwise on integer, logical on floats somewhere)
   //{ ",",  0, 0 }, /// Not used?
    
   // LZ only
   { "+%",   5, 1, IMMUTABLE_TYPE, 0, {NOBJ_VARTYPE_FLT, NOBJ_VARTYPE_FLT, NOBJ_VARTYPE_FLT} },
   { "-%",   5, 1, IMMUTABLE_TYPE, 0, {NOBJ_VARTYPE_FLT, NOBJ_VARTYPE_FLT, NOBJ_VARTYPE_FLT} },
  };


//------------------------------------------------------------------------------
//

char cln[300];

char *cline_now(void)
{
  sprintf(cln, "%s", &(cline[cline_i]));

  return(cln);
}

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
      EXP_BUFFER_ENTRY token = exp_buffer[i];
      
      fprintf(ofp, "\n(%16s) N%d %-24s %c rq:%c %s", __FUNCTION__, token.node_id, exp_buffer_id_str[exp_buffer[i].buf_id], type_to_char(token.op.type), type_to_char(token.op.req_type), exp_buffer[i].name);
      
      fprintf(ofp, "  %d:", token.p_idx);
      for(int pi=0; pi<token.p_idx; pi++)
	{
	  fprintf(ofp, " %d", token.p[pi]);
	}
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
      EXP_BUFFER_ENTRY token = exp_buffer2[i];

      if( (exp_buffer2[i].buf_id < 0) || (exp_buffer2[i].buf_id > EXP_BUFF_ID_MAX) )
	{
	  printf("\nN%d buf_id invalid", token.node_id);
	}
      
      fprintf(ofp, "\n(%16s) N%d %-24s %c rq:%c %s", __FUNCTION__, token.node_id, exp_buffer_id_str[exp_buffer2[i].buf_id], type_to_char(token.op.type), type_to_char(token.op.req_type), exp_buffer2[i].name);
      
      fprintf(ofp, "  %d:", token.p_idx);
      for(int pi=0; pi<token.p_idx; pi++)
	{
	  fprintf(ofp, " %d", token.p[pi]);
	}
    }
  fprintf(ofp, "\n=================");
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

    case NOBJ_VARTYPE_VOID:
      c = 'v';
      break;
      
    default:
      c = '?';
      break;
    }
  
  return(c);
}

NOBJ_VARTYPE char_to_type(char ch)
{
  NOBJ_VARTYPE ret_t = '?';
  
  switch(ch)
    {
    case 'i':
      ret_t = NOBJ_VARTYPE_INT;
      break;

    case 'f':
      ret_t = NOBJ_VARTYPE_FLT;
      break;

    case 's':
      ret_t = NOBJ_VARTYPE_INT;
      break;

    case 'v':
      ret_t = NOBJ_VARTYPE_VOID;
      break;
    }

  return(ret_t);
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

#define NUM_OPERATORS (sizeof(op_info)/sizeof(struct _OP_INFO))
void output_float(OP_STACK_ENTRY token)
{
  printf("\nop float");
  fprintf(ofp, "\n(%16s) %s %c %c %s", __FUNCTION__, type_stack_str(), type_to_char(token.type), type_to_char(token.req_type), token.name);
  add_exp_buffer_entry(token, EXP_BUFF_ID_FLT);
}

void output_integer(OP_STACK_ENTRY token)
{
  printf("\nop integer");
  fprintf(ofp, "\n(%16s) %s %c %c %s", __FUNCTION__, type_stack_str(), type_to_char(token.type), type_to_char(token.req_type), token.name);
  add_exp_buffer_entry(token, EXP_BUFF_ID_INTEGER);
}

void output_operator(OP_STACK_ENTRY op)
{
  char *tokptr;
  
  printf("\nop operator");
  fprintf(ofp, "\n(%16s) %s %c %c %s", __FUNCTION__, type_stack_str(), type_to_char(op.type), type_to_char(op.req_type), op.name);
  if( token_is_function(op.name, &tokptr) )
    {
      add_exp_buffer_entry(op, EXP_BUFF_ID_FUNCTION);
    }
  else
    {
      add_exp_buffer_entry(op, EXP_BUFF_ID_OPERATOR);
    }
}

void output_function(OP_STACK_ENTRY op)
{
  printf("\nop function");
  fprintf(ofp, "\n(%16s) %s %c %c %s", __FUNCTION__, type_stack_str(), type_to_char(op.type), type_to_char(op.req_type), op.name);
  add_exp_buffer_entry(op, EXP_BUFF_ID_FUNCTION);
}

void output_variable(OP_STACK_ENTRY op)
{
  printf("\nop variable");
  fprintf(ofp, "\n(%16s) %s %c %c %s", __FUNCTION__, type_stack_str(), type_to_char(op.type), type_to_char(op.req_type), op.name);
  add_exp_buffer_entry(op, EXP_BUFF_ID_VARIABLE);
}

void output_string(OP_STACK_ENTRY op)
{
  printf("\nop string");
  fprintf(ofp, "\n(%16s) %s %c %c %s", __FUNCTION__, type_stack_str(), type_to_char(op.type), type_to_char(op.req_type), op.name); 
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
#if 1
  OP_STACK_ENTRY op;
  
  printf("\nSub expression start");
  fprintf(ofp, "\n(%16s)", __FUNCTION__);

  strcpy(op.name,  "");
  op.type = NOBJ_VARTYPE_UNKNOWN;
  add_exp_buffer_entry(op, EXP_BUFF_ID_SUB_START);
#endif
}

void output_sub_end(void)
{
#if 1
  OP_STACK_ENTRY op;
  
  printf("\nSub expression end");
  fprintf(ofp, "\n(%16s)", __FUNCTION__);

  strcpy(op.name, "");
  op.type = NOBJ_VARTYPE_UNKNOWN;
  add_exp_buffer_entry(op, EXP_BUFF_ID_SUB_END);
#endif
}

void output_expression_start(char *expr)
{
  strcpy(current_expression, expr);
  
  if( strlen(expr) > 0 )
    {
      printf("\nExpression start");
      fprintf(ofp, "\n========================================================");
      fprintf(ofp, "\n%s", expr);
      fprintf(chkfp, "\n\n\n%s", expr);
      
      fprintf(ofp, "\n========================================================");

      fprintf(ofp, "\n(%16s)", __FUNCTION__);
      
      // We have a new expression, process the previous one which will be in the
      // buffer
      
      //  expression_tree_process(expr);
      
    }
  
  // Clear the buffer ready for the new expression that has just come in
  clear_exp_buffer();

  first_token = 1;
}

////////////////////////////////////////////////////////////////////////////////

void syntax_error(char *fmt, ...)
{
  va_list valist;
  char line[80];
  
  va_start(valist, fmt);

  vsprintf(line, fmt, valist);
  va_end(valist);

  printf("\n%s", cline);
  printf("\n");
  for(int i=0; i<cline_i; i++)
    {
      printf(" ");
    }
  printf("^");
  
  printf("\n\n   %s", line);
}

////////////////////////////////////////////////////////////////////////////////

int scan_expression(void);
int check_function(void);
int scan_function(char *cmd_dest);

////////////////////////////////////////////////////////////////////////////////

int next_composite_line(FILE *fp)
{
  int all_spaces = 1;
  
  if( fgets(cline, MAX_NOPL_LINE, fp) == NULL )
    {
      cline_i = 0;
      return(0);
    }

  cline_i = 0;

  return(1);
}

////////////////////////////////////////////////////////////////////////////////

void drop_space()
{
  while( isspace(cline[cline_i]) && (cline[cline_i] != '\0') )
    {
      cline_i++;
    }
#if 0
  if( cline_i > 0 )
    {
      cline_i--;
    }
#endif
}

////////////////////////////////////////////////////////////////////////////////

// Scan for a literal
// Leading space means drop spaces before looking for the literal,
// trailing mena sdrop spaces after finding it

int scan_literal(char *lit)
{
  char *origlit = lit;

  printf("\n%s:lit='%s' '%s'", __FUNCTION__, lit, &(cline[cline_i]));
  
  if( *lit == ' ' )
    {
      lit++;
      drop_space();
    }

  printf("\n%s:After drop space:%s", __FUNCTION__, &(cline[cline_i]));

  while( (*lit != '\0') && (*lit != ' ') )
    {
      printf("\n%s:while loop:%s", __FUNCTION__, &(cline[cline_i]));
      if( cline[cline_i] == '\0' )
	{
	  syntax_error("Bad literal '%s'", origlit);
	  return(0);
	}
      
      if( *lit != cline[cline_i] )
	{
	  // Not a match, fail
	  return(0);
	}
      
      lit++;
      cline_i++;
    }
  
  if( *lit == ' ' )
    {
      lit++;
      drop_space();
    }
  
  // reached end of literal string , all ok
  return(1);
}

////////////////////////////////////////////////////////////////////////////////

// Check for a literal string.
// Leading space means drop spaces before looking for the literal,
// trailing mena sdrop spaces after finding it

int check_literal(int save, char *lit)
{
  int save_cli = cline_i;

  printf("\n%s:lit='%s' ' %s'", __FUNCTION__, lit, &(cline[cline_i]));

  if( *lit == ' ' )
    {
      printf("\n    dropping space");
      lit++;
      drop_space();
    }

  printf("\n%s:After drop space:'%s' '%s'", __FUNCTION__, lit, &(cline[cline_i]));

  if( cline[cline_i] == '\0' )
    {
      printf("\n  ret0 Empty test string");
      if( save )
	{
	  cline_i = save_cli;
	}
      return(0);
    }
  
  while( (*lit != '\0') && (cline[cline_i] != '\0'))
    {
      if( *lit != cline[cline_i] )
	{
	  printf("\n  '%c' != '%c'", *lit, cline[cline_i]);
	  // Not a match, fail

	  printf("\nret0");
	  if( save )
	    {
	      cline_i = save_cli;
	    }
	  return(0);
	}
      lit++;
      cline_i++;
    }

  printf("\n%s:After while():%s", __FUNCTION__, &(cline[cline_i]));
  
  if( cline[cline_i-1] == '\0' )
    {
      if( save)
	{
	  cline_i = save_cli;
	}
      printf("\nret0");
      return(0);
    }
  
  // reached end of literal string , all ok
  if(save)
    {
      cline_i = save_cli;
    }
  printf("\nret1");
  return(1);

}



// Scans for a variable name string part
int scan_vname(char *vname_dest)
{
  char vname[300];
  int vname_i = 0;
  char ch;

  printf("\n%s: '%s'", __FUNCTION__, &(cline[cline_i]));

  drop_space();
  
  if( isalpha(ch = cline[cline_i++]) )
    {
      vname[vname_i++] = ch;
      
      while( isalnum(ch = cline[cline_i]) )
	{
	vname[vname_i++] = ch;
	cline_i++;
	}

      vname[vname_i] = '\0';
      
      strcpy(vname_dest, vname);
      printf("\n%s: ret1 '%s'", __FUNCTION__, vname);
      return(1);
    }

  printf("\n%s: ret0", __FUNCTION__);
  strcpy(vname_dest, "");
  return(0);
}

//
// Checks for a variable name string part
//

int check_vname(int save)
{
  int save_cli = cline_i;
  printf("\n%s '%s':", __FUNCTION__, &(cline[cline_i]));

  drop_space();
  
  if( isalpha(cline[cline_i]) )
    {
      cline_i++;
      
      while( isalnum(cline[cline_i]) )
	{
	  cline_i++;
	}

      
      if( save )
	{
	  cline_i = save_cli;
	}

      printf("\n%s ret1 '%s':", __FUNCTION__, &(cline[cline_i]));
      return(1);
    }

  if( save )
    {
      cline_i = save_cli;
    }
  
  printf("\n%s ret0 '%s':", __FUNCTION__, &(cline[cline_i]));
  return(0);
}

// Scan variable reference
// This puts a variable ref in the output stream. handles arrays
// and puts appropriate codes for array indices in stream
//
// The variable name is captured and flags which specify the type.
// The expressions that are array indices aren't captured.
// They will be expressions on the stack and the type flags will ensure an array
// type qcode is set for the variable reference.

int scan_variable(char *variable_dest)
{
  char vname[300];
  char chstr[2];
  int var_is_string  = 0;
  int var_is_integer = 0;
  int var_is_float   = 0;
  int var_is_array   = 0;

  printf("\n%s:", __FUNCTION__);
 
  chstr[1] = '\0';
  
  if( scan_vname(vname) )
    {
      printf("\n%s: '%s'", __FUNCTION__, &(cline[cline_i]));
      
      // Could just be a vname
      switch( chstr[0] = cline[cline_i] )
	{
	case '%':
	  var_is_integer = 1;
	  strcat(vname, chstr);
	  cline_i++;
	  break;
	  
	case '$':
	  var_is_string = 1;
	  strcat(vname, chstr);
	  cline_i++;
	  break;

	default:
	  var_is_float = 1;
	  break;
	}

      // Is it an array?
      printf("\n%s: Ary test '%s'", __FUNCTION__, &(cline[cline_i]));
      if( check_literal(SAVE_I,"(") )
	{
	  printf("\n%s: is array", __FUNCTION__);
	  
	  var_is_array = 1;
	  
	  // Add token to output stream for index or indices
	  scan_expression();

	  // Could be string array, which has two expressions in
	  // the brackets
	  if( check_literal(SAVE_I," ,") )
	    {
	      if( var_is_string )
		{
		  // All OK, string array
		  scan_literal(" ,");
		  scan_expression();
		}
	      else
		{
		  syntax_error("Two indices in non-string variable name");
		}
	    }

	  if( scan_literal(" )") )
	    {
	      return(1);
	    }
	  
	}
      
      printf("\n%s:ret1 vname='%s' is str:%d int:%d flt:%d ary:%d", __FUNCTION__,
	     vname,
	     var_is_string,
	     var_is_integer,
	     var_is_float,
	     var_is_array
	     );
      return(1);
      
    }
  
  return(0);
}

int check_variable(int save)
{
  int save_cli = cline_i;
  
  char vname[300];
  char chstr[2];
  int var_is_string  = 0;
  int var_is_integer = 0;
  int var_is_float   = 0;
  int var_is_array   = 0;

  printf("\n%s:", __FUNCTION__);
 
  chstr[1] = '\0';
  
  if( check_vname(NO_SAVE_I) )
    {
      printf("\n%s: '%s'", __FUNCTION__, &(cline[cline_i]));
      
      // Could just be a vname
      switch( chstr[0] = cline[cline_i] )
	{
	case '%':
	  var_is_integer = 1;
	  strcat(vname, chstr);
	  cline_i++;
	  break;
	  
	case '$':
	  var_is_string = 1;
	  strcat(vname, chstr);
	  cline_i++;
	  break;

	default:
	  var_is_float = 1;
	  break;
	}

      // Is it an array?
      printf("\n%s: Ary test '%s'", __FUNCTION__, &(cline[cline_i]));
      
      if( check_literal(NO_SAVE_I,"(") )
	{
	  printf("\n%s: is array", __FUNCTION__);
	  
	  var_is_array = 1;
	  
	  // Add token to output stream for index or indices
	  check_expression();

	  // Could be string array, which has two expressions in
	  // the brackets
	  if( check_literal(NO_SAVE_I," ,") )
	    {
	      if( var_is_string )
		{
		  // All OK, string array
		  if( check_literal(NO_SAVE_I, " ,") )
		    {
		      
		      if( check_expression() )
			{
			  // All OK
			}
		      else
			{
			  cline_i = save_cli;
			  return(0);
			}
		    }
		  else
		    {
		      cline_i = save_cli;
		      return(0);
		    }
		}
	      else
		{
		  cline_i = save_cli;
		  return(0);
		}
	    }
	  
	  if( check_literal(NO_SAVE_I, " )") )
	    {
	      cline_i = save_cli;
	      return(1);
	    }
	  
	}
      
      printf("\n%s:ret1 vname='%s' is str:%d int:%d flt:%d ary:%d", __FUNCTION__,
	     vname,
	     var_is_string,
	     var_is_integer,
	     var_is_float,
	     var_is_array
	     );
      if(save)
	{
	  cline_i = save_cli;
	}
      
      return(1);
      
    }

  if(save)
    {
      cline_i = save_cli;
    }
  return(0);
}

int check_variable2(int save)
{
  if( check_vname(save) )
    {
      
      return(1);
    }
  return(0);
}

int check_operator(void)
{
  int save_cli = cline_i;
  
  printf("\n%s: %s", __FUNCTION__, cline_now());

  drop_space();
  
  if( check_literal(SAVE_I, ",") )
    {
      cline_i = save_cli;
      return(scan_literal(","));
    }

  for(int i=0; i<NUM_OPERATORS; i++)
    {
      if( strncmp(&(cline[cline_i]), op_info[i].name, strlen(op_info[i].name)) == 0 )
	{
	  // Match
	  
	  printf("\n%s: ret1", __FUNCTION__);
	  cline_i = save_cli;
	  return(1);
	}
    }

  printf("\n%s:ret0", __FUNCTION__);
  cline_i = save_cli;
  return(0);
}

int scan_operator(void)
{
  printf("\n%s: '%s'", __FUNCTION__, cline_now());

  drop_space();
  
  if( check_literal(SAVE_I, ",") )
    {
      return(scan_literal(","));
    }
  
  for(int i=0; i<NUM_OPERATORS; i++)
    {
      if( strncmp(&(cline[cline_i]), op_info[i].name, strlen(op_info[i].name)) == 0 )
	{
	  // Match
	  cline_i += strlen(op_info[i].name);
	  printf("\n%s: ret1 '%s'", __FUNCTION__, cline_now());
	  return(1);
	}
    }

  printf("\n%s: ret0 '%s'", __FUNCTION__, cline_now());
  return(0);
}

int check_integer(void)
{
  printf("\n%s:", __FUNCTION__);
  if( isdigit(cline[cline_i]) )
    {
      return(1);
    }
  
  return(0);
}

int scan_integer(char *intdest)
{
  printf("\n%s:", __FUNCTION__);
  char intval[20];
  char chstr[2];

  intval[0] = '\0';
  
  while( isdigit(chstr[0] = cline[cline_i]) )
    {
      strcat(intval, chstr);
      cline_i++;
    }

  strcpy(intdest, intval);
}

int isfloatdigit(char c)
{
  printf("\n%s:", __FUNCTION__);
  if( isdigit(c) || (c == '.') )
    {
      return(1);
    }
  
  return(0);
}

int check_float(void)
{
  printf("\n%s:", __FUNCTION__);
  if( isfloatdigit(cline[cline_i]) )
    {
      return(1);
    }
  
  return(0);
}

int scan_float(char *fltdest)
{
  printf("\n%s:", __FUNCTION__);
  char intval[20];
  char chstr[2];

  intval[0] = '\0';
  
  while( isfloatdigit(chstr[0] = cline[cline_i]) )
    {
      strcat(intval, chstr);
      cline_i++;
    }

  strcpy(fltdest, intval);
}

int check_number(void)
{
  printf("\n%s:", __FUNCTION__);
  if( check_float() )
    {
      return(1);
    }

  if( check_integer() )
    {
      return(1);
    }

  return(0);
}

int scan_number(void)
{
  printf("\n%s:", __FUNCTION__);
  char fltval[40];

  if( check_float() )
    {
      scan_float(fltval);
      return(1);
    }

  if( check_integer() )
    {
      char intval[40];
      
      scan_integer(intval);
      return(1);
    }

  syntax_error("Not a number");
  return(0);
}

int check_sub_expr(void)
{
  printf("\n%s:", __FUNCTION__);
  
  if( check_literal(SAVE_I," (") )
    {
      printf("\n%s: ret1", __FUNCTION__);
      return(1);
    }

  printf("\n%s: ret0", __FUNCTION__);
  return(0);
}

int scan_sub_expr(void)
{
  printf("\n%s:", __FUNCTION__);
  if( scan_literal(" (") )
    {
      if( scan_expression() )
	{
	  if( scan_literal(" )") )
	    {
	      return(1);
	    }
	}
    }

  syntax_error("Expression error");
  return(0);
}

int check_atom(void)
{
  printf("\n%s:", __FUNCTION__);
  if( check_literal(SAVE_I," \"") )
    {
      // String
      return(1);
    }

  if( check_number() )
    {
      // Int or float
      return(1);
    }

  if( check_variable(SAVE_I) )
    {
      // Variable
      return(1);
    }

  
  return(0);
}

int scan_string(void)
{
  char chstr[2];
  char strval[300];
  printf("\n%s:", __FUNCTION__);

  strval[0] = '\0';
  chstr[1] = '\0';
  
  if( scan_literal(" \"") )
    {
      printf("\n  (in if) '%s'", &(cline[cline_i]));
      
      while(((chstr[0] = cline[cline_i]) != '"') && (cline[cline_i] != '\0') )
	{
	  printf("\n  (in wh) '%s'", &(cline[cline_i]));
	  strcat(strval, chstr);
	  cline_i++;
	  printf("\n  (in wh) '%s'", &(cline[cline_i]));
	}

      
      if( cline[cline_i] == '"' )
	{
	  cline_i++;
	  printf("\n%s: ret1", __FUNCTION__);
	  return(1);
	}
    }

  syntax_error("Bad string");
  return(0);
}

int scan_atom(void)
{
  char vname[300];
  printf("\n%s:", __FUNCTION__);
  
  if( check_literal(SAVE_I," \"") )
    {
      // String
      return(scan_string());
    }

  if( check_number() )
    {
      // Int or float
      return(scan_number());
    }

  if( check_variable(SAVE_I) )
    {
      // Variable
      return(scan_variable(vname));
    }

  syntax_error("Not an atom");  
  return(0);
}

int check_eitem(void)
{
  printf("\n%s:", __FUNCTION__);
  if( check_operator() )
    {
      return(1);
    }
  
  if( check_function() )
    {
      return(1);
    }
  
  if( check_atom() )
    {
      return(1);
    }
  
  if( check_sub_expr() )
    {
      return(1);
    }
  
  return(0);
}

int scan_eitem(void)
{
  char fnval[40];
  printf("\n%s:", __FUNCTION__);
  
  if( check_operator() )
    {
      return(scan_operator() );
    }
  
  if( check_function() )
    {
      return(scan_function(fnval) );
    }
  
  if( check_atom() )
    {
      return(scan_atom());
    }
  
  if( check_sub_expr() )
    {
      return(scan_sub_expr());
    }

  syntax_error("Not an atom");
  return(0);
}


int check_expression(void)
{
  int save_cli = cline_i;
  
  drop_space();
  
  printf("\n%s: '%s'", __FUNCTION__, &(cline[cline_i]));
  if( check_eitem() )
    {
      printf("\n%s:ret1 '%s'", __FUNCTION__, &(cline[cline_i]));
      cline_i = save_cli;
      return(1);
    }

  printf("\n%s:ret0 '%s'", __FUNCTION__, &(cline[cline_i]));
  cline_i = save_cli;
  return(0);
}

int scan_expression(void)
{
  printf("\n%s: '%s'", __FUNCTION__, &(cline[cline_i]));

  drop_space();
  
  while( check_eitem() )
    {
      if( scan_eitem() )
	{
	  // All OK
	}
      else
	{
	  syntax_error("Expression error");
	  printf("\n%s: ret0 '%s'", __FUNCTION__, &(cline[cline_i]));
	  return(0);
	}
    }

  printf("\n%s: ret1 '%s'", __FUNCTION__, &(cline[cline_i]));
  return(1);
}


int check_command(void)
{
  printf("\n%s:", __FUNCTION__);
  for(int i=0; i<NUM_FUNCTIONS; i++)
    {
      if( fn_info[i].command && strncmp(&(cline[cline_i]), fn_info[i].name, strlen(fn_info[i].name)) == 0 )
	{
	  // Match
	  printf("\n%s: ret1=> '%s'", __FUNCTION__, fn_info[i].name);
	  return(1);
	}
    }
  
  return(0);
}

int scan_command(char *cmd_dest)
{
  printf("\n%s:", __FUNCTION__);
  
  for(int i=0; i<NUM_FUNCTIONS; i++)
    {
      if( fn_info[i].command && (strncmp(&(cline[cline_i]), fn_info[i].name, strlen(fn_info[i].name)) == 0) )
	{
	  // Match
	  strcpy(cmd_dest, fn_info[i].name);
	  cline_i += strlen(fn_info[i].name);
	  if( scan_expression() )
	    {
	      printf("\n%s: ret1 =>'%s'", __FUNCTION__, cmd_dest);
	      return(1);
	    }
	  else
	    {
	      printf("\n%s: expression failed", __FUNCTION__);
	      return(0);
	    }
	}
    }

  strcpy(cmd_dest, "");
  return(0);
}

int check_function(void)
{
  for(int i=0; i<NUM_FUNCTIONS; i++)
    {
      if( (!fn_info[i].command) && strncmp(&(cline[cline_i]), fn_info[i].name, strlen(fn_info[i].name)) == 0 )
	{
	  // Match
	  return(1);
	}
    }
  
  return(0);
}

int scan_function(char *cmd_dest)
{
  printf("\n%s:", __FUNCTION__);
  for(int i=0; i<NUM_FUNCTIONS; i++)
    {
      if( !(fn_info[i].command) && (strncmp(&(cline[cline_i]), fn_info[i].name, strlen(fn_info[i].name)) == 0) )
	{
	  // Match
	  strcpy(cmd_dest, fn_info[i].name);
	  cline_i += strlen(fn_info[i].name);
	  return(1);
	}
    }

  strcpy(cmd_dest, "");
  return(0);
}

int check_assignment(void)
{
  printf("\n%s:", __FUNCTION__);
  int save_cli = cline_i;
  
  if( check_variable(NO_SAVE_I) )
    {
      if( check_literal(NO_SAVE_I, " =") )
	{
	  if( check_expression() )
	    {
	      cline_i = save_cli;
	      return(1);
	    }
	}
    }
  
  cline_i = save_cli;
  return(0);
}

int scan_assignment(void)
{
  char vname[300];
  printf("\n%s:", __FUNCTION__);
  
  if( scan_variable(vname) )
    {
      if( scan_literal(" =") )
	{
	  if( scan_expression() )
	    {
	      printf("\n%s: ret1", __FUNCTION__);
	      return(1);
	    }
	}
    }

  syntax_error("Assignment error");
  return(0);
}

int check_line(void)
{
  int save_cli = cline_i;
  
  printf("\n%s:", __FUNCTION__);

  if( check_assignment() )
    {
      cline_i = save_cli;
      return(1);
    }
  if( check_command() )
    {
      cline_i = save_cli;
      return(1);
    }
  if( check_literal(SAVE_I," LOCAL"))
    {
      cline_i = save_cli;
      return(1);
    }
  if( check_literal(SAVE_I," GLOBAL"))
    {
      cline_i = save_cli;
      return(1);
    }
  if( check_literal(SAVE_I," IF"))
    {
      cline_i = save_cli;
      return(1);
    }
  if( check_literal(SAVE_I," ELSE"))
    {
      cline_i = save_cli;
      return(1);
    }
  if( check_literal(SAVE_I," ENDIF"))
    {
      cline_i = save_cli;
      return(1);
    }
  if( check_literal(SAVE_I," DO"))
    {
      cline_i = save_cli;
      return(1);
    }
  if( check_literal(SAVE_I," WHILE"))
    {
      cline_i = save_cli;
      return(1);
    }
  if( check_literal(SAVE_I," REPEAT"))
    {
      cline_i = save_cli;
      return(1);
    }
  if( check_literal(SAVE_I," UNTIL"))
    {
      cline_i = save_cli;
      return(1);
    }

  cline_i = save_cli;
    return(0);
}
  
int scan_line()
{
  char cmdname[300];
  printf("\n%s:", __FUNCTION__);
  
  if( check_assignment() )
    {
      return(scan_assignment());
    }
  
  if( check_command() )
    {
      printf("\n%s:check_command:1 ", __FUNCTION__);
      scan_command(cmdname);
      return(1);
    }
  
  if( check_literal(SAVE_I," LOCAL") )
    {
      scan_literal(" LOCAL");
      return(1);
    }
  
  if( check_literal(SAVE_I," GLOBAL") )
    {
      scan_literal(" GLOBAL");
      return(1);
    }
  
  if( check_literal(SAVE_I," IF") )
    {
      scan_literal(" IF");
      return(1);
    }
  
  if( check_literal(SAVE_I," ELSE") )
    {
      scan_literal(" ELSE");
      return(1);
    }
  
  if( check_literal(SAVE_I," ENDIF") )
    {
      scan_literal(" ENDIF");
      return(1);
    }
  
  if( check_literal(SAVE_I," DO") )
    {
      scan_literal(" DO");
      return(1);
    }
  
  if( check_literal(SAVE_I," WHILE") )
    {
      scan_literal(" WHILE");
      return(1);
    }
  
  if( check_literal(SAVE_I," REPEAT") )
    {
      scan_literal(" REPEAT");
      return(1);
    }
  
  if( check_literal(SAVE_I," UNTIL") )
    { 
      if( scan_literal(" UNTIL") )
	{
	  if( scan_expression() )
	    {
	      printf("\n%s: ret1", __FUNCTION__);
	      return(1);	      
	    }
	}

      printf("\n%s: ret0", __FUNCTION__);
      return(0);
    }

  printf("\n%s: ret0", __FUNCTION__);
  return(0);    
}


////////////////////////////////////////////////////////////////////////////////

int scan_cline()
{
  int ret = 0;
  printf("\n%s:", __FUNCTION__);
  
  drop_space();
  
  while( check_line() && (strlen(&(cline[cline_i])) > 0))
    {
      printf("\n%s: Checked len=%ld, '%s'", __FUNCTION__, strlen(&(cline[cline_i])), &(cline[cline_i]));
      if( !scan_line() )
      {
	printf("\n%s: scan_line==0 len=%ld '%s'", __FUNCTION__, strlen(&(cline[cline_i])), &(cline[cline_i]));
	return(0);
      }
      
      drop_space();
      
      if ( check_literal(SAVE_I,":") )
	{
	  scan_literal(":");
	}
      else
	{
	  if( strlen(&(cline[cline_i])) == 0 )
	    {
	      return(1);
	    }
	  else
	    {
	      return(0);
	    }
	}
      
      drop_space();
    }

  printf("\n%s: after wh len=%ld '%s'", __FUNCTION__, strlen(&(cline[cline_i])), &(cline[cline_i]));
  if( strlen(&(cline[cline_i])) == 0 )
    {
      return(0);
    }
  
  return(0);
}

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[])
{
  char *line = argv[1];
  char varname[200];
  int all_spaces;
  int n_lines_ok    = 0;
  int n_lines_bad   = 0;
  int n_lines_blank = 0;
  
  ofp = fopen("testtok_op.txt", "w");

  fp = fopen(argv[1], "r");

  if( fp == NULL )
    {
      printf("\nCould not open %s", argv[1]);
      exit(-1);
    }

  // read the file and tokenise each line
  while(!feof(fp) )
    {
      if( !next_composite_line(fp) )
	{
	  break;
	}

      // Check it's not a line full of spaces
      all_spaces = 1;
      for(int i=0; i<strlen(cline); i++)
	{
	  if( !isspace(cline[i]) )
	    {
	      all_spaces = 0;
	    }
	}
      
      if( all_spaces )
	{
	  n_lines_blank++;
	  continue;
	}
      
      printf("\n=======================cline==========================");
      printf("\n==%s==", cline);
      // Recursive decent parse
      if( scan_cline(fp) )
	{
	  n_lines_ok++;
	  printf("\ncline scanned OK");
	  
	}
      else
	{
	  n_lines_bad++;
	  printf("\ncline failed scan");
	}
    }

  printf("\n");
  printf("\n %d lines scanned Ok",       n_lines_ok);
  printf("\n %d lines scanned failed",   n_lines_bad);
  printf("\n %d lines blank",            n_lines_blank);
  fclose(ofp);

}
