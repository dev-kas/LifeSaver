#include <stdio.h>
#include <windows.h>

#ifndef MOD_NOREPEAT
#define MOD_NOREPEAT 0x4000
#endif

enum { HK_HIDE = 1, HK_UNHIDE, HK_QUIT };

int main(void) {
  if (AttachConsole(ATTACH_PARENT_PROCESS)) {
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);
    freopen("CONIN$", "r", stdin);
  }

  printf("program started\n");

  if (!RegisterHotKey(NULL, HK_HIDE, MOD_ALT | MOD_NOREPEAT, 'H')) {
    printf("[ERROR] failed to register alt+h (err=%lu)\n", GetLastError());
    return 1;
  }

  if (!RegisterHotKey(NULL, HK_UNHIDE, MOD_ALT | MOD_NOREPEAT, 'U')) {
    printf("[ERROR] failed to register alt+u (err=%lu)\n", GetLastError());
    return 1;
  }

  if (!RegisterHotKey(NULL, HK_QUIT, MOD_ALT | MOD_NOREPEAT, 'Q')) {
    printf("[ERROR] failed to register alt+q (err=%lu)\n", GetLastError());
    return 1;
  }

  printf("hoykeys registered. listening for events\n");

  HWND hidden_hwnd = NULL;
  MSG msg;

  while (GetMessage(&msg, NULL, 0, 0) > 0) {
    if (msg.message == WM_HOTKEY) {
      switch (msg.wParam) {
      case HK_HIDE: {
        HWND fg = GetForegroundWindow();
        if (fg && fg != hidden_hwnd) {
          if (hidden_hwnd && IsWindow(hidden_hwnd)) {
            ShowWindow(hidden_hwnd, SW_SHOW);
          }
          hidden_hwnd = fg;
          ShowWindow(hidden_hwnd, SW_HIDE);
          printf("hide hwnd: %p\n", (void *)hidden_hwnd);
        }
        break;
      }
      case HK_UNHIDE: {
        if (hidden_hwnd && IsWindow(hidden_hwnd)) {
          ShowWindow(hidden_hwnd, SW_SHOW);
          SetForegroundWindow(hidden_hwnd);
          printf("restored hwnd: %p\n", (void *)hidden_hwnd);
          hidden_hwnd = NULL;
        }
        break;
      }
      case HK_QUIT: {
        if (hidden_hwnd && IsWindow(hidden_hwnd)) {
          ShowWindow(hidden_hwnd, SW_SHOW);
        }
        PostQuitMessage(0);
        break;
      }
      }
    }
  }

  UnregisterHotKey(NULL, HK_HIDE);
  UnregisterHotKey(NULL, HK_UNHIDE);
  UnregisterHotKey(NULL, HK_QUIT);

  return 0;
}
