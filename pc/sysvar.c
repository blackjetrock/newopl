#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nopl.h"

// Current pak
PAK          pkb_curp = 0;
PAK_ADDR     pkw_cpad = 0;
FL_REC_TYPE  flb_rect = 0;
int          flw_crec = 0;
