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

#define NUM_OPERATORS (sizeof(op_info)/sizeof(struct _OP_INFO))

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
  if( fgets(cline, MAX_NOPL_LINE, fp) == NULL )
    {
      cline_i = 0;
      return(0);
    }
  
  return(1);
}

////////////////////////////////////////////////////////////////////////////////

void drop_space()
{
  while( isspace(cline[cline_i]) && (cline[cline_i] != '\0') )
    {
      cline_i++;
    }

  if( cline_i > 0 )
    {
      cline_i--;
    }
}

////////////////////////////////////////////////////////////////////////////////

// Scan for a literal
// Leading space means drop spaces before looking for the literal,
// trailing mena sdrop spaces after finding it

int scan_literal(char *lit)
{
  char *origlit = lit;
  
  if( *lit == ' ' )
    {
      lit++;
      drop_space();
    }

  while( (*lit != '\0') && (*lit != ' ') )
    {
      if( cline[cline_i] == '\0' )
	{
	  syntax_error("Bad literal '%s'", origlit);
	  return(0);
	}
      
      if( *lit == cline[cline_i++] )
	{
	  // Not a match, fail
	  return(0);
	}
    }
  
  if( *lit == ' ' )
    {
      lit++;
      drop_space();
    }
  
  // reached end of literal string , all ok
  return(1);
}

// Check for a literal string.
// Leading space means drop spaces before looking for the literal,
// trailing mena sdrop spaces after finding it

int check_literal(char *lit)
{
  int save_cli = cline_i;

  if( *lit == ' ' )
    {
      lit++;
      drop_space();
    }
  
  while( (*lit != '\0') && (cline[cline_i] != '\0'))
    {
      if( *lit == cline[cline_i++] )
	{
	  // Not a match, fail
	  cline_i = save_cli;
	  return(0);
	}
    }

  if( cline[cline_i-1] == '\0' )
    {
      cline_i = save_cli;
      return(0);
    }
  
  // reached end of literal string , all ok
  cline_i = save_cli;
  return(1);

}



// Scans for a variable name string part
int scan_vname(char *vname_dest)
{
  char vname[300];
  int vname_i = 0;
  char ch;
  
  if( isalpha(ch = cline[cline_i++]) )
    {
      vname[vname_i++] = ch;
      
      while( isalnum(ch = cline[cline_i++]) )
	{
	vname[vname_i++] = ch;
	}

      strcpy(vname_dest, vname);
      return(1);
    }

  strcpy(vname_dest, "");
  return(0);
}

// Checks for a variable name string part

int check_vname(void)
{
  int save_cli = cline_i;

  if( isalpha(cline[cline_i++]) )
    {
      while( isalnum(cline[cline_i++]) )
	{
	}
      
      cline_i = save_cli;
      return(1);
    }

  cline_i = save_cli;
  return(0);
}

// Scan variable reference
// This puts a variable ref in the output stream. handles arrays
// and puts appropriate codes for array indices in stream

int scan_variable(char *variable_dest)
{
  char vname[300];
  char chstr[2];
  int var_is_string  = 0;
  int var_is_integer = 0;
  int var_is_float   = 0;
  int var_is_array   = 0;
  
  chstr[1] = '\0';
  
  if( scan_vname(vname) )
    {
      // Could just be a vname
      switch( chstr[0] = cline[cline_i] )
	{
	case '%':
	  var_is_integer = 1;
	  strcat(vname, chstr);
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
      if( check_literal(" ( ") )
	{
	  var_is_array = 1;
	  
	  // Add token to output stream for index or indices
	  scan_expression();

	  // Could be string array, which has two expressions in
	  // the brackets
	  if( check_literal(" , ") )
	    {
	      if( var_is_string )
		{
		  // All OK, string array
		  scan_literal(" , ");
		  scan_expression();
		}
	      else
		{
		  syntax_error("Two indices in non-string variable name");
		}
	    }

	  if( scan_literal(" ) ") )
	    {
	      return(1);
	    }
	  
	}
      return(1);
      
    }
  
  return(0);
}

int check_variable(void)
{
  if( check_vname() )
    {
      return(1);
    }
  return(0);
}

int check_operator(void)
{
  for(int i=0; i<NUM_OPERATORS; i++)
    {
      if( strncmp(&(cline[cline_i]), op_info[i].name, strlen(op_info[i].name)) == 0 )
	{
	  // Match
	  return(1);
	}
    }

  return(0);
}

int scan_operator(void)
{
  for(int i=0; i<NUM_OPERATORS; i++)
    {
      if( strncmp(&(cline[cline_i]), op_info[i].name, strlen(op_info[i].name)) == 0 )
	{
	  // Match
	  cline_i += strlen(op_info[i].name);
	  return(1);
	}
    }
}

int check_integer(void)
{
  if( isdigit(cline[cline_i]) )
    {
      return(1);
    }
  
  return(0);
}

int scan_integer(char *intdest)
{
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
  if( isdigit(c) || (c == '.') )
    {
      return(1);
    }
  
  return(0);
}

int check_float(void)
{
  if( isfloatdigit(cline[cline_i]) )
    {
      return(1);
    }
  
  return(0);
}

int scan_float(char *fltdest)
{
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
  if( check_literal(" ( ") )
    {
      return(1);
    }

  return(0);
}

int scan_sub_expr(void)
{
  if( scan_literal(" ( ") )
    {
      if( scan_expression() )
	{
	  if( scan_literal(" ) ") )
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
  if( check_literal(" \"") )
    {
      // String
      return(1);
    }

  if( check_number() )
    {
      // Int or float
      return(1);
    }

  if( check_variable() )
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

  strval[0] = '\0';
  chstr[1] = '\0';
  
  if( scan_literal(" \"") )
    {
      while(((chstr[0] = cline[cline_i]) != '"') && (cline[cline_i] != '\0') )
	{
	  strcat(strval, chstr);
	  cline_i++;
	}

      if( cline[cline_i-1] == '"' )
	{
	  return(1);
	}
    }

  syntax_error("Bad string");
  return(0);
}

int scan_atom(void)
{
  char vname[300];
  
  if( check_literal(" \"") )
    {
      // String
      return(scan_string());
    }

  if( check_number() )
    {
      // Int or float
      return(scan_number());
    }

  if( check_variable() )
    {
      // Variable
      return(scan_variable(vname));
    }

  syntax_error("Not an atom");  
  return(0);
}

int check_eitem(void)
{
  if( check_operator() ||
      check_function() ||
      check_atom() ||
      check_sub_expr() )
    {
      return(1);
    }

  return(0);
}

int scan_eitem(void)
{
  char fnval[40];
  
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
  if( check_eitem() )
    {
      return(1);
    }
  
  return(0);
}

int scan_expression(void)
{
  while( check_eitem() )
    {
      if( scan_eitem() )
	{
	  // All OK
	}
      else
	{
	  syntax_error("Expression error");
	  return(0);
	}
    }
  return(1);
}


int check_command(void)
{
  for(int i=0; i<NUM_FUNCTIONS; i++)
    {
      if( fn_info[i].command && strncmp(&(cline[cline_i]), fn_info[i].name, strlen(fn_info[i].name)) == 0 )
	{
	  // Match
	  return(1);
	}
    }
  
  return(0);
}

int scan_command(char *cmd_dest)
{
  for(int i=0; i<NUM_FUNCTIONS; i++)
    {
      if( fn_info[i].command && (strncmp(&(cline[cline_i]), fn_info[i].name, strlen(fn_info[i].name)) == 0) )
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
  int save_cli = cline_i;
  
  if( check_variable() )
    {
      if( check_literal(" = ") )
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
  
  if( scan_variable(vname) )
    {
      if( scan_literal(" = ") )
	{
	  
	}
    }

  syntax_error("Assignment error");
  return(0);
}

int check_line(void)
{
  if( check_assignment()   |
      check_command()      |
      check_literal(" LOCAL ") |
      check_literal(" GLOBAL ") |
      check_literal(" IF ") |
      check_literal(" ELSE ") |
      check_literal(" ENDIF ") |
      check_literal(" DO ") |
      check_literal(" WHILE ") |
      check_literal(" REPEAT ") |
      check_literal(" UNTIL ") )
    {
      return(1);
    }
  
  return(0);
}

int scan_line()
{
  char cmdname[300];
  
  if( check_assignment() )
    {
      return(scan_assignment());
    }
  
  if( check_command() )
    {
      scan_command(cmdname);
    }
  
  if( check_literal(" LOCAL ") )
    {
      scan_literal(" LOCAL ");
    }
  
  if( check_literal(" GLOBAL ") )
    {
      scan_literal(" GLOBAL ");
    }
  
  if( check_literal(" IF ") )
    {
      scan_literal(" IF ");
    }
  
  if( check_literal(" ELSE ") )
    {
      scan_literal(" ELSE ");
    }
  
  if( check_literal(" ENDIF ") )
    {
      scan_literal(" ENDIF ");
    }
  
  if( check_literal(" DO ") )
    {
      scan_literal(" DO ");
    }
  
  if( check_literal(" WHILE ") )
    {
      scan_literal(" WHILE ");
    }
  
  if( check_literal(" REPEAT ") )
    {
      scan_literal(" REPEAT ");
    }
  
  if( check_literal(" UNTIL ") )
    { 
      scan_literal(" UNTIL ");
    }
  
  return(0);    
}


////////////////////////////////////////////////////////////////////////////////

int scan_cline()
{
  int ret = 0;
  
  drop_space();
  
  while( check_line() )
    {
      if( scan_line() )
      {
	return(0);
      }
      
      drop_space();
      if ( !scan_literal(":") )
	{
	  return(0);
	}
      drop_space();
    }

  return(1);
}

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[])
{
  char *line = argv[1];
  char varname[200];


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

      // Recursive decent parse
      if( scan_cline(fp) )
	{
	  printf("\ncline scanned OK");
	  
	}
      else
	{
	  printf("\ncline filed scan");
	}

    }

  printf("\n");
}
