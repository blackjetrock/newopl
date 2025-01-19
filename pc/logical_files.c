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
