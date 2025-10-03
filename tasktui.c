#include <ncurses.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TASKS 100
#define MAX_DESC 128

typedef struct {
  char desc[MAX_DESC];
  int estimate; // in minutes
} Task;

Task tasks[MAX_TASKS];
int task_count = 0;

void draw_ui(int selected) {
  clear();
  mvprintw(0, 0, "Simple Task TUI (press 'a' to add, 'q' to quit)");

  int total = 0;
  for (int i = 0; i < task_count; i++) {
    if (i == selected)
      attron(A_REVERSE);
    mvprintw(i + 2, 2, "%d. %-40s [%d min]", i + 1, tasks[i].desc,
             tasks[i].estimate);
    if (i == selected)
      attroff(A_REVERSE);
    total += tasks[i].estimate;
  }

  mvprintw(LINES - 2, 0, "Total estimate: %d min (%.2f h)", total,
           total / 60.0);
  refresh();
}

void add_task() {
  echo();
  char desc[MAX_DESC];
  int est;

  mvprintw(LINES - 4, 0, "Task description: ");
  getnstr(desc, MAX_DESC - 1);

  mvprintw(LINES - 3, 0, "Estimate (minutes): ");
  scanw("%d", &est);

  noecho();

  if (task_count < MAX_TASKS) {
    strncpy(tasks[task_count].desc, desc, MAX_DESC - 1);
    tasks[task_count].desc[MAX_DESC - 1] = '\0';
    tasks[task_count].estimate = est;
    task_count++;
  }
}

int main() {
  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);

  int selected = 0;
  int ch;

  while (1) {
    draw_ui(selected);
    ch = getch();

    switch (ch) {
    case 'q':
      endwin();
      return 0;
    case 'a':
      add_task();
      break;
    case KEY_UP:
      if (selected > 0)
        selected--;
      break;
    case KEY_DOWN:
      if (selected < task_count - 1)
        selected++;
      break;
    }
  }

  endwin();
  return 0;
}
