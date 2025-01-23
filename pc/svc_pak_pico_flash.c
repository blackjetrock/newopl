////////////////////////////////////////////////////////////////////////////////
//
// Linux pak services
//
// provides the read and save services that allow paks to be
// located on a Linux filesystem
//

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

////////////////////////////////////////////////////////////////////////////////
//
// Simple datapack on a Linux filesystem
//
// The pack is in a file and uses the Psion filesystem for EPROMs
//
//------------------------------------------------------------------------------
// Read a byte from the flash pak

uint8_t pk_rbyt_pico_flash(PAK_ADDR pak_addr)
{
  // Seek to byte and read it
  return( *((uint8_t *)(flash_pak_base_read + pak_addr)));
}

//------------------------------------------------------------------------------
//
// Write a block of data to the pak
//
// The pak needs to be pre-erased, that is done during
// formatting, so all we do here is write the data.
// Writing to the flash has to be done on 256 byte boundaries and in blocks
// of 256 bytes, so we have to read the area around the write, modify it with the new data
// and then write it back. 
// Note writes use an offset into the flash, not an address (as reads do).



void pk_save_pico_flash(PAK_ADDR pak_addr, int len, uint8_t *src)
{
  PAK_ADDR rd_blk_start, rd_blk_len;;
  PAK_ADDR dat_blk_end, dat_blk_end_blk_start;
  int dat_offset;
  PAK_ADDR start_last_blk_written;
  int write_length;
  int rd_blk_num;
  uint8_t blk_buffer[256];
  
  // Where the write will end (last byte written)
  dat_blk_end = pak_addr + len - 1;

  // Start of last block written
  start_last_blk_written = dat_blk_end / 256;

  // Where the read starts
  rd_blk_start = pak_addr / 256;
  rd_blk_len   = start_last_blk_written - rd_blk_start + 256;
  rd_blk_num   = rd_blk_len / 256;
  dat_offset   = pak_addr % 256;

#if DB_PK_SAVE  
  printf("\n%s:pak_addr:%016X len:%016X", __FUNCTION__, pak_addr, len);
  printf("\nst last blk wrttn:%016X", start_last_blk_written);
  printf("\nrd_blk_start:%016X rd_blk_len:%016X", rd_blk_start, rd_blk_len);
#endif
  
  // Now read the data , modify with new data and write it back
  // We should be able to do this a block at a time instead of having a big
  // buffer

  uint8_t *dat_src = src;
  int dest_idx = dat_offset;

  int blk_idx = 0;

#if DB_PK_SAVE
  printf("\nFLASH READ");
#endif

  // Read first block (addresses not offsets)
  memcpy( blk_buffer, ((uint8_t *)(flash_pak_base_read+rd_blk_start+blk_idx*256)), 256);
  
  while(blk_idx < rd_blk_len/256)
    {
#if DB_PK_SAVE
      printf("\nBLK %d", blk_idx);
      printf("\ndest_idx:%d dat_src:%016X", dest_idx, dat_src);
#endif

      // Put modified data in block, ensure we don't write any data after src block
      if( dat_src < (src + len) )
	{
	  blk_buffer[dest_idx++] = *(dat_src++);
	}
      else
	{
	  dest_idx++;
	}
      
      // Keep index within the block buffer
      if( dest_idx >= 256 )
	{
	  // Start of new block
	  dest_idx = 0;
#if DB_PK_SAVE
	  printf("\nFLASH WRITE");
	  printf("\nWr addr:%016X", flash_pak_base_write + rd_blk_start + blk_idx * 256);
#endif

	  // Write the block back
	  printf("\nStart blocking");
	  //	  multicore_lockout_start_blocking();
	  put_core1_into_ram();
	  sleep_ms(1000);
	  
	  printf("\nStart blocking done");
	  uint32_t ints = save_and_disable_interrupts();
	  
	  flash_range_program(flash_pak_base_write + rd_blk_start + blk_idx * 256, blk_buffer, 256);
	  printf("\nEnd blocking");
	  //multicore_lockout_end_blocking();
	  restore_interrupts (ints);
	  
	  take_core1_out_of_ram();
	  printf("\nEnd blocking done");
	  
	  blk_idx++;

	  // Read next block (addresses not offsets)
	  memcpy( blk_buffer, ((uint8_t *)(flash_pak_base_read+rd_blk_start+blk_idx*256)), 256);
	  
	}
      
    }
}

//------------------------------------------------------------------------------
//
// Format the flash pak
//
// Write a pak header and also sets the rest of the poack to FF
//

void pk_format_pico_flash(void)
{
  PAK_ID pak_id;
  
  printf("\n%s:Erasing %016X for %d bytes\n", __FUNCTION__, flash_pak_base_write, FLASH_PAK_SIZE);
  //sleep_ms(100);


  //multicore_lockout_start_blocking();

  put_core1_into_ram();

  sleep_ms(1000);
  uint32_t ints = save_and_disable_interrupts();
  flash_range_erase(flash_pak_base_write, FLASH_PAK_SIZE);
  restore_interrupts (ints);
  
  //multicore_lockout_end_blocking();
  take_core1_out_of_ram();
  


  // Now write a pak header
  pk_build_id_string(pak_id, FLASH_PAK_SIZE, 24, 8, 21, 8,  time_us_32());
  
  pk_save_pico_flash(0, 10, pak_id);
    
  printf("\nDone");
}


