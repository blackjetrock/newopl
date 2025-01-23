////////////////////////////////////////////////////////////////////////////////
//
// Linux Pack Services
//
// 
//

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

////////////////////////////////////////////////////////////////////////////////
//
// Pack is in a file and uses the Psion file system
//
//------------------------------------------------------------------------------
// Read a byte from the flash pak
//
////////////////////////////////////////////////////////////////////////////////


#define PACK_FN "datapack.bin"

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

void pk_save_pico_flash(PAK_ADDR pak_addr, int len, uint8_t *src)
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

void pk_format_pico_flash(void)
{
  PAK_ID  pak_id;
  FILE    *pfp;
  uint8_t byte = 0xFF;
  
  pfp = fopen(PACK_FN, "r");
  if( pfp == NULL )
    {
      return;
    }
  
  printf("\n%s:Formatting\n", __FUNCTION__, flash_pak_base_write, FLASH_PAK_SIZE);

  // Write 0xff to the entire pack
  for(int i=0; i< FLASH_PAK_SIZE)
    {
      pk_save_pico_flash(i, 1, &byte)
    }
  
  // Now write a pak header
  pk_build_id_string(pak_id, FLASH_PAK_SIZE, 24, 8, 21, 8,  time_us_32());
  
  pk_save_pico_flash(0, 10, pak_id);
    
  printf("\nDone");

  fclose(pfp);
}


