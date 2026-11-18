/* em_far_light.h - the workers of 001E1E60's flag-0x23 branch (AREA21 sets
 * render flag 0x23): a far point drawn as a screen sprite, its GS read-back
 * probe and the flare along the screen centre axis. One translation per
 * original function, from the decomp C (byte-identical C but 001CEFD0, a
 * NEARMISS whose original instructions were followed):
 *
 *   001D73A0(a0)  0 <= a0 < 0x10: the word at +8 of entry D_00275C08 of the
 *                 OTHER bank of the read-back list D_008172C0 (bank =
 *                 1 - ctx+0x9C, 0x480 bytes, entries 0x48), i.e. what
 *                 001D7410 read back from the GS for the point queued in
 *                 the previous frame; otherwise 0. a0 is only range-checked.
 *   001D72D0(pos) the list is full (D_00275C08 >= 0x10): -1. Else the point
 *                 through the camera rows at 0x70003AC0 (vf28..vf31), the
 *                 perspective divide of x, y, z, the fog lane from vf23
 *                 (vf2.w = vf23.z + vf23.w * w, then min vf23.x, max 0) and
 *                 12.4 conversion; behind the camera (clip w < 0): -1; else
 *                 001D7200((sx >> 4) - 0x700, (sy >> 4) - 0x790).
 *   001D7200(x, y) the list is full: -1. Else x clamped to [0, 0x1FF] (the
 *                 upper test is float(x) < 512), y to [0, 0xDF], both stored
 *                 as floats at +0 / +4 of the entry, the counter bumped;
 *                 returns the entry index.
 *   001D75E0(idx, pos, tex0, rgba, w, h) the sprite: w and h scaled by the
 *                 reference projection of (16, 16, 480, 1) through ctx+0x2340
 *                 (001026A0), the point projected into 0x70003600 as in
 *                 001D72D0, rejected behind the camera or outside the window
 *                 (0x7000..0x9000 x 0x7900..0x8700 in 12.4), UVs clipped to
 *                 the window (0x70003670..7C, 0x70003A38 / 3C), and a VIF
 *                 DIRECT + GIF sprite packet appended to channel idx's list
 *                 (the cursor at ctx + 0x10 + 4 idx, advanced by 0x90).
 *                 Returns the packet + 0x10 address, or 0.
 *   001CEFD0(pos, rgba) the flare: the clip test of pos through
 *                 001CD370(2) (= ctx + 0x22C0, vf24..vf27): any of the six
 *                 newest clip flags set -> nothing. The projection as above
 *                 into 0x70003600; fog F (0x7000360C) outside 0..0xFF ->
 *                 nothing. The centred screen offset (cx, cy) gives r =
 *                 sqrt(cx*cx + cy*cy) (MULA then MADD) / 393.84616, clamped
 *                 to 1; the colour 0x70003610 = ftoi0(rgba * (1 - r))
 *                 (00102900, 00102990); seven sprites at the offsets times
 *                 the factors D_00251370[i], sizes D_00251390[i], TEX0
 *                 alternating, through 001CF320; then 001CB900(D_007635C0,
 *                 0xFFF000, 2).
 *   001CF320(xy, rgba, tex0, size) one flare sprite: 8 quadwords from
 *                 001CB5F0(D_007635C0, 0xFFF000, 8), half extents
 *                 float_to_int(48 size) << 4 and float_to_int(24 size) << 4.
 *
 * Memory goes through `map` by original address (the read-back list, the
 * context, D_00275670 / D_00275C08, the scratchpad 0x70003600..3F,
 * 0x70003670..7F, 0x70003A20..3F, 0x70003AC0..FF, the tables D_00251370 /
 * D_00251390 and the packets). Stack locals the original never hands to a
 * callee are C locals; the reference vector of 001D75E0 and the colour of
 * 001CEFD0 are host arrays (their callees are translations called here).
 *
 * VU0: `vu` holds vf0..vf31, ACC, Q and CLIP. These functions write the
 * registers the original writes (vf1, vf2, vf24..vf31, ACC, Q, CLIP) and read
 * vf23 as their caller left it. The callees' own VU0 temporaries (001026A0:
 * vf4..vf9 and ACC; 00102900: vf4..vf6; 00102990: vf4, vf5) are not written
 * back: nothing here or after reads them before reloading. EE arithmetic
 * goes through em_ee_float.h; the clip test as em_area00_hud's (DAZ'd
 * magnitudes; an exponent-255 lane faults UNMEASURED).
 *
 * Callees run here: 001D7200, 001CF320, 001CD370 (an address), 001026A0
 * (em_effect_original), 00102900 (em_sdk_vu0), 00102990 (ftoi0 of four
 * lanes), 001281C0 (em_player_float_to_int). Hooks: 0011E748 (sqrt),
 * 001CB5F0 and 001CB900 (the packet chain).
 *
 * Fail-stop: an unmapped address, a NULL or failing hook, or an unmeasured
 * float form latches the first fault; every entry then returns -1. */
#ifndef EM_FAR_LIGHT_H
#define EM_FAR_LIGHT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    EM_FAR_LIGHT_FAULT_NONE = 0,
    EM_FAR_LIGHT_FAULT_UNMAPPED = 1,
    EM_FAR_LIGHT_FAULT_WORKER = 2,
    EM_FAR_LIGHT_FAULT_UNMEASURED = 3
};

typedef struct {
    uint32_t vf[32][4];
    uint32_t acc[4];
    uint32_t q;
    uint32_t clip;
} EmFarLightVu;

typedef struct {
    void *ctx;
    /* host bytes of [address, address + size), or NULL (a fault) */
    uint8_t *(*map)(void *ctx, uint32_t address, uint32_t size, int write);
    EmFarLightVu *vu;
    int (*w_0011E748)(void *ctx, uint32_t x, uint32_t *result);
    int (*w_001CB5F0)(void *ctx, uint32_t table, int32_t id, int32_t count, uint32_t *packet);
    int (*w_001CB900)(void *ctx, uint32_t table, int32_t id, int32_t mode);
    int32_t fault;
    uint32_t fault_function, fault_address;
} EmFarLight;

int em_far_light_001D73A0(EmFarLight *f, int32_t a0, int32_t *v0);
int em_far_light_001D7200(EmFarLight *f, int32_t x, int32_t y, int32_t *v0);
int em_far_light_001D72D0(EmFarLight *f, const uint32_t pos[4], int32_t *v0);
/* w, h: raw float bits (f12, f13); *v0 the returned address or 0 */
int em_far_light_001D75E0(EmFarLight *f, int32_t idx, const uint32_t pos[4], uint64_t tex0,
                          uint32_t rgba, uint32_t w, uint32_t h, uint32_t *v0);
int em_far_light_001CEFD0(EmFarLight *f, const uint32_t pos[4], const uint32_t rgba[4]);
int em_far_light_001CF320(EmFarLight *f, uint32_t xy, uint32_t rgba, uint64_t tex0, uint32_t size);

/* The two VU0 sequences these functions share with 001E17E0 (em_grid_water),
 * vf1 loaded by the caller. 0, or -1 for an unmeasured float form.
 *   clip:    vf2 = vf24 x + vf25 y + vf26 z + vf27 (implicit w 1), then
 *            CLIP = (CLIP << 6 | the x, y, z flags against |w|) & 0xFFFFFF.
 *   project: the projection described at 001D72D0 (vf28..vf31, Q, the fog
 *            lane from vf23, ftoi4); clip = vf2 before the scale, screen =
 *            the final vf2. */
int em_far_light_vu_clip(EmFarLightVu *v);
int em_far_light_vu_project(EmFarLightVu *v, uint32_t clip[4], uint32_t screen[4]);

#ifdef __cplusplus
}
#endif

#endif
