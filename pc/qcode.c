#include <ctype.h>
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

  r  = (m->stack[(m->rta_pc)++]) << 8;
  r |= (m->stack[(m->rta_pc)++])  & 0x0F;

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
  uint16_t ind_ptr;
  uint8_t  len;

  int done = 0;
  
  while(!done)
    {
      // Get the qcode using the PC from the stack
      
      qcode = m->stack[m->rta_pc];

      printf("\nExecuting QCode %02X at %04X", qcode, m->rta_pc);
      
      (m->rta_pc)++;
      switch(qcode)
	{
	case 0x16:
	  // QI_LS_STR_SIM_IND
	  //
	  // Get pointer to string
	  ind_ptr = qcode_next_16(m);
	  
	  // Push string literal
	  qcode_get_string_push_stack(m);
	  break;
	  
	case 0x24:
	  // QI_STR_CON
	  // Get string from qcodes and push onto stack
	  len = qcode_next_8(m);
	  push_machine_8(m, len);

	  printf("\n  Len:%d", len);
	  
	  for(int i=0; i<len; i++)
	    {
	      push_machine_8(m, qcode_next_8(m));
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

