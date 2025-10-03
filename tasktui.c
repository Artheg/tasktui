#include <ncurses.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#define MAX_TASKS 200
#define MAX_DESC 128

typedef struct {
  char desc[MAX_DESC];
  int estimate; // in minutes
  bool done;
} Task;

Task tasks[MAX_TASKS];
int task_count = 0;

enum Filter { FILTER_ALL, FILTER_UNDONE, FILTER_DONE };
enum Filter current_filter = FILTER_ALL;

char save_path[512];

// ---------- persistence helpers ----------
void ensure_config_dir() {
  const char *home = getenv("HOME");
  if (!home)
    home = ".";
  snprintf(save_path, sizeof(save_path), "%s/.config/tasktui", home);

  struct stat st = {0};
  if (stat(save_path, &st) == -1) {
    mkdir(save_path, 0700);
  }

  strncat(save_path, "/tasks.txt", sizeof(save_path) - strlen(save_path) - 1);
}

void save_tasks() {
  FILE *f = fopen(save_path, "w");
  if (!f)
    return;
  for (int i = 0; i < task_count; i++) {
    fprintf(f, "%d|%d|%s\n", tasks[i].done ? 1 : 0, tasks[i].estimate,
            tasks[i].desc);
  }
  fclose(f);
}

void load_tasks() {
  FILE *f = fopen(save_path, "r");
  if (!f)
    return;

  char line[256];
  while (fgets(line, sizeof(line), f)) {
    if (task_count >= MAX_TASKS)
      break;
    int done, est;
    char desc[MAX_DESC];
    if (sscanf(line, "%d|%d|%[^\n]", &done, &est, desc) == 3) {
      tasks[task_count].done = done;
      tasks[task_count].estimate = est;
      strncpy(tasks[task_count].desc, desc, MAX_DESC - 1);
      tasks[task_count].desc[MAX_DESC - 1] = '\0';
      task_count++;
    }
  }
  fclose(f);
}

// ---------- filtering ----------
bool filter_match(Task *t) {
  if (current_filter == FILTER_ALL)
    return true;
  if (current_filter == FILTER_UNDONE)
    return !t->done;
  if (current_filter == FILTER_DONE)
    return t->done;
  return true;
}

// ---------- UI ----------
void draw_ui(int selected) {
  clear();
  mvprintw(0, 0, "Simple Task TUI");

  int total = 0;
  int visible_index = 0;
  for (int i = 0; i < task_count; i++) {
    if (!filter_match(&tasks[i]))
      continue;

    if (visible_index == selected)
      attron(A_REVERSE);

    if (tasks[i].done)
      attron(COLOR_PAIR(1));
    mvprintw(visible_index + 2, 2, "%d. %-40s [%d min] %s", i + 1,
             tasks[i].desc, tasks[i].estimate, tasks[i].done ? "(done)" : "");
    if (tasks[i].done)
      attroff(COLOR_PAIR(1));

    if (visible_index == selected)
      attroff(A_REVERSE);
    if (!tasks[i].done)
      total += tasks[i].estimate;

    visible_index++;
  }

  mvprintw(LINES - 3, 0, "Total remaining estimate: %d min (%.2f h)", total,
           total / 60.0);
  const char *fstr = (current_filter == FILTER_ALL      ? "All"
                      : current_filter == FILTER_UNDONE ? "Undone"
                                                        : "Done");
  mvprintw(LINES - 2, 0, "Filter: %s", fstr);

  mvprintw(
      LINES - 1, 0,
      "Keys: a=add  e=edit  x=toggle  d=delete  j/k=move  /=filter  q=quit");
  refresh();
}

// ---------- core ops ----------
void add_task() {
  echo();
  char desc[MAX_DESC];
  int est;

  mvprintw(LINES - 5, 0, "Task description: ");
  clrtoeol();
  getnstr(desc, MAX_DESC - 1);

  mvprintw(LINES - 4, 0, "Estimate (minutes): ");
  clrtoeol();
  scanw("%d", &est);

  noecho();

  if (task_count < MAX_TASKS) {
    strncpy(tasks[task_count].desc, desc, MAX_DESC - 1);
    tasks[task_count].desc[MAX_DESC - 1] = '\0';
    tasks[task_count].estimate = est;
    tasks[task_count].done = false;
    task_count++;
  }
  save_tasks();
}

void edit_task(int idx) {
  if (idx < 0)
    return;

  int visible_index = 0;
  for (int i = 0; i < task_count; i++) {
    if (!filter_match(&tasks[i]))
      continue;
    if (visible_index == idx) {
      echo();
      char desc[MAX_DESC];
      char est_str[32];

      mvprintw(LINES - 6, 0, "Edit description (current: %s): ", tasks[i].desc);
      clrtoeol();
      getnstr(desc, MAX_DESC - 1);

      mvprintw(LINES - 5, 0,
               "Edit estimate (minutes, current: %d): ", tasks[i].estimate);
      clrtoeol();
      getnstr(est_str, sizeof(est_str) - 1);

      noecho();

      if (strlen(desc) > 0) {
        strncpy(tasks[i].desc, desc, MAX_DESC - 1);
        tasks[i].desc[MAX_DESC - 1] = '\0';
      }
      if (strlen(est_str) > 0) {
        int new_est = atoi(est_str);
        if (new_est > 0)
          tasks[i].estimate = new_est;
      }

      save_tasks();
      return;
    }
    visible_index++;
  }
}

void delete_task(int idx) {
  if (idx < 0)
    return;

  int visible_index = 0;
  for (int i = 0; i < task_count; i++) {
    if (!filter_match(&tasks[i]))
      continue;
    if (visible_index == idx) {
      for (int j = i; j < task_count - 1; j++) {
        tasks[j] = tasks[j + 1];
      }
      task_count--;
      save_tasks();
      return;
    }
    visible_index++;
  }
}

void toggle_done(int idx) {
  if (idx < 0)
    return;

  int visible_index = 0;
  for (int i = 0; i < task_count; i++) {
    if (!filter_match(&tasks[i]))
      continue;
    if (visible_index == idx) {
      tasks[i].done = !tasks[i].done;
      save_tasks();
      return;
    }
    visible_index++;
  }
}

int count_visible_tasks() {
  int c = 0;
  for (int i = 0; i < task_count; i++) {
    if (filter_match(&tasks[i]))
      c++;
  }
  return c;
}

// ---------- main ----------
int main() {
  ensure_config_dir();
  load_tasks();

  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  start_color();
  init_pair(1, COLOR_GREEN, COLOR_BLACK); // done tasks in green

  int selected = 0;
  int ch;

  while (1) {
    int visible_count = count_visible_tasks();
    if (selected >= visible_count)
      selected = visible_count - 1;
    if (selected < 0)
      selected = 0;

    draw_ui(selected);
    ch = getch();

    switch (ch) {
    case 'q':
      save_tasks();
      endwin();
      return 0;
    case 'a':
      add_task();
      break;
    case 'e':
      edit_task(selected);
      break;
    case 'd':
      delete_task(selected);
      break;
    case 'x':
      toggle_done(selected);
      break;
    case 'j':
    case KEY_DOWN:
      if (selected < visible_count - 1)
        selected++;
      break;
    case 'k':
    case KEY_UP:
      if (selected > 0)
        selected--;
      break;
    case '/': // cycle filter
      current_filter = (current_filter + 1) % 3;
      selected = 0;
      break;
    }
  }

  endwin();
  return 0;
}
