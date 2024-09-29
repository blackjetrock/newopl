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
//
// Execute one QCode 
//
////////////////////////////////////////////////////////////////////////////////

int execute_qcode(NOBJ_MACHINE *m)
{
  NOBJ_QCODE qcode;
  uint16_t   ind_ptr;
  uint8_t    len;
  uint8_t    data8;
  char       str[NOBJ_FILENAME_MAXLEN];
  uint16_t   str_addr;
  char       procpath[NOBJ_FILENAME_MAXLEN];
  FILE       *fp;
  uint8_t    max_sz;
  int        i;
  uint8_t    field_flag;
  int        done = 0;
  
  while(!done)
    {
      // Get the qcode using the PC from the stack
      
      qcode = m->stack[m->rta_pc];

      printf("\nExecuting QCode %02X at %04X", qcode, m->rta_pc);
      
      (m->rta_pc)++;
      switch(qcode)
	{
	case 0x09:
	  db_qcode("QI_STR_SIM_IND");

	  // Get pointer to string
	  ind_ptr = qcode_next_16(m);
	  printf("\nind ptr:%04X", ind_ptr);
	    
	  // Add to FP
	  ind_ptr += m->rta_fp;

	  printf("\nIND Addr: %04X", ind_ptr);
	  
	  // Then that address has the address
	  ind_ptr = stack_entry_16(m, ind_ptr);

	  // Now stack the string
	  len = stack_entry_8(m, ind_ptr++);

	  for(i=0; i<len; i++)
	    {
	      str[i] = stack_entry_8(m, ind_ptr++);
	    }

	  str[i] = '\0';
	  
	  push_machine_string(m, len, str);
		  
	  break;
	  
	case 0x16:
	  db_qcode("QI_LS_STR_SIM_IND");
	  
	  //
	  // Get pointer to string
	  ind_ptr = qcode_next_16(m);
	  printf("\nind ptr:%04X", ind_ptr);
	    
	  // Add to FP
	  ind_ptr += m->rta_fp;

	  printf("\nIND Addr: %04X", ind_ptr);
	  
	  // Then that address has the address
	  ind_ptr = stack_entry_16(m, ind_ptr);
	  
	  // Get the maximum size
	  max_sz = m->stack[ind_ptr-1];

	  // Push string max length
	  push_machine_8(m, max_sz);

	  // Push address of string
	  push_machine_16(m, ind_ptr);

	  // Push field flag
	  push_machine_8(m, 0);
	  break;

	case 0x20:
	  db_qcode("QI_STK_LIT_BYTE");
	  
	  data8 = qcode_next_8(m);
	  push_machine_8(m, data8);
	  
	  break;

	case 0x24:
	  db_qcode("QI_STR_CON");
	  
	  // Get string from qcodes and push onto stack
	  len = qcode_next_8(m);

	  printf("\n  Len:%d", len);

	  for(i=0; i<len; i++)
	    {
	     str[i] = qcode_next_8(m);
	    }
	  
	  str[i] = '\0';
	  
	  push_machine_string(m, len, str);
	  break;

	case 0x79:
	  db_qcode("QCO_RETURN");
	  break;

	case 0x7A:
	  db_qcode("QCO_RETURN_NULL");
	  break;
	  
	case 0x7B:
	  db_qcode("QCO_RETURN_ZERO");
	  break;
	  
	case 0x7C:
	  db_qcode("QCO_RETURN_NULL");
	  break;
	  
	case 0x7D:
	  db_qcode("QCO_PROC");
	  
	  // Get proc name
	  len = qcode_next_8(m);
	  //push_machine_8(m, len);

	  printf("\n  Len:%d", len);

	  for(i=0; i<len; i++)
	    {
	      str[i] = qcode_next_8(m);
	    }

	  str[i] = '\0';
	  strcpy(procpath, "A:");
	  strcat(procpath, str);
	  
	  strcat(str, ".OB3");

	  
	  // We have the name, open the file
	  debug("\nLoading PROC %s", str);
	  
	  // Load the procedure file
	  fp = fopen(str, "r");
	  
	  if( fp == NULL )
	    {
	      printf("\nCannot open '%s'", str);
	      exit(-1);
	    }
	  
	  printf("\nLoaded '%s'", str);
	  
	  // Discard header
	  read_ob3_header(fp);
	  
	  // Initialise the machine
	  //init_machine(&machine);
	  
	  // Put some parameters on the stack for our test code
	  //push_parameters(m);
	  
	  // Push proc onto stack
	  push_proc(fp, m, procpath, 0);

	  fclose(fp);
	  
	  break;

	case 0x81:
	  db_qcode("QCO_ASS_STR");

	  // Drop string
	  pop_machine_string(m, &len, str);

	  // Check for field
	  field_flag = pop_machine_8(m);
	  
	  // Drop string reference
	  str_addr = pop_machine_16(m);
	  max_sz = pop_machine_8(m);
	  
	  
	  // Assign
	  if( field_flag )
	    {
	    }
	  else
	    {
	      // Assign string to variable
	      // We are pointing at length byte of string
	      // Check lengths
	      if( len <= max_sz )
		{
		  // All OK
		  // Copy data

		  // We copy the length with the data
		  for(i=0; i<len+1; i++)
		    {
		      m->stack[str_addr+i] = str[i];
		    }

		  // No need to zero the remaining space
		 
		}
	      else
		{
		  printf("\n*** String too big. Len %d but variable max is %d", len, max_sz);
		}
	    }
	  
	  break;
	  
	default:
	  done = 1;
	  break;
	  
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

