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

  refresh();
  
  variable_win = create_win(10, 20, 0, 40);
  memory_win = create_win(10, 20, 0, 0);

  wprintw(variable_win, "variables");
  wrefresh(variable_win);
  wprintw(memory_win,"memory");
  wrefresh(memory_win);
  printf("\n%s", __FUNCTION__);
  
    //scrollok(stdscr, TRUE);
}

void tui_end(void)
{
  printf("\n%s", __FUNCTION__);
  
  delwin(variable_win);
  delwin(memory_win);
  endwin();
  printf("\n%s", __FUNCTION__);
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

void tui_step(NOBJ_MACHINE *m, int *done)
{
  
  int c = wgetch(stdscr);

  // Display variables
  switch(c)
    {
    case 'q':
      *done = 1;
      break;

    case 'm':
      wprintw(memory_win, "\n%04X ", m->rta_sp);
      for(int i=0; i<8; i++)
	{
	  wprintw(memory_win, "%04X ", m->stack[m->rta_sp+i]);
	}
      break;
    }
  
  wrefresh(memory_win);
}

////////////////////////////////////////////////////////////////////////////////
