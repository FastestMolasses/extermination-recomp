/* em_fast.h - the fast-forward start of a recorded playback (host tooling).
 *
 * EM_FAST_UNTIL=<segment>:<offset> (with EM_INPUT_PLAY, tools/watch_takes.py
 * --from / --at) plays the recording from its first tick with no picture, no
 * sound and no pacing, and switches to the normal presentation (window, 59.94 Hz
 * pacing, sound) at the first tick whose live segment index is the recording's
 * <segment> and whose offset in it is at least <offset> (the same pair the
 * playback log's "seg" and "off" columns hold). One process and one state, so
 * the game's logic is the one a plain playback has: the fast phase only skips
 * what no later tick reads (em_norender.h: the picture), the pacing wait and the
 * speakers' output. It is a developer/viewer switch (docs/LAUNCHER_OPTIONS.md
 * "Not launcher options"; proof: tools/test_fast_from.py).
 *
 * During the fast phase: no EM_NO_RENDER honour check beyond the names that read
 * a picture or a GS diagnostic (a capture is held until the switch,
 * em_replay.c), the window stays hidden (em_window_show), the frames are not
 * presented (em_gfx_present_hold), the output is silent, and the loop is unpaced.
 * At the switch the audio's rings drop what the skipped part left (em_bgm_fast_end). */
#ifndef EM_FAST_H
#define EM_FAST_H

#include <stdint.h>

extern int em_fast_flag;            /* 1 while the fast phase runs */
extern int em_fast_norender_ok;     /* the fast phase may skip the picture's work */

static inline int em_fast_active(void) { return em_fast_flag; }

/* Read EM_FAST_UNTIL. 1: a fast phase is requested (the flag is set), 0: none,
 * -1: malformed (reported). Call before the window is created. */
int em_fast_init(void);
/* After em_frame_init: the fast phase runs unpaced. 1 when it runs. */
int em_fast_start_unpaced(void);
/* The requested stop point; 0 when none. */
int em_fast_target(uint32_t *segment, uint32_t *offset);
/* What the switch drives, set once by main (the window and the gfx device). */
void em_fast_bind(void (*end)(void *), void *user);
/* The switch: the normal presentation from here. Idempotent. */
void em_fast_end(void);

#endif
