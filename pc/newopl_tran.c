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
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include "nopl.h"
#include "newopl.h"
#include "nopl_obj.h"

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

  for(int i=0; i<strlen(token); i++)
    {
      if( !( isdigit(*token) || (*token == '.')))
	{
	  all_digits = 0;
	}

      token++;
    }
  
  return(all_digits);
}

int token_is_integer(char *token)
{
  int all_digits = 1;

  for(int i=0; i<strlen(token); i++)
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
}
  fn_info[] =
  {
    { "EOL" },
    { "=" },
    { "ABS" },
    { "ACOS" },
    { "ADDR" },
    { "APPEND" },
    { "ASC" },
    { "ASIN" },
    { "AT" },
    { "ATAN" },
    { "BACK" },
    { "BEEP" },
    { "BREAK" },
    { "CHR$" },
    { "CLOCK" },
    { "CLOSE" },
    { "CLS" },
    { "CONTINUE" },
    { "COPY" },
    { "COPYW" },
    { "COS" },
    { "COUNT" },
    { "CREATE" },
    { "CURSOR" },
    { "DATIM$" },
    { "DAY" },
    { "DAYNAME$" },
    { "DAYS" },
    { "DEG" },
    { "DELETE" },
    { "DELETEW" },
    { "DIR$" },
    { "DIRW$" },
    { "DISP" },
    { "DOW" },
    { "EDIT" },
    { "EOF" },
    { "ERASE" },
    { "ERR" },
    { "ERR$" },
    { "ESCAPE" },
    { "EXIST" },
    { "EXP" },
    { "FIND" },
    { "FINDW" },
    { "FIRST" },
    { "FIX$" },
    { "FLT" },
    { "FREE" },
    { "GEN$" },
    { "GET" },
    { "GET$" },
    { "GLOBAL" },
    { "GOTO" },
    { "HEX$" },
    { "HOUR" },
    { "IABS" },
    { "INPUT" },
    { "INT" },
    { "INTF" },
    { "KEY" },
    { "KEY$" },
    { "KSTAT" },
    { "LAST" },
    { "LEFT$" },
    { "LEN" },
    { "LN" },
    { "LOC" },
    { "LOCAL" },
    { "LOG" },
    { "LOWER$" },
    { "LPRINT" },
    { "MAX" },
    { "MEAN" },
    { "MENU" },
    { "MENUN" },
    { "MID$" },
    { "MIN" },
    { "MINUTE" },
    { "MONTH" },
    { "MONTH$" },
    { "NEXT" },
    { "NUM$" },
    { "OFF" },
    { "OPEN" },
    { "ONERR" },
    { "PAUSE" },
    { "PEEKB" },
    { "PEEKW" },
    { "PI" },
    { "POKEB" },
    { "POKEW" },
    { "POS" },
    { "POSITION" },
    { "PRINT" },
    { "RAD" },
    { "RAISE" },
    { "RANDOMIZE" },
    { "RECSIZE" },
    { "REM" },
    { "RENAME" },
    { "REPT$" },
    { "RETURN" },
    { "RIGHT$" },
    { "RND" },
    { "SCI$" },
    { "SECOND" },
    { "SIN" },
    { "SPACE" },
    { "SQR" },
    { "STD" },
    { "STOP" },
    { "SUM" },
    { "TAN" },
    { "TRAP" },
    { "UDG" },
    { "UPDATE" },
    { "UPPER$" },
    { "USE" },
    { "USR" },
    { "USR$" },
    { "VAL" },
    { "VAR" },
    { "VIEW" },
    { "WEEK" },
    { "YEAR" },
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

////////////////////////////////////////////////////////////////////////////////

struct _OP_INFO
{
  char *name;
  int  precedence;
  int left_assoc;
}
  op_info[] =
  {
    { "+", 3, 0},
    { "-", 3, 0},
    { "*", 5, 1},
    { "/", 5, 1},
    { ",", 0, 0 },
  };

#define NUM_OPERATORS (sizeof(op_info)/sizeof(struct _OP_INFO))

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
// A new variable has appeared in an expression. Modify the expression based on
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

FILE *ofp;

void output_float(OP_STACK_ENTRY token)
{
  printf("\nop float");
  fprintf(ofp, "\n(%16s) %c %s", __FUNCTION__, type_to_char(token.type), token.name);
}

void output_integer(OP_STACK_ENTRY token)
{
  printf("\nop integer");
  fprintf(ofp, "\n(%16s) %c %s", __FUNCTION__, type_to_char(token.type), token.name);
}

void output_operator(OP_STACK_ENTRY op)
{
  printf("\nop operator");
  fprintf(ofp, "\n(%16s) %c %s", __FUNCTION__, type_to_char(op.type), op.name);
}

void output_variable(OP_STACK_ENTRY op)
{
  printf("\nop variable");
  fprintf(ofp, "\n(%16s) %c %s", __FUNCTION__, type_to_char(op.type), op.name);
}

void output_string(OP_STACK_ENTRY op)
{
  printf("\nop string");
  fprintf(ofp, "\n(%16s) %c %s", __FUNCTION__, type_to_char(op.type), op.name);
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
      
      o.name = "";
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

void process_token(char *token)
{
  char *tokptr;
  OP_STACK_ENTRY o1;
  OP_STACK_ENTRY o2;
  int opr1, opr2;
  
  printf("\n   T:'%s'", token);

  o1.name = token;
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

      // Commas delimit sub expressions, reset the expression type.
      expression_type = NOBJ_VARTYPE_UNKNOWN;
      return;
    }

  if( strcmp(o1.name, "(")==0 )
    {
      OP_STACK_ENTRY o;

      o.name = "(";
      o.type = NOBJ_VARTYPE_UNKNOWN;
      
      op_stack_push(o);
      return;
    }

  if( strcmp(o1.name, ")")==0 )
    {
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
      
      o1.name = tokptr;
      o1.type = expression_type;
      op_stack_push(o1);
      return;
    }
  
  if( token_is_float(o1.name) )
    {
      output_float(o1);
      return;
    }

  if( token_is_integer(o1.name) )
    {
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
      o1.name = tokptr;
      o1.type = expression_type;
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

void process_expression(char *line)
{
  char token[NOPL_MAX_TOKEN+1];
  int i;
  int op_delimiter = 0;
  char cap_string[NOPL_MAX_TOKEN+1];
  int capture_string = 0;
  char ss[2];
  int end_cap_later = 0;

  printf("\n==========================");
  printf("\n%s", line);
  printf("\n==========================");

  
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

