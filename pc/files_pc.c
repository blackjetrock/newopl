////////////////////////////////////////////////////////////////////////////////
//
// Functions for file support
//
// PC
//
////////////////////////////////////////////////////////////////////////////////

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>

#include "nopl.h"

//------------------------------------------------------------------------------

void files_create(char *filename, int logfile)
{
}

void files_open(char *filename, int logfile)
{
  logical_file_info[logfile].open = 1;
}
