

#include "nopl.h"

#define MENU_DB 0

////////////////////////////////////////////////////////////////////////////////
//
// Uses menustr to put a menu on the display, returns the index of
// the selected item, or -1 if menu was exited

int mn_menu(char *str)
{
  char sels[MAX_NOPL_MENU_SELS];
  char *sp;
  int num_sels = 0;
  int strx, stry;

#if TUI
  //getyx(output_win, stry, strx);
 
  // Display menu string

  //wprintw(output_win, "%s\n", str);

  //wrefresh(output_win);
#else
#if MENU_DB
  printf("\n%s\n", str);
#endif
#endif
  
  // Find the selection letters
  sp = str;
  //  sels[num_sels++] = *sp;
  
  int i=0;
  int ipos[32];
  int iposx[32];
  int iposy[32];
  char selchar[32];
  
  char frag[2] = " ";

  // Find each of the menu items, print them and store their first characters
  // and the cursor positions of those characters.
  
  while( *sp != '\0')
    {
      if( (i==0) || (*sp == ',') || (*sp == ' ') )
	{
	  if( i==0 )
	    {
	      selchar[num_sels] = *(sp);
	      sels[num_sels] = *(sp);
	    }
	  else
	    {
	      selchar[num_sels] = *(sp+1);
	      sels[num_sels] = *(sp+1);	      
	    }

#if TUI
	  getyx(output_win, iposy[num_sels], iposx[num_sels]);
#endif
	  frag[0] = *sp;
#if TUI
	  wprintw(output_win, "%s", frag);
#else
	  printf("%s", frag);
#endif
	  num_sels++;
	}
      else
	{
	  frag[0] = *sp;
#if TUI
	  wprintw(output_win, "%s", frag);
#else
	  printf("%s", frag);
#endif
	}
      
      sp++;
      i++;
    }

#if TUI
  wprintw(output_win, "\n");
  wrefresh(output_win);
#else
  printf("\n");
#endif
  
#if MENU_DB
  // Dump all the data gathered
  printf("\nStr:'%s'", str);
  
  for(int s=0; s<num_sels; s++)
    {
      printf("\n%02i:selchar:'%c' X,y:%d,%d", s,  selchar[s], iposx[s], iposy[s]);
    }
  printf("\n...\n");

#endif
  
  // Wait for one of the selection letters to be pressed
  int done = 0;
  int selnum = 0;
  int key;

  keypad(stdscr, TRUE);

  while(!done)
    {
#if TUI
      mvprintw(stry, strx+ipos[selnum], "");
      wrefresh(output_win);
#endif
      
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
	  for(int i=selnum; (i!=selnum-1)&&!done; i=((i+1) % num_sels))
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
