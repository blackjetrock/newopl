// NewOPL Translater

// Translates NewOPL to byte code.

// Processes files: reads file, translates it and writes bytecode file.

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

// 1 when line defines the procedure (i.e. the first line)
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

////////////////////////////////////////////////////////////////////////////////

void process_token(char *token)
{
  // Another token has arrived, process it using the shunting algorithm
  if( token_is_float(token) )
    {
    }

  if( token_is_integer(token) )
    {
    }

  printf("\n   T:'%s'", token);
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
      printf("\n%s", line);
      return;
    }

  if( strstr(line, "LOCAL") != NULL )
    {
      printf("\n%s", line);
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
	  printf("\nstr");
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
	  process_token("EOL");
	}
    }
}


int main(int argc, char *argv[])
{
  FILE *fp;
  FILE *ofp;
  
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
}

