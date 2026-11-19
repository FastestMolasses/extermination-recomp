/* em_sight_lock.h - 00199770's lock mark: the reticle 001DD170 (kind 1,
 * the distance label) as the missile launcher's sight draws it from the
 * aim camera's sight switch 001999C0 (docs/CAMERA_LIVE.md section 7,
 * docs/AIM_FIRE.md section 7). Prefix em_sight_lock_.
 *
 * Runs the translations em_aim_fire_reticle_001DD170 / 001DD2F0 / 001DD600
 * and the number formatter 001C5FB0 (em_status_draw_001C5FB0, its string
 * written at D_008111D0 through the host's map, v0 = 0x008111D0). The other
 * callees (001CBA50, 001281C0) go to the host's call.
 *
 * vf23 (the reticle's depth-ramp input, read as its caller left it): the
 * fog quadword at *D_00275670 + 0xA0. Measured in the original (lane
 * PORT-FAILSTOPS, a hidden fork run with the launcher fitted and class-2
 * targets in range): vf23 is the same at the entries of 001999C0, 00199770
 * and every 001DD170 of a tick, and it is that quadword in 57 of 90 ticks;
 * in the others another owner's leftover. vf23 reaches only the w lane:
 * the word 0x7000360C and vf2.w / ACC.w afterwards; the packets, the label
 * and every other byte are the same for any vf23 (make
 * test-sight-lock-reference). The VU registers start at zero apart from
 * vf23 and are dropped after the call (no later port owner reads them). */
#ifndef EM_SIGHT_LOCK_H
#define EM_SIGHT_LOCK_H

#include <stdint.h>

#include "game/em_aim_fire_reticle.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    void *context;
    /* original address -> bytes (NULL refused). Reads: *D_00275670 and the
     * render context (+0x10 slot cursors, +0xA0, +0x2240), the position
     * quadword, 0x70003AC0, the stack below sp, the .data templates
     * D_002533D0..D_0025348F; writes: 0x70003600..0x7000362F, the stack,
     * the slot packets and cursors, D_008111D0..D_008111D7. */
    uint8_t *(*map)(void *context, uint32_t address, uint32_t size, int write);
    /* 001CBA50 and 001281C0: 0, or -1. */
    int (*call)(void *context, EmAimFireReticleCall *call);
    uint32_t fault_function, fault_address; /* the first fault */
} EmSightLock;

/* 001DD170(style, position, kind, rgba, value) at stack pointer sp: 0 done,
 * -1 fault (fault_function / fault_address set). */
int em_sight_lock_001DD170(EmSightLock *h, uint32_t sp, uint32_t style, uint32_t position, uint32_t kind,
                           uint32_t rgba, uint32_t value);

#ifdef __cplusplus
}
#endif

#endif /* EM_SIGHT_LOCK_H */
