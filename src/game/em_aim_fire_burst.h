/* em_aim_fire_burst.h - AREA11's host of the two class-0xD burst nodes (type
 * 5, the blast the drums and a box's break spawn): the emitter 001E4CE0
 * (em_area02_misc's translation) and the ring 001E4610 (em_level11_port's).
 * Everywhere else the area runtime runs them; AREA11 has no area runtime, so
 * here their memory is the aim / fire composition's map and their callees
 * re-enter the composition (em_aim_fire_runtime's other_tick). Also the
 * flamethrower's burst node 001E3630 (em_effect_001E3630's translation; the
 * effect 0x80000039 that 00188630 mode 4 spawns), in every area. Nothing here
 * computes. docs/EFFECT_MANAGER.md section 8.2. */
#ifndef EM_AIM_FIRE_BURST_H
#define EM_AIM_FIRE_BURST_H

#include <stdint.h>

#include "game/em_aim_fire_live.h"

#define EM_AIM_FIRE_BURST_CALLBACK 0x001E4CE0u
#define EM_AIM_FIRE_RING_CALLBACK 0x001E4610u

/* The composition's call of any of these nodes (a[0] = the node): 1 when
 * f->function is one of them and it ran, 0 for another function, -1 on a
 * fault. */
int em_aim_fire_burst_call(EmAimFireLive *live, EmAimFireTargetCall *f);

#endif
