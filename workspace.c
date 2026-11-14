#include <dwmapi.h>
#include <stdbool.h>
#include <windows.h>
#include <winuser.h>

#include "workspace.h"

Workspace g_ws[NUM_WORKSPACES];
static int g_current_ws = 0;
static int g_prev_ws = 0;
static bool g_is_switching = false;

int find_window_ws(HWND hwnd) {
  for (int w = 0; w < NUM_WORKSPACES; w++) {
    for (int i = 0; i < g_ws[w].count; i++) {
      if (g_ws[w].windows[i] == hwnd)
        return w;
    }
  }
  return -1;
}

bool ws_is_switching(void) { return g_is_switching; }
void ws_set_is_switching(bool val) { g_is_switching = val; }

int ws_prev(void) { return g_prev_ws; }
int ws_current(void) { return g_current_ws; }
void ws_set_current(int ws) {
  if (ws < 0 || ws >= NUM_WORKSPACES)
    return;

  g_prev_ws = g_current_ws;
  g_current_ws = ws;
}

void ws_add(int ws, HWND hwnd) {
  if (ws < 0 || ws >= NUM_WORKSPACES)
    return;
  if (find_window_ws(hwnd) != -1)
    return;
  if (g_ws[ws].count < MAX_WINDOWS) {
    g_ws[ws].windows[g_ws[ws].count++] = hwnd;
  }
}

void ws_remove(int ws, HWND hwnd) {
  if (ws < 0 || ws >= NUM_WORKSPACES)
    return;
  for (int i = 0; i < g_ws[ws].count; i++) {
    if (g_ws[ws].windows[i] == hwnd) {
      g_ws[ws].windows[i] = g_ws[ws].windows[--g_ws[ws].count];
      if (g_ws[ws].last_focused == hwnd)
        g_ws[ws].last_focused = NULL;
      break;
    }
  }
}
