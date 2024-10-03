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



int defline = 1;

int token_is_float(char *token)
{
  int all_digits = 1;

  for(int i=0; i<strlen(token); i++)
    {
      if( !isdigit(*(token)) || (*token == '.') || (*token == '-'))
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

struct _FN_INFO
{
  char *name;
}
  fn_info[] =
  {
    { "SIN" },
    { "COS" },
    { "TAN" },
    { "PRINT" },
    { "EOL" },
    { "GOTO" },
    { "=" },
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
	  
	  printf("\n%s is operator", token);
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

FILE *ofp;

void output_float(char *token)
{
  fprintf(ofp, "\n(%s) %s", __FUNCTION__, token);
}

void output_integer(char *token)
{
  fprintf(ofp, "\n(%s) %s", __FUNCTION__, token);
}

void output_operator(char *op)
{
  fprintf(ofp, "\n(%s) %s", __FUNCTION__, op);
}

void output_variable(char *op)
{
  fprintf(ofp, "\n(%s) %s", __FUNCTION__, op);
}

void output_string(char *op)
{
  fprintf(ofp, "\n(%s) %s", __FUNCTION__, op);
}


// Stack function in operator stack
char *op_stack[NOPL_MAX_OP_STACK+1];
int op_stack_ptr = 0;

void op_stack_push(char *token)
{
  printf("\n Push:'%s'", token);
  
  if( op_stack_ptr < NOPL_MAX_OP_STACK )
    {
      op_stack[op_stack_ptr++] = token;
    }
  else
    {
      printf("\n%s: Operator stack full", __FUNCTION__);
      exit(-1);
    }
}

// Copies data into string

void op_stack_pop(char **token)
{
  if( op_stack_ptr == 0 )
    {
      printf("\n%s: Operator stack empty", __FUNCTION__);
      exit(-1);
    }
  
  op_stack_ptr --;

  *token = op_stack[op_stack_ptr];
  printf("\nPop '%s'", *token);
}

// Return a pointer to the top entry of the stack, empty string if empty
char *op_stack_top(void)
{
  if( op_stack_ptr == 0 )
    {
      return("");
    }

  return( op_stack[op_stack_ptr-1] );
}

void op_stack_display(void)
{
  char *s;
  
  fprintf(ofp, "\n\nOperator Stack\n");
  
  for(int i=0; i<op_stack_ptr-1; i++)
    {
      s = op_stack[i];
      fprintf(ofp, "\n%03d: %s", i, s);
    }
}

// End of shunting algorithm, fush the stack

void op_stack_finalise(void)
{
  char *o;
  
  while( strlen(op_stack_top()) != 0 )
    {
      op_stack_pop(&o);
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
  char *o1, *o2;
  int opr1, opr2;
  
  printf("\n   T:'%s'", token);

  o1 = token;
  
  // Another token has arrived, process it using the shunting algorithm
  // First, check the stack for work to do

  o2 = op_stack_top();
  opr1 = operator_precedence(o1);
  opr2 = operator_precedence(o2);
  
  if( token_is_operator(token, &(tokptr)) )
    {
      while( ( strcmp(o2, ")") != 0 ) &&
	     ( (opr2 > opr1) || ((opr1 == opr2) && operator_left_assoc(o1)))
	     )
	{
	  printf("\nPop 1");
	  
	  op_stack_pop(&o2);
	  output_operator(o2);
	}
      
      op_stack_push(o1);
    }
  
  if( token_is_float(token) )
    {
      output_float(token);
      return;
    }

  if( token_is_integer(token) )
    {
      output_integer(token);
      return;
    }

  if( token_is_variable(token) )
    {
      output_variable(token);
      return;
    }

  if( token_is_string(token) )
    {
      output_string(token);
      return;
    }
  
  if( token_is_function(token, &tokptr) )
    {
      op_stack_push(tokptr);
      return;
    }

  if( strcmp(token, ",")==0 )
    {
      while( strcmp(op_stack_top(), ")") != 0 )
	{
	  printf("\nPop 2");
	  op_stack_pop(&o2);
	  output_operator(o2);
	}
    }

  if( strcmp(token, "(")==0 )
    {
      op_stack_push("(");
    }

  if( strcmp(token, ")")==0 )
    {
      while(  (strcmp(op_stack_top(), "(") != 0) && (strlen(op_stack_top())!=0) )
	{
	  printf("\nPop 3");
	  op_stack_pop(&o2);
	  output_operator(o2);
	}

      if( strlen(op_stack_top())==0 )
	{
	  // Mismatched parentheses
	  printf("\nMismatched parentheses");
	}

      printf("\nPop 4");
      op_stack_pop(&o2);

      if( strcmp(o2, "(") != 0 )
	{
	  printf("\n**** Should be left parenthesis");
	}

      if( token_is_function(op_stack_top(), &tokptr) )
	{
	  printf("\nPop 5");
	  op_stack_pop(&o2);
	  output_operator(o2);
	}
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

void process_expression(char *line)
{
  char token[NOPL_MAX_TOKEN+1];
  int i;
  int op_delimiter = 0;
  char cap_string[NOPL_MAX_TOKEN+1];
  int capture_string = 0;
  char ss[2];
  int end_cap_later = 0;
  
  if( strstr(line, "GLOBAL") != NULL )
    {
      printf("\n>>>%s", line);
      return;
    }

  if( strstr(line, "LOCAL") != NULL )
    {
      printf("\n>>>%s", line);
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

