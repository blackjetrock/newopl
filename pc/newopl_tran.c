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

// Reads next composite line into buffer

char current_expression[200];
int first_token = 1;

OP_STACK_ENTRY op_stack[NOPL_MAX_OP_STACK+1];

int op_stack_ptr = 0;


NOBJ_VARTYPE exp_type_stack[MAX_EXP_TYPE_STACK];
int exp_type_stack_ptr = 0;

#define SAVE_I     1
#define NO_SAVE_I  0

#define VAR_REF      1
#define VAR_DECLARE  0

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
NOBJ_VARTYPE char_to_type(char ch);

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

////////////////////////////////////////////////////////////////////////////////
//
// Operator info
//
// Some operators have a mutable type, they are polymorphic. Some are not.
// The possible types for operators are listed here
//

#if 0
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
   { "@",         9, 0,   MUTABLE_TYPE, 1, {NOBJ_VARTYPE_INT, NOBJ_VARTYPE_FLT, NOBJ_VARTYPE_STR} },
   { "=",         1, 0,   MUTABLE_TYPE, 1, {NOBJ_VARTYPE_INT, NOBJ_VARTYPE_FLT, NOBJ_VARTYPE_STR} },
   { ":=",        1, 0,   MUTABLE_TYPE, 1, {NOBJ_VARTYPE_INT, NOBJ_VARTYPE_FLT, NOBJ_VARTYPE_STR} },
   { "+",         3, 0,   MUTABLE_TYPE, 0, {NOBJ_VARTYPE_INT, NOBJ_VARTYPE_FLT, NOBJ_VARTYPE_STR} },
   { "-",         3, 0,   MUTABLE_TYPE, 0, {NOBJ_VARTYPE_INT, NOBJ_VARTYPE_FLT, NOBJ_VARTYPE_STR} },
   { "*"     ,    5, 1,   MUTABLE_TYPE, 0, {NOBJ_VARTYPE_INT, NOBJ_VARTYPE_FLT, NOBJ_VARTYPE_INT} },
   { "/     ",    5, 1,   MUTABLE_TYPE, 0, {NOBJ_VARTYPE_INT, NOBJ_VARTYPE_FLT, NOBJ_VARTYPE_INT} },
   { "     >",    5, 1,   MUTABLE_TYPE, 0, {NOBJ_VARTYPE_INT, NOBJ_VARTYPE_FLT, NOBJ_VARTYPE_STR} },
   {      "AND",  5, 1,   MUTABLE_TYPE, 0, {NOBJ_VARTYPE_INT, NOBJ_VARTYPE_FLT, NOBJ_VARTYPE_INT} },
   // (Handle bitwise on integer, logical on floats somewhere)
   //{ ",",  0, 0 }, /// Not used?
    
   // LZ only
   { "+%",        5, 1, IMMUTABLE_TYPE, 0, {NOBJ_VARTYPE_FLT, NOBJ_VARTYPE_FLT, NOBJ_VARTYPE_FLT} },
   { "-%",        5, 1, IMMUTABLE_TYPE, 0, {NOBJ_VARTYPE_FLT, NOBJ_VARTYPE_FLT, NOBJ_VARTYPE_FLT} },
  };

#endif


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
      if( !isalnum(token[i]) )
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
#if 1
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
  
  return(0);
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
#endif

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

void output_qcode(void)
{

  // Run through the final expression buffer, converting into QCode
  for(int i=0; i<exp_buffer2_i; i++)
    {
      EXP_BUFFER_ENTRY token = exp_buffer2[i];

      if( (exp_buffer2[i].op.buf_id < 0) || (exp_buffer2[i].op.buf_id > EXP_BUFF_ID_MAX) )
	{
	  dbprintf("\nN%d op.buf_id invalid", token.node_id);
	}
      
      fprintf(ofp, "\n(%16s) N%d %-24s %c rq:%c %s", __FUNCTION__, token.node_id, exp_buffer_id_str[exp_buffer2[i].op.buf_id], type_to_char(token.op.type), type_to_char(token.op.req_type), exp_buffer2[i].name);
      
      fprintf(ofp, "  %d:", token.p_idx);
      for(int pi=0; pi<token.p_idx; pi++)
	{
	  fprintf(ofp, " %d", token.p[pi]);
	}
    }

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
	  break;

	case NOBJ_VARTYPE_STR:
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
      exit(-1);
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
      exit(-1);
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
  
  fprintf(ofp, "\n\nType Check Stack (%d)\n", type_check_stack_ptr);

  for(int i=0; i<type_check_stack_ptr; i++)
    {
      s = type_check_stack[i].name;
      type = type_check_stack[i].op.type;
      
      fprintf(ofp, "\n%03d: '%s' type:%c (%d)", i, s, type_to_char(type), type);
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

void dump_exp_buffer(void)
{
  char *idstr;
  
  fprintf(ofp, "\nExpression buffer");
  fprintf(ofp, "\n=================");
  
  for(int i=0; i<exp_buffer_i; i++)
    {
      EXP_BUFFER_ENTRY token = exp_buffer[i];
      
      fprintf(ofp, "\n(%16s) N%d %-24s %c rq:%c '%s' nidx:%d", __FUNCTION__, token.node_id, exp_buffer_id_str[exp_buffer[i].op.buf_id], type_to_char(token.op.type), type_to_char(token.op.req_type), exp_buffer[i].name, token.op.vi.num_indices);
      
      fprintf(ofp, "  %d:", token.p_idx);
      for(int pi=0; pi<token.p_idx; pi++)
	{
	  fprintf(ofp, " %d", token.p[pi]);
	}
    }
  
  fprintf(ofp, "\n=================");
}

void dump_exp_buffer2(FILE *fp)
{
  char *idstr;
  
  fprintf(fp, "\nExpression buffer 2");
  fprintf(fp, "\n===================");
  
  for(int i=0; i<exp_buffer2_i; i++)
    {
      EXP_BUFFER_ENTRY token = exp_buffer2[i];

      if( (exp_buffer2[i].op.buf_id < 0) || (exp_buffer2[i].op.buf_id > EXP_BUFF_ID_MAX) )
	{
	  fprintf(fp, "\nN%d op.buf_id invalid", token.node_id);
	}
      
      fprintf(fp, "\n(%16s) N%d %-24s %c rq:%c '%s'", __FUNCTION__, token.node_id, exp_buffer_id_str[exp_buffer2[i].op.buf_id], type_to_char(token.op.type), type_to_char(token.op.req_type), exp_buffer2[i].name);
      
      fprintf(fp, "  %d:", token.p_idx);
      for(int pi=0; pi<token.p_idx; pi++)
	{
	  fprintf(fp, " %d", token.p[pi]);
	}
    }
  fprintf(fp, "\n=================");
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

  dump_exp_buffer2(ofp);
  
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

	  dump_exp_buffer2(ofp);
	  return(1);	  
	}
    }
  
  return(0);
}

////////////////////////////////////////////////////////////////////////////////
//
// return a type that can be reached by as few type conversions as possible.

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
      exit(-1);
    }
}

// Copies data into string

void infix_stack_pop(char *entry)
{
  EXP_BUFFER_ENTRY o;
  
  if( infix_stack_ptr == 0 )
    {
      fprintf(ofp, "\n%s: Operator stack empty", __FUNCTION__);
      exit(-1);
    }
  
  infix_stack_ptr --;

  strcpy(entry, infix_stack[infix_stack_ptr]);
  
  fprintf(ofp, "\n%s: '%s'", __FUNCTION__, entry);
}

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

      switch(be.op.buf_id)
	{
	case EXP_BUFF_ID_VARIABLE:
	  fprintf(ofp, "\nVar: %s ary:%d NumIdx:%d", be.name,
		  be.op.vi.is_array,
		  be.op.vi.num_indices);

	  if( be.op.vi.is_array )
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
	  
	case EXP_BUFF_ID_AUTOCON:
	  break;
	}
    }

  // There may not be a result if there was just a command
  result[0] = '\0';
  
  if( infix_stack_ptr != 0 )
    {
      infix_stack_pop(result);
    }
  else
    {
      fprintf(ofp, "\nInfix stack empty");
    }
  return(result);
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
  
  // We copy results over to a second buffer, this allows easy insertion of
  // needed extra codes

  exp_buffer2_i = 0;
  
  // We can check for an assignment and adjust the assignment token to
  // differentiate it from the equality token.

  // If first token is a variable
  if( exp_buffer[0].op.buf_id ==  EXP_BUFF_ID_VARIABLE)
    {
      // and the last token is an '=', then this is an assignment
      if( strcmp(exp_buffer[exp_buffer_i-1].name, "=") == 0 )
	{
	  // Assignment, make token more specific
	  strcpy(exp_buffer[exp_buffer_i-1].name, ":=");
	}
    }
  
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

	case EXP_BUFF_ID_VARIABLE:
	  be.p_idx = 0;
	  type_check_stack_push(be);
	  break;

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

	  // Operators have to be typed correctly depending on their
	  // operands. Some of them are mutable (polymorphic) and we have to bind them to their
	  // type here.
	  // Some are immutable and cause errors if theior operators are not correct
	  
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
		      res.op.type      = op1.op.type;
		      res.op.req_type  = op1.op.type;
		      type_check_stack_push(res);
					    
		    }
		  else
		    {
		      // Error
		      fprintf(ofp, "\nType of %s or %s is not %c", op1.name, op2.name, type_to_char(op_info.type[0]));
		      exit(-1);
		    }
		}
	      else
		{
		  // Mutable type is dependent on the arguments, e.g.
		  //  A$ = "RTY"
		  // requires that a string equality is used, similarly
		  // INT and FLT need the correctly typed operator.

		  // INT and FLT have an additional requirement where INT is used
		  // as long as possible, and also assignment can turn FLT into INT
		  // or INT into FLT
		  
		  fprintf(ofp, "\n Mutable type %d %d", op1.op.type, op2.op.type);
		  
		  // Check types are valid for this operator
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
			      // The types of the operands are the same as the required type, all ok
			      be.op.type = op1.op.type;
			      be.op.req_type = op1.op.req_type;
			    }
			  else
			    {
			      // The types of the argument aren't the required type, we may be able to
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

				  // Insert entry
				  insert_buf2_entry_after_node_id(op1.node_id, autocon);
				  insert_buf2_entry_after_node_id(op2.node_id, autocon);
				  
				  break;
				  
				default:
				  // No auto conversion is available, so this is an error
				  fprintf(ofp, "\nType is not the require dtype and no auto conversion available,");
				  fprintf(ofp, "\n Node N%d", be.node_id);
				  exit(-1);
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
			      be.op.type = op2.op.type;
			      be.op.req_type = op2.op.req_type;
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


			  //			  exp_buffer2[exp_buffer2_i++] = be;
 
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
		      fprintf(ofp, "\n%s:Syntax error at node N%d", __FUNCTION__, be.node_id);
		      exit(-1);
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
  
  dump_exp_buffer();
  typecheck_expression();
  dump_exp_buffer();
  dump_exp_buffer2(ofp);
  dump_exp_buffer2(icfp);
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
  op->name[0] = '\0';
  
}

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

void output_return(OP_STACK_ENTRY op)
{
  printf("\nop return");
  fprintf(ofp, "\n(%16s) %s %c %c %s", __FUNCTION__, type_stack_str(), type_to_char(op.type), type_to_char(op.req_type), op.name); 
  add_exp_buffer_entry(op, EXP_BUFF_ID_RETURN);
}

void output_print(OP_STACK_ENTRY op)
{
  printf("\nop print");
  fprintf(ofp, "\n(%16s) %s %c %c %s", __FUNCTION__, type_stack_str(), type_to_char(op.type), type_to_char(op.req_type), op.name); 
  add_exp_buffer_entry(op, op.buf_id);
}

void output_proc_call(OP_STACK_ENTRY op)
{
  printf("\nop proc call");
  fprintf(ofp, "\n(%16s) %s %c %c %s", __FUNCTION__, type_stack_str(), type_to_char(op.type), type_to_char(op.req_type), op.name); 
  add_exp_buffer_entry(op, EXP_BUFF_ID_PROC_CALL);
}

#if 0
void output_assign(OP_STACK_ENTRY op)
{
  printf("\nop operator");
  strcpy(op.name, ":=");
  
  fprintf(ofp, "\n(%16s) %s %c %c %s", __FUNCTION__, type_stack_str(), type_to_char(op.type), type_to_char(op.req_type), op.name); 
  add_exp_buffer_entry(op, EXP_BUFF_ID_OPERATOR);
}
#endif

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
      fprintf(ofp, "\n%s: Operator stack empty", __FUNCTION__);
      exit(-1);
    }
  
  op_stack_ptr --;

  o = op_stack[op_stack_ptr];
  fprintf(ofp, "\nPop '%s'", o.name);
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

  dbprintf("\n------------------");
  dbprintf("\nOperator Stack     (%d)\n", op_stack_ptr);
  
  for(int i=0; i<op_stack_ptr; i++)
    {
      s = op_stack[i].name;
      dbprintf("\n%03d: %s type:%d", i, s, op_stack[i].type);
    }

  dbprintf("\n------------------\n");
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

  fprintf(ofp, "\nFinalise stack");
  while( strlen(op_stack_top().name) != 0 )
    {
      o = op_stack_pop();

      if( o.req_type = NOBJ_VARTYPE_UNKNOWN )
	{
	  o.req_type = expression_type;
	  o.type = expression_type;
	}
      
      output_operator(o);
    }

}

void process_expression_types(void)
{
  char *infix;

  dbprintf("\n%s:", __FUNCTION__);
  
  if( strlen(current_expression) > 0 )
    {
      // Process the RPN as a tree
      expression_tree_process(current_expression);

      dbprintf("\n==INFIX==\n",0);
      dbprintf("==%s==", infix = infix_from_rpn());
      dbprintf("\n\n",0);

      fprintf(chkfp, "\n%s", infix);
      
      // Generate the QCode from the tree output
      output_qcode();
    }
}

void init_output(void)
{
  ofp = fopen("output.txt", "w");
  chkfp = fopen("check.txt", "w");
  icfp = fopen("intcode.txt", "w");
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
      fprintf(ofp, "\nExp stack empty on pop");
      exit(-1);
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

void process_token(OP_STACK_ENTRY *token)
{
  char *tokptr;
  OP_STACK_ENTRY o1;
  OP_STACK_ENTRY o2;
  int opr1, opr2;
  
  fprintf(ofp, "\n   Frst:%d T:'%s' exptype:%c bufid:'%s'", first_token, token->name, type_to_char(expression_type), exp_buffer_id_str[token->buf_id]);

  o1 = *token;
  //strcpy(o1.name, token);
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
	  dbprintf("\nPop 2");
	  o2 = op_stack_pop();
	  output_operator(o2);
	}

      //output_marker("-------- Comma 2");
      output_sub_start();
      
      // Commas delimit sub expressions, reset the expression type.
      expression_type = NOBJ_VARTYPE_UNKNOWN;
      first_token = 0;
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

      // Sub expression, push (save) the expression type and process the sub expression
      // as a new one
      exp_type_push(expression_type);
      expression_type = NOBJ_VARTYPE_UNKNOWN;
      first_token = 0;
      
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
	}

      fprintf(ofp, "\nPop 4");
      o2 = op_stack_pop();

      //output_marker("-------- Sub E 2");
      output_sub_end();
      
      if( strcmp(o2.name, "(") != 0 )
	{
	  dbprintf("\n**** Should be left parenthesis");
	}

      if( token_is_function(op_stack_top().name, &tokptr) )
	{
	  fprintf(ofp, "\nPop 5");
	  o2 = op_stack_pop();
	  output_function(o2);
	}

      expression_type = exp_type_pop();

      output_sub_end();
      //output_marker("-------- Sub E 3");
      first_token = 0;
      return;
    }

    switch( o1.buf_id )
    {
    case EXP_BUFF_ID_PRINT:
    case EXP_BUFF_ID_PRINT_SPACE:
    case EXP_BUFF_ID_PRINT_NO_CR:
      fprintf(ofp, "\nBuff id print");

      // PRINT has special parsing and the CRLF flag processing
      o1.req_type = expression_type;
      output_print(o1);
      //return;
      break;

    case EXP_BUFF_ID_RETURN:
      fprintf(ofp, "\nBuff id return");

      // RETURN needs to change depending on the type of the expression we are to return.
      // The type of that expression must also match that of the procedure we are translating
      o1.req_type = expression_type;
      output_return(o1);
      return;
      break;
      
    case EXP_BUFF_ID_PROC_CALL:
      fprintf(ofp, "\nBuff id proc call");
      
      // Parser supplies type
      o1.req_type = expression_type;
      output_proc_call(o1);
      return;
      break;
#if 0
    case EXP_BUFF_ID_ASSIGN:
      fprintf(ofp, "\nBuff id assign");
      
      // Parser supplies type
      o1.req_type = expression_type;
      output_assign(o1);
      return;
      break;
#endif 
    }

#define OP_PREC(OP) (operator_precedence(OP.name))
  
  if( token_is_operator(o1.name, &(tokptr)) )
    {
      dbprintf("\nToken is operator o1 name:%s o2 name:%s", o1.name, o2.name);
      dbprintf("\nopr1:%d opr2:%d", opr1, opr2);
      
      while( (strlen(op_stack_top().name) != 0) && (strcmp(op_stack_top().name, ")") != 0 ) &&
	     ( OP_PREC(op_stack_top()) > opr1) || ((opr1 == OP_PREC(op_stack_top()) && operator_left_assoc(o1.name)))
	     )
	{
	  fprintf(ofp, "\nPop 1");
	  
	  o2 = op_stack_pop();
	  opr1 = operator_precedence(o1.name);
	  opr2 = operator_precedence(o2.name);

	  output_operator(o2);
	}

      fprintf(ofp, "\nPush 1");
      
      strcpy(o1.name, tokptr);
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
      return;
    }

  if( token_is_integer(o1.name) )
    {
      o1.type = NOBJ_VARTYPE_INT;
      
      modify_expression_type(o1.type);
      o1.req_type = expression_type;
      output_integer(o1);
      first_token = 0;
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

            if( o1.vi.is_array )
	{
#if 0
	  OP_STACK_ENTRY ob;

	  init_op_stack_entry(&ob);
	  ob.req_type = o1.req_type;
	  ob.type = o1.type;
	  strcpy(ob.name, "@");
	  
	  // Array index calculations will follow, we use an operator to
	  // bind them to the variable reference
	  //	  output_operator(ob);

	  process_token(&ob);	  
#endif
	}

      // The type of the variable will affect the expression type
      
      output_variable(o1);
      first_token = 0;

      
      return;
    }

  if( token_is_string(o1.name) )
    {
      o1.type = NOBJ_VARTYPE_STR;
      output_string(o1);
      modify_expression_type(o1.type);
      first_token = 0;
      return;
    }
  
  first_token = 0;

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
  dbprintf("\nFinalise expression");

  // Now finalise the translation
  op_stack_finalise();
  process_expression_types();
  
  dbprintf("\nFinalise expression done.");
}

void dummy(void)
{
  int num_commas;
  
  // Assignment can be done using an expression
  scan_expression(&num_commas, HEED_COMMA);
  finalise_expression();
  process_expression_types();
}

////////////////////////////////////////////////////////////////////////////////
//
//
//
//
////////////////////////////////////////////////////////////////////////////////

int n_lines_ok    = 0;
int n_lines_bad   = 0;
int n_lines_blank = 0;

void translate_file(FILE *fp, FILE *ofp)
{
  char line[MAX_NOPL_LINE+1];
  int all_spaces = 0;
  int scanned_procdef = 0;
  int done_declares = 0;
  
  // Read lines from file and translate each line as a unit
  // Lines are separated by cr or ':'
  
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

      ////////////////
      
      fprintf(ofp, "\n");
      for(int i=0; i<strlen(cline)+4; i++)
	{
	  fprintf(ofp, "*");
	}

      fprintf(ofp, "\n**%s**", cline);

      fprintf(ofp, "\n");

      for(int i=0; i<strlen(cline)+4; i++)
	{
	  fprintf(ofp, "*");
	}
      fprintf(ofp, "\n");

      indent_none();
      
      // Recursive decent parse

      if( !scanned_procdef )
	{
	  //output_expression_start(cline);
	  if( scan_procdef() )
	    {
	      scanned_procdef = 1;
	      n_lines_ok++;
	      dbprintf("\ncline scanned OK");
	      //finalise_expression();
	      continue;
	    }
	  else
	    {
	      n_lines_bad++;
	      dbprintf("\ncline failed scan");
	    }
	}

      // Variable declarations
      if( !done_declares )
	{
	  int idx = cline_i;
	  
	  if( check_declare(&idx) )
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
	      //finalise_expression();	      
	      continue;
	    }
	  else
	    {
	      // Not a declaration so we are done with them
	      done_declares = 1;
	    }
	}

      indent_none();
      
      if( scan_cline() )
	{
	  n_lines_ok++;
	  dbprintf("\ncline scanned OK");
	  
	}
      else
	{
	  n_lines_bad++;
	  dbprintf("\ncline failed scan");
	}
    }
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
  
  init_output();
  
  // Open file and process on a line by line basis
  fp = fopen(argv[1], "r");

  if( fp == NULL )
    {
      fprintf(ofp, "\nCould not open '%s'", argv[1]);
      exit(-1);
    }

  //ofp = fopen("out.opl.tran", "w");

  parser_check();
  
  translate_file(fp, ofp);

  dump_exp_buffer();
  dump_exp_buffer2(ofp);
  
  fclose(fp);

  fclose(chkfp);
  


  dbprintf("\n");
  dbprintf("\n %d lines scanned Ok",       n_lines_ok);
  dbprintf("\n %d lines scanned failed",   n_lines_bad);
  dbprintf("\n %d lines blank",            n_lines_blank);
  dbprintf("\n");

  printf("\n");
  printf("\n");
  printf("\n %d lines scanned Ok",       n_lines_ok);
  printf("\n %d lines scanned failed",   n_lines_bad);
  printf("\n %d lines blank",            n_lines_blank);
  printf("\n");

  printf("\n");
  //  fclose(ofp);
  uninit_output();  
}

