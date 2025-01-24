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

#define DEBUG 0

////////////////////////////////////////////////////////////////////////////////
//
// Logical files
//
////////////////////////////////////////////////////////////////////////////////

int current_logfile;
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

#if DEBUG
  printf("\nlfs: b:%d", b);
#endif
  
  while(b !=  QCO_END_FIELDS )
    {
      int len;

      // We have a start for the string
      LFI.field_name[LFI.num_field_names] = &(LFI.field_name_charpool[LFI.fname_pool_i]);

      // Type
      LFI.field_type[LFI.num_field_names] = b;

      len = *(flist++);

#if DEBUG
      printf("\nlfs: len:%d", len);
#endif
      
      for(int i=0; i<len; i++)
	{
	  LFI.field_name_charpool[(LFI.fname_pool_i)++] = *(flist++);
#if DEBUG
	  printf("\nlfs: ch:%c", LFI.field_name_charpool[(LFI.fname_pool_i-1)]);
#endif
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
      if( strcmp(LFI.field_name[i], field_name)==0 )
	{
	  return(i);
	}
    }
  return(-1);
}

//------------------------------------------------------------------------------

char *address_tab_n(int n, int logfile, int before)
{
  char *p = &(logical_file_info[logfile].buffer[0]);
  char *buf_start = p;
  char *buf_end = buf_start + logical_file_info[logfile].buffer_size - 1; 
  int n_found = 0;

#if DEBUG
  printf("\n\n%s n:%d buf_start:%p buf_end:%p", __FUNCTION__, n, buf_start, buf_end);
#endif
  
  if( n == 0 )
    {
#if DEBUG
      printf("\nret(%p) zero", p);
#endif
      return(p);
    }

  for(int i=0; (n_found<n) && (i<logical_file_info[logfile].buffer_size); i++, p++)
    {
      if( *p == NOPL_FIELD_DELIMITER )
	{
#if DEBUG
	  printf("\nFound");
#endif
	  n_found++;

	  if(n_found == n )
	    {
	      if( before )
		{
		  p--;
		  if( p < buf_start )
		    {
		      p = buf_start;
		    }
#if DEBUG
		  printf("\nret(%p) before", p);
#endif
		  return(p);
		}
	      else
		{
		  p++;
		  if( p > buf_end )
		    {
		      // We allow the code to move the null at the end of the buffer so we can insert data
		      // after the last delimiter
		      p = buf_end+1;
		    }
#if DEBUG
		  printf("\nret(%p) after", p);
#endif
		  return(p);
		}

	    }
	}
    }

  if( p > buf_end )
    {
      p = buf_end;
    }

#if DEBUG
  printf("\nret(%p) end", p);
#endif
  return(p);
}

//------------------------------------------------------------------------------

char field_val[256];

#define BEFORE 1
#define AFTER  0

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
	  start = address_tab_n(field_num, logfile, AFTER);
	}

      end = address_tab_n(field_num+1, logfile, BEFORE);

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

#if DEBUG
  printf("\n\n%s: buf:%p ", __FUNCTION__, &(logical_file_info[logfile].buffer[0]));
#endif

#if DEBUG
  for(int i=0; i< logical_file_info[logfile].buffer_size; i++)
    {
      printf(" %02X", logical_file_info[logfile].buffer[i]);
    }
  printf("\n");
#endif
  
  if( (field_num = logfile_get_field_index(m, logfile, field_name)) != -1 )
    {
#if DEBUG
      printf("Field num:%d Name:%s", field_num, field_name);
#endif
      
      // Find nth field delimited by FIELD_DELIMITER character
      if( field_num == 0 )
	{
	  start = &(logical_file_info[logfile].buffer[0]);
	}
      else
	{
	  start = address_tab_n(field_num, logfile, AFTER);
	}

      end = address_tab_n(field_num+1, logfile, BEFORE);

#if DEBUG
      printf("\nStart:%p End:%p", start, end);
#endif
      
      // We need to insert the field data at the address we have worked out, pushing the other data up
      // by the length of the field

      char *new_end = start + strlen(field_val)-1;
      int delta = strlen(field_val);
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
	  from = start;
	  to   = start+delta;
	  memmove(to, from, buf_end - from );
#if DEBUG
	  printf("\nmemmove(%p, %p, %d)", to, from, buf_end-from);
#endif
	}

      for(int i=0; i<strlen(field_val); i++)
	{
	  *(start+i) = *(field_val+i);
	}

      logical_file_info[logfile].buffer_size += strlen(field_val);
    }

#if DEBUG
  printf("\n\n%s: buf:%p ", __FUNCTION__, &(logical_file_info[logfile].buffer[0]));
#endif

#if DEBUG
  for(int i=0; i< logical_file_info[logfile].buffer_size; i++)
    {
      printf(" %02X", logical_file_info[logfile].buffer[i]);
    }
  printf("\n");
#endif


}

