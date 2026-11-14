#include <dwmapi.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>

#ifndef MOD_NOREPEAT
#define MOD_NOREPEAT 0x4000
#endif

#include "workspace.h"

static void set_dwm_transitions_disabled(HWND hwnd, bool disabled) {
  BOOL val = disabled ? TRUE : FALSE;
  DwmSetWindowAttribute(hwnd, DWMWA_TRANSITIONS_FORCEDISABLED, &val,
                        sizeof(val));
}

static int find_owner_ws(HWND hwnd) {
  HWND cur = GetWindow(hwnd, GW_OWNER);
  for (int depth = 0; cur && depth < 16; depth++) {
    int ws = find_window_ws(cur);
    if (ws != -1)
      return ws;
    cur = GetWindow(cur, GW_OWNER);
  }
  return -1;
}

static bool is_manageable_window(HWND hwnd) {
  if (!hwnd || !IsWindow(hwnd) || !IsWindowVisible(hwnd))
    return false;
  if (hwnd == GetShellWindow() || hwnd == GetDesktopWindow())
    return false;

  LONG_PTR style = GetWindowLongPtrA(hwnd, GWL_STYLE);
  if (style & WS_CHILD)
    return false;

  LONG_PTR ex_style = GetWindowLongPtrA(hwnd, GWL_EXSTYLE);
  if ((ex_style & WS_EX_TOOLWINDOW) && !(ex_style & WS_EX_APPWINDOW))
    return false;
  if (ex_style & WS_EX_NOACTIVATE)
    return false;

  // skip cloaked windows bg uwp apps
  DWORD cloaked = 0;
  if (SUCCEEDED(DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &cloaked,
                                      sizeof(cloaked)))) {
    if (cloaked != 0)
      return false;
  }

  // skip shell ui
  char cls[64];
  if (GetClassNameA(hwnd, cls, sizeof(cls)) > 0) {
    if (strcmp(cls, "Shell_TrayWnd") == 0 ||
        strcmp(cls, "Shell_SecondaryTrayWnd") == 0 ||
        strcmp(cls, "Progman") == 0 || strcmp(cls, "WorkerW") == 0)
      return false;
  }

  return true;
}

static BOOL CALLBACK enum_init_proc(HWND hwnd, LPARAM lParam) {
  (void)lParam;
  if (is_manageable_window(hwnd))
    ws_add(0, hwnd);
  return TRUE;
}

static void CALLBACK win_event_proc(HWINEVENTHOOK hook, DWORD event, HWND hwnd,
                                    LONG idObject, LONG idChild, DWORD thread,
                                    DWORD time) {
  (void)hook;
  (void)thread;
  (void)time;

  // ignore buttons/menu/etc
  if (idObject != OBJID_WINDOW || idChild != CHILDID_SELF || !hwnd)
    return;
  if (ws_is_switching())
    return;

  if (event == EVENT_OBJECT_DESTROY) {
    int ws = find_window_ws(hwnd);
    if (ws != -1) {
      ws_remove(ws, hwnd);
    }
    return;
  }

  if (event == EVENT_OBJECT_SHOW) {
    int ws = find_window_ws(hwnd);

    if (ws != -1) {
      // a window from another workspace tried to SH_SHOW itself
      if (ws != ws_current()) {
        set_dwm_transitions_disabled(hwnd, true);
        ShowWindow(hwnd, SW_HIDE);
        printf("blocked window (%p) from another workspace (%d) from showing\n",
               (void *)hwnd, ws + 1);
      }
    } else if (is_manageable_window(hwnd)) {
      // new window opened while on current workspace
      int target_ws = find_owner_ws(hwnd);
      if (target_ws == -1)
        target_ws = ws_current();
      ws_add(target_ws, hwnd);
      if (target_ws != ws_current()) {
        set_dwm_transitions_disabled(hwnd, true);
        ShowWindow(hwnd, SW_HIDE);
        printf("routed owned window (%p) to workspace (%d)\n", (void *)hwnd,
               target_ws + 1);
      } else {
        printf("added new window (%p) to current workspace (%d)\n",
               (void *)hwnd, target_ws + 1);
      }
    }
  }
}

static void switch_workspace(int target) {
  if (target < 0 || target >= NUM_WORKSPACES || target == ws_current())
    return;

  ws_set_is_switching(true);

  // save current fg
  HWND fg = GetForegroundWindow();
  if (fg && find_window_ws(fg) == ws_current()) {
    g_ws[ws_current()].last_focused = fg;
  }

  // hide all windows in current workspace
  for (int i = g_ws[ws_current()].count - 1; i >= 0; i--) {
    HWND h = g_ws[ws_current()].windows[i];
    if (!IsWindow(h)) {
      ws_remove(ws_current(), h);
    } else {
      set_dwm_transitions_disabled(h, true);
      ShowWindow(h, SW_HIDE);
    }
  }

  ws_set_current(target);

  // show all windows in the target workspace
  HWND to_focus = NULL;
  for (int i = g_ws[ws_current()].count - 1; i >= 0; i--) {
    HWND h = g_ws[ws_current()].windows[i];
    if (!IsWindow(h)) {
      ws_remove(ws_current(), h);
    } else {
      ShowWindow(h, SW_SHOWNA);
      set_dwm_transitions_disabled(h, false);
      if (h == g_ws[ws_current()].last_focused)
        to_focus = h;
    }
  }

  if (!to_focus && g_ws[ws_current()].count > 0) {
    to_focus = g_ws[ws_current()].windows[g_ws[ws_current()].count - 1];
  }
  if (to_focus)
    SetForegroundWindow(to_focus);

  ws_set_is_switching(false);
  printf("workspace transition complete: %d -> %d\n", ws_prev() + 1,
         ws_current() + 1);
}

static void move_fg_to_workspace(int target) {
  if (target < 0 || target >= NUM_WORKSPACES)
    return;
  HWND fg = GetForegroundWindow();
  if (!fg)
    return;

  int old_ws = find_window_ws(fg);
  if (old_ws == -1) {
    if (!is_manageable_window(fg))
      return;
  } else {
    if (old_ws == target)
      return;
    ws_remove(old_ws, fg);
  }

  ws_add(target, fg);

  if (target != ws_current()) {
    ws_set_is_switching(true);
    set_dwm_transitions_disabled(fg, true);
    ShowWindow(fg, SW_HIDE);
    ws_set_is_switching(false);
  }

  printf("move window %p to workspace %d\n", (void *)fg, target + 1);
}

static void restore_all_windows(void) {
  ws_set_is_switching(true);
  for (int w = 0; w < NUM_WORKSPACES; w++) {
    for (int i = 0; i < g_ws[w].count; i++) {
      HWND h = g_ws[w].windows[i];
      if (IsWindow(h)) {
        ShowWindow(h, SW_SHOWNA);
        set_dwm_transitions_disabled(h, false);
      }
    }
  }
  ws_set_is_switching(false);
}

enum {
  HK_SWITCH_BASE = 1, // 1..9 -> alt+1..9
  HK_MOVE_BASE = 11,  // 11..19 -> shift+alt+1..9
  HK_PREV = 20,       // alt+a
  HK_NEXT = 21,       // alt+s
  HK_SWAP = 22,       // alt+d
  HK_QUIT = 99,       // shift+ctrl+alt+q
};

int main(void) {
  if (AttachConsole(ATTACH_PARENT_PROCESS)) {
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);
    freopen("CONIN$", "r", stdin);
  }

  printf("program started\n");

  EnumWindows(enum_init_proc, 0);
  printf("initialized workspace 1 with %d windows\n", g_ws[0].count);

  // hook window show and destroy events
  HWINEVENTHOOK hook_show =
      SetWinEventHook(EVENT_OBJECT_SHOW, EVENT_OBJECT_SHOW, NULL,
                      win_event_proc, 0, 0, WINEVENT_OUTOFCONTEXT);
  HWINEVENTHOOK hook_destroy =
      SetWinEventHook(EVENT_OBJECT_DESTROY, EVENT_OBJECT_DESTROY, NULL,
                      win_event_proc, 0, 0, WINEVENT_OUTOFCONTEXT);

  // register workspace hotkeys
  for (int i = 0; i < NUM_WORKSPACES; i++) {
    UINT vk = (UINT)('1' + i);

    if (!RegisterHotKey(NULL, HK_SWITCH_BASE + i, MOD_ALT | MOD_NOREPEAT, vk))
      printf("warning: failed to register alt+%d (err=%lu)\n", i + 1,
             GetLastError());

    if (!RegisterHotKey(NULL, HK_MOVE_BASE + i,
                        MOD_ALT | MOD_SHIFT | MOD_NOREPEAT, vk))
      printf("warning: failed to register alt+shift+%d (err=%lu)\n", i + 1,
             GetLastError());
  }

  if (!RegisterHotKey(NULL, HK_PREV, MOD_ALT | MOD_NOREPEAT, 'A')) {
    printf("[ERROR] failed to register alt+a (err=%lu)\n", GetLastError());
    return 1;
  }

  if (!RegisterHotKey(NULL, HK_NEXT, MOD_ALT | MOD_NOREPEAT, 'S')) {
    printf("[ERROR] failed to register alt+s (err=%lu)\n", GetLastError());
    return 1;
  }

  if (!RegisterHotKey(NULL, HK_SWAP, MOD_ALT | MOD_NOREPEAT, 'D')) {
    printf("[ERROR] failed to register alt+d (err=%lu)\n", GetLastError());
    return 1;
  }

  if (!RegisterHotKey(NULL, HK_QUIT,
                      MOD_SHIFT | MOD_CONTROL | MOD_ALT | MOD_NOREPEAT, 'Q')) {
    printf("[ERROR] failed to register shift+ctrl+alt+q (err=%lu)\n",
           GetLastError());
    return 1;
  }

  printf("hoykeys registered. listening for events\n");

  MSG msg;

  while (GetMessage(&msg, NULL, 0, 0) > 0) {
    if (msg.message == WM_HOTKEY) {
      int id = (int)msg.wParam;

      if (id >= HK_SWITCH_BASE && id < HK_SWITCH_BASE + NUM_WORKSPACES) {
        switch_workspace(id - HK_SWITCH_BASE);
      } else if (id >= HK_MOVE_BASE && id < HK_MOVE_BASE + NUM_WORKSPACES) {
        move_fg_to_workspace(id - HK_MOVE_BASE);
      } else if (id == HK_PREV) {
        switch_workspace((ws_current() + NUM_WORKSPACES - 1) % NUM_WORKSPACES);
      } else if (id == HK_NEXT) {
        switch_workspace((ws_current() + 1) % NUM_WORKSPACES);
      } else if (id == HK_SWAP) {
        switch_workspace(ws_prev());
      } else if (id == HK_QUIT) {
        restore_all_windows();
        PostQuitMessage(0);
      }
    }

    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  UnhookWinEvent(hook_show);
  UnhookWinEvent(hook_destroy);

  for (int i = 0; i < NUM_WORKSPACES; i++) {
    UnregisterHotKey(NULL, HK_SWITCH_BASE + i);
    UnregisterHotKey(NULL, HK_MOVE_BASE + i);
  }

  UnregisterHotKey(NULL, HK_PREV);
  UnregisterHotKey(NULL, HK_NEXT);
  UnregisterHotKey(NULL, HK_SWAP);
  UnregisterHotKey(NULL, HK_QUIT);

  return 0;
}
