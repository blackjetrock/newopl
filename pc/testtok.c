#include <stdio.h>
#include <ctype.h>

int main(int argc, char *argv[])
{
  char *line = argv[1];
  char varname[200];
  
  printf("\nInput:'%s'", argv[1]);

  // split into space delimited chunks
  while( isspace(*line) )
    {
      line++;
    }

  // See if we have a variable
  if( sscanf(line, "%[A-Za-z0-9%$()]", &varname) )
    {
      printf("\nSTR/INT var: %s", varname); 
    }
  printf("\n");
}
