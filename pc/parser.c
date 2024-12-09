#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <inttypes.h>
#include "nopl.h"
#include "newopl.h"
#include "nopl_obj.h"
#include "parser.h"

//FILE *fp;

// Tokenise OPL
// one argument: an OPL file

// reads next composite line into buffer
char cline[MAX_NOPL_LINE];
int cline_i = 0;
int if_level = 0;
int unique_level = 0;

#define SAVE_I     1
#define NO_SAVE_I  0

// Information about the procedure being translated
char          procedure_name[NOBJ_VARNAME_MAXLEN+1];
NOBJ_VARTYPE  procedure_type;
int           procedure_has_return = 0;

#define I_WHERE     &(cline[cline_i])
#define IDX_WHERE   &(cline[idx])

////////////////////////////////////////////////////////////////////////////////
//
// Index for QCode generation
//
// Set up by header building to point at position where qcode should go

int qcode_idx         = 0;
int pass_number       = 0;
int qcode_header_len  = 0;     // Actually length of ob3 file, header and qcodes
int qcode_start_idx   = 0;      // Where the QCode starts
int qcode_len         = 0;     // Length of QCode
int size_of_qcode_idx = 0;

////////////////////////////////////////////////////////////////////////////////


char *exp_buffer_id_str[] =
  {
    "EXP_BUFF_ID_???",
    "EXP_BUFF_ID_TKN",
    "EXP_BUFF_ID_NONE",
    "EXP_BUFF_ID_SUB_START",
    "EXP_BUFF_ID_SUB_END",
    "EXP_BUFF_ID_VARIABLE",
    "EXP_BUFF_ID_INTEGER",
    "EXP_BUFF_ID_FLT",
    "EXP_BUFF_ID_STR",
    "EXP_BUFF_ID_FUNCTION",
    "EXP_BUFF_ID_OPERATOR",
    "EXP_BUFF_ID_OPERATOR_UNARY",
    "EXP_BUFF_ID_AUTOCON",
    "EXP_BUFF_ID_COMMAND",
    "EXP_BUFF_ID_PROC_CALL",
    "EXP_BUFF_ID_ASSIGN",
    "EXP_BUFF_ID_CONDITIONAL",
    "EXP_BUFF_ID_RETURN",
    "EXP_BUFF_ID_VAR_ADDR_NAME",
    "EXP_BUFF_ID_PRINT",
    "EXP_BUFF_ID_PRINT_SPACE",
    "EXP_BUFF_ID_PRINT_NEWLINE",
    "EXP_BUFF_ID_LPRINT",
    "EXP_BUFF_ID_LPRINT_SPACE",
    "EXP_BUFF_ID_LPRINT_NEWLINE",
    "EXP_BUFF_ID_IF",
    "EXP_BUFF_ID_ENDIF",
    "EXP_BUFF_ID_ELSE",
    "EXP_BUFF_ID_ELSEIF",
    "EXP_BUFF_ID_DO",
    "EXP_BUFF_ID_UNTIL",
    "EXP_BUFF_ID_WHILE",
    "EXP_BUFF_ID_ENDWH",
    "EXP_BUFF_ID_GOTO",
    "EXP_BUFF_ID_TRAP",
    "EXP_BUFF_ID_LABEL",
    "EXP_BUFF_ID_CONTINUE",
    "EXP_BUFF_ID_BREAK",
    "EXP_BUFF_ID_META",
    "EXP_BUFF_ID_BRAENDIF",
    "EXP_BUFF_ID_FIELDVAR",
    "EXP_BUFF_ID_LOGICALFILE",
    "EXP_BUFF_ID_MAX",
  };

#define NUM_BUFF_ID (sizeof(exp_buffer_id_str) / sizeof(char *))

////////////////////////////////////////////////////////////////////////////////

NOBJ_VAR_INFO var_info[MAX_VAR_INFO];
int num_var_info = 0;

////////////////////////////////////////////////////////////////////////////////

struct _FN_INFO
{
  char *name;
  int command;         // 1 if command, 0 if function
  int trappable;       // 1 if can be used with TRAP
  char argparse;       // How to parse the args   O: scan_onoff
                       //                         V: varname list
                       //                         otherwise: scan_expression()
  char *argtypes;
  char *resulttype;
  uint8_t qcode;
}
  fn_info[] =
  {
    { "ABS",      0,  0, ' ',  "f",         "f", 0x00 },
    { "ACOS",     0,  0, ' ',  "f",         "f", 0x00 },
    { "ADDR",     0,  0, 'V',  "V",         "i", 0x00 },
    { "APPEND",   1,  1, ' ',  "",          "v", 0x00 },
    { "ASC",      0,  0, ' ',  "s",         "i", 0x00 },
    { "ASIN",     0,  0, ' ',  "f",         "f", 0x00 },
    { "AT",       1,  0, ' ',  "ii",        "v", 0x4C },
    { "ATAN",     0,  0, ' ',  "f",         "f", 0x00 },
    { "BACK",     1,  1, ' ',  "",          "v", 0x00 },
    { "BEEP",     1,  0, ' ',  "ii",        "v", 0x00 },
    //{ "BREAK",    1,  0, ' ',  "",          "v", 0x00 },
    { "CHR$",     0,  0, ' ',  "i",         "s", 0x00 },
    { "CLOCK",    0,  0, ' ',  "i",         "i", 0x00 },
    { "CLOSE",    1,  1, ' ',  "",          "v", 0x00 },
    { "CLS",      1,  0, ' ',  "",          "v", 0x00 },
    //    { "CONTINUE", 1,  0, ' ',  "",          "v", 0x00 },  // Conditional
    { "COPYW",    1,  1, ' ',  "ss",        "v", 0x00 },
    { "COPY",     1,  1, ' ',  "ss",        "v", 0x00 },
    { "COS",      0,  0, ' ',  "f",         "f", 0x00 },
    { "COUNT",    0,  0, ' ',  "",          "i", 0x00 },
    //{ "CREATE",   1,  1, ' ',  "ii",        "v", 0x00 },  // file format
    { "CURSOR",   1,  0, 'O',  "i",         "v", 0x00 },
    { "DATIM$",   0,  0, ' ',  "",          "s", 0x00 },
    { "DAYNAME$", 0,  0, ' ',  "i",         "s", 0x00 },
    { "DAYS",     0,  0, ' ',  "iii",       "i", 0x00 },
    { "DAY",      0,  0, ' ',  "",          "i", 0x00 },
    { "DEG",      0,  0, ' ',  "f",         "f", 0x00 },
    { "DELETEW",  1,  1, ' ',  "",          "v", 0x00 },
    { "DELETE",   1,  1, ' ',  "",          "v", 0x00 },
    { "DIRW$",    0,  0, ' ',  "s",         "s", 0x00 },
    { "DIR$",     0,  0, ' ',  "s",         "s", 0x00 },
    { "DISP",     0,  0, ' ',  "i$",        "i", 0x00 },
    { "DOW",      0,  0, ' ',  "iii",       "i", 0x00 },
    { "EDIT",     1,  1, ' ',  "s",         "v", 0x00 },
    //  { "ENDIF",    1,  0, ' ',  "",          "v", 0x00 },
    { "EOF",      0,  0, ' ',  "",          "i", 0x00 },
    { "ERASE",    1,  1, ' ',  "",          "v", 0x00 },
    { "ERR$",     0,  0, ' ',  "i",         "s", 0x00 },
    { "ERR",      0,  0, ' ',  "",          "i", 0x00 },
    { "ESCAPE",   1,  0, 'O',  "i",         "v", 0x00 },
    { "EXIST",    0,  0, ' ',  "i",         "s", 0x00 },
    { "EXP",      0,  0, ' ',  "f",         "f", 0x00 },
    { "FINDW",    0,  0, ' ',  "s",         "i", 0x00 },
    { "FIND",     0,  0, ' ',  "s",         "i", 0x00 },
    { "FIRST",    1,  1, ' ',  "",          "v", 0x00 },
    { "FIX$",     0,  0, ' ',  "fii",       "s", 0x00 },
    { "FLT",      0,  0, ' ',  "i",         "f", 0x00 },
    { "FREE",     0,  0, ' ',  "",          "i", 0x00 },
    { "GEN$",     0,  0, ' ',  "fi",        "s", 0x00 },
    { "GET$",     0,  0, ' ',  "",          "s", 0x00 },
    { "GET",      0,  0, ' ',  "",          "i", 0x00 },
    { "HEX$",     0,  0, ' ',  "i",         "s", 0x00 },
    { "HOUR",     0,  0, ' ',  "",          "i", 0x00 },
    { "IABS",     0,  0, ' ',  "i",         "i", 0x00 },
    { "INPUT",    1,  1, ' ',  "i",         "i", 0x00 },
    { "INTF",     0,  0, ' ',  "f",         "f", 0x00 },
    { "INT",      0,  0, ' ',  "f",         "i", 0x00 },
    //    { "IF",       1,  0, ' ',  "i",         "v", 0x00 },
    { "KEY$",     0,  0, ' ',  "",          "s", 0x00 },
    { "KEY",      0,  0, ' ',  "",          "i", 0x00 },
    { "KSTAT",    1,  0, ' ',  "i",         "v", 0x00 },
    { "LAST",     1,  1, ' ',  "",          "v", 0x00 },
    { "LEFT$",    0,  0, ' ',  "si",        "s", 0x00 },
    { "LEN",      0,  0, ' ',  "s",         "i", 0x00 },
    { "LN",       0,  0, ' ',  "f",         "f", 0x00 },
    { "LOC",      0,  0, ' ',  "ss",        "i", 0x00 },
    { "LOG",      0,  0, ' ',  "f",         "f", 0x00 },
    { "LOWER$",   0,  0, ' ',  "s",         "s", 0x00 },
    //{ "LPRINT",   1,  0, ' ',  "i",         "v", 0x00 },    // As print
    { "MAX",      0,  0, ' ',  "ii",        "f", 0x00 },
    { "MEAN",     0,  0, ' ',  "ii",        "f", 0x00 },    // Multiple forms
    { "MENUN",    0,  0, ' ',  "is",        "i", 0x00 },    // Multiple forms
    { "MENU",     0,  0, ' ',  "s",         "i", 0x00 },
    { "MID$",     0,  0, ' ',  "sii",       "s", 0x00 },
    { "MINUTE",   0,  0, ' ',  "",          "i", 0x00 },
    { "MIN",      0,  0, ' ',  "ii",        "f", 0x00 },    // Multiple forms
    { "MONTH$",   0,  0, ' ',  "i",         "s", 0x00 },    
    { "MONTH",    0,  0, ' ',  "",          "i", 0x00 },
    { "NEXT",     0,  1, ' ',  "",          "v", 0x00 },
    { "NUM$",     0,  0, ' ',  "fi",        "s", 0x00 },
    // { "NOT",      0,  0, ' ',  "i",         "i", 0x00 },
    { "OFFX",     1,  0, ' ',  "i",         "v", 0x00 },    // OFF or OFF x%
    { "OFF",      1,  0, ' ',  "",          "v", 0x00 },    // OFF or OFF x%
    //{ "OPEN",     1,  1, ' ',  "ii",        "v", 0x00 },    // File format
    //    { "ONERR",    1,  0, ' ',  "",          "v", 0x00 },    
    { "PAUSE",    1,  0, ' ',  "i" ,        "v", 0x00 },
    { "PEEKB",    0,  0, ' ',  "i",         "i", 0x00 },
    { "PEEKW",    0,  0, ' ',  "i",         "i", 0x00 },
    { "PI",       0,  0, ' ',  "",          "f", 0x00 },
    { "POKEB",    1,  0, ' ',  "ii",        "v", 0x00 },
    { "POKEW",    1,  0, ' ',  "ii",        "v", 0x00 },
    { "POSITION", 1,  1, ' ',  "i",         "v", 0x00 },
    { "POS",      0,  0, ' ',  "",          "i", 0x00 },
    //{ "PRINT",    1,  0, ' ',  "i",         "v", 0x00 },
    { "RAD",      0,  0, ' ',  "f",         "f", 0x00 },
    { "RAISE",    1,  0, ' ',  "i",         "v", 0x00 },
    { "RANDOMIZE",1,  0, ' ',  "i",         "v", 0x00 },
    { "RECSIZE",  0,  0, ' ',  "",          "i", 0x00 },
    { "REM",      1,  0, ' ',  "",          "v", 0x00 },
    { "RENAME",   1,  1, ' ',  "ss",        "v", 0x00 },
    { "REPT$",    0,  0, ' ',  "si",        "s", 0x00 },
    { "RETURN",   1,  0, ' ',  "i",         "v", 0x00 },
    { "RIGHT$",   0,  0, ' ',  "si",        "s", 0x00 },
    { "RND",      0,  0, ' ',  "",          "f", 0x00 },
    { "SCI$",     0,  0, ' ',  "fii",       "s", 0x00 },
    { "SECOND",   0,  0, ' ',  "",          "i", 0x00 },
    { "SIN",      0,  0, ' ',  "f",         "f", 0x00 },
    { "SPACE",    0,  0, ' ',  "",          "f", 0x00 },
    { "SQR",      0,  0, ' ',  "f",         "f", 0x00 },
    { "STD",      0,  0, ' ',  "ii",        "f", 0x00 },  // Multiple forms
    { "STOP",     1,  0, ' ',  "",          "v", 0x00 },
    { "SUM",      0,  0, ' ',  "ii",        "f", 0x00 },  // Multiple forms
    { "TAN",      0,  0, ' ',  "f",         "f", 0x00 },
    //{ "TRAP",     1,  0, ' ',  "",          "v", 0x00 },  // Prefix, not command?
    { "UDG",      1,  0, ' ',  "iiiiiiiii", "v", 0x00 },
    { "UPDATE",   1,  1, ' ',  "",          "v", 0x00 },
    { "UPPER$",   0,  0, ' ',  "s",         "s", 0x00 },
    { "USE",      1,  1, ' ',  "i",         "v", 0x00 },   // Need A,B,C,D type
    { "USR$",     0,  0, ' ',  "ii",        "s", 0x00 },
    { "USR",      0,  0, ' ',  "ii",        "i", 0x00 },
    { "VAL",      0,  0, ' ',  "s",         "f", 0x00 },
    { "VAR",      0,  0, ' ',  "ii",        "f", 0x00 },   // Multiple forms
    { "VIEW",     0,  0, ' ',  "is",        "i", 0x00 },   
    { "WEEK",     0,  0, ' ',  "iii",       "i", 0x00 },
    { "YEAR",     0,  0, ' ',  "",          "i", 0x00 },
  };


#define NUM_FUNCTIONS (sizeof(fn_info)/sizeof(struct _FN_INFO))

int token_is_function(char *token, char **tokstr)
{
  dbprintf( "\n%s:", __FUNCTION__);
  
  for(int i=0; i<NUM_FUNCTIONS; i++)
    {
      if( (strcmp(token, fn_info[i].name) == 0) )
	{
	  *tokstr = &(fn_info[i].name[0]);
	  
	  dbprintf("%s is function", token);
	  return(1);
	}
    }
  dbprintf( "\n%s:%s is not function", __FUNCTION__, token);
  return(0);
  
}

//------------------------------------------------------------------------------
// Return the function return value type

NOBJ_VARTYPE function_return_type(char *fname)
{
  char *rtype;
  char rt;
  NOBJ_VARTYPE vt;
  
  for(int i=0; i<NUM_FUNCTIONS; i++)
    {
      if( strcmp(fname, fn_info[i].name) == 0 )
	{
	  rtype = fn_info[i].resulttype;
	  rt = *rtype;
	  
	  dbprintf( "\n%s: '%s' =>%c", __FUNCTION__, fname, rt);

	  vt = char_to_type(rt);
	  dbprintf( "\n%s: '%s' =>%d", __FUNCTION__, fname, vt);
	  return(vt);
	  
	}
    }
  
  return(NOBJ_VARTYPE_UNKNOWN);
}

// Return the function return value type
int function_num_args(char *fname)
{

  for(int i=0; i<NUM_FUNCTIONS; i++)
    {
      if( strcmp(fname, fn_info[i].name) == 0 )
	{
	  return(strlen(fn_info[i].argtypes));
	}
    }
  return(0);
}

NOBJ_VARTYPE function_arg_type_n(char *fname, int n)
{
  char *atypes;
  
  for(int i=0; i<NUM_FUNCTIONS; i++)
    {
      if( strcmp(fname, fn_info[i].name) == 0 )
	{
	  atypes = fn_info[i].argtypes;
	  
	  //dbprintf( "\n%s: n:%d at:'%s' =>%c", __FUNCTION__, n, atypes, *(atypes+n));	  
	  return(char_to_type(*(atypes+n)));
	}
    }
  
  return(NOBJ_VARTYPE_UNKNOWN);
}

////////////////////////////////////////////////////////////////////////////////
//
// Operator info
//
// Some operators have a mutable type, they are polymorphic. Some are not.
// The possible types for operators are listed here
//


OP_INFO  op_info[] =
  {
    // Array dereference internal operator (not used)
    { "@",    1, 9, NOBJ_VARTYPE_UNKNOWN, 0, "",     0,   MUTABLE_TYPE, 1, {NOBJ_VARTYPE_INT, NOBJ_VARTYPE_FLT, NOBJ_VARTYPE_STR} },
    { "=",    1, 2, NOBJ_VARTYPE_INT,     0, "",     0,   MUTABLE_TYPE, 0, {NOBJ_VARTYPE_INT, NOBJ_VARTYPE_FLT, NOBJ_VARTYPE_STR} },
    { "<>",   1, 2, NOBJ_VARTYPE_INT,     0, "",     0,   MUTABLE_TYPE, 0, {NOBJ_VARTYPE_INT, NOBJ_VARTYPE_FLT, NOBJ_VARTYPE_STR} },
    { ":=",   0, 1, NOBJ_VARTYPE_UNKNOWN, 0, "",     0,   MUTABLE_TYPE, 1, {NOBJ_VARTYPE_INT, NOBJ_VARTYPE_FLT, NOBJ_VARTYPE_STR, NOBJ_VARTYPE_INTARY, NOBJ_VARTYPE_FLTARY, NOBJ_VARTYPE_STRARY} },
    { "+",    1, 3, NOBJ_VARTYPE_UNKNOWN, 0, "",     0,   MUTABLE_TYPE, 0, {NOBJ_VARTYPE_INT, NOBJ_VARTYPE_FLT, NOBJ_VARTYPE_STR} },
    { "-",    1, 3, NOBJ_VARTYPE_UNKNOWN, 1, "UMIN", 1,   MUTABLE_TYPE, 0, {NOBJ_VARTYPE_INT, NOBJ_VARTYPE_FLT, NOBJ_VARTYPE_STR} },
    { "**",   1, 5, NOBJ_VARTYPE_UNKNOWN, 0, "",     1,   MUTABLE_TYPE, 0, {NOBJ_VARTYPE_INT, NOBJ_VARTYPE_FLT, NOBJ_VARTYPE_INT} },
    { "*",    1, 5, NOBJ_VARTYPE_UNKNOWN, 0, "",     0,   MUTABLE_TYPE, 0, {NOBJ_VARTYPE_INT, NOBJ_VARTYPE_FLT, NOBJ_VARTYPE_INT} },
    { "/",    1, 5, NOBJ_VARTYPE_UNKNOWN, 0, "",     1,   MUTABLE_TYPE, 0, {NOBJ_VARTYPE_INT, NOBJ_VARTYPE_FLT, NOBJ_VARTYPE_INT} },
    { ">=",   1, 2, NOBJ_VARTYPE_INT,     0, "",     1,   MUTABLE_TYPE, 0, {NOBJ_VARTYPE_INT, NOBJ_VARTYPE_FLT, NOBJ_VARTYPE_STR} },
    { "<=",   1, 2, NOBJ_VARTYPE_INT,     0, "",     1,   MUTABLE_TYPE, 0, {NOBJ_VARTYPE_INT, NOBJ_VARTYPE_FLT, NOBJ_VARTYPE_STR} },
    { ">",    1, 2, NOBJ_VARTYPE_INT,     0, "",     1,   MUTABLE_TYPE, 0, {NOBJ_VARTYPE_INT, NOBJ_VARTYPE_FLT, NOBJ_VARTYPE_STR} },
    { "<",    1, 2, NOBJ_VARTYPE_INT,     0, "",     1,   MUTABLE_TYPE, 0, {NOBJ_VARTYPE_INT, NOBJ_VARTYPE_FLT, NOBJ_VARTYPE_STR} },

    // (Handle bitwise on integer, logical on floats somewhere)
    { "AND",  1, 1, NOBJ_VARTYPE_UNKNOWN, 0, "",     0,   MUTABLE_TYPE, 0, {NOBJ_VARTYPE_INT, NOBJ_VARTYPE_FLT, NOBJ_VARTYPE_INT} },
    { "OR",   1, 1, NOBJ_VARTYPE_UNKNOWN, 0, "",     0,   MUTABLE_TYPE, 0, {NOBJ_VARTYPE_INT, NOBJ_VARTYPE_FLT, NOBJ_VARTYPE_INT} },
    { "NOT",  1, 1, NOBJ_VARTYPE_UNKNOWN, 1, "UNOT", 0,   MUTABLE_TYPE, 0, {NOBJ_VARTYPE_INT, NOBJ_VARTYPE_FLT, NOBJ_VARTYPE_INT} },

    // LZ only
    { "+%",   1, 5, NOBJ_VARTYPE_UNKNOWN, 0, "",     1, IMMUTABLE_TYPE, 0, {NOBJ_VARTYPE_FLT, NOBJ_VARTYPE_FLT, NOBJ_VARTYPE_FLT} },
    { "-%",   1, 5, NOBJ_VARTYPE_UNKNOWN, 0, "",     1, IMMUTABLE_TYPE, 0, {NOBJ_VARTYPE_FLT, NOBJ_VARTYPE_FLT, NOBJ_VARTYPE_FLT} },


    // Unary versions
    { "UMIN", 1, 6, NOBJ_VARTYPE_UNKNOWN, 0, "",     0,   MUTABLE_TYPE, 0, {NOBJ_VARTYPE_INT, NOBJ_VARTYPE_FLT, NOBJ_VARTYPE_FLT} },
    { "UNOT", 1, 1, NOBJ_VARTYPE_UNKNOWN, 0, "",     0,   MUTABLE_TYPE, 0, {NOBJ_VARTYPE_INT, NOBJ_VARTYPE_FLT, NOBJ_VARTYPE_FLT} },
  };


#define NUM_OPERATORS_VAL (sizeof(op_info)/sizeof(struct _OP_INFO))

int num_operators(void)
{
  return(NUM_OPERATORS_VAL);
}

////////////////////////////////////////////////////////////////////////////////
//
// Keyword that aren't in the tables above as they are handled differently,
// but still can't be used as variables etc.
//
////////////////////////////////////////////////////////////////////////////////


typedef struct _OTHER_KEYWORD_INFO
{
  char *name;
} OTHER_KEYWORD_INFO;

OTHER_KEYWORD_INFO other_keywords[] =
  {
    {"IF"},
    {"ENDIF"},
    {"ELSE"},
    {"ELSEIF"},
    {"DO"},
    {"UNTIL"},
    {"WHILE"},
    {"ENDWH"}
  };

#define NUM_OTHER_KEYWORDS (sizeof(other_keywords)/sizeof(struct _OTHER_KEYWORD_INFO))

//------------------------------------------------------------------------------

void make_var_type_array(NOBJ_VARTYPE *vt)
{
  switch(*vt)
    {
    case NOBJ_VARTYPE_INT:
      *vt = NOBJ_VARTYPE_INTARY;
      return;
      break;

    case NOBJ_VARTYPE_FLT:
      *vt = NOBJ_VARTYPE_FLTARY;
      return;
      break;

    case NOBJ_VARTYPE_STR:
      *vt = NOBJ_VARTYPE_STRARY;
      return;
      break;
    }

  // A type that cannot be turned into an array
  internal_error("Type %s canot be turned into an array", type_to_str(*vt));
  
}

//------------------------------------------------------------------------------

int var_type_is_array(NOBJ_VARTYPE vt)
{
  switch(vt)
    {
    case NOBJ_VARTYPE_INTARY:
    case NOBJ_VARTYPE_FLTARY:
    case NOBJ_VARTYPE_STRARY:
      return(1);
      break;
    }
  
  return(0);  
}

//------------------------------------------------------------------------------

int token_is_other_keyword(char *token)
{
  dbprintf( "");
  
  for(int i=0; i<NUM_OTHER_KEYWORDS; i++)
    {
      if( (strcmp(token, other_keywords[i].name) == 0) )
	{
	  dbprintf("'%s' is other keyword", token);
	  return(1);
	}
    }
  dbprintf( "'%s' is not other keyword", token);
  return(0);
  
}

char *var_class_to_str(NOPL_VAR_CLASS vc)
{
  switch(vc)
    {
    case NOPL_VAR_CLASS_UNKNOWN:
      return("Unknown");
      break;

    case NOPL_VAR_CLASS_LOCAL:
      return("Local");
      break;

    case NOPL_VAR_CLASS_GLOBAL:
      return("Global");
      break;

    case NOPL_VAR_CLASS_EXTERNAL:
      return("External");
      break;

    case NOPL_VAR_CLASS_PARAMETER:
      return("Parameter");
      break;

    case NOPL_VAR_CLASS_CALC_MEMORY:
      return("Calc Memory");
      break;
    }

  return("????");
}


////////////////////////////////////////////////////////////////////////////////

void fprint_var_info(FILE *fp, NOBJ_VAR_INFO *vi)
{
  
  fprintf(fp, "VAR: '%18s' %10s %-14s %10s max_str:%3d max_ary:%3d num_ind:%3d offset:%04X",
	  vi->name,
	  var_class_to_str(vi->class),
	  type_to_str(vi->type),
	  (vi->is_ref)?    "REFRNCE":"DECLARE",
	  vi->max_string,
	  vi->max_array,
	  vi->num_indices,
	  vi->offset
	  );
}

void print_var_info(NOBJ_VAR_INFO *vi)
{
  fprintf(ofp, "\n");
  fprint_var_info(ofp, vi);
}

void init_var_info(NOBJ_VAR_INFO *vi)
{
  vi->name[0]     = '\0';
  vi->class       = NOPL_VAR_CLASS_UNKNOWN;
#if 0
  vi->is_global   = 0;
  vi->is_ext      = 0;
#endif
  vi->is_ref      = 0;
#if 0
  vi->is_integer  = 0;
  vi->is_float    = 0;
  vi->is_string   = 0;
  vi->is_array    = 0;
#endif
  vi->type = NOBJ_VARTYPE_UNKNOWN;
  vi->max_string  = 0;
  vi->max_array   = 0;
  vi->num_indices = 0;
}

NOBJ_VAR_INFO *find_var_info(char *name)
{
  for(int i=0; i<num_var_info; i++)
    {
      // Must be an exact match, case insensitive
      if( strlen(name) != strlen(var_info[i].name) )
	{
	  continue;
	}
      
      if( strn_match(name, var_info[i].name, strlen(var_info[i].name)) )
	{
	  // Got a match
	  return(&(var_info[i]));
	}
    }
  
  // Not found
  dump_vars(ofp);
  
  dbprintf("******");
  dbprintf("\nCould not find var '%s'", name);
  dbprintf("******");
  return(NULL);
}

  
//------------------------------------------------------------------------------
//
// Adds variable to info list
//
// If this is a declare then:
//
// Checks for it already being present, error if so
// Adds to list if room
//
// If reference:
//
// Checks for it already being present, ok, if so
// If not present then adds as an external
//

void add_var_entry(NOBJ_VAR_INFO *vi)
{
  if(num_var_info < (MAX_VAR_INFO-1))
    {
      var_info[num_var_info] = *vi;
      num_var_info++;
    }
  else
    {
      syntax_error("Too many variables");
    }
}

// We don't add field variables to the bariable list as they are referred to by name
// and they don't appear in the qcode tables.

void add_var_info(NOBJ_VAR_INFO *vi)
{
  NOBJ_VAR_INFO *srch_vi;
  int mem_n = 0;
  
  dbprintf("Name:%s", vi->name);
  
  // See if variable name already present
  srch_vi = find_var_info(vi->name);

  if( srch_vi == NULL )
    {
      dbprintf("Not already present");
      
      // Not present
      if( vi->is_ref )
	{
	  // This is an external, add it to the list
	  
	  if( vi->name[1] == '.' )
	    {
	      vi->class = NOPL_VAR_CLASS_FIELDVAR;
	    }
	  else
	    {
	      // See if this is a calc memory
	      
	      if( sscanf(vi->name, " M%d", &mem_n) == 1 )
		{
		  vi->class = NOPL_VAR_CLASS_CALC_MEMORY;
		  vi->offset = mem_n;
		}
	      else
		{
		  vi->class = NOPL_VAR_CLASS_EXTERNAL;
		}
	    }
	  add_var_entry(vi);	  
	}
      else
	{
	  // Declaring a new variable
	  add_var_entry(vi);	  
	}
    }
  else
    {
      dbprintf("Already present");
      
      // Variable already present
      if( vi->is_ref )
	{
	  dbprintf("  reference so OK");
	  // This is OK, just referring to the variable
	}
      else
	{
	  dbprintf("  Declaration so possibly BAD");
	  
	  // This isn't necessarily OK, we have the variable declared twice
	  // It is OK on pass 2, pass 1 will check for doubly defined variables
	  if( pass_number == 1 )
	    {
	      syntax_error("Variable '%s' declared twice", vi->name);
	    }
	}
    }
}


//------------------------------------------------------------------------------
//
// Look at the variable table and calculate all the offsets that should be used in the QCode
// to access variables.
//
//
// All referenced from FP on the stack:
//
//
// 2 bytes         FP
// 2 bytes         Global table address (Also has parameter addresses) (Start of glob table)
// globsize bytes  Global table
//                 Indirect addresses to parameters
//                 Global data
//                 External indirection table
//                 Locals
//
// Each procedure header has the form:
// 
//                           Device (zero if top procedure)
//                           Return RTA_PC
//                           ONERR address
//                           BASE_SP
// RTA_FP points at:         Previous RTA_FP
//                           Start address of the global name table
//                           Global name table
//                           Indirection table for externals/parameters
//                           This is followed by the variables, and finally by the Q code.
//
// Local variables and global variables declared in the current procedure are
// accessed directly. A reference to such variables is by an offset from the
// current RTA_FP.
// Parameters and externally declared
//  global
//  variables
//  are
//  accessed
// indirectly.
//  The addresses of these variables are held in the indirection
// table, the required address in this table is found by adding the offset in
// the Q code to the current RTA_FP.
//
// The header itself:
//

// size of the variables on stack
// size of Q code length
// number of parameters
// type of parameter
// size of global area
// global name
// global type
// offset
// global name
// global type
// offset
// global name
// global type
// offset
// bytes of externals
// external name
// external type
// bytes of string fix-ups
// string fix-up offset (from FP)
// max length of string
// string fix-up offset (from FP)
// max length of string
// bytes of array fix-ups
// array fix-up offset (from FP)
// size of array


// 0000 : 017D Size of variable space
// 
// 014C     Size of the qcode  LOCAL L1, L2%, L3$(5)
// 03       Number of parameters
// 
// 02 00 01 Parameter types
// 
// 0008 : 00 2F  Size of global varname table
// 
// 02 47 31 01 FFB3
// G1, float type, address
// 
// 03 47 32 25 00 FFB1
// G2%, integer type, address
// 
// 03 47 33 24 02 FFA3
// G3$, string type, address
// 
// 02 47 34 04 FF80
// G4, float array type, address
// 
// 03 47 35 25 03 FF74
// G5%, integer array type, address
// 
// 03 47 36 24 05 FF18
// G6$, string array type, address
// 
// 03 47 37 25 00 FF15
// G7%, integer type, address
// 
// 
// 
// 
// 0039 : 0020 
// Size of external varname table
// 
// 02 45 31 01 
// E1, float type
// 
// 03 45 32 25 00 
// E2%, integer type
// 
// 03 45 33 24 02 
// E3$, string type
// 
// 02 45 34 04 
// E4, float array type
// 
// 03 45 35 25 03 
// E5%, integer array type
// 
// 03 45 36 24 05 
// E6$, string array type
// 
// 02 4C 35 04 
// E7%, integer type
// 
// 
// 
// 
// 005B : 000C 
// Size of string fixups
// 
// FF04 05 
// Fixup for L3$ maximum string length
// 
// FE85 0C 
// Fixup for L6$() maximum string length
// 
// FFA2 0D 
// Fixup for G3$ maximum string length
// 
// FF17 0E 
// Fixup for G6$() maximum string length
// 
// 0069 : 0018 
// Size of array fixups
// 
// FEE2 0004 
// Fixup for L4() array length
// 
// FED6 0005 
// Fixup for L5%() array length
// 
// FE86 0006 
// Fixup for L6$() array length
// 
// FF80 0004 
// Fixup for G4() array length
// 
// FF74 0005 
// Fixup for G5%() array length
// 
// FF18 0006 
// Fixup for G6$() array length
// 
// 0083 : 
// Start of the QCode instructions
// 



typedef struct _NOPL_QCH_FIELD
{
  char *name;
  int length;
} NOPL_QCH_FIELD;

NOPL_QCH_FIELD qch_fields[] =
  {
    {"size of the variables on stack", 2},
    {"size of Q code length",          2},
    {"number of parameters",           1},
    {"size of global area",            2},
    {"bytes of externals",             2},
    {"bytes of string fix-ups",        2},
    {"bytes of array fix-ups",         2},
  };

// Offsets into QCode header
#define NOPL_QCH_SIZE_VARS      0x00
#define NOPL_QCH_SIZE_QCODE     0x02
#define NOPL_QCH_NUM_PARAM      0x04

uint8_t qcode_header[ MAX_QCODE_HEADER];

int calculate_num_in_class( NOPL_VAR_CLASS class)
{
  int num_param = 0;
  
  for(int i=0; i<num_var_info; i++)
    {
      // Must be an exact match, case insensitive
      if( var_info[i].class == class  )
	{
	  num_param++;
	}
    }
  return(num_param);
}

NOBJ_VAR_INFO *get_class_vi_n( NOPL_VAR_CLASS class, int n)
{
  int ni = 0;
  
  for(int i=0; i<num_var_info; i++)
    {
      // Must be an exact match, case insensitive
      if( var_info[i].class == class  )
	{
	  if( ni == n )
	    {
	      return(&(var_info[i]));
	    }
	  ni++;

	}
    }

  internal_error("QCH entry not found");
  return(NULL);
}

int size_of_type(NOBJ_VAR_INFO *vi)
{
  switch(vi->type)
    {
    case NOBJ_VARTYPE_INT:
      return(2);
      break;
      
    case NOBJ_VARTYPE_FLT:
      return(8);
      break;
      
    case NOBJ_VARTYPE_STR:
      return(vi->max_string+2);
      break;

    case NOBJ_VARTYPE_INTARY:
      return(2*vi->max_array+2);
      break;

    case NOBJ_VARTYPE_FLTARY:
      return(8*vi->max_array+2);
      break;

    case NOBJ_VARTYPE_STRARY:
      return((vi->max_string + 1)*vi->max_array+3);
      break;

    case NOBJ_VARTYPE_VAR_ADDR:
      internal_error("Var addr has no size");
      break;

    case NOBJ_VARTYPE_UNKNOWN:
      internal_error("Unkown has no size");
      break;

    case NOBJ_VARTYPE_VOID:
      internal_error("Void has no size");
      break;
    }
}

int data_offset_of_type(NOBJ_VAR_INFO *vi)
{
  switch(vi->type)
    {
    case NOBJ_VARTYPE_INT:
      return(0);
      break;
      
    case NOBJ_VARTYPE_FLT:
      return(0);
      break;
      
    case NOBJ_VARTYPE_STR:
      return(1);
      break;

    case NOBJ_VARTYPE_INTARY:
      return(0);
      break;

    case NOBJ_VARTYPE_FLTARY:
      return(0);
      break;

    case NOBJ_VARTYPE_STRARY:
      return(1);
      break;

    case NOBJ_VARTYPE_VAR_ADDR:
      internal_error("Var addr has no data offset");
      break;

    case NOBJ_VARTYPE_UNKNOWN:
      internal_error("Unkown has no data offset");
      break;

    case NOBJ_VARTYPE_VOID:
      internal_error("Void has no data offset");
      break;
    }
}


void calculate_var_offsets(void)
{
  // Start with globals, the global table is after the previous FP. It has
  // <name length> <name string> <globa offset (of data) from FP>
  //
  // Location of first global (offset) is negative (Sizeof global table) +
  //                                                     ([size of start of global table ptr])
  //                                                     (size of ext/parameter indirection table)
  //
  // Offsets can't be calcuated until all of these values are known
  // Once offsets are known then the fixups can be calculated
  //
  // The translation of the file is performed on pass 1 in order to get the variable
  // information. Then on pass2 the qcode (OB3) header is created, and qcode generation
  // is performed on each line.
  //
  // Variables ar ein this order (which determines offsets used in the QCode)
  //
  // Globals
  // Indirection table (parameters and externals
  // Locals
  //
}

uint8_t qcode_header[MAX_QCODE_HEADER];

// We need not generate any qcode on the first pass
// That pass is only generating a full variable table, including externals

void build_qcode_header(void)
{
  int num_params = 0;
  int num_globals = 0;
  int num_externals = 0;
  int idx = qcode_idx;

  // Size of variables
  int idx_size_of_vars_on_stack = idx;
  int     size_of_vars_on_stack = 0;
  
  dbprintf("===Pass number:%d qcode_idx:%04X===", pass_number, qcode_idx);
  
  if( pass_number == 1 )
    {
      return;
    }

  dbprintf("===Building qcode header===");
  
  idx = set_qcode_header_byte_at(idx, 2, 0x0000);

  // Size of QCode
  size_of_qcode_idx = idx;
  idx = set_qcode_header_byte_at(idx, 2, 0x0000);

  num_params    = calculate_num_in_class(NOPL_VAR_CLASS_PARAMETER);
  num_globals   = calculate_num_in_class(NOPL_VAR_CLASS_GLOBAL);
  num_externals = calculate_num_in_class(NOPL_VAR_CLASS_EXTERNAL);

  dbprintf("===Num externals: %d===", num_externals);
  dbprintf("===Num parameters:%d===", num_params);
					  
  // Num Parameters
  idx = set_qcode_header_byte_at(idx, 1, num_params);
    
  // Now the types of the parameters
  for(int i=0; i<num_params; i++)
    {
      idx = set_qcode_header_byte_at(idx, 1, get_class_vi_n(NOPL_VAR_CLASS_PARAMETER, ((num_params-1)-i))->type);
    }

  // Now the size of the global table (fixed up later)
  int idx_global_size = idx;
  
  idx = set_qcode_header_byte_at(idx, 2, 0x0000);

  //------------------------------------------------------------------------------
  
  // Now the globals
  int idx_global_start = idx;

  dbprintf("===Num globals:%d===", num_globals);
  dbprintf("**Globals**");
  
  for(int i=0; i<num_globals; i++)
    {
      NOBJ_VAR_INFO *vi;
      vi = get_class_vi_n(NOPL_VAR_CLASS_GLOBAL, i);

      idx = set_qcode_header_string_at(idx, vi->name);
      idx = set_qcode_header_byte_at(idx, 1, vi->type);
      idx = set_qcode_header_byte_at(idx, 2, vi->offset);
    }

  int idx_global_end = idx;
  int global_table_size = idx_global_end-idx_global_start;
  set_qcode_header_byte_at(idx_global_size, 2, global_table_size);

  dbprintf("Global start:%d Global end:%d global_table_size:%d", idx_global_start, idx_global_end, global_table_size);
  //------------------------------------------------------------------------------
  
  // Now Externals and parameters
  // Now the size of the external table (fixed up later)
  int idx_external_size = idx;

  dbprintf("**Externals**");
  
  idx = set_qcode_header_byte_at(idx, 2, 0x0000);

  // Now the externals
  int idx_external_start = idx;

  for(int i=0; i<num_externals; i++)
    {
      NOBJ_VAR_INFO *vi;
      vi = get_class_vi_n(NOPL_VAR_CLASS_EXTERNAL, i);

      idx = set_qcode_header_string_at(idx, vi->name);
      idx = set_qcode_header_byte_at(idx, 1, vi->type);
      //    idx = set_qcode_header_byte_at(idx, 2, vi->offset);
    }

  int idx_external_end = idx;
  set_qcode_header_byte_at(idx_external_size, 2, idx_external_end-idx_external_start);

  // Now the offsets can be calculated
  //
  int first_byte_after_global_table = 2 + global_table_size;
  int first_byte_of_globals = first_byte_after_global_table+num_params*2+num_externals*2;

  dbprintf("===First byte after global table:%04X  first_byte_of_globals:%04X===", first_byte_after_global_table, first_byte_of_globals);

  // First globals
  int var_ptr = first_byte_of_globals;
  int last_v_ptr = 0;
  
  for(int i=0; i<num_var_info; i++)
    {

      // Must be an exact match, case insensitive
      if( (var_info[i].class == NOPL_VAR_CLASS_GLOBAL) )
	{
	  last_v_ptr = var_ptr;
	  var_ptr += size_of_type(&(var_info[i]) );

	  var_info[i].offset = -(var_ptr-data_offset_of_type(&(var_info[i])));
	  //var_info[i].offset = -(var_ptr);
	  dbprintf("%d %s %04X delta:%d offset:%04X", i, var_info[i].name, -var_ptr, var_ptr - last_v_ptr,-(var_ptr-data_offset_of_type(&(var_info[i]))));
	}
    }

  dbprintf("**Rewriting globals**");
  
  // rewrite the globals in order to write the offsets with the correct values
  int idx2 = idx_global_start;

  for(int i=0; i<num_globals; i++)
    {
      NOBJ_VAR_INFO *vi;
      vi = get_class_vi_n(NOPL_VAR_CLASS_GLOBAL, i);

      idx2 = set_qcode_header_string_at(idx2, vi->name);
      idx2 = set_qcode_header_byte_at(idx2, 1, vi->type);
      idx2 = set_qcode_header_byte_at(idx2, 2, vi->offset);
    }

  //------------------------------------------------------------------------------
  //
  // Now the Parameters and external offsets, which are offsets into the indirection
  // table. These are only used in the QCode generation
  
  int ind_ptr = -(first_byte_after_global_table+2);
  
  for(int i=0; i<num_var_info; i++)
    {
      if( (var_info[i].class == NOPL_VAR_CLASS_EXTERNAL) ||
	  (var_info[i].class == NOPL_VAR_CLASS_PARAMETER) )
	{
	  var_info[i].offset = ind_ptr;

	  dbprintf("%d %s offset:%04X", i, var_info[i].name, ind_ptr);

	  ind_ptr -= 2;
	}
    }

  //------------------------------------------------------------------------------
  // Now the Locals
  
  for(int i=0; i<num_var_info; i++)
    {

      // Must be an exact match, case insensitive
      if( var_info[i].class == NOPL_VAR_CLASS_LOCAL )
	{
	  last_v_ptr = var_ptr;
	  var_ptr += size_of_type(&(var_info[i]) );
	  var_info[i].offset = -(var_ptr-data_offset_of_type(&(var_info[i])));

	  //var_info[i].offset = -(var_ptr);
	  dbprintf("%d %s %04X delta:%d", i, var_info[i].name, -var_ptr, var_ptr - last_v_ptr);
	}
    }

  //------------------------------------------------------------------------------
  
  // Now the fixups can be calculated
  // First the string max lengths

  // Length of string fixups, fill in later
  int size_of_string_fixup_idx = idx;

  dbprintf("size of string fixup idx:%04X idx:%04X", size_of_string_fixup_idx, idx);
  
  idx = set_qcode_header_byte_at(idx, 2, 0x0000);
						  
  for(int i=0; i<num_var_info; i++)
    {
      if(
	 ((var_info[i].type == NOBJ_VARTYPE_STR) ||
	  (var_info[i].type == NOBJ_VARTYPE_STRARY)) &&
	 ((var_info[i].class == NOPL_VAR_CLASS_GLOBAL) ||
	  (var_info[i].class == NOPL_VAR_CLASS_LOCAL))
	 )
	{
	  idx = set_qcode_header_byte_at(idx, 2, var_info[i].offset-1);
	  idx = set_qcode_header_byte_at(idx, 1, var_info[i].max_string);
	  dbprintf("%04X %02X", var_info[i].offset-1, var_info[i].max_string);
	}
    }

  int len_string_fixups = idx - size_of_string_fixup_idx - 2;
  set_qcode_header_byte_at(size_of_string_fixup_idx, 2, len_string_fixups);

  dbprintf("Size of string fixups:%02X", len_string_fixups);

  // Now the arry sizes
  // Length of array fixups, fill in later
  int size_of_array_fixup_idx = idx;
  idx = set_qcode_header_byte_at(idx, 2, 0x0000);
						  
  for(int i=0; i<num_var_info; i++)
    {
      if(
	 ((var_info[i].type == NOBJ_VARTYPE_INTARY) ||
	  (var_info[i].type == NOBJ_VARTYPE_FLTARY) ||
	  (var_info[i].type == NOBJ_VARTYPE_STRARY)) &&
	 ((var_info[i].class == NOPL_VAR_CLASS_GLOBAL) ||
	  (var_info[i].class == NOPL_VAR_CLASS_LOCAL))
	 )
	{
	  idx = set_qcode_header_byte_at(idx, 2, var_info[i].offset-0);
	  idx = set_qcode_header_byte_at(idx, 1, (var_info[i].max_array) >> 8);
	  idx = set_qcode_header_byte_at(idx, 1, (var_info[i].max_array) &  0xff);
	  dbprintf("%04X %04X", var_info[i].offset-0, var_info[i].max_array);
	}
    }

  int len_array_fixups = idx - size_of_array_fixup_idx - 2;
  set_qcode_header_byte_at(size_of_array_fixup_idx, 2, len_array_fixups);

  dbprintf("Size of array fixups:%02X", len_array_fixups);

  // Write the size of variables into the header
  size_of_vars_on_stack = var_ptr - first_byte_of_globals;

  dbprintf("Var_ptr:%04X first byte glob:%04X size vars:%04X", var_ptr, first_byte_of_globals, size_of_vars_on_stack);
  set_qcode_header_byte_at(idx_size_of_vars_on_stack, 2, var_ptr);
  
  qcode_header_len = idx;

  // The header is done, we now generate the first two bytes of the QCode, the STOP SIGN
  qcode_start_idx = idx;
  dbprintf("First byte of QCode:%04X", qcode_start_idx);
  
  idx = set_qcode_header_byte_at(idx, 1, QCO_STOP);
  idx = set_qcode_header_byte_at(idx, 1, RTF_SIN);
  
  // Leave the qcode index pointing after the header and stop sine
  qcode_idx = idx;

  dbprintf("After build qcode_idx:%04X", qcode_idx);
}

int print_qch_field(int idx, FILE *fp, char *title, int len)
{
  fprintf(fp, "\n%04X: %s: ", idx, title);

  for(int i=0; i<len; i++)
    {
      fprintf(fp, "%02X", (int)qcode_header[idx++]);
    }
  
  return(idx);
}

int set_qcode_header_byte_at(int idx, int len, int val)
{
  dbprintf("idx:%04X len:%d val:%02X", idx, len, val);
  switch(len)
    {
    case 1:
      qcode_header[idx++] = val;    
      break;

    case 2:
      qcode_header[idx++] = val >>  8;    
      qcode_header[idx++] = val &   0xFF;    
      break;

    default:
      internal_error("Bad length in %s", __FUNCTION__);
      break;
    }
  
  return(idx);
}

// Put string into QCode
int set_qcode_header_string_at(int idx, char *str)
{
  int len = strlen(str);

  dbprintf("len of %s is %ld", str, strlen(str));
  qcode_header[idx++] = len;
  
  for(int i=0; i<len; i++)
    {
      qcode_header[idx++] = *(str++);
    }
  
  return idx;
}

//------------------------------------------------------------------------------


void dump_qcode_data(void)
{
  FILE *fp;
  int idx = 0;
  int num_param, num_global, num_ext;
  
  fp = fopen("qcode_data.txt", "w");

  idx = print_qch_field(idx, fp, "size of the variables on stack", 2);
  idx = print_qch_field(idx, fp, "size of Q code", 2);

  num_param = qcode_header[idx];
  idx = print_qch_field(idx, fp, "number of parameters", 1);

  for(int i=0; i<num_param;i++)
    {
      idx = print_qch_field(idx, fp, "param_type", 1);
    }

  // Globals
  int global_size = (int)(qcode_header[idx]<<8) + (int)(qcode_header[idx+1]);
  printf("\nglobal size:%04X", global_size);
  
  idx = print_qch_field(idx, fp, "size of global area", 2);

  int idx_global_start = idx;

  while((idx-idx_global_start) < global_size)
    {
      int gname_len = qcode_header[idx];

      //fprintf(fp, "idx=%02X", idx);
      idx = print_qch_field(idx, fp, "Len", 1);

      for(int i=0; i<gname_len; i++)
	{
	  idx = print_qch_field(idx, fp, "name", 1);
	}
      
      idx = print_qch_field(idx, fp, "Type", 1);
      idx = print_qch_field(idx, fp, "Offset", 2);
    }

  // Externals
  
  int external_size = (int)(qcode_header[idx]<<8) + (int)(qcode_header[idx+1]);
  printf("\nExternals size:%04X", external_size);
  
  idx = print_qch_field(idx, fp, "size of external area", 2);

  int idx_external_start = idx;

  while((idx-idx_external_start) < external_size)
    {
      int ename_len = qcode_header[idx];

      //fprintf(fp, "idx=%02X", idx);
      idx = print_qch_field(idx, fp, "Len", 1);

      for(int i=0; i<ename_len; i++)
	{
	  idx = print_qch_field(idx, fp, "name", 1);
	}
      
      idx = print_qch_field(idx, fp, "Type", 1);
      //      idx = print_qch_field(idx, fp, "Offset", 2);
    }

  fprintf(fp, "\n\n\nQCode header_len: %04X", qcode_header_len);
  
  for(int i=0; i<qcode_header_len; i++)
    {
      fprintf(fp, "\n%04X:%02X", i, qcode_header[i]);
    }
  
  fclose (fp);

  ////////////////////////////////////////////////////////////////////////////////
  //
  // Binary output
  //
  ////////////////////////////////////////////////////////////////////////////////
  

  FILE *objfp;

  printf("\nWriting binary...");
  printf("\nLength:%04X", qcode_header_len);
  
  objfp = fopen("ob3_nopl.bin", "wb");

  fprintf(objfp, "ORG%c%c%c%c%c", 0x01, 0xce, 0x83, 0x01, 0xca);
  
  for(int i=0; i<qcode_header_len+qcode_len; i++)
    {
      //      fprintf(objfp, "%c", qcode_header[i]);
      fputc(qcode_header[i], objfp);
    }

  fputc(0x00, objfp);
  fputc(0x00, objfp);
  fclose(objfp);
  
}

//------------------------------------------------------------------------------
//

char cln[300];

char *cline_now(int idx)
{
  sprintf(cln, "%s", &(cline[idx]));

  return(cln);
}


////////////////////////////////////////////////////////////////////////////////


char *type_to_str(NOBJ_VARTYPE t)
{
  char *c;
  
  switch(t)
    {
    case NOBJ_VARTYPE_INT:
      c = "Integer";
      break;

    case NOBJ_VARTYPE_FLT:
      c = "Float";
      break;

    case NOBJ_VARTYPE_STR:
      c = "String";
      break;

    case NOBJ_VARTYPE_INTARY:
      c = "Integer array";
      break;

    case NOBJ_VARTYPE_FLTARY:
      c = "Float   array";
      break;

    case NOBJ_VARTYPE_STRARY:
      c = "String  array";
      break;

    case NOBJ_VARTYPE_UNKNOWN:
      c = "Unknown";
      break;

    case NOBJ_VARTYPE_VOID:
      c = "Void";
      break;
      
    default:
      c = "?????";
      break;
    }
  
  return(c);
}

NOBJ_VARTYPE char_to_type(char ch)
{
  NOBJ_VARTYPE ret_t = '?';
  
  switch(ch)
    {
    case 'i':
      ret_t = NOBJ_VARTYPE_INT;
      break;

    case 'f':
      ret_t = NOBJ_VARTYPE_FLT;
      break;

    case 's':
      ret_t = NOBJ_VARTYPE_STR;
      break;

    case 'I':
      ret_t = NOBJ_VARTYPE_INTARY;
      break;

    case 'F':
      ret_t = NOBJ_VARTYPE_FLTARY;
      break;

    case 'S':
      ret_t = NOBJ_VARTYPE_STRARY;
      break;

    case 'v':
      ret_t = NOBJ_VARTYPE_VOID;
      break;

    case 'V':
      ret_t = NOBJ_VARTYPE_VAR_ADDR;
      break;
    }

  return(ret_t);
}

////////////////////////////////////////////////////////////////////////////////

void syntax_error(char *fmt, ...)
{
  va_list valist;
  char line[80];
  
  va_start(valist, fmt);

  vsprintf(line, fmt, valist);
  va_end(valist);

  dbprintf("Syntax error\n");
  dbprintf("'%s'", cline);
  
  for(int i=0; i<cline_i-1; i++)
    {
      dbprintf(" ");
    }
  dbprintf("^");
  dbprintf("\n   %s", line);
    
  printf("\n\n\nSyntax error\n");
  printf("\n%s", cline);
  printf("\n");

  for(int i=0; i<cline_i-1; i++)
    {
      printf(" ");
    }
  printf("^");
  
  printf("\n\n   %s", line);
  printf("\n");
  
  //  exit(-1);
}

void typecheck_error(char *fmt, ...)
{
  va_list valist;
  char line[80];
  
  va_start(valist, fmt);

  vsprintf(line, fmt, valist);
  va_end(valist);

  printf("\n\n\nType check error\n");
  printf("\n%s", cline);
  printf("\n");
  for(int i=0; i<cline_i-1; i++)
    {
      printf(" ");
    }
  printf("^");
  
  printf("\n\n   %s", line);
  printf("\n");
}

//------------------------------------------------------------------------------
//
// Error in the compiler logic
//

void internal_error(char *fmt, ...)
{
  va_list valist;
  char line[80];
  
  va_start(valist, fmt);

  vsprintf(line, fmt, valist);
  va_end(valist);

  printf("\n\n\n*** Internal compiler error ***\n");
  
  printf("\n%s", cline);
  printf("\n");
  for(int i=0; i<cline_i-1; i++)
    {
      printf(" ");
    }
  printf("^");
  
  printf("\n\n   %s", line);
  printf("\n");
  
  //exit(-1);
}

////////////////////////////////////////////////////////////////////////////////

void remove_trailing_newline(int idx)
{
  if( cline[strlen(&(cline[idx]))-1] == '\n' )
    {
      cline[strlen(&(cline[idx]))-1] = ' ';
    }
}

// Finds the next colon or null, and returns what it finds.
// The colon is turned into a null if it exists

int nulled_idx = -1;

int nullify_colon(int idx)
{
  int start_idx = idx;

  dbprintf("Start idx:%d", idx);
  
  while( (cline[idx] != ':') && ( cline[idx] != '\0') )
    {
      idx++;
    }

  switch(cline[idx])
    {
    case ':':
      // We found a colon, turn it to a null and store data to
      // continue on next call
      nulled_idx = idx;
      cline[idx] = '\0';
      break;

    case '\0':
      nulled_idx = -1;
      break;
    }

  dbprintf("Start idx:%d line='%s'", start_idx, &(cline[start_idx]));
  
  return(start_idx);
}

// Get first colon delimited line
// Returns -1 if no line, otherwise index into cline of the line.

int start_get_line(int start_idx)
{
  int retval=-1;
  
  dbprintf("Start idx:%d", start_idx);
  
  remove_trailing_newline(start_idx);
  
  // Find first colon, nullify it, we have a line
  retval = nullify_colon(start_idx);

  dbprintf("line:'%s;", retval);

  return(retval);
}

int get_next_line(void)
{
  
}


////////////////////////////////////////////////////////////////////////////////

// Gets next line. Lines come from the input source, which provides
// composite lines. A composite line is one or more lines delimited by
// colons.
//
// The lines returned by this function are set up within the memory of the
// composite line. The start is the character after the colon, and the
// next colon is turned into a '\0' if necessary.
//
// The cline array holds the current composite line
//

int multi_line_in_progress = 0;
int line_start = 0;

int next_composite_line(FILE *fp)
{
  int all_spaces = 1;

  dbprintf("------------------------------");

  // Do we have a colon delimited line in progress?
  if( multi_line_in_progress )
    {
      // See if we have another delimited line to return
      
    }
  else
    {
      // Read another composite line and return the first delimited line
      if( fgets(cline, MAX_NOPL_LINE, fp) == NULL )
	{
	  cline_i = 0;
	  dbprintf("ret0");
	  return(0);
	}
      
      cline_i = 0;
      line_start = 0;

      // Remove the newline on th eend of the line
      //cline_i = start_get_line(cline_i);
      
      remove_trailing_newline(cline_i);
      
      
      dbprintf("ret1");
      return(1);
    }
}

////////////////////////////////////////////////////////////////////////////////

void drop_space(int *index)
{
  int idx = *index;
  
  while( isspace(cline[idx]) && (cline[idx] != '\0') )
    {
      idx++;
    }
  *index = idx;
  
}

//------------------------------------------------------------------------------

void drop_colon(int *index)
{
  int idx = *index;
  dbprintf("Entry");
  
  if ( check_literal(&idx," :") )
    {
      dbprintf("Dropping colon");
      //      fprintf(chkfp, "  dropping colon");
      *index = idx;
    }
  
  dbprintf("Exit");
}

////////////////////////////////////////////////////////////////////////////////
//
// The '=' in an assignment needs to be a different token to the '='
// of equality

int scan_assignment_equals(void)
{
  char *lit = " =";
  char *origlit = lit;
  OP_STACK_ENTRY op;
  
  indent_more();
  
  init_op_stack_entry(&op);
  
  dbprintf("%s:lit='%s' '%s'", __FUNCTION__, lit, &(cline[cline_i]));
  
  if( *lit == ' ' )
    {
      lit++;
      drop_space(&cline_i);
    }

  dbprintf("%s:After drop space:'%s'", __FUNCTION__, &(cline[cline_i]));

  while( (*lit != '\0') && (*lit != ' ') )
    {
      dbprintf("%s:while loop:%s", __FUNCTION__, &(cline[cline_i]));
      if( cline[cline_i] == '\0' )
	{
	  syntax_error("Bad literal '%s'", origlit);
	  return(0);
	}
      
      if( *lit != cline[cline_i] )
	{
	  // Not a match, fail
	  dbprintf("ret1");  
	  return(0);
	}
      
      lit++;
      cline_i++;
    }
  
  if( *lit == ' ' )
    {
      lit++;
      drop_space(&cline_i);
    }
  
  // reached end of literal string , all ok

  if( *origlit == ' ' )
    {
      strcpy(op.name, origlit+1);
    }
  else
    {
      strcpy(op.name, origlit);
    }

  op.buf_id = EXP_BUFF_ID_OPERATOR;
  strcpy(op.name, ":=");
  process_token(&op);
  
  dbprintf("ret1");  
  return(1);
}

////////////////////////////////////////////////////////////////////////////////
//
// Scan for a literal
// Leading space means drop spaces before looking for the literal,
// trailing means drop spaces after finding it
//
////////////////////////////////////////////////////////////////////////////////

int scan_literal_core(char *lit, OP_STACK_ENTRY *op)
{
  char *origlit = lit;

  indent_more();
  
  init_op_stack_entry(op);
  
  dbprintf("%s:lit='%s' '%s'", __FUNCTION__, lit, &(cline[cline_i]));
  
  if( *lit == ' ' )
    {
      lit++;
      drop_space(&cline_i);
    }

  dbprintf("%s:After drop space:'%s'", __FUNCTION__, &(cline[cline_i]));

  while( (*lit != '\0') && (*lit != ' ') )
    {
      dbprintf("%s:while loop:%s", __FUNCTION__, &(cline[cline_i]));
      if( cline[cline_i] == '\0' )
	{
	  dbprintf("ret0");  
	  syntax_error("Bad literal '%s'", origlit);
	  return(0);
	}
      
      if( toupper(*lit) != toupper(cline[cline_i]) )
	{
	  // Not a match, fail
	  dbprintf("ret0");  
	  return(0);
	}
      
      lit++;
      cline_i++;
    }
  
  if( *lit == ' ' )
    {
      lit++;
      drop_space(&cline_i);
    }
  
  // reached end of literal string , all ok

  if( *origlit == ' ' )
    {
      strcpy(op->name, origlit+1);
    }
  else
    {
      strcpy(op->name, origlit);
    }
}

//------------------------------------------------------------------------------
// Scans for a literal and stores a level with it

int scan_literal_level(char *lit, int level)
{
  OP_STACK_ENTRY op;

  if( scan_literal_core(lit, &op) )
    {
      op.level = level;
      process_token(&op);
      
      dbprintf("ret1");  
      return(1);
    }
  
  return(0);
}

int scan_literal(char *lit)
{
  OP_STACK_ENTRY op;

  if( scan_literal_core(lit, &op) )
    {
      process_token(&op);
      
      dbprintf("ret1");  
      return(1);
    }
  
  return(0);
}

////////////////////////////////////////////////////////////////////////////////

// Check for a literal string.
// Leading space means drop spaces before looking for the literal,
// trailing means drop spaces after finding it

int check_literal(int *index, char *lit)
{
  int idx = *index;
  int orig_idx = *index;
  
  indent_more();
  
  dbprintf("%s:lit='%s' idx=%d '%s'", __FUNCTION__, lit, idx, &(cline[idx]));

  if( *lit == ' ' )
    {
      dbprintf("dropping space");
      lit++;
      drop_space(&idx);
    }

  dbprintf("%s:After drop space:'%s' idx=%d '%s'", __FUNCTION__, lit, idx, &(cline[idx]));

  if( cline[idx] == '\0' )
    {
      dbprintf("%s  ret0 Empty test string", __FUNCTION__);
      *index = idx;
      return(0);
    }
  
  while( (*lit != '\0') && (cline[idx] != '\0'))
    {
      if( toupper(*lit) != toupper(cline[idx]) )
	{
	  dbprintf("  '%c' != '%c'", *lit, cline[idx]);
	  
	  // Not a match, fail
	  dbprintf("%s: ret0", __FUNCTION__);
	  *index = idx;
	  return(0);
	}
      lit++;
      idx++;
    }

  dbprintf("%s:After while():%s", __FUNCTION__, &(cline[idx]));
  
  if( *lit != '\0' )
    {
      *index = idx;

      dbprintf("ret0 Full string not seen");
      return(0);
    }
  
  // reached end of literal string , all ok
  *index = idx;
  dbprintf("ret1 Match. '%' == '%s'", lit, &(cline[orig_idx]));
  return(1);

}

////////////////////////////////////////////////////////////////////////////////
//
//

int is_logfile_char(char ch)
{
  switch(ch)
    {
    case 'A':
    case 'B':
    case 'C':
    case 'D':
      return(1);
      break;
    }

  return(0);
}




////////////////////////////////////////////////////////////////////////////////

// Scans for a variable name string part
int scan_vname(char *vname_dest)
{
  char vname[300];
  int vname_i = 0;
  char ch;

  indent_more();
  dbprintf("%s: '%s'", __FUNCTION__, &(cline[cline_i]));

  drop_space(&cline_i);
  
  if( isalpha(cline[cline_i]) || is_logfile_char(cline[cline_i]) )
    {
      ch = cline[cline_i];
      cline_i++;
      
      vname[vname_i++] = ch;
      
      while( isalnum(cline[cline_i]) || (cline[cline_i] == '.'))
	{
	  ch = cline[cline_i];
	  vname[vname_i++] = ch;
	  cline_i++;
	}

      vname[vname_i] = '\0';
      
      strcpy(vname_dest, vname);
      dbprintf("%s: ret1 '%s'", __FUNCTION__, vname);
      return(1);
    }

  dbprintf("%s: ret0", __FUNCTION__);
  strcpy(vname_dest, "");
  return(0);
}

//------------------------------------------------------------------------------

//
//
// Checks for a variable name string part
//

int check_vname(int *index)
{
  int idx = *index;
  indent_more();
  
  drop_space(&idx);

  dbprintf("%s '%s':", __FUNCTION__, &(cline[idx]));
  
  if( isalpha(cline[idx]) || (cline[idx] == '.'))
    {
      idx++;
      
      while( isalnum(cline[idx]) || (cline[idx] == '.') )
	{
	  idx++;
	}

      dbprintf("%s ret1 '%s':", __FUNCTION__, &(cline[idx]));
      *index = idx;
      return(1);
    }

  dbprintf("%s ret0 '%s':", __FUNCTION__, &(cline[idx]));
  *index = idx;
  return(0);
}

////////////////////////////////////////////////////////////////////////////////
//
// Scan variable
//
// The variable can be a reference, or a declaration.
// Declarations have integers and not expressions in array brackets.
//
// This puts a variable ref in the output stream. handles arrays
// and puts appropriate codes for array indices in stream
//
// The variable name is captured and flags which specify the type.
// The expressions that are array indices aren't captured.
// They will be expressions on the stack and the type flags will ensure an array
// type qcode is set for the variable reference.
//
////////////////////////////////////////////////////////////////////////////////

void set_op_var_type(OP_STACK_ENTRY *op, NOBJ_VAR_INFO *vi)
{
#if 0
  if( vi->is_array )
    {
      if( vi->is_integer )
	{
	  op->type = NOBJ_VARTYPE_INTARY;
	}
      
      if( vi->is_float )
	{
	  op->type = NOBJ_VARTYPE_FLTARY;
	}
      
      if( vi->is_string )
	{
	  op->type = NOBJ_VARTYPE_STRARY;
	}
    }
  else
    {  
      if( vi->is_integer )
	{
	  op->type = NOBJ_VARTYPE_INT;
	}
      
      if( vi->is_float )
	{
	  op->type = NOBJ_VARTYPE_FLT;
	}
      
      if( vi->is_string )
	{
	  op->type = NOBJ_VARTYPE_STR;
	}
    }
#endif
  op->type = vi->type;
}

//------------------------------------------------------------------------------

int scan_variable(NOBJ_VAR_INFO *vi, int ref_ndeclare, NOPL_OP_ACCESS access)
{
  char vname[300];
  char chstr[2];
  int idx = cline_i;
  OP_STACK_ENTRY op;

  init_op_stack_entry(&op);
  indent_more();
  
  dbprintf("'%s' ref_ndeclare:%d", I_WHERE, ref_ndeclare);

  init_var_info(vi);
  vi->is_ref = ref_ndeclare;

  // Default to local, this can be overridden for global later.
  vi->class = NOPL_VAR_CLASS_LOCAL;
  
  chstr[1] = '\0';
  
  if( scan_vname(vname) )
    {
      dbprintf("%s: '%s' vname='%s'", __FUNCTION__, &(cline[cline_i]), vname);
      
      // Could just be a vname
      switch( chstr[0] = cline[cline_i] )
	{
	case '%':
	  vi->type = NOBJ_VARTYPE_INT;
	  strcat(vname, chstr);
	  cline_i++;
	  break;
	  
	case '$':
	  vi->type = NOBJ_VARTYPE_STR;
	  strcat(vname, chstr);
	  cline_i++;
	  break;

	default:
	  vi->type = NOBJ_VARTYPE_FLT;
	  break;
	}

      // Is it a keyword?
      if( token_is_other_keyword(vname) )
	{
	  // Definitely not a variable
	  dbprintf("'%s' is an other keypwod");
	  return(0);
	}
 
      // Is it an array?
      dbprintf("Array test '%s'", I_WHERE);
      idx = cline_i;
      
      if( check_literal(&idx,"(") )
	{
	  if( !scan_literal("(") )
	    {
	      syntax_error("Malformed array index");
	      return(0);
	    }
	  
	  dbprintf("'%s' is array", vname);
	  
	  make_var_type_array(&(vi->type));
	  
	  // Add token to output stream for index or indices

	  if( ref_ndeclare )
	    {
	      int num_subexpr;

	      scan_expression(&num_subexpr, HEED_COMMA);
	      vi->num_indices = num_subexpr+1;
	    }
	  else
	    {
	      (vi->num_indices)++;
	      
	      //scan_integer(&(vi->max_array));
	      if( vi->type == NOBJ_VARTYPE_STRARY )
		{
		  // Strings are arrays only if there are two indices
		  //		  vi->is_array = 0;
		  scan_integer(&(vi->max_string));
		}
	      else
		{
		  scan_integer(&(vi->max_array));
		}
	    }
	  
	  print_var_info(vi);
      
	  // Could be string array, which has two expressions in
	  // the brackets
	  idx = cline_i;
	  if( check_literal(&idx," ,") )
	    {
	      if( vi->type == NOBJ_VARTYPE_STRARY )
		{
		  // All OK, string array
		  //make_var_type_array(&vi->type);
		  
		  scan_literal(" ,");
		  
		  if( ref_ndeclare )
		    {
		      int num_subexpr;
		      // A reference with two indices in brackets is a syntax error
		      syntax_error("Bad string array");
		      return(0);
		       
		      //scan_expression(&num_subexpr, HEED_COMMA);
		      //vi->num_indices = vi->num_indices+1;
		      
		    }
		  else
		    {
		      // Set the values correctly
		      vi->max_array = vi->max_string;
		      scan_integer(&(vi->max_string));
		      (vi->num_indices)++;
		    }
		}
	      else
		{
		  syntax_error("Two indices in non-string variable '%s' of type %s",
			       vi->name,
			       type_to_str(vi->type));
		}
	    }

	  // There's a correction here. If a string array has just one index, then it's
	  // actually a string, not an array, if this is a declaration. For references
	  // string arrays have just one index
	  
	  if( (vi->type == NOBJ_VARTYPE_STRARY) && (!ref_ndeclare) )
	    {
	      if( vi->num_indices == 1 )
		{
		  // the brackets define the length of the string, not an array
		  vi->type = NOBJ_VARTYPE_STR;
		  
		}
	    }
	  
	  if( scan_literal(" )") )
	    {
	      dbprintf("%s:ret1 vname='%s' %s", __FUNCTION__, vname, type_to_str(vi->type) );
	      op.access = access;
	      
	      strcpy(vi->name, vname);
	      strcpy(op.name, vname);
	      set_op_var_type(&op, vi);
	      op.vi = *vi;

	       // Create and open field variables aren't real variables. They need to be handled differently
	      if( access == NOPL_OP_ACCESS_FIELDVAR )
		{ 
		  op.buf_id = EXP_BUFF_ID_FIELDVAR;
		}
	      
	      process_token(&op);

	      if( pass_number == 2,1 )
		{
		  add_var_info(vi);
		}
	      return(1);
	    }
	}
      
      dbprintf("%s:ret1 vname='%s' %s", __FUNCTION__, vname, type_to_str(vi->type) );

      // Use correct access depending on variable access (ref or value)
      // which is really read or write
      op.access = access;
      
      strcpy(vi->name, vname);
      strcpy(op.name, vname);
      set_op_var_type(&op, vi);
      op.vi = *vi;

      // Create and open field variables aren't real variables. They need to be handled differently
      if( access == NOPL_OP_ACCESS_FIELDVAR )
	{
	  op.buf_id = EXP_BUFF_ID_FIELDVAR;
	  op.type = vi->type;
	}
      
      process_token(&op);

      if( pass_number == 2,1 )
	{
	  add_var_info(vi);
	}

      dbprintf("ret1");
      return(1);
    }
  
  dbprintf("%s:ret0", __FUNCTION__);
  return(0);
}


int check_variable(int *index)
{
  int idx = *index;
  int orig_index = *index;
  
  char vname[300];
  char chstr[2];
  int var_is_string  = 0;
  int var_is_integer = 0;
  int var_is_float   = 0;
  int var_is_array   = 0;

  indent_more();
  
  drop_space(&idx);
  
  dbprintf("'%s'", IDX_WHERE);

  vname[0] = '\0';
  chstr[1] = '\0';
  
  if( check_vname(&idx) )
    {
      dbprintf("Name: '%s'", vname);
      
      // Could just be a vname
      switch( chstr[0] = cline[idx] )
	{
	case '%':
	  var_is_integer = 1;
	  strcat(vname, chstr);
	  idx++;
	  break;
	  
	case '$':
	  var_is_string = 1;
	  strcat(vname, chstr);
	  idx++;
	  break;

	default:
	  var_is_float = 1;
	  break;
	}

     
      // Is it an array?
      dbprintf("%s: Ary test '%s'", __FUNCTION__, &(cline[idx]));
      
      if( check_literal(&idx, "(") )
	{
	  dbprintf("%s: is array", __FUNCTION__);
	  
	  var_is_array = 1;
	  
	  // Add token to output stream for index or indices
	  // If it's an empty expression then it's not an expression. This is an
	  // ADDR address reference to an array
	  if( !check_expression_list(&idx) )
	    {
	      // Not a variable reference, probably an ADDR function argument.
	      *index = idx;
	      dbprintf("ret0 Not an expression");
	      return(0);
	    }

#if 0
	  // Could be string array, which has two expressions in
	  // the brackets
	  if( check_literal(&idx," ,") )
	    {
	      if( var_is_string )
		{
		  // All OK, string array
		  if( check_expression(&idx, HEED_COMMA) )
		    {
		      // All OK
		    }
		  else
		    {
		      *index = idx;
		      return(0);
		    }
		}
	      else
		{
		  *index = idx;
		  dbprintf("%s:ret0 ", __FUNCTION__);
		  return(0);
		}
	    }
#endif

	  dbprintf("Expression parsed, checking for )");
	  if( check_literal(&idx, " )") )
	    {
	      dbprintf("%s:ret1 vname='%s' is str:%d int:%d flt:%d ary:%d", __FUNCTION__,
		       vname,
		       var_is_string,
		       var_is_integer,
		       var_is_float,
		       var_is_array
		       );
 	      *index = idx;
	      dbprintf("%s:ret1 ", __FUNCTION__);
	      return(1);
	    }
	}
      
      dbprintf("%s:ret1 vname='%s' is str:%d int:%d flt:%d ary:%d", __FUNCTION__,
	     vname,
	     var_is_string,
	     var_is_integer,
	     var_is_float,
	     var_is_array
	     );
      
      *index = idx;
      dbprintf("%s:ret1 ", __FUNCTION__);
      return(1);
      
    }

  *index = idx;
  dbprintf("%s:ret0 ", __FUNCTION__);
  return(0);
}

int check_variable2(int *index)
{
  int idx = *index;
  indent_more();
  
  if( check_vname(&idx) )
    {
      *index = idx;
      return(1);
    }

  *index = idx;
  return(0);
}

////////////////////////////////////////////////////////////////////////////////

int check_operator(int *index, int *is_comma, int ignore_comma)
{
  int idx = *index;

  indent_more();
  drop_space(&idx);

  dbprintf("'%s' igncomma:%d", cline_now(idx), ignore_comma);
  
#if 0
  if( !ignore_comma )
    {
      if( check_literal(&idx, " ,") )
	{
	  *index = idx;
	  *is_comma = 1;
	  
	  dbprintf("ret1:is comma: %d", *is_comma);
	  return(1);
	}
    }

#endif
  
  for(int i=0; i<NUM_OPERATORS; i++)
    {
      if( strn_match(&(cline[idx]), op_info[i].name, strlen(op_info[i].name)) )
	{
	  // Match
	  *is_comma = 0;

	  dbprintf("ret1:is comma: %d", *is_comma);
	  idx += strlen(op_info[i].name);
	  *index = idx;
	  return(1);
	}
    }

  dbprintf("ret0:is comma: %d", *is_comma);

  *is_comma = 0;
  *index = idx;
  return(0);
}

//------------------------------------------------------------------------------

int scan_operator(int *is_comma, int ignore_comma)
{
  int idx = cline_i;
  OP_STACK_ENTRY op;

  *is_comma = 0;
  
  indent_more();
  
  init_op_stack_entry(&op);
  
  dbprintf("%s: '%s'", __FUNCTION__, cline_now(idx));

  drop_space(&idx);

  cline_i = idx;

  if( !ignore_comma )
    {
      if( check_literal(&idx, " ,") )
	{
	  if(scan_literal(" ,") )
	    {
	      *is_comma = 1;
	      dbprintf("ret1:is comma: %d", *is_comma);
	      return(1);
	    }
	  else
	    {
	      *is_comma = 0;
	      dbprintf("ret0:is comma: %d", *is_comma);
	      return(0);
	    }
	  //      *is_comma = 1;
	  //dbprintf("is comma:%d", is_comma);
	  //return(scan_literal(" ,"));
	}
    }
  
  for(int i=0; i<NUM_OPERATORS; i++)
    {
      if( strn_match(&(cline[cline_i]), op_info[i].name, strlen(op_info[i].name)) )
	{
	  *is_comma = 0;
	  
	  // Match
	  cline_i += strlen(op_info[i].name);
	  dbprintf("%s: ret1 '%s' nb:%d", __FUNCTION__, cline_now(cline_i), op.num_bytes);
	  strcpy(op.name, op_info[i].name);
	  op.buf_id = EXP_BUFF_ID_OPERATOR;
	  process_token(&op);
	  return(1);
	}
    }

  *is_comma = 0;
  dbprintf("%s: ret0 '%s'", __FUNCTION__, cline_now(cline_i));
  return(0);
}

////////////////////////////////////////////////////////////////////////////////

int check_integer(int *index)
{
  int idx = *index;
  int num_digits = 0;
  indent_more();
  
  drop_space(&idx);
  
  dbprintf("%s: '%s'", __FUNCTION__, &(cline[idx]));

  char intval[20];
  char chstr[2];

  chstr[1] = '\0';
  intval[0] = '\0';

#if FLT_INCLUDES_SIGN
  // Can start with '-'
  if(  (chstr[0] = cline[idx]) == '-' )
    {
      strcat(intval, chstr);
      idx++;
    }
#endif
  
  while( isdigit(chstr[0] = cline[idx]) )
    {
      strcat(intval, chstr);
      idx++;
      num_digits++;
    }

  *index = idx;
  
  if( (num_digits > 0) && (num_digits <= 5) )
    {
      dbprintf("ret1");
      return(1);
    }

  dbprintf("%s:ret0", __FUNCTION__);
  return(0);
}

//------------------------------------------------------------------------------
//
// Scan for integer and return it as an integer
//

int scan_integer(int *intdest)
{
  int num_digits = 0;
  OP_STACK_ENTRY op;
  char intval[20];
  char chstr[2];

  indent_more();
  
  init_op_stack_entry(&op);
  
  drop_space(&cline_i);
  
  dbprintf("'%s'", I_WHERE);

  chstr[1] = '\0';
  intval[0] = '\0';

#if FLT_INCLUDES_SIGN
  // Can start with '-'
  if(  (chstr[0] = cline[cline_i]) == '-' )
    {
      strcat(intval, chstr);
      cline_i++;
    }
#endif
  
  while( isdigit(chstr[0] = cline[cline_i]) )
    {
      strcat(intval, chstr);
      cline_i++;
      num_digits++;
    }

  // Convert to integer
  sscanf(intval, "%d", intdest);
  //x  strcpy(intdest, intval);
  
  if( (num_digits > 0) && (num_digits <=5) )
    {
      dbprintf("ret1");

      strcpy(op.name, intval);

      op.integer = *intdest;
      op.type = NOBJ_VARTYPE_INT;
      process_token(&op);

      dbprintf("ret1  num_dig:%d intval='%s'", num_digits, intval);

      return(1);
    }

  dbprintf("%s:ret0", __FUNCTION__);
  return(0);
}

int isfloatdigit(char c)
{
  dbprintf("%s:", __FUNCTION__);
  if( isdigit(c) || (c == '.') || (c == 'E')
#if FLT_INCLUDES_SIGN
      || (c == '-')
#endif
      )
    {
      return(1);
    }
  
  return(0);
}

//------------------------------------------------------------------------------

int check_float(int *index)
{
  int idx = *index;
  char chstr[2];
  int decimal_present = 0;
  int num_digits = 0;
  char fltval[20];
  double float_val = 0.0;
  
  indent_more();
  
  dbprintf("%s:", __FUNCTION__);
  
  drop_space(&idx);
  
  fltval[0] = '\0';
  chstr[1] = '\0';

  #if FLT_INCLUDES_SIGN
  // Can start with '-'
  if(  (chstr[0] = cline[idx]) == '-' )
    {
      strcat(fltval, chstr);
      idx++;
    }
#endif
  
  while( isfloatdigit(chstr[0] = cline[idx]) )
    {
      if( chstr[0] == '.' )
	{
	  decimal_present = 1;
	}
      
      strcat(fltval, chstr);
      idx++;
      num_digits++;
    }

  sscanf(fltval, "%le", &float_val);

  dbprintf("Fltval:'%s' float:%le", fltval, float_val);
  
  if( ((num_digits > 0) &&  decimal_present) ||
      ((!decimal_present) && (float_val > 65535.0)) )
    {
      dbprintf("%s: ret1", __FUNCTION__);
      *index = idx;
      return(1);
    }
  
  dbprintf("%s: ret0", __FUNCTION__);

  return(0);
}

//------------------------------------------------------------------------------

// Any number with a decimal point in it is a float.
// The Psion also allows any number without a decimal point to be a float if it won't
// fit in a word as an integer.
// The scanning and checking first checks for an integer, if it won't fit then the float
// is scanned or checked for. That has to accept these larger nunbers.

int scan_float(char *fltdest)
{

  char fltval[20];
  char chstr[2];
  int decimal_present = 0;
  int num_digits = 0;
  OP_STACK_ENTRY op;
  double float_val = 0.0;

  indent_more();
  
  dbprintf("%s:", __FUNCTION__);
  init_op_stack_entry(&op);
  
  drop_space(&cline_i);
  
  fltval[0] = '\0';
  chstr[1] = '\0';

#if FLT_INCLUDES_SIGN
  // Can start with '-'
  if(  (chstr[0] = cline[cline_i]) == '-' )
    {
      strcat(fltval, chstr);
      cline_i++;
    }
#endif
  
  while( isfloatdigit(chstr[0] = cline[cline_i]) )
    {
      if( chstr[0] == '.' )
	{
	  decimal_present = 1;
	}
      
      strcat(fltval, chstr);
      cline_i++;
      num_digits++;
    }

  sscanf(fltval, "%le", &float_val);
  strcpy(fltdest, fltval);
  dbprintf("Fltval:'%s' float:%le", fltval, float_val);
  
  //  if( (num_digits > 0) &&  decimal_present )
  if( ((num_digits > 0) &&  decimal_present) ||
      ((!decimal_present) && (float_val > 65535.0)) )
    {
      dbprintf("%s: ret1", __FUNCTION__);
      strcpy(op.name, fltval);
      op.buf_id = EXP_BUFF_ID_FLT;
      process_token(&op);
      return(1);
    }
  
  dbprintf("%s: ret0", __FUNCTION__);
  return(0);
  
}

////////////////////////////////////////////////////////////////////////////////

int check_hex(int *index)
{
  int idx = *index;
  int num_digits = 0;
  indent_more();
  
  drop_space(&idx);
  
  dbprintf("%s: '%s'", __FUNCTION__, &(cline[idx]));

  char intval[20];
  char chstr[2];

  intval[0] = '\0';

  // We must have a '$' at the start, or a '-$'
  
  if( ((cline[idx]) == '$') )
    {
      // All OK, positive
    }
  else
    {
      dbprintf("%s:ret0 not '$'", __FUNCTION__);
      return(0);
    }
  
  idx++;
  while( isxdigit(chstr[0] = cline[idx]) )
    {
      strcat(intval, chstr);
      idx++;
      num_digits++;
    }

  *index = idx;
  
  if( num_digits > 0 )
    {
      dbprintf("ret1");
      return(1);
    }

  dbprintf("%s:ret0 no digits", __FUNCTION__);
  return(0);
}

//------------------------------------------------------------------------------

int scan_hex(int *intdest)
{
  int num_digits = 0;
  OP_STACK_ENTRY op;
  char intval[20];
  char chstr[2];
  
  indent_more();
  
  init_op_stack_entry(&op);
  
  drop_space(&cline_i);
  
  dbprintf("%s:", __FUNCTION__);

  intval[0] = '\0';

  // We must have a '$' at the start
  if( cline[cline_i] != '$' )
    {
      dbprintf("%s:ret0", __FUNCTION__);
      return(0);
    }
  
  cline_i++;

  while( isxdigit(chstr[0] = cline[cline_i]) )
    {
      strcat(intval, chstr);
      cline_i++;
      num_digits++;
    }

  // Convert to integer
  sscanf(intval, "%x", intdest);
  //x  strcpy(intdest, intval);
  
  if( num_digits > 0 )
    {
      dbprintf("ret1");

      sprintf(intval, "%d", *intdest);
      strcpy(op.name, intval);

      op.integer = *intdest;
      op.type = NOBJ_VARTYPE_INT;
      process_token(&op);

      dbprintf("%s:ret1  %s", __FUNCTION__, intval);

      return(1);
    }

  dbprintf("%s:ret0", __FUNCTION__);
  return(0);
}

//------------------------------------------------------------------------------
//
// Check integer before float.
// If we have an integer that is greater than will fit in a word, then we fail it
// as an integer and allow it to be a float constant. this is what the Psion appesrs to do.

int check_number(int *index)
{
  int idx = *index;
  indent_more();
  
  dbprintf("%s:", __FUNCTION__);

  drop_space(&idx);
	     
  if( check_float(&idx) )
    {
      dbprintf("%s: ret1", __FUNCTION__);
      *index = idx;
      return(1);
    }

  if( check_integer(&idx) )
    {
      dbprintf("%s: ret1", __FUNCTION__);
      *index = idx;
      return(1);
    }
  
  if( check_hex(&idx) )
    {
      dbprintf("%s: ret1", __FUNCTION__);
      
      *index = idx;
      return(1);
    }

  dbprintf("%s: ret0", __FUNCTION__);
  return(0);
}

//------------------------------------------------------------------------------

int scan_number(void)
{
  int idx = cline_i;
  char fltval[40];

  indent_more();
  
  dbprintf("%s:", __FUNCTION__);
  
  drop_space(&cline_i);

  
  idx = cline_i;
  if( check_float(&idx) )
    {
      scan_float(fltval);
      return(1);
    }
  
  idx = cline_i;
  if( check_integer(&idx) )
    {
      int intval;
      scan_integer(&intval);
      return(1);
    }
  
  idx = cline_i;
  if( check_hex(&idx) )
    {
      int intval;
      scan_hex(&intval);
      return(1);
    }

  syntax_error("Not a number");
  return(0);
}

////////////////////////////////////////////////////////////////////////////////

int check_sub_expr(int *index)
{
  int idx = *index;

  indent_more();
  
  dbprintf("%s:", __FUNCTION__);
  
  if( check_literal(&idx," (") )
    {
      if( check_expression(&idx, IGNORE_COMMA) )
	{
	    if( check_literal(&idx," )") )
	      {
		dbprintf("%s: ret1", __FUNCTION__);
		*index = idx;
		return(1);
	      }
	    else
	      {
		dbprintf("ret0: Expected ')'");
	      }
	}
      else
	{
	  dbprintf("ret0: Expected expression");
	}
      
    }
  else
    {
      dbprintf("ret0: No '('");
      return(0);
    }
  
  *index = idx;
  return(0);
}

//------------------------------------------------------------------------------

int scan_sub_expr(void)
{
  int num_subexpr;
  
  OP_STACK_ENTRY op;
  indent_more();
  
  init_op_stack_entry(&op);
  
  dbprintf("%s:", __FUNCTION__);
  if( scan_literal(" (") )
    {
      //      strcpy(op.name, "(");
      // process_token(&op);

      dbprintf("Before scan expression");
      if( scan_expression(&num_subexpr, HEED_COMMA) )
	{
	  dbprintf("scan expr ok, scanning for )");
	  if( scan_literal(" )") )
	    {
	      //     strcpy(op.name, ")");
	      //process_token(&op);
	      
	      dbprintf("ret1");
	      return(1);
	    }
	}
    }

  dbprintf("%s:ret0", __FUNCTION__);
  syntax_error("Expression error");
  return(0);
}

////////////////////////////////////////////////////////////////////////////////

int check_atom(int *index)
{
  int idx = *index;
  indent_more();
  
  dbprintf("%s:", __FUNCTION__);

  dbprintf("%s:Checking for character constant", __FUNCTION__);
  
  idx = *index;
  if( check_literal(&idx," %") )
    {
      // Character code
      if( (cline[idx] != '\0') && (cline[idx] != ' ') )
	{
	  idx++; 
	  *index = idx;
	  dbprintf("ret1");
	  return(1);
	}
    }

  dbprintf("%s:Checking for string", __FUNCTION__);
  idx = *index;
  if( check_literal(&idx," \"") )
    {
      // String
      // Skip past string data to trailing double quote
      while( cline[idx] != '\0' )
	{
	  if( cline[idx] == '\"' )
	    {
	      // Found end of string, skip the quotes

	      break;
	    }
	  idx++;
	}

      // If at end of string then fail, no trailing quote
      if( cline[idx] == '\0' )
	{
	  *index = idx;
	  dbprintf("%s:ret0 No quote at end of string", __FUNCTION__);
	  return(0);
	}

      idx++;
      *index = idx;
      dbprintf("ret1");
      return(1);
    }

  idx = *index;
  if( check_number(&idx) )
    {
      // Int or float
      *index = idx;
      dbprintf("ret1");
      return(1);
    }

  idx = *index;
  if( check_proc_call(&idx) )
    {
      dbprintf("ret1");
      
      *index = idx;
      return(1);
    }

  idx = *index;
  if( check_variable(&idx) )
    {
      // Variable
      *index = idx;
      dbprintf("ret1");
      return(1);
    }

  *index = idx;
  dbprintf("%s:ret0", __FUNCTION__);
  return(0);
}

//------------------------------------------------------------------------------
// Scan a string and output a token which is the string
//
// Double quotes can be included in the string by using ""
//

int scan_string(void)
{
  char chstr[2];
  char strval[300];
  OP_STACK_ENTRY op;
  indent_more();
  
  init_op_stack_entry(&op);
  
  dbprintf("%s:", __FUNCTION__);

  strval[0] = '\0';
  chstr[1] = '\0';
  
  if( check_literal(&cline_i, " \"") )
    {
      dbprintf("  (in if) '%s'", &(cline[cline_i]));
      
      while( cline[cline_i] != '\0' )
	{
	  chstr[0] = cline[cline_i];
	  
	  if( cline[cline_i] == '"' )
	    {
	      // Check for double double quotes
	      if( cline[cline_i+1] == '"' )
		{
		  cline_i++;
		  cline_i++;
		  strcat(strval, "\"");
		  dbprintf("  (in wh) '%s'", &(cline[cline_i]));
		}
	      else
		{
		  // End of string
		  cline_i++;
		  dbprintf("%s: ret1", __FUNCTION__);
		  strcpy(op.name, "\"");
		  strcat(op.name, strval);
		  strcat(op.name, "\"");
		  process_token(&op);
		  return(1);
		}
	    }
	  else
	    {
	      dbprintf("  (in wh) '%s'", &(cline[cline_i]));
	      strcat(strval, chstr);
	      cline_i++;
	      dbprintf("  (in wh) '%s'", &(cline[cline_i]));
	    }
	}
    }
  syntax_error("Bad string");
  dbprintf("ret1");
  return(0);
}

////////////////////////////////////////////////////////////////////////////////

int scan_character(void)
{
  if( (cline[cline_i] != '\0') && (cline[cline_i] != ' ') )
    {
      return(1);
    }
  return(0);
}

int scan_atom(void)
{
  int idx = cline_i;
  NOBJ_VAR_INFO vi;

  indent_more();
  
  init_var_info(&vi);
  dbprintf("%s:", __FUNCTION__);

  idx = cline_i;
  if( check_literal(&idx," %") )
    {
      if( scan_literal(" %" ) )
	{
	  // Hexadecimal number
	  if(scan_character())
	    {
	      dbprintf("ret1");
	      return(1);
	    }
	  else
	    {
	      dbprintf("%s:ret0", __FUNCTION__);
	      return(0);
	    }
	}
    }

  idx = cline_i;
  if( check_literal(&idx," \"") )
    {
      // String
      if(scan_string())
	{
	  dbprintf("ret1");
	  return(1);
	}
      else
	{
	  dbprintf("%s:ret0", __FUNCTION__);
	  return(0);
	}
    }

  idx = cline_i;
  if( check_number(&idx) )
    {
      // Int or float
      if(scan_number())
	{
	  dbprintf("ret1");
	  return(1);
	}
      else
	{
	  dbprintf("%s:ret0", __FUNCTION__);
	  return(0);
	}
    }

  idx = cline_i;
  if( check_proc_call(&idx) )
    {
      if(scan_proc_call())
	{
	  dbprintf("ret1 proc call");
	  return(1);
	}
      else
	{
	  dbprintf("ret0 proc cll");
	  return(0);
	}
    }

  idx = cline_i;
  if( check_variable(&idx) )
    {
      // Variable
      init_var_info(&vi);
      if(scan_variable(&vi, VAR_REF, NOPL_OP_ACCESS_READ))
	{
	  print_var_info(&vi);
	  dbprintf("ret1");
	  return(1);
	}
      
      dbprintf("%s:ret0", __FUNCTION__);
      return(0);
    }

  syntax_error("Not an atom");
  dbprintf("%s:ret0", __FUNCTION__);
  return(0);
}

////////////////////////////////////////////////////////////////////////////////

int check_eitem(int *index, int *is_comma, int ignore_comma)
{
  int idx = *index;

  *is_comma = 0;
  indent_more();
  
  dbprintf("%s: '%s'", __FUNCTION__, &(cline[idx]));

  idx = *index;
  if( check_function(&idx) )
    {
      *index = idx;
      dbprintf("ret1");
      return(1);
    }

  idx = *index;
  if( check_atom(&idx) )
    {
      *index = idx;
      dbprintf("ret1");
      return(1);
    }
  
  idx = *index;
  if( check_sub_expr(&idx) )
    {
      *index = idx;
      dbprintf("ret1");
      return(1);
    }

  idx = *index;
  if( check_addr_name(&idx) )
    {
      *index = idx;
      dbprintf("ret1");
      return(1);
    }
  
  idx = *index;
  dbprintf("%s:ret0", __FUNCTION__);
  return(0);
}

//------------------------------------------------------------------------------

int scan_eitem(int *num_commas, int ignore_comma)
{
  int is_comma = 0;
  int idx = cline_i;
  char fnval[40];
  OP_STACK_ENTRY op;
  
  indent_more();
  
  init_op_stack_entry(&op);
  
  dbprintf("%s:", __FUNCTION__);

  idx = cline_i;
  if( check_function(&idx) )
    {
      *num_commas = 0;
      return(scan_function(fnval) );
    }

  idx = cline_i;
  if( check_atom(&idx) )
    {
      *num_commas = 0;
      return(scan_atom());
    }
  
  idx = cline_i;
  if( check_sub_expr(&idx) )
    {
      *num_commas = 0;
      return(scan_sub_expr());
    }

  idx = cline_i;
  if( check_addr_name(&idx) )
    {
      *num_commas = 0;
      return(scan_addr_name());
    }
  
  idx = cline_i;
  syntax_error("Not an atom");
  *num_commas = 0;
  return(0);
}


////////////////////////////////////////////////////////////////////////////////
//
//
//

int check_expression(int *index, int ignore_comma)
{
  int idx = *index;
  int n_commas = 0;
  int n_commas2 = 0;
  int is_comma;
  
  indent_more();
  
  dbprintf("'%s'", &(cline[idx]));

  drop_space(&idx);
  
  //cline_i = idx;

  if( check_eitem(&idx, &is_comma, ignore_comma) )
    {
      // All OK
    }
  else
    {
      // We allow an operator at the start, for unary operators
      if( check_operator(&idx, &is_comma, ignore_comma) )
	{
	  // All OK, after the unary operator there should be an eitem, we scan for it
	  // here so the while() below works on the foloowing operator
	  if( check_eitem(&idx, &n_commas2, ignore_comma) )
	    {
	      n_commas += n_commas2;
	      dbprintf("n commas now:%d", n_commas);
	      
	      // All OK
	    }
	  else
	    {
	      syntax_error("Expression error, expecting term");
	      dbprintf("ret0 '%s'", &(cline[cline_i]));
	      return(0);
	    }
	}
      else
	{
	  // We allow an empty expression when scanning but not when checking
	  dbprintf("ret0 '%s'", &(cline[idx]));
	  return(0);
	}
    }

  // Now perform
  //
  // (<operator> <eitem>)+
  //

  //idx = cline_i;
  
  while( check_operator(&idx, &is_comma, ignore_comma) )
    {
      // We allow any number of operstors between eitems (for unary operstors)
      while( check_operator(&idx, &is_comma, ignore_comma) )
	{
	}
      
      if( check_eitem(&idx, &n_commas2, ignore_comma) )
	{
	  n_commas += n_commas2;
	  dbprintf("n commas now:%d", n_commas);

	  // All OK
	}
      else
	{
	  syntax_error("Expression error, expecting term");
	  dbprintf("ret0 '%s'", &(cline[idx]));
	  *index = idx;
	  return(0);
	}
      
      //idx = cline_i;
    }

  //  *num_commas = n_commas;
  *index = idx;
  dbprintf("ret1 '%s' commas:%d", &(cline[idx]), n_commas);
  return(1);
}

////////////////////////////////////////////////////////////////////////////////
//
//

int scan_expression(int *num_commas, int ignore_comma)
{
  int idx = cline_i;
  int n_commas = 0;
  int n_commas2 = 0;
  int is_comma;
  
  indent_more();
  
  dbprintf("'%s' igncomma:%d", &(cline[idx]), ignore_comma);

  drop_space(&idx);
  cline_i = idx;

  if( check_eitem(&idx, &n_commas2, ignore_comma) )
    {
      
      if( scan_eitem(&n_commas2, ignore_comma) )
	{
	  // All OK
	}
    }
  else
    {
      // We allow an operator at the start, for unary operators
      if( check_operator(&idx, &is_comma, ignore_comma) )
	{
	  // All OK
	  if( scan_operator(&is_comma, ignore_comma) )
	    {
	      // All OK, after the unary operator there should be an eitem, we scan for it
	      // here so the while() below works on the foloowing operator
	      if( scan_eitem(&n_commas2, ignore_comma) )
		{
		  n_commas += n_commas2;
		  dbprintf("n commas now:%d", n_commas);
		  
		  // All OK
		}
	      else
		{
		  syntax_error("Expression error, expecting term");
		  dbprintf("ret0 '%s'", &(cline[cline_i]));
		  return(0);
		}
	    }
	  else
	    {
	      syntax_error("Expression error, expecting operator");
	      dbprintf("ret0 '%s'", &(cline[cline_i]));
	      return(0);
	    }
	}
      else
	{
	  // We allow an empty expression when scanning but not when checking
	  dbprintf("ret1 '%s'", &(cline[cline_i]));
	  return(1);
	}
    }
  
  // Now perform
  //
  // (<operator> <eitem>)+
  //

  idx = cline_i;
  int idx2 = idx;
  
  dbprintf("'%s' Before while igncomma:%d", &(cline[idx]), ignore_comma);
  while( check_operator(&idx, &is_comma, ignore_comma) )
    {
      idx = idx2;
      
      // We allow any number of operstors between eitems, for unary operstors
      while( check_operator(&idx, &is_comma, ignore_comma) )
	{
	  if( scan_operator(&is_comma, ignore_comma) )
	    {
	      // All OK
	      // After an operator, we can have an operator (unary)
	      idx = cline_i;
	    }
	  else
	    {
	      syntax_error("Expression error, expecting operator");
	      dbprintf("ret0 '%s'", &(cline[cline_i]));
	      return(0);
	    }
	  
	}
      
      dbprintf("'%s' Before scan op igncomma:%d", &(cline[idx]), ignore_comma);
#if 0
      if( scan_operator(&is_comma, ignore_comma) )
	{
	  // All OK
	  // After an operator, we can have an operator (unary)
	  idx = cline_i;
	}
      else
	{
	  syntax_error("Expression error, expecting operator");
	  dbprintf("ret0 '%s'", &(cline[cline_i]));
	  return(0);
	}
#endif
      
      dbprintf("'%s' Before scan_eitem igncomma:%d", &(cline[idx]), ignore_comma);
      if( scan_eitem(&n_commas2, ignore_comma) )
	{
	  n_commas += n_commas2;
	  dbprintf("n commas now:%d", n_commas);

	  // All OK
	}
      else
	{
	  syntax_error("Expression error, expecting term");
	  dbprintf("ret0 '%s'", &(cline[cline_i]));
	  return(0);
	}
      
      idx = cline_i;
    }

  *num_commas = n_commas;
  dbprintf("ret1 '%s' commas:%d", &(cline[cline_i]), n_commas);
  return(1);
}

////////////////////////////////////////////////////////////////////////////////
//

// We do not scan on/off as we want to map them to 1/0
// before sending to the output stream.

int check_onoff(int *index, int *onoff_val)
{
  int idx = *index;
  int orig_idx = *index;
  
  indent_more();
  
  dbprintf("%s: '%s'", __FUNCTION__, &(cline[cline_i]));

  idx = orig_idx;
  if( check_literal(&idx, " ON") )
    {
      *onoff_val = 1;
      *index = idx;
      dbprintf("ret1");
      return(1);
    }

  idx = orig_idx;
  if( check_literal(&idx, " OFF") )
    {
      *onoff_val = 0;
      *index = idx;
      dbprintf("ret1");
      return(1);
    }

  dbprintf("%s:ret0", __FUNCTION__);
  return(0);
  
}

////////////////////////////////////////////////////////////////////////////////
//
// An expression list is a list of comma delimited expressions

int check_expression_list(int *index)
{
  int idx = *index;
  indent_more();
  
  dbprintf("%s: '%s'", __FUNCTION__, &(cline[idx]));

  if( check_expression(&idx, HEED_COMMA) )
    {
      while( check_literal(&idx, " ,") )
	{
	  if( !check_expression(&idx, HEED_COMMA) )
	    {
	      // Should have had an expression after the comma
	      dbprintf("ret0 No expression after comma '%s'", &(cline[idx]));
	      *index = idx;
	      return(0);
	    }
	}

      // All OK
      dbprintf("ret1 '%s'", &(cline[idx]));
      *index = idx;
      return(1);
    }

  // Bad
  dbprintf("ret0 No initial expression '%s'", &(cline[idx]));
  *index = idx;
  return(0);

}

//------------------------------------------------------------------------------

int scan_expression_list(int *num_expressions)
{
  int is_comma = 0;
  int idx = cline_i;
  char fnval[40];
  OP_STACK_ENTRY op;
  int num_commas = 0;

  *num_expressions = 0;
  
  indent_more();
  
  init_op_stack_entry(&op);
  
  dbprintf("%s:", __FUNCTION__);

  if( scan_expression(&num_commas, HEED_COMMA) )
    {
      idx = cline_i;
      (*num_expressions)++;
	  
      while( check_literal(&idx, " ,") )
	{
	  scan_literal(" ,");

	  
	  if( !scan_expression(&idx, HEED_COMMA) )
	    {
	      // Should have had an expression after the comma
	      dbprintf("ret0 No expression after comma '%s'", &(cline[idx]));
	      return(0);
	    }

	  (*num_expressions)++;
	  // Set up idx for next iteration
	  idx = cline_i;
	}

      // All OK
      dbprintf("ret1 '%s'", &(cline[idx]));
      return(1);
    }

  // Bad
  dbprintf("ret0 No initial expression '%s'", &(cline[idx]));
  return(0);
}

////////////////////////////////////////////////////////////////////////////////
//

int scan_logical_file(void)
{
  OP_STACK_ENTRY op;
  int idx = cline_i;
  
  indent_more();
  
  init_op_stack_entry(&op);
  
  dbprintf("%s:", __FUNCTION__);
  
  idx = cline_i;
  
  if( check_literal(&idx, " A") )
    {
      cline_i = idx;
      op.buf_id = EXP_BUFF_ID_LOGICALFILE;
      strcpy(op.name, "A");
      process_token(&op);
      dbprintf("ret1:");
      return(1);
    }

  idx = cline_i;
  
  if( check_literal(&idx, " B") )
    {
      cline_i = idx;
      op.buf_id = EXP_BUFF_ID_LOGICALFILE;
      strcpy(op.name, "B");
      process_token(&op);
      dbprintf("ret1:");
      return(1);
    }

  idx = cline_i;
  
  if( check_literal(&idx, " C") )
    {
      cline_i = idx;
      op.buf_id = EXP_BUFF_ID_LOGICALFILE;
      strcpy(op.name, "C");
      process_token(&op);
      dbprintf("ret1:");
      return(1);
    }

  idx = cline_i;
  
  if( check_literal(&idx, " D") )
    {
      cline_i = idx;
      op.buf_id = EXP_BUFF_ID_LOGICALFILE;
      strcpy(op.name, "D");
      process_token(&op);
      dbprintf("ret1:");
      return(1);
    }

  dbprintf("ret0:");
  return(0);

}

////////////////////////////////////////////////////////////////////////////////
//
// Scan list for CREATE or OPEN.
// this is a list like an expression list, except that:
// the first element is a string expression (file name),
// the second is a logical file name
// and the rest are variable names
//
// We also want to put the CREATE or OPEN after the file name so insert the token there

int scan_createopen_list(char *keyword, int create_nopen)
{
  int is_comma = 0;
  int idx = cline_i;
  char fnval[40];
  OP_STACK_ENTRY op;
  int num_commas = 0;
  NOBJ_VAR_INFO *srch_vi;
  NOBJ_VAR_INFO vi;
  
  indent_more();
  
  init_op_stack_entry(&op);
  
  dbprintf("%s:", __FUNCTION__);

  // File name first
   if( scan_expression(&num_commas, HEED_COMMA) )
     {
       // All OK if this is a string

       idx = cline_i;
       if( check_literal(&idx, " ,") )
	 {
	   scan_literal(" ,");
	 }
       
     }
   else
     {
       dbprintf("ret0: Not an expression for file name");
       return(0);
     }
   
   
   // We now put the command into the output
   op.buf_id = EXP_BUFF_ID_META;
   strcpy(op.name, keyword);
   process_token(&op);
  
   // logical file next
   if( scan_logical_file() )
     {
       // All OK if this is a string
       idx = cline_i;
       if( check_literal(&idx, " ,") )
	 {
	   scan_literal(" ,");
	 }
       
     }
   else
     {
       dbprintf("ret0: Not a logical file");
       return(0);
     }

   while( check_variable(&idx) )
     {
       init_var_info(&vi);
       
       if( scan_variable(&vi, VAR_FIELD, NOPL_OP_ACCESS_FIELDVAR))
	 {
	   // Find the variable and update to make it a local or global
	   srch_vi = find_var_info(vi.name);
	   
	   if( srch_vi != NULL )
	     {
	       srch_vi->class = create_nopen?NOPL_VAR_CLASS_CREATE:NOPL_VAR_CLASS_OPEN;
	       srch_vi->is_ref = 0;
	       
	       dbprintf("%s variable:'%s'", keyword, vi.name);
	       print_var_info(&vi);
	       
	       // Store info about the variable
	       //add_var_info(&vi);
	     }
	 }
       
       idx = cline_i;
       if( check_literal(&idx, " ,") )
	 {
	   scan_literal(" ,");
	 }
     }
   
   drop_space(&cline_i);
   
   if( cline[cline_i] == '\0' )
     {
#if 0
       // Store info about the variable
       var_info[num_var_info] = vi;
       num_var_info++;
#endif
       dbprintf("ret1");
       return(1);
     }
   
   dbprintf("ret0");
   return(0);
}

//////////////////////////////////////////////////////////////////////////////

char *addr_name_suffix[] =
  {
   "%()",
   "$()",
   "%",
   "$",
  };

#define NUM_ADDR_SUFFIX (sizeof(addr_name_suffix)/sizeof(char *))


int check_addr_name(int *index)
{
  int idx = *index;
  
  indent_more();
  
  dbprintf("%s: '%s'", __FUNCTION__, &(cline[idx]));

  if( check_vname(&idx) )
    {
      // Now add any of the suffixes we allow
      for(int i=0; i<NUM_ADDR_SUFFIX; i++)
	{
	  if( check_literal(&idx, addr_name_suffix[i]) )
	    {
	      *index = idx;
	      dbprintf("ret1");  
	      return(1);
	    }
	}
    }

  
  dbprintf("%s: ret0", __FUNCTION__);
  return(0);
}

//------------------------------------------------------------------------------

int scan_addr_name(void)
{
  char vname[NOBJ_VARNAME_MAXLEN+1];
  int idx;
  OP_STACK_ENTRY op;
  
  indent_more();
  
  init_op_stack_entry(&op);
  
  dbprintf("%s: '%s'", __FUNCTION__, &(cline[cline_i]));

  if( scan_vname(vname) )
    {
      // Now add any of the suffixes we allow
      for(int i=0; i<NUM_ADDR_SUFFIX; i++)
	{
	  idx = cline_i;
	  if( check_literal(&idx, addr_name_suffix[i]) )
	    {
	      strcat(vname, addr_name_suffix[i]);
	      strcpy(op.name, vname);
	      op.type = NOBJ_VARTYPE_VAR_ADDR;
	      op.buf_id = EXP_BUFF_ID_VAR_ADDR_NAME;
	      process_token(&op);

	      // Move index on
	      cline_i = idx;
	      dbprintf("ret1");  
	      return(1);
	    }
	}
    }

  
  dbprintf("%s: ret0", __FUNCTION__);
  return(0);
}

////////////////////////////////////////////////////////////////////////////////
//
// Case insensitive string compare
//

int strn_match(char *s1, char *s2, int n)
{
  int match = 1;
  int i = 0;

  if( (strlen(s1)==0) || (strlen(s2) == 0) )
    {
      return(0);
    }

  //dbprintf("n:%d", n);
  
  while( (i < n) )
    {
      //dbprintf("i:%d", i);

      if( *s1 == '\0' )
	{
	  match = 0;
	  break;
	}
      
      if( *s2 == '\0' )
	{
	  break;
	}

      //dbprintf("%c ?? %c", *s1, *s2);
      
      if( toupper(*s1) != toupper(*s2) )
	{
	  match = 0;
	  break;
	}
      
      s1++;
      s2++;
      i++;
    }

  return(match);
}

////////////////////////////////////////////////////////////////////////////////
//
// Command parsing
//
// We have an table of commands which also has various parse fields for each
// command. There's direct parsing of arguments, for instane on/off vs
// expressions after the command name.
//

int check_command(int *index)
{
  int idx = *index;
  indent_more();
  
  drop_space(&idx);
  
  dbprintf("%s:", __FUNCTION__);
  
  for(int i=0; i<NUM_FUNCTIONS; i++)
    {
      //dbprintf("Checking '%s' against '%s'", &(cline[idx]), fn_info[i].name);
      //printf("\n%s", fn_info[i].name);
      if( fn_info[i].command && (strn_match(&(cline[idx]), fn_info[i].name, strlen(fn_info[i].name))) )
	{
	  
	  // Match
	  dbprintf("%s: ret1 found=> '%s'", __FUNCTION__, fn_info[i].name);
	  *index = idx + strlen(fn_info[i].name);
	  return(1);
	}
    }
  dbprintf("%s: ret0", __FUNCTION__);
  return(0);
}

//------------------------------------------------------------------------------

int scan_command(char *cmd_dest, int trappable_only)
{
  int onoff_val;
  int num_subexpr = 0;  
  OP_STACK_ENTRY op;
  int num_expr = 0;
  
  indent_more();
  
  init_op_stack_entry(&op);
  
  drop_space(&cline_i);
  
  dbprintf("Trappable only:%d", trappable_only);

  for(int i=0; i<NUM_FUNCTIONS; i++)
    {
      if( fn_info[i].command && (strn_match(&(cline[cline_i]), fn_info[i].name, strlen(fn_info[i].name))) )
	{
	  // Are we only looking for trappable commands?
	  if( trappable_only && !(fn_info[i].trappable) )
	    {
	      dbprintf("ret0 Trappable only and not a trappable command");
	      return(0);
	    }
	  
	  // Match
	  strcpy(cmd_dest, fn_info[i].name);
	  cline_i += strlen(fn_info[i].name);

	  // Send command to output stream
	  strcpy(op.name, cmd_dest);
	  op.buf_id = EXP_BUFF_ID_FUNCTION;
	  process_token(&op);

	  // The arguments have to be in a sub expression.
	  dbprintf( "\nSTARTEXP");
	  sprintf(op.name, "(");
	  process_token(&op);
	  
	  switch(fn_info[i].argparse)
	    {
	      // ON/OFF
	    case 'O':
	      if( check_onoff(&cline_i, &onoff_val) )
		{
		  // ON and off map to 1 and 0
		  sprintf(op.name, "%d", onoff_val);
		  process_token(&op);
		  
		  dbprintf("%s: ret1 =>'%s'", __FUNCTION__, cmd_dest);
		  dbprintf( "\nENDEXP");
		  sprintf(op.name, ")");
		  process_token(&op);

		  // Scan the argument
		  return(1);
		}
	      else
		{
		  dbprintf("%s: expression failed", __FUNCTION__);
		  return(0);
		}
	      break;

	      // Variable name list
	    case 'V':
	      if( scan_addr_name() )
		{
		  dbprintf("%s: ret1 =>'%s'", __FUNCTION__, cmd_dest);
		  dbprintf( "\nENDEXP");
		  sprintf(op.name, ")");
		  process_token(&op);
		  return(1);
		}
	      else
		{
		  dbprintf("%s: scan_addr_name() failed", __FUNCTION__);
		  return(0);
		}
	      break;
	      
	      // Expression scanning will push expression to output stream.
	    default:
	      if( scan_expression_list(&num_expr) )
		{
		  dbprintf("ret1 =>'%s'", cmd_dest);
		  dbprintf( "\nENDEXP");
		  sprintf(op.name, ")");
		  process_token(&op);
		  
		  return(1);
		}
	      else
		{
		  dbprintf("Expression failed");
		  return(0);
		}
	      break;
	    }
	}
    }

  strcpy(cmd_dest, "");
  dbprintf("ret0");
  return(0);
}

//------------------------------------------------------------------------------

int scan_function(char *cmd_dest)
{
  OP_STACK_ENTRY op;
  int idx;
  int num_expr;
  
  indent_more();
  
  init_op_stack_entry(&op);
  
  drop_space(&cline_i);
  
  dbprintf(" '%s'", &(cline[cline_i]));

  for(int i=0; i<NUM_FUNCTIONS; i++)
    {
      if( !(fn_info[i].command) && strn_match(&(cline[cline_i]), fn_info[i].name, strlen(fn_info[i].name)) )
	{
	  // Match
	  strcpy(cmd_dest, fn_info[i].name);
	  cline_i += strlen(fn_info[i].name);
	  strcpy(op.name, fn_info[i].name);
	  op.buf_id = EXP_BUFF_ID_FUNCTION;
	  process_token(&op);

	  // If the function has no arguments, add a dummy empty expression after it
	  // as the shunting algorithm has to have brackets after a function
	  if( strlen(fn_info[i].argtypes) == 0 )
	    {
	      switch( fn_info[i].argparse )
		{
		case 'V':
		  // The V argument type is a variable name. We use an argparse string of ""
		  // and so need to skip the dummy argument.
		  break;

		default:
		  fprintf(ofp, "\nDummy argument expression added");
		  op.buf_id = EXP_BUFF_ID_NONE;
		  strcpy(op.name, "(");
		  process_token(&op);
		  strcpy(op.name, ")");
		  process_token(&op);
		  break;
		}
	    }
	  else
	    {
	      int num_commas = 0;

	      idx = cline_i;
	      
	      if( check_literal(&idx, " (") )
		{
		  scan_literal(" (");
		  
		  if( !scan_expression_list(&num_expr) )
		    {
		      dbprintf("ret0 Not an expression");
		      return(0);
		    }
		  
		  if( !scan_literal(" )" ))
		    {
		      dbprintf("ret0 No )");
		      return(0);
		    }
		  else
		    {
		      dbprintf("ret1");
		      return(1);
		    }
		}
	      else
		{
		  dbprintf("ret0 No (");
		  return(0);
		}
	    }
	  dbprintf("ret1");
	  return(1);
	}
    }
  
  strcpy(cmd_dest, "");
  dbprintf("ret0");
  return(0);
}

//------------------------------------------------------------------------------

int check_function(int *index)
{
  int idx = *index;

  indent_more();
  
  drop_space(&idx);
  
  dbprintf(" '%s'", &(cline[idx]));

  for(int i=0; i<NUM_FUNCTIONS; i++)
    {
      if( !(fn_info[i].command) && strn_match(&(cline[idx]), fn_info[i].name, strlen(fn_info[i].name)) )
	{
	  dbprintf("Match '%s' with '%s'", &(cline[idx]), fn_info[i].name);
	  
	  // Match
	  idx += strlen(fn_info[i].name);
	  
	  dbprintf(" '%s'", &(cline[idx]));
	  dbprintf(" '%c'", fn_info[i].name[strlen(fn_info[i].name)-1]);
	  dbprintf(" Check for correct type of function and string");
	  
	  switch( fn_info[i].name[strlen(fn_info[i].name)-1] )
	    {
	    case '%':
	    case '$':
	      break;
	      
	    default:
	      // The function does not end in type character
	      switch(cline[idx] )
		{
		case '%':
		case '$':
		  // The line token does have a type character at the end, so it's not a match
		  dbprintf("%s: ret0", __FUNCTION__);
		  return(0);
		  break;
		}
	      break;
	    }
  
	  // If the function has no arguments, add a dummy empty expression after it
	  // as the shunting algorithm has to have brackets after a function
	  if( strlen(fn_info[i].argtypes) == 0 )
	    {
	      switch( fn_info[i].argparse )
		{
		case 'V':
		  // The V argument type is a variable name. We use an argparse string of ""
		  // and so need to skip the dummy argument.
		  break;

		default:
		  dbprintf("No arguments");

		  break;
		}
	    }
	  else
	    {
	      int num_commas = 0;

	      if( check_literal(&idx, " (") )
		{
		  
		  if( !check_expression_list(&idx) )
		    {
		      syntax_error("Not an expression");
		      dbprintf("ret0");
		      return(0);
		    }

		  if( !check_literal(&idx, " )" ) )
		    {
		      syntax_error("No )");
		      dbprintf("ret0");
		      return(0);
		    }
		}
	      else
		{
		  syntax_error("No (");
		  dbprintf("ret0");
		  return(0);
		}
	    }
	  
	  *index = idx;
	  dbprintf("ret1");
	  return(1);
	}
    }

  *index = idx;
  dbprintf("ret0");
  return(0);
}

////////////////////////////////////////////////////////////////////////////////

int check_assignment(int *index)
{
  int idx = *index;
  indent_more();
  
  dbprintf("%s:", __FUNCTION__);
  
  if( check_variable(&idx) )
    {
      if( check_literal(&idx, " =") )
	{
	  if( check_expression(&idx, HEED_COMMA) )
	    {
	      *index = idx;
	      dbprintf("ret1");
	      return(1);
	    }
	}
    }

  dbprintf("%s:ret0", __FUNCTION__);
  *index = idx;
  return(0);
}

//------------------------------------------------------------------------------

int scan_assignment(void)
{
  NOBJ_VAR_INFO vi;
  int num_subexpr  = 0;
  
  indent_more();
  
  init_var_info(&vi);
  dbprintf("%s:", __FUNCTION__);

  if( scan_variable(&vi, VAR_REF, NOPL_OP_ACCESS_WRITE) )
    {
      print_var_info(&vi);
      
      if( scan_assignment_equals() )
	{
	  if( scan_expression(&num_subexpr, HEED_COMMA) )
	    {
	      vi.num_indices = num_subexpr+1;

	      add_var_info(&vi);
	      dbprintf("%s: ret1", __FUNCTION__);
	      return(1);
	    }
	}
    }

  syntax_error("Assignment error");
  return(0);
}

////////////////////////////////////////////////////////////////////////////////

int is_textlabelchar(char c)
{
  if( isalnum(c) )
    {
      return(1);
    }

  if( c == '.' )
    {
      return(1);
    }
  

  return(0);
}

////////////////////////////////////////////////////////////////////////////////

int check_textlabel(int *index, char *label_dest, NOBJ_VARTYPE *type)
{
  int idx = *index;
  char chstr[2];
  indent_more();
  
  chstr[1] = '\0';
  
  *label_dest = '\0';
  
  drop_space(&idx);
  
  dbprintf("'%s'", &(cline[idx]));

  // Just a colon is not a text label  
  if( cline[idx] == ':' )
    {
      *index = idx;
      dbprintf("ret0 (just colon)");
      return(0);
    }

  while( /*(cline[idx] != ':') && */(cline[idx] != '\0') && (cline[idx] != ' ') && (is_textlabelchar(cline[idx])) )
    {
      if( strlen(label_dest) > NOBJ_VARNAME_MAXLEN-1 )
	{
	  break;
	}
      
      chstr[0] = cline[idx];
      strcat(label_dest, chstr);
      idx++;
    }

  dbprintf("'%s' is a text label", label_dest);
  
  *index = idx;

  // Work out the type of the label
  switch(chstr[0])
    {
    case '%':
      *type = NOBJ_VARTYPE_INT;
      break;

    case '$':
      *type = NOBJ_VARTYPE_STR;
      break;

    default:
      *type = NOBJ_VARTYPE_FLT;
      break;
    }
  
  dbprintf("%s:ret1", __FUNCTION__);  
  return(1);
}

////////////////////////////////////////////////////////////////////////////////


int check_label(int *index)
{
  int idx = *index;
  char textlabel[NOBJ_VARNAME_MAXLEN+1];
  NOBJ_VARTYPE type;

  indent_more();
  
  dbprintf("");
  
  if( check_textlabel(&idx, textlabel, &type))
    {
      // labels should not have any type character after them, so that's type FLT
      if( type == NOBJ_VARTYPE_FLT )
	{
	  if( check_literal(&idx, "::") )
	    {
	      dbprintf("ret1");
	      *index = idx;
	      return(1);
	    }
	}
    }
  
  dbprintf("%s:ret0", __FUNCTION__);
  return(0);
}

//------------------------------------------------------------------------------


int scan_label(char *dest_label)
{
  int idx = cline_i;
  char textlabel[NOBJ_VARNAME_MAXLEN+1];
  NOBJ_VARTYPE type;

  indent_more();
  
  dbprintf("");
  
  if( check_textlabel(&idx, textlabel, &type))
    {
      if( type == NOBJ_VARTYPE_FLT )
	{
	  cline_i = idx;
	  
	  if( scan_literal("::") )
	    {
	      strcpy(dest_label, textlabel);
	      dbprintf("ret1");
	      return(1);
	    }
	}
    }

  strcpy(dest_label, "");
  dbprintf("%s:ret0", __FUNCTION__);
  return(0);
}

////////////////////////////////////////////////////////////////////////////////
//
// RETURN
//
// This keyword can have either nothing after it  or an expression
//
// An expression is a an extension from the original which could only have
// a variable after RETURN, or nothing.
//

int check_return(int *index)
{
  int idx = *index;
  char textlabel[NOBJ_VARNAME_MAXLEN+1];
  indent_more();
  
  dbprintf("%s: '%s'", __FUNCTION__, &(cline[idx]));
  
  if( check_literal(&idx, " RETURN"))
    {
      dbprintf("ret1");
      *index = idx;
      return(1);
    }
  
  dbprintf("%s:ret0", __FUNCTION__);
  return(0);
}

//------------------------------------------------------------------------------
//
// RETURN can be present with or without an expression, the qcode genrated is different
// depending on the presence of an expression. We set the 
int scan_return(void)
{
  int idx = cline_i;
  OP_STACK_ENTRY op;
  char v[NOBJ_VARNAME_MAXLEN+1];

  indent_more();
  
  init_op_stack_entry(&op);
  
  dbprintf("%s:", __FUNCTION__);
  
  if( check_literal(&idx, " RETURN"))
    {
      // We may or may not have an expression after the RETURN
      cline_i = idx;
      
      if(check_expression(&idx, HEED_COMMA))
	{
	  int num_subexpr;
	  
	  if( scan_expression(&num_subexpr, HEED_COMMA) )
	    {
	      dbprintf("%s:ret1 Expression present", __FUNCTION__);

	      // The expression we just scanned needs to be the same type as the procedure we
	      // are translating.
	      
	      //	      op.type = NOBJ_VARTY;
	      op.access = NOPL_OP_ACCESS_EXP;
	      op.buf_id = EXP_BUFF_ID_RETURN;
	      strcpy(op.name, "RETURN");
	      process_token(&op);
	      return(1);
	    }
	}
      else
	{
	  // No expression after the RETURN, this is valid
	  // The type is void
	  
	  dbprintf("%s:ret1 Expression not present", __FUNCTION__);
	  op.buf_id = EXP_BUFF_ID_RETURN;
	  op.access = NOPL_OP_ACCESS_NO_EXP;
	  op.type = NOBJ_VARTYPE_VOID;
	  strcpy(op.name, "RETURN");
	  process_token(&op);
	  return(1);
	}
    }
  
  dbprintf("%s:ret0", __FUNCTION__);
  return(0);
}


////////////////////////////////////////////////////////////////////////////////
//
// ONERR
//
// This keyword can be:
//
// ONERR OFF
// ONERR label::
//
//

int check_onerr(int *index)
{
  int idx = *index;

  indent_more();
  
  dbprintf("%s: '%s'", __FUNCTION__, &(cline[idx]));
  
  if( check_literal(&idx, " ONERR"))
    {
      dbprintf("ret1");
      *index = idx;
      return(1);
    }
  
  dbprintf("%s:ret0", __FUNCTION__);
  return(0);
}

//------------------------------------------------------------------------------

int scan_onerr(void)
{
  int idx = cline_i;
  OP_STACK_ENTRY op;
  char label[NOPL_MAX_LABEL+1];
  int word = 0;
  int ok = 0;
  int output_label = 0;
  
  indent_more();
  
  init_op_stack_entry(&op);
  
  dbprintf("%s:", __FUNCTION__);
  
  if( check_literal(&idx, " ONERR"))
    {
      // We accept either 'OFF' or a label, which will be turned into a jump offset 

      cline_i = idx;
      
      if(check_label(&idx))
	{
	  if( scan_label(label) )
	    {

	      output_label = 1;
	      
	      // The qcode has a word after it which is 
	      word = 0x1ab1;
	      ok = 1;	      
	    }
	}
      
      if( check_literal(&idx, " OFF"))
	{
	  cline_i = idx;
	  
	  dbprintf("ret1: OFF");
	  word = 0;
	  ok = 1;
	}
      
      if( ok)
	{
	  // All OK, output the code and two bytes following it	  
	  op.buf_id = EXP_BUFF_ID_FUNCTION;
	  op.num_bytes = 2;
	  op.bytes[0] = word >> 8;
	  op.bytes[1] = word  & 0xFF;
	  
	  strcpy(op.name, "ONERR");
	  process_token(&op);

	  if( output_label )
	    {
	      output_generic(op, label, EXP_BUFF_ID_LABEL);
	      dbprintf("Label '%s'", label);
	      
	    }
	  dbprintf("ret1");
	  return(1);
	}
      else
	{
	  syntax_error("Bad ONERR");
	  dbprintf("ret0");
	  return(0);
	}
    }
  
  // Error

  syntax_error("Bad ONERR");
  dbprintf("ret0");
  return(0);
}

////////////////////////////////////////////////////////////////////////////////
//

// The different types of print

#define PRINT_TYPE_PRINT 0
#define PRINT_TYPE_LPRINT 1

struct
{
  char *token_name;
  char *op_name;
  int  buf_id_newline;
  int  buf_id_space;
  int  buf_id_print;
}
  print_info[] =
  {
    {" PRINT",  "PRINT",  EXP_BUFF_ID_PRINT_NEWLINE,  EXP_BUFF_ID_PRINT_SPACE,   EXP_BUFF_ID_PRINT},
    {" LPRINT", "LPRINT", EXP_BUFF_ID_LPRINT_NEWLINE, EXP_BUFF_ID_LPRINT_SPACE,  EXP_BUFF_ID_LPRINT},
  };

  
  
////////////////////////////////////////////////////////////////////////////////
//
// PRINT
//
// Followed by an expression, or followed by
// list of epressions separted by either ';' or ','.
//
// The delimiters determine the behaviour of newlines between expressions.
// A CRFL flag controls the printing of newlines.
// 
// 
//

int check_print(int *index, int print_type)
{
  int idx = *index;
  char textlabel[NOBJ_VARNAME_MAXLEN+1];
  indent_more();
  
  dbprintf("%s: '%s'", __FUNCTION__, &(cline[idx]));
  
  if( check_literal(&idx, print_info[print_type].token_name))
    {
      dbprintf("ret1");
      *index = idx;
      return(1);
    }
  
  dbprintf("%s:ret0", __FUNCTION__);
  return(0);
}

//------------------------------------------------------------------------------

int scan_print(int print_type)
{
  int idx = cline_i;
  OP_STACK_ENTRY op;
  char v[NOBJ_VARNAME_MAXLEN+1];
  int print_token_needed = 0;
  
  indent_more();
  
  init_op_stack_entry(&op);
  
  dbprintf("print type:%d", print_type);
  
  if( check_literal(&idx, print_info[print_type].token_name) )
    {
      // There is a list of expressions (which may be empty)
      // expressions are delimited by ';' or ','
      // PRINT ';'
      // is not valid.
      //
      // PRINT
      //
      // is valid and generates just a PRINT_NEWLINE token
      //
      
      if( check_expression(&idx, IGNORE_COMMA))
	{
	  int num_subexpr;
	  int delimiter_present;
	  int comma_present;
	  int scolon_present;

	  idx = cline_i;
	  if(check_literal(&idx, print_info[print_type].token_name))
	    {
#if 0
	      dbprintf("Start PRINT token");
	      
	      op.buf_id = EXP_BUFF_ID_PRINT;
	      strcpy(op.name, print_info[print_type].op_name);
	      process_token(&op);
#endif
	    }
	  else
	    {
	      dbprintf("ret0: Expected PRINT");
	      return(0);
	    }

	  // The scan_literal above will generate a PRINT token for us so we don't need it done below
	  print_token_needed = 0;
	  
	  // Scanning expressions here, we do not want comma to be accepted as that
	  // needs to be compiled to a 'print space' qcode
	  cline_i = idx;

	  // We check the entire expression and delimiter and then scan so we can order the
	  // expression tokens and PRINT tokens correctly

	  dbprintf("Before while");
	  while( check_expression(&idx, IGNORE_COMMA) )
	    {
	      dbprintf("Check expression ok, in while loop");
	      
	      //idx = cline_i;

	      delimiter_present = 0;
	      comma_present = 0;
	      scolon_present = 0;

	      // We may need a PRINT token generated, we do it like this so we don't get a
	      // spurious token at the end of every line. (Actually a spurious command/expression)
	      if( print_token_needed,0 )
		{
		  dbprintf("Print token needed");
		  
		  op.buf_id = print_info[print_type].buf_id_print;
		  strcpy(op.name, print_info[print_type].op_name);
		  process_token(&op);
		}
	      
	      // Scan expression, we will add delimiter qcodes in a new command/expression

	      dbprintf("Scan expression");
	      scan_expression( &num_subexpr, IGNORE_COMMA);

	      // Expression scanned, add PRINT token
	      // Just flush the operator stack so the IF is at the end of the output
	      op_stack_finalise();
	      strcpy(op.name, print_info[print_type].op_name);
	      output_generic(op, print_info[print_type].op_name, print_info[print_type].buf_id_print);

	      idx = cline_i;

	      dbprintf("After scan expression I");
	      if( check_literal(&idx, " ,") )
		{
		  scan_literal(" ,");
		  idx = cline_i;
		  
		  // We need a PRINT space qcode to be generated
		  delimiter_present = 1;
		  comma_present = 1;
		}

	      dbprintf("After scan expression II");
	      if( check_literal(&idx, " ;") )
		{
		  // We need a PRINT space qcode to be generated
		  scan_literal(" ;");
		  idx = cline_i;
		  
		  delimiter_present = 1;
		  scolon_present = 1;
		}

	      dbprintf("Checking if delimiter was present");
	      if( !delimiter_present )
		{
		  dbprintf("No delimiter present");

		  // End the PRINT and start a new expression/command
 		  finalise_expression();
		  output_expression_start(&cline[cline_i]);

		  op.buf_id = print_info[print_type].buf_id_newline;
		  strcpy(op.name, print_info[print_type].op_name);
		  process_token(&op);
		  
		  // Now complete that command/expression and start a new PRINT
		  // to handle the next expression to be printed.
		  // We do this with a flag as if we are done with processing we don't need a new
		  // PRINT
		  
		  finalise_expression();
		  output_expression_start(&cline[cline_i]);

		  print_token_needed = 1;
		}
	      else
		{
		  dbprintf("Delimiter present");
		  if( comma_present )
		    {
		      dbprintf("Comma present");
		      
		      // End the PRINT, put in the 'print space' code 
		      finalise_expression();
		      output_expression_start(&cline[cline_i]);

		      op.buf_id = print_info[print_type].buf_id_space;

		      strcpy(op.name, print_info[print_type].op_name);
		      process_token(&op);
		      
		      // Now complete that command/expression and start a new PRINT
		      // to handle the next expression to be printed
		      finalise_expression();
		      output_expression_start(&cline[cline_i]);

		      print_token_needed = 1;
		    }

		  if( scolon_present )
		    {
		      dbprintf("Semi colon present");
		      
		      // Now complete that command/expression and start a new PRINT
		      // to handle the next expression to be printed
		      finalise_expression();
		      output_expression_start(&cline[cline_i]);

		      print_token_needed = 1;
		    }
		}
	      
	      idx = cline_i;
	    }
	  
	  // There could be a semicolon on the end of the print command

	  dbprintf("Check for trailing delimiter");
	  
	  if( check_literal(&idx, " ;") )
	    {
	      dbprintf("Semi colon trailing");
	      
	      scan_literal(" ;");
	      
	      // We need a PRINT qcode to be generated
	      op.buf_id = print_info[print_type].buf_id_print;
	      strcpy(op.name, print_info[print_type].op_name);
	      process_token(&op);
	      dbprintf("%s:ret1 Expression, semicolon terminated", __FUNCTION__);
	      return(1);
	      
	    }

	  if( check_literal(&idx, " ,") )
	    {
	      dbprintf("Comma trailing");
	      scan_literal(" ,");
	      
	      // We need a PRINT qcode to be generated
	      op.buf_id = print_info[print_type].buf_id_space;
	      strcpy(op.name, print_info[print_type].op_name);
	      process_token(&op);
	      dbprintf("%s:ret1 Expression, semicolon terminated", __FUNCTION__);
	      return(1);
	      
	    }
	  
	  dbprintf("%s:ret1 Expression ", __FUNCTION__);
	  return(1);
	  
	}
      else
	{

	  // No expression after the PRINT, this is valid, comma and semicolon after

	  // We don't want a PRINT qcode generated in this case, so use check function to
	  // remove the PRINT from the input stream
	  
	  // PRINT is not needed. We generate the newline here
	  cline_i = idx;
	  
	  dbprintf("%s:ret1 Expression not present", __FUNCTION__);
	  op.buf_id = print_info[print_type].buf_id_newline;
	  op.type = NOBJ_VARTYPE_VOID;
	  strcpy(op.name, print_info[print_type].op_name);
	  process_token(&op);
	  return(1);
	}
    }
  
  dbprintf("ret0: Expected PRINT");
  return(0);
}

////////////////////////////////////////////////////////////////////////////////

int check_input(int *index)
{
  int idx = *index;
  char textlabel[NOBJ_VARNAME_MAXLEN+1];

  indent_more();
  
  dbprintf("%s: '%s'", __FUNCTION__, &(cline[idx]));
  
  if( check_literal(&idx, " INPUT"))
    {
      dbprintf("ret1");
      *index = idx;
      return(1);
    }
  
  dbprintf("%s:ret0", __FUNCTION__);
  return(0);
}

//------------------------------------------------------------------------------

int scan_input(void)
{
  NOBJ_VAR_INFO vi;

  if( scan_literal(" INPUT") )
    {
      if(scan_variable(&vi, VAR_REF, NOPL_OP_ACCESS_READ))
	{
	  print_var_info(&vi);
	  dbprintf("ret1");
	  return(1);
	}
    }

  dbprintf("ret0");
  return(0);
}


////////////////////////////////////////////////////////////////////////////////

int check_proc_call(int *index)
{
  int idx = *index;
  char textlabel[NOBJ_VARNAME_MAXLEN+1];
  NOBJ_VARTYPE type;
  
  indent_more();
  
  dbprintf("%s:", __FUNCTION__);
  if( check_textlabel(&idx, textlabel, &type))
    {
      dbprintf("'%s' is text label", textlabel);
      if( check_literal(&idx, ":") )
	{
	  dbprintf("ret1");
	  *index = idx;
	  dbprintf("%s:ret1 idx='%s'", __FUNCTION__, &(cline[idx]));
	  return(1);
	}

      if( check_literal(&idx, "%:") )
	{
	  dbprintf("ret1");
	  *index = idx;
	  dbprintf("%s:ret1 idx='%s'", __FUNCTION__, &(cline[idx]));
	  return(1);
	}

      if( check_literal(&idx, "$:") )
	{
	  dbprintf("ret1");
	  *index = idx;
	  dbprintf("%s:ret1 idx='%s'", __FUNCTION__, &(cline[idx]));
	  return(1);
	}
    }
  
  dbprintf("%s:ret0", __FUNCTION__);
  return(0);
}

//------------------------------------------------------------------------------

struct
{
  char *suffix;
  NOBJ_VARTYPE type;
} proc_call_info[] =
  {
    { ":",  NOBJ_VARTYPE_FLT},
    { "%:", NOBJ_VARTYPE_INT},
    { "$:", NOBJ_VARTYPE_STR},
  };

#define NUM_PROC_CALL_INFO 3

int scan_proc_call(void)
{
  int idx = cline_i;
  OP_STACK_ENTRY op;
  char textlabel[NOBJ_VARNAME_MAXLEN+1];
  int num_expr = 0;
  NOBJ_VARTYPE type;
      
  indent_more();
  
  init_op_stack_entry(&op);
  
  dbprintf("%s:", __FUNCTION__);
  if( check_textlabel(&idx, textlabel, &type))
    {
      dbprintf("%s:*** '%s'", __FUNCTION__, textlabel);
      fflush(ofp);

      for(int i= 0; i<NUM_PROC_CALL_INFO; i++)
	{
	  if( check_literal(&idx, proc_call_info[i].suffix) )
	    {
	      // We are as far as the end of the proc name, plus colon
	      // scan that far
	      
	      strcat(textlabel, proc_call_info[i].suffix);
	      
	      cline_i = idx;
	      
	      // If there's an open bracket then we have parameters
	      if( check_literal(&idx, " (") )
		{
		  scan_literal(" (");
		  if( check_expression_list(&idx) )
		    {
		      scan_expression_list(&num_expr);
		    }
		  scan_literal(" )");
		}
	      
	      dbprintf("ret1");
	      op.num_parameters = num_expr;
	      op.type = proc_call_info[i].type;
	      op.buf_id = EXP_BUFF_ID_PROC_CALL;
	      strcpy(op.name, textlabel);
	      process_token(&op);
	      return(1);
	    }
	}
    }
  
  dbprintf("%s:ret0", __FUNCTION__);
  return(0);
}

////////////////////////////////////////////////////////////////////////////////

int check_do(int *index)
{
  int idx = *index;

  indent_more();
  
  dbprintf("'%s'", IDX_WHERE);
  
  if( check_literal(&idx, " DO"))
    {
      dbprintf("ret1");
      *index = idx;
      return(1);
    }
  
  dbprintf("ret0");
  return(0);
  
}
//------------------------------------------------------------------------------

int scan_do(LEVEL_INFO levels)
{
  int idx = cline_i;
  int num_sub_expr;
  OP_STACK_ENTRY op;

  init_op_stack_entry(&op);
  
  indent_more();
  
  dbprintf("Entry '%s'", I_WHERE);

  idx = cline_i;
  
  if( check_literal(&idx, " DO"))
    {
      levels.do_level = ++unique_level;;
      
      cline_i = idx;

      op.level = levels.do_level;
      
      // Just flush the operator stack so the UNTIL is at the end of the output
      output_generic(op, "DO", EXP_BUFF_ID_DO);

      idx = cline_i;
      drop_colon(&idx);
      cline_i = idx;

      while(1)
	{
	  // Otherwise it must be a line
	  if( scan_line(levels) )
	    {
	      dbprintf("Line scanned ok");
		
	      // All OK, repeat
	      n_lines_ok++;

	      idx = cline_i;
	      drop_colon(&idx);
	      cline_i = idx;
	    }
	  else
	    {
	      dbprintf("Checking for conditionals");
	      
	      idx = cline_i;
	      
	      if( check_literal(&idx, " CONTINUE") )
		{
		  // Accept the token
		  cline_i = idx;

		  // Add a token to the output stream
		  init_op_stack_entry(&op);
		  op.level = levels.do_level;
		  
		  // Just flush the operator stack so the UNTIL is at the end of the output
		  output_generic(op, "CONTINUE", EXP_BUFF_ID_CONTINUE);
		  continue;
		}

	      idx = cline_i;
	      if( check_literal(&idx, " BREAK") )
		{
		  // Accept the token
		  cline_i = idx;

		  // Add a token to the output stream
		  init_op_stack_entry(&op);
		  op.level = levels.do_level;
		  
		  // Just flush the operator stack so the UNTIL is at the end of the output
		  output_generic(op, "BREAK", EXP_BUFF_ID_BREAK);
		  continue;
		}
	      
	      idx = cline_i;
	      
	      if( check_literal(&idx, " UNTIL") )
		{
		  dbprintf("UNTIL found in DO");
		      
		  // Accept the check as a scan
		  cline_i = idx;
		      
		  int num_subexpr;
		      
		  // We must have an expression after the until
		  if( scan_expression(&num_subexpr, HEED_COMMA) )
		    {
		      // All OK
			  
		      op.level = levels.do_level;
			  
		      // Just flush the operator stack so the UNTIL is at the end of the output
		      op_stack_finalise();
		      output_generic(op, "UNTIL", EXP_BUFF_ID_UNTIL);
		      dbprintf("ret1");
		      return(1);
		    }
		  else
		    {
		      // Bad DO expression
		      syntax_error("Bad UNTIL expression");
		      dbprintf("ret0");
		      return(0);
		    }
		}

	      dbprintf("ret0");
	      return(0);
	    }
	}
    }
  else
    {
      syntax_error("Expected DO");
      dbprintf("ret0");
      return(0);
    }
  
  syntax_error("Bad IF");
  dbprintf("ret0");
  return(0);
}

////////////////////////////////////////////////////////////////////////////////

int check_while(int *index)
{
  int idx = *index;

  indent_more();
  
  dbprintf("'%s'", IDX_WHERE);
  
  if( check_literal(&idx, " WHILE"))
    {
      dbprintf("ret1");
      *index = idx;
      return(1);
    }
  
  dbprintf("ret0");
  return(0);
  
}

//------------------------------------------------------------------------------

int scan_while(LEVEL_INFO levels)
{
  int idx = cline_i;
  int num_sub_expr;
  OP_STACK_ENTRY op;

  init_op_stack_entry(&op);
  
  indent_more();
  
  dbprintf("Entry '%s'", I_WHERE);

  idx = cline_i;
  
  if( check_literal(&idx, " WHILE"))
    {
      levels.do_level = ++unique_level;;
      
      cline_i = idx;

      // TODO: We need to disable assignmemt in this expression scan
      //
      if( scan_expression(&num_sub_expr, IGNORE_COMMA) )
	{
	  // Finish the expression and then output the WHILE token

	  // Just flush the operator stack so the WHILE is at the end of the output
	  op_stack_finalise();

	  // Put the WHILE token after the expression
	  op.level = levels.do_level;
	  output_generic(op, "WHILE", EXP_BUFF_ID_WHILE);

	  idx = cline_i;
	  drop_colon(&idx);
	  cline_i = idx;

	  while(1)
	    {
	      // Otherwise it must be a line
	      if( scan_line(levels) )
		{
		  // All OK, repeat
		  n_lines_ok++;
      
		  idx = cline_i;
		  drop_colon(&idx);
		  cline_i = idx;
		}
	      else
		{
		  dbprintf("Checking for conditionals");

		  idx = cline_i;
		  
		  if( check_literal(&idx, " CONTINUE") )
		    {
		      // Accept the token
		      cline_i = idx;
		      
		      // Add a token to the output stream
		      init_op_stack_entry(&op);
		      op.level = levels.do_level;
		      
		      // Just flush the operator stack so the UNTIL is at the end of the output
		      output_generic(op, "CONTINUE", EXP_BUFF_ID_CONTINUE);
		      continue;
		    }

		  idx = cline_i;
		  
		  if( check_literal(&idx, " BREAK") )
		    {
		      // Accept the token
		      cline_i = idx;
		      
		      // Add a token to the output stream
		      init_op_stack_entry(&op);
		      op.level = levels.do_level;
		      
		      // Just flush the operator stack so the UNTIL is at the end of the output
		      output_generic(op, "BREAK", EXP_BUFF_ID_BREAK);
		      continue;
		    }
		  
		  idx = cline_i;
		  
		  if( check_literal(&idx, " ENDWH") )
		    {
		      dbprintf("ENDWH found in if");
		      
		      cline_i = idx;
		      
		      op.level = levels.do_level;
		      op.buf_id = EXP_BUFF_ID_ENDWH;
		      strcpy(op.name, "ENDWH");
		      process_token(&op);
		      dbprintf("ret1");
		      return(1);
		    }

		  dbprintf("ret0");

		  return(0);
		}
	    }
	}
      else
	{
	  syntax_error("Bad expression");
	  dbprintf("ret0");
	  return(0);
	}
      
    }
  else
    {
      syntax_error("Expected WHILE");
      dbprintf("ret0");
      return(0);
    }
  
  syntax_error("Bad WHILE");
  dbprintf("ret0");
  return(0);
}


////////////////////////////////////////////////////////////////////////////////

int check_if(int *index)
{
  int idx = *index;

  indent_more();
  
  dbprintf("'%s'", IDX_WHERE);
  
  if( check_literal(&idx, " IF"))
    {
      dbprintf("ret1");
      *index = idx;
      return(1);
    }
  
  dbprintf("ret0");
  return(0);
  
}

//------------------------------------------------------------------------------
//
// The conditional commands have a 'level associated with them. That allows, for instance,
// an IF to be matched with an ENDIF or ELSE even when nesting occurs.
//
//------------------------------------------------------------------------------
//
//
// Once an ELSE is seen, no ELSEIFs are allowed
// Only one ELSE allowed
//

int scan_if(LEVEL_INFO levels)
{
  int idx = cline_i;
  int num_sub_expr;
  OP_STACK_ENTRY op;
  int else_seen = 0;
  
  init_op_stack_entry(&op);
  
  indent_more();
  
  dbprintf("Entry '%s'", I_WHERE);

  idx = cline_i;
  
  if( check_literal(&idx, " IF"))
    {
      levels.if_level = ++unique_level;;
      
      cline_i = idx;

      // TODO: We need to disable assignmemt in this expression scan
      //
      if( scan_expression(&num_sub_expr, IGNORE_COMMA) )
	{
	  // Finish the expression and then output the IF token
	  // Just flush the operator stack so the IF is at the end of the output
	  op_stack_finalise();

	  // Put the IF token after the expression
	  op.level = levels.if_level;
	  
	  op.buf_id = EXP_BUFF_ID_IF;
	  strcpy(op.name, "IF");

	  output_if(op);

	  idx = cline_i;
	  drop_colon(&idx);
	  cline_i = idx;
	  
	  while(1)
	    {
	      // Otherwise it must be a line
	      if( scan_line(levels) )
		{
		  dbprintf("Line scanned ok");
				
		  // All OK, repeat
		  n_lines_ok++;
      
		  idx = cline_i;
		  drop_colon(&idx);
		  cline_i = idx;

		  // After scanning a line we need to 
		}
	      else
		{
		  dbprintf("Checking for conditionals");
		  idx = cline_i;
		  
		  if( check_literal(&idx, " CONTINUE") )
		    {
		      // Accept the token
		      cline_i = idx;
		      
		      // Add a token to the output stream
		      init_op_stack_entry(&op);
		      op.level = levels.do_level;
		      
		      // Just flush the operator stack so the UNTIL is at the end of the output
		      output_generic(op, "CONTINUE", EXP_BUFF_ID_CONTINUE);
		      continue;
		    }

		  idx = cline_i;
		  
		  if( check_literal(&idx, " BREAK") )
		    {
		      // Accept the token
		      cline_i = idx;
		      
		      // Add a token to the output stream
		      init_op_stack_entry(&op);
		      op.level = levels.do_level;
		      
		      // Just flush the operator stack so the UNTIL is at the end of the output
		      output_generic(op, "BREAK", EXP_BUFF_ID_BREAK);
		      continue;
		    }
		  
		  idx = cline_i;
		  
		  if( check_literal(&idx, " ENDIF") )
		    {
		      dbprintf("ENDIF found in if");
		      
		      cline_i = idx;
		      
		      op.level = levels.if_level;
		      op.buf_id = EXP_BUFF_ID_ENDIF;
		      strcpy(op.name, "ENDIF");
		      process_token(&op);
		      dbprintf("ret1");
		      return(1);
		    }

		  idx = cline_i;
		  
		  if( check_literal(&idx, " ELSEIF") )
		    {
		      dbprintf("ELSEIF found in if");

		      if( else_seen )
			{
			  dbprintf("ELSEIF not allowed after ELSE");
			  syntax_error("ELSEIF not allowed after ELSE");
			  return(0);
			}
		      else
			{
			  
			  // Accept the check as a scan
			  cline_i = idx;
			  
			  int num_subexpr;
			  
			  // We must have an expression after the elseif
			  if( scan_expression(&num_subexpr, HEED_COMMA) )
			    {
			      // All OK
			      
			      op.level = levels.if_level;
			      
			      // Just flush the operator stack so the IF is at the end of the output
			      op_stack_finalise();

			      // Insert a jump to the ENDIF of the clause so the previous IF or ELSEIF
			      // jumps over the rest of the IF clauses.
			      op.buf_id = EXP_BUFF_ID_META;
			      strcpy(op.name, "BRAENDIF");   
			      process_token(&op);


			      output_generic(op, "ELSEIF", EXP_BUFF_ID_ELSEIF);
			      dbprintf("Done ELSEIF");
			      
			      idx = cline_i;
			      drop_colon(&idx);
			      cline_i = idx;
			      continue;
			    }
			  else
			    {
			      // Bad elseif expression
			      syntax_error("Bad ELSEIF expression");
			      dbprintf("ret0");
			      return(0);
			    }
			}
		    }
		  idx = cline_i;
		  
		  if( check_literal(&idx, " ELSE") )
		    {
		      dbprintf("ELSE found in if");
		      
		      cline_i = idx;
		      
		      // Output an ELSE, we only allow one per IF clause
		      if( else_seen )
			{
			  dbprintf("Only one ELSE allowed in IF clause");
			  syntax_error("Only one ELSE allowed in IF clause");
			  return(0);
			}
		      else
			{
			  
			  // Put the ELSE token in the output
			  op.level = levels.if_level;
			  op.buf_id = EXP_BUFF_ID_ELSE;
			  strcpy(op.name, "ELSE");
			  output_generic(op, "ELSE", EXP_BUFF_ID_ELSE);
			  else_seen = 1;
			  
			  dbprintf("Done ELSE");

			  idx = cline_i;
			  drop_colon(&idx);
			  cline_i = idx;

			  continue;
			}
		    }
		  
#if 0
		  // Error, should have a message from the lower parsers
		  syntax_error("Bad line");
#endif
		  dbprintf("ret0");

		  return(0);
		}
	    }
	}
      else
	{
	  syntax_error("Bad expression");
	  dbprintf("ret0");
	  return(0);
	}
      
    }
  else
    {
      syntax_error("Expected IF");
      dbprintf("ret0");
      return(0);
    }
  
  syntax_error("Bad IF");
  dbprintf("ret0");
  return(0);
}

////////////////////////////////////////////////////////////////////////////////
//
// Check CREATE/OPEN
//
// We need the intermediate code to be :
//
// CREATE/OPEN
// Logical file
// List of field variables
// End of field variables marker
//
// This structure allows the QCode to be easily generated
//

int check_createopen(int *index, int create_nopen)
{
  //  OP_STACK_ENTRY op;
  int idx = *index;
  char *c_no_s;
  
  //init_op_stack_entry(&op);
  
  indent_more();
  
  drop_space(&idx);
  
  dbprintf("Create:%d Open:%d", create_nopen, !create_nopen);

  if( create_nopen )
    {
      c_no_s = "CREATE";
    }
  else
    {
      c_no_s = "OPEN";
    }
  
  idx = cline_i;
  if( check_literal(&idx, c_no_s) )
    {
      dbprintf("ret1");
      return(1);
    }

  dbprintf("ret0");
  return(0);
  
}

//------------------------------------------------------------------------------


int scan_createopen2(int create_nopen)
{
  int idx = cline_i;
  NOBJ_VAR_INFO vi;
  NOBJ_VAR_INFO *srch_vi;
  
  char *keyword;
  OP_STACK_ENTRY op;

  indent_more();  
  
  init_op_stack_entry(&op);
  init_var_info(&vi);
  
  if( create_nopen )
    {
      keyword = " CREATE";
    }
  else
    {
      keyword = " OPEN";
    }
  
  op.buf_id = EXP_BUFF_ID_META;
  strcpy(op.name, keyword);
  
  dbprintf("'%s'", I_WHERE);

  if( check_literal(&idx, keyword) )
    {
      process_token(&op);

      cline_i = idx;
      
      while( check_variable(&idx) )
	{
	  init_var_info(&vi);
	  
	  if( scan_variable(&vi, VAR_DECLARE, NOPL_OP_ACCESS_READ))
	    {
	      // Find the variable and update to make it a local or global
	      srch_vi = find_var_info(vi.name);

	      if( srch_vi != NULL )
		{
		  srch_vi->class = create_nopen?NOPL_VAR_CLASS_CREATE:NOPL_VAR_CLASS_OPEN;
		  srch_vi->is_ref = 0;
		  
		  dbprintf("%s variable:'%s'", keyword, vi.name);
		  print_var_info(&vi);
		  
		  // Store info about the variable
		  add_var_info(&vi);
		}
	    }
	  
	  idx = cline_i;
	  if( check_literal(&idx, " ,") )
	    {
	      scan_literal(" ,");
	    }
	}

      drop_space(&cline_i);

      if( cline[cline_i] == '\0' )
	{
#if 0
	  // Store info about the variable
	  var_info[num_var_info] = vi;
	  num_var_info++;
#endif

	  op.buf_id = EXP_BUFF_ID_META;
	  strcpy(op.name, "ENDFIELDS");
	  process_token(&op);
	  
	  dbprintf("ret1");
	  return(1);
	}
    }

  dbprintf("%s:ret0", __FUNCTION__);
  return(0);
}



int scan_createopen(int create_nopen)
{
  int idx = cline_i;
  NOBJ_VAR_INFO vi;
  NOBJ_VAR_INFO *srch_vi;
  
  char *keyword;
  OP_STACK_ENTRY op;

  indent_more();  
  
  init_op_stack_entry(&op);
  init_var_info(&vi);
  
  if( create_nopen )
    {
      keyword = " CREATE";
    }
  else
    {
      keyword = " OPEN";
    }
  
  op.buf_id = EXP_BUFF_ID_META;
  strcpy(op.name, keyword);
  
  dbprintf("'%s'", I_WHERE);

  int num_expr = 0;
  
  if( check_literal(&idx, keyword) )
    {
      //process_token(&op);

      cline_i = idx;

      if( scan_createopen_list(keyword, create_nopen) )
	{
	  dbprintf("ret1");
	  dbprintf( "ENDEXP");
#if 0
	  sprintf(op.name, ")");
	  process_token(&op);
#endif
	  
	  op.buf_id = EXP_BUFF_ID_META;
	  strcpy(op.name, "ENDFIELDS");
	  process_token(&op);

	  return(1);
	}
      else
	{
	  dbprintf("ret0Expression failed");
	  return(0);
	}
    }

  dbprintf("ret0");
  return(0);
}



////////////////////////////////////////////////////////////////////////////////

int check_line(int *index)
{
  OP_STACK_ENTRY op;
  int idx = *index;

  init_op_stack_entry(&op);
  
  indent_more();
  
  drop_space(&idx);
  
  dbprintf("%s:", __FUNCTION__);

  idx = cline_i;
  if( check_literal(&idx, " REM ") )
    {
      dbprintf("%s:ret1: Comment", __FUNCTION__);
      return(1);
    }

  idx = cline_i;
  if( check_assignment(&idx) )
    {
      dbprintf("ret1");
  
      *index = idx;
      return(1);
    }

  idx = cline_i;
  if( check_print(&idx, PRINT_TYPE_PRINT) )
    {
      dbprintf("ret1");
  
      *index = idx;
      return(1);
    }

  idx = cline_i;
  if( check_print(&idx, PRINT_TYPE_LPRINT) )
    {
      dbprintf("ret1");
  
      *index = idx;
      return(1);
    }

  idx = cline_i;
  if( check_input(&idx) )
    {
      dbprintf("ret1");
  
      *index = idx;
      return(1);
    }

  idx = cline_i;
  if( check_return(&idx) )
    {
      dbprintf("ret1");
  
      *index = idx;
      return(1);
    }

  idx = cline_i;
  if( check_onerr(&idx) )
    {
      dbprintf("ret1");
  
      *index = idx;
      return(1);
    }

  // Has to be before proc call
  idx = cline_i;
  if( check_label(&idx) )
    {
      dbprintf("ret1");
      *index = idx;
      return(1);
    }

  idx = cline_i;
  if( check_proc_call(&idx) )
    {
      dbprintf("ret1");
  
      *index = idx;
      return(1);
    }

  idx = cline_i;
  if( check_if(&idx) )
    {
      dbprintf("ret1");
      *index = idx;
      return(1);
    }

  idx = cline_i;
  if( check_do(&idx) )
    {
      dbprintf("ret1 do");
      *index = idx;
      return(1);
    }

  idx = cline_i;
  if( check_createopen(&idx, 1))
    {
      dbprintf("ret1");
      *index = idx;
      return(1);
    }

  idx = cline_i;
  if( check_createopen(&idx,0))
    {
      dbprintf("ret1");
      *index = idx;
      return(1);
    }

  idx = cline_i;
  if( check_literal(&idx," OFF"))
    {
      dbprintf("ret1");
      *index = idx;
      return(1);
    }

  
  idx = cline_i;
  if( check_command(&idx) )
    {
      dbprintf("ret1");
      *index = idx;
      return(1);
    }

  
  idx = cline_i;
  if( check_function(&idx) )
    {
      dbprintf("ret1");
      *index = idx;
      return(1);
    }


  idx = cline_i;
  if( check_literal(&idx," ELSEIF"))
    {
      if( check_expression(&idx, HEED_COMMA) )
	{
	  dbprintf("ret1");
	  *index = idx;
	  return(1);
	}
    }

  idx = cline_i;
  if( check_while(&idx))
    {
      dbprintf("ret1");
      *index = idx;
      return(1);
    }

  idx = cline_i;
  if( check_literal(&idx," GOTO"))
    {
      char textlabel[NOBJ_VARNAME_MAXLEN+1];
      
      if( check_label(&idx) )
	{
	  dbprintf("ret1");
	  *index = idx;
	  return(1);
	}
    }

  idx = cline_i;
  if( check_literal(&idx," TRAP"))
    {
      dbprintf("ret1");
      *index = idx;
      return(1);
    }

  dbprintf("ret1");
  *index = idx;
  return(0);
}

////////////////////////////////////////////////////////////////////////////////
//
// Scan another line.
//
// Lines are part of 'composite lines' which are just text made up of colon
// delimited lines.
//
// scan_line pulls the next line from the composite line handler and scans it
//


int scan_line(LEVEL_INFO levels)
{
  int idx = cline_i;
  char cmdname[300];
  char label[NOPL_MAX_LABEL+1];
  
  indent_more();
  
  drop_space(&cline_i);
  
  dbprintf("%s:", __FUNCTION__);

  if( !pull_next_line() )
    {
      dbprintf("ret0 pull_next_line=0");
      return(0);
    }

  if( strlen(&cline[cline_i]) > 0 )
    {
      if( !is_all_spaces(cline_i) )
	{
	  dbprintf("===================================expr==========================================");
	  fprintf(chkfp, "\n===================================expr==========================================");
	  //fprintf(chkfp, "\n%ld\n", strlen(&(cline[cline_i])));
	  fprintf(chkfp, "\n%s\n", &(cline[cline_i]));
	}
    }
  
  // If it's a REM, then the line parses and we generate no tokens
  
  idx = cline_i;
  if( check_literal(&idx, " REM") )
    {
      cline_i = strlen(cline);
      dbprintf("Comment ignored");
      dbprintf("ret1");
      return(1);
    }

  idx = cline_i;
  if( check_assignment(&idx) )
    {

      if(scan_assignment())
	{
	  dbprintf("ret1");
	  return(1);
	}
      else
	{
	  dbprintf("ret0");
	  return(0);
	}
    }

  // Has to be before proc call
  idx = cline_i;
  if( check_label(&idx))
    {
      if( scan_label(label) )
	{
	  OP_STACK_ENTRY op;

	  init_op_stack_entry(&op);
	  
	  // Put a token in the output stream to mark the label position
	  output_generic(op, label, EXP_BUFF_ID_LABEL);

	  dbprintf("ret1 label");
	  return(1);
	}
      else
	{
	  dbprintf("ret0 label");
	  return(0);
	}
    }

  idx = cline_i;
  if( check_proc_call(&idx) )
    {
      if(scan_proc_call())
	{
	  dbprintf("ret1 proc call");
	  return(1);
	}
      else
	{
	  dbprintf("ret0 proc cll");
	  return(0);
	}
    }

  idx = cline_i;
  if( check_print(&idx, PRINT_TYPE_PRINT) )
    {
      if(scan_print(PRINT_TYPE_PRINT))
	{
	  dbprintf("ret1 print");
	  return(1);
	}
      else
	{
	  dbprintf("ret0 print");
	  return(0);
	}
    }

  idx = cline_i;
  if( check_print(&idx, PRINT_TYPE_LPRINT) )
    {
      
      if(scan_print(PRINT_TYPE_LPRINT))
	{
	  dbprintf("ret1 lprint");
	  return(1);
	}
      else
	{
	  dbprintf("ret0 lprint");
	  return(0);
	}

    }

  idx = cline_i;
  if( check_input(&idx) )
    {
      if(scan_input())
      	{
	  dbprintf("ret1 input");
	  return(1);
	}
      else
	{
	  dbprintf("ret0 input");
	  return(0);
	}

    }

  idx = cline_i;
  if( check_return(&idx) )
    {
      if(scan_return())
	{
	  dbprintf("ret1 return");
	  return(1);
	}
      else
	{
	  dbprintf("ret0 return");
	  return(0);
	}
    }

  idx = cline_i;
  if( check_onerr(&idx) )
    {
      if(scan_onerr())
	{
	  dbprintf("ret1 onerr");
	  return(1);
	}
      else
	{
	  dbprintf("ret0 onerr");
	  return(0);
	}
    }

  idx = cline_i;
  if( check_if(&idx) )
    {
      dbprintf("%s:check_if: ", __FUNCTION__);
      scan_if(levels);

      dbprintf("ret1 if");
      return(1);
    }
  
  idx = cline_i;
  if( check_do(&idx) )
    {
      scan_do(levels);

      dbprintf("ret1 do");
      return(1);
    }

  // Off has two forms. One, with a number following is for the LZ
  
  idx = cline_i;
  if( check_literal(&idx," OFF"))
    {
      OP_STACK_ENTRY op;
      int num_subexpr;
      
      init_op_stack_entry(&op);
      op.buf_id = EXP_BUFF_ID_META;
      
      // Scan the function name
      // We accept the check() result as we may want to adjust the function name
      cline_i = idx;
      
      // Optional number
      if( check_expression(&idx, HEED_COMMA) )
	{
	  // We will use the OFFX command
	  strcpy(op.name, "OFFX");
	  process_token(&op);
	  
	  if( scan_expression(&num_subexpr, HEED_COMMA) )
	    {
	      dbprintf("ret1");
	      return(1);
	    }
	  else
	    {
	      dbprintf("ret0: Expected a number");
	      return(0);
	    }
	}
      else
	{
	  // Use the OFF command
	  strcpy(op.name, "OFF");
	  process_token(&op);
	  dbprintf("ret1");
	  return(1);    
	}
      
      cline_i = idx;
      dbprintf("ret0");
      return(0);    
    }

  idx = cline_i;
  if( check_command(&idx) )
    {
      dbprintf("%s:check_command: ", __FUNCTION__);
      scan_command(cmdname, SCAN_ALL);
      dbprintf("ret1 command");
      return(1);
    }

  idx = cline_i;
  if( check_function(&idx) )
    {
      dbprintf("%s:check_function: ", __FUNCTION__);
      scan_function(cmdname);
      dbprintf("ret1 function");
      return(1);
    }
  
  idx = cline_i;
  if( check_literal(&idx," DO") )
    {
      scan_literal(" DO");
      dbprintf("ret1 do");
      return(1);
    }

  idx = cline_i;
  if( check_createopen(&idx, 1))
    {
      scan_createopen(1);
      dbprintf("ret1");
      return(1);
    }
  
  idx = cline_i;
  if( check_createopen(&idx, 0))
    {
      scan_createopen(0);
      dbprintf("ret1");
      return(1);
    }

  idx = cline_i;
  if( check_while(&idx) )
    {
      if( scan_while(levels) )
	{
	  dbprintf("ret1 while");
	  return(1);
	}
      
      dbprintf("ret0 while");
      return(0);
    }

  idx = cline_i;
  if( check_literal(&idx," GOTO"))
    {
      char label[NOPL_MAX_LABEL];
      OP_STACK_ENTRY op;

      init_op_stack_entry(&op);
      
      cline_i = idx;
      
      if( scan_label(label) )
	{
	  dbprintf("ret1 goto");
	  output_generic(op, label, EXP_BUFF_ID_GOTO);
	  return(1);
	}
    }

  idx = cline_i;
  if( check_literal(&idx," TRAP"))
    {
      OP_STACK_ENTRY op;
      
      init_op_stack_entry(&op);

      // Accept the TRAP and insert a TRAP token
      cline_i = idx;

      output_generic(op, "TRAP", EXP_BUFF_ID_TRAP);

      // We want the TRAP before the next command
      finalise_expression();
      output_expression_start(&cline[cline_i]);

      if( scan_command(cmdname, SCAN_TRAPPABLE) )
	{
	  dbprintf("ret1");
	  return(1);
	}
      else
	{
	  dbprintf("ret0: Scanning for trappable command failed");
	  return(0);
	}
    }

}

////////////////////////////////////////////////////////////////////////////////


FILE *infp;
int output_expression_started = 0;

void initialise_line_supplier(FILE *fp)
{
  infp = fp;

  output_expression_started = 0;
  cline_i = 0;
  cline[0] = '\0';
  
}



////////////////////////////////////////////////////////////////////////////////

int is_all_spaces(int idx)
{
  int all_spaces = 1;
  
  // Check it's not a line full of spaces
  all_spaces = 1;
  
  for(int i=0; i<strlen(&(cline[idx])); i++)
    {
      dbprintf("cline[%d] = '%c' (%d)", i, cline[i+idx], cline[i+idx]);
      
      if( !isspace(cline[i+idx]) )
	{
	  all_spaces = 0;
	}
    }

  dbprintf("Is %sall spaces", all_spaces?"":"not ");
  return(all_spaces);
}

////////////////////////////////////////////////////////////////////////////////


int pull_next_line(void)
{
  int all_spaces;
  
  dbprintf("");
  dbprintf("Processing epression just parsed");
  
  
  while( isspace( cline[strlen(cline)-1]) )
    {
      cline[strlen(cline)-1] = '\0';
    }

  if( output_expression_started )
    {
      output_expression_started = 0;
      finalise_expression();
    }
  
  output_expression_start(cline);
  output_expression_started = 1;	  

  dbprintf("Checking for existing data in cline. cline_i=%d strlen:%d ", cline_i, strlen(&(cline[cline_i])));

  // If cline_i is 0 then assume we don't need to pull another line
  if( strlen(&(cline[cline_i])) > 0 )
    {
      dbprintf("Data still in line buffer, check not all space");
      
      // As long as it's not all spaces
      if( !is_all_spaces(cline_i) )
	{
	  dbprintf("ret1  Not all spaces");
	  return(1);
	}
    }
  
  do
    {
      dbprintf("Reading line");
      
      if(!feof(infp) )
	{
	  if( !next_composite_line(infp) )
	    {
	      cline[0] = '\0';
	    }
	}
      else
	{
	  dbprintf("ret0");
	  return(0);
	}
#if 0      
      // Check it's not a line full of spaces
      all_spaces = 1;
      for(int i=0; i<strlen(cline); i++)
	{
	  if( !isspace(cline[i]) )
	    {
	      all_spaces = 0;
	    }
	}
#endif
      
      if( all_spaces = is_all_spaces(0) )
	{
	  dbprintf("Line was all spaces");
	  n_lines_blank++;
	}
    }
  while(all_spaces);
  
  ////////////////

  dbprintf("Got a line: '%s'", cline);
  
  fprintf(ofp, "\n");
  for(int i=0; i<strlen(cline)+4; i++)
    {
      fprintf(ofp, "*");
    }
  
  fprintf(ofp, "\n**%s**", cline);
  
  fprintf(ofp, "\n");
  
  for(int i=0; i<strlen(cline)+4; i++)
    {
      fprintf(ofp, "*");
    }
  fprintf(ofp, "\n");

  output_expression_start(cline);
  indent_none();

  dbprintf("ret1");
  return(1);
}

////////////////////////////////////////////////////////////////////////////////
//
//

int scan_param_list(void)
{
  int idx = cline_i;
  NOBJ_VAR_INFO vi;
  NOBJ_VAR_INFO *srch_vi;
  
  idx = cline_i;

  // If there's an open bracket then we have parameters
  if( check_literal(&idx, " (") )
    {
      scan_literal(" (");

      while( check_variable(&idx) )
	{
	  init_var_info(&vi);
	  
	  if( scan_variable(&vi, VAR_PARAMETER, NOPL_OP_ACCESS_READ))
	    {

	      // Find the variable just added and make it a parameter
	      srch_vi = find_var_info(vi.name);
	      if( srch_vi != NULL )
		{
		  
		  srch_vi->class = NOPL_VAR_CLASS_PARAMETER;
		  //vi.is_global = !local_nglobal;
		  srch_vi->is_ref = 0;
		  //vi.is_param = 1;
		  
		  dbprintf("Parameter list variable:'%s'", vi.name);
		  print_var_info(&vi);
		}
	      
	      // Store info about the variable
	      //add_var_info(&vi);
	    }
	  
	  idx = cline_i;
	  if( check_literal(&idx, " ,") )
	    {
	      scan_literal(" ,");
	    }
	}

      // End of paramter list
      if( check_literal(&idx, " )") )
	{
	  scan_literal(" )");
	}
      
      drop_space(&cline_i);
      
      if( cline[cline_i] == '\0' )
	{
	  dbprintf("ret1");
	  return(1);
	}
    } 

  dbprintf("%s:ret0", __FUNCTION__);
  return(0);
}

////////////////////////////////////////////////////////////////////////////////

int scan_procdef(void)
{
  int idx = cline_i;
  char textlabel[NOBJ_VARNAME_MAXLEN+1];
  OP_STACK_ENTRY op;
  int num_subexpr;
    
  init_op_stack_entry(&op);
  op.buf_id = EXP_BUFF_ID_META;
  strcpy(op.name, "PROCDEF");   
  process_token(&op);

  indent_more();
  
  dbprintf("");
  
  if( check_textlabel(&idx, textlabel, &procedure_type))
    {
      cline_i = idx;

      dbprintf("Text label:'%s'", textlabel);
      if( scan_literal(":") )
	{

	  strcpy(procedure_name, textlabel);

	  // Now scan the parameter list
	  scan_param_list();
	  dbprintf("ret1 Type:%c", type_to_char(procedure_type));
	  return(1);
	}
    }
  
  dbprintf("%s:ret0", __FUNCTION__);
  return(0);
}

////////////////////////////////////////////////////////////////////////////////
//
// Scans LOCAL and GLOBAL
//
// We don't want any tokens in the output stream for this
//

#define SCAN_LOCAL   1
#define SCAN_GLOBAL  0

int scan_localglobal(int local_nglobal)
{
  int idx = cline_i;
  NOBJ_VAR_INFO vi;
  NOBJ_VAR_INFO *srch_vi;
  
  char *keyword;
  OP_STACK_ENTRY op;

  indent_more();  
  
  init_op_stack_entry(&op);
  init_var_info(&vi);
  
  if( local_nglobal )
    {
      keyword = " LOCAL";
    }
  else
    {
      keyword = " GLOBAL";
    }
  
  op.buf_id = EXP_BUFF_ID_META;
  strcpy(op.name, keyword);
  
  dbprintf("'%s'", I_WHERE);

  if( check_literal(&idx, keyword) )
    {
      process_token(&op);

      cline_i = idx;
      
      while( check_variable(&idx) )
	{
	  init_var_info(&vi);
	  
	  if( scan_variable(&vi, VAR_DECLARE, NOPL_OP_ACCESS_READ))
	    {
	      // Find the variable and update to make it a local or global
	      srch_vi = find_var_info(vi.name);

	      if( srch_vi != NULL )
		{
		  srch_vi->class = local_nglobal?NOPL_VAR_CLASS_LOCAL:NOPL_VAR_CLASS_GLOBAL;
		  srch_vi->is_ref = 0;
		  
		  dbprintf("%s variable:'%s'", keyword, vi.name);
		  print_var_info(&vi);
		  
		  // Store info about the variable
		  //add_var_info(&vi);
		}
	    }
	  
	  idx = cline_i;
	  if( check_literal(&idx, " ,") )
	    {
	      scan_literal(" ,");
	    }
	}

      drop_space(&cline_i);

      if( cline[cline_i] == '\0' )
	{
#if 0
	  // Store info about the variable
	  var_info[num_var_info] = vi;
	  num_var_info++;
#endif
	  dbprintf("ret1");
	  return(1);
	}
    }

  dbprintf("%s:ret0", __FUNCTION__);
  return(0);
}

////////////////////////////////////////////////////////////////////////////////

int check_declare(int *index)
{
  int idx = *index;
  indent_more();
  
  dbprintf("'%s'", IDX_WHERE);
  
  if( check_literal(&idx, " LOCAL") )
    {
      dbprintf("%s:ret 1", __FUNCTION__);
      *index = idx;
      return(1);
    }

  if( check_literal(&idx, " GLOBAL") )
    {
      dbprintf("%s:ret 1", __FUNCTION__);
      *index = idx;
      return(1);
    }

  dbprintf("ret 0");
  *index = idx;
  return(0);
}

//------------------------------------------------------------------------------

int scan_declare(void)
{
  int idx = cline_i;
  indent_more();
  
  dbprintf("%s:", __FUNCTION__);
  
  if( check_literal(&idx, " LOCAL") )
    {
      if( scan_localglobal(SCAN_LOCAL) )
	{
	  dbprintf("%s:ret 1", __FUNCTION__);
	  return(1);
	}
      else
	{
	  syntax_error("Bad LOCAL");
	}
    }

  idx = cline_i;
  
  if( check_literal(&idx, " GLOBAL") )
    {
      if( scan_localglobal(SCAN_GLOBAL) )
	{
	  dbprintf("%s:ret 1", __FUNCTION__);
	  return(1);
	}
      else
	{
	  syntax_error("Bad GLOBAL");
	}
    }

  dbprintf("%s:ret 0", __FUNCTION__);
  return(0);
}

////////////////////////////////////////////////////////////////////////////////
//
// Checks and initialises the parser
//

void parser_check(void)
{
  procedure_has_return = 0;
  
  dbprintf("NUM_BUFF_ID    :%d", NUM_BUFF_ID);
  dbprintf("EXP_BUFF_ID_MAX:%d", EXP_BUFF_ID_MAX);
  dbprintf("");
  
  assert( NUM_BUFF_ID == (EXP_BUFF_ID_MAX +1));
}

