/* em_haze_ring_live.h - the effect node behaviour 001E76A0 (em_haze_ring)
 * over the area runtime: AREA21's boss fight starts one (the guided run's
 * g77 row 5922). Memory is the runtime's bytes; every hook re-enters
 * em_area01_runtime_call by address (001028B8, 001AFC10, sin / cos, rand,
 * 001CAAC0, and the render context's 001D1F80 / 001D7510 / 001D1FF0
 * through em_area01_rcl_workers). See docs/AREA21_PORT.md "Haze ring". */
#ifndef EM_HAZE_RING_LIVE_H
#define EM_HAZE_RING_LIVE_H

#include <stdint.h>

#include "game/em_area01_runtime.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 001E76A0(node) with sp the original stack pointer at its entry. 0, or -1
 * with *where (the faulting address) and *code (EM_HAZE_RING_FAULT_*). */
int em_haze_ring_live_001E76A0(EmArea01Runtime *r, uint32_t sp, uint32_t node, uint32_t *where, int32_t *code);

#ifdef __cplusplus
}
#endif

#endif
