////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <string.h>
#include <ncurses.h>
#include <termio.h>
#include <unistd.h>

#include "nopl.h"

////////////////////////////////////////////////////////////////////////////////

WINDOW *variable_win;
WINDOW *memory_win;
WINDOW *machine_win;

WINDOW *create_win(int h, int w, int y, int x)
{
  WINDOW *win;
  
  win = newwin(h, w, y, x);
  //box(win, '*' , '*');
  wborder(win, 0,0,0,0,0,0,0,0);
  wrefresh(win);

  delwin(win);
  win = newwin(h-2, w-2, y+1, x+1);
  
  return(win);
}

int scr_maxx, scr_maxy;

void tui_init(void)
{
  //initscr();  
  //timeout(1);
  printf("\n%s", __FUNCTION__);
  
  initscr();

  //cbreak();
  //noecho();
  //nodelay(stdscr, TRUE);

  clear();

  printw("main");

  getmaxyx(stdscr, scr_maxy, scr_maxx);
  
  refresh();
  
  variable_win = create_win(scr_maxy/2-1, scr_maxx/2-1, 0, scr_maxx/2);
  memory_win   = create_win(scr_maxy/3-1, scr_maxx/2-1, 0, 0);
  machine_win  = create_win(scr_maxy/4-1, scr_maxx/2-1, scr_maxy/2, 0);
  scrollok(memory_win, TRUE);
  scrollok(variable_win, TRUE);
    
  wprintw(variable_win, "variables");
  wrefresh(variable_win);

  wprintw(machine_win, "Machine");
  wrefresh(machine_win);
	  
  wprintw(memory_win,"memory");
  wrefresh(memory_win);
  printf("\n%s", __FUNCTION__);
  

}

void tui_end(void)
{
  printf("\n%s", __FUNCTION__);
  
  delwin(variable_win);
  delwin(memory_win);
  endwin();
  printf("\n%s, (cols:%d,rows:%d)", __FUNCTION__, scr_maxx, scr_maxy);
}
#if 0
bool kbhit(void)
{
    struct termios original;
    tcgetattr(STDIN_FILENO, &original);

    struct termios term;
    memcpy(&term, &original, sizeof(term));

    term.c_lflag &= ~ICANON;
    tcsetattr(STDIN_FILENO, TCSANOW, &term);

    int characters_buffered = 0;
    ioctl(STDIN_FILENO, FIONREAD, &characters_buffered);

    tcsetattr(STDIN_FILENO, TCSANOW, &original);

    bool pressed = (characters_buffered != 0);

    return pressed;
}
#endif
void echoOff(void)
{
    struct termios term;
    tcgetattr(STDIN_FILENO, &term);

    term.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

void echoOn(void)
{
    struct termios term;
    tcgetattr(STDIN_FILENO, &term);

    term.c_lflag |= ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

////////////////////////////////////////////////////////////////////////////////
//
// Keyboard

#define NUM_KEYS 35

struct
{
  char c;
  int k;
  int p5;
} keys[NUM_KEYS] =
  {
   {'a', 2, 6},
   {'b', 3, 6},
   {'c', 4, 6},
   {'d', 7, 6},
   {'e', 5, 6},
   {'f', 6, 6},
   {'g', 2, 5},
   {'h', 3, 5},
   {'i', 4, 5},
   {'j', 7, 5},
   {'k', 5, 5},
   {'l', 6, 5},
   {'m', 2, 4},
   {'n', 3, 4},
   {'o', 4, 4},
   {'p', 7, 4},
   {'q', 5, 4},
   {'r', 6, 4},
   {'s', 2, 3},
   {'t', 3, 3},
   {'u', 4, 3},
   {'v', 7, 3},
   {'w', 5, 3},
   {'x', 6, 3},
   {'y', 4, 2},
   {'z', 7, 2},
   {' ', 5, 2},
   { 10, 6, 2},  // EXE
   {'^', 2, 2},  // SHIFT
   {'[', 3, 2},  // DEL
   {'!', 1, 2},  // Mode
   {'+', 1, 3},  // UP
   {'-', 1, 4},  // DOWN
   {'<', 1, 5},
   {'>', 1, 6},

  };

////////////////////////////////////////////////////////////////////////////////

void display_stack(NOBJ_MACHINE *m)
{
  wprintw(memory_win, "\n%04X ", m->rta_sp);
  for(int i=0; i<8; i++)
    {
      wprintw(memory_win, "%04X ", m->stack[m->rta_sp+i]);
    }
  wrefresh(memory_win);
}

void tui_display_machine(NOBJ_MACHINE *m)
{
  wclear(machine_win);
  wprintw(machine_win, "\nrta_fp:%04X ", m->rta_fp);
  wprintw(machine_win, "\nrta_sp:%04X ", m->rta_sp);

  wrefresh(machine_win);
}

NOPL_FLOAT tui_num_from_mem(uint8_t *mp)
{
  NOPL_FLOAT n;
  int b;

  n.sign = *(mp++);
  n.exponent = *(mp++);
  
  // Pop digits
  for(int i = (NUM_MAX_DIGITS/2)-1; i>=0; i--)
    {
      b = *(mp++);
      n.digits[i*2] = b >> 4;
      n.digits[i*2+1] = b & 0xF;
    }

  return(n);
}



//------------------------------------------------------------------------------

void tui_display_variables(NOBJ_MACHINE *m)
{
  FILE *fp;
  char line[300];
  char localglobal[20];
  char class[50];
  char type[50];
  char ref[50];
  int  max_string;
  int  max_array;
  int  num_indices;
  int  offset;
  
  int i;
  char varname[50];
  
  fp = fopen("vars.txt", "r");

  if( fp == NULL )
    {
      return;
    }

  while(!feof(fp) )
    {
      int16_t mp;
      
      fgets(line, sizeof(line), fp);

      if( sscanf(line, "%d: VAR: ' %[^']' %s %s %s max_str: %d max_ary: %d num_ind: %d offset:%X", &i, varname, class, type, ref, &max_string, &max_array, &num_indices, &offset) == 9 )
	{
	  mp = offset+m->rta_fp;
	  if( strcmp(type, "Integer")==0 )
	    {
	      wprintw(variable_win, "\n%s (Addr:%04X) %04X", varname, mp, (m->stack[mp])*256+(m->stack[mp+1]));
	    }
	  
	  if( strcmp(type, "Float")==0 )
	    {
	      NOPL_FLOAT num;
	      mp = offset+m->rta_fp;
	      num = tui_num_from_mem(&(m->stack[mp]));
	      wprintw(variable_win, "\n%s %s", varname, num_as_text(&num, ""));
#if 0
	      wprintw(variable_win, "\n%s", varname);
	      
	      if(m->stack[mp++])
		{
		  wprintw(variable_win, "-");
		}
	      else
		{
		  wprintw(variable_win, " ");
		}
	      
	      int exponent = m->stack[mp++];
	      
	      wprintw(variable_win, "%d.", m->stack[mp++]);
	      
	      for(int i=1; i<NUM_MAX_DIGITS; i++)
		{
		  wprintw(variable_win, "%d", m->stack[mp++]);
		}
	      
	      wprintw(variable_win, " E%d", exponent);
#endif
	    }
	}
    }

  fclose(fp);

  wrefresh(variable_win);
}

//------------------------------------------------------------------------------

void tui_step(NOBJ_MACHINE *m, int *done)
{
  int donek = 0;
  
  while(!donek)
    {
      int c;
      
      display_stack(m);
      tui_display_machine(m);
      tui_display_variables(m);
      
      c = wgetch(stdscr);
      
      // Display variables
      switch(c)
	{
	case 'q':
	  *done = 1;
	  donek = 1;
	  break;

	case '\n':
	  donek = 1;
	  break;
	  
	case 'm':
	  display_stack(m);
	  break;

	case 'M':
	  tui_display_machine(m);
	  break;
	  
	case 'v':
	  tui_display_variables(m);
	  break;
	}
    }
}

////////////////////////////////////////////////////////////////////////////////
