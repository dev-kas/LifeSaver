#include <windows.h>

#include <initguid.h>

#include <audiopolicy.h>
#include <mmdeviceapi.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <tlhelp32.h>

#include "audio.h"
#include "workspace.h"

#define MAX_MUTED_PIDS 256
#define MAX_PROCS 2048

typedef struct {
  DWORD pid;
  DWORD ppid;
  char exe[MAX_PATH];
} ProcEntry;

static ProcEntry g_procs[MAX_PROCS];
static int g_proc_count = 0;

static DWORD g_muted_pids[MAX_MUTED_PIDS];
static int g_muted_count = 0;

static void snapshot_processes(void) {
  g_proc_count = 0;
  HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  if (snap == INVALID_HANDLE_VALUE)
    return;

  PROCESSENTRY32 pe;
  pe.dwSize = sizeof(PROCESSENTRY32);

  if (Process32First(snap, &pe)) {
    do {
      if (g_proc_count >= MAX_PROCS)
        break;
      ProcEntry *e = &g_procs[g_proc_count++];
      e->pid = pe.th32ProcessID;
      e->ppid = pe.th32ParentProcessID;
      snprintf(e->exe, sizeof(e->exe), "%s", pe.szExeFile);
    } while (Process32Next(snap, &pe));
  }
  CloseHandle(snap);
}

static const ProcEntry *find_proc(DWORD pid) {
  for (int i = 0; i < g_proc_count; i++) {
    if (g_procs[i].pid == pid)
      return &g_procs[i];
  }
  return NULL;
}

static bool is_pid_in_muted_list(DWORD pid) {
  for (int i = 0; i < g_muted_count; i++) {
    if (g_muted_pids[i] == pid)
      return true;
  }
  return false;
}

static void add_muted_pid(DWORD pid) {
  if (!is_pid_in_muted_list(pid) && g_muted_count < MAX_MUTED_PIDS) {
    g_muted_pids[g_muted_count++] = pid;
  }
}

static void remove_muted_pid(DWORD pid) {
  for (int i = 0; i < g_muted_count; i++) {
    if (g_muted_pids[i] == pid) {
      g_muted_pids[i] = g_muted_pids[--g_muted_count];
      return;
    }
  }
}

static bool ws_has_pid(int ws, DWORD target_pid) {
  for (int i = 0; i < g_ws[ws].count; i++) {
    HWND h = g_ws[ws].windows[i];
    if (!IsWindow(h))
      continue;

    DWORD pid = 0;
    GetWindowThreadProcessId(h, &pid);
    if (pid == target_pid)
      return true;
  }
  return false;
}

static bool pid_is_in_any_ws(DWORD target_pid) {
  for (int w = 0; w < NUM_WORKSPACES; w++) {
    if (ws_has_pid(w, target_pid))
      return true;
  }
  return false;
}

static DWORD resolve_tracked_pid(DWORD pid) {
  if (pid_is_in_any_ws(pid))
    return pid;

  const ProcEntry *start = find_proc(pid);
  if (!start)
    return pid;

  DWORD cur_pid = start->ppid;
  for (int depth = 0; depth < 8 && cur_pid != 0; depth++) {
    const ProcEntry *parent = find_proc(cur_pid);
    if (!parent)
      break;
    if (_stricmp(parent->exe, start->exe) != 0)
      break;
    if (pid_is_in_any_ws(parent->pid))
      return parent->pid;
    if (parent->ppid == cur_pid)
      break;
    cur_pid = parent->ppid;
  }

  return pid;
}

static bool pid_is_only_in_hidden_ws(DWORD target_pid) {
  if (ws_has_pid(ws_current(), target_pid))
    return false;

  for (int w = 0; w < NUM_WORKSPACES; w++) {
    if (w != ws_current() && ws_has_pid(w, target_pid))
      return true;
  }
  return false;
}

static void process_audio_sessions(bool restore_only) {
  void *tmp = NULL;

  if (!restore_only)
    snapshot_processes();

  if (FAILED(CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL,
                              &IID_IMMDeviceEnumerator, &tmp)))
    return;
  IMMDeviceEnumerator *enumerator = (IMMDeviceEnumerator *)tmp;

  IMMDevice *device = NULL;
  if (FAILED(enumerator->lpVtbl->GetDefaultAudioEndpoint(enumerator, eRender,
                                                         eConsole, &device))) {
    enumerator->lpVtbl->Release(enumerator);
    return;
  }

  tmp = NULL;
  if (FAILED(device->lpVtbl->Activate(device, &IID_IAudioSessionManager2,
                                      CLSCTX_ALL, NULL, &tmp))) {
    device->lpVtbl->Release(device);
    enumerator->lpVtbl->Release(enumerator);
    return;
  }
  IAudioSessionManager2 *mgr = (IAudioSessionManager2 *)tmp;

  IAudioSessionEnumerator *session_enum = NULL;
  if (FAILED(mgr->lpVtbl->GetSessionEnumerator(mgr, &session_enum))) {
    mgr->lpVtbl->Release(mgr);
    device->lpVtbl->Release(device);
    enumerator->lpVtbl->Release(enumerator);
    return;
  }

  int count = 0;
  session_enum->lpVtbl->GetCount(session_enum, &count);

  for (int i = 0; i < count; i++) {
    IAudioSessionControl *ctrl = NULL;
    if (FAILED(session_enum->lpVtbl->GetSession(session_enum, i, &ctrl)))
      continue;

    tmp = NULL;
    IAudioSessionControl2 *ctrl2 = NULL;
    if (SUCCEEDED(ctrl->lpVtbl->QueryInterface(ctrl, &IID_IAudioSessionControl2,
                                               &tmp)))
      ctrl2 = (IAudioSessionControl2 *)tmp;

    tmp = NULL;
    ISimpleAudioVolume *vol = NULL;
    if (SUCCEEDED(
            ctrl->lpVtbl->QueryInterface(ctrl, &IID_ISimpleAudioVolume, &tmp)))
      vol = (ISimpleAudioVolume *)tmp;

    if (ctrl2 && vol) {
      DWORD pid = 0;
      if (SUCCEEDED(ctrl2->lpVtbl->GetProcessId(ctrl2, &pid)) && pid != 0) {
        DWORD owner_pid = restore_only ? pid : resolve_tracked_pid(pid);
        bool should_mute = !restore_only && pid_is_only_in_hidden_ws(owner_pid);

        if (should_mute) {
          BOOL already_muted = FALSE;
          if (SUCCEEDED(vol->lpVtbl->GetMute(vol, &already_muted)) &&
              !already_muted) {
            vol->lpVtbl->SetMute(vol, TRUE, NULL);
            add_muted_pid(pid);
            printf("audio muted pid %lu\n", owner_pid);
          }
        } else if (is_pid_in_muted_list(pid)) {
          vol->lpVtbl->SetMute(vol, FALSE, NULL);
          remove_muted_pid(pid);
          printf("audio unmuted pid: %lu\n", pid);
        }
      }
    }

    if (vol)
      vol->lpVtbl->Release(vol);
    if (ctrl2)
      ctrl2->lpVtbl->Release(ctrl2);
    ctrl->lpVtbl->Release(ctrl);
  }

  session_enum->lpVtbl->Release(session_enum);
  mgr->lpVtbl->Release(mgr);
  device->lpVtbl->Release(device);
  enumerator->lpVtbl->Release(enumerator);
}

void audio_init(void) { CoInitializeEx(NULL, COINIT_APARTMENTTHREADED); }
void audio_shutdown(void) {
  audio_restore_all();
  CoUninitialize();
}

void audio_sync(void) { process_audio_sessions(false); }

void audio_restore_all(void) {
  if (g_muted_count > 0) {
    process_audio_sessions(true);
    g_muted_count = 0;
  }
}
