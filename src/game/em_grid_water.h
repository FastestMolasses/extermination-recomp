/* em_grid_water.h - 001E1AD0's two host workers (render flag 0x22; AREA21
 * sets it): 001E1760, which steps the 16 x 16 surface table D_0081E0F0
 * and draws it as textured strips, and 001E17E0, which draws a 16 x 16
 * grid of projected points as triangle strips. One translation per
 * original function, from the decomp C (byte-identical C but 001E13E0 and
 * 001E17E0, NEARMISS units whose remaining differences are register
 * choices only: their original instructions were followed):
 *
 *   001E1760(chan)  001D6E60(chan, 0x258000, 8, 8), 001D1F80(chan, 0, 7),
 *                 001D1FF0(chan, 1), 001D6F60(chan, the doubleword at
 *                 ctx +0x1E0, 0x80), 001E10A0(), 001E13E0(chan),
 *                 001D1F20(chan). (ctx = the word D_00275670.)
 *   001E10A0()    one step of the table (records of 0x18 bytes: x, y,
 *                 height z, velocity w, u, v): per cell, in row order,
 *                 sum = 0 + z above + z below + z left + z right (rows and
 *                 columns wrap at 16) - (0.25 + 4 z); w = w * 0.961 +
 *                 0.0022 sum; u = j / 16 + 0.03 (z - z right); v = i / 16 +
 *                 0.03 (z - z below). Then two rand() draws (00122BB8)
 *                 pick column a and row b ((r >> 6) & 15) and the velocity
 *                 gets 1.2 at (b, a), 0.90000004 at its four neighbours and
 *                 0.54 at the four diagonals (wrapping), in that order;
 *                 then z += w over the whole table.
 *   001E13E0(chan) 16 strips (rows i and i + 1, the last pairing row 15
 *                 with row 0 at v + 1.0 and y + 0x1000): each a CNT tag of
 *                 0x68 qwords (the cursor at ctx +0x10 + 4 chan advanced by
 *                 0x690), a zero quadword with DIRECT 0x67 at +0xC, the GIF
 *                 tag 0x600A4000_00008011 / 0x512512, then 17 vertex pairs
 *                 of 0x60 bytes (ST = u, v, 1.0; RGBAQ 0x40, 0x40, 0x40,
 *                 0x80; XYZ2 = float_to_int(30720 + 16 x), float_to_int of
 *                 30720 + 16 y (+ the row bias), 0, 0); the 17th repeats the
 *                 row's first record at u + 1.0 and x 0x8800. The word at
 *                 +0xC of each ST quadword is not written.
 *   001E17E0(chan) returns the cursor as it was on entry. vf24..vf27 =
 *                 001CD370(2) (= ctx +0x22C0), vf28..vf31 = the camera
 *                 rows at 0x70003AC0. 16 rows i = -8..7: z0 = 93.75 i -
 *                 46.875, z1 the same for i + 1; a CNT tag of 0x22 qwords
 *                 (cursor + 0x230), a zero quadword with DIRECT 0x21 at
 *                 +0xC, the GIF tag 0x20024000_00008010 / 0x44, then per
 *                 column j = -8..7 (x = 93.75 j - 46.875, height 10) the two
 *                 points (x, 10, z0) and (x, 10, z1), each: flag >>= 1,
 *                 the clip test (em_far_light_vu_clip; a newest flag set:
 *                 flag |= 4), the projection (em_far_light_vu_project) stored
 *                 as one quadword, and the word +0xC = 0xFFFF when flag is
 *                 nonzero. The flag starts at 0 per row.
 *
 * Memory goes through `map` by original address (D_00275670, the context,
 * D_0081E0F0..+0x1800, the scratchpad 0x70003AC0..FF and the channel
 * packets). VU0 as em_far_light's: 001E17E0 writes vf1, vf2, vf24..vf31,
 * ACC, Q and CLIP and reads vf23 (the fog ramp) as its caller left it.
 * EE arithmetic goes through em_ee_float.h; float_to_int is 001281C0
 * (em_player_float_to_int).
 *
 * Hooks: 00122BB8 (rand), and 001E1760's translated callees 001D6E60,
 * 001D1F80, 001D1FF0, 001D6F60 and 001D1F20 (em_load_veil_particles,
 * em_static_world).
 *
 * Fail-stop: an unmapped address, a NULL or failing hook or an unmeasured
 * float form latches the first fault; every entry then returns -1. A NULL
 * hook faults only when reached. */
#ifndef EM_GRID_WATER_H
#define EM_GRID_WATER_H

#include <stdint.h>

#include "game/em_far_light.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
    EM_GRID_WATER_FAULT_NONE = 0,
    EM_GRID_WATER_FAULT_UNMAPPED = 1,
    EM_GRID_WATER_FAULT_WORKER = 2,
    EM_GRID_WATER_FAULT_UNMEASURED = 3
};

typedef struct {
    void *ctx;
    /* host bytes of [address, address + size), or NULL (a fault) */
    uint8_t *(*map)(void *ctx, uint32_t address, uint32_t size, int write);
    EmFarLightVu *vu;
    int (*w_00122BB8)(void *ctx, int32_t *result);
    int (*w_001D6E60)(void *ctx, int32_t chan, int32_t a1, int32_t a2, int32_t a3);
    int (*w_001D1F80)(void *ctx, int32_t chan, int32_t a1, int32_t a2);
    int (*w_001D1FF0)(void *ctx, int32_t chan, int32_t a1);
    int (*w_001D6F60)(void *ctx, int32_t chan, uint64_t tex0, int32_t a2);
    int (*w_001D1F20)(void *ctx, int32_t chan);
    int32_t fault;
    uint32_t fault_function, fault_address;
} EmGridWater;

int em_grid_water_001E1760(EmGridWater *g, int32_t chan);
int em_grid_water_001E10A0(EmGridWater *g);
int em_grid_water_001E13E0(EmGridWater *g, int32_t chan);
/* *v0: the channel cursor as it was on entry */
int em_grid_water_001E17E0(EmGridWater *g, int32_t chan, uint32_t *v0);

#ifdef __cplusplus
}
#endif

#endif
