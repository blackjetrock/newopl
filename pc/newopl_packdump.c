////////////////////////////////////////////////////////////////////////////////
//
//
// Dumps a datapack file
//
// Currently dumps Linux newopl file
//
////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <string.h>


// Process command line
//
// One flag: -c : Check pack is OK, returns 0 or 1
//                and PASS or FAIL on stdout
//

int check_pack = 0;
char filename[200];
int pass = 0;
FILE *fp;

int main(int argc, char *argv[])
{
  for(int a=1; a<argc; a++)
    {
      if( strcmp(argv[a], "-c")==0 )
	{
	  check_pack = 1;
	  printf("\nChecking pack");
	}

      if( a == (argc-1) )
	{
	  strcpy(filename, argv[a]);
	  printf("\nProcessing file '%s'", filename);
	}
    }

  //------------------------------------------------------------------------------
  //
  // Open the file
  //
  fp = fopen(filename, "r");

  if( fp == NULL )
    {
      printf("\nCannot open '%s'", filename);
      printf("\nFAIL\n");
      return(0);
    }
  
  //------------------------------------------------------------------------------
  //
  // Read and dump the file
  //
  // Header
  //
  
  
  printf("\n");
  if( pass )
    {
      printf("\nPASS");
    }
  else
    {
      printf("\nFAIL");
    }
  
    printf("\n");
}






