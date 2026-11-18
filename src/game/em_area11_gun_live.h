/* em_area11_gun_live.h - the AREA11 security gun 0x825940's return-visit
 * lifecycles 4 (patrol) and 1 (aim and fire) bound live
 * (docs/SECURITY_GUN.md section 5.1).
 *
 * This module adds no behaviour. It runs em_gun_rest_tick (the verified
 * translation, oracle tools/test_security_gun_rest_reference.py) over the
 * gun's live storage and binds its workers to their existing translations:
 *
 * Memory (EmGunRestMem, by original address):
 *   the gun's +0x28 / +0x2A     the AREA11 node's two halfwords
 *   the gun's +0x1F0..+0x2EF    EmActor.scratch
 *   the gun's +0x110..+0x11F    its bone-slot words (em_area11_boxes_owner_slot)
 *   the gun's +0x4C             its bound draw method (read only)
 *   D_00275B40                  the gun's +0x110 (the walk sets it)
 *   the gun's bones             EmOwnerBone +0x70 rot, +0x7C trans, +0x90 world
 *   the lamp (+0x220)           its +0xA0 colour (the node's) and +0x110 slot
 *                               words (em_indicator_bind_live); its slot's
 *                               +0x90 takes the 00102958 copy through
 *                               em_indicator_bind_live_set_node
 *   0x82A730 / 0x82A740         the resident AREA11 overlay (module loader)
 *   the shot node's +0x100      written once by 0x827400; no port reader
 *   0x700031D0 / 0x700031D4     the probe's record and entity words; the
 *                               sight's restore after a missed second probe
 *                               is held until the next probe of this call
 *   everything else             the aim / fire composition's map
 *                               (em_aim_fire_binding_bytes: pool fields, the
 *                               player record, the probe scratch, 0x70003B68)
 *                               and, for the scratchpad words no owner holds
 *                               (written before they are read), this module
 *
 * Workers: the owner services as the first-visit binding (001C6380,
 * 001A2370, 001B17A0, the +0x4C draw, rand, 001FBD50); the SDK float and
 * vector leaves on their translations (em_sdk_math_original, em_effect_original,
 * em_coll_probe_original, em_owner_services_original; 0011E520 through
 * em_area02_misc); the probes 0019AA80 / 0019A570 / 0019B6C0, the sight
 * sprite 001CD520, the beam 001E2BA0 and the effect spawn 001EFD90 through the
 * aim / fire composition (em_aim_fire_binding_frame); 001AFA90 the pool's
 * allocator, the shot node adopted by em_aim_fire_runtime and bound to its
 * 001F5040 once its +0x10 holds it (after the call).
 *
 * Any address or worker outside this list faults (fail-stop). */
#ifndef EM_AREA11_GUN_LIVE_H
#define EM_AREA11_GUN_LIVE_H

#include <stdint.h>

#include "game/em_actor_pool.h"
#include "game/em_scene_state.h"
#include "game/em_security_gun.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    EmActorPool *pool;
    EmSceneState *scene;
    EmActor *gun;
    int16_t *h28;                       /* +0x28 and, adjacent, +0x2A */
    float *(*node_a0)(EmActor *actor);  /* a pool record's +0xA0 storage */
} EmArea11GunLive;

/* One call of 0x825940 in lifecycle 4 or 1. 1 while allocated, -1 on a
 * fault (in *fault: the address and EM_GUN_FAULT_*). */
int em_area11_gun_live_tick(const EmArea11GunLive *g, EmGunFault *fault);

#ifdef __cplusplus
}
#endif

#endif /* EM_AREA11_GUN_LIVE_H */
