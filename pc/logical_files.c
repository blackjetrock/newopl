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

// Logical files
//

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
	  return(logfile);
	}
    }
  return(-1);
}

//------------------------------------------------------------------------------
