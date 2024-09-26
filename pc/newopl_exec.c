// NewOPL Executor
//
// As NewOPL uses the same bytecode as original OPL, it should be able to
// execute original OPL as well as new.
//
// Executes a byte code file.
//
//
//  6.2  MEMORY MAP
//  
//  
//  
//                  $FFFF   I-------------------------------I
//                          I          System ROM           I
//                  $8000   I-------------------------------I
//                          I    Not Used (except LA/OS)    I
//    $8000  $6000  $4000   I-------------------------------I
//    LA/OS  XP/OS  CM/OS   I       Processor Stack         I
//    $7F00  $5F00  $3F00   I-------------------------------I
//                          I        Language Stack         I
//                          I         (grows down)          I
//                          I                               I
//                          I          (grows up)           I
//                          I       Allocated Cells         I
//                          I...............................I
//                          I       System Variables        I
//                  $2000   I-------------------------------I
//                          I     Not Used (except LA/OS)   I
//                  $0400   I-------------------------------I
//                          I       Hardware Addresses      I
//                  $0100   I-------------------------------I
//                          I  Transient Application Area   I
//                  $00E0   I-------------------------------I
//                          I        System Variables       I
//                  $0040   I-------------------------------I
//                          I           Not used            I
//                  $0020   I-------------------------------I
//                          I       Internal Registers      I
//                  $0000   I-------------------------------I
//  
//  
//  17.2.13  LANGUAGE POINTERS
//  
//  
//  There are three key pointers used by the language:
//  
//                  RTA_SP          Language stack pointer
//                  RTA_PC          Program counter
//                  RTA_FP          Frame (procedure) pointer
//  
//  RTA_SP points at the lowest byte  of  the  stack.   So  if  an  integer  is
//  stacked,  RTA_SP  is  decremented by 2 and the word is saved at the address
//  pointed to by RTA_SP.
//  
//  RTA_PC points at the current operand/operator executed and  is  incremented
//  after  execution - except at the start of a procedure or a GOTO when RTA_PC
//  is set up appropriately.
//  
//  RTA_FP points into the header of the current procedure.
//  
//  Each procedure header has the form:
//  
//                          Device (zero if top procedure)
//                          Return RTA_PC
//                          ONERR address
//                          BASE_SP
//  RTA_FP points at:       Previous RTA_FP
//                          Start address of the global name table
//                          Global name table
//                          Indirection table for externals/parameters
//  
//  This is followed by the variables, and finally by the Q code.
//  
//  RTA_FP points at the previous RTA_FP, so it is easy to jump up through  all
//  the  procedures  above.   The  language  uses  this when resolving external
//  references and when handling errors.
//
//
  
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>

#include "newopl.h"
#include "nopl_obj.h"
#include "newopl_exec.h"
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

#if 0
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
#endif

#if 0
  // Push two floats on to the stack

#endif

#if 1
  push_machine_8(m, 'T');
  push_machine_8(m, 'S');
  push_machine_8(m, 'R');
  push_machine_8(m, 0x03);
  push_machine_8(m, 0x02);
  push_machine_8(m, 0x01);
  
#endif
  
  printf("\nPush parameters done.");

}


void print_machine_state(NOBJ_MACHINE *m)
{
  printf("\n  SP:%04X",              m->rta_sp);
  printf("\n  FP:%04X",              m->rta_fp);
  printf("\n  PC:%04X",              m->rta_pc);
  printf("\n");
}

////////////////////////////////////////////////////////////////////////////////
//
//
// Pushes a procedure onto the stack of a machine
//
// The procedure is in the form of an OB3 file, with file header
// and then QCode
//
//
////////////////////////////////////////////////////////////////////////////////

void push_proc(FILE *fp, NOBJ_MACHINE *m, char *name, int top)
{
  // We build a header for the frame
  int      base_sp           = 0;
  int      previous_fp       = m->rta_fp;
  char     device_ch         = *name;
  uint16_t size_of_variables = 0;
  uint16_t size_of_qcode     = 0;
  uint16_t size_of_global_table     = 0;
  uint16_t size_of_external_table     = 0;
  uint8_t  num_parameters;

  printf("\n\n");
  printf("\nPushing procedure '%s'", name);
  printf("\n  Top:%d",               top);
  printf("\n  onto machine:");

  print_machine_state(m);
  
  // Get sizes from file
  if( !read_item_16(fp, &size_of_variables) )
    {
      // Error
    }

  // Work out the base SP value
  // This contains data we set up:
  //     the global table
  //     the indirection table
  //
  // and data not in the OB3 file:
  //
  //     space for the variables
  
  base_sp += m->rta_sp - size_of_variables - 11 - 7;
  
  if( !read_item_16(fp, &size_of_qcode) )
    {
      // Error
    }

  if( !read_item(fp, &num_parameters, 1, sizeof(uint8_t)) )
    {
      // Error
    }

  printf("\nSize of variables    :%02X %d", size_of_variables, size_of_variables);
  printf("\nSize of QCode        :%02X %d", size_of_qcode, size_of_qcode);
  printf("\nNumber of parameters :%02X %d", num_parameters, num_parameters);

#if 0
  // Proc name
  char *np = name;
  
  while( *np != '\0' )
    {
      push_machine_8(m, *np);
      np++;
    }

  push_machine_8(m, (uint8_t) strlen(name));

  // Read parameter types and push them
  uint8_t par_type;
  
  for(int np = 0; np<num_parameters; np++)
    {
      if( !read_item(fp, &par_type, 1, sizeof(uint8_t)) )
	{
	  // Error
	}
      
      push_machine_8(m, par_type);
    }
  
  // Num parameters
  push_machine_8(m, num_parameters);

#endif

#if 1
  uint8_t par_type;
  uint8_t par_types[16];
  
  for(int np = 0; np<num_parameters; np++)
    {
      if( !read_item(fp, &par_type, 1, sizeof(uint8_t)) )
	{
	  // Error
	}
      
      // Store parameter types
      par_types[np] = par_type;
    }

#endif
  
  // Device
  if( top )
    {
      push_machine_8(m, 0x00);
    }
  else
    {
      push_machine_8(m, device_ch);
    }
  
  //  Return PC
  push_machine_16(m, m->rta_pc);

  // ONERR address
  push_machine_16(m, 0x0000);

  // Base SP. Used as SP if this is an ONERR procedure
  push_machine_16(m, base_sp);

  // FP points at the frame we are pushing
  m->rta_fp = m->rta_sp-1;

  printf("\nrta_fp = %04X", m->rta_fp);
  
  // Previous FP
  push_machine_16(m, previous_fp);

  // Get size of global table from file
  if( !read_item_16(fp, &size_of_global_table) )
    {
      // Error
    }
  
  printf("\nSize of global table:%02X %d", size_of_global_table, size_of_global_table);

  uint16_t start_of_global_table = m->rta_sp-size_of_global_table-1;
  
  push_machine_16(m, start_of_global_table);
  
  // Push global area on to stack Fill global area on stack (not push)

  for(int i=0; i<size_of_global_table;)
    {
      int start_i = i;
      int k = i;
      
      // Read variable data from file
      // Build up the record in the stack
      
      uint8_t gv_name_len;
      if( !read_item(fp, &gv_name_len, 1, sizeof(uint8_t)) )
	{
	  // Error
	}

      m->stack[k++] = gv_name_len;

      printf("\n  Name length:%d", gv_name_len);

      printf("  '");
      for(int j=0; j<gv_name_len; j++)
	{
	  uint8_t gv_name_char;
	  if( !read_item(fp, &gv_name_char, 1, sizeof(uint8_t)) )
	    {
	      // Error
	    }

	  m->stack[k++] = gv_name_char;
	  printf("%c", gv_name_char);
	}

      printf("' ");
      
      //      k += gv_name_len;

      uint8_t gv_type;
      if( !read_item(fp, &gv_type, 1, sizeof(uint8_t)) )
	{
	  // Error
	}

      m->stack[k++] = gv_name_len;

      printf(" Type:%d", gv_type);
      
      uint16_t gv_addr;
      uint16_t gv_eff_addr;
      
      if( !read_item_16(fp, &gv_addr) )
	{
	  // Error
	}

      gv_eff_addr = (uint16_t)m->rta_fp + gv_addr;

      printf("\nEff %04X = %04X - %04X", (int)gv_eff_addr, (int)gv_addr, (int)base_sp);
      printf("  ADDR:%04X -> %04X", gv_addr, gv_eff_addr);

      m->stack[k++] = gv_eff_addr >> 8;
      m->stack[k++] = gv_eff_addr & 0xFF;

      for(int ii=start_i; ii<start_i+4+gv_name_len; ii++)
	{
	  printf("\n%04X: %02X", ii, m->stack[ii]);
	}

      i = k;
    }

  // Move SP on by size of global table
  m->rta_sp -= size_of_global_table;
  
  // Get size of external table from file
  if( !read_item_16(fp, &size_of_external_table) )
    {
      // Error
    }
  
  printf("\nSize of external table:%02X %d", size_of_external_table, size_of_external_table);

  // Push external area on to stack 

  for(int i=0; i<size_of_external_table; i++)
    {
      uint8_t ev;
      if( !read_item(fp, &ev, 1, sizeof(uint8_t)) )
	{
	  // Error
	}
      
      push_machine_8(m, ev);
    }

  // 

  // Indirection table
  push_machine_16(m, previous_fp);

  // Variables go here

  // QCode goes here
  
}


////////////////////////////////////////////////////////////////////////////////
//
// Displays the contents of a machine 
//
////////////////////////////////////////////////////////////////////////////////

// Displays from the last procedure pushed on the stack

void display_machine_procs(NOBJ_MACHINE *m)
{
  uint16_t fp = m->rta_fp;
  int fp_next;
  
  int sp = m->rta_sp;
  uint16_t dp;
  uint16_t val;
  uint8_t val8;

  printf("\n\nProcedures:");

  while(fp != 0 )
    {
      print_machine_state(m);
      
      // Get procedure name
      
      // Get data about this proc
      dp = get_machine_16(m, fp-8, &val);
      printf("\n%04X:  Indirection table: %04X", dp, val);

      dp = get_machine_16(m, dp, &val);
      printf("\n%04X:  Global Name table: %04X", dp, val);

      dp = get_machine_16(m, dp, &val);
      printf("\n%04X:  Start address of global name table: %04X", dp, val);

      dp = get_machine_16(m, dp, &val);
      printf("\n%04X:  Previous FP: %02X", dp, val);

      fp = val;

      dp = get_machine_16(m, dp, &val);
      printf("\n%04X:  Base SP: %04X", dp, val);

      dp = get_machine_16(m, dp, &val);
      printf("\n%04X:  ONERR Address: %04X", dp, val);

      dp = get_machine_16(m, dp, &val);
      printf("\n%04X:  Return PC: %04X", dp, val);

      dp = get_machine_8(m, dp, &val8);
      printf("\n%04X:  Device: %02X", dp, val8);

      // Move to next procedure
      //get_machine_16(m, fp, &fp);

    }
}

void display_machine(NOBJ_MACHINE *m)
{
  display_machine_procs(m);

  // Dump the stack
  for(int i= NOBJ_MACHINE_STACK_SIZE; i>=m->rta_sp; i--)
    {
      printf("\n%04X: %02X", i, m->stack[i]);
    }
  
  printf("\n");
}

////////////////////////////////////////////////////////////////////////////////
//
//
//
//
////////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[])
{

  printf("\nSize of NOBJ_PROC:%ld", sizeof(NOBJ_PROC));
  
  // Load the procedure file
  fp = fopen(argv[1], "r");

  if( fp == NULL )
    {
      printf("\nCannot open '%s'", argv[1]);
      exit(-1);
    }

  printf("\nLoaded '%s'", argv[1]);

  // Discard header
  read_ob3_header(fp);
  
  // Initialise the machine
  init_machine(&machine);

  // Put some parameters on the stack for our test code
  push_parameters(&machine);

#if 0
  // Reading the proc into data structures and then pushing it on the stack isn't
  // going to work too well with limited RAM.
  
  // Read the object file
  read_proc_file(fp, &proc);

  // Push it it on the stack
  push_proc_on_stack(&proc, &machine);
#endif

  // Push proc onto stack
  push_proc(fp, &machine, "A:EX4", 1);

  printf("\n\n");
  
  display_machine(&machine);
  
  // Execute the QCodes
  exec_proc(&proc);
  
  printf("\n");
}
