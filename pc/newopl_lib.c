#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>

#include "newopl.h"
#include "nopl.h"
#include "nopl_obj.h"

uint16_t swap_uint16(uint16_t n)
{
  int h = (n & 0xFF00) >> 8;
  int l = (n & 0x00FF);

  return((l << 8) + h);
}

int read_item(FILE *fp, void *ptr, int n, size_t size)
{
  int ni = fread(ptr, n, size, fp);
  
  if( feof(fp) || (ni == 0))
    {
      // No more file
      return(0);
    }
  
  return(1);
}


#define READ_ITEM(FP,DEST,NUM,TYPE,ERROR)		\
  if(!read_item(FP, (void *)&(DEST), NUM, sizeof(TYPE)))	\
    { \
      printf(ERROR); \
      return; \
    }


void read_proc_file(FILE *fp, NOBJ_PROC *p)
{
  READ_ITEM(  fp, p->var_space_size, 1, NOBJ_VAR_SPACE_SIZE, "\nError reading var space size.");
  p->var_space_size.size = swap_uint16(p->var_space_size.size);

  READ_ITEM(fp, p->qcode_space_size,   1,                     NOBJ_QCODE_SPACE_SIZE, "\nError reading qcode space size.");
  READ_ITEM(fp, p->num_parameters.num, 1,                     NOBJ_NUM_PARAMETERS,   "\nError reading number of parameters.");
  READ_ITEM(fp, p->parameter_types,    p->num_parameters.num, NOBJ_PARAMETER_TYPE,   "\nError reading parameter types.");

  READ_ITEM( fp,  p->global_varname_size, 1, NOBJ_GLOBAL_VARNAME_SIZE, "\nError reading global varname size.");
  p->global_varname_size.size = swap_uint16(p->global_varname_size.size);
  
  //  printf("\nGlobal varname size:%d", p->global_varname_size.size);
  
  // Global varname is more complicated to read. Each entry is length
  // prefixed, so read them until the length we have read matches the
  // size we have just read.

  int length_read = 0;
  int num_global_vars = 0;
  
do
    {
      uint8_t len;
      uint8_t varname[NOBJ_VARNAME_MAXLEN];
      NOBJ_VARTYPE vartype;
      NOBJ_VARADDR varaddr;
      vartype = 0;
      varaddr = 0;
      
      if(!read_item(fp, (void *)&len, 1, sizeof(len)))
	{
	  printf("\nError reading global varname entry length");
	  return;
	}

      memset(varname, 0, sizeof(varname));
      
      //      printf("\nVarname entry len=%d", len);
      
      length_read += sizeof(len);

      // 3 bytes on end are type and addr, so read rest of data into name field
      
      if(!read_item(fp, (void *)&varname, len, sizeof(uint8_t)))
	{
	  printf("\nError reading global varname entry name field");
	  return;
	}
      
      //printf("\nvarname='%s'", varname);

      length_read += len;

      // read type and addr

      if(!read_item(fp, (void *)&vartype, 1, sizeof(NOBJ_VARTYPE)))
	{
	  printf("\nError reading global varname entry type field");
	  return;
	}

      if(!read_item(fp, (void *)&varaddr, 1, sizeof(NOBJ_VARADDR)))
	{
	  printf("\nError reading global varname entry addr field");
	  return;
	}

      //printf("\nVarname type=%02X", vartype);
      //printf("\nVaraddr addr=%02X", varaddr);
      length_read += 3;

      //printf("\nLength read:%d out of %d", length_read, p->global_varname_size.size);

      // Add variable to list of globals
      p->global_varname[num_global_vars].type    = vartype;
      p->global_varname[num_global_vars].address = varaddr;
      strcpy(p->global_varname[num_global_vars].varname, varname);
      
      num_global_vars++;      
    }

 while (length_read < p->global_varname_size.size );

 p->global_varname_num = num_global_vars;
}
