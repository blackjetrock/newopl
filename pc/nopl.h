#define MAX_NOPL_LINE         256
#define PRT_MAX_LINE          400
#define NOPL_MAX_LABEL         12
#define NOPL_MAX_SUFFIX_BYTES   8
#define MAX_COND_FIXUP        400

#include <stdarg.h>

#include "newopl.h"

#include "nopl_obj.h"
#include "newopl_types.h"



#include "newopl_exec.h"
#include "newopl_lib.h"

#include "qcode.h"
#include "errors.h"
#include "qcode_clock.h"
#include "parser.h"

#include "machine.h"

#include "calc.h"

#ifdef TUI
#include "tui.h"
#endif


