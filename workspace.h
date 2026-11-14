#ifndef WORKSPACE_H
#define WORKSPACE_H

#include <stdbool.h>
#include <windows.h>

#define NUM_WORKSPACES 9
#define MAX_WINDOWS 256

typedef struct {
  HWND windows[MAX_WINDOWS];
  int count;
  HWND last_focused;
} Workspace;

extern Workspace g_ws[];

int find_window_ws(HWND hwnd);

bool ws_is_switching(void);
void ws_set_is_switching(bool val);

int ws_prev(void);
int ws_current(void);
void ws_set_current(int ws);

void ws_add(int ws, HWND hwnd);
void ws_remove(int ws, HWND hwnd);

#endif // WORKSPACE_H
