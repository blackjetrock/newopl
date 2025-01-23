////////////////////////////////////////////////////////////////////////////////
//
// Linux Pack Services
//
// 
//

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "nopl.h"

////////////////////////////////////////////////////////////////////////////////
//


// Info for each logical drive

LINUX_FILE_INFO linux_file_info[NOPL_NUM_LOGICAL_FILES];

#define LINFI linux_file_info[logfile]

////////////////////////////////////////////////////////////////////////////////
//
// Pack is in a file and uses the Psion file system
//
//------------------------------------------------------------------------------
// Read a byte from the flash pak
//
////////////////////////////////////////////////////////////////////////////////


#define PACK_FN "datapack.bin"
#define FLASH_PAK_SIZE    ((uint32_t)(1024*1024*4))

void pk_open_linux(int logfile, char *filename)
{
  LINFI.fp = fopen(filename, "r+");
}

void pk_close_linux(int logfile, char *filename)
{
  if( LINFI.fp != NULL )
    {
      fclose(LINFI.fp);
    }
}


uint8_t pk_rbyt_linux(PAK_ADDR pak_addr)
{
  uint8_t byte;
  FILE *pfp;

  pfp = fopen(PACK_FN, "r");
  if( pfp == NULL )
    {
      return(0);
    }
  
  fseek(pfp, pak_addr, SEEK_SET);
  fread(&byte, 1, 1, pfp);

  fclose(pfp);
}

//------------------------------------------------------------------------------
//
// Write a block of data to the pak
//

void pk_save_linux(PAK_ADDR pak_addr, int len, uint8_t *src)
{
  FILE *pfp;

  pfp = fopen(PACK_FN, "r");
  if( pfp == NULL )
    {
      return;
    }

  fseek(pfp, pak_addr, SEEK_SET);
  fwrite(src, len, 1, pfp);

  fclose(pfp);
}

//------------------------------------------------------------------------------
//
// Format the flash pak
//
// Write a pak header and also sets the rest of the pack to FF
//

void pk_format_linux(void)
{
  PAK_ID  pak_id;
  FILE    *pfp;
  uint8_t byte = 0xFF;
  
  pfp = fopen(PACK_FN, "r");
  if( pfp == NULL )
    {
      return;
    }
  
  printf("\n%s:Formatting PAK_SIZE:%X\n", __FUNCTION__, FLASH_PAK_SIZE);

  // Write 0xff to the entire pack
  for(int i=0; i< FLASH_PAK_SIZE;i++)
    {
      pk_save_linux(i, 1, &byte);
    }
  
  // Now write a pak header
  pk_build_id_string(pak_id, FLASH_PAK_SIZE, 24, 8, 21, 8,  0xaabbccdd);
  
  pk_save_linux(0, 10, pak_id);
    
  printf("\nDone");

  fclose(pfp);
}


