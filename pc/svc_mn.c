

#include "nopl.h"


////////////////////////////////////////////////////////////////////////////////
//
// Uses menustr to put a menu on the display, returns the index of
// the selected item, or -1 if menu was exited

int mn_menu(char *str)
{
  char sels[MAX_NOPL_MENU_SELS];
  char *sp;
  int num_sels = 0;

  // Display menu string
#if TUI
  wprintw(output_win, "%s\n", str);
  wrefresh(output_win);
#else
  printf("\n%s\n", str);
#endif
  
  // Find the selection letters
  sp = str;
  sels[num_sels++] = *sp;
  
  while( *sp != '\0')
    {
      if( *sp == ',' )
	{
	  sels[num_sels++] = *(sp+1);
	}
      sp++;
    }

#if 0
  printf("\n%d sels\n", num_sels);
  
  for(int i=0; i<num_sels; i++)
    {
      printf("\n%i:%c", i, sels[i]);
    }
  printf("\n");
#endif
  
  // Wait for one of the selection letters to be pressed
  int done = 0;
  int selnum = 0;
  int key;

  keypad(stdscr, TRUE);

  while(!done)
    {
#if TUI
      key = wgetch(stdscr);
#else
      key = fgetc(stdin);
#endif
      
      switch(key)
	{
	case ERR:
	  break;
	  
	case 27:
	  selnum = 0;
	  done = 1;
	  break;

	default:
#if 0
	  wprintw(output_win, "\nKey:%02X", key);
	  wrefresh(output_win);
#endif
	  for(int i= 0; i<num_sels; i++)
	    {
	      if( key == sels[i] )
		{
		  selnum = i;
		  done = 1;
		}
	    }
	  break;
	}
    }
  
  nodelay(stdscr, 0);
  
  return(selnum);
}
