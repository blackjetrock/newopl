#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#define MAX_NOPL_LINE 300

FILE *fp;

// Tokenise OPL
// one argument: an OPL file

// reads next composiute line into buffer
char cline[MAX_NOPL_LINE];
int cline_i = 0;

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
    }

  if( cline_i > 0 )
    {
      cline_i--;
    }
}

////////////////////////////////////////////////////////////////////////////////

int scan_literal(char *lit)
{
  
  while( *lit != '\0' )
    {
      if( *lit == cline[cline_i++] )
	{
	  // Not a match, fail
	  return(0);
	}
    }

  // reached end of literal string , all ok
  return(1);
}

int check_literal(char *lit)
{
  int save_cli = cline_i;
  
  while( *lit != '\0' )
    {
      if( *lit == cline[cline_i++] )
	{
	  // Not a match, fail
	  cline_i = save_cli;
	  return(0);
	}
    }

  // reached end of literal string , all ok
  cline_i = save_cli;
  return(1);

}

int check_assignment()
{

}

int check_line()
{
  if( check_assignment(fp)   |
      check_command(fp)      |
      check_literal("LOCAL") |
      check_literal("GLOBAL") |
      check_literal("IF") |
      check_literal("ELSE") |
      check_literal("ENDIF") |
      check_literal("DO") |
      check_literal("WHILE") |
      check_literal("REPEAT") |
      check_literal("UNTIL") )
    {
      return(1);
    }
  
  return(0);
}

int scan_line()
{
  if( check_assignment(fp) )
    {
      return(scan_assignment(fp));
    }
  
  if( check_command(fp) )
    {
      scan_command(fp);
    }
  
  if( check_literal("LOCAL") )
    {
      scan_literal("LOCAL");
    }
  
  if( check_literal("GLOBAL") )
    {
      scan_literal("GLOBAL");
    }
  
  if( check_literal("IF") )
    {
      scan_literal("IF");
    }
  
  if( check_literal("ELSE") )
    {
      scan_literal("ELSE");
    }
  
  if( check_literal("ENDIF") )
    {
      scan_literal("ENDIF");
    }
  
  if( check_literal("DO") )
    {
      scan_literal("DO");
    }
  
  if( check_literal("WHILE") )
    {
      scan_literal("WHILE");
    }
  
  if( check_literal("REPEAT") )
    {
      scan_literal("REPEAT");
    }
  
  if( check_literal("UNTIL") )
    { 
      scan_literal("UNTIL");
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
