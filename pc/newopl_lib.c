//
// Common functions that deal with QCodes
//

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


#define READ_ITEM(FP,DEST,NUM,TYPE,ERROR)			\
  if(!read_item(FP, (void *)&(DEST), NUM, sizeof(TYPE)))	\
    {								\
      printf(ERROR);						\
      return;							\
    }


void read_proc_file(FILE *fp, NOBJ_PROC *p)
{
  READ_ITEM(  fp, p->var_space_size, 1, NOBJ_VAR_SPACE_SIZE, "\nError reading var space size.");
  p->var_space_size.size = swap_uint16(p->var_space_size.size);

  READ_ITEM(fp, p->qcode_space_size,   1,                     NOBJ_QCODE_SPACE_SIZE, "\nError reading qcode space size.");
  p->qcode_space_size.size = swap_uint16(p->qcode_space_size.size);
  
  READ_ITEM(fp, p->num_parameters.num, 1,                     NOBJ_NUM_PARAMETERS,   "\nError reading number of parameters.");
  READ_ITEM(fp, p->parameter_types,    p->num_parameters.num, NOBJ_PARAMETER_TYPE,   "\nError reading parameter types.");

  READ_ITEM( fp,  p->global_varname_size, 1, NOBJ_GLOBAL_VARNAME_SIZE, "\nError reading global varname size.");
  p->global_varname_size.size = swap_uint16(p->global_varname_size.size);
  
  //  printf("\nGlobal varname size:%d", p->global_varname_size.size);

  //------------------------------------------------------------------------------
  // Global varname is more complicated to read. Each entry is length
  // prefixed, so read them until the length we have read matches the
  // size we have just read.
  //------------------------------------------------------------------------------
  
  int length_read = 0;
  int num_global_vars = 0;
  
  do
    {
      uint8_t len;
      uint8_t varname[NOBJ_VARNAME_MAXLEN];
      NOBJ_VARTYPE vartype;
      NOBJ_ADDR addr;
      vartype = 0;
      addr = 0;
      
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

      if(!read_item(fp, (void *)&addr, 1, sizeof(NOBJ_ADDR)))
	{
	  printf("\nError reading global varname entry addr field");
	  return;
	}

      //printf("\nVarname type=%02X", vartype);
      //printf("\nAddr addr=%02X", addr);
      length_read += 3;

      //printf("\nLength read:%d out of %d", length_read, p->global_varname_size.size);

      // Add variable to list of globals
      p->global_varname[num_global_vars].type    = vartype;
      p->global_varname[num_global_vars].address = swap_uint16(addr);
      strcpy(p->global_varname[num_global_vars].varname, varname);
      
      num_global_vars++;      
    }

  while (length_read < p->global_varname_size.size );

  p->global_varname_num = num_global_vars;

  //------------------------------------------------------------------------------
  //
  // Now read the external varname table
  //
  //------------------------------------------------------------------------------

  READ_ITEM( fp,  p->external_varname_size, 1, NOBJ_EXTERNAL_VARNAME_SIZE, "\nError reading external varname size.");
  p->external_varname_size.size = swap_uint16(p->external_varname_size.size);

  length_read = 0;
  int num_external_vars = 0;
  
  do
    {
      uint8_t len;
      uint8_t varname[NOBJ_VARNAME_MAXLEN];
      NOBJ_VARTYPE vartype;
      vartype = 0;
      
      if(!read_item(fp, (void *)&len, 1, sizeof(len)))
	{
	  printf("\nError reading external varname entry length");
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

      //printf("\nVarname type=%02X", vartype);
      length_read += 1;

      //printf("\nLength read:%d out of %d", length_read, p->global_varname_size.size);

      // Add variable to list of globals
      p->external_varname[num_external_vars].type    = vartype;
      strcpy(p->external_varname[num_external_vars].varname, varname);
      
      num_external_vars++;      
    }

  while (length_read < p->external_varname_size.size );

  p->external_varname_num = num_external_vars;

  //------------------------------------------------------------------------------
  //
  // Now read the string length fixup table
  //
  //------------------------------------------------------------------------------

  READ_ITEM( fp,  p->strlen_fixup_size, 1, NOBJ_STRLEN_FIXUP_SIZE, "\nError reading string length fixup size.");
  p->strlen_fixup_size.size = swap_uint16(p->strlen_fixup_size.size);
 
  length_read = 0;
  int num_strlen_fixup = 0;
 
  do
    {
      NOBJ_ADDR             addr;
      NOBJ_STRLEN_FIXUP_LEN strlen;
      
      // Read address and strlen
      
      if(!read_item(fp, (void *)&addr, 1, sizeof(NOBJ_ADDR)))
	{
	  printf("\nError reading strlen fixup address field");
	  return;
	}

      //printf("\nVarname type=%02X", vartype);
      length_read += sizeof(NOBJ_ADDR);

      if(!read_item(fp, (void *)&strlen, 1, sizeof(NOBJ_STRLEN_FIXUP_LEN)))
	{
	  printf("\nError reading strlen fixup length field");
	  return;
	}

      //printf("\nVarname type=%02X", vartype);
      length_read += sizeof(NOBJ_STRLEN_FIXUP_LEN);

      //printf("\nLength read:%d out of %d", length_read, p->global_varname_size.size);

      // Add variable to list of globals
      p->strlen_fixup[num_strlen_fixup].address  = swap_uint16(addr);
      p->strlen_fixup[num_strlen_fixup].len      = strlen;
      
      num_strlen_fixup++;      
    }

  while (length_read < p->strlen_fixup_size.size );

  p->strlen_fixup_num = num_strlen_fixup;

  //------------------------------------------------------------------------------
  //
  // Now read the array size fixup table
  //
  //------------------------------------------------------------------------------

  READ_ITEM( fp,  p->arysz_fixup_size, 1, NOBJ_ARYSZ_FIXUP_SIZE, "\nError reading string length fixup size.");
  p->arysz_fixup_size.size = swap_uint16(p->arysz_fixup_size.size);
 
  length_read = 0;
  int num_arysz_fixup = 0;
 
  do
    {
      NOBJ_ADDR             addr;
      NOBJ_ARYSZ_FIXUP_LEN arysz;
      
      // Read address and arysz
      
      if(!read_item(fp, (void *)&addr, 1, sizeof(NOBJ_ADDR)))
	{
	  printf("\nError reading arysz fixup address field");
	  return;
	}

      //printf("\nVarname type=%02X", vartype);
      length_read += sizeof(NOBJ_ADDR);

      if(!read_item(fp, (void *)&arysz, 1, sizeof(NOBJ_ARYSZ_FIXUP_LEN)))
	{
	  printf("\nError reading arysz fixup length field");
	  return;
	}

      //printf("\nVarname type=%02X", vartype);
      length_read += sizeof(NOBJ_ARYSZ_FIXUP_LEN);

      //printf("\nLength read:%d out of %d", length_read, p->global_varname_size.size);

      // Add variable to list of globals
      p->arysz_fixup[num_arysz_fixup].address  = swap_uint16(addr);
      p->arysz_fixup[num_arysz_fixup].len      = arysz;
      
      num_arysz_fixup++;      
    }

  while (length_read < p->arysz_fixup_size.size );

  p->arysz_fixup_num = num_arysz_fixup;

  //------------------------------------------------------------------------------
  //
  // QCode
  //
  // The QCode could be large, so malloc an area to store it rather than having a fixed
  // array size in the PROC structure
  //
  //------------------------------------------------------------------------------

  
}

////////////////////////////////////////////////////////////////////////////////
//
// Decode variable types
//
////////////////////////////////////////////////////////////////////////////////

char res[7];

char *decode_vartype(NOBJ_VARTYPE t)
{
  switch(t)
    {
    case 0x00:
      strcpy(res, "INT   ");
      break;
    case 0x01:
      strcpy(res, "FLT   ");
      break;
    case 0x02:
      strcpy(res, "STR   ");
      break;
    case 0x03:
      strcpy(res, "INTARY");
      break;
    case 0x04:
      strcpy(res, "FLTARY");
      break;
    case 0x05:
      strcpy(res, "STRARY");
      break;
    default:
      strcpy(res, "??????");
      break;
      
    }
  
  return(res);
}
