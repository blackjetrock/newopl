#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>

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
  
  printf("\n  Len:%d", s->len);

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
  printf("\nind ptr:%04X", s->ind_ptr);
  
  // Add to FP
  s->ind_ptr += m->rta_fp;
  
  printf("\nIND Addr: %04X", s->ind_ptr);
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

void qca_int(NOBJ_MACHINE *m, NOBJ_QCS *s)
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

void qca_push_proc(NOBJ_MACHINE *m, NOBJ_QCS *s)
{
  db_qcode("QCO_PROC");

  FILE *fp;
  
  // Get proc name
  s->len = qcode_next_8(m);

  printf("\n  Len:%d", s->len);

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
      printf("\nCannot open '%s'", s->str);
      exit(-1);
    }
	  
  printf("\nLoaded '%s'", s->str);
	  
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
  printf("\n==============================================Unwind============================");
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

      printf("\nStack reset");
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
	  printf("\n*** String too big. Len %d but variable max is %d", s->len, s->max_sz);
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

////////////////////////////////////////////////////////////////////////////////
//
//


NOBJ_QCODE_INFO qcode_info[] =
  {
    { 0x00, "QI_INT_SIM_FP",     {qca_fp,           qca_null,        qca_push_ind}},
    { 0x09, "QI_STR_SIM_IND",    {qca_fp,           qca_ind,         qca_str_ind_con}},
    { 0x0D, "QI_LS_INT_SIM_FP",  {qca_fp,           qca_null,        qca_int }},
    { 0x0F, "QI_LS_STR_SIM_FP",  {qca_fp,           qca_null,        qca_str }},
    { 0x16, "QI_LS_STR_SIM_IND", {qca_fp,           qca_ind,         qca_str }},
    { 0x20, "QI_STK_LIT_BYTE",   {qca_null,         qca_null,        qca_push_qc_byte}},
    { 0x22, "QI_INT_CON",        {qca_null,         qca_null,        qca_int_qc_con}},
    { 0x24, "QI_STR_CON",        {qca_null,         qca_null,        qca_str_qc_con}},

    { 0x79, "QCO_RETURN",        {qca_unwind_proc,  qca_null,        qca_null}},
    { 0x7A, "QCO_RETURN_NOUGHT", {qca_unwind_proc,  qca_push_nought, qca_null}},
    { 0x7B, "QCO_RETURN_ZERO",   {qca_unwind_proc,  qca_push_zero,   qca_null}},
    { 0x7C, "QCO_RETURN_NULL",   {qca_unwind_proc,  qca_push_null,   qca_null}},
    { 0x7D, "QCO_PROC",          {qca_push_proc,    qca_push_null,   qca_null}},
    { 0x7F, "QCO_ASS_INT",       {qca_ass_int,      qca_null,        qca_null}},
    { 0x81, "QCO_ASS_STR",       {qca_ass_str,      qca_null,        qca_null}},
    { 0x84, "QCO_DROP_NUM",      {qca_pop_num,      qca_null,        qca_null}},
  };

#define SIZEOF_QCODE_INFO (sizeof(qcode_info)/sizeof(NOBJ_QCODE_INFO))

////////////////////////////////////////////////////////////////////////////////
//
// Execute one QCode 
//
////////////////////////////////////////////////////////////////////////////////

int execute_qcode(NOBJ_MACHINE *m)
{
  uint8_t    field_flag;
  NOBJ_QCS   s;
  int        found;

  s.done = 0;
  
  while(!s.done)
    {
      // Get the qcode using the PC from the stack
      
      s.qcode = m->stack[m->rta_pc];

      printf("\nExecuting QCode %02X at %04X", s.qcode, m->rta_pc);
      
      (m->rta_pc)++;

      found = 0;

      for(int q=0; q<SIZEOF_QCODE_INFO; q++)
	{
	  if( s.qcode == qcode_info[q].qcode )
	    {
	      printf("\n Executing %s", qcode_info[q].name);
	      
	      // Perform actions
	      for(int a=0; a<NOBJ_QC_NUM_ACTIONS; a++)
		{
		  qcode_info[q].action[a](m, &s);
		}
	      found = 1;
	      break;
	    }
	}

      if( !found )
	{
	  s.done = 1;
	}

      if ( m->rta_fp == 0 )
	{
	  printf("\n===  Exit ====");
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

