//
// Dumps an object file to stdout
//
//

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include "nopl.h"

#define QCO_OPEN     0x65
#define QCO_CREATE   0x5E

struct _QCODE_DESC
{
  uint8_t qcode;
  char *bytes;
  char *pops;
  char *pushes;
  char *desc;  
}
  qcode_decode[] =
    {
      //        Inline  Pull    Push
      //
     {0x00,	"v",	"-",	"I",	"Push local/global integer variable value"},
     {0x01,	"v",	"-",	"F",	"Push local/global float variable value"},
     {0x02,	"v",	"-",	"S",	"Push local/global string variable value"},
     {0x03,	"v",	"I",	"I",	"Pop integer index and push local/global integer array variable value"},
     {0x04,	"v",	"I",	"F",	"Pop integer index and push local/global float array variable value"},
     {0x05,	"v",	"I",	"S",	"Pop integer index and push local/global string array variable value"},
     {0x06,	"m",	"-",	"F",	"Push calculator memory. Is followed by a byte indicating which of the 10 memories."},
     {0x07,	"V",	"-",	"I",	"Push parameter/external integer variable value"},
     {0x08,	"V",	"-",	"F",	"Push parameter/external float variable value"},
     {0x09,	"V",	"-",	"S",	"Push parameter/external string variable value"},
     {0x0A,	"V",	"I",	"I",	"Pop integer index and push parameter/external integer array variable value"},
     {0x0B,	"V",	"I",	"F",	"Pop integer index and push parameter/external float array variable value"},
     {0x0C,	"V",	"I",	"S",	"Pop integer index and push parameter/external string array variable value"},
     {0x0D,	"v",	"-",	"i",	"Push local/global integer variable reference"},
     {0x0E,	"v",	"-",	"f",	"Push local/global float variable reference"},
     {0x0F,	"v",	"-",	"s",	"Push local/global string variable reference"},
     {0x10,	"v",	"I",	"i",	"Pop integer index and push local/global integer array variable reference"},
     {0x11,	"v",	"I",	"f",	"Pop integer index and push local/global float array variable reference"},
     {0x12,	"v",	"I",	"s",	"Pop integer index and push local/global string array variable reference"},
     {0x13,	"m",	"-",	"f",	"Push calculator memory reference. Is followed by a byte indicating which of the 10 memories."},
     {0x14,	"V",	"-",	"i",	"Push parameter/external integer variable reference"},
     {0x15,	"V",	"-",	"f",	"Push parameter/external float variable reference"},
     {0x16,	"V",	"-",	"s",	"Push parameter/external string variable reference"},
     {0x17,	"V",	"I",	"i",	"Pop integer index and push parameter/external integer array variable reference"},
     {0x18,	"V",	"I",	"f",	"Pop integer index and push parameter/external float array variable reference"},
     {0x19,	"V",	"I",	"s",	"Pop integer index and push parameter/external string array variable reference"},
     {0x1A,	"f",	"S",	"I",	"Push file field as integer. Is followed by 1 byte logical file name (0-3 for A-D)"},
     {0x1B,	"f",	"S",	"F",	"Push file field as float. Is followed by 1 byte logical file name (0-3 for A-D)"},
     {0x1C,	"f",	"S",	"S",	"Push file field as string. Is followed by 1 byte logical file name (0-3 for A-D)"},
     {0x1D,	"f",	"S",	"I",	"Push reference of file integer field. Is followed by 1 byte logical file name (0-3 for A-D)"},
     {0x1E,	"f",	"S",	"F",	"Push reference of file float field. Is followed by 1 byte logical file name (0-3 for A-D)"},
     {0x1F,	"f",	"S",	"S",	"Push reference of file string field. Is followed by 1 byte logical file name (0-3 for A-D)"},
     {0x20,	"B",	"-",	"B",	"Push byte literal"},
     {0x21,	"I",	"-",	"I",	"Push word literal (same as integer literal)"},
     {0x22,	"I",	"-",	"I",	"Push integer literal"},
     {0x23,	"F",	"-",	"F",	"Push float literal"},
     {0x24,	"S",	"-",	"S",	"Push string literal"},
     {0x25,	"-",	"-",	"-",	"Special call to machine code. Not used by the organiser's compiler."},
     {0x26,	"-",	"-",	"-",	"Calls UT$LEAV, which quits OPL?? Not used by the organiser's compiler."},
     {0x27,	"-",	"II",	"I",	"Less than integer"},
     {0x28,	"-",	"II",	"I",	"Less than equal integer"},
     {0x29,	"-",	"II",	"I",	"Greater than integer"},
     {0x2A,	"-",	"II",	"I",	"Greater than equal integer"},
     {0x2B,	"-",	"II",	"I",	"Not equal integer"},
     {0x2C,	"-",	"II",	"I",	"Equal integer"},
     {0x2D,	"-",	"II",	"I",	"Plus integer"},
     {0x2E,	"-",	"II",	"I",	"Minus integer"},
     {0x2F,	"-",	"II",	"I",	"Multiply integer"},
     {0x30,	"-",	"II",	"I",	"Divide integer"},
     {0x31,	"-",	"II",	"I",	"Power integer"},
     {0x32,	"-",	"I",	"I",	"unary minus integer"},
     {0x33,	"-",	"I",	"I",	"NOT integer"},
     {0x34,	"-",	"II",	"I",	"AND integer"},
     {0x35,	"-",	"II",	"I",	"OR integer"},
     {0x36,	"-",	"FF",	"I",	"Less than float"},
     {0x37,	"-",	"FF",	"I",	"Less than equal float"},
     {0x38,	"-",	"FF",	"I",	"Greater than float"},
     {0x39,	"-",	"FF",	"I",	"greater than equal float"},
     {0x3A,	"-",	"FF",	"I",	"Not equal float"},
     {0x3B,	"-",	"FF",	"I",	"Equal float"},
     {0x3C,	"-",	"FF",	"F",	"Plus float"},
     {0x3D,	"-",	"FF",	"F",	"Minus float"},
     {0x3E,	"-",	"FF",	"F",	"Multiply float"},
     {0x3F,	"-",	"FF",	"F",	"Divide float"},
     {0x40,	"-",	"FF",	"F",	"Power float"},
     {0x41,	"-",	"F",	"F",	"Unary minus float"},
     {0x42,	"-",	"F",	"I",	"NOT float"},
     {0x43,	"-",	"FF",	"I",	"AND float"},
     {0x44,	"-",	"FF",	"I",	"OR float"},
     {0x45,	"-",	"SS",	"I",	"Less than string"},
     {0x46,	"-",	"SS",	"I",	"Less than equal string"},
     {0x47,	"-",	"SS",	"I",	"Greater than string"},
     {0x48,	"-",	"SS",	"I",	"Greater than equal string"},
     {0x49,	"-",	"SS",	"I",	"Not equal string"},
     {0x4A,	"-",	"SS",	"I",	"Equal string"},
     {0x4B,	"-",	"SS",	"S",	"Plus string"},
     {0x4C,	"-",	"II",	"-",	"AT"},
     {0x4D,	"-",	"II",	"-",	"BEEP"},
     {0x4E,	"-",	"-",	"-",	"CLS"},
     {0x4F,	"O",	"-",	"-",	"CURSOR"},
     {0x50,	"O",	"-",	"-",	"ESCAPE"},
     {0x51,	"D",	"-",	"-",	"GOTO"},
     {0x52,	"-",	"-",	"-",	"OFF"},
     {0x53,	"D",	"-",	"-",	"ONERR"},
     {0x54,	"-",	"I",	"-",	"PAUSE"},
     {0x55,	"-",	"II",	"-",	"POKEB"},
     {0x56,	"-",	"II",	"-",	"POKEW"},
     {0x57,	"-",	"I",	"-",	"RAISE"},
     {0x58,	"-",	"F",	"-",	"RANDOMIZE"},
     {0x59,	"-",	"-",	"-",	"STOP"},
     {0x5A,	"-",	"-",	"-",	"TRAP"},
     {0x5B,	"-",	"-",	"-",	"APPEND"},
     {0x5C,	"-",	"-",	"-",	"CLOSE"},
     {0x5D,	"-",	"SS",	"-",	"COPY"},
     {0x5E,	"f+list",	"S",	"-",	"CREATE"},
     {0x5F,	"-",	"S",	"-",	"DELETE"},
     {0x60,	"-",	"-",	"-",	"ERASE"},
     {0x61,	"-",	"-",	"-",	"FIRST"},
     {0x62,	"-",	"-",	"-",	"LAST"},
     {0x63,	"-",	"-",	"-",	"NEXT"},
     {0x64,	"-",	"-",	"-",	"BACK"},
     {0x65,	"f+list",	"S",	"-",	"OPEN"},
     {0x66,	"-",	"I",	"-",	"POSITION"},
     {0x67,	"-",	"SS",	"-",	"RENAME"},
     {0x68,	"-",	"-",	"-",	"UPDATE"},
     {0x69,	"f",	"-",	"-",	"USE"},
     {0x6A,	"-",	"I",	"-",	"KSTAT"},
     {0x6B,	"-",	"s",	"-",	"EDIT"},
     {0x6C,	"-",	"i",	"-",	"INPUT integer"},
     {0x6D,	"-",	"f",	"-",	"INPUT float"},
     {0x6E,	"-",	"s",	"-",	"INPUT string"},
     {0x6F,	"-",	"I",	"-",	"PRINT integer"},
     {0x70,	"-",	"F",	"-",	"PRINT float"},
     {0x71,	"-",	"S",	"-",	"PRINT string"},
     {0x72,	"-",	"-",	"-",	"PRINT ,"},
     {0x73,	"-",	"-",	"-",	"PRINT newline"},
     {0x74,	"-",	"I",	"-",	"LPRINT integer"},
     {0x75,	"-",	"F",	"-",	"LPRINT float"},
     {0x76,	"-",	"S",	"-",	"LPRINT string"},
     {0x77,	"-",	"-",	"-",	"LPRINT ,"},
     {0x78,	"-",	"-",	"-",	"LPRINT newline"},
     {0x79,	"-",	"I/F/S",	"-",	"RETURN"},
     {0x7A,	"-",	"-",	"-",	"RETURN (integer 0)"},
     {0x7B,	"-",	"-",	"-",	"RETURN (float 0)"},
     {0x7C,	"-",	"-",	"-",	"RETURN (string "")"},
     {0x7D,	"S",	"params",	"F/I/S",	"Procedure call."},
     {0x7E,	"D",	"I",	"-",	"Branch if false"},
     {0x7F,	"-",	"iI",	"-",	"Assign integer"},
     {0x80,	"-",	"fF",	"-",	"Assign float"},
     {0x81,	"-",	"sS",	"-",	"Assign string"},
     {0x82,	"-",	"B",	"-",	"drop byte from stack"},
     {0x83,	"-",	"I",	"-",	"drop integer from stack"},
     {0x84,	"-",	"F",	"-",	"drop float from stack"},
     {0x85,	"-",	"S",	"-",	"drop string from stack"},
     {0x86,	"-",	"I",	"F",	"autoconversion int to float"},
     {0x87,	"-",	"F",	"I",	"autoconversion float to int"},
     {0x88,	"-",	"-",	"-",	"End of field list for CREATE/OPEN"},
     {0x89,	"code",	"-",	"-",	"Inline assembly. Not used by the organiser's compiler."},
     {0x8A,	"-",	"i/f",	"I",	"ADDR"},
     {0x8B,	"-",	"S",	"I",	"ASC"},
     {0x8C,	"-",	"-",	"I",	"DAY"},
     {0x8D,	"-",	"IS",	"I",	"DISP"},
     {0x8E,	"-",	"-",	"I",	"ERR"},
     {0x8F,	"-",	"S",	"I",	"FIND"},
     {0x90,	"-",	"-",	"I",	"FREE"},
     {0x91,	"-",	"-",	"I",	"GET"},
     {0x92,	"-",	"-",	"I",	"HOUR"},
     {0x93,	"-",	"I",	"I",	"IABS"},
     {0x94,	"-",	"F",	"I",	"INT"},
     {0x95,	"-",	"-",	"I",	"KEY"},
     {0x96,	"-",	"S",	"I",	"LEN"},
     {0x97,	"-",	"SS",	"I",	"LOC"},
     {0x98,	"-",	"S",	"I",	"MENU"},
     {0x99,	"-",	"-",	"I",	"MINUTE"},
     {0x9A,	"-",	"-",	"I",	"MONTH"},
     {0x9B,	"-",	"I",	"I",	"PEEKB"},
     {0x9C,	"-",	"I",	"I",	"PEEKW"},
     {0x9D,	"-",	"-",	"I",	"RECSIZE"},
     {0x9E,	"-",	"-",	"I",	"SECOND"},
     {0x9F,	"-",	"II",	"I",	"USR"},
     {0xA0,	"-",	"II",	"I",	"VIEW"},
     {0xA1,	"-",	"-",	"I",	"YEAR"},
     {0xA2,	"-",	"-",	"I",	"COUNT"},
     {0xA3,	"-",	"-",	"I",	"EOF"},
     {0xA4,	"-",	"S",	"I",	"EXIST"},
     {0xA5,	"-",	"-",	"I",	"POS"},
     {0xA6,	"-",	"F",	"F",	"ABS"},
     {0xA7,	"-",	"F",	"F",	"ATAN"},
     {0xA8,	"-",	"F",	"F",	"COS"},
     {0xA9,	"-",	"F",	"F",	"DEG"},
     {0xAA,	"-",	"F",	"F",	"EXP"},
     {0xAB,	"-",	"F",	"F",	"FLT"},
     {0xAC,	"-",	"F",	"F",	"INTF"},
     {0xAD,	"-",	"F",	"F",	"LN"},
     {0xAE,	"-",	"F",	"F",	"LOG"},
     {0xAF,	"-",	"-",	"F",	"PI"},
     {0xB0,	"-",	"F",	"F",	"RAD"},
     {0xB1,	"-",	"-",	"F",	"RND"},
     {0xB2,	"-",	"F",	"F",	"SIN"},
     {0xB3,	"-",	"F",	"F",	"SQR"},
     {0xB4,	"-",	"F",	"F",	"TAN"},
     {0xB5,	"-",	"S",	"F",	"VAL"},
     {0xB6,	"-",	"-",	"F",	"SPACE"},
     {0xB7,	"-",	"S",	"S",	"DIR$"},
     {0xB8,	"-",	"I",	"S",	"CHR$"},
     {0xB9,	"-",	"-",	"S",	"DATIM$"},
     {0xBA,	"-",	"-",	"S",	"ERR$"},
     {0xBB,	"-",	"FII",	"S",	"FIX$"},
     {0xBC,	"-",	"FI",	"S",	"GEN$"},
     {0xBD,	"-",	"-",	"S",	"GET$"},
     {0xBE,	"-",	"I",	"S",	"HEX$"},
     {0xBF,	"-",	"-",	"S",	"KEY$"},
     {0xC0,	"-",	"SI",	"S",	"LEFT$"},
     {0xC1,	"-",	"S",	"S",	"LOWER$"},
     {0xC2,	"-",	"SII",	"S",	"MID$"},
     {0xC3,	"-",	"FI",	"S",	"NUM$"},
     {0xC4,	"-",	"SI",	"S",	"RIGHT$"},
     {0xC5,	"-",	"SI",	"S",	"REPT$"},
     {0xC6,	"-",	"FII",	"S",	"SCI$"},
     {0xC7,	"-",	"S",	"S",	"UPPER$"},
     {0xC8,	"-",	"II",	"S",	"USR$"},
     {0xC9,	"-",	"s",	"I",	"ADDR (string)"},
     {0xCA,	"SI",	"-",	"-",	"Used in .LNO files by the Developer Emulator to store procedure debug info."},
     {0xCB,	"II",	"-",	"-",	"Used in .LNO files by the Developer Emulator to store line and columns number of a statement."},
     {0xCC,	"-",	"FF",	"F",	"<%"},
     {0xCD,	"-",	"FF",	"F",	">%"},
     {0xCE,	"-",	"FF",	"F",	"+%"},
     {0xCF,	"-",	"FF",	"F",	"-%"},
     {0xD0,	"-",	"FF",	"F",	"*%"},
     {0xD1,	"-",	"FF",	"F",	"/%"},
     {0xD2,	"-",	"I",	"-",	"OFFX"},
     {0xD3,	"-",	"SS",	"-",	"COPYW"},
     {0xD4,	"-",	"S",	"-",	"DELETEW"},
     {0xD5,	"-",	"IIIIIIIII",	"-",	"UDG"},
     {0xD6,	"-",	"I",	"I",	"CLOCK"},
     {0xD7,	"-",	"III",	"I",	"DOW"},
     {0xD8,	"-",	"S",	"I",	"FINDW"},
     {0xD9,	"-",	"IS",	"I",	"MENUN"},
     {0xDA,	"-",	"III",	"I",	"WEEK"},
     {0xDB,	"-",	"F",	"F",	"ACOS"},
     {0xDC,	"-",	"F",	"F",	"ASIN"},
     {0xDD,	"-",	"III",	"F",	"DAYS"},
     {0xDE,	"-",	"Flist",	"F",	"MAX"},
     {0xDF,	"-",	"Flist",	"F",	"MEAN"},
     {0xE0,	"-",	"Flist",	"F",	"MIN"},
     {0xE1,	"-",	"Flist",	"F",	"STD"},
     {0xE2,	"-",	"Flist",	"F",	"SUM"},
     {0xE3,	"-",	"Flist",	"F",	"VAR"},
     {0xE4,	"-",	"I",	"S",	"DAYNAME$"},
     {0xE5,	"-",	"S",	"S",	"DIRW$"},
     {0xE6,	"-",	"I",	"S",	"MONTH$"},
    };

typedef int    (*QC_BYTE_LEN_FN)(int i, NOBJ_QCODE *qc);
typedef char * (*QC_BYTE_PRT_FN)(int i, NOBJ_QCODE *qc);

typedef struct
{
  char *code;
  QC_BYTE_LEN_FN len_fn;
  QC_BYTE_PRT_FN prt_fn;
} QC_BYTE_CODE;

// prt fn called first, then len fn

int null_qc_byte_len_fn_2(int i, NOBJ_QCODE *qc)
{
  return(2);
}

int null_qc_byte_len_fn_1(int i, NOBJ_QCODE *qc)
{
  return(1);
}

char prt_res[NOBJ_PRT_MAX_LINE];
int qc_len = 0;

char *qc_byte_prt_fn_v(int i, NOBJ_QCODE *qc)
{
  sprintf(prt_res, "\n%04X: %02X%02X       (%d)", i+1, *(qc+1), *(qc+2), (*(qc+1))*256+*(qc+2));
  return(prt_res);
}


char *qc_byte_prt_fn_V(int i, NOBJ_QCODE *qc)
{
  sprintf(prt_res, "\n%04X: %02X%02X       (%d)", i+1, *(qc+1), *(qc+2), (*(qc+1))*256+*(qc+2));
  return(prt_res);
}

char *qc_byte_prt_fn_B(int i, NOBJ_QCODE *qc)
{
  sprintf(prt_res, "\n%04X: %02X           (%d)", i+1, *(qc+1), *(qc+1));
  return(prt_res);
}

char *qc_byte_prt_fn_O(int i, NOBJ_QCODE *qc)
{
  sprintf(prt_res, "\n%04X: %02X           (%d)", i+1, *(qc+1), *(qc+1));
  return(prt_res);
}

char *qc_byte_prt_fn_f(int i, NOBJ_QCODE *qc)
{
  sprintf(prt_res, "\n%04X: %02X           (%c)", i+1, *(qc+1), 'A'+*(qc+1));
  return(prt_res);
}

char *qc_byte_prt_fn_m(int i, NOBJ_QCODE *qc)
{
  sprintf(prt_res, "\n%04X: %02X%02X       (M%d)", i+1, *(qc+1), *(qc+2), ((*(qc+1))*256+(*(qc+2)))/8 );
  return(prt_res);
}

char *qc_byte_prt_fn_I(int i, NOBJ_QCODE *qc)
{
  sprintf(prt_res, "\n%04X: %02X%02X       (%d)", i+1, *(qc+1), *(qc+2), (*(qc+1))*256+*(qc+2));
  return(prt_res);
}

char *qc_byte_prt_fn_D(int i, NOBJ_QCODE *qc)
{
  // Where we go for a distance of 0
  // Instruction is 3 bytes long and distance of 0002 takes us to next instruction.
  
  int dest = i + 3 - 2;
  int16_t  distance = (*(qc+1))*256+*(qc+2);
  
  dest += distance;
  
  sprintf(prt_res, "\n%04X: Dest:%04X (Dist:%04X)", i+1, dest, distance);

  return(prt_res);
}

char *qc_byte_prt_fn_S(int i, NOBJ_QCODE *qc)
{
  qc_len = *(++qc);
  
  char chs[2];
  chs[1] = '\0';

  sprintf(prt_res, "\n%04X: Len:%d\n%04X: '", i+1, *(qc++), i+2);
  
  for(int j=0; j<qc_len; j++)
    {
      //printf("\nj:%d", j);
      chs[0] = *(qc++);

      strcat(prt_res, chs);
    }

  strcat(prt_res, "'");

  return(prt_res);
}

int qc_byte_len_fn_S(int i, NOBJ_QCODE *qc)
{
  return(qc_len+1);
}

// Compact floating point form

char *qc_byte_prt_fn_F(int i, NOBJ_QCODE *qc)
{
  uint8_t first_byte;
   int8_t exponent;
  uint8_t sign;
  char line[100];
  char digits[30];
  
  first_byte = *(++qc);
  qc_len = first_byte & 0x7F;
  sign = first_byte & 0x80;
  
  sprintf(prt_res, "\n%04X: Len:%d\n%04X: '", i+1, *(qc++), i+2);

  line[0] = '\0';
  
  for(int j=0; j<qc_len-1; j++)
    {
      sprintf(digits, "%02X", *(qc++));
      strcat(line, digits);
    }
  
  exponent = *(qc++);
  
  sprintf(digits, "E%d", exponent);
  strcat(line, digits);

  strcat(prt_res, line);
  return(prt_res);
}

// Floating point compact format
int qc_byte_len_fn_F(int i, NOBJ_QCODE *qc)
{
  return(qc_len+1);
}

char *null_qc_byte_prt_fn(int i, NOBJ_QCODE *qc)
{
  return("");
}

int null_qc_byte_fn(int i, NOBJ_QCODE *qc)
{
  return(0);
}

QC_BYTE_CODE qc_byte_code[] =
  {
   
   {"v",      null_qc_byte_len_fn_2,      qc_byte_prt_fn_v},
   {"V",      null_qc_byte_len_fn_2,      qc_byte_prt_fn_V},
   {"-",      null_qc_byte_fn,            null_qc_byte_prt_fn  },
   {"m",      null_qc_byte_len_fn_2,      qc_byte_prt_fn_m},
   {"f",      null_qc_byte_len_fn_1,      qc_byte_prt_fn_f},
   {"I",      null_qc_byte_len_fn_2,      qc_byte_prt_fn_I},
   {"F",           qc_byte_len_fn_F,      qc_byte_prt_fn_F},
   {"S",           qc_byte_len_fn_S,      qc_byte_prt_fn_S},
   {"B",      null_qc_byte_len_fn_1,      qc_byte_prt_fn_B},
   {"O",      null_qc_byte_len_fn_1,      qc_byte_prt_fn_O},
   {"D",      null_qc_byte_len_fn_2,      qc_byte_prt_fn_D},
   {"f+list", null_qc_byte_fn,       null_qc_byte_prt_fn  },
  };

//------------------------------------------------------------------------------

void pr_uint8(uint8_t n)
{
  int l = (n & 0x00FF);

  printf("%02X", l);
}

void pr_var_space_size(NOBJ_VAR_SPACE_SIZE *x)
{
  printf("\nVar Space Size:%04X", x->size);
}

void pr_qcode_space_size(NOBJ_QCODE_SPACE_SIZE *x)
{
  printf("\nQCode Space Size:%04X", x->size);
}

void pr_global_varname_size(NOBJ_GLOBAL_VARNAME_SIZE *x)
{
  printf("\nGlobal varname Size:%04X", x->size);
}

void pr_external_varname_size(NOBJ_EXTERNAL_VARNAME_SIZE *x)
{
  printf("\nExternal varname Size:%04X", x->size);
}

void pr_num_parameters(NOBJ_NUM_PARAMETERS *x)
{
  printf("\nNumber of parameters:");
  pr_uint8(x->num);
}

void pr_parameter_types(NOBJ_PROC *p)
{
  printf("\nParameter types:");
  
  for(int i=0; i<p->num_parameters.num; i++)
    {
      printf("\n%2d %s (%d)",
	     i,
	     decode_vartype(p->parameter_types[i]),
	     p->parameter_types[i]
	     );
    }
}

////////////////////////////////////////////////////////////////////////////////

void decode_qc(int *i,  NOBJ_QCODE **qc)
{
  int found = 0;

  for(int j=0; j<(sizeof(qcode_decode)/sizeof(struct _QCODE_DESC)); j++)
    {
      
      if( qcode_decode[j].qcode == **qc )
	{
	  printf("\n%04X: %02X       %s", *i, **qc, qcode_decode[j].desc);
	  printf("     (bytes code:%s)", qcode_decode[j].bytes);
	  
	  for(int qcb = 0; qcb < (sizeof(qc_byte_code)/sizeof(QC_BYTE_CODE)); qcb++)
	    {
	      if( strcmp(qcode_decode[j].bytes, qc_byte_code[qcb].code) == 0 )
		{
		  printf("%s", (*qc_byte_code[qcb].prt_fn)(*i, *qc));
		  *qc +=       (*qc_byte_code[qcb].len_fn)(*i, *qc);
		  *i +=        (*qc_byte_code[qcb].len_fn)(*i, *qc);
		}
	    }
	  
	  found = 1;
	  break;
	}
    }
  
  if( !found )
    {
      printf("\n%04X: %02X ????", *i, **qc);
    }
}
////////////////////////////////////////////////////////////////////////////////

char *type_to_string(NOBJ_VARTYPE t)
{
  switch(t)
    {
    case NOBJ_VARTYPE_INT:
      return("Integer");
      break;

    case NOBJ_VARTYPE_FLT:
      return("Float");
      break;

    case NOBJ_VARTYPE_STR:
      return("String");
      break;

    case NOBJ_VARTYPE_INTARY:
      return("Integer Array");
      break;

    case NOBJ_VARTYPE_FLTARY:
      return("Float Array");
      break;

    case NOBJ_VARTYPE_STRARY:
      return("String Array");
      break;

    case NOBJ_VARTYPE_VAR_ADDR:
      return("Variable Address");
      break;

    case NOBJ_VARTYPE_UNKNOWN:
      return("Unknown");
      break;

    case NOBJ_VARTYPE_VOID:
      return("Void");
      break;
      
    default:
      return("????");
      break;
    }
  
  return("????");
}

////////////////////////////////////////////////////////////////////////////////
//
//
//
////////////////////////////////////////////////////////////////////////////////

void dump_proc(NOBJ_PROC *proc)
{
  printf("\nEnter:%s", __FUNCTION__);

  pr_var_space_size(&(proc->var_space_size));
  pr_qcode_space_size(&(proc->qcode_space_size));
  pr_num_parameters(&(proc->num_parameters));
  pr_parameter_types(proc);
  pr_global_varname_size(&proc->global_varname_size);

  printf("\nGlobal variables (%d)", proc->global_varname_num);
  for(int i=0; i<proc->global_varname_num; i++)
    {
      printf("\n%2d: %-16s %s (%02X) %04X",
	     i,
	     proc->global_varname[i].varname,
	     decode_vartype(proc->global_varname[i].type),
	     proc->global_varname[i].type,
	     proc->global_varname[i].address
	     );
    }
  printf("\n");

  pr_external_varname_size(&proc->external_varname_size);

  printf("\nExternal variables (%d)", proc->external_varname_num);
  for(int i=0; i<proc->external_varname_num; i++)
    {
      printf("\n%2d: %-16s %s (%02X)",
	     i,
	     proc->external_varname[i].varname,
	     decode_vartype(proc->external_varname[i].type),
	     proc->external_varname[i].type
	     );
    }
  printf("\n");

  printf("\nString length fixups (%d)", proc->strlen_fixup_num);
  
  for(int i=0; i<proc->strlen_fixup_num; i++)
    {
      printf("\n%2d: %04X %02X",
	     i,
	     proc->strlen_fixup[i].address,
	     proc->strlen_fixup[i].len
	     );
    }
  printf("\n");

  printf("\nArray size fixups (%d)", proc->arysz_fixup_num);
  
  for(int i=0; i<proc->arysz_fixup_num; i++)
    {
      printf("\n%2d: %04X %04X",
	     i,
	     proc->arysz_fixup[i].address,
	     proc->arysz_fixup[i].len
	     );
    }
  printf("\n");
  
  // Now display the QCode
  NOBJ_QCODE *qc = proc->qcode;

  printf("\nQCode\n");
  if ( qc == 0 )
    {
      printf("\nNo QCode");
    }
  else
    {
      for(int i=0; i<proc->qcode_space_size.size; qc++, i++)
	{
	  int found = 0;
	  
	  // The fields that are after certain file qcodes needs a different decode
	  switch(*qc)
	    {
	    case QCO_OPEN:
	    case QCO_CREATE:
	      decode_qc(&i, &qc);

	      // Decode the field information
	      qc++;
	      i++;
	      printf("\n      Logical file:%02X (%c)", *qc, 'A'+*qc);
	      qc++;
	      i++;
	      
	      while( (*qc) != 0x88 )
		{
		  printf("\n        Type:%02X (%s)", *qc, type_to_string(*qc));
		  qc++;
		  i++;
		  printf("\n        ");

		  int len = *(qc++);
		  i++;
		  for(int ii=0; ii<len; ii++)
		    {
		      printf("%c", *(qc++));
		      i++;
		    }
		}
	      
	      decode_qc(&i, &qc);
	      break;

	    default:
	      decode_qc(&i, &qc);
	      break;
	    }
	}
    }
      
  printf("\n");
      
  printf("\nQCode Data\n");
      
  qc = proc->qcode;
      
  for(int i=0; i<proc->qcode_space_size.size; qc++, i++)
    {
      if( (i % 16)==0 )
	{
	  printf("\n%04X:", i);
	}
	  
      printf("%02X ", *qc);
    }
      
  printf("\n");
}




int main(int argc, char *argv[])
{
  char filename[100];
  char extension[10];
  char name[100];

  strcpy(filename, argv[1]);
  sscanf(filename, "%[^.].%s", name, extension);

  if( (argc == 3) && (strcmp(argv[2], "--no-filenames")!=0),1 )
    {
      printf("\nFilename:'%s'", name);
      printf("\nFile ext:'%s", extension);
    }

  NOBJ_PROC proc;

  FILE *fp;
  
  fp = fopen(argv[1], "r");

  if( fp == NULL )
    {
      printf("\nCannot open '%s'", argv[1]);
      exit(-1);
    }

  // If this is an OB3 file then drop the header
  if( strcmp(extension, "ORG") || strcmp(extension, "OB3") == 0 )
    {
      printf("\nDropping OB3 header...");
      read_ob3_header(fp);
    }
  
  // Read the object file
  read_proc_file(fp, &proc);
  
  // Dump the proc information
  dump_proc(&proc);
  
 
 fclose(fp);
 
 
}
