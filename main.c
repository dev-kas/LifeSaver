#include <stdio.h>
#include <windows.h>

int main(void) {
  if (AttachConsole(ATTACH_PARENT_PROCESS)) {
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);
    freopen("CONIN$", "r", stdin);
  }

  printf("Hello world!\n");
  return 0;
}
