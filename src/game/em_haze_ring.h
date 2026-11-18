/* em_haze_ring.h - the effect node behaviour 001E76A0 (a word of 001EF9D0's
 * kind table, D_00258CF0; AREA21's boss fight starts one at the guided
 * run's g77 row 5922) and the ring it draws each frame. One translation
 * per original function, from the decomp C (001E76A0 and 001E0AC0
 * byte-identical C; 001E0750 a NEARMISS whose remaining differences are
 * register choices only: its original instructions were followed):
 *
 *   001E76A0(self) the byte +4 is a state. 0: +4 = 1, the float +0x20 =
 *                 300, then as 1. 1: 001028B8(+0xB0, +0xB0, +0xC0) (the
 *                 point moves by the velocity), +0x20 -= 1 and +4 = 3 when
 *                 the result is <= 0, then 001E0AC0(+0xB0, D_0026E9A0,
 *                 75.0, 30.0). 2 and 3: 001AFC10(self). Others: nothing.
 *   001E0AC0(p, style; f12 fx, f13 fy) buf = the channel-3 cursor (the
 *                 word at ctx +0x1C; ctx = the word D_00275670);
 *                 001D1F80(3, 2, 1), 001D7510(3, 0, 0), 001D1FF0(3, 2);
 *                 then for i = 0..63: a = 2 (pi i) / 64 - pi, v = (p.x +
 *                 fx sin a, p.y + fy cos a, p.z, p.w) (0011DE90, 0011E2A8),
 *                 001E0750(v, style). Then the closing tag at the cursor
 *                 (byte +3 = 0x60, word +4 = 0, halfword +0 = 0), the
 *                 cursor + 0x10, and 001CAAC0(p, buf).
 *   001E0750(pos, style) a screen-sampling sprite at pos. The clip test of
 *                 pos through 001CD370(0) (= ctx + 0x2240, vf24..vf27):
 *                 any of the six newest CLIP flags set -> nothing. Else the
 *                 projection through the camera rows at 0x70003AC0
 *                 (vf28..vf31): Q = 1 / w, x and y scaled by Q, then w -=
 *                 vf3.x (vf3 = the GPR the zero float was moved into:
 *                 x = y = 0), Q = 1 / w again and z scaled by it; the fog
 *                 lane from vf23 (w = vf23.z + vf23.w w, min vf23.x, max
 *                 0); ftoi4 into 0x70003600. scale = 8 (256 / w), w of the
 *                 unscaled projection. A CNT tag of 0x14 qwords at the
 *                 channel-3 cursor (advanced by 0x150), a zero quadword
 *                 with DIRECT 0x13 at +0xC, the GIF tag
 *                 0x302EC000_00008006 / 0x421, then six vertices of 0x30
 *                 bytes: RGBAQ (0x80, 0x80, 0x80, style[3]) for the first,
 *                 alpha 0 for the rest; XYZ2 = float_to_int(screen x +
 *                 scale corner x), float_to_int(screen y + scale corner y)
 *                 (the six corner pairs D_00253530), screen z, 0; ST =
 *                 (scale (-1/5120 + 1/2560 r1 / 2^31) + (-3.5 + X / 8192),
 *                 scale (-1/2240 + 1/1120 r2 / 2^31) + (-7.5625 + Y /
 *                 4096), 1.0), r1, r2 two rand() words (00122BB8) per
 *                 vertex and X, Y the vertex's stored XYZ2 words. The word
 *                 +0xC of each ST quadword is not written.
 *
 * Memory goes through `map` by original address (D_00275670, the context,
 * D_0026E9A0, D_00253530, the scratchpad 0x70003600..0F and 0x70003AC0..FF,
 * the node and the packets). 001E0AC0's vector v (a stack local the
 * original hands to 001E0750) is a host array; the original's other stack
 * locals are C locals.
 *
 * VU0 (em_far_light's EmFarLightVu): 001E0750 writes vf1, vf2, vf3 (x and
 * y only: its z and w are the upper half of a 128-bit GPR the original
 * never set, left as they were), vf24..vf31, ACC, Q and CLIP, and reads
 * vf23 (the fog ramp) as its caller left it. 001E76A0 and 001E0AC0 touch
 * VU0 only through their callees (001028B8's vf4..vf6 are the hook's).
 * EE arithmetic goes through em_ee_float.h; float_to_int is 001281C0
 * (em_player_float_to_int); 001CD370 is computed here.
 *
 * Hooks (each gets `sp`, the original stack pointer at its call): 001028B8,
 * 001AFC10, 001D1F80, 001D7510, 001D1FF0, 0011DE90 (sin), 0011E2A8 (cos),
 * 00122BB8 (rand) and 001CAAC0.
 *
 * Fail-stop: an unmapped address, a NULL or failing hook or an unmeasured
 * float form latches the first fault; every entry then returns -1. A NULL
 * hook faults only when reached. */
#ifndef EM_HAZE_RING_H
#define EM_HAZE_RING_H

#include <stdint.h>

#include "game/em_far_light.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
    EM_HAZE_RING_FAULT_NONE = 0,
    EM_HAZE_RING_FAULT_UNMAPPED = 1,
    EM_HAZE_RING_FAULT_WORKER = 2,
    EM_HAZE_RING_FAULT_UNMEASURED = 3
};

typedef struct {
    void *ctx;
    /* host bytes of [address, address + size), or NULL (a fault) */
    uint8_t *(*map)(void *ctx, uint32_t address, uint32_t size, int write);
    EmFarLightVu *vu;
    int (*w_001028B8)(void *ctx, uint32_t sp, uint32_t dst, uint32_t a, uint32_t b);
    int (*w_001AFC10)(void *ctx, uint32_t sp, uint32_t self);
    int (*w_001D1F80)(void *ctx, uint32_t sp, int32_t chan, int32_t a1, int32_t a2);
    int (*w_001D7510)(void *ctx, uint32_t sp, int32_t chan, int32_t a1, int32_t a2);
    int (*w_001D1FF0)(void *ctx, uint32_t sp, int32_t chan, int32_t a1);
    int (*w_0011DE90)(void *ctx, uint32_t sp, uint32_t x, uint32_t *result);
    int (*w_0011E2A8)(void *ctx, uint32_t sp, uint32_t x, uint32_t *result);
    int (*w_00122BB8)(void *ctx, uint32_t sp, int32_t *result);
    int (*w_001CAAC0)(void *ctx, uint32_t sp, uint32_t pos, uint32_t payload);
    int32_t fault;
    uint32_t fault_function, fault_address;
} EmHazeRing;

/* sp: the original stack pointer at the function's entry. */
int em_haze_ring_001E76A0(EmHazeRing *h, uint32_t self, uint32_t sp);
/* fx, fy: raw float bits (f12, f13) */
int em_haze_ring_001E0AC0(EmHazeRing *h, uint32_t p, uint32_t style, uint32_t fx, uint32_t fy, uint32_t sp);
int em_haze_ring_001E0750(EmHazeRing *h, const uint32_t pos[4], uint32_t style, uint32_t sp);

#ifdef __cplusplus
}
#endif

#endif
