////////////////////////////////////////////////////////////////////////////////
//
// Files
//
////////////////////////////////////////////////////////////////////////////////

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>

#include "nopl.h"

////////////////////////////////////////////////////////////////////////////////
//
// Logical files
//
////////////////////////////////////////////////////////////////////////////////

NOPL_LOGICAL_FILE logical_file_info[NOPL_NUM_LOGICAL_FILES];

//------------------------------------------------------------------------------

void init_logical_files(void)
{
  for(int logfile=0; logfile<NOPL_NUM_LOGICAL_FILES; logfile++)
    {
      logical_file_info[logfile].open = 0;
      strcpy(logical_file_info[logfile].name, "");
    }
}

//------------------------------------------------------------------------------
//
// We build a list of field names, as pointers to chars.
// The strings themselves are stored in a pool of characters (char array)

#define LFI logical_file_info[logfile]

void logfile_store_field_names(NOBJ_MACHINE *m, int logfile, uint8_t *flist)
{
  uint8_t b;
  
  LFI.num_field_names = 0;
  LFI.fname_pool_i = 0;

  // Scan the QCode list and build the field names
  b = *(flist++);

  printf("\nlfs: b:%d", b);
  
  while(b !=  QCO_END_FIELDS )
    {
      int len;

      // We have a start for the string
      LFI.field_name[LFI.num_field_names] = &(LFI.field_name_charpool[LFI.fname_pool_i]);

      // Type
      LFI.field_type[LFI.num_field_names] = b;

      len = *(flist++);

      printf("\nlfs: len:%d", len);
	
      for(int i=0; i<len; i++)
	{
	  LFI.field_name_charpool[(LFI.fname_pool_i)++] = *(flist++);
	  printf("\nlfs: ch:%c", LFI.field_name_charpool[(LFI.fname_pool_i-1)]);
	}

      LFI.field_name_charpool[(LFI.fname_pool_i)++] = '\0';
      (LFI.num_field_names)++;

      b = *(flist++);
    }
}

//------------------------------------------------------------------------------

int logfile_get_field_index(NOBJ_MACHINE *m, int logfile, char *field_name)
{
  for(int i=0; i<LFI.num_field_names; i++)
    {
      if( strcmp(LFI.field_name[i], field_name) )
	{
	  return(i);
	}
    }
  return(-1);
}

//------------------------------------------------------------------------------

char *address_tab_n(int n, int logfile)
{
  char *p = &(logical_file_info[logfile].buffer[0]);
  int n_found = 0;

  if( n == 0 )
    {
      return(p);
    }
  
  for(int i=0; (i<n) && (i<logical_file_info[logfile].buffer_size); i++)
    {
      if( *p == NOPL_FIELD_DELIMITER )
	{
	  n_found++;

	  if(n_found == n )
	    {
	      return(p);
	    }
	}
    }

  return(p);
}

//------------------------------------------------------------------------------

char field_val[256];

char *logfile_get_field_as_str(NOBJ_MACHINE *m, int logfile, char *field_name)
{
  int field_num;
  char *start, *end;
  
  if( (field_num = logfile_get_field_index(m, logfile, field_name)) != -1 )
    {
      // Get field value
      // Find nth field delimited by FIELD_DELIMITER character

      if( field_num == 0 )
	{
	  start = &(logical_file_info[logfile].buffer[0]);
	}
      else
	{
	  start = address_tab_n(field_num, logfile)+1;
	}

      end = address_tab_n(field_num+1, logfile)-1;

      // Copy field value
      int i;
      for(i = 0; i<end-start+1; i++)
	{
	  field_val[i] = *(start+i);
	}

      field_val[i] = '\0';
    }

  return(field_val);
}

//------------------------------------------------------------------------------

void logfile_put_field_as_str(NOBJ_MACHINE *m, int logfile, char *field_name, char *field_val)
{
  int field_num;
  char *start, *end;
  
  if( (field_num = logfile_get_field_index(m, logfile, field_name)) != -1 )
    {
      // Find nth field delimited by FIELD_DELIMITER character
      if( field_num == 0 )
	{
	  start = &(logical_file_info[logfile].buffer[0]);
	}
      else
	{
	  start = address_tab_n(field_num, logfile)+1;
	}

      end = address_tab_n(field_num+1, logfile)-1;

      // We need to resize the data in the buffer to make the space between the tabs (or ends)
      // match the field value length
      char *new_end = start + strlen(field_val);
      int delta = new_end - end;
      char *buf_end = &(logical_file_info[logfile].buffer[0]) + logical_file_info[logfile].buffer_size;
      
      // Shift data as long as new end is in buffer
      if( new_end - start + 1 > NOPL_LOGICAL_FILE_BUFFER_LEN )
	{
	  runtime_error(ER_RT_RB, "Field won't fit in buffer");
	  return;
	}

      // Move the data
      // No move if delta is zero
      char *from, *to;
      
      if( delta != 0 )
	{
	  // Move data
	  from = end;
	  to   = new_end;
	  memmove(to, from, buf_end - from );
	}

      for(int i=0; i<strlen(field_val); i++)
	{
	  *(start+i) = *(field_val+i);
	}
    }
}

