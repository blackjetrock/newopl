//
// Dumps an object file to stdout
//
//

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include "newopl.h"
#include "nopl_obj.h"

FILE *fp;

NOBJ_VAR_SPACE_SIZE      var_space_size;
NOBJ_QCODE_SPACE_SIZE    qcode_space_size;
NOBJ_NUM_PARAMETERS      num_parameters;
NOBJ_PARAMETER_TYPE      parameter_types[NOBJ_MAX_PARAMETERS];
NOBJ_GLOBAL_VARNAME_SIZE global_varname_size;

int read_item(void *ptr, int n, size_t size)
{
  int ni = fread(ptr, n, size, fp);
  
  if( feof(fp) || (ni == 0))
    {
      // No more file
      return(0);
    }
  
  return(1);
}

void pr_uint16(uint16_t n)
{
  int h = (n & 0xFF00) >> 8;
  int l = (n & 0x00FF);

  printf("%02X%02X", l, h);
}

void pr_uint8(uint8_t n)
{
  int l = (n & 0x00FF);

  printf("%02X", l);
}

void pr_var_space_size(NOBJ_VAR_SPACE_SIZE *x)
{
  printf("\nVar Space Size:");
  pr_uint16(x->size);
}

void pr_qcode_space_size(NOBJ_QCODE_SPACE_SIZE *x)
{
  printf("\nVar Space Size:");
  pr_uint16(x->size);
}

void pr_global_varname_size(NOBJ_GLOBAL_VARNAME_SIZE *x)
{
  printf("\nGlobal varname Size:");
  pr_uint16(x->size);
}

void pr_num_parameters(NOBJ_NUM_PARAMETERS *x)
{
  printf("\nNumber of parameters:");
  pr_uint8(x->num);
}

void pr_parameter_types(void)
{
  printf("\nParameter types:");
  for(int i=0; i<num_parameters.num; i++)
    {
      pr_uint8(parameter_types[i]);
      printf(" ");
    }
}


  
int main(int argc, char *argv[])
{
  fp = fopen(argv[1], "r");

  if( fp == NULL )
    {
      printf("\nCannot open '%s'", argv[1]);
      exit(-1);
    }

  // Read the object file

  if(!read_item((void *)&var_space_size, 1, sizeof(NOBJ_VAR_SPACE_SIZE)))
    {
      printf("\nError reading var space size.");
      return(0);
    }

  if(!read_item((void *)&qcode_space_size, 1, sizeof(NOBJ_QCODE_SPACE_SIZE)))
    {
      printf("\nError reading qcode space size.");
      return(0);
    }

  if(!read_item((void *)&num_parameters.num, 1, sizeof(NOBJ_NUM_PARAMETERS)))
    {
      printf("\nError reading number of parameters.");
      return(0);
    }

  if(!read_item((void *)&parameter_types, num_parameters.num, sizeof(NOBJ_PARAMETER_TYPE)))
    {
      printf("\nError reading parameter types.");
      return(0);
    }

  if(!read_item((void *)&global_varname_size, 1, sizeof(NOBJ_GLOBAL_VARNAME_SIZE)))
    {
      printf("\nError reading global varname size");
      return(0);
    }

  // Global varname is more complicated to read. Each entry is length
  // prefixed, so read them until the length we have read matches the
  // size we have just read.

  int length_read = 0;
  
do
    {
      uint8_t len;
      uint8_t varname[NOBJ_VARNAME_MAXLEN];
      NOBJ_VARTYPE vartype;
      NOBJ_VARADDR varaddr;
      vartype = 0;
      varaddr = 0;
      
      if(!read_item((void *)&len, 1, sizeof(len)))
	{
	  printf("\nError reading global varname entry length");
	  return(0);
	}

      memset(varname, 0, sizeof(varname));
      
      printf("\nVarname entry len=%d", len);
      
      length_read += sizeof(len);

      // 3 bytes on end are type and addr, so read rest of data into name field
      
      if(!read_item((void *)&varname, len, sizeof(uint8_t)))
	{
	  printf("\nError reading global varname entry name field");
	  return(0);
	}
      
      printf("\nvarname='%s'", varname);

      length_read += len;

      // read type and addr

      if(!read_item((void *)&vartype, 1, sizeof(NOBJ_VARTYPE)))
	{
	  printf("\nError reading global varname entry type field");
	  return(0);
	}

      if(!read_item((void *)&varaddr, 1, sizeof(NOBJ_VARADDR)))
	{
	  printf("\nError reading global varname entry addr field");
	  return(0);
	}

      printf("\nVarname type=%02X", vartype);
      printf("\nVaraddr addr=%02X", varaddr);
      length_read += 3;
    }
 while (length_read < global_varname_size.size );  
  fclose(fp);

  printf("\nDump of @%s'\n", argv[1]);

  pr_var_space_size(&var_space_size);
  pr_qcode_space_size(&qcode_space_size);
  pr_num_parameters(&num_parameters);
  pr_parameter_types();
  pr_global_varname_size(&global_varname_size);

  printf("\n");
    
}
