// NewOPL Executor
//
// As NewOPL uses the same bytecode as original OPL, it should be able to
// execute original OPL as well as new.

// Executes a byte code file.

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>

#include "newopl.h"
#include "nopl_obj.h"
#include "newopl_lib.h"

NOBJ_MACHINE machine;

void exec_qcode(NOBJ_MACHINE *m, NOBJ_QCODE qc)
{
}

void exec_proc(NOBJ_PROC *proc)
{
  NOBJ_QCODE *qc = proc->qcode;

  for(int i=0; i<proc->qcode_space_size.size; qc++, i++)
    {
      // Execute the QCode, applied to the machine state
      exec_qcode(&machine, *qc);
    }
}

FILE *fp;
NOBJ_PROC proc;

int main(int argc, char *argv[])
{

  // Load the procedure file
  fp = fopen(argv[1], "r");

  if( fp == NULL )
    {
      printf("\nCannot open '%s'", argv[1]);
      exit(-1);
    }

  printf("\nLoaded '%s'", argv[1]);

  // Read the object file
  read_proc_file(fp, &proc);

  // Execute the QCodes
  exec_proc(&proc);
  
  printf("\n");
}
