#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>

#include "nopl.h"
#include "newopl.h"
#include "nopl_obj.h"
#include "newopl_exec.h"
#include "newopl_lib.h"

#include "qcode.h"

////////////////////////////////////////////////////////////////////////////////

void db_qcode(char *s, ...)
{
  va_list valist;

  va_start(valist, s);
  printf("\n   ");
  
  vprintf(s, valist);
  va_end(valist);

}

////////////////////////////////////////////////////////////////////////////////
//

void qcode_get_string_push_stack(NOBJ_MACHINE *m)
{
  int len;

  len =  m->stack[(m->rta_pc)++];
  push_machine_8(m, len);

  for(int i=0; i<len; i++)
    {
      push_machine_8(m, m->stack[(m->rta_pc)++]);  
    }

}


////////////////////////////////////////////////////////////////////////////////
//
//
//
////////////////////////////////////////////////////////////////////////////////

uint16_t qcode_next_16(NOBJ_MACHINE *m)
{
  uint16_t r;

  debug("\n%s: PC:%04X", __FUNCTION__, m->rta_pc);
  
  r  = (m->stack[(m->rta_pc)++]) << 8;
  r |= (m->stack[(m->rta_pc)++]);

  debug("\n%s: %04X", __FUNCTION__, r);
  
  return(r);
}

uint8_t qcode_next_8(NOBJ_MACHINE *m)
{
  uint8_t r;

  r  = (m->stack[(m->rta_pc)++]);

  debug("\n%s: %02X at %04X", __FUNCTION__, r, (m->rta_pc)-1);
  
  return(r);
}

////////////////////////////////////////////////////////////////////////////////

// QCode state
// Used to pass execution state of a single QCode

typedef struct _NOBJ_QCS
{
  NOBJ_QCODE qcode;
  NOBJ_INT   integer;
  uint16_t   ind_ptr;
  uint8_t    len;
  uint8_t    data8;
  char       str[NOBJ_FILENAME_MAXLEN];
  uint16_t   str_addr;
  uint16_t   addr;
  char       procpath[NOBJ_FILENAME_MAXLEN];
  uint8_t    max_sz;
  int        i;
  uint8_t    field_flag;
  int        done;
} NOBJ_QCS;

typedef void (*NOBJ_QC_ACTION)(NOBJ_MACHINE *m, NOBJ_QCS *s);

#define NOBJ_QC_NUM_ACTIONS 3

typedef struct
{
  NOBJ_QCODE      qcode;
  char            *name;
  NOBJ_QC_ACTION  action[NOBJ_QC_NUM_ACTIONS];
  
} NOBJ_QCODE_INFO;

////////////////////////////////////////////////////////////////////////////////

void qca_null(NOBJ_MACHINE *m, NOBJ_QCS *s)
{
}


void qca_str_ind_con(NOBJ_MACHINE *m, NOBJ_QCS *s)
{
  
  // Now stack the string
  s->len = stack_entry_8(m, (s->ind_ptr++) );

  int i;
  
  for(i=0; i<s->len; i++)
    {
      s->str[i] = stack_entry_8(m, s->ind_ptr++);
    }
  
  s->str[i] = '\0';
  
  push_machine_string(m, s->len, s->str);
}

void qca_int_qc_con(NOBJ_MACHINE *m, NOBJ_QCS *s)
{
  
  // Get int from qcodes and push onto stack
  push_machine_8(m, qcode_next_8(m));
  push_machine_8(m, qcode_next_8(m));
}

void qca_str_qc_con(NOBJ_MACHINE *m, NOBJ_QCS *s)
{
  
  // Get string from qcodes and push onto stack
  s->len = qcode_next_8(m);
  
  debug("\n  Len:%d", s->len);

  int i;
  
  for(i=0; i<s->len; i++)
    {
      s->str[i] = qcode_next_8(m);
    }
  
  s->str[i] = '\0';
  
  push_machine_string(m, s->len, s->str);
}

void qca_fp(NOBJ_MACHINE *m, NOBJ_QCS *s)
{
  // Get pointer to string
  s->ind_ptr = qcode_next_16(m);
  debug("\nind ptr:%04X", s->ind_ptr);
  
  // Add to FP
  s->ind_ptr += m->rta_fp;
  
  debug("\nIND Addr: %04X", s->ind_ptr);
}

void qca_ind(NOBJ_MACHINE *m, NOBJ_QCS *s)
{
  // Then that address has the address
  s->ind_ptr = stack_entry_16(m, s->ind_ptr);

}

void qca_str(NOBJ_MACHINE *m, NOBJ_QCS *s)
{
  // Get the maximum size
  s->max_sz = m->stack[s->ind_ptr-1];
  
  // Push string max length
  push_machine_8(m, s->max_sz);
  
  // Push address of string
  push_machine_16(m, s->ind_ptr);
  
  // Push field flag
  push_machine_8(m, 0);
}

void qca_push_int_addr(NOBJ_MACHINE *m, NOBJ_QCS *s)
{
  // Push address of integer
  push_machine_16(m, s->ind_ptr);
  
  // Push field flag
  push_machine_8(m, 0);
}

void qca_push_ind(NOBJ_MACHINE *m, NOBJ_QCS *s)
{
  // Push address we calculated
  push_machine_16(m, s->ind_ptr);
}

void qca_push_int(NOBJ_MACHINE *m, NOBJ_QCS *s)
{
  // Push integer at the address we calculated
  push_machine_16(m,  stack_entry_16(m, s->ind_ptr));
}

void qca_push_str(NOBJ_MACHINE *m, NOBJ_QCS *s)
{
  // Push string at the address we calculated
  push_machine_16(m,  stack_entry_16(m, s->ind_ptr));
}

void qca_push_proc(NOBJ_MACHINE *m, NOBJ_QCS *s)
{
  db_qcode("QCO_PROC");

  FILE *fp;
  
  // Get proc name
  s->len = qcode_next_8(m);

  debug("\n  Len:%d", s->len);

  int i;
  
  for(i=0; i<s->len; i++)
    {
      s->str[i] = qcode_next_8(m);
    }

  s->str[i] = '\0';
  strcpy(s->procpath, "A:");
  strcat(s->procpath, s->str);
	  
  strcat(s->str, ".OB3");

	  
  // We have the name, open the file
  debug("\nLoading PROC %s", s->str);
	  
  // Load the procedure file
  fp = fopen(s->str, "r");
	  
  if( fp == NULL )
    {
      debug("\nCannot open '%s'", s->str);
      exit(-1);
    }
	  
  debug("\nLoaded '%s'", s->str);
	  
  // Discard header
  read_ob3_header(fp);
	  
  // Initialise the machine
  //init_machine(&machine);
	  
  // Put some parameters on the stack for our test code
  //push_parameters(m);
	  
  // Push proc onto stack
  push_proc(fp, m, s->procpath, 0);

  fclose(fp);
}

void qca_push_qc_byte(NOBJ_MACHINE *m, NOBJ_QCS *s)
{
  s->data8 = qcode_next_8(m);
  push_machine_8(m, s->data8);
}

void qca_unwind_proc(NOBJ_MACHINE *m, NOBJ_QCS *s)
{
  // Dump stack for debug
  debug("\n==============================================Unwind============================");
  display_machine(m);
	  
  // Unwind the procedure (remove it from the stack), then
  // stack a zero floating point value
  uint16_t last_fp;

  last_fp = stack_entry_16(m, (m->rta_fp)+FP_OFF_NEXT_FP);

  if( last_fp == 0 )
    {
      // There is not a previous procedure top return to
      // Ignore PC and FP, set the stack to empty
      init_sp(m, 0x3F00);       // For full example 4

      debug("\nStack reset");
    }
  else
    {
      // There is a previous procedure to return to
	      
      // To unwind the stack, set SP to the previous frame base SP
      m->rta_sp = stack_entry_16(m, last_fp+FP_OFF_BASE_SP);
	      
      // Set PC to return PC address
      m->rta_pc = stack_entry_16(m, (m->rta_fp)+FP_OFF_RETURN_PC);
    }
	  
  // Set FP to last FP
  // If zero then this signals the end of the execution
	  
  m->rta_fp = last_fp;
}

void qca_ass_int(NOBJ_MACHINE *m, NOBJ_QCS *s)
{
  NOBJ_INT integer;
  
  // Drop int
  integer = pop_machine_int(m);

  // Check for field
  s->field_flag = pop_machine_8(m);
	  
  // Drop int address
  s->addr = pop_machine_16(m);
	  
  // Assign
  if( s->field_flag )
    {
    }
  else
    {
      // Assign integer to variable
      m->stack[s->addr+0] = integer >> 8;
      m->stack[s->addr+1] = integer  & 0xFF;
    }
}

void qca_ass_str(NOBJ_MACHINE *m, NOBJ_QCS *s)
{

  // Drop string
  pop_machine_string(m, &(s->len), s->str);

  // Check for field
  s->field_flag = pop_machine_8(m);
	  
  // Drop string reference
  s->str_addr = pop_machine_16(m);
  s->max_sz = pop_machine_8(m);
	  
  // Assign
  if( s->field_flag )
    {
    }
  else
    {
      // Assign string to variable
      // We are pointing at length byte of string
      // Check lengths
      if( s->len <= s->max_sz )
	{
	  // All OK
	  // Copy data

	  // We copy the length with the data
	  for(int i=0; i<s->len+1; i++)
	    {
	      m->stack[s->str_addr+i] = s->str[i];
	    }

	  // No need to zero the remaining space
		 
	}
      else
	{
	  debug("\n*** String too big. Len %d but variable max is %d", s->len, s->max_sz);
	}
    }
}


void qca_push_zero(NOBJ_MACHINE *m, NOBJ_QCS *s)
{
  // Now push a zero
  push_machine_8(m, 0);
  push_machine_8(m, 0);
  push_machine_8(m, 0);
  push_machine_8(m, 0);
  push_machine_8(m, 0);
  push_machine_8(m, 0);
  push_machine_8(m, 0);
  push_machine_8(m, 0);
}

void qca_push_null(NOBJ_MACHINE *m, NOBJ_QCS *s)
{
  // Now push a zero
  push_machine_8(m, 0);
  push_machine_8(m, 0);
}

void qca_push_nought(NOBJ_MACHINE *m, NOBJ_QCS *s)
{
  // Now push a nought
  push_machine_8(m, 0);
  push_machine_8(m, 0);
}

void qca_pop_num(NOBJ_MACHINE *m, NOBJ_QCS *s)
{
  pop_machine_8(m);
  pop_machine_8(m);
  pop_machine_8(m);
  pop_machine_8(m);
  pop_machine_8(m);
  pop_machine_8(m);
  pop_machine_8(m);
  pop_machine_8(m);
}

void qca_pop_int(NOBJ_MACHINE *m, NOBJ_QCS *s)
{
  s->integer = pop_machine_int(m);
}

void qca_pop_str(NOBJ_MACHINE *m, NOBJ_QCS *s)
{
  s->integer = pop_machine_int(m);
}

void qca_print_int(NOBJ_MACHINE *m, NOBJ_QCS *s)
{
  printf("%d", s->integer);
}

void qca_print_str(NOBJ_MACHINE *m, NOBJ_QCS *s)
{
  for(int i=0; i<s->len; i++)
    {
      printf("%c", s->str[i]);
    }
}

void qca_print_cr(NOBJ_MACHINE *m, NOBJ_QCS *s)
{
  printf("\n");
}


////////////////////////////////////////////////////////////////////////////////
//
//


NOBJ_QCODE_INFO qcode_info[] =
  {
    { QI_INT_SIM_FP,     "QI_INT_SIM_FP",     {qca_fp,           qca_null,        qca_push_int}},
    { QI_STR_SIM_FP,     "QI_STR_SIM_FP",     {qca_fp,           qca_null,        qca_str_ind_con}},
    { QI_STR_SIM_IND,    "QI_STR_SIM_IND",    {qca_fp,           qca_ind,         qca_str_ind_con}},
    { QI_LS_INT_SIM_FP,  "QI_LS_INT_SIM_FP",  {qca_fp,           qca_null,        qca_push_int_addr }},
    { QI_LS_STR_SIM_FP,  "QI_LS_STR_SIM_FP",  {qca_fp,           qca_null,        qca_str }},
    { QI_LS_STR_SIM_IND, "QI_LS_STR_SIM_IND", {qca_fp,           qca_ind,         qca_str }},
    { QI_STK_LIT_BYTE,   "QI_STK_LIT_BYTE",   {qca_null,         qca_null,        qca_push_qc_byte}},
    { QI_INT_CON,        "QI_INT_CON",        {qca_null,         qca_null,        qca_int_qc_con}},
    { QI_STR_CON,        "QI_STR_CON",        {qca_null,         qca_null,        qca_str_qc_con}},

    { QCO_PRINT_INT,     "QCO_PRINT_INT",     {qca_pop_int,      qca_print_int,   qca_null}},
    { QCO_PRINT_STR,     "QCO_PRINT_STR",     {qca_pop_str,      qca_print_str,   qca_null}},
    { QCO_PRINT_CR,      "QCO_PRINT_CR",      {qca_null,         qca_print_cr,    qca_null}},

    { QCO_RETURN,        "QCO_RETURN",        {qca_unwind_proc,  qca_null,        qca_null}},
    { QCO_RETURN_NOUGHT, "QCO_RETURN_NOUGHT", {qca_unwind_proc,  qca_push_nought, qca_null}},
    { QCO_RETURN_ZERO,   "QCO_RETURN_ZERO",   {qca_unwind_proc,  qca_push_zero,   qca_null}},
    { QCO_RETURN_NULL,   "QCO_RETURN_NULL",   {qca_unwind_proc,  qca_push_null,   qca_null}},
    { QCO_PROC,          "QCO_PROC",          {qca_push_proc,    qca_push_null,   qca_null}},
    { QCO_ASS_INT,       "QCO_ASS_INT",       {qca_ass_int,      qca_null,        qca_null}},
    { QCO_ASS_STR,       "QCO_ASS_STR",       {qca_ass_str,      qca_null,        qca_null}},
    { QCO_DROP_NUM,      "QCO_DROP_NUM",      {qca_pop_num,      qca_null,        qca_null}},
  };

#define SIZEOF_QCODE_INFO (sizeof(qcode_info)/sizeof(NOBJ_QCODE_INFO))

////////////////////////////////////////////////////////////////////////////////
//
// Display frame
//
////////////////////////////////////////////////////////////////////////////////


void display_frame(NOBJ_MACHINE *m)
{
  int fp = m->rta_fp;
  
  // rta_fp points to the frame pointer entry in the frame

  int retadd  = stack_entry_16(m, fp+6);
  int onerr   = stack_entry_16(m, fp+4);
  int basesp  = stack_entry_16(m, fp+2);
  int framep  = stack_entry_16(m, fp+0);

  printf("\nReturn PC: %04X", retadd);
  printf("\nONERR    : %04X", onerr);
  printf("\nBASE SP  : %04X", basesp);
  printf("\nFP       : %04X", framep);
  
  printf("\n");
}

////////////////////////////////////////////////////////////////////////////////
//
// Execute one QCode 
//
////////////////////////////////////////////////////////////////////////////////
//
// Single step mode aows execution to be stepped and information queried
// before execution of next instruction
//

int execute_qcode(NOBJ_MACHINE *m, int single_step)
{
  uint8_t    field_flag;
  NOBJ_QCS   s;
  int        found;
  char outline[250];
  
  s.done = 0;
  
  while(!s.done)
    {
      // Get the qcode using the PC from the stack
      
      s.qcode = m->stack[m->rta_pc];

      debug("\nExecuting QCode %02X at %04X", s.qcode, m->rta_pc);
      
      (m->rta_pc)++;

      
      found = 0;
      int qci = 0;
      
      for(int q=0; q<SIZEOF_QCODE_INFO; q++)
	{
	  if( s.qcode == qcode_info[q].qcode )
	    {
	      qci = q;
	      
	      found = 1;
	      break;
	    }
	}

      if( !found )
	{
	  debug("\nNot found so exit: %02X\n", s.qcode);
	  s.done = 1;
	}

      if( single_step )
	{
	  sprintf(outline, "rta_sp:%04X rta_fp:%04X rta_pc:%04X qcode:%02X %s",
		 m->rta_sp,
		 m->rta_fp,
		 m->rta_pc,
		 s.qcode,
		 qcode_info[qci].name
		 );

	  printf("\n%s", outline);

	  for(int i = 0; i<40-(strlen(outline) % 40); i++)
	    {
	      printf(" ");
	    }

	  printf(" %04X: ", m->rta_sp-16);
	  
	  for(int i=m->rta_sp-16; i<=m->rta_sp+16; i++)
	    {
	      if( i == m->rta_sp )
		{
		  printf("(%02X) ", m->stack[i]);
		}
	      else
		{
		  printf("%02X ", m->stack[i]);
		}
	    }
	  
	  printf(" :%04X ", m->rta_sp+16);
	  printf("\n");
	  
	  int done = 0;
	  char cmdline[250];
	  while(!done)
	    {
	      fgets(cmdline, 250, stdin);

	      if( cmdline[strlen(cmdline)-1] == '\n' )
		{
		  cmdline[strlen(cmdline)-1] = '\0';
		}
	      
	      if( strlen(cmdline)==0 )
		{
		  done = 1;
		}
	      
	      if( strcmp(cmdline, "cont") == 0 )
		{
		  done = 1;
		}

	      if( strcmp(cmdline, "f") == 0 )
		{
		  // Display frame
		  display_frame(m);
		}
	      
	      if( strcmp(cmdline, "s") == 0 )
		{
		  // Print the stack out, around rta_sp
		  printf("\nrta_sp:%04X", m->rta_sp);
		  
		  for(int i=m->rta_sp; i<=0x3F00; i++)
		    {
		      printf("\n%04X: %02X '%c'", i, m->stack[i], isprint(m->stack[i])?m->stack[i]:' ');
		    }
		  printf("\n");
		}
	      
	      if( sscanf(cmdline, "cont") == 1 )
		{
		  done = 1;
		}
	    }
	}

      debug("\n\n Executing %s\n", qcode_info[qci].name);
      
      // Perform actions
      for(int a=0; a<NOBJ_QC_NUM_ACTIONS; a++)
	{
	  qcode_info[qci].action[a](m, &s);
	}

      if ( m->rta_fp == 0 )
	{
	  debug("\n===  Exit ====");
	  display_machine(m);
	  exit(0);
	  // Exit?
	}
    }
}

////////////////////////////////////////////////////////////////////////////////
//
//  Top level execution of QCodes
//
//  Exits with exit QCode or invalid QCode
//
////////////////////////////////////////////////////////////////////////////////

void exec_loop(void)
{

}

