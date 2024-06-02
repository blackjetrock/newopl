// NewOPL Executor
//
// As NewOPL uses the same bytecode as original OPL, it should be able to
// execute original OPL as well as new.
//
// Executes a byte code file.
//

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

void push_parameters(NOBJ_MACHINE *m)
{
  printf("\nPush parameters...");
  
  // parameters for the code in xx0.bin
  push_machine_8(m, 'C');
  push_machine_8(m, 'B');
  push_machine_8(m, 'A');
  push_machine_8(m, 0x03);
  push_machine_8(m, 0x02);

  push_machine_8(m, 0x12);
  push_machine_8(m, 0x00);
  push_machine_8(m, 0x00);

  push_machine_8(m, 0x00);
  push_machine_8(m, 0x01);
  push_machine_8(m, 0x17);
  push_machine_8(m, 0x50);
  push_machine_8(m, 0x00);
  push_machine_8(m, 0x00);
  push_machine_8(m, 0x00);
  push_machine_8(m, 0x00);
  push_machine_8(m, 0x01);

  // Number of parameters
  push_machine_8(m, 0x03);

  printf("\nPush pareameters done.");

}

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

  // Initialise the machine
  init_machine(&machine);

  // Put some parameters on the stack for our test code
  push_parameters(&machine);
  
  // Read the object file
  read_proc_file(fp, &proc);

  // Push it it on the stack
  push_proc_on_stack(&proc, &machine);

  // Execute the QCodes
  exec_proc(&proc);
  
  printf("\n");
}
