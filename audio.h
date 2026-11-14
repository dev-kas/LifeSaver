#ifndef AUDIO_H
#define AUDIO_H

void audio_init(void);
void audio_shutdown(void);

// mutes PIDs only in hidden workspaces,
// unmites PIDs active in current workspace
void audio_sync(void);

// unmutes everything (on quit)
void audio_restore_all(void);

#endif // AUDIO_H
