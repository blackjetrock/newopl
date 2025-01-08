////////////////////////////////////////////////////////////////////////////////

#include <ncurses.h>
#include <termio.h>
#include <unistd.h>
#include "nopl.h"

////////////////////////////////////////////////////////////////////////////////

WINDOW *variable_win;
WINDOW *memory_win;

WINDOW *create_win(int h, int w, int y, int x)
{
  WINDOW *win;
  
  win = newwin(h, w, y, x);
  //box(win, '*' , '*');
  wborder(win, 0,0,0,0,0,0,0,0);

  wrefresh(win);
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
  
  variable_win = create_win(scr_maxy/2-5, scr_maxx/2-5, 0, scr_maxx/2);
  memory_win   = create_win(scr_maxy/2-5, scr_maxx/2-5, 0, 0);
  scrollok(memory_win, TRUE);
  
  wprintw(variable_win, "variables");

  wrefresh(variable_win);

	  
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

//------------------------------------------------------------------------------

void tui_display_variables(NOBJ_MACHINE *m)
{
  FILE *fp;
  char line[300];

  int i;
  char varname[50];
  
  fp = fopen("vars.txt", "r");

  if( fp == NULL )
    {
      return;
    }

  while(!feof(fp) )
    {
      fgets(line, sizeof(line), fp);

      if( sscanf(line, "%d: VAR: ' %[^']'", &i, varname) == 2 )
	{
	  wprintw(variable_win, "\n%s ", varname);
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
      int c = wgetch(stdscr);
      
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
	  
	case 'v':
	  tui_display_variables(m);
	  break;
	}
      

    }
}

////////////////////////////////////////////////////////////////////////////////
