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
#include "newopl_lib.h"

FILE *fp;

NOBJ_PROC                proc;

void pr_uint8(uint8_t n)
{
  int l = (n & 0x00FF);

  printf("%02X", l);
}

void pr_var_space_size(NOBJ_VAR_SPACE_SIZE *x)
{
  printf("\nVar Space Size:%04X", x->size);
}

void pr_qcode_space_size(NOBJ_QCODE_SPACE_SIZE *x)
{
  printf("\nVar Space Size:%04X", x->size);
}

void pr_global_varname_size(NOBJ_GLOBAL_VARNAME_SIZE *x)
{
  printf("\nGlobal varname Size:%04X", x->size);
}

void pr_external_varname_size(NOBJ_EXTERNAL_VARNAME_SIZE *x)
{
  printf("\nExternal varname Size:%04X", x->size);
}

void pr_num_parameters(NOBJ_NUM_PARAMETERS *x)
{
  printf("\nNumber of parameters:");
  pr_uint8(x->num);
}

void pr_parameter_types(void)
{
  printf("\nParameter types:");

  for(int i=0; i<proc.num_parameters.num; i++)
    {
      pr_uint8(proc.parameter_types[i]);
      printf(" ");
    }
}


void dump_proc(NOBJ_PROC *proc)
{
  pr_var_space_size(&(proc->var_space_size));
  pr_qcode_space_size(&(proc->qcode_space_size));
  pr_num_parameters(&(proc->num_parameters));
  pr_parameter_types();
  pr_global_varname_size(&proc->global_varname_size);

  printf("\nGlobal variables (%d)", proc->global_varname_num);
  for(int i=0; i<proc->global_varname_num; i++)
    {
      printf("\n%2d: %-16s %s (%02X) %04X",
	     i,
	     proc->global_varname[i].varname,
	     decode_vartype(proc->global_varname[i].type),
	     proc->global_varname[i].type,
	     proc->global_varname[i].address
	     );
    }
  printf("\n");

  pr_external_varname_size(&proc->external_varname_size);

  printf("\nExternal variables (%d)", proc->external_varname_num);
  for(int i=0; i<proc->external_varname_num; i++)
    {
      printf("\n%2d: %-16s %s (%02X)",
	     i,
	     proc->external_varname[i].varname,
	     decode_vartype(proc->external_varname[i].type),
	     proc->external_varname[i].type
	     );
    }
  printf("\n");

  printf("\nString length fixups (%d)", proc->strlen_fixup_num);
  
  for(int i=0; i<proc->strlen_fixup_num; i++)
    {
      printf("\n%2d: %04X %02X",
	     i,
	     proc->strlen_fixup[i].address,
	     proc->strlen_fixup[i].len
	     );
    }
  printf("\n");

  printf("\nArray size fixups (%d)", proc->arysz_fixup_num);
  
  for(int i=0; i<proc->arysz_fixup_num; i++)
    {
      printf("\n%2d: %04X %02X",
	     i,
	     proc->arysz_fixup[i].address,
	     proc->arysz_fixup[i].len
	     );
    }
  printf("\n");
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
  read_proc_file(fp, &proc);
  
  // Dump the proc information
  dump_proc(&proc);
  
 
 fclose(fp);
 
 
}
