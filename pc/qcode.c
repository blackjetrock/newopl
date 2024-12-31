#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>

#include "nopl.h"

////////////////////////////////////////////////////////////////////////////////

void dbpfq(const char *caller, char *fmt, ...)
{
  va_list valist;

  va_start(valist, fmt);
  fprintf(exdbfp, "\n(%s)", caller);
  
  vfprintf(exdbfp, fmt, valist);
  va_end(valist);
  fflush(exdbfp);
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

  dbq("PC:%04X", m->rta_pc);
  
  r  = (m->stack[(m->rta_pc)++]) << 8;
  r |= (m->stack[(m->rta_pc)++]);

  dbq("val:%04X", r);
  
  return(r);
}

//------------------------------------------------------------------------------

uint8_t qcode_next_8(NOBJ_MACHINE *m)
{
  uint8_t r;

  r  = (m->stack[(m->rta_pc)++]);

  dbq("%s: %02X at %04X", __FUNCTION__, r, (m->rta_pc)-1);
  
  return(r);
}


void qca_null(NOBJ_MACHINE *m, NOBJ_QCS *s)
{
}

//------------------------------------------------------------------------------


void qca_str_ind_con(NOBJ_MACHINE *m, NOBJ_QCS *s)
{
  int dp = s->ind_ptr-1;

  // Now stack the string
  s->len = stack_entry_8(m, dp++ );

  int i;
  
  for(i=0; i<s->len; i++)
    {
      s->str[i] = stack_entry_8(m, dp++);
    }
  
  s->str[i] = '\0';
  
  push_machine_string(m, s->len, s->str);

  // Push field
  //push_machine_8(m, 0);
}

//------------------------------------------------------------------------------
//
// get the variable address and push the value as a float

void qca_num_ind_con(NOBJ_MACHINE *m, NOBJ_QCS *s)
{
  int dp = s->ind_ptr+NUM_BYTE_LENGTH-1;

  // Now stack the float
  for(int i=0; i<NUM_BYTE_LENGTH; i++)
    {
      push_machine_8(m, stack_entry_8(m, dp--));
    }
}

void qca_int_qc_con(NOBJ_MACHINE *m, NOBJ_QCS *s)
{
  int h, l;

  h = qcode_next_8(m);
  l = qcode_next_8(m);
  
  // Get int from qcodes and push onto stack
  push_machine_8(m, l);
  push_machine_8(m, h);
}

void qca_str_qc_con(NOBJ_MACHINE *m, NOBJ_QCS *s)
{
  
  // Get string from qcodes and push onto stack
  s->len = qcode_next_8(m);
  
  dbq("  Len:%d", s->len);

  int i;
  
  for(i=0; i<s->len; i++)
    {
      s->str[i] = qcode_next_8(m);
    }
  
  s->str[i] = '\0';
  
  push_machine_string(m, s->len, s->str);
}

//------------------------------------------------------------------------------
//
// Get compact float form and push a full float on the stack

void qca_num_qc_con(NOBJ_MACHINE *m, NOBJ_QCS *s)
{
  int n;
  NOPL_FLOAT num;
  uint8_t digit[NUM_MAX_DIGITS];

  n = qcode_next_8(m);

  dbq("n:%d", n);

  // Clear float
  for(int i=0; i<NUM_MAX_DIGITS; i++)
    {
      digit[i] = 0;
    }
  
  // Copy digit bytes
  // We need to reverse them
  
  for(int i=0; i<n-1; i++)
    {
      // Two digits in each byte
      digit[i] = qcode_next_8(m);
    }

  for(int i=0; i<n-1; i++)
    {
      int j = (n-1-1)-i;
      
      push_machine_8(m, digit[j]);
      
      dbq("Byte %d: j:%d digit:(%02X)", i, j, digit);
    }

  // Zero bytes on stack
  for(int i=0; i<NUM_MAX_DIGITS/2-(n-1); i++)
    {
      push_machine_8(m, 0x00);
    }

  int b = qcode_next_8(m);
  num.sign = b & 0x80;
  b &= 0x7f;
  num.exponent = b;

  push_machine_8(m, num.exponent);
  push_machine_8(m, num.sign);

  dbq("Sign:%d Exponent:%d (%02X)", num.sign, num.exponent, num.exponent);
}

//------------------------------------------------------------------------------

void qca_fp(NOBJ_MACHINE *m, NOBJ_QCS *s)
{
  // Get pointer to string
  s->ind_ptr = qcode_next_16(m);
  
  dbq("ind ptr:%04X", s->ind_ptr);
  
  // Add to FP
  s->ind_ptr += m->rta_fp;
  
  dbq("ind_ptr: %04X", s->ind_ptr);
}

//------------------------------------------------------------------------------

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

void qca_push_ind_addr(NOBJ_MACHINE *m, NOBJ_QCS *s)
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

void qca_push_int_at_ind(NOBJ_MACHINE *m, NOBJ_QCS *s)
{
  int val = stack_entry_16(m, s->ind_ptr);

  dbq("val=%d (%04X)", val, val);
  
  // Push integer at the address we calculated
  push_machine_16(m,  val);
}

void qca_push_str(NOBJ_MACHINE *m, NOBJ_QCS *s)
{
  // Push string at the address we calculated
  push_machine_16(m,  stack_entry_16(m, s->ind_ptr));
}

void qca_push_proc(NOBJ_MACHINE *m, NOBJ_QCS *s)
{
  dbq("QCO_PROC");

  FILE *fp;
  
  // Get proc name
  s->len = qcode_next_8(m);

  dbq("  Len:%d", s->len);

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
  dbq("Loading PROC %s", s->str);
	  
  // Load the procedure file
  fp = fopen(s->str, "r");
	  
  if( fp == NULL )
    {
      runtime_error("Cannot open '%s'", s->str);
      dbq("Cannot open '%s'", s->str);
      exit(-1);
    }
	  
  dbq("Loaded '%s'", s->str);
	  
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
  // Dump stack for dbq
  dbq("==============================================Unwind============================");
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

      dbq("Stack reset");
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

//------------------------------------------------------------------------------

void qca_ass_int(NOBJ_MACHINE *m, NOBJ_QCS *s)
{
  // Drop int
  s->integer = pop_machine_int(m);

  // Check for field
  s->field_flag = pop_machine_8(m);
  
  // Drop int address
  s->addr = pop_machine_16(m);

  dbq("Int:%d (%04X) Addr:%04X", s->integer, s->integer, s->addr);

  if( s->field_flag )
    {
    }
  else
    {
      // Assign integer to variable
      m->stack[s->addr+0] = s->integer >> 8;
      m->stack[s->addr+1] = s->integer  & 0xFF;
    }
}

//------------------------------------------------------------------------------

void qca_ass_num(NOBJ_MACHINE *m, NOBJ_QCS *s)
{
  uint8_t  num_bytes[NUM_BYTE_LENGTH];
  
  // Drop float bytes
  for(int i=0; i<NUM_BYTE_LENGTH; i++)
    {
      num_bytes[i] = pop_machine_8(m);
    }
  
  // Check for field
  s->field_flag = pop_machine_8(m);
  
  // Drop int address
  s->addr = pop_machine_16(m);

  if( s->field_flag )
    {
    }
  else
    {
      // Copy bytes from stack to memory
      for(int i=0; i<NUM_BYTE_LENGTH; i++)
	{
	  m->stack[s->addr+i] = num_bytes[i];
	}
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
  //  s->len      = pop_machine_8(m);
	  
  // Assign string to variable
  // We are pointing at first byte of string
  
  // All OK
  // Copy data
  
  // We copy the length with the data
  m->stack[s->str_addr-1] = s->len;
  
  for(int i=0; i<s->len; i++)
    {
      m->stack[s->str_addr+i] = s->str[i];
    }
  
  // No need to zero the remaining space
  
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
  s->num = pop_machine_num(m);
}

void qca_pop_int(NOBJ_MACHINE *m, NOBJ_QCS *s)
{
  s->integer = pop_machine_int(m);
}

// Pop a variable ref, then push the address
void qca_pop_ref(NOBJ_MACHINE *m, NOBJ_QCS *s)
{
  int addr;
  int field;
  
  // Field flag
  field = pop_machine_8(m);
  
  // Address
  addr = pop_machine_16(m);
  
  push_machine_16(m, addr);
  
  dbq("Field:%d (%02X) Addr:%d (%04X)", field, field, addr, addr);
}

void qca_pop_2int(NOBJ_MACHINE *m, NOBJ_QCS *s)
{
  dbq("Int:%d (%04X) Int2:%d (%04X)", s->integer, s->integer, s->integer2, s->integer2);
  s->integer  = pop_machine_int(m);
  s->integer2 = pop_machine_int(m);
  dbq("Int:%d (%04X) Int2:%d (%04X)", s->integer, s->integer, s->integer2, s->integer2);
}

void qca_pop_2num(NOBJ_MACHINE *m, NOBJ_QCS *s)
{
  s->num   = pop_machine_num(m);
  s->num2  = pop_machine_num(m);
}

void qca_add(NOBJ_MACHINE *m, NOBJ_QCS *s)
{
  dbq("Int:%d (%04X) Int2:%d (%04X)", s->integer, s->integer, s->integer2, s->integer2);
  s->result = s->integer+s->integer2;
  
  dbq("Res:%d (%04X)", s->result, s->result);
}

void qca_sub(NOBJ_MACHINE *m, NOBJ_QCS *s)
{
  s->result = s->integer2 - s->integer;
}

void qca_mul(NOBJ_MACHINE *m, NOBJ_QCS *s)
{
  s->result = s->integer * s->integer2;
}

void qca_div(NOBJ_MACHINE *m, NOBJ_QCS *s)
{
  s->result = s->integer2 / s->integer;
}

void qca_push_result(NOBJ_MACHINE *m, NOBJ_QCS *s)
{
  dbq("res:%d (%04X)", s->result, s->result);
  push_machine_16(m, s->result);
}

void qca_push_num_result(NOBJ_MACHINE *m, NOBJ_QCS *s)
{
  dbq_num("Push num result: ", &(s->num_result));
  push_machine_num(m, &(s->num_result));
}

void qca_push_num(NOBJ_MACHINE *m, NOBJ_QCS *s)
{
  dbq("Num:%d", s->num);
  push_machine_num(m, &(s->num));
}

void qca_umin_num(NOBJ_MACHINE *m, NOBJ_QCS *s)
{
  dbq("Num:%d", s->num);

  if( s->num.sign == 0 )
    {
      s->num.sign = 0x80;
    }
  else
    {
      s->num.sign = 0x00;
    }
}

void qca_umin_int(NOBJ_MACHINE *m, NOBJ_QCS *s)
{
  dbq("Int:%d", s->integer);
  s->result = - s->integer;
  dbq("-ve Int:%d", s->integer);

}

void qca_pop_str(NOBJ_MACHINE *m, NOBJ_QCS *s)
{
  pop_machine_string(m, &(s->len), s->str);
}

void qca_pop_2str(NOBJ_MACHINE *m, NOBJ_QCS *s)
{
  pop_machine_string(m, &(s->len), s->str);
  pop_machine_string(m, &(s->len2), s->str2);
}

//------------------------------------------------------------------------------

void qca_print_int(NOBJ_MACHINE *m, NOBJ_QCS *s)
{
  printf("%d", s->integer);
}

void qca_print_num(NOBJ_MACHINE *m, NOBJ_QCS *s)
{
  NOPL_FLOAT n = s->num;

  if(n.sign)
    {
      printf("-");
    }

  printf("%d.", n.digits[0]);

  for(int i=1; i<NUM_MAX_DIGITS; i++)
    {
      printf("%d", n.digits[i]);
    }

  printf(" E%d", (int)n.exponent);
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

void qca_print_sp(NOBJ_MACHINE *m, NOBJ_QCS *s)
{
  printf(" ");
}

//------------------------------------------------------------------------------

void qca_eq_str(NOBJ_MACHINE *m, NOBJ_QCS *s)
{
  NOBJ_INT res = 0;
  
  dbq("Int:%d (%04X) Int2:%d (%04X)", s->integer, s->integer, s->integer2, s->integer2);

  // Terminate for C
  s->str[s->len]   = '\0';
  s->str2[s->len2] = '\0';
  
  if( strcmp(s->str, s->str2) == 0 )
    {
      res = NOBJ_TRUE;    
    }
  else
    {
      res = NOBJ_FALSE;
    }

  // Push result
  push_machine_16(m, res);
}

//------------------------------------------------------------------------------

void qca_eq_int(NOBJ_MACHINE *m, NOBJ_QCS *s)
{
  NOBJ_INT res = 0;
  
  dbq("Int:%d (%04X) Int2:%d (%04X)", s->integer, s->integer, s->integer2, s->integer2);

  if( s->integer == s->integer2 )
    {
      res = NOBJ_TRUE;    
    }
  else
    {
      res = NOBJ_FALSE;
    }

  // Push result
  push_machine_16(m, res);
}

void qca_ne_int(NOBJ_MACHINE *m, NOBJ_QCS *s)
{
  NOBJ_INT res = 0;
  
  if( s->integer != s->integer2 )
    {
      res = NOBJ_TRUE;    
    }
  else
    {
      res = NOBJ_FALSE;
    }

  dbq("Int:%d (%04X) Int2:%d (%04X) res:%d", s->integer, s->integer, s->integer2, s->integer2, res);
  
  // Push result
  push_machine_16(m, res);
}

void qca_gt_int(NOBJ_MACHINE *m, NOBJ_QCS *s)
{
  NOBJ_INT res = 0;
  
  dbq("Int:%d (%04X) Int2:%d (%04X)", s->integer, s->integer, s->integer2, s->integer2);

  if( s->integer2 > s->integer )
    {
      res = NOBJ_TRUE;    
    }
  else
    {
      res = NOBJ_FALSE;
    }

  // Push result
  push_machine_16(m, res);
}

void qca_lt_int(NOBJ_MACHINE *m, NOBJ_QCS *s)
{
  NOBJ_INT res = 0;
  
  dbq("Int:%d (%04X) Int2:%d (%04X)", s->integer, s->integer, s->integer2, s->integer2);

  if( s->integer2 < s->integer )
    {
      res = NOBJ_TRUE;    
    }
  else
    {
      res = NOBJ_FALSE;
    }

  // Push result
  push_machine_16(m, res);
}

void qca_gte_int(NOBJ_MACHINE *m, NOBJ_QCS *s)
{
  NOBJ_INT res = 0;
  
  dbq("Int:%d (%04X) Int2:%d (%04X)", s->integer, s->integer, s->integer2, s->integer2);

  if( s->integer2 >= s->integer )
    {
      res = NOBJ_TRUE;    
    }
  else
    {
      res = NOBJ_FALSE;
    }

  // Push result
  push_machine_16(m, res);
}

void qca_lte_int(NOBJ_MACHINE *m, NOBJ_QCS *s)
{
  NOBJ_INT res = 0;
  
  dbq("Int:%d (%04X) Int2:%d (%04X)", s->integer, s->integer, s->integer2, s->integer2);

  if( s->integer2 <= s->integer )
    {
      res = NOBJ_TRUE;    
    }
  else
    {
      res = NOBJ_FALSE;
    }

  // Push result
  push_machine_16(m, res);
}

void qca_add_int(NOBJ_MACHINE *m, NOBJ_QCS *s)
{
  NOBJ_INT res = 0;
  
  s->result = s->integer2 + s->integer;

  dbq("Int:%d (%04X) + Int2:%d (%04X) = %d (%04X)", s->integer, s->integer, s->integer2, s->integer2, s->result, s->result);
}

void qca_and_int(NOBJ_MACHINE *m, NOBJ_QCS *s)
{
  NOBJ_INT res = 0;
  
  s->result = s->integer2 &  s->integer;

  dbq("Int:%d (%04X) + Int2:%d (%04X) = %d (%04X)", s->integer, s->integer, s->integer2, s->integer2, s->result, s->result);
}

void qca_or_int(NOBJ_MACHINE *m, NOBJ_QCS *s)
{
  NOBJ_INT res = 0;
  
  s->result = s->integer2 | s->integer;

  dbq("Int:%d (%04X) + Int2:%d (%04X) = %d (%04X)", s->integer, s->integer, s->integer2, s->integer2, s->result, s->result);
}

void qca_not_int(NOBJ_MACHINE *m, NOBJ_QCS *s)
{
  NOBJ_INT res = 0;
  
  s->result = ~s->integer;

  dbq("Int:%d (%04X) + Int2:%d (%04X) = %d (%04X)", s->integer, s->integer, s->integer2, s->integer2, s->result, s->result);
}

//------------------------------------------------------------------------------

void qca_eq_num(NOBJ_MACHINE *m, NOBJ_QCS *s)
{
  NOBJ_INT res = 0;
  
  if( num_eq(&(s->num), &(s->num2)) )
    {
      res = NOBJ_TRUE;    
    }
  else
    {
      res = NOBJ_FALSE;
    }

  // Push result
  push_machine_16(m, res);
}

void qca_ne_num(NOBJ_MACHINE *m, NOBJ_QCS *s)
{
  NOBJ_INT res = 0;
  
  if( num_ne((&s->num), &(s->num2)) )
    {
      res = NOBJ_TRUE;    
    }
  else
    {
      res = NOBJ_FALSE;
    }

  // Push result
  push_machine_16(m, res);
}

void qca_gt_num(NOBJ_MACHINE *m, NOBJ_QCS *s)
{
  NOBJ_INT res = 0;
  
  if( num_gt((&s->num), &(s->num2)) )
    {
      res = NOBJ_TRUE;    
    }
  else
    {
      res = NOBJ_FALSE;
    }

  // Push result
  push_machine_16(m, res);
}

void qca_lt_num(NOBJ_MACHINE *m, NOBJ_QCS *s)
{
  NOBJ_INT res = 0;
  
  if( num_lt(&(s->num2), &(s->num)) )
    {
      res = NOBJ_TRUE;    
    }
  else
    {
      res = NOBJ_FALSE;
    }

  // Push result
  push_machine_16(m, res);
}

void qca_gte_num(NOBJ_MACHINE *m, NOBJ_QCS *s)
{
  NOBJ_INT res = 0;
  
  if( num_gte(&(s->num2), &(s->num)) )
    {
      res = NOBJ_TRUE;    
    }
  else
    {
      res = NOBJ_FALSE;
    }

  // Push result
  push_machine_16(m, res);
}

void qca_lte_num(NOBJ_MACHINE *m, NOBJ_QCS *s)
{
  NOBJ_INT res = 0;
  
  if( num_lte(&(s->num2), &(s->num)) )
    {
      res = NOBJ_TRUE;    
    }
  else
    {
      res = NOBJ_FALSE;
    }

  // Push result
  push_machine_16(m, res);
}

//------------------------------------------------------------------------------

void qca_add_num(NOBJ_MACHINE *m, NOBJ_QCS *s)
{
  num_add(&(s->num), &(s->num2), &(s->num_result));
  dbq_num("num: ", &(s->num));
  dbq_num("num2:", &(s->num2));
  dbq_num("res: ", &(s->num_result));
}

void qca_sub_num(NOBJ_MACHINE *m, NOBJ_QCS *s)
{
  num_sub(&(s->num), &(s->num2), &(s->num_result));
  dbq_num("num: ", &(s->num));
  dbq_num("num2:", &(s->num2));
  dbq_num("res: ", &(s->num_result));
  dbq_num_exploded("res: ", &(s->num_result));
}

////////////////////////////////////////////////////////////////////////////////

void qca_asc(NOBJ_MACHINE *m, NOBJ_QCS *s)
{
  s->result = s->str[0];
}

void qca_len(NOBJ_MACHINE *m, NOBJ_QCS *s)
{
  s->result = strlen(s->str);
}

void qca_loc(NOBJ_MACHINE *m, NOBJ_QCS *s)
{
  char *answer = strstr(s->str2, s->str);

  if( answer == NULL )
    {
      s->result = 0;
    }
  else
    {
      s->result = answer - s->str2;
    }
}

void qca_bra_false(NOBJ_MACHINE *m, NOBJ_QCS *s)
{
  NOBJ_INT flag;
  int16_t offset;

  // Get flag to test
  flag = pop_machine_16(m);

  // Get offset
  offset = m->stack[m->rta_pc++];
  offset <<= 8;
  offset |= m->stack[m->rta_pc++];
  
  dbq("Flag:(%04X) offset %d (%04X)", flag, offset, offset);

  if( flag == 0 )
    {
      m->rta_pc += offset -2;
    }
}

void qca_goto(NOBJ_MACHINE *m, NOBJ_QCS *s)
{
  int16_t offset;

  // Get offset
  offset = m->stack[m->rta_pc++];
  offset <<= 8;
  offset |= m->stack[m->rta_pc++];
  
  dbq("Offset %d (%04X)", offset, offset);

  m->rta_pc += offset -2;
}

////////////////////////////////////////////////////////////////////////////////
//
//


NOBJ_QCODE_INFO qcode_info[] =
  {
    { QI_INT_SIM_FP,     "QI_INT_SIM_FP",     {qca_fp,           qca_null,        qca_push_int_at_ind}},
    { QI_STR_SIM_FP,     "QI_STR_SIM_FP",     {qca_fp,           qca_null,        qca_str_ind_con}},
    { QI_NUM_SIM_FP,     "QI_NUM_SIM_FP",     {qca_fp,           qca_null,        qca_num_ind_con}},

    { QI_STR_SIM_IND,    "QI_STR_SIM_IND",    {qca_fp,           qca_ind,         qca_str_ind_con}},
    { QI_LS_INT_SIM_FP,  "QI_LS_INT_SIM_FP",  {qca_fp,           qca_null,        qca_push_ind_addr }},
    { QI_LS_STR_SIM_FP,  "QI_LS_STR_SIM_FP",  {qca_fp,           qca_null,        qca_str }},
    { QI_LS_NUM_SIM_FP,  "QI_LS_NUM_SIM_FP",  {qca_fp,           qca_null,        qca_push_ind_addr}},
    { QI_LS_STR_SIM_IND, "QI_LS_STR_SIM_IND", {qca_fp,           qca_ind,         qca_str }},
    { QI_STK_LIT_BYTE,   "QI_STK_LIT_BYTE",   {qca_null,         qca_null,        qca_push_qc_byte}},
    { QI_INT_CON,        "QI_INT_CON",        {qca_null,         qca_null,        qca_int_qc_con}},
    { QI_STR_CON,        "QI_STR_CON",        {qca_null,         qca_null,        qca_str_qc_con}},
    { QI_NUM_CON,        "QI_NUM_CON",        {qca_null,         qca_null,        qca_num_qc_con}},

    { QCO_EQ_STR,        "QI_EQ_STR",         {qca_pop_2str,     qca_eq_str,      qca_null}},
    { QCO_EQ_INT,        "QI_EQ_INT",         {qca_pop_2int,     qca_eq_int,      qca_null}},
    { QCO_NE_INT,        "QI_NE_INT",         {qca_pop_2int,     qca_ne_int,      qca_null}},
    { QCO_GT_INT,        "QI_GT_INT",         {qca_pop_2int,     qca_gt_int,      qca_null}},
    { QCO_LT_INT,        "QI_LT_INT",         {qca_pop_2int,     qca_lt_int,      qca_null}},
    { QCO_GTE_INT,       "QI_GTE_INT",        {qca_pop_2int,     qca_gte_int,     qca_null}},
    { QCO_LTE_INT,       "QI_LTE_INT",        {qca_pop_2int,     qca_lte_int,     qca_null}},

    { QCO_EQ_NUM,        "QI_EQ_NUM",         {qca_pop_2num,     qca_eq_num,      qca_null}},
    { QCO_NE_NUM,        "QI_NE_NUM",         {qca_pop_2num,     qca_ne_num,      qca_null}},
    { QCO_GT_NUM,        "QI_GT_NUM",         {qca_pop_2num,     qca_gt_num,      qca_null}},
    { QCO_LT_NUM,        "QI_LT_NUM",         {qca_pop_2num,     qca_lt_num,      qca_null}},
    { QCO_GTE_NUM,       "QI_GTE_NUM",        {qca_pop_2num,     qca_gte_num,     qca_null}},
    { QCO_LTE_NUM,       "QI_LTE_NUM",        {qca_pop_2num,     qca_lte_num,     qca_null}},

    { QCO_PRINT_INT,     "QCO_PRINT_INT",     {qca_pop_int,      qca_print_int,   qca_null}},
    { QCO_PRINT_NUM,     "QCO_PRINT_NUM",     {qca_pop_num,      qca_print_num,   qca_null}},
    { QCO_PRINT_STR,     "QCO_PRINT_STR",     {qca_pop_str,      qca_print_str,   qca_null}},
    { QCO_PRINT_CR,      "QCO_PRINT_CR",      {qca_null,         qca_print_cr,    qca_null}},
    { QCO_PRINT_SP,      "QCO_PRINT_SP",      {qca_null,         qca_print_sp,    qca_null}},
    { QCO_UMIN_NUM,      "QCO_UMIN_NUM",      {qca_pop_num,      qca_umin_num,    qca_push_num}},
    { QCO_UMIN_INT,      "QCO_UMIN_INT",      {qca_pop_int,      qca_umin_int,    qca_push_result}},
    { QCO_NOT_INT,       "QCO_NOT_INT",       {qca_pop_int,      qca_not_int,     qca_push_result}},
    
    { QCO_BRA_FALSE,     "QCO_BRA_FALSE",     {qca_bra_false,    qca_null,        qca_null}},
    { QCO_GOTO,          "QCO_GOTO",          {qca_goto,         qca_null,        qca_null}},
    { QCO_RETURN,        "QCO_RETURN",        {qca_unwind_proc,  qca_null,        qca_null}},
    
    { QCO_RETURN_NOUGHT, "QCO_RETURN_NOUGHT", {qca_unwind_proc,  qca_push_nought, qca_null}},
    { QCO_RETURN_ZERO,   "QCO_RETURN_ZERO",   {qca_unwind_proc,  qca_push_zero,   qca_null}},
    { QCO_RETURN_NULL,   "QCO_RETURN_NULL",   {qca_unwind_proc,  qca_push_null,   qca_null}},
    { QCO_PROC,          "QCO_PROC",          {qca_push_proc,    qca_push_null,   qca_null}},
    { QCO_ASS_INT,       "QCO_ASS_INT",       {qca_ass_int,      qca_null,        qca_null}},
    { QCO_ASS_STR,       "QCO_ASS_STR",       {qca_ass_str,      qca_null,        qca_null}},
    { QCO_ASS_NUM,       "QCO_ASS_NUM",       {qca_ass_num,      qca_null,        qca_null}},
    
    // DROP int
    //DROP str
    { QCO_DROP_NUM,      "QCO_DROP_NUM",      {qca_pop_num,      qca_null,        qca_null}},
    { QCO_DROP_WORD,     "QCO_DROP_WORD",     {qca_pop_int,      qca_null,        qca_null}},
    { QCO_DROP_STR,      "QCO_DROP_STR",      {qca_pop_str,      qca_null,        qca_null}},
    { QCO_ADD_INT,       "QCO_ADD_INT",       {qca_pop_2int,     qca_add,         qca_push_result}},
    { QCO_SUB_INT,       "QCO_SUB_INT",       {qca_pop_2int,     qca_sub,         qca_push_result}},
    { QCO_MUL_INT,       "QCO_MUL_INT",       {qca_pop_2int,     qca_mul,         qca_push_result}},
    { QCO_DIV_INT,       "QCO_DIV_INT",       {qca_pop_2int,     qca_div,         qca_push_result}},
    { RTF_ADDR,          "RTF_ADDR",          {qca_pop_ref,      qca_null,        qca_null}},
    { RTF_ASC,           "RTF_ASC",           {qca_pop_str,      qca_asc,         qca_push_result}},
    { RTF_LEN,           "RTF_LEN",           {qca_pop_str,      qca_len,         qca_push_result}},
    { RTF_LOC,           "RTF_LOC",           {qca_pop_2str,     qca_loc,         qca_push_result}},
    { RTF_SECOND,        "RTF_SECOND",        {qca_clock_second, qca_null,        qca_null}},
    { RTF_MINUTE,        "RTF_MINUTE",        {qca_clock_minute, qca_null,        qca_null}},
    { RTF_HOUR,          "RTF_HOUR",          {qca_clock_hour,   qca_null,        qca_null}},
    { RTF_DAY,           "RTF_DAY",           {qca_clock_day,    qca_null,        qca_null}},
    { RTF_MONTH,         "RTF_MONTH",         {qca_clock_month,  qca_null,        qca_null}},
    { RTF_YEAR,          "RTF_YEAR",          {qca_clock_year,   qca_null,        qca_null}},
    { QCO_AND_INT,       "QCO_AND_INT",       {qca_pop_2int,     qca_and_int,     qca_push_result}},
    { QCO_OR_INT,        "QCO_OR_INT",        {qca_pop_2int,     qca_or_int,      qca_push_result}},
    { QCO_ADD_INT,       "QCO_ADD_INT",       {qca_pop_2int,     qca_add_int,     qca_push_result}},
    { QCO_ADD_NUM,       "QCO_ADD_NUM",       {qca_pop_2num,     qca_add_num,     qca_push_num_result}},
    { QCO_SUB_NUM,       "QCO_SUB_NUM",       {qca_pop_2num,     qca_sub_num,     qca_push_num_result}},


   
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

  int retadd   = stack_entry_16(m, fp+6);
  int onerr    = stack_entry_16(m, fp+4);
  int basesp   = stack_entry_16(m, fp+2);
  int framep   = stack_entry_16(m, fp+0);
  int globtab  = stack_entry_16(m, fp-2);

  printf("\n%04X: Return PC    : %04X", fp+6, retadd  );
  printf("\n%04X: ONERR        : %04X", fp+4, onerr   );
  printf("\n%04X: BASE SP      : %04X", fp+2, basesp  );
  printf("\n%04X: FP           : %04X", fp+0, framep  );
  printf("\n%04X: Global Table : %04X", fp-2, globtab );
  
  printf("\n");
}

////////////////////////////////////////////////////////////////////////////////
//
// Display variables
//
////////////////////////////////////////////////////////////////////////////////


void display_variables(NOBJ_MACHINE *m)
{
  int fp = m->rta_fp;
  FILE *vfp;
  char line[300];
  
  // Read the vars.txt file and display the variables
  vfp = fopen("vars.txt", "r");

  if( vfp == NULL )
    {
      printf("\nCannot open vars.txt");
      return;
    }

  int done = 0;

  int i;
  char vname[200];
  char class[200];
  char type[200];
  char decref[200];
  int max_str;
  int max_ary;
  int num_ind;
  int int_offset;
  int16_t offset;

  while(!done )
    {
      if( fgets(line, 300, vfp) == NULL )
	{
	  // we are done
	  done = 1;
	}

      // Print the line and then the value
      //printf("\n%s   ", line);

      // get information
      int scanret = sscanf(line, "%d: VAR: '%[^']' %s %s %s max_str: %d max_ary: %d num_ind: %d offset:%x",
	     &i,
	     &(vname[0]),
	     &(class[0]),
	     &(type[0]),
	     &(decref[0]),
	     &max_str,
	     &max_ary,
	     &num_ind,
	     &int_offset
	     );

      if( scanret == 9 )
	{
	  offset = (uint16_t)int_offset;

	  printf("\n%d:  VAR: '%s' %7s %10s %10s max_str:  %d max_ary:  %d num_ind:  %d offset:%d    ", 
		 i,
		 vname,
		 class,
		 type,
		 decref,
		 max_str,
		 max_ary,
		 num_ind,
		 offset
		 );

	  printf("  %04X:  ", offset+fp+2);	  
	  
	  for(int i=0; i<8; i++)
	    {
	      //	      printf("%02X ", stack_entry_8(m, fp+2+i+offset));
	      printf("%02X ", m->stack[fp+i+offset]);
	    }
	}
    }

  printf("\n");
  return;
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

      dbq("\n--------------------------------------------------------------------------------\n");
      dbq("Executing QCode %02X at %04X", s.qcode, m->rta_pc);
      
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
	  dbq("Not found so exit: %02X\n", s.qcode);
	  s.done = 1;
	  continue;
	}

      sprintf(outline, "rta_sp:%04X rta_fp:%04X rta_pc:%04X qcode:%02X %s",
	      m->rta_sp,
	      m->rta_fp,
	      m->rta_pc,
	      s.qcode,
	      qcode_info[qci].name
	      );

      dbq(outline);
      
      if( single_step )
	{
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

	      if( strcmp(cmdline, "v") == 0 )
		{
		  // Display variables using addresses from the var.txt file
		  // which has to be from a compilation of this .opl file
		  display_variables(m);
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

      dbq("Executing %s\n", qcode_info[qci].name);
      
      // Perform actions
      for(int a=0; a<NOBJ_QC_NUM_ACTIONS; a++)
	{
	  qcode_info[qci].action[a](m, &s);
	}

      sprintf(outline, "rta_sp:%04X rta_fp:%04X rta_pc:%04X qcode:%02X %s",
	      m->rta_sp,
	      m->rta_fp,
	      m->rta_pc,
	      s.qcode,
	      qcode_info[qci].name
	      );

      dbq(outline);

      if ( m->rta_fp == 0 )
	{
	  dbq("===  Exit ====");
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

