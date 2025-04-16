////////////////////////////////////////////////////////////////////////////////
// 
// Pack Services
//

// Packs are different to the original, but the same format is used as
// it works well in RAM chips and flash.
//
// A:Pico flash
// B:Recreation serial EEPROM
//
// Linux version has A,B,C and D as linux file system calls
// Each file is in a different file.

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "nopl.h"

#define DEBUG 0
#define DB_PK_SETP 0

////////////////////////////////////////////////////////////////////////////////

PAK_ID pkt_id = {0,0,0,0,0,0,0,0,0,0};

PK_DRIVER_SET pk_drivers[] =
  {
#if 0
   {pk_rbyt_pico_flash,     pk_save_pico_flash,      pk_format_pico_flash},
   {pk_rbyt_serial_eeprom,  pk_save_serial_eeprom,   pk_format_serial_eeprom},
#else
   {pk_open_linux, pk_create_linux,    pk_close_linux, pk_exist_linux, pk_rbyt_linux,          pk_save_linux,           pk_format_linux},
   {pk_open_linux, pk_create_linux,    pk_close_linux, pk_exist_linux, pk_rbyt_linux,          pk_save_linux,           pk_format_linux},
   {pk_open_linux, pk_create_linux,    pk_close_linux, pk_exist_linux, pk_rbyt_linux,          pk_save_linux,           pk_format_linux},
   {pk_open_linux, pk_create_linux,    pk_close_linux, pk_exist_linux, pk_rbyt_linux,          pk_save_linux,           pk_format_linux},
#endif
  };

////////////////////////////////////////////////////////////////////////////////
//
// Build a pak ID string
//

void pk_build_id_string(uint8_t *id,
			int size_in_bytes,
			int year,
			int month,
			int day,
			int hour,
			int32_t unique)
{
  uint16_t csum = 0;
  
  id[0] = 0x78;
  id[1] = (size_in_bytes / 8192) & 0xFF;
  id[2] = year;
  id[3] = month;
  id[4] = day;
  id[5] = hour;
  id[6] = (unique & 0x00FF) >> 0;
  id[7] = (unique & 0xFF00) >> 8;

  csum += id[0] << 8;
  csum += id[1];
  csum += id[2] << 8;
  csum += id[3];
  csum += id[4] << 8;
  csum += id[5];
  csum += id[6] << 8;
  csum += id[7];
  
  id[8] = (csum & 0x00FF) >> 0;
  id[9] = (csum & 0xFF00) >> 8;
}

//------------------------------------------------------------------------------

int pk_valid_pak(PAK pak)
{
  if( (pak>=0) && (pak <=1) )
    {
      return(1);
    }

  return(0);
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void pk_setp(PAK pak)
{
  if ( !pk_valid_pak(pak) )
    {
      runtime_error(ER_PK_IV, "Bad pak");
      return;
    }
  
  // Set up current pak
  pkb_curp = pak;
  pkw_cpad = 0;
  
  // We now switch the drivers to point to the appropriate hardware
  
  // Check the header and see if it is valid
#if DB_PK_SETP
  printf("\n%s:\n", __FUNCTION__);
#endif
  
}

void pk_save(int len, uint8_t *src)
{
#if DEBUG
  printf("\n%s:%08X %d", __FUNCTION__, pkw_cpad, len);
#endif
  
  (*pk_drivers[pkb_curp].save)(pkw_cpad, len, src);

  // Update the current address
  pkw_cpad += len;
}

////////////////////////////////////////////////////////////////////////////////
//
// Read bytes from pak

void pk_read(int len, uint8_t *dest)
{
  for(int i=0; i<len; i++)
    {
      *(dest++) = pk_rbyt();
    }
}

uint8_t pk_rbyt(void)
{
  // Branch to the pak drivers
  return( (*pk_drivers[pkb_curp].rbyt)(pkw_cpad++));
}


uint16_t pk_rwrd(void)
{
  uint16_t word = 0;

  word  = pk_rbyt() << 8;
  word |= pk_rbyt();
  
}

void pk_skip(int len)
{
  // Not needed as we have no address counters
  pkw_cpad += len;
}

////////////////////////////////////////////////////////////////////////////////

int pk_qadd(void)
{
  return(pkw_cpad); 
}

////////////////////////////////////////////////////////////////////////////////

void pk_sadd(int addr)
{
  pkw_cpad = addr;
}

////////////////////////////////////////////////////////////////////////////////

void pk_pkof(void)
{
  // Do nothing for now
}

////////////////////////////////////////////////////////////////////////////////

void pk_open(int logfile)
{
  char *filename = logical_file_info[logfile].name;

  dbq("Opening '%s'", filename);
  
  // Branch to the pak drivers
  return( (*pk_drivers[pkb_curp].open)(logfile, filename+2));
}

//------------------------------------------------------------------------------

void pk_create(int logfile)
{
  char *filename = logical_file_info[logfile].name;

  dbq("Creating '%s'", filename);
  
  // Branch to the pak drivers
  return( (*pk_drivers[pkb_curp].create)(logfile, filename+2));
}

////////////////////////////////////////////////////////////////////////////////

void pk_close(int logfile)
{
  char *filename = logical_file_info[logfile].name;

  dbq("Closing '%s'", filename);
  
  // Branch to the pak drivers
  return( (*pk_drivers[pkb_curp].close)(logfile, filename+2));
}

////////////////////////////////////////////////////////////////////////////////

int pk_exist(char *filename)
{
  //  char *filename = logical_file_info[logfile].name;
  
  // Branch to the pak drivers
  return( (*pk_drivers[pkb_curp].exist)(filename+2));
}

////////////////////////////////////////////////////////////////////////////////

void pk_fmat(int logfile)
{
  // Branch to the pak drivers
  return( (*pk_drivers[pkb_curp].format)(logfile));
}

////////////////////////////////////////////////////////////////////////////////
