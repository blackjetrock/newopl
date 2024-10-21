#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <inttypes.h>
#include "nopl.h"
#include "newopl.h"
#include "nopl_obj.h"
#include "parser.h"

//FILE *fp;

// Tokenise OPL
// one argument: an OPL file

// reads next composite line into buffer
char cline[MAX_NOPL_LINE];
int cline_i = 0;


#define SAVE_I     1
#define NO_SAVE_I  0

#define VAR_REF      1
#define VAR_DECLARE  0

char procedure_name[NOBJ_VARNAME_MAXLEN+1];

////////////////////////////////////////////////////////////////////////////////


char *exp_buffer_id_str[] =
  {
    "EXP_BUFF_ID_???",
    "EXP_BUFF_ID_TKN",
    "EXP_BUFF_ID_NONE",
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
    "EXP_BUFF_ID_PROC_CALL",
    "EXP_BUFF_ID_ASSIGN",
    "EXP_BUFF_ID_CONDITIONAL",
    "EXP_BUFF_ID_RETURN",
    "EXP_BUFF_ID_MAX",
  };

#define NUM_BUFF_ID (sizeof(exp_buffer_id_str) / sizeof(char *))


#if 0
// Per-expression
// Indices start at 1, 0 is 'no p'
int node_id_index = 1;

EXP_BUFFER_ENTRY exp_buffer[MAX_EXP_BUFFER];
int exp_buffer_i = 0;

EXP_BUFFER_ENTRY exp_buffer2[MAX_EXP_BUFFER];
int exp_buffer2_i = 0;
#endif

////////////////////////////////////////////////////////////////////////////////

struct _FN_INFO
{
  char *name;
  int command;         // 1 if command, 0 if function
  int trapable;        // 1 if can be used with TRAP
  char argparse;       // How to parse the args   O: scan_onoff
                       //                         otherwise: scan_expression()
  char *argtypes;
  char *resulttype;
  uint8_t qcode;
}
  fn_info[] =
  {
    { "EOL",      0,  0, ' ',  "ii",        "f", 0x00 },
    { "ABS",      0,  0, ' ',  "f",         "f", 0x00 },
    { "ACOS",     0,  0, ' ',  "f",         "f", 0x00 },
    { "ADDR",     0,  0, ' ',  "ii",        "f", 0x00 },
    { "APPEND",   1,  1, ' ',  "ii",        "v", 0x00 },
    { "ASC",      0,  0, ' ',  "i",         "s", 0x00 },
    { "ASIN",     0,  0, ' ',  "f",         "f", 0x00 },
    { "AT",       1,  0, ' ',  "ii",        "v", 0x4C },
    { "ATAN",     0,  0, ' ',  "f",         "f", 0x00 },
    { "BACK",     1,  1, ' ',  "ii",        "v", 0x00 },
    { "BEEP",     1,  0, ' ',  "ii",        "v", 0x00 },
    { "BREAK",    1,  0, ' ',  "ii",        "v", 0x00 },
    { "CHR$",     0,  0, ' ',  "i",         "s", 0x00 },
    { "CLOCK",    0,  0, ' ',  "ii",        "f", 0x00 },
    { "CLOSE",    1,  1, ' ',  "ii",        "v", 0x00 },
    { "CLS",      1,  0, ' ',  "",          "v", 0x00 },
    { "CONTINUE", 1,  0, ' ',  "ii",        "v", 0x00 },
    { "COPY",     1,  1, ' ',  "ii",        "v", 0x00 },
    { "COPYW",    1,  1, ' ',  "ii",        "v", 0x00 },
    { "COS",      0,  0, ' ',  "f",         "f", 0x00 },
    { "COUNT",    0,  0, ' ',  "ii",        "f", 0x00 },
    { "CREATE",   1,  1, ' ',  "ii",        "v", 0x00 },
    { "CURSOR",   1,  0, 'O',  "i",         "v", 0x00 },
    { "DATIM$",   0,  0, ' ',  "ii",        "f", 0x00 },
    { "DAY",      0,  0, ' ',  "",          "i", 0x00 },
    { "DAYNAME$", 0,  0, ' ',  "ii",        "f", 0x00 },
    { "DAYS",     0,  0, ' ',  "ii",        "f", 0x00 },
    { "DEG",      0,  0, ' ',  "ii",        "f", 0x00 },
    { "DELETE",   1,  1, ' ',  "ii",        "v", 0x00 },
    { "DELETEW",  1,  1, ' ',  "ii",        "v", 0x00 },
    { "DIR$",     0,  0, ' ',  "ii",        "f", 0x00 },
    { "DIRW$",    0,  0, ' ',  "ii",        "f", 0x00 },
    { "DISP",     0,  0, ' ',  "ii",        "f", 0x00 },
    { "DOW",      0,  0, ' ',  "ii",        "f", 0x00 },
    { "EDIT",     1,  1, ' ',  "ii",        "v", 0x00 },
    { "EOF",      0,  0, ' ',  "ii",        "f", 0x00 },
    { "ERASE",    1,  1, ' ',  "ii",        "v", 0x00 },
    { "ERR",      0,  0, ' ',  "ii",        "f", 0x00 },
    { "ERR$",     0,  0, ' ',  "ii",        "f", 0x00 },
    { "ESCAPE",   1,  0, 'O',  "ii",        "v", 0x00 },
    { "EXIST",    0,  0, ' ',  "ii",        "f", 0x00 },
    { "EXP",      0,  0, ' ',  "f",         "f", 0x00 },
    { "FIND",     0,  0, ' ',  "ii",        "f", 0x00 },
    { "FINDW",    0,  0, ' ',  "ii",        "f", 0x00 },
    { "FIRST",    1,  1, ' ',  "ii",        "v", 0x00 },
    { "FIX$",     0,  0, ' ',  "ii",        "f", 0x00 },
    { "FLT",      0,  0, ' ',  "i",         "f", 0x00 },
    { "FREE",     0,  0, ' ',  "ii",        "f", 0x00 },
    { "GEN$",     0,  0, ' ',  "ii",        "f", 0x00 },
    { "GET",      0,  0, ' ',  "",          "i", 0x00 },
    { "GET$",     0,  0, ' ',  "",          "s", 0x00 },
    { "HEX$",     0,  0, ' ',  "ii",        "f", 0x00 },
    { "HOUR",     0,  0, ' ',  "",          "i", 0x00 },
    { "IABS",     0,  0, ' ',  "i",         "i", 0x00 },
    { "INPUT",    1,  1, ' ',  "ii",        "v", 0x00 },
    { "INT",      0,  0, ' ',  "f",         "i", 0x00 },
    { "INTF",     0,  0, ' ',  "ii",        "f", 0x00 },
    { "KEY$",     0,  0, ' ',  "",          "s", 0x00 },
    { "KEY",      0,  0, ' ',  "",          "i", 0x00 },
    { "KSTAT",    1,  0, ' ',  "ii",        "v", 0x00 },
    { "LAST",     1,  1, ' ',  "ii",        "v", 0x00 },
    { "LEFT$",    0,  0, ' ',  "ii",        "f", 0x00 },
    { "LEN",      0,  0, ' ',  "s",         "i", 0x00 },
    { "LN",       0,  0, ' ',  "f",         "f", 0x00 },
    { "LOC",      0,  0, ' ',  "ii",        "f", 0x00 },
    { "LOG",      0,  0, ' ',  "ii",        "f", 0x00 },
    { "LOWER$",   0,  0, ' ',  "ii",        "f", 0x00 },
    { "LPRINT",   1,  0, ' ',  "ii",        "v", 0x00 },
    { "MAX",      0,  0, ' ',  "ii",        "f", 0x00 },
    { "MEAN",     0,  0, ' ',  "ii",        "f", 0x00 },
    { "MENU",     0,  0, ' ',  "ii",        "f", 0x00 },
    { "MENUN",    0,  0, ' ',  "ii",        "f", 0x00 },
    { "MID$",     0,  0, ' ',  "ii",        "f", 0x00 },
    { "MIN",      0,  0, ' ',  "ii",        "f", 0x00 },
    { "MINUTE",   0,  0, ' ',  "",          "i", 0x00 },
    { "MONTH",    0,  0, ' ',  "",          "i", 0x00 },
    { "MONTH$",   0,  0, ' ',  "ii",        "f", 0x00 },
    { "NEXT",     0,  1, ' ',  "ii",        "f", 0x00 },
    { "NUM$",     0,  0, ' ',  "ii",        "f", 0x00 },
    { "OFF",      1,  0, ' ',  "ii",        "v", 0x00 },
    { "OPEN",     1,  1, ' ',  "ii",        "v", 0x00 },
    { "ONERR",    1,  0, ' ',  "ii",        "v", 0x00 },
    { "PAUSE",    1,  0, ' ',  "i" ,        "v", 0x00 },
    { "PEEKB",    0,  0, ' ',  "ii",        "f", 0x00 },
    { "PEEKW",    0,  0, ' ',  "ii",        "f", 0x00 },
    { "PI",       0,  0, ' ',  "ii",        "f", 0x00 },
    { "POKEB",    1,  0, ' ',  "ii",        "v", 0x00 },
    { "POKEW",    1,  0, ' ',  "ii",        "v", 0x00 },
    { "POS",      0,  0, ' ',  "ii",        "f", 0x00 },
    { "POSITION", 1,  1, ' ',  "ii",        "v", 0x00 },
    { "PRINT",    1,  0, ' ',  "i",         "v", 0x00 },
    { "RAD",      0,  0, ' ',  "ii",        "f", 0x00 },
    { "RAISE",    1,  0, ' ',  "i",         "v", 0x00 },
    { "RANDOMIZE",1,  0, ' ',  "ii",        "v", 0x00 },
    { "RECSIZE",  0,  0, ' ',  "ii",        "f", 0x00 },
    { "REM",      1,  0, ' ',  "",          "v", 0x00 },
    { "RENAME",   1,  1, ' ',  "ii",        "v", 0x00 },
    { "REPT$",    0,  0, ' ',  "ii",        "f", 0x00 },
    { "RETURN",   1,  0, ' ',  "i",         "v", 0x00 },
    { "RIGHT$",   0,  0, ' ',  "ii",        "f", 0x00 },
    { "RND",      0,  0, ' ',  "" ,         "f", 0x00 },
    { "SCI$",     0,  0, ' ',  "ii",        "f", 0x00 },
    { "SECOND",   0,  0, ' ',  "",          "i", 0x00 },
    { "SIN",      0,  0, ' ',  "f",         "f", 0x00 },
    { "SPACE",    0,  0, ' ',  "ii",        "f", 0x00 },
    { "SQR",      0,  0, ' ',  "f",         "f", 0x00 },
    { "STD",      0,  0, ' ',  "ii",        "f", 0x00 },
    { "STOP",     1,  0, ' ',  "",          "v", 0x00 },
    { "SUM",      0,  0, ' ',  "ii",        "f", 0x00 },
    { "TAN",      0,  0, ' ',  "f",         "f", 0x00 },
    { "TRAP",     1,  0, ' ',  "ii",        "v", 0x00 },
    { "UDG",      1,  0, ' ',  "iiiiiiiii", "v", 0x00 },
    { "UPDATE",   1,  1, ' ',  "ii",        "v", 0x00 },
    { "UPPER$",   0,  0, ' ',  "ii",        "f", 0x00 },
    { "USE",      1,  1, ' ',  "ii",        "v", 0x00 },
    { "USR",      0,  0, ' ',  "ii",        "v", 0x00 },
    { "USR$",     0,  0, ' ',  "ii",        "f", 0x00 },
    { "VAL",      0,  0, ' ',  "ii",        "f", 0x00 },
    { "VAR",      0,  0, ' ',  "ii",        "f", 0x00 },
    { "VIEW",     0,  0, ' ',  "ii",        "f", 0x00 },
    { "WEEK",     0,  0, ' ',  "ii",        "f", 0x00 },
    { "YEAR",     0,  0, ' ',  "ii",        "f", 0x00 },
  };


#define NUM_FUNCTIONS (sizeof(fn_info)/sizeof(struct _FN_INFO))

int token_is_function(char *token, char **tokstr)
{
  dbprintf( "\n%s:", __FUNCTION__);
  
  for(int i=0; i<NUM_FUNCTIONS; i++)
    {
      if( (strcmp(token, fn_info[i].name) == 0) )
	{
	  *tokstr = &(fn_info[i].name[0]);
	  
	  dbprintf("%s is function", token);
	  return(1);
	}
    }
  dbprintf( "\n%s:%s is not function", __FUNCTION__, token);
  return(0);
  
}


// Return the function return value type
NOBJ_VARTYPE function_return_type(char *fname)
{
  char *rtype;
  char rt;
  NOBJ_VARTYPE vt;
  
  for(int i=0; i<NUM_FUNCTIONS; i++)
    {
      if( strcmp(fname, fn_info[i].name) == 0 )
	{
	  rtype = fn_info[i].resulttype;
	  rt = *rtype;
	  
	  dbprintf( "\n%s: '%s' =>%c", __FUNCTION__, fname, rt);

	  vt = char_to_type(rt);
	  dbprintf( "\n%s: '%s' =>%d", __FUNCTION__, fname, vt);
	  return(vt);
	  
	}
    }
  
  return(NOBJ_VARTYPE_UNKNOWN);
}

// Return the function return value type
int function_num_args(char *fname)
{

  for(int i=0; i<NUM_FUNCTIONS; i++)
    {
      if( strcmp(fname, fn_info[i].name) == 0 )
	{
	  return(strlen(fn_info[i].argtypes));
	}
    }
  return(0);
}

NOBJ_VARTYPE function_arg_type_n(char *fname, int n)
{
  char *atypes;
  
  for(int i=0; i<NUM_FUNCTIONS; i++)
    {
      if( strcmp(fname, fn_info[i].name) == 0 )
	{
	  atypes = fn_info[i].argtypes;
	  
	  //dbprintf( "\n%s: n:%d at:'%s' =>%c", __FUNCTION__, n, atypes, *(atypes+n));	  
	  return(char_to_type(*(atypes+n)));
	}
    }
  
  return(NOBJ_VARTYPE_UNKNOWN);
}

////////////////////////////////////////////////////////////////////////////////
//
// Operator info
//
// Some operators have a mutable type, they are polymorphic. Some are not.
// The possible types for operators are listed here
//


OP_INFO  op_info[] =
  {
    // Array dereference internal operator
    { "@",    9, 0,   MUTABLE_TYPE, 1, {NOBJ_VARTYPE_INT, NOBJ_VARTYPE_FLT, NOBJ_VARTYPE_STR} },
    { "=",    1, 0,   MUTABLE_TYPE, 1, {NOBJ_VARTYPE_INT, NOBJ_VARTYPE_FLT, NOBJ_VARTYPE_STR} },
    { "<>",   1, 0,   MUTABLE_TYPE, 1, {NOBJ_VARTYPE_INT, NOBJ_VARTYPE_FLT, NOBJ_VARTYPE_STR} },
    { ":=",   1, 0,   MUTABLE_TYPE, 1, {NOBJ_VARTYPE_INT, NOBJ_VARTYPE_FLT, NOBJ_VARTYPE_STR} },
    { "+",    3, 0,   MUTABLE_TYPE, 0, {NOBJ_VARTYPE_INT, NOBJ_VARTYPE_FLT, NOBJ_VARTYPE_STR} },
    { "-",    3, 0,   MUTABLE_TYPE, 0, {NOBJ_VARTYPE_INT, NOBJ_VARTYPE_FLT, NOBJ_VARTYPE_STR} },
    { "*",    5, 1,   MUTABLE_TYPE, 0, {NOBJ_VARTYPE_INT, NOBJ_VARTYPE_FLT, NOBJ_VARTYPE_INT} },
    { "/",    5, 1,   MUTABLE_TYPE, 0, {NOBJ_VARTYPE_INT, NOBJ_VARTYPE_FLT, NOBJ_VARTYPE_INT} },
    { ">",    5, 1,   MUTABLE_TYPE, 0, {NOBJ_VARTYPE_INT, NOBJ_VARTYPE_FLT, NOBJ_VARTYPE_STR} },
    { "<",    5, 1,   MUTABLE_TYPE, 0, {NOBJ_VARTYPE_INT, NOBJ_VARTYPE_FLT, NOBJ_VARTYPE_STR} },
    { "AND",  5, 1,   MUTABLE_TYPE, 0, {NOBJ_VARTYPE_INT, NOBJ_VARTYPE_FLT, NOBJ_VARTYPE_INT} },
    { ";",    0, 0,   MUTABLE_TYPE, 0, {NOBJ_VARTYPE_INT, NOBJ_VARTYPE_FLT, NOBJ_VARTYPE_INT} },
    // (Handle bitwise on integer, logical on floats somewhere)
    //{ ",",  0, 0 }, /// Not used?
    
    // LZ only
    { "+%",   5, 1, IMMUTABLE_TYPE, 0, {NOBJ_VARTYPE_FLT, NOBJ_VARTYPE_FLT, NOBJ_VARTYPE_FLT} },
    { "-%",   5, 1, IMMUTABLE_TYPE, 0, {NOBJ_VARTYPE_FLT, NOBJ_VARTYPE_FLT, NOBJ_VARTYPE_FLT} },
  };


#define NUM_OPERATORS_VAL (sizeof(op_info)/sizeof(struct _OP_INFO))

int num_operators(void)
{
  return(NUM_OPERATORS_VAL);
}

////////////////////////////////////////////////////////////////////////////////

void print_var_info(NOBJ_VAR_INFO *vi)
{
  dbprintf( "\nVAR INFO: '%18s' gbl:%d ref:%d int:%d flt:%d str:%d ary:%d max_str:%d max_ary:%d num_ind:%d",
 	 vi->name,
	 vi->is_global,
	 vi->is_ref,
	 vi->is_integer,
	 vi->is_float,
	 vi->is_string,
	 vi->is_array,
	 vi->max_string,
	 vi->max_array,
	 vi->num_indices
	 );
}

void init_var_info(NOBJ_VAR_INFO *vi)
{
  vi->name[0]     = '\0';
  vi->is_global   = 0;
  vi->is_ref      = 0;
  vi->is_integer  = 0;
  vi->is_float    = 0;
  vi->is_string   = 0;
  vi->is_array    = 0;
  vi->max_string  = 0;
  vi->max_array   = 0;
  vi->num_indices = 0;
}

//------------------------------------------------------------------------------
//

char cln[300];

char *cline_now(int idx)
{
  sprintf(cln, "%s", &(cline[idx]));

  return(cln);
}

#if 0

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
  
  dbprintf( "\nExpression buffer");
  dbprintf( "\n=================");
  
  for(int i=0; i<exp_buffer_i; i++)
    {
      EXP_BUFFER_ENTRY token = exp_buffer[i];
      
      dbprintf( "\n(%16s) N%d %-24s %c rq:%c %s nidx:%d", __FUNCTION__, token.node_id, exp_buffer_id_str[exp_buffer[i].buf_id], type_to_char(token.op.type), type_to_char(token.op.req_type), exp_buffer[i].name, token,op,vi,num_indices);
      
      dbprintf( "  %d:", token.p_idx);
      for(int pi=0; pi<token.p_idx; pi++)
	{
	  dbprintf( " %d", token.p[pi]);
	}
    }
  
  dbprintf( "\n=================");
}

void dump_exp_buffer2(void)
{
  char *idstr;
  
  dbprintf( "\nExpression buffer 2");
  dbprintf( "\n===================");
  
  for(int i=0; i<exp_buffer2_i; i++)
    {
      EXP_BUFFER_ENTRY token = exp_buffer2[i];

      if( (exp_buffer2[i].buf_id < 0) || (exp_buffer2[i].buf_id > EXP_BUFF_ID_MAX) )
	{
	  dbprintf("N%d buf_id invalid", token.node_id);
	}
      
      dbprintf( "\n(%16s) N%d %-24s %c rq:%c %s", __FUNCTION__, token.node_id, exp_buffer_id_str[exp_buffer2[i].buf_id], type_to_char(token.op.type), type_to_char(token.op.req_type), exp_buffer2[i].name);
      
      dbprintf( "  %d:", token.p_idx);
      for(int pi=0; pi<token.p_idx; pi++)
	{
	  dbprintf( " %d", token.p[pi]);
	}
    }
  dbprintf( "\n=================");
}

#endif

////////////////////////////////////////////////////////////////////////////////

#if 0
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
#endif

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
      ret_t = NOBJ_VARTYPE_STR;
      break;

    case 'v':
      ret_t = NOBJ_VARTYPE_VOID;
      break;
    }

  return(ret_t);
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
  for(int i=0; i<cline_i-1; i++)
    {
      printf(" ");
    }
  printf("^");
  
  printf("\n\n   %s", line);
  printf("\n");
  
  exit(-1);
}

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
  if( cline[strlen(cline)-1] == '\n' )
    {
      cline[strlen(cline)-1] = ' ';
    }
  
  return(1);
}

////////////////////////////////////////////////////////////////////////////////

void drop_space(int *index)
{
  int idx = *index;
  
  while( isspace(cline[idx]) && (cline[idx] != '\0') )
    {
      idx++;
    }
  *index = idx;
  
}

////////////////////////////////////////////////////////////////////////////////
//
// The '=' in an assignment needs to be a different token to the '='
// of equality

int scan_assignment_equals(void)
{
  char *lit = " =";
  char *origlit = lit;
  OP_STACK_ENTRY op;
  
  indent_more();
  
  init_op_stack_entry(&op);
  
  dbprintf("%s:lit='%s' '%s'", __FUNCTION__, lit, &(cline[cline_i]));
  
  if( *lit == ' ' )
    {
      lit++;
      drop_space(&cline_i);
    }

  dbprintf("%s:After drop space:'%s'", __FUNCTION__, &(cline[cline_i]));

  while( (*lit != '\0') && (*lit != ' ') )
    {
      dbprintf("%s:while loop:%s", __FUNCTION__, &(cline[cline_i]));
      if( cline[cline_i] == '\0' )
	{
	  syntax_error("Bad literal '%s'", origlit);
	  return(0);
	}
      
      if( *lit != cline[cline_i] )
	{
	  // Not a match, fail
	  dbprintf("%s:ret1", __FUNCTION__);  
	  return(0);
	}
      
      lit++;
      cline_i++;
    }
  
  if( *lit == ' ' )
    {
      lit++;
      drop_space(&cline_i);
    }
  
  // reached end of literal string , all ok

  if( *origlit == ' ' )
    {
      strcpy(op.name, origlit+1);
    }
  else
    {
      strcpy(op.name, origlit);
    }

  strcpy(op.name, ":=");
  process_token(&op);
  
  dbprintf("%s:ret1", __FUNCTION__);  
  return(1);
}

////////////////////////////////////////////////////////////////////////////////
//
// Scan for a literal
// Leading space means drop spaces before looking for the literal,
// trailing means drop spaces after finding it
//
////////////////////////////////////////////////////////////////////////////////

int scan_literal(char *lit)
{
  char *origlit = lit;
  OP_STACK_ENTRY op;
  indent_more();
  
  init_op_stack_entry(&op);
  
  dbprintf("%s:lit='%s' '%s'", __FUNCTION__, lit, &(cline[cline_i]));
  
  if( *lit == ' ' )
    {
      lit++;
      drop_space(&cline_i);
    }

  dbprintf("%s:After drop space:'%s'", __FUNCTION__, &(cline[cline_i]));

  while( (*lit != '\0') && (*lit != ' ') )
    {
      dbprintf("%s:while loop:%s", __FUNCTION__, &(cline[cline_i]));
      if( cline[cline_i] == '\0' )
	{
	  syntax_error("Bad literal '%s'", origlit);
	  return(0);
	}
      
      if( *lit != cline[cline_i] )
	{
	  // Not a match, fail
	  dbprintf("%s:ret1", __FUNCTION__);  
	  return(0);
	}
      
      lit++;
      cline_i++;
    }
  
  if( *lit == ' ' )
    {
      lit++;
      drop_space(&cline_i);
    }
  
  // reached end of literal string , all ok

  if( *origlit == ' ' )
    {
      strcpy(op.name, origlit+1);
    }
  else
    {
      strcpy(op.name, origlit);
    }
  
  process_token(&op);
  
  dbprintf("%s:ret1", __FUNCTION__);  
  return(1);
}

////////////////////////////////////////////////////////////////////////////////

// Check for a literal string.
// Leading space means drop spaces before looking for the literal,
// trailing means drop spaces after finding it

int check_literal(int *index, char *lit)
{
  int idx = *index;
  
  indent_more();
  
  dbprintf("%s:lit='%s' idx=%d '%s'", __FUNCTION__, lit, idx, &(cline[idx]));

  if( *lit == ' ' )
    {
      dbprintf("dropping space");
      lit++;
      drop_space(&idx);
    }

  dbprintf("%s:After drop space:'%s' idx=%d '%s'", __FUNCTION__, lit, idx, &(cline[idx]));

  if( cline[idx] == '\0' )
    {
      dbprintf("%s  ret0 Empty test string", __FUNCTION__);
      *index = idx;
      return(0);
    }
  
  while( (*lit != '\0') && (cline[idx] != '\0'))
    {
      if( *lit != cline[idx] )
	{
	  dbprintf("  '%c' != '%c'", *lit, cline[idx]);
	  // Not a match, fail

	  dbprintf("%s: ret0", __FUNCTION__);
	  *index = idx;
	  return(0);
	}
      lit++;
      idx++;
    }

  dbprintf("%s:After while():%s", __FUNCTION__, &(cline[idx]));
  
  if( cline[idx-1] == '\0' )
    {
      *index = idx;

      dbprintf("%s: ret0", __FUNCTION__);
      return(0);
    }
  
  // reached end of literal string , all ok
  *index = idx;
  dbprintf("%s:ret1 ", __FUNCTION__);
  return(1);

}


// Scans for a variable name string part
int scan_vname(char *vname_dest)
{
  char vname[300];
  int vname_i = 0;
  char ch;

  indent_more();
  dbprintf("%s: '%s'", __FUNCTION__, &(cline[cline_i]));

  drop_space(&cline_i);
  
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
      dbprintf("%s: ret1 '%s'", __FUNCTION__, vname);
      return(1);
    }

  dbprintf("%s: ret0", __FUNCTION__);
  strcpy(vname_dest, "");
  return(0);
}

//
// Checks for a variable name string part
//

int check_vname(int *index)
{
  int idx = *index;
  indent_more();
  
  drop_space(&idx);

  dbprintf("%s '%s':", __FUNCTION__, &(cline[idx]));
  
  if( isalpha(cline[idx]) )
    {
      idx++;
      
      while( isalnum(cline[idx]) )
	{
	  idx++;
	}

      dbprintf("%s ret1 '%s':", __FUNCTION__, &(cline[idx]));
      *index = idx;
      return(1);
    }

  dbprintf("%s ret0 '%s':", __FUNCTION__, &(cline[idx]));
  *index = idx;
  return(0);
}

////////////////////////////////////////////////////////////////////////////////
//
// Scan variable
//
// The variable can be a reference, or a declaration.
// Declarations have integers and not expressions in array brackets.
//
// This puts a variable ref in the output stream. handles arrays
// and puts appropriate codes for array indices in stream
//
// The variable name is captured and flags which specify the type.
// The expressions that are array indices aren't captured.
// They will be expressions on the stack and the type flags will ensure an array
// type qcode is set for the variable reference.
//
////////////////////////////////////////////////////////////////////////////////

void set_op_var_type(OP_STACK_ENTRY *op, NOBJ_VAR_INFO *vi)
{
  if( vi->is_array )
    {
      if( vi->is_integer )
	{
	  op->type = NOBJ_VARTYPE_INTARY;
	}
      
      if( vi->is_float )
	{
	  op->type = NOBJ_VARTYPE_FLTARY;
	}
      
      if( vi->is_string )
	{
	  op->type = NOBJ_VARTYPE_STRARY;
	}
    }
  else
    {  
      if( vi->is_integer )
	{
	  op->type = NOBJ_VARTYPE_INT;
	}
      
      if( vi->is_float )
	{
	  op->type = NOBJ_VARTYPE_FLT;
	}
      
      if( vi->is_string )
	{
	  op->type = NOBJ_VARTYPE_STR;
	}
    }
}

int scan_variable(char *variable_dest, NOBJ_VAR_INFO *vi, int ref_ndeclare)
{
  char vname[300];
  char chstr[2];
  int idx = cline_i;
  OP_STACK_ENTRY op;

  init_op_stack_entry(&op);
  indent_more();
  
  dbprintf("%s:", __FUNCTION__);

  init_var_info(vi);
  vi->is_ref = ref_ndeclare;

  // Default to local, this can be overridden for global later.
  vi->is_global = 0;
  
  chstr[1] = '\0';
  
  if( scan_vname(vname) )
    {
      dbprintf("%s: '%s' vname='%s'", __FUNCTION__, &(cline[cline_i]), vname);
      
      // Could just be a vname
      switch( chstr[0] = cline[cline_i] )
	{
	case '%':
	  vi->is_integer = 1;
	  strcat(vname, chstr);
	  cline_i++;
	  break;
	  
	case '$':
	  vi->is_string = 1;
	  strcat(vname, chstr);
	  cline_i++;
	  break;

	default:
	  vi->is_float = 1;
	  break;
	}

      // Is it an array?
      dbprintf("%s: Ary test '%s'", __FUNCTION__, &(cline[cline_i]));
      idx = cline_i;
      
      if( check_literal(&idx,"(") )
	{
	  if( !scan_literal("(") )
	    {
	      syntax_error("Malformed array index");
	      return(0);
	    }
	  
	  dbprintf("%s: is array", __FUNCTION__);
	  
	  vi->is_array = 1;
	  
	  // Add token to output stream for index or indices

	  if( ref_ndeclare )
	    {
	      int num_subexpr;
	      
	      scan_expression(&num_subexpr);
	      vi->num_indices = num_subexpr+1;
	    }
	  else
	    {
	      //scan_integer(&(vi->max_array));
	      if( vi->is_string )
		{
		  // Strings are arrays only if there are two indices
		  vi->is_array = 0;
		  scan_integer(&(vi->max_string));
		}
	      else
		{
		  scan_integer(&(vi->max_array));
		}
	    }

	  dbprintf("num idx:%d", vi->num_indices);
      
#if 1
	  // Could be string array, which has two expressions in
	  // the brackets
	  idx = cline_i;
	  if( check_literal(&idx," ,") )
	    {
	      if( vi->is_string )
		{
		  // All OK, string array
		  vi->is_array = 1;
		  scan_literal(" ,");

		  if( ref_ndeclare )
		    {
		      int num_subexpr;
		      scan_expression(&num_subexpr);
		      vi->num_indices = num_subexpr+1;
		    }
		  else
		    {
		      // Set the values correctly
		      vi->max_array = vi->max_string;
		      scan_integer(&(vi->max_string));
		    }
		}
	      else
		{
		  syntax_error("Two indices in non-string variable name");
		}
	    }
#endif
	  if( scan_literal(" )") )
	    {
	      dbprintf("%s:ret1 vname='%s' is str:%d int:%d flt:%d ary:%d", __FUNCTION__,
		     vname,
		     vi->is_string,
		     vi->is_integer,
		     vi->is_float,
		     vi->is_array
		     );
	      
	      strcpy(vi->name, vname);
	      strcpy(op.name, vname);
	      set_op_var_type(&op, vi);
	      op.vi = *vi;
	      process_token(&op);
	      return(1);
	    }
	}
      
      dbprintf("%s:ret1 vname='%s' is str:%d int:%d flt:%d ary:%d", __FUNCTION__,
	     vname,
	     vi->is_string,
	     vi->is_integer,
	     vi->is_float,
	     vi->is_array
	     );
      
      strcpy(vi->name, vname);
      strcpy(op.name, vname);
      set_op_var_type(&op, vi);
      op.vi = *vi;
      process_token(&op);
      
      return(1);
    }
  
  dbprintf("%s:ret0", __FUNCTION__);
  return(0);
}

int check_variable(int *index)
{
  int idx = *index;
  
  char vname[300];
  char chstr[2];
  int var_is_string  = 0;
  int var_is_integer = 0;
  int var_is_float   = 0;
  int var_is_array   = 0;

  indent_more();
  
  drop_space(&idx);
  
  dbprintf("%s:", __FUNCTION__);

  vname[0] = '\0';
  chstr[1] = '\0';
  
  if( check_vname(&idx) )
    {
      dbprintf("%s: '%s'", __FUNCTION__, &(cline[idx]));
      
      // Could just be a vname
      switch( chstr[0] = cline[idx] )
	{
	case '%':
	  var_is_integer = 1;
	  strcat(vname, chstr);
	  idx++;
	  break;
	  
	case '$':
	  var_is_string = 1;
	  strcat(vname, chstr);
	  idx++;
	  break;

	default:
	  var_is_float = 1;
	  break;
	}

      // Is it an array?
      dbprintf("%s: Ary test '%s'", __FUNCTION__, &(cline[idx]));
      
      if( check_literal(&idx, "(") )
	{
	  dbprintf("%s: is array", __FUNCTION__);
	  
	  var_is_array = 1;
	  
	  // Add token to output stream for index or indices
	  check_expression(&idx);

	  // Could be string array, which has two expressions in
	  // the brackets
	  if( check_literal(&idx," ,") )
	    {
	      if( var_is_string )
		{
		  // All OK, string array
		  if( check_expression(&idx) )
		    {
		      // All OK
		    }
		  else
		    {
		      *index = idx;
		      return(0);
		    }
		}
	      else
		{
		  *index = idx;
		  dbprintf("%s:ret0 ", __FUNCTION__);
		  return(0);
		}
	    }
	  
	  if( check_literal(&idx, " )") )
	    {
	      *index = idx;
	      dbprintf("%s:ret1 ", __FUNCTION__);
	      return(1);
	    }
	}
      
      dbprintf("%s:ret1 vname='%s' is str:%d int:%d flt:%d ary:%d", __FUNCTION__,
	     vname,
	     var_is_string,
	     var_is_integer,
	     var_is_float,
	     var_is_array
	     );
      
      *index = idx;
      dbprintf("%s:ret1 ", __FUNCTION__);
      return(1);
      
    }

  *index = idx;
  dbprintf("%s:ret0 ", __FUNCTION__);
  return(0);
}

int check_variable2(int *index)
{
  int idx = *index;
  indent_more();
  
  if( check_vname(&idx) )
    {
      *index = idx;
      return(1);
    }

  *index = idx;
  return(0);
}

////////////////////////////////////////////////////////////////////////////////

int check_operator(int *index, int *is_comma)
{
  int idx = *index;
  indent_more();
  
  dbprintf("%s: %s", __FUNCTION__, cline_now(idx));

  drop_space(&idx);
  
  if( check_literal(&idx, " ,") )
    {
      *index = idx;
      dbprintf("ret1:is comma: %d", *is_comma);
      return(1);

#if 0
      if(scan_literal(" ,") )
	{
	  *is_comma = 1;
	  dbprintf("ret1:is comma: %d", *is_comma);
	  return(1);
	}
      else
	{
	  *is_comma = 0;
	  dbprintf("ret0:is comma: %d", *is_comma);
	  return(0);
	}
#endif
    }

  for(int i=0; i<NUM_OPERATORS; i++)
    {
      if( strncmp(&(cline[idx]), op_info[i].name, strlen(op_info[i].name)) == 0 )
	{
	  // Match
	  *is_comma = 0;

	  dbprintf("ret1:is comma: %d", *is_comma);
	  idx += strlen(op_info[i].name);
	  *index = idx;
	  return(1);
	}
    }

  dbprintf("ret0:is comma: %d", *is_comma);

  *is_comma = 0;
  *index = idx;
  return(0);
}

int scan_operator(int *is_comma)
{
  int idx = cline_i;
  OP_STACK_ENTRY op;

  *is_comma = 0;
  
  indent_more();
  
  init_op_stack_entry(&op);
  
  dbprintf("%s: '%s'", __FUNCTION__, cline_now(idx));

  drop_space(&idx);

  cline_i = idx;
  
  if( check_literal(&idx, " ,") )
    {
      if(scan_literal(" ,") )
	{
	  *is_comma = 1;
	  dbprintf("ret1:is comma: %d", *is_comma);
	  return(1);
	}
      else
	{
	  *is_comma = 0;
	  dbprintf("ret0:is comma: %d", *is_comma);
	  return(0);
	}
      //      *is_comma = 1;
      //dbprintf("is comma:%d", is_comma);
      //return(scan_literal(" ,"));
    }
  
  for(int i=0; i<NUM_OPERATORS; i++)
    {
      if( strncmp(&(cline[cline_i]), op_info[i].name, strlen(op_info[i].name)) == 0 )
	{
	  *is_comma = 0;
	  
	  // Match
	  cline_i += strlen(op_info[i].name);
	  dbprintf("%s: ret1 '%s'", __FUNCTION__, cline_now(cline_i));
	  strcpy(op.name, op_info[i].name);
	  process_token(&op);
	  return(1);
	}
    }

  *is_comma = 0;
  dbprintf("%s: ret0 '%s'", __FUNCTION__, cline_now(cline_i));
  return(0);
}

////////////////////////////////////////////////////////////////////////////////

int check_integer(int *index)
{
  int idx = *index;
  int num_digits = 0;
  indent_more();
  
  drop_space(&idx);
  
  dbprintf("%s: '%s'", __FUNCTION__, &(cline[idx]));

  char intval[20];
  char chstr[2];

  intval[0] = '\0';
  
  while( isdigit(chstr[0] = cline[idx]) )
    {
      strcat(intval, chstr);
      idx++;
      num_digits++;
    }

  *index = idx;
  
  if( num_digits > 0 )
    {
      dbprintf("%s:ret1", __FUNCTION__);
      return(1);
    }

  dbprintf("%s:ret0", __FUNCTION__);
  return(0);
}

//------------------------------------------------------------------------------
//
// Scan for integer and return it as an integer
//

int scan_integer(int *intdest)
{
  int num_digits = 0;
  OP_STACK_ENTRY op;
  indent_more();
  
  init_op_stack_entry(&op);
  
  drop_space(&cline_i);
  
  dbprintf("%s:", __FUNCTION__);

  char intval[20];
  char chstr[2];

  intval[0] = '\0';
  
  while( isdigit(chstr[0] = cline[cline_i]) )
    {
      strcat(intval, chstr);
      cline_i++;
      num_digits++;
    }

  // Convert to integer
  sscanf(intval, "%d", intdest);
  //x  strcpy(intdest, intval);
  
  if( num_digits > 0 )
    {
      dbprintf("%s:ret1", __FUNCTION__);

      strcpy(op.name, intval);

      op.integer = *intdest;
      op.type = NOBJ_VARTYPE_INT;
      process_token(&op);

      dbprintf("%s:ret1  %s", __FUNCTION__, intval);

      return(1);
    }

  dbprintf("%s:ret0", __FUNCTION__);
  return(0);
}

int isfloatdigit(char c)
{
  dbprintf("%s:", __FUNCTION__);
  if( isdigit(c) || (c == '.') )
    {
      return(1);
    }
  
  return(0);
}

int check_float(int *index)
{
  int idx = *index;
  char chstr[2];
  int decimal_present = 0;
  int num_digits = 0;
  indent_more();
  
  dbprintf("%s:", __FUNCTION__);
  
  drop_space(&idx);
  
  while( isfloatdigit(chstr[0] = cline[idx]) )
    {
      if( chstr[0] == '.' )
	{
	  decimal_present = 1;
	}
      
      idx++;
      num_digits++;
    }


  if( (num_digits > 0) &&  decimal_present )
    {
      dbprintf("%s: ret1", __FUNCTION__);
      *index = idx;
      return(1);
    }
  
  dbprintf("%s: ret0", __FUNCTION__);

  return(0);
}

int scan_float(char *fltdest)
{

  char fltval[20];
  char chstr[2];
  int decimal_present = 0;
  int num_digits = 0;
  OP_STACK_ENTRY op;

  indent_more();
  
  dbprintf("%s:", __FUNCTION__);
  init_op_stack_entry(&op);
  
  drop_space(&cline_i);
  
  fltval[0] = '\0';
  
  while( isfloatdigit(chstr[0] = cline[cline_i]) )
    {
      if( chstr[0] == '.' )
	{
	  decimal_present = 1;
	}
      
      strcat(fltval, chstr);
      cline_i++;
      num_digits++;
    }

  strcpy(fltdest, fltval);
  if( (num_digits > 0) &&  decimal_present )
    {
      dbprintf("%s: ret1", __FUNCTION__);
      strcpy(op.name, fltval);
      process_token(&op);
      return(1);
    }
  
  dbprintf("%s: ret0", __FUNCTION__);
  return(0);
  
}

int check_number(int *index)
{
  int idx = *index;
  indent_more();
  
  dbprintf("%s:", __FUNCTION__);

  drop_space(&idx);
	     
  if( check_float(&idx) )
    {
      dbprintf("%s: ret1", __FUNCTION__);
      *index = idx;
      return(1);
    }

  if( check_integer(&idx) )
    {
      dbprintf("%s: ret1", __FUNCTION__);
      *index = idx;
      return(1);
    }

  dbprintf("%s: ret0", __FUNCTION__);
  return(0);
}

//------------------------------------------------------------------------------

int scan_number(void)
{
  int idx = cline_i;
  char fltval[40];

  indent_more();
  
  dbprintf("%s:", __FUNCTION__);
  
  drop_space(&cline_i);
  
  if( check_float(&idx) )
    {
      scan_float(fltval);
      return(1);
    }

  idx = cline_i;
  if( check_integer(&idx) )
    {
      int intval;
      scan_integer(&intval);
      return(1);
    }

  syntax_error("Not a number");
  return(0);
}

int check_sub_expr(int *index)
{
  int idx = *index;

  indent_more();
  
  dbprintf("%s:", __FUNCTION__);
  
  if( check_literal(&idx," (") )
    {
      dbprintf("%s: ret1", __FUNCTION__);
      *index = idx;
      return(1);
    }

  dbprintf("%s: ret0", __FUNCTION__);
  *index = idx;
  return(0);
}

int scan_sub_expr(void)
{
  int num_subexpr;
  
  OP_STACK_ENTRY op;
  indent_more();
  
  init_op_stack_entry(&op);
  
  dbprintf("%s:", __FUNCTION__);
  if( scan_literal(" (") )
    {
      strcpy(op.name, "(");
      process_token(&op);
      
      if( scan_expression(&num_subexpr) )
	{
	  if( scan_literal(" )") )
	    {
	      strcpy(op.name, ")");
	      process_token(&op);
	      
	      dbprintf("%s:ret1", __FUNCTION__);
	      return(1);
	    }
	}
    }

  dbprintf("%s:ret0", __FUNCTION__);
  syntax_error("Expression error");
  return(0);
}

int check_atom(int *index)
{
  int idx = *index;
  indent_more();
  
  dbprintf("%s:", __FUNCTION__);
  
  idx = *index;
  if( check_literal(&idx," %") )
    {
      // Character code
      if( (cline[idx] != '\0') && (cline[idx] != ' ') )
	{
	  idx++; 
	  *index = idx;
	  dbprintf("%s:ret1", __FUNCTION__);
	  return(1);
	}
    }

  idx = *index;
  if( check_literal(&idx," \"") )
    {
      // String
      *index = idx;
      dbprintf("%s:ret1", __FUNCTION__);
      return(1);
    }

  idx = *index;
  if( check_number(&idx) )
    {
      // Int or float
      *index = idx;
      dbprintf("%s:ret1", __FUNCTION__);
      return(1);
    }

  idx = *index;
  if( check_variable(&idx) )
    {
      // Variable
      *index = idx;
      dbprintf("%s:ret1", __FUNCTION__);
      return(1);
    }

  *index = idx;
  dbprintf("%s:ret1", __FUNCTION__);
  return(0);
}

//------------------------------------------------------------------------------
// Scan a string and output a token which is the string

int scan_string(void)
{
  char chstr[2];
  char strval[300];
  OP_STACK_ENTRY op;
  indent_more();
  
  init_op_stack_entry(&op);
  
  dbprintf("%s:", __FUNCTION__);

  strval[0] = '\0';
  chstr[1] = '\0';
  
  if( check_literal(&cline_i, " \"") )
    {
      dbprintf("  (in if) '%s'", &(cline[cline_i]));
      
      while(((chstr[0] = cline[cline_i]) != '"') && (cline[cline_i] != '\0') )
	{
	  dbprintf("  (in wh) '%s'", &(cline[cline_i]));
	  strcat(strval, chstr);
	  cline_i++;
	  dbprintf("  (in wh) '%s'", &(cline[cline_i]));
	}

      
      if( cline[cline_i] == '"' )
	{
	  cline_i++;
	  dbprintf("%s: ret1", __FUNCTION__);
	  strcpy(op.name, "\"");
	  strcat(op.name, strval);
	  strcat(op.name, "\"");
	  process_token(&op);
	  return(1);
	}
    }

  syntax_error("Bad string");
  dbprintf("%s:ret1", __FUNCTION__);
  return(0);
}

int scan_character(void)
{
  if( (cline[cline_i] != '\0') && (cline[cline_i] != ' ') )
    {
      return(1);
    }
  return(0);
}

int scan_atom(void)
{
  int idx = cline_i;
  NOBJ_VAR_INFO vi;
  char vname[300];
  indent_more();
  
  init_var_info(&vi);
  dbprintf("%s:", __FUNCTION__);

  idx = cline_i;
  if( check_literal(&idx," %") )
    {
      if( scan_literal(" %" ) )
	{
	  // Hxadecimal number
	  if(scan_character())
	    {
	      dbprintf("%s:ret1", __FUNCTION__);
	      return(1);
	    }
	  else
	    {
	      dbprintf("%s:ret0", __FUNCTION__);
	      return(0);
	    }
	}
    }

  idx = cline_i;
  if( check_literal(&idx," \"") )
    {
      // String
      if(scan_string())
	{
	  dbprintf("%s:ret1", __FUNCTION__);
	  return(1);
	}
      else
	{
	  dbprintf("%s:ret0", __FUNCTION__);
	  return(0);
	}
    }

  idx = cline_i;
  if( check_number(&idx) )
    {
      // Int or float
      if(scan_number())
	{
	  dbprintf("%s:ret1", __FUNCTION__);
	  return(1);
	}
      else
	{
	  dbprintf("%s:ret0", __FUNCTION__);
	  return(0);
	}
    }

  idx = cline_i;
  if( check_variable(&idx) )
    {
      // Variable
      init_var_info(&vi);
      if(scan_variable(vname, &vi, VAR_REF))
	{
	  print_var_info(&vi);
	  dbprintf("%s:ret1", __FUNCTION__);
	  return(1);
	}
      
      dbprintf("%s:ret0", __FUNCTION__);
      return(0);
    }

  syntax_error("Not an atom");
  dbprintf("%s:ret0", __FUNCTION__);
  return(0);
}


int check_eitem(int *index, int *is_comma)
{
  int idx = *index;

  *is_comma = 0;
  indent_more();
  
  dbprintf("%s: '%s'", __FUNCTION__, &(cline[idx]));
  
  if( check_operator(&idx, is_comma) )
    {
      *index = idx;
      dbprintf("%s:ret1 comma:1", __FUNCTION__);
      return(1);
    }

  idx = *index;
  if( check_function(&idx) )
    {
      *index = idx;
      dbprintf("%s:ret1", __FUNCTION__);
      return(1);
    }

  idx = *index;
  if( check_atom(&idx) )
    {
      *index = idx;
      dbprintf("%s:ret1", __FUNCTION__);
      return(1);
    }

  idx = *index;
  if( check_sub_expr(&idx) )
    {
      *index = idx;
      dbprintf("%s:ret1", __FUNCTION__);
      return(1);
    }
  
  idx = *index;
  dbprintf("%s:ret0", __FUNCTION__);
  return(0);
}

//------------------------------------------------------------------------------

int scan_eitem(int *num_commas)
{
  int is_comma = 0;
  int idx = cline_i;
  char fnval[40];
  OP_STACK_ENTRY op;
  
  indent_more();
  
  init_op_stack_entry(&op);
  
  dbprintf("%s:", __FUNCTION__);
  
  if( check_operator(&idx, &is_comma) )
    {
      if(scan_operator(&is_comma) )
	{
	  dbprintf("Is comma;%d", is_comma);
	  *num_commas = is_comma;
	  return(1);
	}
      else
	{
	  *num_commas = 0;
	  return(0);
	}
      
    }

  idx = cline_i;
  if( check_function(&idx) )
    {
      *num_commas = 0;
      return(scan_function(fnval) );
    }

  idx = cline_i;
  if( check_atom(&idx) )
    {
      *num_commas = 0;
      return(scan_atom());
    }

  idx = cline_i;
  if( check_sub_expr(&idx) )
    {
      *num_commas = 0;
      return(scan_sub_expr());
    }

  idx = cline_i;
  syntax_error("Not an atom");
  *num_commas = 0;
  return(0);
}


int check_expression(int *index)
{
  int idx = *index;
  int num_eitems = 0;
  int is_comma = 0;
  indent_more();
  
  drop_space(&idx);
  
  dbprintf("%s: '%s'", __FUNCTION__, &(cline[idx]));
  while( check_eitem(&idx, &is_comma) && (cline[idx] != '\0') )
    {
      num_eitems++;
    }

  if( num_eitems > 0 )
    {
      dbprintf("%s:ret1 '%s'", __FUNCTION__, &(cline[idx]));
      *index = idx;
      return(1);
    }

  dbprintf("%s:ret0 '%s'", __FUNCTION__, &(cline[idx]));
  *index = idx;
  
  return(0);
}

////////////////////////////////////////////////////////////////////////////////

int scan_expression(int *num_commas)
{
  int idx = cline_i;
  int n_commas = 0;
  int n_commas2 = 0;
  int is_comma;
  
  indent_more();
  
  dbprintf("%s: '%s'", __FUNCTION__, &(cline[idx]));

  drop_space(&idx);
  cline_i = idx;

  while( check_eitem(&idx, &is_comma) )
    {
      if( scan_eitem(&n_commas2) )
	{
	  n_commas += n_commas2;
	  dbprintf("n commas now:%d", n_commas);

	  // All OK
	}
      else
	{
	  syntax_error("Expression error");
	  dbprintf("%s: ret0 '%s'", __FUNCTION__, &(cline[cline_i]));
	  return(0);
	}
      idx = cline_i;
    }

  *num_commas = n_commas;
  dbprintf("%s: ret1 '%s' commas:%d", __FUNCTION__, &(cline[cline_i]), n_commas);
  return(1);
}

////////////////////////////////////////////////////////////////////////////////
//

// We do not scan on/off as we want to map them to 1/0
// before sending to th eoutput stream.

int check_onoff(int *index, int *onoff_val)
{
  int idx = *index;
  indent_more();
  
  dbprintf("%s: '%s'", __FUNCTION__, &(cline[cline_i]));
  
  if( check_literal(&idx, " ON") )
    {
      *onoff_val = 1;
      dbprintf("%s:ret1", __FUNCTION__);
      return(1);
    }

  idx = cline_i;
  if( check_literal(&idx, " OFF") )
    {
      *onoff_val = 0;
      dbprintf("%s:ret1", __FUNCTION__);
      return(1);
    }

  dbprintf("%s:ret0", __FUNCTION__);
  return(0);
  
}

////////////////////////////////////////////////////////////////////////////////
//
// Command parsing
//
// We have an table of commands which also has various parse fields for each
// command. these direct parsing of arguments, for instane on/off vs
// expressions after the command name.
//

int check_command(int *index)
{
  int idx = *index;
  indent_more();
  
  drop_space(&idx);
  
  dbprintf("%s:", __FUNCTION__);
  
  for(int i=0; i<NUM_FUNCTIONS; i++)
    {
      if( fn_info[i].command && strncmp(&(cline[idx]), fn_info[i].name, strlen(fn_info[i].name)) == 0 )
	{
	  // Match
	  dbprintf("%s: ret1 found=> '%s'", __FUNCTION__, fn_info[i].name);
	  *index = idx + strlen(fn_info[i].name);
	  return(1);
	}
    }
  dbprintf("%s: ret0", __FUNCTION__);
  return(0);
}

//------------------------------------------------------------------------------

int scan_command(char *cmd_dest)
{
  int onoff_val;
  int num_subexpr = 0;  
  OP_STACK_ENTRY op;
  indent_more();
  
  init_op_stack_entry(&op);
  
  drop_space(&cline_i);
  
  dbprintf("%s:", __FUNCTION__);

  for(int i=0; i<NUM_FUNCTIONS; i++)
    {
      if( fn_info[i].command && (strncmp(&(cline[cline_i]), fn_info[i].name, strlen(fn_info[i].name)) == 0) )
	{
	  // Match
	  strcpy(cmd_dest, fn_info[i].name);
	  cline_i += strlen(fn_info[i].name);

	  // Send command to output stream
	  strcpy(op.name, cmd_dest);
	  process_token(&op);

	  // The arguments have to be in a sub expression.
	  dbprintf( "\nSTARTEXP");
	  sprintf(op.name, "(");
	  process_token(&op);
	  
	  switch(fn_info[i].argparse)
	    {
	      // ON/OFF
	    case 'O':
	      if( check_onoff(&cline_i, &onoff_val) )
		{
		  // ON and off map to 1 and 0
		  sprintf(op.name, "%d", onoff_val);
		  process_token(&op);
		  
		  dbprintf("%s: ret1 =>'%s'", __FUNCTION__, cmd_dest);
		  dbprintf( "\nENDEXP");
		  sprintf(op.name, ")");
		  process_token(&op);

		  return(1);
		}
	      else
		{
		  dbprintf("%s: expression failed", __FUNCTION__);
		  return(0);
		}
	      break;

	      // Expression scanning will push expression to output stream.
	    default:
	      if( scan_expression(&num_subexpr) )
		{
		  dbprintf("%s: ret1 =>'%s'", __FUNCTION__, cmd_dest);
		  dbprintf( "\nENDEXP");
		  sprintf(op.name, ")");
		  process_token(&op);
		  
		  return(1);
		}
	      else
		{
		  dbprintf("%s: expression failed", __FUNCTION__);
		  return(0);
		}
	      break;
	    }


	}
    }

  strcpy(cmd_dest, "");
  return(0);
}

int check_function(int *index)
{
  int idx = *index;
  indent_more();
  
  drop_space(&idx);
  
  dbprintf("%s: '%s'", __FUNCTION__, &(cline[idx]));
    
  for(int i=0; i<NUM_FUNCTIONS; i++)
    {
      if( (!fn_info[i].command) && strncmp(&(cline[idx]), fn_info[i].name, strlen(fn_info[i].name)) == 0 )
	{
	  // Match
	  dbprintf("%s: ret1 Found fn=>'%s'", __FUNCTION__, fn_info[i].name);
	  *index = idx+strlen(fn_info[i].name);
	  return(1);
	}
    }
  
  dbprintf("%s: ret0", __FUNCTION__);
  *index = idx;
  return(0);
}

int scan_function(char *cmd_dest)
{
  OP_STACK_ENTRY op;
  indent_more();
  
  init_op_stack_entry(&op);
  
  drop_space(&cline_i);
  
  dbprintf("%s:", __FUNCTION__);
  for(int i=0; i<NUM_FUNCTIONS; i++)
    {
      if( !(fn_info[i].command) && (strncmp(&(cline[cline_i]), fn_info[i].name, strlen(fn_info[i].name)) == 0) )
	{
	  // Match
	  strcpy(cmd_dest, fn_info[i].name);
	  cline_i += strlen(fn_info[i].name);
	  strcpy(op.name, fn_info[i].name);
	  process_token(&op);

	  // If the function has no arguments, add a dummy empty expression after it
	  // as the shunting algorithm has to have brackets after a function
	  if( strlen(fn_info[i].argtypes) == 0 )
	    {
	      fprintf(ofp, "\nDummy argument expression added");
	      strcpy(op.name, "(");
	      process_token(&op);
	      strcpy(op.name, ")");
	      process_token(&op);
	    }
	  return(1);
	}
    }

  strcpy(cmd_dest, "");
  return(0);
}

int check_assignment(int *index)
{
  int idx = *index;
  indent_more();
  
  dbprintf("%s:", __FUNCTION__);
  
  if( check_variable(&idx) )
    {
      if( check_literal(&idx, " =") )
	{
	  if( check_expression(&idx) )
	    {
	      *index = idx;
	      dbprintf("%s:ret1", __FUNCTION__);
	      return(1);
	    }
	}
    }

  dbprintf("%s:ret0", __FUNCTION__);
  *index = idx;
  return(0);
}

int scan_assignment(void)
{
  char vname[300];
  NOBJ_VAR_INFO vi;
  int num_subexpr  = 0;
  
  indent_more();
  
  init_var_info(&vi);
  dbprintf("%s:", __FUNCTION__);

  if( scan_variable(vname, &vi, VAR_REF) )
    {
      print_var_info(&vi);
      
      if( scan_assignment_equals() )
	{
	  if( scan_expression(&num_subexpr) )
	    {
	      vi.num_indices = num_subexpr+1;
	      dbprintf("%s: ret1", __FUNCTION__);
	      return(1);
	    }
	}
    }

  syntax_error("Assignment error");
  return(0);
}

////////////////////////////////////////////////////////////////////////////////

int check_textlabel(int *index, char *label_dest)
{
  int idx = *index;
  char chstr[2];
  indent_more();
  
  chstr[1] = '\0';
  
  *label_dest = '\0';
  
  drop_space(&idx);
  
  dbprintf("%s: '%s'", __FUNCTION__, &(cline[idx]));

  // Just a colon is not a text label  
  if( cline[idx] == ':' )
    {
      *index = idx;
      dbprintf("%s:ret0 (just colon)", __FUNCTION__);
      return(0);
    }

  while( (cline[idx] != ':') && (cline[idx] != '\0') && (cline[idx] != ' ') )
    {
      chstr[0] = cline[idx];
      strcat(label_dest, chstr);
      idx++;
    }

  if( cline[idx] == ':' )
    {
      *index = idx;
      dbprintf("%s:ret1", __FUNCTION__);
      return(1);
    }
  
  dbprintf("%s:ret0", __FUNCTION__);  
  return(0);
}

int check_label(int *index)
{
  int idx = *index;
  char textlabel[NOBJ_VARNAME_MAXLEN+1];
  indent_more();
  
  dbprintf("%s:", __FUNCTION__);
  
  if( check_textlabel(&idx, textlabel))
    {
      if( check_literal(&idx, "::") )
	{
	  dbprintf("%s:ret1", __FUNCTION__);
	  *index = idx;
	  return(1);
	}
    }
  
  dbprintf("%s:ret0", __FUNCTION__);
  return(0);
}

int scan_label(void)
{
  int idx = cline_i;
  char textlabel[NOBJ_VARNAME_MAXLEN+1];
  indent_more();
  
 dbprintf("%s:", __FUNCTION__);
  if( check_textlabel(&idx, textlabel))
    {
      cline_i = idx;
      
      if( scan_literal("::") )
	{
	  dbprintf("%s:ret1", __FUNCTION__);
	  return(1);
	}
    }
  
  dbprintf("%s:ret0", __FUNCTION__);
  return(0);
}

////////////////////////////////////////////////////////////////////////////////
//
// RETURN
//
// This keyword can have either nothing after it  or an expression
//
// An expression is a an extension from the original which could only have
// a variable after RETURN, or nothing.
//

int check_return(int *index)
{
  int idx = *index;
  char textlabel[NOBJ_VARNAME_MAXLEN+1];
  indent_more();
  
  dbprintf("%s: '%s'", __FUNCTION__, &(cline[idx]));
  
  if( check_literal(&idx, " RETURN"))
    {
      dbprintf("%s:ret1", __FUNCTION__);
      *index = idx;
      return(1);
    }
  
  dbprintf("%s:ret0", __FUNCTION__);
  return(0);
}

//------------------------------------------------------------------------------

int scan_return(void)
{
  int idx = cline_i;
  OP_STACK_ENTRY op;
  char v[NOBJ_VARNAME_MAXLEN+1];

  indent_more();
  
  init_op_stack_entry(&op);
  
  dbprintf("%s:", __FUNCTION__);
  
  if( check_literal(&idx, " RETURN"))
    {
      // We may or may not have an expression after the 
      if(check_expression(&idx))
	{
	  int num_subexpr;
	  
	  if( scan_expression(&num_subexpr) )
	    {
	      dbprintf("%s:ret1 Expression present", __FUNCTION__);

	      // The expression we just scanned needs to be the same type as the procedure we
	      // are translating.
	      
	      //	      op.type = NOBJ_VARTY;
	      op.buf_id = EXP_BUFF_ID_RETURN;
	      strcpy(op.name, "RETURN");
	      process_token(&op);
	      return(1);
	    }
	}
      else
	{
	  // No expression after the RETURN, this is valid
	  // The type is void
	  
	  dbprintf("%s:ret1 Expression not present", __FUNCTION__);
	  op.buf_id = EXP_BUFF_ID_RETURN;
	  op.type = NOBJ_VARTYPE_VOID;
	  strcpy(op.name, "RETURN");
	  process_token(&op);
	  return(1);
	}
    }
  
  dbprintf("%s:ret0", __FUNCTION__);
  return(0);
}

////////////////////////////////////////////////////////////////////////////////

int check_proc_call(int *index)
{
  int idx = *index;
  char textlabel[NOBJ_VARNAME_MAXLEN+1];
  indent_more();
  
  dbprintf("%s:", __FUNCTION__);
  if( check_textlabel(&idx, textlabel))
    {
      if( check_literal(&idx, ":") )
	{
	  dbprintf("%s:ret1", __FUNCTION__);
	  *index = idx;
	  return(1);
	}

      if( check_literal(&idx, "%:") )
	{
	  dbprintf("%s:ret1", __FUNCTION__);
	  *index = idx;
	  return(1);
	}

      if( check_literal(&idx, "$:") )
	{
	  dbprintf("%s:ret1", __FUNCTION__);
	  *index = idx;
	  return(1);
	}
    }
  
  dbprintf("%s:ret0", __FUNCTION__);
  return(0);
}

//------------------------------------------------------------------------------

int scan_proc_call(void)
{
  int idx = cline_i;
  OP_STACK_ENTRY op;
  char textlabel[NOBJ_VARNAME_MAXLEN+1];
  indent_more();
  
  init_op_stack_entry(&op);
  
  dbprintf("%s:", __FUNCTION__);
  if( check_textlabel(&idx, textlabel))
    {
      dbprintf("%s:*** '%s'", __FUNCTION__, textlabel);
      fflush(ofp);
      
      cline_i = idx;
      
      if( scan_literal(":") )
	{
	  dbprintf("%s:ret1", __FUNCTION__);
	  op.type = NOBJ_VARTYPE_FLT;
	  op.buf_id = EXP_BUFF_ID_PROC_CALL;
	  strcpy(op.name, textlabel);
	  process_token(&op);
	  return(1);
	}

      if( scan_literal("%:") )
	{
	  dbprintf("%s:ret1", __FUNCTION__);
	  op.type = NOBJ_VARTYPE_INT;
	  strcpy(op.name, textlabel);
	  process_token(&op);
	  return(1);
	}

      if( scan_literal("$:") )
	{
	  dbprintf("%s:ret1", __FUNCTION__);
	  op.type = NOBJ_VARTYPE_STR;
	  strcpy(op.name, textlabel);
	  process_token(&op);
	  return(1);
	}
    }
  
  dbprintf("%s:ret0", __FUNCTION__);
  return(0);
}

////////////////////////////////////////////////////////////////////////////////

int check_line(int *index)
{
  OP_STACK_ENTRY op;
  int idx = *index;

  init_op_stack_entry(&op);
  
  indent_more();
  
  drop_space(&idx);
  
  dbprintf("%s:", __FUNCTION__);

  idx = cline_i;
  if( check_assignment(&idx) )
    {
      dbprintf("%s:ret1", __FUNCTION__);
  
      *index = idx;
      return(1);
    }

  idx = cline_i;
  if( check_return(&idx) )
    {
      dbprintf("%s:ret1", __FUNCTION__);
  
      *index = idx;
      return(1);
    }

  idx = cline_i;
  if( check_proc_call(&idx) )
    {
      dbprintf("%s:ret1", __FUNCTION__);
  
      *index = idx;
      return(1);
    }

  idx = cline_i;

  if( check_command(&idx) )
    {
      dbprintf("%s:ret1", __FUNCTION__);
      *index = idx;
      return(1);
    }

  idx = cline_i;

  if( check_function(&idx) )
    {
      dbprintf("%s:ret1", __FUNCTION__);
      *index = idx;
      return(1);
    }

#if 0
  idx = cline_i;
  if( check_literal(&idx," LOCAL"))
    {
      dbprintf("%s:ret1", __FUNCTION__);
      *index = idx;
      return(1);
    }

  idx = cline_i;
  if( check_literal(&idx," GLOBAL"))
    {
      dbprintf("%s:ret1", __FUNCTION__);
      *index = idx;
      return(1);
    }
#endif
  
  idx = cline_i;
  if( check_literal(&idx," IF"))
    {
      strcpy(op.name, "IF");

      dbprintf("%s:ret1", __FUNCTION__);
      *index = idx;
      return(1);
    }

  idx = cline_i;
  if( check_literal(&idx," ELSEIF"))
    {
      if( check_expression(&idx) )
	{
	  dbprintf("%s:ret1", __FUNCTION__);
	  *index = idx;
	  return(1);
	}
    }
  
  idx = cline_i;
  if( check_literal(&idx," ELSE"))
    {
      dbprintf("%s:ret1", __FUNCTION__);
      *index = idx;
      return(1);
    }

  idx = cline_i;
  if( check_literal(&idx," ENDIF"))
    {
      dbprintf("%s:ret1", __FUNCTION__);
      *index = idx;
      return(1);
    }

  idx = cline_i;
  if( check_literal(&idx," DO"))
    {
      dbprintf("%s:ret1", __FUNCTION__);
      *index = idx;
      return(1);
    }

  idx = cline_i;
  if( check_literal(&idx," WHILE"))
    {
      dbprintf("%s:ret1", __FUNCTION__);
      *index = idx;
      return(1);
    }

  idx = cline_i;
  if( check_literal(&idx," ENDWH"))
    {
      dbprintf("%s:ret1", __FUNCTION__);
      *index = idx;
      return(1);
    }

  idx = cline_i;
  if( check_literal(&idx," REPEAT"))
    {
      dbprintf("%s:ret1", __FUNCTION__);
      *index = idx;
      return(1);
    }

  idx = cline_i;
  if( check_literal(&idx," UNTIL"))
    {
      if( check_expression(&idx) )
	{
	  dbprintf("%s:ret1", __FUNCTION__);
	  *index = idx;
	  return(1);
	}
    }
  
  idx = cline_i;
  if( check_literal(&idx," GOTO"))
    {
      char textlabel[NOBJ_VARNAME_MAXLEN+1];
      
      if( check_label(&idx) )
	{
	  dbprintf("%s:ret1", __FUNCTION__);
	  *index = idx;
	  return(1);
	}
    }

  dbprintf("%s:ret1", __FUNCTION__);
  *index = idx;
  return(0);
}

////////////////////////////////////////////////////////////////////////////////

int scan_line()
{
  int idx = cline_i;
  char cmdname[300];

  indent_more();
  
  drop_space(&cline_i);
  
  dbprintf("%s:", __FUNCTION__);
  
  idx = cline_i;
  if( check_assignment(&idx) )
    {
      return(scan_assignment());
    }

  idx = cline_i;
  if( check_proc_call(&idx) )
    {
      return(scan_proc_call());
    }

  idx = cline_i;
  if( check_return(&idx) )
    {
      return(scan_return());
    }

  idx = cline_i;
  if( check_command(&idx) )
    {
      dbprintf("%s:check_command: ", __FUNCTION__);
      scan_command(cmdname);
      return(1);
    }

  idx = cline_i;
  if( check_function(&idx) )
    {
      dbprintf("%s:check_command: ", __FUNCTION__);
      scan_function(cmdname);
      return(1);
    }

#if 0
  idx = cline_i;
  if( check_literal(&idx," LOCAL") )
    {
      scan_literal(" LOCAL");
      return(1);
    }

  idx = cline_i;
  if( check_literal(&idx," GLOBAL") )
    {
      scan_literal(" GLOBAL");
      return(1);
    }
#endif
  
  idx = cline_i;
  if( check_literal(&idx," IF") )
    {
      if( scan_literal(" IF") )
	{
	  int num_subexpr;
	  
	  if( scan_expression(&num_subexpr) )
	    {
	      return(1);
	    }
	}
      
      return(0);
    }

  idx = cline_i;
  if( check_literal(&idx," ELSEIF") )
    {
      if( scan_literal(" ELSEIF") )
	{
	  int num_subexpr;
	  
	  if( scan_expression(&num_subexpr) )
	    {
	      return(1);
	    }
	}
    }

  idx = cline_i;
  if( check_literal(&idx," ELSE") )
    {
      scan_literal(" ELSE");
      return(1);
    }


  idx = cline_i;
  if( check_literal(&idx," ENDIF") )
    {
      scan_literal(" ENDIF");
      return(1);
    }

  idx = cline_i;
  if( check_literal(&idx," DO") )
    {
      scan_literal(" DO");
      return(1);
    }

  idx = cline_i;
  if( check_literal(&idx," WHILE") )
    {
      if( scan_literal(" WHILE") )
	{
	  int sub_expr;
	  if( scan_expression(&sub_expr) )
	    {
	      dbprintf("%s: ret1", __FUNCTION__);
	      return(1);
	    }
	}
      
      dbprintf("%s: ret0", __FUNCTION__);
      return(0);
    }

  idx = cline_i;
  if( check_literal(&idx," ENDWH") )
    {
      scan_literal(" ENDWH");
      return(1);
    }

  idx = cline_i;
  if( check_literal(&idx," REPEAT") )
    {
      scan_literal(" REPEAT");
      return(1);
    }
  
  idx = cline_i;
  if( check_literal(&idx," UNTIL") )
    {
      if( scan_literal(" UNTIL") )
	{
	  int sub_expr;
	  if( scan_expression(&sub_expr) )
	    {
	      dbprintf("%s: ret1", __FUNCTION__);
	      return(1);	      
	    }
	}

      dbprintf("%s: ret0", __FUNCTION__);
      return(0);
    }
  
  idx = cline_i;
  if( check_literal(&idx," GOTO"))
    {
      cline_i = idx;
      
      if( scan_label() )
	{
	  //	  *index = idx;
	  return(1);
	}
    }

  cline_i = idx;
  dbprintf("%s: ret0", __FUNCTION__);
  return(0);    
}


////////////////////////////////////////////////////////////////////////////////

int scan_cline(void)
{
  int idx = cline_i;
  int ret = 0;

  indent_more();
  
  dbprintf("%s:", __FUNCTION__);
  
  drop_space(&idx);
  
  while( check_line(&idx) && (strlen(&(cline[idx])) > 0))
    {
      dbprintf("%s: Checked len=%ld, '%s'", __FUNCTION__, strlen(&(cline[idx])), &(cline[idx]));
      //cline_i = idx;

      output_expression_start(cline);
      if( !scan_line() )
	{
	  finalise_expression();
	  
	  dbprintf("%s: scan_line==0 len=%ld '%s'", __FUNCTION__, strlen(&(cline[idx])), &(cline[idx]));
	  syntax_error("Syntax error in line");
	  return(0);
	}

      finalise_expression();
      
      idx = cline_i;
      
      drop_space(&idx);
      
      if ( check_literal(&idx,":") )
	{
	  cline_i = idx;
	  scan_literal(":");
	}
      else
	{
	  if( strlen(&(cline[idx])) == 0 )
	    {
	      return(1);
	    }
	  else
	    {
	      return(0);
	    }
	}

      //      idx = cline_i;
      drop_space(&cline_i);
      //cline_i = idx;
      
    }

  dbprintf("%s: after wh len=%ld '%s'", __FUNCTION__, strlen(&(cline[idx])), &(cline[idx]));
  syntax_error("Syntax error in line");
  
  if( strlen(&(cline[cline_i])) == 0 )
    {
      return(0);
    }
  
  return(0);
}

////////////////////////////////////////////////////////////////////////////////

int scan_procdef(void)
{
  int idx = cline_i;
  char textlabel[NOBJ_VARNAME_MAXLEN+1];
  indent_more();
  
  dbprintf("%s:", __FUNCTION__);
  if( check_textlabel(&idx, textlabel))
    {
      cline_i = idx;
      
      if( scan_literal(":") )
	{
	  dbprintf("%s:ret1", __FUNCTION__);
	  strcpy(procedure_name, textlabel);
	  
	  return(1);
	}
    }
  
  dbprintf("%s:ret0", __FUNCTION__);
  return(0);
}

////////////////////////////////////////////////////////////////////////////////
//
// Scans LOCAL and GLOBAL
//
#define SCAN_LOCAL   1
#define SCAN_GLOBAL  0

int scan_localglobal(int local_nglobal)
{
  int idx = cline_i;
  char varname[NOBJ_VARNAME_MAXLEN+1];
  NOBJ_VAR_INFO vi;
  char *keyword;
  indent_more();
  
  init_var_info(&vi);
  
  if( local_nglobal )
    {
      keyword = " LOCAL";
    }
  else
    {
      keyword = " GLOBAL";
    }
  
  dbprintf("%s:", __FUNCTION__);
  
  if( scan_literal(keyword) )
    {
      idx = cline_i;


      while( check_variable(&idx) )
	{
	  init_var_info(&vi);
	  
	  scan_variable(varname, &vi, VAR_DECLARE);
	  vi.is_global = !local_nglobal;
	  vi.is_ref = 0;
	  
	  dbprintf("%s:%s variable:'%s'", __FUNCTION__, keyword, varname);
	  print_var_info(&vi);
	  
	  idx = cline_i;
	  if( check_literal(&idx, " ,") )
	    {
	      scan_literal(" ,");
	    }
	}

      drop_space(&cline_i);

      if( cline[cline_i] == '\0' )
	{
	  dbprintf("%s:ret1", __FUNCTION__);
	  return(1);
	}
    }

  dbprintf("%s:ret0", __FUNCTION__);
  return(0);
}

////////////////////////////////////////////////////////////////////////////////

int check_declare(int *index)
{
  int idx = *index;
  indent_more();
  
  dbprintf("%s:", __FUNCTION__);
  
  if( check_literal(&idx, " LOCAL") )
    {
      dbprintf("%s:ret 1", __FUNCTION__);
      *index = idx;
      return(1);
    }

  if( check_literal(&idx, " GLOBAL") )
    {
      dbprintf("%s:ret 1", __FUNCTION__);
      *index = idx;
      return(1);
    }

  dbprintf("%s:ret 0", __FUNCTION__);
  *index = idx;
  return(0);
}

//------------------------------------------------------------------------------

int scan_declare(void)
{
  int idx = cline_i;
  indent_more();
  
  dbprintf("%s:", __FUNCTION__);
  
  if( check_literal(&idx, " LOCAL") )
    {
      if( scan_localglobal(SCAN_LOCAL) )
	{
	  dbprintf("%s:ret 1", __FUNCTION__);
	  return(1);
	}
      else
	{
	  syntax_error("Bad LOCAL");
	}
    }

  idx = cline_i;
  
  if( check_literal(&idx, " GLOBAL") )
    {
      if( scan_localglobal(SCAN_GLOBAL) )
	{
	  dbprintf("%s:ret 1", __FUNCTION__);
	  return(1);
	}
      else
	{
	  syntax_error("Bad GLOBAL");
	}
    }

  dbprintf("%s:ret 0", __FUNCTION__);
  return(0);
}

void parser_check(void)
{
  dbprintf("NUM_BUFF_ID    :%d", NUM_BUFF_ID);
  dbprintf("EXP_BUFF_ID_MAX:%d", EXP_BUFF_ID_MAX);
  dbprintf("");
  
  assert( NUM_BUFF_ID == (EXP_BUFF_ID_MAX +1));
}
