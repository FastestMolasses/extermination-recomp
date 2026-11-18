/* em_sight_live.h - the live host of the weapon sights (em_sight: the
 * drawers the aim camera's sight switch 001999C0 calls; docs/CAMERA_LIVE.md
 * section 7). Prefix em_sight_live_.
 *
 * em_camera_live's aim_call hands it the eleven drawer calls of 001999C0
 * (001DB830, 001DB9D0, 001DBCB0, 001DBD50, 001DBE20, 001DBF00, 001DC020,
 * 001DC610, 001DC890, 001DC960, 00199770) with its own map and call. The
 * translation runs over:
 *   - the stack below the aim host's sp: a private window 0x7F000000..
 *     0x7F000FFF (the aim camera's frames start at 0x7F001000); the drawers'
 *     locals sit at their original offsets, so a callee reads what the
 *     original's would;
 *   - the ELF's .data the drawers read (D_00253240..D_002533CF, the strings
 *     D_0026E6F0..D_0026E80F), read only, from assets/sight_tables.emst
 *     (tools/export_sight_tables.py), loaded at the first call;
 *   - the class-2 list words D_00275B90 / D_00275B98 and its entry words,
 *     read only, from the collision world's live class-2 list (cursor =
 *     0x0028AF30 - 4 live, count = live; an entry's word is its owner's
 *     original address), and the entries' bytes through the owners'
 *     record views;
 *   - 0x70003600..0x7000363F (002082B0's colour ramp): the camera's shared
 *     scratch quadwords when they are one contiguous owner (the collision
 *     face scratch the area build binds); otherwise 0x70003620 faults;
 *   - then the aim camera's map (the player, its gun, 0x700038A0..), then
 *     the render context (D_00275670, its slot cursors, the slot packets,
 *     0x70003AC0).
 * Leaves: 001D1F80 em_rcl_001D1F80, 001D64A0 em_rcl_001D64A0, float_to_int
 * em_player_float_to_int, 00102948 / 001028B8 / 001028D0 / 001026A0
 * em_aim_fire_sdk_memory_call, 00102850 em_rvr_00102850, 00183B80
 * em_aim_fire_target_00183B80 over the same map; 00207F80, 001CBA50 and
 * 00183C40 through the bound services (em_sgp_00207F80, em_sgp_001CBA50,
 * the aim/fire composition's frame); sin, cos, sqrt and fabs through the
 * aim camera's call.
 *
 * Fail-stop: 001DD170 (00199770's lock mark) is not bound: its vf23 input
 * (the fog ramp 001CD520 loads earlier in the frame) has no certified
 * value at the camera step (docs/AIM_FIRE.md, "Not reachable in AREA11"),
 * so reaching it faults, named. Any other refusal faults too and is
 * reported once. */
#ifndef EM_SIGHT_LIVE_H
#define EM_SIGHT_LIVE_H

#include <stddef.h>
#include <stdint.h>

#include "game/em_aim_fire_target.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EM_SIGHT_TABLES_PATH "assets/sight_tables.emst"

typedef struct {
    /* 00207F80(slot, x0, y0, x1, y1, rgba): 1, or 0 refused. */
    int (*rect)(int32_t slot, int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t rgba);
    /* 001CBA50(slot, x, y, advance, height, text, style): `text` holds its
     * NUL within `avail` bytes, `style` the record's 8 bytes. 1, or 0. */
    int (*text)(int32_t slot, int32_t x, int32_t y, int32_t advance, int32_t height, const uint8_t *text,
                uint32_t avail, const uint8_t style[8]);
    /* A callee the aim/fire composition serves (00183C40): 0, or -1. */
    int (*frame)(EmAimFireTargetCall *call);
} EmSightLiveServices;

/* Binds the services (copied); NULL unbinds. */
void em_sight_live_bind(const EmSightLiveServices *services);

/* One of the eleven calls: 0 done, -1 fault (reported), 1 not a sight
 * drawer. `map` / `call` are the aim camera's (the fallback memory and the
 * math leaves). */
int em_sight_live_call(EmAimFireTargetCall *f, void *(*map)(void *, uint32_t, size_t, int),
                       int (*call)(void *, EmAimFireTargetCall *));

#ifdef __cplusplus
}
#endif

#endif /* EM_SIGHT_LIVE_H */
